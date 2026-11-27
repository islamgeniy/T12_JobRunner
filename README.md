# SOC 3010 Operating Systems — Term Project

- **Команда (Team):** 12 (job ids `T12-1`, `T12-2`, …)
- **Курс:** SOC 3010 Operating Systems, Inha University in Tashkent, Fall 2026
- **Лидер команды (Team leader):** Islam

## Summary (English)

**Team 12** · **Team leader:** Islam · SOC 3010 Operating Systems, Fall 2026

A small distributed **job runner** in C for Ubuntu Linux: `client` submits shell commands, `server` queues and schedules them and keeps a journal (`jobs.log`) so no accepted job is lost, and `worker` (on another machine) runs each job with `fork` + `exec("/bin/sh", "-c", cmd)`, enforces its timeout and reports its exit status. Job ids start with the team number: `T12-1`, `T12-2`, …

| # | Component | Owner |
|---|-----------|-------|
| 1 | Protocol and client | Islam |
| 2 | Scheduler and policy | Amir |
| 3 | Worker and limits | Mirakbar |
| 4 | Durability and recovery | Abdulloh |
| 5 | Operations and report | Jamoliddin |
| 6 | Testing and load | Bulat |

Build: `make` in the repository root produces `./server`, `./worker`, `./client`. Usage and the full contract are below (in Russian) and in `docs/protocol.md`. AI use is disclosed in the section "Использование AI".

## Что мы делаем

Небольшой **job runner** («запускалка задач») на C для Ubuntu Linux. Задача (job) — это shell-команда, которую клиент хочет выполнить на другом компьютере. Систему составляют три программы:

| Программа | Что делает |
|-----------|------------|
| `client` | Отправляет задачу, спрашивает её статус, отменяет её или забирает её вывод. Один запрос, один ответ, выход. |
| `server` | Работает постоянно. Принимает клиентов, держит очередь задач, выбирает следующую задачу и отдаёт её свободному worker. Ведёт журнал `jobs.log`, поэтому после падения ни одна задача не теряется. |
| `worker` | Работает на другой машине. Подключается к серверу, получает задачу, запускает её через `fork` + `exec("/bin/sh", "-c", cmd)`, собирает вывод, следит за таймаутом и сообщает результат. |

Id задач начинаются с номера команды: `T12-1`, `T12-2`, …

## Кто за что отвечает

Владелец (owner) — человек, который на защите объясняет свой компонент и меняет его код вживую. Смотреть и предлагать правки может любой, но **чужие файлы меняем только после договорённости с владельцем**.

| # | Component | Owner | Ветка | Файлы |
|---|-----------|-------|-------|-------|
| 1 | Protocol and client | Islam | `protocol-client` | `src/protocol.h`, `src/protocol.c`, `src/client.c`, `docs/protocol.md` |
| 2 | Scheduler and policy | Amir | `scheduler` | `src/scheduler.h`, `src/scheduler.c`, `src/server.c` |
| 3 | Worker and limits | Mirakbar | `worker` | `src/worker.c` |
| 4 | Durability and recovery | Abdulloh | `durability` | `src/jobs.h`, `src/jobs.c` |
| 5 | Operations and report | Jamoliddin | `ops-report` | `Makefile`, `README.md`, `.gitignore`, `report.pdf` |
| 6 | Testing and load | Bulat | `testing` | `tests/*.sh` |

Что именно делает каждый:

