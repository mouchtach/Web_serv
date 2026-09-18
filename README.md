*This project has been created as part of the 42 curriculum by ymouchta, azmakhlo.*

# webserv

A non-blocking HTTP/1.0-1.1 server written from scratch in **C++98**, driven end-to-end by a single `poll()` loop — no request ever blocks another.

---

## Description

`webserv` is a custom implementation of an HTTP server, inspired by servers like NGINX. The goal of the project is to understand, at a low level, how a web server actually works: parsing raw HTTP requests, managing sockets, and generating valid HTTP responses — all without relying on any external HTTP or networking library.

The server:

- Listens on multiple ports/virtual servers at once, configured through an NGINX-style configuration file.
- Accepts connections and handles many clients concurrently using a **single thread** and a single `poll()` call — never `read()`/`recv()`/`write()`/`send()` without going through `poll()` first, and every socket/pipe is non-blocking.
- Parses raw HTTP requests (request line, headers, body) incrementally, as bytes arrive.
- Serves static files (`GET`), accepts file uploads (`POST`), and deletes resources (`DELETE`).
- Supports directory listing (`autoindex`) and custom index files.
- Executes CGI scripts (e.g. Python) through `fork()` + `execve()`, communicating with the child process through non-blocking pipes.
- Returns proper HTTP status codes and supports custom error pages per status code.
- Never crashes and never leaks. A single misbehaving client or CGI must not stop the server from serving other clients.

This project is a core part of the 42 network curriculum, meant to demystify how protocols like HTTP work under the hood, and how a real server (like NGINX) is built around non-blocking I/O.

---

## Instructions

### Requirements

- A C++98-compliant compiler (`c++`/`g++`/`clang++`)
- `make`
- A POSIX-compliant OS (Linux / macOS)
- `python3` (only needed to run the bundled CGI demo scripts)

### Compilation

```bash
make
```

This builds the `webserv` binary and creates the `uploads/` directory used by the `/upload` route.

Other available targets:

| Target             | Effect                                             |
|--------------------|-----------------------------------------------------|
| `make clean`        | Removes object files                                |
| `make fclean`        | Removes object files, the binary, and `uploads/`     |
| `make re`             | `fclean` + `all`                                     |
| `make clean_users`     | Wipes `cgi/users.json` (registered accounts/tokens) |

### Running the server

```bash
./webserv [path/to/config.conf]
```

If no configuration file is given, `config/config.conf` is used by default.

```
[INFO] Parsing config file: config/config.conf
[INFO] Loading saved tokens...
[INFO] Setting up sockets...
[OK]   webserv is ready.
```

Once running, open a browser at `http://localhost:<port>` (default: `8081`) to reach the demo website (file manager, CGI runner, login/signup).

You can also test it directly with `curl`:

```bash
curl -v http://localhost:8081/
curl -v -X POST -F "file=@somefile.txt" http://localhost:8081/upload
curl -v -X DELETE http://localhost:8081/upload/somefile.txt
```

### Configuration file

The configuration file syntax is inspired by NGINX. See `config/config.conf` for a complete example, and `TECHNICAL.md` for the full list of supported directives.

---

## Resources

Classic references used to understand the topic:

- [RFC 7230 — HTTP/1.1: Message Syntax and Routing](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 — HTTP/1.1: Semantics and Content](https://datatracker.ietf.org/doc/html/rfc7231)
- [RFC 3875 — The Common Gateway Interface (CGI) Version 1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- `man poll`, `man socket`, `man fork`, `man execve`, `man pipe`, `man fcntl`
- [NGINX documentation](https://nginx.org/en/docs/) — used as a reference for the configuration file syntax (`server`, `location`, directive inheritance)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- 42 subject PDF (`en_subject.pdf`) and 42 peer discussions / evaluation sheets

### Use of AI

AI (an LLM assistant) was used during this project as a support tool, not as a replacement for understanding or writing the core logic ourselves:

- **Debugging help**: explaining segfaults/undefined behavior around raw sockets, pipes, and `fork()`, and suggesting where to add checks (e.g. non-blocking read/write edge cases, partial `send()`/`recv()`).
- **Code review**: pointing out C++98-specific pitfalls (no `auto`, no range-based `for`, no `nullptr`, container/iterator usage) and memory-management issues (RAII, missing `delete`, dangling pointers).
- **Documentation**: drafting and formatting this `README.md` and the accompanying `TECHNICAL.md`, based on the actual source code and behavior of the project, which was then reviewed and corrected by us.
- **Design discussion**: sanity-checking the overall `poll()`-based architecture (single loop, `FD_info` tagging of file descriptors, CGI pipe handling) against known non-blocking server patterns.

All HTTP parsing, socket/CGI handling, and configuration parsing logic was designed and implemented by us; AI was not used to generate the networking/parsing core of the project.

---

## Authors

- **ymouchta**
- **azmakhlo**
