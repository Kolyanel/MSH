#!/usr/bin/env bash

###############################################################################
# MSH READLINE DIAGNOSTIC TEST
#
# Запуск:
#     bash /root/Coding_C/test_readline.sh
#
# Скрипт находится:
#     /root/Coding_C/test_readline.sh
#
# Исходники:
#     /root/Coding/shell
#
# Бинарник:
#     /root/Coding_C/msh
#
# Логи:
#     /root/Coding/shell/test_readline_logs/
###############################################################################

set +e

###############################################################################
# 0. CONFIGURATION
###############################################################################

SCRIPT_DIR="/root/Coding_C"
PROJECT_DIR="/root/Coding/shell"
BINARY="/root/Coding_C/msh"

CORE_DIR="$PROJECT_DIR/msh/src/core"
INCLUDE_DIR="$PROJECT_DIR/msh/include"
MYLIB_DIR="$PROJECT_DIR/mylib"

LOG_ROOT="$PROJECT_DIR/test_readline_logs"
RUN_ID="$(date '+run_%Y%m%d_%H%M%S')"
LOG_DIR="$LOG_ROOT/$RUN_ID"

mkdir -p "$LOG_DIR"

PASS=0
FAIL=0
WARN=0

###############################################################################
# OUTPUT
###############################################################################

pass()
{
    PASS=$((PASS + 1))
    printf '[PASS] %s\n' "$1"
}

fail()
{
    FAIL=$((FAIL + 1))
    printf '[FAIL] %s\n' "$1"
}

warn()
{
    WARN=$((WARN + 1))
    printf '[WARN] %s\n' "$1"
}

info()
{
    printf '[INFO] %s\n' "$1"
}

section()
{
    printf '\n'
    printf '===============================================================================\n'
    printf '%s\n' "$1"
    printf '===============================================================================\n'
}

###############################################################################
# FILE HELPERS
###############################################################################

require_file()
{
    local file="$1"

    if [ -f "$file" ]; then
        pass "Файл существует: $file"
        return 0
    fi

    fail "ОТСУТСТВУЕТ файл: $file"
    return 1
}

###############################################################################
# 0. CONFIGURATION
###############################################################################

section "0. КОНФИГУРАЦИЯ ТЕСТА"

printf 'SCRIPT_DIR  = %s\n' "$SCRIPT_DIR"
printf 'PROJECT_DIR = %s\n' "$PROJECT_DIR"
printf 'BINARY      = %s\n' "$BINARY"
printf 'CORE_DIR    = %s\n' "$CORE_DIR"
printf 'LOG_DIR     = %s\n' "$LOG_DIR"

if [ -d "$PROJECT_DIR" ]; then
    pass "Каталог проекта найден"
else
    fail "Каталог проекта не найден: $PROJECT_DIR"
    exit 1
fi

if [ -x "$BINARY" ]; then
    pass "Бинарник найден и исполняемый"
else
    fail "Бинарник не найден или не исполняемый: $BINARY"
fi

###############################################################################
# 1. STRUCTURE
###############################################################################

section "1. СТРУКТУРА READLINE"

REQUIRED_FILES="
$INCLUDE_DIR/readline_internal.h
$CORE_DIR/readline.c
$CORE_DIR/rl_init.c
$CORE_DIR/rl_event.c
$CORE_DIR/rl_cursor.c
$CORE_DIR/rl_history.c
$CORE_DIR/rl_suggest.c
$CORE_DIR/rl_complete.c
$CORE_DIR/rl_layout.c
$CORE_DIR/rl_render.c
$CORE_DIR/editor.c
$CORE_DIR/input.c
$CORE_DIR/delete.c
$CORE_DIR/redraw.c
$CORE_DIR/terminal.c
$CORE_DIR/finish.c
$CORE_DIR/history.c
$CORE_DIR/prompt.c
"

for file in $REQUIRED_FILES; do
    require_file "$file"
done

###############################################################################
# 2. MYLIB / UTF8 / BUF
###############################################################################

section "2. ПРОВЕРКА MYLIB / UTF-8 / BUF"

MYLIB_FILES="
$MYLIB_DIR/include/buf.h
$MYLIB_DIR/include/utf8.h
$MYLIB_DIR/src/buf.c
$MYLIB_DIR/src/utf8.c
"

for file in $MYLIB_FILES; do
    require_file "$file"
done

###############################################################################
# 3. HEADER ANALYSIS
###############################################################################

section "3. АНАЛИЗ readline_internal.h"

HEADER_LOG="$LOG_DIR/header_analysis.txt"

