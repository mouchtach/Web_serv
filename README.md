# webserv

> A non-blocking HTTP server written in C++98 — the 42 School project.

---

## Overview

In the vast realm of internet protocols, the **Hypertext Transfer
Protocol (HTTP)** stands as the backbone of communication for the
World Wide Web. To truly comprehend the intricacies of web
communication, **42** introduced a wonderful project to embark on —
one that challenges our skills and understanding as always.

Yes, we are talking about the **`webserv`** project: building a
custom HTTP server in C++98, with a particular emphasis on
**non-blocking** behavior.

### Why C++98?

I believe 42 does this on purpose, so that students gain a deeper
understanding of how C++ is implemented — and its history.

### Why non-blocking?

Building a web server in C++ involves various challenges, and one
common issue is related to **blocking operations**. In a traditional
blocking model, a thread waits until data is available or until a
write completes — during that time it is inactive, and the server
struggles to handle other incoming connections.

To address this, this server is driven end-to-end by **`poll()`** —
a single thread monitors every socket and pipe it owns (listening
sockets, client connections, CGI stdin/stdout pipes) and only acts
on the ones that are actually ready, so no request ever blocks
another.

---

## How this server uses `poll()`

Every file descriptor the server cares about — server sockets,
client sockets, and CGI pipes — lives in one `std::vector<pollfd>`.
The whole program is one loop:

```
while (true) {
    poll(_pollfds.data(), _pollfds.size(), -1);

    for each pollfd:
        POLLIN   -> pollinprocess(fd)
                       FD_SERVER -> newConnection(fd)      accept()
                       FD_CLIENT -> readFromClient(fd)      recv() + parse()
                       CGI_OUT   -> cgiReadOutput(fd)       read() CGI stdout
        POLLOUT  -> polloutprocess(fd)
                       CGI_IN    -> cgiWriteBody(fd)        write() CGI stdin
                       (client)  -> send() response bytes
        POLLERR/POLLHUP/POLLNVAL -> removeClient(fd)
}
```

Each fd is tagged in `_fdInfos` with an `FD_type`
(`FD_SERVER`, `FD_CLIENT`, `CGI_OUT`, `CGI_IN`) so `pollinprocess` /
`polloutprocess` know how to route it. Sockets and pipes are all
`O_NONBLOCK`, so `recv`, `send`, `read`, and `write` never stall the
loop — partial reads/writes are just picked up again on the next
`poll()` cycle (see `Response::getSentBytes` / `addBytesSent` and
`Client::getCgiBodySent` / `addCgiBodySent`).

---

## Features

- HTTP/1.0 and HTTP/1.1 request parsing (header + body, chunked-free,
  `Content-Length`–driven)
- `GET`, `POST`, `DELETE` on static files
- Directory listing via `autoindex`, or falling back to a location's
  `index` file
- `nginx`-style config file with multiple `server` blocks and
  `location` overrides
- Per-location method restrictions, root override, and custom
  `client_max_body_size`
- CGI execution (`fork` + `execve`, non-blocking pipes for stdin/stdout)
- Cookie-based session auth backed by CGI (`login.py` / `signup.py`)
- Custom error pages per status code, with a generic HTML fallback

---

## Configuration

Config files are parsed by `ConfigParssing`
(`ReadConfig` → `removeComments` → `tokenize` → `parseConfig` →
`validate`) into one or more `Config` objects, each holding a list
of `Location` overrides.

**Server-level directives**

| Directive               | Example                          |
|--------------------------|----------------------------------|
| `listen`                 | `listen 8081;`                   |
| `server_name`             | `server_name localhost;`         |
| `root`                    | `root ./www;`                    |
| `index`                   | `index index.html;`              |
| `autoindex`               | `autoindex off;`                 |
| `client_max_body_size`     | `client_max_body_size 200000;`   |
| `error_page`               | `error_page 404 ./www/errors/404.html;` |
| `methods`                  | `methods GET POST DELETE;`       |
| `location <path> { ... }`  | overrides any of the above per-path |

**Location-only directives**

| Directive         | Purpose                                      |
|--------------------|-----------------------------------------------|
| `cgi_extension`     | script extension handled as CGI (`.py`)       |
| `cgi_path`          | interpreter binary (`/usr/bin/python3`)       |
| `return`            | `return <code> <url>;` redirect               |

`ConfigParssing::validate()` rejects two server blocks sharing the
same `port` + `server_name`.

---

## Request lifecycle (`WebServ::handleRequest`)

1. `Client::matchLocation()` — longest-prefix match against
   `Config::getLocations()`. `/signup`, `/login`, `/cgi` are wired to
   their CGI scripts here (`signup.py`, `login.py`, `run_c.py`).
