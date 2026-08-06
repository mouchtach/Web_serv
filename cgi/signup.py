#!/usr/bin/env python3
import sys, os, json, secrets

USERS_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "users.json")

def load_users():
    if not os.path.exists(USERS_FILE):
        return {}
    with open(USERS_FILE, "r") as f:
        try:
            return json.load(f)
        except ValueError:
            return {}

def save_users(users):
    with open(USERS_FILE, "w") as f:
        json.dump(users, f)

def respond(status_line, body_dict):
    print("Status: %s\r\nContent-Type: application/json\r\n" % status_line)
    print(json.dumps(body_dict))

length = int(os.environ.get("CONTENT_LENGTH", 0))
raw = sys.stdin.read(length)

from urllib.parse import parse_qs
params = parse_qs(raw)
username = params.get("username", [""])[0]
password = params.get("password", [""])[0]

if not username or not password:
    respond("400 Bad Request", {"error": "username and password required"})
    sys.exit(0)

users = load_users()

if username in users:
    respond("409 Conflict", {"error": "user already exists"})
    sys.exit(0)

token = secrets.token_hex(16)
users[username] = {"password": password, "token": token}
save_users(users)

respond("200 OK", {"username": username})