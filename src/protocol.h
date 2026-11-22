#define MAX_LINE   4096
#define MAX_CMD    3800
#define MAX_OUTPUT (16 * 1024 * 1024)

/* Буферизованный читатель: хранит байты, прочитанные «лишними». */
typedef struct {
    int    fd;
    char   buf[MAX_LINE];
    size_t len;              /* сколько байт лежит в buf */
} reader_t;

void reader_init(reader_t *r, int fd);

/* Пишет все n байт (цикл по write). 0 — успех, -1 — ошибка. */
int send_all(int fd, const void *data, size_t n);

/* Как printf, но в сокет, и сама добавляет '\n'. 0 / -1. */
int send_line(int fd, const char *fmt, ...);

/* Читает одну строку до '\n', кладёт её в out без '\n'.
   1 — строка прочитана, 0 — соединение закрыто, -1 — ошибка или строка длиннее MAX_LINE. */
int recv_line(reader_t *r, char *out, size_t size);

/* Читает ровно n байт (сначала из r->buf, потом из сокета). 0 / -1. */
int recv_bytes(reader_t *r, void *out, size_t n);
