/*
 * protocol.c — implementation of protocol.h.
 * Owner: Islam (Protocol and client).
 */
#include "protocol.h"

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int ignore_sigpipe(void)
{
    struct sigaction sa;

    sa.sa_handler = SIG_IGN;            /* ignore: write() returns -1, EPIPE */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    return sigaction(SIGPIPE, &sa, NULL);   /* 0, or -1 on error */
}

int tcp_connect(const char *host, const char *port)
{
    struct addrinfo hints, *res, *ai;
    int fd = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;        /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;    /* TCP */

    if (getaddrinfo(host, port, &hints, &res) != 0)
        return -1;

    /* A host name can resolve to several addresses: try them in order. */
    for (ai = res; ai != NULL; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0)
            continue;
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;                      /* connected */
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

int send_all(int fd, const void *data, size_t n)
{
    const char *p = data;

    while (n > 0) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR)         /* interrupted by a signal: retry */
                continue;
            return -1;                  /* e.g. EPIPE: the peer is gone */
        }
        p += w;                         /* write may send only part of it */
        n -= (size_t)w;
    }
    return 0;
}

int send_line(int fd, const char *fmt, ...)
{
    char line[MAX_LINE];
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf(line, sizeof line - 1, fmt, ap);   /* leave room for '\n' */
    va_end(ap);

    if (len < 0 || len >= (int)sizeof line - 1)
        return -1;                      /* too long: refuse instead of cutting */
    if (memchr(line, '\n', (size_t)len) != NULL)
        return -1;                      /* a '\n' inside would split the message */

    line[len++] = '\n';
    return send_all(fd, line, (size_t)len);
}

int recv_line(int fd, char *out, size_t size)
{
    size_t n = 0;
    char ch;

    if (size == 0)
        return -1;

    for (;;) {
        ssize_t got = read(fd, &ch, 1); /* one byte: never read past '\n' */
        if (got == 0)
            return 0;                   /* peer closed; a half line is dropped */
        if (got < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (ch == '\n')
            break;                      /* end of the message */
        if (n + 1 >= size)
            return -1;                  /* too long: never cut it into pieces */
        out[n++] = ch;
    }

    if (n > 0 && out[n - 1] == '\r')
        n--;                            /* tolerate "\r\n" from manual testing */
    out[n] = '\0';
    return 1;
}

int recv_bytes(int fd, void *out, size_t n)
{
    char *p = out;

    while (n > 0) {
        ssize_t got = read(fd, p, n);
        if (got == 0)
            return -1;                  /* closed before all bytes arrived */
        if (got < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        p += got;
        n -= (size_t)got;
    }
    return 0;
}

int parse_long(const char *s, long min, long max, long *out)
{
    char *end;
    long v;

    if (s == NULL || *s == '\0')
        return -1;
    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0 || *end != '\0' || v < min || v > max)
        return -1;                      /* "10abc", "", overflow, out of range */
    *out = v;
    return 0;
}