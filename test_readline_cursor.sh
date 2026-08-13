#!/usr/bin/env bash

set -u
set -o pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
MSH_BIN="${MSH_BIN:-$SCRIPT_DIR/msh}"

LOG_ROOT="${LOG_ROOT:-/root/Coding/shell/test_readline_logs}"

RUN_ID="$(date '+run_%Y%m%d_%H%M%S')"
RUN_DIR="$LOG_ROOT/cursor_$RUN_ID"


die()
{
        printf 'ERROR: %s\n' "$*" >&2
        exit 1
}


mkdir -p "$RUN_DIR" ||
        die "cannot create log dir"


[ -x "$MSH_BIN" ] ||
        die "msh not executable"


echo "============================================================"
echo " MSH READLINE CURSOR TEST"
echo "============================================================"
echo
echo "Binary : $MSH_BIN"
echo "Logs   : $RUN_DIR"
echo


cat > "$RUN_DIR/cursor_test.py" <<'PYTHON'
#!/usr/bin/env python3

import os
import pty
import sys
import time
import select
import struct
import termios
import fcntl
import signal


BINARY = sys.argv[1]
LOGDIR = sys.argv[2]


# ------------------------------------------------------------
# Test terminal geometry
# ------------------------------------------------------------

ROWS = 5
COLS = 10


# ------------------------------------------------------------
# Expected cursor position after prompt
#
# Prompt visual width:
#
#     msh: ~/Coding_C: >>
#
# = 20 columns
#
# Terminal width:
#
#     10 columns
#
# Therefore:
#
#     20 / 10 = 2 rows
#     20 % 10 = 0 columns
#
# readline origin:
#
#     zero-based: (2, 0)
#
# Terminal DSR response:
#
#     one-based: ESC [ 3 ; 1 R
# ------------------------------------------------------------

EXPECTED_ORIGIN = (2, 0)

DSR_REQUEST = b"\x1b[6n"
DSR_RESPONSE = b"\x1b[3;1R"


# ------------------------------------------------------------
# Calculate expected readline-relative final cursor.
#
# This follows the same terminal model currently used by
# rl_layout.c:
#
#     - one input character occupies one logical column
#     - when COLS is reached, position becomes next row/0
#
# This is deliberately based on Unicode characters rather
# than UTF-8 byte length.
# ------------------------------------------------------------

def expected_final_cursor(text):

    row = 0
    col = 0

    for ch in text.decode("utf-8"):

        #
        # Current readline UTF-8 width model used by the
        # cursor/layout tests treats these characters as
        # one display column.
        #
        width = 1

        #
        # A character that does not fit starts on the next
        # physical row.
        #
        if width > COLS - col:

            row += 1
            col = 0

        col += width

        #
        # Once the last column is occupied, readline's model
        # explicitly moves to the next physical row.
        #
        if col >= COLS:

            row += 1
            col = 0

    return (row, col)


# ------------------------------------------------------------
# PTY size
# ------------------------------------------------------------

def set_size(fd):

    size = struct.pack(
        "HHHH",
        ROWS,
        COLS,
        0,
        0
    )

    fcntl.ioctl(
        fd,
        termios.TIOCSWINSZ,
        size
    )


# ------------------------------------------------------------
# Send DSR response
# ------------------------------------------------------------

def send_dsr_response(fd):

    os.write(
        fd,
        DSR_RESPONSE
    )


# ------------------------------------------------------------
# Count DSR requests in output
# ------------------------------------------------------------

def count_dsr_requests(data):

    count = 0
    pos = 0

    while True:

        pos = data.find(
            DSR_REQUEST,
            pos
        )

        if pos < 0:
            break

        count += 1
        pos += len(DSR_REQUEST)

    return count


# ------------------------------------------------------------
# Read one chunk from PTY
# ------------------------------------------------------------

def read_chunk(fd):

    try:

        return os.read(
            fd,
            4096
        )

    except OSError:

        return b""


# ------------------------------------------------------------
# Extract CURSOR_DEBUG lines
# ------------------------------------------------------------

def extract_debug(data):

    text = data.decode(
        "utf-8",
        errors="replace"
    )

    return [
        line
        for line in text.splitlines()
        if "CURSOR_DEBUG:" in line
    ]


# ------------------------------------------------------------
# Parse the last CURSOR_DEBUG cursor position.
# ------------------------------------------------------------

