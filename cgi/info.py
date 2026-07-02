#!/usr/bin/env python3
import os
import datetime

method = os.environ.get("REQUEST_METHOD", "?")
query = os.environ.get("QUERY_STRING", "")
proto = os.environ.get("SERVER_PROTOCOL", "?")
script = os.environ.get("SCRIPT_NAME", "?")
remote = os.environ.get("REMOTE_ADDR", "?")
now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

body = (
    "Content-Type: text/plain\r\n"
    "\r\n"
    f"[cgi] process spawned via fork/execve\n"
    f"[cgi] interpreter : python3\n"
    f"[cgi] script      : {script}\n"
    f"[cgi] method       : {method}\n"
    f"[cgi] protocol     : {proto}\n"
    f"[cgi] query_string : {query if query else '(none)'}\n"
    f"[cgi] remote_addr  : {remote}\n"
    f"[cgi] server_time  : {now}\n"
    f"[cgi] exit status  : 0\n"
)
print(body, end="")