{
    echo "FILE: $INCLUDE_DIR/readline_internal.h"
    echo
    echo "===== TYPES ====="
    grep -nE 'typedef|struct|enum' \
        "$INCLUDE_DIR/readline_internal.h" 2>/dev/null

    echo
    echo "===== t_rl ====="
    grep -n -A120 -B5 \
        'struct s_rl' "$INCLUDE_DIR/readline_internal.h" 2>/dev/null

    echo
    echo "===== FUNCTION DECLARATIONS ====="
    grep -nE '^[[:space:]]*[a-zA-Z_][a-zA-Z0-9_[:space:]\*]*[[:space:]]+[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(' \
        "$INCLUDE_DIR/readline_internal.h" 2>/dev/null
} > "$HEADER_LOG"

pass "Анализ заголовка сохранён: $HEADER_LOG"

###############################################################################
# 4. CORE FILE LIST
###############################################################################

section "4. ФАКТИЧЕСКИЕ CORE-ФАЙЛЫ"

CORE_FILES_LOG="$LOG_DIR/core_files.txt"

find "$CORE_DIR" \
    -maxdepth 1 \
    -type f \
    -name '*.c' \
    -printf '%f\n' 2>/dev/null |
    sort > "$CORE_FILES_LOG"

cat "$CORE_FILES_LOG"

info "Полный список сохранён: $CORE_FILES_LOG"

###############################################################################
# 5. FUNCTION INVENTORY
###############################################################################

section "5. ПОИСК ФУНКЦИЙ READLINE"

FUNCTION_LOG="$LOG_DIR/functions.txt"

{
    echo "===== FUNCTION-LIKE DEFINITIONS ====="
    echo

    for file in "$CORE_DIR"/*.c; do
        [ -f "$file" ] || continue

        awk '
        /^[[:space:]]*#/ {
            next
        }

        /^[[:space:]]*(static[[:space:]]+)?[A-Za-z_][A-Za-z0-9_[:space:]\*]*[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(/ {
            line=$0

            if (line ~ /^[[:space:]]*(if|for|while|switch)[[:space:]]*\(/)
                next

            if (line ~ /^[[:space:]]*(return|sizeof)[[:space:]]*/)
                next

            print FILENAME ":" FNR ":" line
        }
        ' "$file"
    done
} > "$FUNCTION_LOG"

cat "$FUNCTION_LOG"

pass "Список функций сохранён: $FUNCTION_LOG"

###############################################################################
# 6. DEBUG / PRINTF / ANSI SCAN
###############################################################################

section "6. ПОИСК DEBUG / printf / ANSI В CORE"

DEBUG_LOG="$LOG_DIR/debug_scan.txt"