def parse_last_cursor(debug_lines):

    for line in reversed(debug_lines):

        marker = "cursor=("

        pos = line.find(marker)

        if pos < 0:
            continue

        start = pos + len(marker)

        end = line.find(
            ")",
            start
        )

        if end < 0:
            continue

        value = line[start:end]

        try:

            row, col = value.split(",")

            return (
                int(row),
                int(col)
            )

        except ValueError:

            continue

    return None


# ------------------------------------------------------------
# Parse the last CURSOR_DEBUG origin.
# ------------------------------------------------------------

def parse_last_origin(debug_lines):

    for line in reversed(debug_lines):

        marker = "origin=("

        pos = line.find(marker)

        if pos < 0:
            continue

        start = pos + len(marker)

        end = line.find(
            ")",
            start
        )

        if end < 0:
            continue

        value = line[start:end]

        try:

            row, col = value.split(",")

            return (
                int(row),
                int(col)
            )

        except ValueError:

            continue

    return None


# ------------------------------------------------------------
# Read PTY until a DSR request appears.
# ------------------------------------------------------------

def wait_for_first_dsr(fd, output, timeout):

    deadline = time.time() + timeout

    while time.time() < deadline:

        remaining = deadline - time.time()

        if remaining <= 0:
            break

        r, _, _ = select.select(
            [fd],
            [],
            [],
            min(0.05, remaining)
        )

        if not r:
            continue

        chunk = read_chunk(fd)

        if not chunk:
            break

        output.extend(chunk)

        if count_dsr_requests(output) > 0:
            return True

    return False


# ------------------------------------------------------------
# Service DSR requests.
#
# response_count tells us how many requests have already
# received a response.
# ------------------------------------------------------------

def service_dsr(fd, output, response_count):

    request_count = count_dsr_requests(output)

    while response_count < request_count:

        send_dsr_response(fd)

        response_count += 1

    return response_count


# ------------------------------------------------------------
# Read PTY for a limited amount of time while servicing DSR.
# ------------------------------------------------------------

def drain_terminal(fd, output, response_count, timeout):

    deadline = time.time() + timeout

    while time.time() < deadline:

        remaining = deadline - time.time()

        if remaining <= 0:
            break

        r, _, _ = select.select(
            [fd],
            [],
            [],
            min(0.05, remaining)
        )

        if not r:
            continue

        chunk = read_chunk(fd)

        if not chunk:
            break

        output.extend(chunk)

        response_count = service_dsr(
            fd,
            output,
            response_count
        )

    return response_count


# ------------------------------------------------------------
# Read whatever remains in PTY.
# ------------------------------------------------------------

def read_available(fd, output, timeout=0.3):

    deadline = time.time() + timeout

    while time.time() < deadline:

        remaining = deadline - time.time()

        if remaining <= 0:
            break

        r, _, _ = select.select(
            [fd],
            [],
            [],
            min(0.05, remaining)
        )

        if not r:
            continue

        chunk = read_chunk(fd)

        if not chunk:
            break

        output.extend(chunk)


# ------------------------------------------------------------
# Save raw log
# ------------------------------------------------------------

def save_raw(name, output):

    path = os.path.join(
        LOGDIR,
        name + ".raw"
    )

    with open(
        path,
        "wb"
    ) as f:

        f.write(output)

    return path


# ------------------------------------------------------------
# Run one test
# ------------------------------------------------------------

