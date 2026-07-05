#ifndef MSH_DEBUG_H
#define MSH_DEBUG_H

#include <stdio.h>
#include <unistd.h>

/*
 * Мастер-флаг DEBUG:
 *   -DDEBUG      → все DBG_* включены (если не переопределены)
 *   без -DDEBUG  → все DBG_* выключены
 *
 * Тонкая настройка:
 *   -DDEBUG_SPAWN=0   → выключить только SPAWN
 *   -DDEBUG_EXEC=1    → включить только EXEC
 */

#ifdef DEBUG
# ifndef DEBUG_EXEC
#  define DEBUG_EXEC 1
# endif

# ifndef DEBUG_PIPE
#  define DEBUG_PIPE 1
# endif

# ifndef DEBUG_WAIT
#  define DEBUG_WAIT 1
# endif

# ifndef DEBUG_SPAWN
#  define DEBUG_SPAWN 1
# endif

# ifndef DEBUG_REDIR
#  define DEBUG_REDIR 1
# endif

#ifndef DEBUG_EXPAND
#define DEBUG_EXPAND 1
#endif

#else
# define DEBUG_EXEC 0
# define DEBUG_PIPE 0
# define DEBUG_WAIT 0
# define DEBUG_SPAWN 0
# define DEBUG_REDIR 0
#define DEBUG_EXPAND 0

#endif

/* ============================================================
 * MACROS
 * ============================================================ */

int printf_fd(int out_fd, const char *fmt, ...);

#if DEBUG_EXEC
# define DBG(fmt, ...) \
    do { printf_fd(STDERR_FILENO, "[DEBUG] " fmt, ##__VA_ARGS__); } while (0)
#else
# define DBG(fmt, ...) do {} while (0)
#endif

#if DEBUG_PIPE
# define DBG_PIPE(fmt, ...) \
    do { printf_fd(STDERR_FILENO, "[PIPE] " fmt, ##__VA_ARGS__); } while (0)
#else
# define DBG_PIPE(fmt, ...) do {} while (0)
#endif

#if DEBUG_WAIT
# define DBG_WAIT(fmt, ...) \
    do { printf_fd(STDERR_FILENO, "[WAIT] " fmt, ##__VA_ARGS__); } while (0)
#else
# define DBG_WAIT(fmt, ...) do {} while (0)
#endif

#if DEBUG_SPAWN
# define DBG_SPAWN(fmt, ...) \
    do { printf_fd(STDERR_FILENO, "[SPAWN] " fmt, ##__VA_ARGS__); } while (0)
#else
# define DBG_SPAWN(fmt, ...) do {} while (0)
#endif

#if DEBUG_REDIR
# define DBG_REDIR(fmt, ...) \
    do { printf_fd(STDERR_FILENO, "[REDIR] " fmt, ##__VA_ARGS__); } while (0)
#else
# define DBG_REDIR(fmt, ...) do {} while (0)
#endif

#if DEBUG_EXPAND
#define DBG_EXPAND(fmt, ...) \
	do { printf_fd(STDERR_FILENO, "[EXPAND] " fmt, ##__VA_ARGS__); } while (0)
#else
#define DBG_EXPAND(fmt, ...) do {} while (0)
#endif

#endif // MSH_DEBUG_H