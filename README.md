# SOC 3010 Operating Systems — Term Project

**Team:** T-12
**Course:** SOC 3010 Operating Systems, Inha University in Tashkent, Fall 2026
**Team leader:** Islam

A small **job runner** written in C for Ubuntu Linux. A *job* is a shell command that a client wants
executed on another machine. Three programs cooperate:

| Program  | Role |
|----------|------|
| `client` | Submits a job, asks for its status, cancels it, or fetches its output. One request, one answer, exit. |
| `server` | One long-running process. Accepts clients, keeps the job queue, picks the next job (scheduling policy) and sends it to a free worker. Keeps a journal (`jobs.log`) so no accepted job is lost after a crash. |
| `worker` | Runs on another machine. Connects to the server, receives jobs, runs each one with `fork` + `exec("/bin/sh", "-c", cmd)`, captures its output, enforces its timeout and reports the exit status. |

Job ids issued by our server start with our team number: `T<TEAM_NUMBER>-1`, `T<TEAM_NUMBER>-2`, …

## Components and owners

Anyone may touch any code, but the **owner** is the person who explains and changes that component live at the fair.

| # | Component | Owner | Branch |
|---|-----------|-------|--------|
| 1 | Protocol and client | Mirakbar | `protocol-client` |
| 2 | Scheduler and policy | Abdulloh | `scheduler` |
| 3 | Worker and limits | Islam | `worker` |
| 4 | Durability and recovery | Amir | `durability` |
| 5 | Operations and report | Bulat | `ops-report` |
| 6 | Testing and load | Jamoliddin | `testing` |

What each owner delivers:

1. **Protocol and client** — the wire format (message names, fields, framing, error replies) and the `client` program.
2. **Scheduler and policy** — the job queue, the scheduling policy (FIFO first, then one more policy), the dispatch decision, and a trace that proves the policy works.
3. **Worker and limits** — the `worker`: `fork`, `exec`, `wait`, output capture through a pipe, the timeout kill of the whole process group, honest exit status, no zombies.
4. **Durability and recovery** — the job table and its states, `cancel`, handling a worker that leaves, and the journal (`jobs.log`) with replay at restart.
5. **Operations and report** — the `Makefile`, the repository and its history, `report.pdf`.
6. **Testing and load** — the 8-clients-at-once script, a self-check script that runs rows 1–13 unattended, the zombie and file-descriptor checks.

## Build

On a fresh Ubuntu 22.04 / 24.04 with `build-essential`:

```sh
make          # produces ./server ./worker ./client in the repository root
make clean    # removes them
```

## Run

```sh
./server <port>                          # e.g. ./server 5000
./worker <server-host> <server-port> [K] # K = max parallel jobs, default 1
./client <server-host> <server-port> <verb> ...
```

Client verbs:

| Command | Prints (first line of stdout) | Exit code |
|---------|-------------------------------|-----------|
| `submit "<command>" <timeout> [priority]` | the new job id, e.g. `T<TEAM_NUMBER>-7` | 0 |
| `status <id>` | `<id> QUEUED` · `RUNNING` · `DONE <code>` · `KILLED TIMEOUT` · `KILLED SIGNAL <n>` · `CANCELLED` | 0 |
| `cancel <id>` | `<id> CANCELLED` or `<id> ALREADY FINISHED` | 0 · 1 |
| `fetch <id>` | the job's output, byte for byte (if not finished: `<id> NOT FINISHED` on stderr) | 0 · 1 |
| any verb, unknown id | `<id> UNKNOWN` | 2 |

If the server cannot be reached, the client prints `client: cannot connect to <host>:<port>` to stderr and exits with 3.

## Repository layout

```
Makefile
README.md
report.pdf        # added at the end
src/              # server.c, worker.c, client.c, shared headers,
                  # net.h and journal.h (given files, unchanged)
tests/            # test and load scripts
```

`jobs.log`, `out/` and the built executables are not committed (see `.gitignore`).

## Branch workflow

- `main` holds only reviewed and tested code. Nobody pushes to `main` directly.
- Each owner works on their own branch (see the table above) and opens a pull request when a piece works.
- The team leader reviews each pull request, tests it on two machines with another member, and only then merges it into `main`.