#!/usr/bin/env python3
import os
import subprocess
import sys

CGI_DIR = os.path.dirname(os.path.abspath(__file__))

def respond(status_line, body, content_type="text/plain"):
    print("Status: %s\r\nContent-Type: %s\r\n" % (status_line, content_type))
    print(body)

# اسم السكريبت C جاي فـ QUERY_STRING، مثلا /cgi/run_c.py?file=c.c
query = os.environ.get("QUERY_STRING", "")
params = dict(p.split("=") for p in query.split("&") if "=" in p)
c_filename = params.get("file", "")

if not c_filename or not c_filename.endswith(".c"):
    respond("400 Bad Request", "missing or invalid 'file' parameter")
    sys.exit(0)

c_path = os.path.join(CGI_DIR, c_filename)
if not os.path.exists(c_path):
    respond("404 Not Found", "source file not found: " + c_filename)
    sys.exit(0)

bin_name = c_filename[:-2]              # "c.c" -> "c"
bin_path = os.path.join(CGI_DIR, bin_name)

# 1) كومپايلي إلا ماكانش الباينري، أو إلا كان قديم من المصدر
need_compile = (
    not os.path.exists(bin_path)
    or os.path.getmtime(c_path) > os.path.getmtime(bin_path)
)

if need_compile:
    compile_proc = subprocess.run(
        ["gcc", "-o", bin_path, c_path],
        capture_output=True, text=True
    )
    if compile_proc.returncode != 0:
        respond("500 Internal Server Error",
                "compile failed:\n" + compile_proc.stderr)
        sys.exit(0)

# 2) شغّل الباينري ونقل ليه المتغيرات ديال البيئة (بحال CONTENT_LENGTH)
length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(length) if length > 0 else ""

run_proc = subprocess.run(
    [bin_path],
    input=body,
    capture_output=True, text=True,
    env=os.environ.copy()
)

if run_proc.returncode != 0:
    respond("500 Internal Server Error",
            "program exited with code %d\n%s" % (run_proc.returncode, run_proc.stderr))
    sys.exit(0)

respond("200 OK", run_proc.stdout)