1. **Islam — протокол и клиент.** Формат сообщений между программами (названия, поля, ответы с ошибками) и функции отправки и чтения строк (`send_line`, `recv_line`), которыми пользуются все. Отдельно — как заканчивается ответ на `fetch`. Плюс программа `client`. **Первым делом пишет `docs/protocol.md`**: от него зависят все остальные.
2. **Amir — планировщик.** Главный цикл сервера (`accept`, обслуживание 8 клиентов одновременно), очередь задач, учёт свободных слотов у worker. Политика: сначала FIFO, потом вторая (например, priority with aging). Trace (лог реального запуска), который доказывает, что политика работает.
3. **Mirakbar — worker.** `fork`, `exec`, pipe для вывода, таймаут (убить всю группу процессов), причина смерти задачи (`KILLED TIMEOUT` / `CANCELLED` / `KILLED SIGNAL n`), отсутствие zombie-процессов, корректный выход по `SIGTERM`.
4. **Abdulloh — надёжность.** Таблица задач и их состояния, одна функция `set_state()` для любой смены состояния, `cancel`, обработка ухода worker. Part 2: журнал `jobs.log` (через данный нам `journal.h`) и восстановление таблицы при рестарте; вывод задач в `out/`.
5. **Jamoliddin — сборка и отчёт.** `Makefile` (команда `make` собирает всё на чистой Ubuntu), порядок в репозитории, `report.pdf` (до 4 страниц, 6 разделов).
6. **Bulat — тесты.** Скрипт «8 клиентов одновременно», скрипт, который сам прогоняет тесты 1–13, проверки на zombie-процессы и утечки file descriptors.

## Структура репозитория

```
Makefile                      Jamoliddin
README.md                     Jamoliddin (лидер правит таблицу владельцев)
.gitignore                    Jamoliddin
.gitmessage                   шаблон сообщения коммита
.github/
  pull_request_template.md    шаблон Pull Request
docs/
  protocol.md                 Islam
src/
  net.h                       дан преподавателем, НЕ МЕНЯТЬ
  journal.h                   дан преподавателем, НЕ МЕНЯТЬ
  protocol.h / protocol.c     Islam
  client.c                    Islam
  server.c                    Amir
  scheduler.h / scheduler.c   Amir
  jobs.h / jobs.c             Abdulloh
  worker.c                    Mirakbar
tests/                        Bulat
report.pdf                    Jamoliddin (в конце)
```

В репозиторий **не попадают**: `server`, `worker`, `client` (собранные программы), `jobs.log`, `out/`. Это уже прописано в `.gitignore`.

## Ветки и правила

- **`main`** — только проверенный код. **Напрямую в `main` не пушит никто**, включая лидера.
- У каждого своя ветка (см. таблицу), работаешь только в ней.
- Готовый кусок попадает в `main` только через **Pull Request** (PR, запрос на слияние ветки). Лидер проверяет его и тестирует вместе ещё с одним участником на двух машинах.
- Коммитим **часто и маленькими кусками**. История git показывает, что работа шла неделями, а не за ночь перед дедлайном.
- PR делаем небольшими, раз в несколько дней, а не одним огромным в конце.
- Слияние PR — только кнопкой **Create a merge commit**. Squash не используем, чтобы сохранить историю.

## Как начать (один раз)

```sh
git clone https://github.com/islamgeniy/<repo>.git
cd <repo>
git switch <моя-ветка>                     # например: git switch worker
git config commit.template .gitmessage     # включить шаблон коммита
git config pull.rebase false
```

Если `git push` просит пароль: GitHub не принимает обычный пароль. Выполни `gh auth login` (GitHub CLI) или создай Personal Access Token в Settings → Developer settings.

## Каждый день

```sh
git switch <моя-ветка>
git pull                           # забрать последние изменения своей ветки
# ... пишем код ...
git status                         # что изменилось
git add src/worker.c               # добавляем конкретные файлы, а не всё подряд
git commit                         # откроется шаблон, заполняем его
git push
```

## Забрать новое из `main` в свою ветку

Когда лидер слил что-то в `main` (например, протокол), каждый подтягивает это к себе:

```sh
git fetch origin
git merge origin/main
git push
```

Если git сообщил о конфликте (conflict, обе стороны правили одно место): открой файл в VSCodium, выбери правильный вариант, затем выполни `git add <файл>` и `git commit`. Не получается разобраться — пиши владельцу файла.

## Как сделать Pull Request

1. Запушь свою ветку (`git push`).
2. На GitHub: вкладка **Pull requests** → **New pull request** → base: `main`, compare: твоя ветка.
3. Описание заполнится шаблоном. Заполни все пункты.
4. Reviewer — лидер (Islam). Для PR ветки `protocol-client` (ветка самого лидера) reviewer — любой другой участник.
5. Исправляешь замечания новыми коммитами в той же ветке. PR обновится сам.

