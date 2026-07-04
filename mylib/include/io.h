#ifndef IO_H
#define IO_H

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>

/* ============================================================
 * FLAGS
 * ============================================================ */
#define RL_SHELL (1 << 0)


/* ============================================================
 * DEBUG CONTROL
 * ============================================================ */

#ifndef DEBUG_EXEC
# define DEBUG_EXEC 1
#endif

#ifndef DEBUG_PIPE
# define DEBUG_PIPE 1
#endif

#ifndef DEBUG_WAIT
# define DEBUG_WAIT 1
#endif

#ifndef DEBUG_SPAWN
# define DEBUG_SPAWN 1
#endif

#ifndef DEBUG_REDIR
# define DEBUG_REDIR 1
#endif


/* ============================================================
 * DEBUG MACROS (SAFE VERSION)
 * ============================================================ */

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


/* ============================================================
 * IO API
 * ============================================================ */

char *read_line(FILE *fp, int flg);

int puts_fd(int out_fd, const char *buf, size_t len);

int printf_fd(int out_fd, const char *fmt, ...);

char *read_fd(int in_fd, size_t *sizep);

#endif // IO_H