{
    echo "===== printf / fprintf / puts / perror ====="
    grep -nRE \
        '\b(printf|fprintf|sprintf|snprintf|puts|perror)\s*\(' \
        "$CORE_DIR"/*.c 2>/dev/null

    echo
    echo "===== DEBUG ====="
    grep -nRE \
        'DEBUG|debug|TRACE|trace' \
        "$CORE_DIR"/*.c "$INCLUDE_DIR"/*.h 2>/dev/null

    echo
    echo "===== ANSI ESC ====="
    grep -nRE \
        '\\033|\\x1b|\033|\x1B|\033\[' \
        "$CORE_DIR"/*.c 2>/dev/null
} > "$DEBUG_LOG"

pass "Сканирование debug/ANSI сохранено: $DEBUG_LOG"

###############################################################################
# 7. TERMINAL / REDRAW STATIC ANALYSIS
###############################################################################

section "7. АНАЛИЗ TERMINAL / REDRAW"

TERMINAL_LOG="$LOG_DIR/terminal_analysis.txt"

{
    echo "===== terminal.c ====="
    grep -nE \
        'tcgetattr|tcsetattr|ioctl|TIOCGWINSZ|TIOCGWINSZ|TIOCGWINSZ|6n|write|read|cursor|clear|move' \
        "$CORE_DIR/terminal.c" 2>/dev/null

    echo
    echo "===== redraw.c ====="
    grep -nE \
        'write|printf|prompt|cursor|clear|move|suggest|line|rows|cols|ANSI|033|x1b' \
        "$CORE_DIR/redraw.c" 2>/dev/null

    echo
    echo "===== rl_render.c ====="
    grep -nE \
        'write|printf|prompt|cursor|clear|move|suggest|line|rows|cols|ANSI|033|x1b' \
        "$CORE_DIR/rl_render.c" 2>/dev/null

    echo
    echo "===== rl_layout.c ====="
    grep -nE \
        'row|col|cursor|prompt|suggest|line|width|visible|utf8' \
        "$CORE_DIR/rl_layout.c" 2>/dev/null
} > "$TERMINAL_LOG"

pass "Terminal/redraw анализ сохранён: $TERMINAL_LOG"

###############################################################################
# 8. KEY SYMBOL LOCATIONS
###############################################################################

section "8. ИНДЕКС КЛЮЧЕВЫХ ФУНКЦИЙ"

SYMBOL_LOG="$LOG_DIR/symbol_locations.txt"

{
    for symbol in \
        rl_redraw \
        rl_render \
        rl_layout \
        rl_insert \
        rl_delete \
        rl_backspace \
        rl_cursor_left \
        rl_cursor_right \
        rl_cursor_home \
        rl_cursor_end \
        rl_suggest \
        rl_clear_suggestion \
        rl_finish_line \
        rl_take_line \
        rl_restore \
        rl_get_cursor_position \
        rl_get_terminal_size
    do
        echo "===== $symbol ====="
        grep -nRE \
            "^[[:space:]]*(static[[:space:]]+)?[^;]*[[:space:]]+$symbol[[:space:]]*\(" \
            "$CORE_DIR"/*.c "$INCLUDE_DIR"/*.h 2>/dev/null
        echo
    done
} > "$SYMBOL_LOG"

pass "Расположение ключевых функций сохранено: $SYMBOL_LOG"

###############################################################################
# 9. DUPLICATE FUNCTION DETECTION
###############################################################################

section "9. ПРОВЕРКА ДУБЛИКАТОВ ФУНКЦИЙ"

DUP_LOG="$LOG_DIR/duplicate_functions.txt"

python3 - "$CORE_DIR" > "$DUP_LOG" 2>&1 <<'PY'
import os
import re
import sys
from collections import defaultdict

core = sys.argv[1]

pattern = re.compile(
    r'^\s*'
    r'(?:static\s+)?'
    r'(?:const\s+)?'
    r'[A-Za-z_][A-Za-z0-9_]*'
    r'(?:\s+|\s*\*\s*)+'
    r'([A-Za-z_][A-Za-z0-9_]*)'
    r'\s*\([^;]*\)\s*$'
)

defs = defaultdict(list)

for name in sorted(os.listdir(core)):
    if not name.endswith(".c"):
        continue

    path = os.path.join(core, name)

    try:
        lines = open(path, encoding="utf-8", errors="replace").readlines()
    except OSError:
        continue

    for lineno, line in enumerate(lines, 1):
        stripped = line.strip()

        if stripped.startswith(("#", "//", "/*", "*")):
            continue

        if re.match(r'^(if|for|while|switch|return)\s*\(', stripped):
            continue

        m = pattern.match(line.rstrip())

        if m:
            defs[m.group(1)].append(
                f"{name}:{lineno}"
            )

duplicates = {
    k: v for k, v in defs.items()
    if len(v) > 1
}

if not duplicates:
    print("NO_DUPLICATE_FUNCTION_DEFINITIONS")
    sys.exit(0)

for name, locations in sorted(duplicates.items()):
    print(f"{name}:")
    for location in locations:
        print(f"    {location}")

sys.exit(1)
PY

if grep -q '^NO_DUPLICATE_FUNCTION_DEFINITIONS$' "$DUP_LOG"; then
    pass "Дублирующих определений функций не найдено"
else
    fail "Найдены возможные дублирующие определения функций"
    cat "$DUP_LOG"
fi

###############################################################################
# 10. DECLARATIONS VS DEFINITIONS
###############################################################################

section "10. ПРОВЕРКА ОБЪЯВЛЕНИЙ И РЕАЛИЗАЦИЙ"

DECL_LOG="$LOG_DIR/declarations_vs_definitions.txt"

{
    echo "===== HEADER DECLARATIONS ====="

    grep -nRE \
        '^[[:space:]]*(static[[:space:]]+)?[A-Za-z_][A-Za-z0-9_[:space:]\*]*[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\([^;]*\)[[:space:]]*;' \
        "$INCLUDE_DIR/readline_internal.h" 2>/dev/null

    echo
    echo "===== CORE DEFINITIONS ====="

    grep -nRE \
        '^[[:space:]]*(static[[:space:]]+)?[A-Za-z_][A-Za-z0-9_[:space:]\*]*[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(' \
        "$CORE_DIR"/*.c 2>/dev/null
} > "$DECL_LOG"

pass "Сравнение объявлений/реализаций сохранено: $DECL_LOG"

: <<'BUILD_DISABLED'
###############################################################################
# 11. CMAKE BUILD
###############################################################################

section "11. ПОЛНАЯ СБОРКА CMAKE"

BUILD_LOG="$LOG_DIR/cmake_build.txt"

(
    cd "$PROJECT_DIR" || exit 1
    cmake --build build --clean-first
) > "$BUILD_LOG" 2>&1

BUILD_RC=$?

if [ "$BUILD_RC" -eq 0 ]; then
    pass "CMake build успешно завершён"
else
    fail "CMake build завершился с ошибкой: $BUILD_RC"
fi
BUILD_DISABLED

###############################################################################
# 12. BINARY
###############################################################################

section "12. ПРОВЕРКА БИНАРНИКА"

BINARY_LOG="$LOG_DIR/binary_info.txt"

if [ -f "$BINARY" ]; then
    pass "Бинарник существует: $BINARY"
else
    fail "Бинарник отсутствует: $BINARY"
fi

if [ -x "$BINARY" ]; then
    pass "Бинарник исполняемый"
else
    fail "Бинарник не имеет execute permission"
fi

{
    echo "===== FILE ====="
    file "$BINARY"

    echo
    echo "===== SIZE ====="
    ls -lh "$BINARY"

    echo
    echo "===== TIMESTAMP ====="
    stat "$BINARY"

    echo
    echo "===== READLINE SYMBOLS ====="
    nm -C "$BINARY" 2>/dev/null |
        grep -E \
        'rl_|readline|redraw|render|suggest|cursor|terminal'
} > "$BINARY_LOG" 2>&1

pass "Информация о бинарнике сохранена: $BINARY_LOG"

###############################################################################
# 13. PYTHON PTY TESTER
###############################################################################

section "13. СОЗДАНИЕ PTY-ТЕСТЕРА"

PTY_SCRIPT="$LOG_DIR/pty_test.py"

cat > "$PTY_SCRIPT" <<'PY'
#!/usr/bin/env python3

import os
import pty
import sys
import time
import select
import signal
import re

BINARY = sys.argv[1]
RAW_LOG = sys.argv[2]
REPORT = sys.argv[3]

PROMPT = b"msh:"
COMMAND = b"echo abc"

ESC = b"\x1b"

def read_available(fd, timeout=0.15):
    data = b""
    end = time.time() + timeout

    while time.time() < end:
        remaining = max(0, end - time.time())

        r, _, _ = select.select([fd], [], [], remaining)

        if not r:
            break

        try:
            chunk = os.read(fd, 8192)
        except OSError:
            break

        if not chunk:
            break

        data += chunk

    return data


def write(fd, data):
    os.write(fd, data)
    time.sleep(0.05)


def visible(data):
    text = data.decode("utf-8", errors="replace")

    text = re.sub(r"\x1b\[[0-?]*[ -/]*[@-~]", "", text)
    text = re.sub(r"\x1b[@-_]", "", text)

    return text


def count_exact(data, needle):
    return data.count(needle)


def ansi_sequences(data):
    return re.findall(
        rb"\x1b\[[0-?]*[ -/]*[@-~]",
        data
    )


def main():
    master, slave = pty.openpty()

    pid = os.fork()

    if pid == 0:
        os.setsid()

        os.dup2(slave, 0)
        os.dup2(slave, 1)
        os.dup2(slave, 2)

        os.close(master)
        os.close(slave)

        os.environ["TERM"] = "xterm-256color"
        os.environ["LC_ALL"] = "C.UTF-8"
        os.environ["LANG"] = "C.UTF-8"

        os.execv(BINARY, [BINARY])

    os.close(slave)

    raw = b""
    events = []

    def capture(label, delay=0.20):
        nonlocal raw

        data = read_available(master, delay)
        raw += data

        events.append((label, data))

        return data

    try:
        capture("initial", 1.0)

        write(master, b"echo abc")
        capture("after_echo_abc")

        write(master, b"\x7f")
        capture("after_backspace")

        write(master, b"c")
        capture("after_c")

        write(master, b"\x1b[D")
        capture("after_left")

        write(master, b"\x1b[C")
        capture("after_right")

        write(master, b"\x01")
        capture("after_ctrl_a")

        write(master, b"\x05")
        capture("after_ctrl_e")

        write(master, b"\r")
        capture("after_enter", 0.5)

        write(master, b"exit\r")
        capture("after_exit", 0.5)

    except Exception as exc:
        events.append(("exception", str(exc).encode()))

    finally:
        try:
            os.close(master)
        except OSError:
            pass

        try:
            os.waitpid(pid, 0)
        except OSError:
            pass

    with open(RAW_LOG, "wb") as f:
        f.write(raw)

    with open(REPORT, "w", encoding="utf-8") as f:
        f.write("MSH READLINE PTY TEST\n")
        f.write("====================\n\n")

        f.write(f"binary: {BINARY}\n")
        f.write(f"raw_bytes: {len(raw)}\n")
        f.write(f"echo_abc_occurrences: {count_exact(raw, b'echo abc')}\n")
        f.write(f"prompt_occurrences: {count_exact(raw, PROMPT)}\n")
        f.write(f"ansi_sequence_count: {len(ansi_sequences(raw))}\n")

        f.write("\n===== VISIBLE OUTPUT =====\n")
        f.write(visible(raw))

        f.write("\n\n===== EVENTS =====\n")

        for label, data in events:
            f.write(f"\n--- {label} ---\n")
            f.write(f"bytes={len(data)}\n")
            f.write(
                data.decode(
                    "utf-8",
                    errors="backslashreplace"
                )
            )

        f.write("\n\n===== ANSI =====\n")

        for i, seq in enumerate(ansi_sequences(raw), 1):
            f.write(
                f"{i:04d}: "
                + seq.decode(
                    "ascii",
                    errors="backslashreplace"
                )
                + "\n"
            )


if __name__ == "__main__":
    main()
PY

if python3 -m py_compile "$PTY_SCRIPT" 2>/dev/null; then
    pass "PTY-тестер создан: $PTY_SCRIPT"
else
    fail "Ошибка синтаксиса PTY-тестера"
fi

###############################################################################
# 14. PTY TEST
###############################################################################

section "14. АВТОМАТИЧЕСКИЙ ТЕРМИНАЛЬНЫЙ ТЕСТ"

PTY_RAW="$LOG_DIR/pty_raw.log"
PTY_REPORT="$LOG_DIR/pty_report.txt"

if python3 "$PTY_SCRIPT" \
    "$BINARY" \
    "$PTY_RAW" \
    "$PTY_REPORT"
then
    pass "PTY-тест завершён"
else
    fail "PTY-тест завершился с ошибкой"
fi

###############################################################################
# 15. RAW OUTPUT ANALYSIS
###############################################################################

section "15. АНАЛИЗ ФАКТИЧЕСКОГО PTY-ВЫВОДА"

PTY_ANALYSIS="$LOG_DIR/pty_analysis.txt"

python3 - "$PTY_RAW" > "$PTY_ANALYSIS" <<'PY'
import sys
import re

path = sys.argv[1]

data = open(path, "rb").read()

print("RAW BYTE ANALYSIS")
print("=================")
print()
print("bytes:", len(data))
print()

def positions(needle):
    result = []
    pos = 0

    while True:
        p = data.find(needle, pos)

        if p < 0:
            break

        result.append(p)
        pos = p + 1

    return result

for needle in [
    b"msh:",
    b"echo abc",
    b"abc",
    b"exit",
]:
    p = positions(needle)

    print(
        f"{needle!r}: "
        f"count={len(p)} "
        f"positions={p}"
    )

print()

ansi = re.findall(
    rb"\x1b\[[0-?]*[ -/]*[@-~]",
    data
)

print("ANSI sequences:", len(ansi))
print()

for i, seq in enumerate(ansi, 1):
    print(
        f"{i:04d}: "
        + seq.decode("ascii", errors="backslashreplace")
    )

print()
print("VISIBLE OUTPUT")
print("==============")

text = data.decode("utf-8", errors="replace")

text = re.sub(
    r"\x1b\[[0-?]*[ -/]*[@-~]",
    "",
    text
)

text = re.sub(
    r"\x1b[@-_]",
    "",
    text
)

print(text)
PY

pass "PTY-анализ сохранён: $PTY_ANALYSIS"

###############################################################################
# 16. ANSI ANALYSIS
###############################################################################

section "16. АНАЛИЗ ANSI-ПОСЛЕДОВАТЕЛЬНОСТЕЙ"

ANSI_LOG="$LOG_DIR/ansi_analysis.txt"

python3 - "$PTY_RAW" > "$ANSI_LOG" <<'PY'
import sys
import re
from collections import Counter

path = sys.argv[1]

data = open(path, "rb").read()

seqs = re.findall(
    rb"\x1b\[[0-?]*[ -/]*[@-~]",
    data
)

counter = Counter(seqs)

print("ANSI SEQUENCE SUMMARY")
print("=====================")
print()

for seq, count in counter.most_common():
    print(
        f"{count:6d}  "
        + seq.decode("ascii", errors="backslashreplace")
    )

print()
print("ESCAPE SEQUENCE ORDER")
print("=====================")

for i, seq in enumerate(seqs, 1):
    print(
        f"{i:04d}: "
        + seq.decode("ascii", errors="backslashreplace")
    )

print()
print("SUSPICIOUS PATTERNS")
print("====================")

for seq, count in counter.items():

    s = seq.decode("ascii", errors="backslashreplace")

    if count >= 10:
        print(
            f"REPEATED >= 10: count={count}: {s}"
        )

    if b"2J" in seq or b"2K" in seq:
        print(
            f"CLEAR: count={count}: {s}"
        )

    if b"6n" in seq:
        print(
            f"CURSOR_QUERY: count={count}: {s}"
        )
PY

pass "ANSI-анализ сохранён: $ANSI_LOG"

###############################################################################
# 17. PROMPT ANALYSIS
###############################################################################

section "17. ПОИСК ПОВТОРНОГО PROMPT"

PROMPT_LOG="$LOG_DIR/prompt_analysis.txt"

python3 - "$PTY_RAW" > "$PROMPT_LOG" <<'PY'
import sys

data = open(sys.argv[1], "rb").read()

patterns = [
    b"msh:",
    b">>",
    b"echo abc",
]

print("PROMPT / INPUT OCCURRENCES")
print("==========================")
print()

for p in patterns:
    positions = []
    pos = 0

    while True:
        n = data.find(p, pos)

        if n < 0:
            break

        positions.append(n)
        pos = n + 1

    print(
        f"{p!r}: count={len(positions)} positions={positions}"
    )

print()
print("PROMPT CONTEXT")
print("==============")

needle = b"msh:"

pos = 0
index = 0

while True:
    n = data.find(needle, pos)

    if n < 0:
        break

    index += 1

    start = max(0, n - 80)
    end = min(len(data), n + 160)

    print()
    print(
        f"--- prompt #{index}, byte {n} ---"
    )

    print(
        data[start:end].decode(
            "utf-8",
            errors="backslashreplace"
        )
    )

    pos = n + 1
PY

pass "Анализ prompt сохранён: $PROMPT_LOG"

###############################################################################
# 18. REPETITION DETECTION
###############################################################################

section "18. ПОИСК ПОВТОРНОЙ ОТРИСОВКИ ВВОДА"

REPEAT_LOG="$LOG_DIR/repetition_analysis.txt"

python3 - "$PTY_RAW" > "$REPEAT_LOG" <<'PY'
import sys
import re

data = open(sys.argv[1], "rb").read()

patterns = [
    b"echo abc",
    b"abc",
    b"echo",
]

print("REPEATED INPUT DETECTION")
print("========================")
print()

for needle in patterns:

    positions = []
    pos = 0

    while True:
        n = data.find(needle, pos)

        if n < 0:
            break

        positions.append(n)
        pos = n + 1

    print(f"{needle!r}")
    print(f"count: {len(positions)}")
    print(f"positions: {positions}")

    if len(positions) > 1:
        print(
            "STATUS: POSSIBLE REDRAW REPETITION"
        )

        for i, p in enumerate(positions, 1):
            start = max(0, p - 100)
            end = min(len(data), p + len(needle) + 100)

            print()
            print(
                f"--- occurrence {i} at byte {p} ---"
            )

            print(
                data[start:end].decode(
                    "utf-8",
                    errors="backslashreplace"
                )
            )

    print()
PY

###############################################################################
# 19. SUSPICIOUS PATTERNS
###############################################################################

section "19. ПОИСК ПОДОЗРИТЕЛЬНЫХ КОНСТРУКЦИЙ"

SUSPICIOUS_LOG="$LOG_DIR/suspicious_patterns.txt"

{
    echo "===== redraw calls ====="
    grep -nRE \
        '\brl_redraw\s*\(' \
        "$CORE_DIR"/*.c "$INCLUDE_DIR"/*.h 2>/dev/null

    echo
    echo "===== render calls ====="
    grep -nRE \
        '\brl_render\s*\(' \
        "$CORE_DIR"/*.c "$INCLUDE_DIR"/*.h 2>/dev/null

    echo
    echo "===== finish calls ====="
    grep -nRE \
        '\brl_finish_line\s*\(' \
        "$CORE_DIR"/*.c "$INCLUDE_DIR"/*.h 2>/dev/null

    echo
    echo "===== suggestion calls ====="
    grep -nRE \
        '\brl_(suggest|clear_suggestion)\s*\(' \
        "$CORE_DIR"/*.c "$INCLUDE_DIR"/*.h 2>/dev/null

    echo
    echo "===== prompt output ====="
    grep -nRE \
        'prompt|PROMPT|sh_build_prompt' \
        "$CORE_DIR"/*.c "$INCLUDE_DIR"/*.h 2>/dev/null

    echo
    echo "===== cursor queries ====="
    grep -nRE \
        'rl_get_cursor_position|6n|TIOCGWINSZ' \
        "$CORE_DIR"/*.c "$INCLUDE_DIR"/*.h 2>/dev/null

    echo
    echo "===== clear operations ====="
    grep -nRE \
        'clear|erase|2K|2J|K\\x1b|033\[K' \
        "$CORE_DIR"/*.c "$INCLUDE_DIR"/*.h 2>/dev/null

} > "$SUSPICIOUS_LOG"

pass "Анализ подозрительных конструкций сохранён: $SUSPICIOUS_LOG"

###############################################################################
# 20. SOURCE SNAPSHOT
###############################################################################

section "20. СОХРАНЕНИЕ SNAPSHOT READLINE"

SNAPSHOT="$LOG_DIR/readline_sources_snapshot.txt"

{
    echo "============================================================"
    echo "READLINE SOURCE SNAPSHOT"
    echo "============================================================"
    echo

    for file in \
        "$INCLUDE_DIR/readline_internal.h" \
        "$CORE_DIR/readline.c" \
        "$CORE_DIR/rl_init.c" \
        "$CORE_DIR/rl_event.c" \
        "$CORE_DIR/rl_cursor.c" \
        "$CORE_DIR/rl_history.c" \
        "$CORE_DIR/rl_suggest.c" \
        "$CORE_DIR/rl_complete.c" \
        "$CORE_DIR/rl_layout.c" \
        "$CORE_DIR/rl_render.c" \
        "$CORE_DIR/editor.c" \
        "$CORE_DIR/input.c" \
        "$CORE_DIR/delete.c" \
        "$CORE_DIR/redraw.c" \
        "$CORE_DIR/terminal.c" \
        "$CORE_DIR/finish.c" \
        "$CORE_DIR/history.c" \
        "$CORE_DIR/prompt.c"
    do
        if [ -f "$file" ]; then
            echo
            echo "============================================================"
            echo "FILE: $file"
            echo "============================================================"
            cat "$file"
        fi
    done
} > "$SNAPSHOT"

pass "Snapshot исходников сохранён: $SNAPSHOT"

###############################################################################
# 21. AUTOMATIC DIAGNOSIS
###############################################################################

section "21. АВТОМАТИЧЕСКАЯ ДИАГНОСТИКА"

DIAG_LOG="$LOG_DIR/diagnosis.txt"

python3 - \
    "$PTY_RAW" \
    "$ANSI_LOG" \
    "$PROMPT_LOG" \
    "$REPEAT_LOG" \
    "$TERMINAL_LOG" \
    "$SYMBOL_LOG" \
    "$DUP_LOG" \
    > "$DIAG_LOG" <<'PY'
import sys
import re

raw_path = sys.argv[1]
ansi_path = sys.argv[2]
prompt_path = sys.argv[3]
repeat_path = sys.argv[4]
terminal_path = sys.argv[5]
symbol_path = sys.argv[6]
dup_path = sys.argv[7]

raw = open(raw_path, "rb").read()

print("MSH READLINE AUTOMATIC DIAGNOSIS")
print("================================")
print()

def positions(needle):
    result = []
    pos = 0

    while True:
        p = raw.find(needle, pos)

        if p < 0:
            break

        result.append(p)
        pos = p + 1

    return result


prompt = positions(b"msh:")
echo = positions(b"echo abc")
abc = positions(b"abc")

print("COUNTERS")
print("--------")
print("prompt:", len(prompt), prompt)
print("echo abc:", len(echo), echo)
print("abc:", len(abc), abc)
print()

###############################################################################
# PROMPT
###############################################################################

print("PROMPT")
print("------")

if not prompt:
    print("FAIL: prompt was not detected")
else:
    print(f"prompt occurrences: {len(prompt)}")

    if len(prompt) > 10:
        print(
            "WARNING: unusually many prompt occurrences"
        )

###############################################################################
# REPEATED INPUT
###############################################################################

print()
print("INPUT REPETITION")
print("-----------------")

if len(echo) > 1:
    print(
        "FAIL: 'echo abc' appears multiple times in PTY output"
    )

    print(
        "This can indicate repeated redraw/render/suggestion output."
    )

    for i, p in enumerate(echo, 1):
        start = max(0, p - 80)
        end = min(len(raw), p + 160)

        print()
        print(f"occurrence #{i}: byte {p}")
        print(
            raw[start:end].decode(
                "utf-8",
                errors="backslashreplace"
            )
        )
else:
    print("OK: no repeated complete 'echo abc' detected")

###############################################################################
# ABC REPETITION
###############################################################################

print()
print("VISIBLE TEXT REPETITION")
print("------------------------")

if len(abc) > 3:
    print(
        "WARNING: 'abc' occurs more than three times."
    )
    print(
        "This may be normal because command output contains 'abc',"
    )
    print(
        "so inspect repetition_analysis.txt before changing code."
    )
else:
    print("OK: no excessive 'abc' repetition")

###############################################################################
# ANSI
###############################################################################

print()
print("ANSI")
print("----")

ansi = re.findall(
    rb"\x1b\[[0-?]*[ -/]*[@-~]",
    raw
)

print("ANSI sequences:", len(ansi))

cursor_queries = [
    x for x in ansi
    if x == b"\x1b[6n"
]

print(
    "cursor position queries (CSI 6n):",
    len(cursor_queries)
)

if len(cursor_queries) > 20:
    print(
        "WARNING: many cursor-position queries."
    )

###############################################################################
# CLEAR
###############################################################################

clears = [
    x for x in ansi
    if x in (
        b"\x1b[2K",
        b"\x1b[K",
        b"\x1b[2J",
        b"\x1b[J",
    )
]

print(
    "clear sequences:",
    len(clears)
)

if len(clears) > 20:
    print(
        "WARNING: many terminal clear operations."
    )

###############################################################################
# SOURCE SYMBOLS
###############################################################################

print()
print("SOURCE CORRELATION")
print("------------------")

symbols = open(symbol_path, encoding="utf-8", errors="replace").read()

for name in [
    "rl_redraw",
    "rl_render",
    "rl_suggest",
    "rl_clear_suggestion",
    "rl_get_cursor_position",
    "rl_finish_line",
]:
    lines = [
        line for line in symbols.splitlines()
        if name in line
    ]

    print()
    print(name)

    if lines:
        for line in lines:
            print("  ", line)
    else:
        print("   NOT FOUND")

###############################################################################
# DUPLICATES
###############################################################################

print()
print("DUPLICATES")
print("----------")

dup = open(
    dup_path,
    encoding="utf-8",
    errors="replace"
).read()

if "NO_DUPLICATE_FUNCTION_DEFINITIONS" in dup:
    print("OK: no duplicate function definitions")
else:
    print(dup)

###############################################################################
# FINAL
###############################################################################

print()
print("FINAL DIAGNOSIS")
print("---------------")

issues = []

if len(echo) > 1:
    issues.append(
        "Repeated command text detected in PTY output"
    )

if len(prompt) == 0:
    issues.append(
        "Prompt not detected"
    )

if len(cursor_queries) > 20:
    issues.append(
        "Excessive CSI 6n cursor queries"
    )

if len(clears) > 20:
    issues.append(
        "Excessive terminal clear operations"
    )

if issues:
    print("PROBLEMS DETECTED:")
    for i, issue in enumerate(issues, 1):
        print(f"{i}. {issue}")
else:
    print(
        "No obvious terminal corruption detected by the automatic checks."
    )

print()
print(
    "IMPORTANT: static analysis is diagnostic only."
)
print(
    "Do not modify readline based only on this section."
)
print(
    "Use pty_raw.log + repetition_analysis.txt + ansi_analysis.txt"
)
print(
    "to identify the exact rendering sequence."
)
PY

pass "Диагностика сохранена: $DIAG_LOG"

###############################################################################
# 22. TIMESTAMPS
###############################################################################

section "22. TIMESTAMP / СООТВЕТСТВИЕ БИНАРНИКА"

TIMESTAMP_LOG="$LOG_DIR/timestamps.txt"

{
    echo "===== BINARY ====="
    stat "$BINARY" 2>/dev/null

    echo
    echo "===== SOURCE FILES ====="

    for file in "$CORE_DIR"/*.c "$INCLUDE_DIR"/*.h; do
        [ -f "$file" ] || continue
        stat -c '%y %n' "$file"
    done
} > "$TIMESTAMP_LOG"

pass "Timestamp информация сохранена: $TIMESTAMP_LOG"

###############################################################################
# 23. FINAL SUMMARY
###############################################################################

section "23. ИТОГ"

printf 'PASS: %d\n' "$PASS"
printf 'FAIL: %d\n' "$FAIL"
printf 'WARN: %d\n' "$WARN"
printf '\n'

printf 'Логи:\n'
printf '  %s\n' "$LOG_DIR"
printf '\n'

printf 'Главные файлы:\n'
printf '  %s\n' "$DIAG_LOG"
printf '  %s\n' "$PTY_RAW"
printf '  %s\n' "$PTY_REPORT"
printf '  %s\n' "$PTY_ANALYSIS"
printf '  %s\n' "$REPEAT_LOG"
printf '  %s\n' "$ANSI_LOG"
printf '  %s\n' "$PROMPT_LOG"
printf '  %s\n' "$TERMINAL_LOG"
printf '  %s\n' "$SYMBOL_LOG"
printf '  %s\n' "$DUP_LOG"
printf '  %s\n' "$SNAPSHOT"
printf '\n'

if [ "$FAIL" -eq 0 ]; then
    printf '[RESULT] Явных FAIL не обнаружено.\n'
else
    printf '[RESULT] Обнаружено FAIL: %d\n' "$FAIL"
fi

printf '\n'
printf 'Для повторного запуска:\n'
printf '  bash %s/test_readline.sh\n' "$SCRIPT_DIR"
printf '\n'

###############################################################################
# IMPORTANT EXIT CODE
#
# Не используем FAIL как exit code: Android/Termux может прервать дальнейшую
# диагностику при запуске через оболочку. Скрипт всегда доходит до отчёта.
###############################################################################

exit 0