## Сборка и запуск

Проверяем на Ubuntu 22.04 / 24.04 (можно в VM) с пакетом `build-essential`:

```sh
make                                      # собирает ./server ./worker ./client
make clean                                # удаляет их

./server 5000                             # сервер на порту 5000
./worker <ip-сервера> 5000 [K]            # K = сколько задач одновременно, по умолчанию 1
./client <ip-сервера> 5000 <команда> ...
```

## Команды клиента (вывод должен совпадать до символа)

| Команда | Первая строка вывода | Код выхода |
|---------|----------------------|------------|
| `submit "<команда>" <таймаут> [приоритет]` | id задачи, например `T12-7` | 0 |
| `status <id>` | `<id> QUEUED` · `RUNNING` · `DONE <код>` · `KILLED TIMEOUT` · `KILLED SIGNAL <n>` · `CANCELLED` | 0 |
| `cancel <id>` | `<id> CANCELLED` или `<id> ALREADY FINISHED` | 0 · 1 |
| `fetch <id>` | вывод задачи байт в байт (если не готова: `<id> NOT FINISHED` в stderr) | 0 · 1 |
| любая команда с неизвестным id | `<id> UNKNOWN` | 2 |

Сервер недоступен: `client: cannot connect to <host>:<port>` в stderr, код выхода 3.

## Файлы курса: `net.h` и `journal.h`

- `src/net.h` и `src/journal.h` даны преподавателем и лежат **без изменений**. Details 2 §8: `src/journal.h` должен быть именно данным файлом, без правок. Его берём с eClass (Details 2 §3).
- `journal.h` не требует ничего, кроме `build-essential`. **Не добавляйте `-std=c99`** в флаги компилятора (Details 2 §3).
- Сервер может использовать `server_listen` из `net.h`: в ней уже включён `SO_REUSEADDR`.
- Клиент и все чтения и записи по сети используют функции из `src/protocol.c`. Причины (подробно в `docs/protocol.md` §6): `client_connect` при ошибке делает `exit(1)`, а контракт требует код 3; `read_line` принимает оборванную строку за целое сообщение.

## Использование AI (AI disclosure)

Курс разрешает AI-инструменты при условии, что их использование раскрыто в сдаче (Syllabus). Оценивается то, что студент может объяснить и изменить сам.

<!-- Держать этот список честным и актуальным до сдачи. -->
- Инструмент: Claude (Anthropic) — помощник для обучения и написания кода.
- Для чего использовался: черновики документации (`README.md`, `docs/protocol.md`), черновик кода протокола и клиента (`src/protocol.*`, `src/client.c`), объяснения тем курса.
- Каждый владелец компонента разобрал свой код и может объяснить и изменить его на защите.

## Сроки

| Дата | Что |
|------|-----|
| до 29 ноября | `docs/protocol.md` в `main`; client ↔ server общаются; worker запускает `echo hello` |
| до 6 декабря | полный цикл submit → worker → status/fetch, таймаут, cancel; тесты 1–9 |
| до 13 декабря | zombie, утечки fd, SIGTERM, журнал (тест 13), вторая политика; запуск на 2 машинах |
| 14–18 декабря | только отчёт и финальная проверка (параллельно финальные экзамены); бронь слота защиты |
| **18 декабря, 23:59** | **загрузка на eClass**: ссылка на репозиторий + ZIP + `report.pdf`. Опоздание не принимается |
| 21–25 декабря | защита онлайн, все с **включённой камерой** |

## Шпаргалка git

| Команда | Что делает |
|---------|------------|
| `git status` | что изменено и на какой ты ветке |
| `git log --oneline --graph --all` | дерево коммитов всех веток |
| `git switch <ветка>` | перейти на ветку |
| `git diff` | изменения, которые ещё не добавлены через `add` |
| `git restore <файл>` | отменить незакоммиченные правки в файле (необратимо) |
| `git stash` / `git stash pop` | временно убрать правки / вернуть их |