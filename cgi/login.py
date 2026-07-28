#!/usr/bin/env python3
import sys, os
from urllib.parse import parse_qs
from users_store import load_users

length = int(os.environ.get("CONTENT_LENGTH", 0))
params = parse_qs(sys.stdin.read(length))
username = params.get("username", [""])[0]
password = params.get("password", [""])[0]

users = load_users()
user = users.get(username)

if user and user["password"] == password:
    print("Status: 200 OK\r\nContent-Type: application/json\r\nX-Auth-Token: " + user["token"] + "\r\n")
    print('{"username": "%s"}' % username)
else:
    print("Status: 401 Unauthorized\r\nContent-Type: application/json\r\n")
    print('{"error": "invalid credentials"}')