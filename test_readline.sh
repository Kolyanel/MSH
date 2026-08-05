#!/usr/bin/env bash

set -u
set -o pipefail

###############################################################################
# MSH READLINE AUTOMATED TEST
#
# RUN LOCATION:
#     ~/Coding_C/test_readline.sh
#
# EXECUTABLE:
#     ~/Coding_C/msh
#
# PROJECT / LOGS:
#     ~/Coding/shell/test_readline_logs
#
# IMPORTANT:
#     This script DOES NOT BUILD msh.
###############################################################################

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

MSH_BIN="${MSH_BIN:-$SCRIPT_DIR/msh}"

ANDROID_PROJECT="${ANDROID_PROJECT:-/root/Coding/shell}"
LOG_ROOT="$ANDROID_PROJECT/test_readline_logs"

RUN_ID="$(date '+run_%Y%m%d_%H%M%S')"
RUN_DIR="$LOG_ROOT/$RUN_ID"

PTY_TEST="$RUN_DIR/pty_test.py"

###############################################################################
# Helpers
###############################################################################

die()
{
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

log()
{
    printf '%s\n' "$*"
}

###############################################################################
# Environment
###############################################################################

log "============================================================"
log " MSH READLINE AUTOMATED TEST"
log "============================================================"
log
log "Runner  : $SCRIPT_DIR/test_readline.sh"
log "Binary  : $MSH_BIN"
log "Project : $ANDROID_PROJECT"
log "Logs    : $RUN_DIR"
log

[ -f "$MSH_BIN" ] ||
    die "MSH binary not found: $MSH_BIN"

[ -x "$MSH_BIN" ] ||
    die "MSH binary is not executable: $MSH_BIN"

[ -d "$ANDROID_PROJECT" ] ||
    die "Android project directory not found: $ANDROID_PROJECT"

command -v python3 >/dev/null 2>&1 ||
    die "python3 is required"

mkdir -p "$RUN_DIR" ||
    die "cannot create log directory: $RUN_DIR"

###############################################################################
# Binary information
#
# NO BUILD.
###############################################################################

{
    echo "============================================================"
    echo "MSH BINARY INFORMATION"
    echo "============================================================"
    echo

    echo "Binary:"
    echo "$MSH_BIN"
    echo

    echo "file:"
    file "$MSH_BIN"
    echo

    echo "Size:"
    stat -c '%s bytes' "$MSH_BIN" 2>/dev/null || true
    echo

    echo "Permissions:"
    stat -c '%A %U:%G' "$MSH_BIN" 2>/dev/null || true
    echo

    echo "Timestamp:"
    stat -c '%y' "$MSH_BIN" 2>/dev/null || true
    echo

    echo "Build:"
    echo "NOT PERFORMED"
    echo

    echo "The existing executable is tested as-is."
} > "$RUN_DIR/binary_info.txt"

{
    echo "MSH readline automated test"
    echo
    echo "NO BUILD WAS PERFORMED."
    echo
    echo "The test does not execute:"
    echo "  cmake"
    echo "  make"
    echo "  ninja"
    echo "  gcc"
    echo "  clang"
    echo
    echo "Existing executable:"
    echo "$MSH_BIN"
} > "$RUN_DIR/build.log"

###############################################################################
# Generate PTY tester
###############################################################################

cat > "$PTY_TEST" <<'PYTHON'
#!/usr/bin/env python3

import argparse
import errno
import json
import os
import pty
import re
import select
import signal
import struct
import subprocess
import sys
import termios
import time
from dataclasses import dataclass, asdict


###############################################################################
# Terminal constants
###############################################################################

ROWS = 12
COLS = 29

START_TIMEOUT = 1.0
OUTPUT_TIMEOUT = 1.5
QUIET_TIME = 0.08


ENTER = b"\r"
BACKSPACE = b"\x7f"

CTRL_C = b"\x03"
CTRL_D = b"\x04"

LEFT = b"\x1b[D"
RIGHT = b"\x1b[C"
UP = b"\x1b[A"
DOWN = b"\x1b[B"

HOME = b"\x1b[H"
END = b"\x1b[F"

DELETE = b"\x1b[3~"


###############################################################################
# Result structures
###############################################################################

@dataclass
class Event:
    test: str
    action: str
    sent_hex: str
    sent_text: str
    output_before_hex: str
    output_after_hex: str
    timestamp: float


@dataclass
class TestResult:
    name: str
    expected: str
    actual: str
    passed: bool
    elapsed: float
    output_bytes: int
    error: str = ""


###############################################################################
# PTY session
###############################################################################

class PTYShell:

    def __init__(self, binary, rows=ROWS, cols=COLS):
        self.binary = binary
        self.rows = rows
        self.cols = cols

        self.pid = None
        self.fd = None

        self.raw = bytearray()
        self.events = []

    ###########################################################################
    # Start
    ###########################################################################

    def start(self):
        pid, fd = pty.fork()

        if pid == 0:
            env = os.environ.copy()

            env["TERM"] = "xterm-256color"
            env["COLUMNS"] = str(self.cols)
            env["LINES"] = str(self.rows)

            # Keep locale UTF-8 if available.
            env.setdefault("LANG", "C.UTF-8")
            env.setdefault("LC_ALL", "C.UTF-8")

            os.execve(
                self.binary,
                [self.binary],
                env,
            )

        self.pid = pid
        self.fd = fd

        self.set_winsize()

        self.read_until_quiet(START_TIMEOUT)

    ###########################################################################
    # Window size
    ###########################################################################

    def set_winsize(self):
        winsize = struct.pack(
            "HHHH",
            self.rows,
            self.cols,
            0,
            0,
        )

        try:
            import fcntl

            fcntl.ioctl(
                self.fd,
                termios.TIOCSWINSZ,
                winsize,
            )
        except Exception:
            pass

    ###########################################################################
    # Read
    ###########################################################################

    def read_available(self, duration=0.05):
        data = bytearray()

        deadline = time.monotonic() + duration

        while time.monotonic() < deadline:
            remaining = deadline - time.monotonic()

            if remaining <= 0:
                break

            try:
                ready, _, _ = select.select(
                    [self.fd],
                    [],
                    [],
                    min(0.02, remaining),
                )
            except (OSError, ValueError):
                break

            if not ready:
                continue

            try:
                chunk = os.read(self.fd, 8192)
            except OSError as exc:
                if exc.errno in (errno.EIO, errno.EBADF):
                    break
                raise

            if not chunk:
                break

            data.extend(chunk)
            self.raw.extend(chunk)

        return bytes(data)

    ###########################################################################
    # Read until quiet
    ###########################################################################

    def read_until_quiet(self, timeout=1.0):
        data = bytearray()

        deadline = time.monotonic() + timeout
        last_data = time.monotonic()

        while time.monotonic() < deadline:

            chunk = self.read_available(0.03)

            if chunk:
                data.extend(chunk)
                last_data = time.monotonic()

            if time.monotonic() - last_data >= QUIET_TIME:
                break

        return bytes(data)

    ###########################################################################
    # Send
    ###########################################################################

    def send(self, payload, action):
        before = bytes(self.raw)

        os.write(self.fd, payload)

        after = self.read_until_quiet(OUTPUT_TIMEOUT)

        self.events.append(
            Event(
                test="",
                action=action,
                sent_hex=payload.hex(),
                sent_text=payload.decode(
                    "utf-8",
                    errors="replace",
                ),
                output_before_hex=before.hex(),
                output_after_hex=after.hex(),
                timestamp=time.time(),
            )
        )

        return after

    ###########################################################################
    # Finish
    ###########################################################################

    def finish(self):
        if self.fd is not None:
            try:
                os.close(self.fd)
            except OSError:
                pass

            self.fd = None

        if self.pid is not None:
            try:
                os.waitpid(self.pid, os.WNOHANG)
            except ChildProcessError:
                pass

            self.pid = None


###############################################################################
# ANSI / terminal output helpers
###############################################################################

ANSI_RE = re.compile(
    rb"\x1b(?:"
    rb"\[[0-?]*[ -/]*[@-~]"
    rb"|"
    rb"\][^\x07]*(?:\x07|\x1b\\)"
    rb"|"
    rb"[@-_]"
    rb")"
)


def strip_ansi(data):
    return ANSI_RE.sub(b"", data)


def normalize_terminal_output(data):
    data = strip_ansi(data)

    data = data.replace(b"\r\n", b"\n")
    data = data.replace(b"\r", b"\n")

    return data


def decode_output(data):
    return normalize_terminal_output(data).decode(
        "utf-8",
        errors="replace",
    )


###############################################################################
# Extract accepted command
###############################################################################

def extract_commands(data):
    """
    The readline renderer emits a lot of terminal control traffic.

    After stripping ANSI, look for lines that resemble shell input.
    """

    text = decode_output(data)

    lines = []

    for line in text.splitlines():
        line = line.strip()

        if not line:
            continue

        lines.append(line)

    return lines


###############################################################################
# Test cases
###############################################################################

TESTS = [

    {
        "name": "plain_ascii",
        "actions": [
            ("type", b"abc"),
            ("enter", ENTER),
        ],
        "expected": "abc",
    },

    {
        "name": "backspace",
        "actions": [
            ("type", b"abc"),
            ("backspace", BACKSPACE),
            ("enter", ENTER),
        ],
        "expected": "ab",
    },

    {
        "name": "cursor_left",
        "actions": [
            ("type", b"abc"),
            ("left", LEFT),
            ("type", b"X"),
            ("enter", ENTER),
        ],
        "expected": "abXc",
    },

    {
        "name": "cursor_right",
        "actions": [
            ("type", b"abc"),
            ("left", LEFT),
            ("right", RIGHT),
            ("type", b"X"),
            ("enter", ENTER),
        ],
        "expected": "abcX",
    },

    {
        "name": "home",
        "actions": [
            ("type", b"abc"),
            ("home", HOME),
            ("type", b"X"),
            ("enter", ENTER),
        ],
        "expected": "Xabc",
    },

    {
        "name": "end",
        "actions": [
            ("type", b"abc"),
            ("home", HOME),
            ("type", b"X"),
            ("end", END),
            ("type", b"Y"),
            ("enter", ENTER),
        ],
        "expected": "XabcY",
    },

    {
        "name": "delete",
        "actions": [
            ("type", b"abc"),
            ("left", LEFT),
            ("delete", DELETE),
            ("enter", ENTER),
        ],
        "expected": "ac",
    },

    {
        "name": "utf8",
        "actions": [
            ("type", "привет".encode("utf-8")),
            ("enter", ENTER),
        ],
        "expected": "привет",
    },

    {
        "name": "utf8_backspace",
        "actions": [
            ("type", "привет".encode("utf-8")),
            ("backspace", BACKSPACE),
            ("enter", ENTER),
        ],
        "expected": "приве",
    },

    {
        "name": "utf8_cursor_left",
        "actions": [
            ("type", "привет".encode("utf-8")),
            ("left", LEFT),
            ("type", "X"),
            ("enter", ENTER),
        ],
        "expected": "привеXт",
    },

    {
        "name": "utf8_delete",
        "actions": [
            ("type", "привет".encode("utf-8")),
            ("left", LEFT),
            ("delete", DELETE),
            ("enter", ENTER),
        ],
        "expected": "приве",
    },

    {
        "name": "utf8_home",
        "actions": [
            ("type", "привет".encode("utf-8")),
            ("home", HOME),
            ("type", "X"),
            ("enter", ENTER),
        ],
        "expected": "Xпривет",
    },

    {
        "name": "utf8_end",
        "actions": [
            ("type", "привет".encode("utf-8")),
            ("home", HOME),
            ("type", "X"),
            ("end", END),
            ("type", "Y"),
            ("enter", ENTER),
        ],
        "expected": "XприветY",
    },

    {
        "name": "mixed_utf8",
        "actions": [
            ("type", "abc привет 123".encode("utf-8")),
            ("enter", ENTER),
        ],
        "expected": "abc привет 123",
    },

]


###############################################################################
# Run one test
###############################################################################

def run_test(binary, test):
    session = PTYShell(binary)

    start = time.monotonic()

    try:
        session.start()

        for action, payload in test["actions"]:
            output = session.send(
                payload,
                action,
            )

            session.events[-1].test = test["name"]

        # Give the shell a short period to process Enter and print
        # the next prompt / result.
        final_output = session.read_until_quiet(0.5)

        elapsed = time.monotonic() - start

        raw = bytes(session.raw)

        # Store raw PTY data inside the result.
        test_output = raw

        lines = extract_commands(test_output)

        expected = test["expected"]

        #
        # We cannot blindly compare every terminal line because msh
        # prints its prompt and redraw sequences.
        #
        # Instead search for the expected command as a complete
        # textual fragment.
        #
        normalized = decode_output(test_output)

        passed = False

        if expected in normalized:
            passed = True

        #
        # Stronger check:
        # for UTF-8 tests the expected text must survive intact.
        #
        if any(ord(c) > 127 for c in expected):
            if expected not in normalized:
                passed = False

        #
        # Basic corruption checks.
        #
        if "\ufffd" in normalized:
            passed = False

        error = ""

        if not passed:
            error = (
                "expected command not found in terminal output"
            )

        return TestResult(
            name=test["name"],
            expected=expected,
            actual=normalized,
            passed=passed,
            elapsed=elapsed,
            output_bytes=len(test_output),
            error=error,
        ), session

    except Exception as exc:
        elapsed = time.monotonic() - start

        return TestResult(
            name=test["name"],
            expected=test["expected"],
            actual="",
            passed=False,
            elapsed=elapsed,
            output_bytes=len(session.raw),
            error=repr(exc),
        ), session

    finally:
        session.finish()


###############################################################################
# Main
###############################################################################

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "binary",
        help="path to existing msh executable",
    )

    parser.add_argument(
        "outdir",
        help="test output directory",
    )

    args = parser.parse_args()

    os.makedirs(args.outdir, exist_ok=True)

    results = []
    all_events = []

    raw_path = os.path.join(
        args.outdir,
        "pty_raw.log",
    )

    event_path = os.path.join(
        args.outdir,
        "pty_events.txt",
    )

    report_path = os.path.join(
        args.outdir,
        "pty_report.txt",
    )

    print("MSH READLINE PTY TEST")
    print("=" * 70)
    print()
    print(f"Binary : {args.binary}")
    print(f"Rows   : {ROWS}")
    print(f"Cols   : {COLS}")
    print()

    for test in TESTS:

        result, session = run_test(
            args.binary,
            test,
        )

        results.append(result)
        all_events.extend(session.events)

        #
        # One raw binary file per test.
        #
        safe_name = test["name"] + ".bin"

        with open(
            os.path.join(args.outdir, safe_name),
            "wb",
        ) as f:
            f.write(bytes(session.raw))

        status = "PASS" if result.passed else "FAIL"

        print(
            f"{status:<6} "
            f"{result.name:<22} "
            f"{result.elapsed:7.3f}s "
            f"{result.output_bytes:6d} bytes"
        )

    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed

    ###########################################################################
    # results.json
    ###########################################################################

    with open(
        os.path.join(args.outdir, "results.json"),
        "w",
        encoding="utf-8",
    ) as f:
        json.dump(
            [asdict(r) for r in results],
            f,
            indent=2,
            ensure_ascii=False,
        )

    ###########################################################################
    # pty_raw.log
    ###########################################################################

    with open(
        raw_path,
        "w",
        encoding="utf-8",
        errors="replace",
    ) as f:

        for test in TESTS:

            path = os.path.join(
                args.outdir,
                test["name"] + ".bin",
            )

            try:
                with open(path, "rb") as inp:
                    data = inp.read()
            except OSError:
                continue

            f.write(
                "\n"
                + "=" * 80
                + "\n"
                + f"TEST: {test['name']}\n"
                + "=" * 80
                + "\n"
            )

            f.write(
                "HEX:\n"
                + data.hex()
                + "\n\n"
            )

            f.write(
                "TEXT:\n"
                + data.decode(
                    "utf-8",
                    errors="replace",
                )
                + "\n"
            )

    ###########################################################################
    # pty_events.txt
    ###########################################################################

    with open(
        event_path,
        "w",
        encoding="utf-8",
    ) as f:

        for ev in all_events:
            f.write(
                "=" * 80
                + "\n"
            )

            f.write(
                f"TEST       : {ev.test}\n"
                f"ACTION     : {ev.action}\n"
                f"SENT HEX   : {ev.sent_hex}\n"
                f"SENT TEXT  : {ev.sent_text!r}\n"
                f"TIME       : {ev.timestamp:.6f}\n"
                f"BEFORE HEX : {ev.output_before_hex}\n"
                f"AFTER HEX  : {ev.output_after_hex}\n"
            )

    ###########################################################################
    # pty_report.txt
    ###########################################################################

    with open(
        report_path,
        "w",
        encoding="utf-8",
    ) as f:

        f.write(
            "============================================================\n"
            "MSH READLINE AUTOMATED TEST REPORT\n"
            "============================================================\n\n"
        )

        f.write(
            f"Binary : {args.binary}\n"
            f"Rows   : {ROWS}\n"
            f"Cols   : {COLS}\n\n"
        )

        f.write(
            f"TOTAL  : {len(results)}\n"
            f"PASS   : {passed}\n"
            f"FAIL   : {failed}\n\n"
        )

        for result in results:

            status = "PASS" if result.passed else "FAIL"

            f.write(
                "-" * 70
                + "\n"
            )

            f.write(
                f"{status}: {result.name}\n"
                f"Expected : {result.expected!r}\n"
                f"Actual   : {result.actual!r}\n"
                f"Elapsed  : {result.elapsed:.3f}s\n"
                f"Output   : {result.output_bytes} bytes\n"
            )

            if result.error:
                f.write(
                    f"Error    : {result.error}\n"
                )

            f.write("\n")

        f.write(
            "=" * 70
            + "\n"
        )

        f.write(
            "FINAL RESULT: "
            + ("PASS" if failed == 0 else "FAIL")
            + "\n"
        )

    ###########################################################################
    # Console summary
    ###########################################################################

    print()
    print("=" * 70)
    print(
        f"TOTAL: {len(results)}  "
        f"PASS: {passed}  "
        f"FAIL: {failed}"
    )
    print("=" * 70)

    if failed == 0:
        print("RESULT: PASS")
        return 0

    print("RESULT: FAIL")
    print()
    print(f"Report: {report_path}")

    return 1


