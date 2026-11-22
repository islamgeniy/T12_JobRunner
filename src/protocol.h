/*
 * protocol.h — shared wire helpers for server, worker and client.
 * Owner: Islam (Protocol and client). Format described in docs/protocol.md.
 *
 * Rule: nobody calls read()/write() on a socket directly; use these functions.
 *
 * Framing (Slide 10, "Framing the messages"): both classic rules are used,
 * like HTTP — every message is one line ending in '\n', and the job output
 * in a FETCH / REPORT answer is preceded by its length.
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

#define MAX_LINE   4096                 /* one message line, including '\n' */
#define MAX_CMD    3800                 /* job command, fits into a journal line */
#define MAX_OUTPUT (16 * 1024 * 1024)   /* captured job output, bytes */

/* Makes a write to a closed socket return -1 (EPIPE) instead of killing
 * the process with SIGPIPE. Call once at the start of main. 0 / -1. */
int ignore_sigpipe(void);

/* Connects to host:port over TCP. Returns a socket fd, or -1.
 * Unlike client_connect() from net.h it does NOT exit on error:
 * the caller prints its own message (the client must exit with 3). */
int tcp_connect(const char *host, const char *port);

/* Writes exactly n bytes (loops over write). 0 on success, -1 on error. */
int send_all(int fd, const void *data, size_t n);

/* printf-style, adds '\n' itself. 0 on success, -1 on error or if too long. */
int send_line(int fd, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/* Reads one line up to '\n' into out (without '\n', NUL-terminated).
 * Reads one byte at a time, like read_line() from net.h, so it never
 * takes bytes that belong to the next message.
 *  1 — a whole line was read
 *  0 — the peer closed the connection; a half line (no '\n') is DROPPED
 * -1 — error, or the line does not fit into out (size bytes) */
int recv_line(int fd, char *out, size_t size);

/* Reads exactly n bytes into out. 0 on success,
 * -1 on error or if the connection closed before n bytes arrived. */
int recv_bytes(int fd, void *out, size_t n);

/* Parses a whole decimal number in [min, max]. 0 on success, -1 if s is not one. */
int parse_long(const char *s, long min, long max, long *out);

#endif