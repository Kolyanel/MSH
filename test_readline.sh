#!/usr/bin/env bash

set -u
set -o pipefail

###################################################### #########################
# MSH READLINE AUTOMATED TEST
#
# Checks readline result only:
#
#   READLINE_RESULT:<line>
#
# readline rendering is not tested here.
###############################################################################

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

MSH_BIN="${MSH_BIN:-$SCRIPT_DIR/msh}"

LOG_ROOT="${LOG_ROOT:-/root/Coding/shell/test_readline_logs}"

RUN_ID="$(date '+run_%Y%m%d_%H%M%S')"

RUN_DIR="$LOG_ROOT/$RUN_ID"


die()
{
	printf 'ERROR: %s\n' "$*" >&2
	exit 1
}


mkdir -p "$RUN_DIR" ||
	die "cannot create log directory"


[ -f "$MSH_BIN" ] ||
	die "msh not found: $MSH_BIN"


[ -x "$MSH_BIN" ] ||
	die "msh not executable: $MSH_BIN"


command -v python3 >/dev/null 2>&1 ||
	die "python3 missing"


echo "============================================================"
echo " MSH READLINE TEST"
echo "============================================================"
echo
echo "Binary : $MSH_BIN"
echo "Logs   : $RUN_DIR"
echo


cat > "$RUN_DIR/run_test.py" <<'PYTHON'

#!/usr/bin/env python3

import os
import pty
import select
import sys
import time


BINARY = sys.argv[1]
LOGDIR = sys.argv[2]


TESTS = [

	(
		"ascii",
		[
			b"abc",
			b"\r",
		],
		"abc",
	),

	(
		"backspace",
		[
			b"abc",
			b"\x7f",
			b"\r",
		],
		"ab",
	),

	(
		"left_insert",
		[
			b"abc",
			b"\x1b[D",
			b"X",
			b"\r",
		],
		"abXc",
	),

	(
		"home",
		[
			b"abc",
			b"\x1b[H",
			b"X",
			b"\r",
		],
		"Xabc",
	),

	(
		"end",
		[
			b"abc",
			b"\x1b[H",
			b"X",
			b"\x1b[F",
			b"Y",
			b"\r",
		],
		"XabcY",
	),

		(
		"delete",
		[
			b"abc",
			b"\x1b[D",
			b"\x1b[D",
			b"\x1b[3~",
			b"\r",
		],
		"ac",
	),

	(
		"utf8",
		[
			"привет".encode("utf-8"),
			b"\r",
		],
		"привет",
	),

	(
		"utf8_backspace",
		[
			"привет".encode("utf-8"),
			b"\x7f",
			b"\r",
		],
		"приве",
	),

	(
		"utf8_insert",
		[
			"привет".encode("utf-8"),
			b"\x1b[D",
			b"X",
			b"\r",
		],
		"привеXт",
	),

]


def run_one(name, actions, expected):


	pid, fd = pty.fork()


	if pid == 0:

		env = os.environ.copy()

		env["TERM"] = "xterm-256color"

		env["MSH_READLINE_TEST"] = "1"


		os.execve(
			BINARY,
			[BINARY],
			env
		)



	output = bytearray()


	time.sleep(0.2)


	for action in actions:

		os.write(
			fd,
			action
		)

		time.sleep(0.15)



	end = time.time() + 1.0


	while time.time() < end:

		r, _, _ = select.select(
			[fd],
			[],
			[],
			0.1
		)

		if not r:
			continue


		try:

			data = os.read(
				fd,
				4096
			)

			output.extend(data)


		except OSError:

			break



	raw_file = os.path.join(
		LOGDIR,
		name + ".raw"
	)


	with open(raw_file, "wb") as f:

		f.write(output)



	text = output.decode(
		"utf-8",
		errors="replace"
	)


	result = None


	for line in text.splitlines():

		if line.startswith(
			"READLINE_RESULT:"
		):

			result = line[
				len("READLINE_RESULT:"):
			]



	ok = result == expected



	print(
		"{:<20} {}".format(
			name,
			"PASS" if ok else "FAIL"
		)
	)


	if not ok:

		print(
			"  expected:",
			repr(expected)
		)

		print(
			"  actual  :",
			repr(result)
		)



	return ok



def main():

	total = 0
	passed = 0


	for name, actions, expected in TESTS:

		total += 1

		if run_one(
			name,
			actions,
			expected
		):

			passed += 1



	print()

	print(
		"TOTAL:",
		total
	)

	print(
		"PASS :",
		passed
	)

	print(
		"FAIL :",
		total - passed
	)



	return 0 if passed == total else 1



if __name__ == "__main__":

	sys.exit(
		main()
	)

PYTHON


chmod +x "$RUN_DIR/run_test.py"


python3 \
	"$RUN_DIR/run_test.py" \
	"$MSH_BIN" \
	"$RUN_DIR"


echo
echo "Logs:"
echo "$RUN_DIR"