def run(name, text):

    pid, fd = pty.fork()

    if pid == 0:

        env = os.environ.copy()

        env["TERM"] = "xterm-256color"
        env["COLUMNS"] = str(COLS)
        env["LINES"] = str(ROWS)

        env["MSH_READLINE_TEST"] = "1"
        env["MSH_CURSOR_DEBUG"] = "1"

        time.sleep(0.2)

        os.execve(
            BINARY,
            [BINARY],
            env
        )

    # --------------------------------------------------------
    # Parent
    # --------------------------------------------------------

    set_size(fd)

    time.sleep(0.3)

    output = bytearray()

    # --------------------------------------------------------
    # Phase 1:
    # Wait for initial DSR.
    # --------------------------------------------------------

    if not wait_for_first_dsr(
        fd,
        output,
        2.0
    ):

        print(
            name,
            "ERROR: msh did not issue DSR"
        )

        path = save_raw(
            name,
            output
        )

        print(
            "log:",
            path
        )

        try:
            os.kill(
                pid,
                signal.SIGHUP
            )
        except OSError:
            pass

        try:
            os.close(fd)
        except OSError:
            pass

        return False


    # --------------------------------------------------------
    # Phase 2:
    # Respond to every DSR seen so far.
    # --------------------------------------------------------

    response_count = 0

    response_count = service_dsr(
        fd,
        output,
        response_count
    )


    # --------------------------------------------------------
    # Allow readline to consume the DSR response.
    # --------------------------------------------------------

    time.sleep(0.1)


    # --------------------------------------------------------
    # Phase 3:
    # Send test input.
    # --------------------------------------------------------

    os.write(
        fd,
        text
    )


    # --------------------------------------------------------
    # Phase 4:
    # Drain output and service any additional DSR.
    # --------------------------------------------------------

    response_count = drain_terminal(
        fd,
        output,
        response_count,
        1.0
    )


    # --------------------------------------------------------
    # Give terminal output a final moment to arrive.
    # --------------------------------------------------------

    read_available(
        fd,
        output,
        0.3
    )


    # --------------------------------------------------------
    # Save raw log.
    # --------------------------------------------------------

    path = save_raw(
        name,
        output
    )


    # --------------------------------------------------------
    # Diagnostics
    # --------------------------------------------------------

    debug_lines = extract_debug(
        output
    )

    request_count = count_dsr_requests(
        output
    )

    origin = parse_last_origin(
        debug_lines
    )

    final_cursor = parse_last_cursor(
        debug_lines
    )

    expected_cursor = expected_final_cursor(text)


    print(
        name,
        "bytes:",
        len(output)
    )

    print(
        "DSR requests:",
        request_count
    )

    print(
        "DSR responses:",
        response_count
    )

    print(
        "log:",
        path
    )

    print(
        "expected origin:",
        EXPECTED_ORIGIN
    )

    print(
        "actual origin:",
        origin
    )

    print(
        "expected final cursor:",
        expected_cursor
    )

    print(
        "actual final cursor:",
        final_cursor
    )


    # --------------------------------------------------------
    # Print the last diagnostics.
    # --------------------------------------------------------

    if debug_lines:

        print(
            "last CURSOR_DEBUG:"
        )

        for line in debug_lines[-8:]:

            print(
                "  " + line
            )

    else:

        print(
            "WARNING: no CURSOR_DEBUG lines"
        )


    # --------------------------------------------------------
    # Automatic checks
    # --------------------------------------------------------

    passed = True


    if request_count != response_count:

        print(
            "cursor check: FAIL "
            "(DSR request/response mismatch)"
        )

        passed = False


    if origin != EXPECTED_ORIGIN:

        print(
            "cursor check: FAIL "
            "(origin mismatch)"
        )

        passed = False


    if final_cursor != expected_cursor:

        print(
            "cursor check: FAIL "
            "(final cursor mismatch)"
        )

        passed = False


    if passed:

        print(
            "cursor check: PASS"
        )


    # --------------------------------------------------------
    # Terminate msh.
    # --------------------------------------------------------

    try:

        os.kill(
            pid,
            signal.SIGHUP
        )

    except OSError:

        pass


    try:

        os.close(fd)

    except OSError:

        pass


    return passed


# ------------------------------------------------------------
# Tests
# ------------------------------------------------------------

def main():

    tests = [

        (
            "ascii_cursor_wrap",
            b"abcdefghijklmno"
        ),

        (
            "utf8_cursor_wrap",
            "приветприветпривет".encode(
                "utf-8"
            )
        ),

        (
            "mixed_cursor_wrap",
            "abc привет abc привет".encode(
                "utf-8"
            )
        ),

    ]


    failed = 0


    for name, data in tests:

        print()

        print(
            "------------------------------------------------------------"
        )

        print(
            "TEST:",
            name
        )

        print(
            "------------------------------------------------------------"
        )

        if not run(
            name,
            data
        ):

            failed += 1


    print()

    print(
        "============================================================"
    )


    if failed:

        print(
            "RESULT: FAIL"
        )

        print(
            "Failed tests:",
            failed
        )

        print(
            "============================================================"
        )

        sys.exit(1)


    print(
        "RESULT: PASS"
    )

    print(
        "============================================================"
    )


if __name__ == "__main__":

    main()
PYTHON


chmod +x "$RUN_DIR/cursor_test.py"


python3 \
        "$RUN_DIR/cursor_test.py" \
        "$MSH_BIN" \
        "$RUN_DIR"


echo
echo "Raw logs:"

ls -1 "$RUN_DIR"/*.raw