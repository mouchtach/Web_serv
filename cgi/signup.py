#!/usr/bin/env python3
import sys, os, secrets
from urllib.parse import parse_qs
from users_store import load_users, save_users

length = int(os.environ.get("CONTENT_LENGTH", 0))
params = parse_qs(sys.stdin.read(length))
username = params.get("username", [""])[0]
password = params.get("password", [""])[0]

users = load_users()

if not username or not password:
    print("Status: 400 Bad Request\r\nContent-Type: application/json\r\n")
    print('{"error": "username and password required"}')
elif username in users:
    print("Status: 409 Conflict\r\nContent-Type: application/json\r\n")
    print('{"error": "user already exists"}')
else:
    token = secrets.token_hex(16)
    users[username] = {"password": password, "token": token}
    save_users(users)
    print("Status: 200 OK\r\nContent-Type: application/json\r\nX-Auth-Token: " + token + "\r\n")
    print('{"username": "%s"}' % username)