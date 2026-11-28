/*
 * client.c — one request, one answer, exit.
 * Owner: Islam (Protocol and client). Wire format: docs/protocol.md.
 *
 *   ./client <host> <port> submit "<command>" <timeout> [priority]
 *   ./client <host> <port> status <id>
 *   ./client <host> <port> cancel <id>
 *   ./client <host> <port> fetch  <id>
 *
 * The server's reply is one line: the first word is the result
 * (OK / NO / UNKNOWN / ERR), the rest is the exact text to print.
 */
#include "protocol.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Exit codes: tell a script what the answer was without parsing the text. */
enum {
    EXIT_OK        = 0,   /* OK */
    EXIT_NO        = 1,   /* NO: already finished / not finished */
    EXIT_UNKNOWN   = 2,   /* UNKNOWN id */
    EXIT_NOCONNECT = 3,   /* cannot connect / server broke the connection */
    EXIT_USAGE     = 4    /* bad arguments, or ERR from the server */
};

/* Server address from argv[1] and argv[2]; set once in main. */
static const char *host, *port;

/* Prints the correct way to run the program to stderr. */
static void usage(void)
{
    fprintf(stderr,
            "usage: client <host> <port> submit \"<command>\" <timeout> [priority]\n"
            "       client <host> <port> status|cancel|fetch <id>\n");
}

/* Prints a connection problem to stderr and returns exit code 3,
 * so callers can write: return fail_connection("..."); */
static int fail_connection(const char *what)
{
    fprintf(stderr, "client: %s\n", what);
    return EXIT_NOCONNECT;
}

/* Checks that an id is one word: 1..64 bytes, no spaces, no control
 * characters. Returns 1 if valid, 0 if not. */
static int valid_id(const char *id)
{
    size_t n = strlen(id);
    if (n == 0 || n > 64)
        return 0;
    for (size_t i = 0; i < n; i++)
        if ((unsigned char)id[i] <= ' ')
            return 0;
    return 1;
}

/* Checks the command-line arguments and turns them into one request line:
 *   submit "cmd" 5 2  ->  "SUBMIT 5 2 cmd"
 *   status T12-1      ->  "STATUS T12-1"   (same for cancel / fetch)
 * Writes it into req. Returns 0, or -1 on bad arguments. */
static int build_request(int argc, char **argv, char *req, size_t size)
{
    const char *verb = argv[3];

    if (strcmp(verb, "submit") == 0) {
        long timeout, priority = 0;
        const char *cmd;

        if (argc != 6 && argc != 7)
            return -1;
        cmd = argv[4];
        if (cmd[0] == '\0' || strlen(cmd) > MAX_CMD || strchr(cmd, '\n') != NULL) {
            fprintf(stderr, "client: command must be 1..%d bytes, without newlines\n", MAX_CMD);
            return -1;
        }
        if (parse_long(argv[5], 1, 1000000, &timeout) != 0) {
            fprintf(stderr, "client: timeout must be a whole number of seconds >= 1\n");
            return -1;
        }
        if (argc == 7 && parse_long(argv[6], 0, 9, &priority) != 0) {
            fprintf(stderr, "client: priority must be 0..9\n");
            return -1;
        }
        /* the command is the LAST field: the rest of the line */
        snprintf(req, size, "SUBMIT %ld %ld %s", timeout, priority, cmd);
        return 0;
    }

    if (strcmp(verb, "status") == 0 || strcmp(verb, "cancel") == 0 ||
        strcmp(verb, "fetch") == 0) {
        if (argc != 5 || !valid_id(argv[4]))
            return -1;
        /* verb in upper case on the wire: STATUS / CANCEL / FETCH */
        snprintf(req, size, "%s %s",
                 verb[0] == 's' ? "STATUS" : verb[0] == 'c' ? "CANCEL" : "FETCH",
                 argv[4]);
        return 0;
    }

    return -1;                          /* unknown verb */
}

/* After "OK <nbytes>" to FETCH: copies exactly nbytes of job output from
 * the socket to stdout in 64 KB chunks. Returns 0, or 3 on any problem. */
static int copy_output(int fd, const char *nbytes_text)
{
    char chunk[65536];
    long left;

    if (parse_long(nbytes_text, 0, MAX_OUTPUT, &left) != 0)
        return fail_connection("bad reply from server");

    while (left > 0) {
        size_t n = left < (long)sizeof chunk ? (size_t)left : sizeof chunk;
        if (recv_bytes(fd, chunk, n) != 0)
            return fail_connection("output cut short by server");
        if (send_all(STDOUT_FILENO, chunk, n) != 0)
            return EXIT_NOCONNECT;      /* stdout closed, nothing more to do */
        left -= (long)n;
    }
    return EXIT_OK;
}

/* Whole client: check args -> connect -> send one request -> read one
 * reply -> print it -> exit with the code that matches the first word. */
int main(int argc, char **argv)
{
    char req[MAX_LINE], reply[MAX_LINE];
    int fd, is_fetch;

    /* Writing to a socket the server closed must return -1, not kill us. */
    if (ignore_sigpipe() == -1) {
        perror("sigaction");
        return EXIT_USAGE;
    }

    if (argc < 4) {
        usage();
        return EXIT_USAGE;
    }
    host = argv[1];
    port = argv[2];
    is_fetch = strcmp(argv[3], "fetch") == 0;

    if (build_request(argc, argv, req, sizeof req) != 0) {
        usage();
        return EXIT_USAGE;
    }

    fd = tcp_connect(host, port);
    if (fd < 0) {
        fprintf(stderr, "client: cannot connect to %s:%s\n", host, port);
        return EXIT_NOCONNECT;
    }

    if (send_line(fd, "%s", req) != 0) {
        close(fd);
        return fail_connection("cannot send request");
    }

    if (recv_line(fd, reply, sizeof reply) != 1) {
        close(fd);
        return fail_connection("no reply from server");
    }

    /* Split "WORD rest of line" */
    char *text = strchr(reply, ' ');
    if (text != NULL)
        *text++ = '\0';
    else
        text = "";

    int code;
    if (strcmp(reply, "OK") == 0) {
        if (is_fetch)
            code = copy_output(fd, text);       /* text is <nbytes> */
        else {
            printf("%s\n", text);
            code = EXIT_OK;
        }
    } else if (strcmp(reply, "NO") == 0) {
        fprintf(is_fetch ? stderr : stdout, "%s\n", text);
        code = EXIT_NO;
    } else if (strcmp(reply, "UNKNOWN") == 0) {
        printf("%s\n", text);
        code = EXIT_UNKNOWN;
    } else if (strcmp(reply, "ERR") == 0) {
        fprintf(stderr, "client: server error: %s\n", text);
        code = EXIT_USAGE;
    } else {
        code = fail_connection("bad reply from server");
    }

    close(fd);
    return code;
}