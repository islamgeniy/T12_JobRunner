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

/* Include guard: if several files include this header, it is compiled once. */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>                     /* size_t */

/* Size limits shared by all three programs. */
#define MAX_LINE   4096                 /* one message line, including '\n' */
#define MAX_CMD    3800                 /* job command, fits into a journal line */
#define MAX_OUTPUT (16 * 1024 * 1024)   /* captured job output, bytes */

/* Makes a write to a closed socket return -1 (EPIPE) instead of killing
 * the process with SIGPIPE. Call once at the start of main.
 * Returns 0 on success, -1 on error. */
int ignore_sigpipe(void);

/* Connects to host:port over TCP (IPv4 or IPv6).
 * Returns a connected socket fd, or -1 if it could not connect.
 * Unlike client_connect() from net.h it does NOT exit on error:
 * the caller prints its own message (the client must exit with 3). */
int tcp_connect(const char *host, const char *port);

/* Sends exactly n bytes from data, even if write() sends them in parts.
 * Returns 0 on success, -1 on error (e.g. the peer closed the socket). */
int send_all(int fd, const void *data, size_t n);

/* Sends one message: printf-style text plus '\n' added automatically.
 * Example: send_line(fd, "STATUS %s", id);
 * Returns 0 on success, -1 on error, if the text is too long,
 * or if it contains '\n' (that would split it into two messages).
 * The attribute lets the compiler check fmt against the arguments. */
int send_line(int fd, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/* Receives one message: reads up to '\n' into out (without '\n',
 * NUL-terminated). Reads one byte at a time, like read_line() from net.h,
 * so it never takes bytes that belong to the next message.
 * Returns:
 *  1 — a whole line was read
 *  0 — the peer closed the connection; a half line (no '\n') is DROPPED
 * -1 — error, or the line does not fit into out (size bytes) */
int recv_line(int fd, char *out, size_t size);

/* Receives exactly n bytes into out (used when the length is known,
 * e.g. job output after "OK <nbytes>").
 * Returns 0 on success, -1 on error or if the connection closed
 * before n bytes arrived. */
int recv_bytes(int fd, void *out, size_t n);

/* Converts text s to a whole decimal number and checks it is in [min, max].
 * Rejects "", "10abc", overflow and out-of-range values.
 * Returns 0 and stores the value in *out, or -1 if s is not valid. */
int parse_long(const char *s, long min, long max, long *out);

#endif /* PROTOCOL_H */