2. `Client::checkAccess()` — validates the request's cookie token
   against the in-memory token list:
   - unauthenticated + hitting anything other than
     `/login(.html)` / `/signup(.html)` → `redirectException("/login.html")`
   - authenticated + hitting `/login` or `/signup` → redirected to `/`
   - `/static` is always open
3. `Client::isMethodeAllowed()` — checked against the location's
   `methods` list (empty list = all methods allowed).
4. Dispatch:
   - `/signup`, `/login`, `/cgi` → `WebServ::startCgi()`
   - everything else → `Client::processStatic()`

### Static handling

- **GET** — serves regular files (`Client::sendFile`), redirects
  `dir` → `dir/` when missing the trailing slash, serves the
  location's `index` file if present, or renders an autoindex
  listing when `autoindex on;` is set; otherwise `403`.
- **POST** — parses a multipart body (`getBoundary` /
  `takeBodyContent`), writes the file under the location's root.
- **DELETE** — removes the target file from disk.

`isPathSafe()` rejects `..` and `//` in the URI before any of the
above run.

### CGI handling

`WebServ::startCgi()` creates two pipes, `fork()`s, and `execve()`s
the configured interpreter against the script, with the request
piped in via `CONTENT_LENGTH` / stdin and the environment built by
`buildCgiEnv()` (`REQUEST_METHOD`, `CONTENT_TYPE`, `SCRIPT_FILENAME`,
`HTTP_COOKIE`, …). Both ends of the pipes are non-blocking and
registered with `poll()` as `CGI_IN` / `CGI_OUT`. Once the child
exits, `finalizeCgiResponse()` parses the CGI's raw `Status:` /
header block (`parseCgiOutput`), optionally lifts a `Set-Cookie:
token=...` out of `/login`'s response into the server's token list
(`storeCgiToken`), and builds the HTTP response (`buildCgiResponse`).

`cgi/run_c.py` additionally compiles `cgi/c.c` with `gcc` on demand
before running it — a small "compile-and-run" CGI demo.

### Auth

- `signup.py` / `login.py` persist users to `cgi/users.json`
  (`username` → `{password, token}`), issuing a random hex token via
  `secrets.token_hex`.
- `WebServ::loadTokens()` reloads every stored token from
  `cgi/users.json` on startup so sessions survive a restart.
- `Client::validateToken()` just checks the request's cookie token
  against that in-memory list — no server-side session store beyond
  the vector of known-good tokens.

---

## Class overview

| Class            | Responsibility                                          |
|--------------------|-----------------------------------------------------------|
| `WebServ`           | owns the poll loop, fd bookkeeping, CGI orchestration       |
| `Server`             | wraps one listening socket's `Config`                       |
| `Client`             | per-connection state: request, response, matched location   |
| `Request`            | incremental HTTP request parsing                             |
| `Response`           | status/header/body assembly + partial-send bookkeeping       |
| `Config` / `Location`| parsed directives, `Location` inherits and overrides `Config`|
| `ConfigParssing`     | tokenizes and parses the config file into `Config` objects    |
| `HttpException`      | status-code-carrying exception for request errors             |
| `redirectException`  | thrown by `checkAccess()` to trigger a 302 redirect            |

---

## Build & run

```bash
$ make
$ ./webserv config/config.conf     # or omit the arg to use the default path
```

```
[INFO] Parsing config file: config/config.conf
[INFO] Loading saved tokens...
[INFO] Setting up sockets...
[OK]   webserv is ready.
```

Other targets: `make clean`, `make fclean`, `make re`,
`make clean_users` (wipes `cgi/users.json`).

---

## Project layout

```
.
├── main.cpp
├── config/
│   └── config.conf
├── parssing/                # config file parser
│   ├── config.cpp/.hpp
│   ├── configparssing.cpp/.hpp
│   └── location.cpp/.hpp
├── src/
│   ├── webserv.cpp/.hpp     # poll() loop, CGI orchestration
│   ├── client.cpp/.hpp      # per-connection request handling
│   ├── server.cpp/.hpp
│   ├── redirectException.cpp/.hpp
│   └── static_utils.cpp/.hpp
├── http/
│   ├── request.cpp/.hpp
│   ├── response.cpp/.hpp
│   └── httpexception.hpp
├── cgi/                     # python CGI scripts + demo C program
│   ├── login.py
│   ├── signup.py
│   ├── users_store.py
│   ├── run_c.py
│   └── c.c
└── www/                     # static site served by the console page
```

---

<p align="center"><sub>webserv — single-threaded, poll()-driven, C++98</sub></p>