if __name__ == "__main__":
    sys.exit(main())

PYTHON

chmod +x "$PTY_TEST"

###############################################################################
# Run test
###############################################################################

log
log "Starting PTY readline tests..."
log

TEST_EXIT=0

python3 "$PTY_TEST" \
    "$MSH_BIN" \
    "$RUN_DIR" \
    2>&1 | tee "$RUN_DIR/main.log" || TEST_EXIT=${PIPESTATUS[0]}

###############################################################################
# Final metadata
###############################################################################

{
    echo "============================================================"
    echo "TEST RUN"
    echo "============================================================"
    echo
    echo "Started : $RUN_ID"
    echo "Binary  : $MSH_BIN"
    echo "Logs    : $RUN_DIR"
    echo
    echo "Build   : NOT PERFORMED"
    echo
    echo "Exit code:"
    echo "$TEST_EXIT"
    echo
    if [ "$TEST_EXIT" -eq 0 ]; then
        echo "RESULT: PASS"
    else
        echo "RESULT: FAIL"
    fi
} > "$RUN_DIR/test_summary.txt"

###############################################################################
# Finish
###############################################################################

log
log "============================================================"

if [ "$TEST_EXIT" -eq 0 ]; then
    log " READLINE TEST: PASS"
else
    log " READLINE TEST: FAIL"
fi

log " Logs: $RUN_DIR"
log "============================================================"

exit "$TEST_EXIT"