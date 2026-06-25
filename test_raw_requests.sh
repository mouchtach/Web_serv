#!/bin/bash

# ============================================================
#  Raw HTTP GET Request Tests (uses nc/netcat)
#  These test the parser directly, not via curl.
#  Usage: ./test_raw_requests.sh [host] [port]
# ============================================================

HOST="${1:-127.0.0.1}"
PORT="${2:-8080}"

PASS=0
FAIL=0
TOTAL=0

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[0;33m"
CYAN="\033[0;36m"
RESET="\033[0m"

# Send a raw HTTP request and return the response
send_raw() {
    printf "%s" "$1" | nc -q 1 "$HOST" "$PORT" 2>/dev/null || \
    printf "%s" "$1" | nc -w 2 "$HOST" "$PORT" 2>/dev/null
}

# Extract status code from response (first line: "HTTP/1.1 200 OK")
get_status() {
    echo "$1" | head -1 | grep -o "[0-9][0-9][0-9]"
}

run_raw_test() {
    local description="$1"
    local expected_code="$2"
    local raw_request="$3"

    TOTAL=$((TOTAL + 1))
    response=$(send_raw "$raw_request")
    actual_code=$(get_status "$response")

    if [ "$actual_code" = "$expected_code" ]; then
        echo -e "  ${GREEN}[PASS]${RESET} [$actual_code] $description"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}[FAIL]${RESET} $description"
        echo -e "         expected ${YELLOW}$expected_code${RESET} | got ${RED}${actual_code:-NO_RESPONSE}${RESET}"
        FAIL=$((FAIL + 1))
    fi
}

section() {
    echo ""
    echo -e "${CYAN}══════════════════════════════════════════${RESET}"
    echo -e "${CYAN}  $1${RESET}"
    echo -e "${CYAN}══════════════════════════════════════════${RESET}"
}

echo ""
echo -e "${CYAN}Raw HTTP parser tests against ${HOST}:${PORT}${RESET}"
echo -e "${CYAN}(requires nc/netcat)${RESET}"
echo ""

# ============================================================
#  1. VALID REQUESTS
# ============================================================
section "1. Valid GET requests"

run_raw_test "Minimal valid GET /" "200" \
"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "GET /index.html HTTP/1.1" "200" \
"GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "GET /hello.txt HTTP/1.1" "200" \
"GET /hello.txt HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "GET with query string" "200" \
"GET /index.html?key=value HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "GET with multiple headers" "200" \
"GET / HTTP/1.1\r\nHost: localhost\r\nUser-Agent: TestClient/1.0\r\nAccept: text/html\r\n\r\n"

run_raw_test "GET with Connection: keep-alive" "200" \
"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"

run_raw_test "GET with Connection: close" "200" \
"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"

run_raw_test "HEAD / HTTP/1.1" "200" \
"HEAD / HTTP/1.1\r\nHost: localhost\r\n\r\n"

# ============================================================
#  2. BAD REQUESTS (400)
# ============================================================
section "2. Malformed requests --> 400"

run_raw_test "Missing Host header" "400" \
"GET / HTTP/1.1\r\n\r\n"

run_raw_test "No HTTP version" "400" \
"GET /\r\n\r\n"

run_raw_test "Bad method spelling (GTE)" "501" \
"GTE / HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "Lowercase method (get)" "501" \
"get / HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "Missing space before URI" "400" \
"GET/ HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "Invalid HTTP version (HTTP/2.0)" "400" \
"GET / HTTP/2.0\r\nHost: localhost\r\n\r\n"

run_raw_test "Path traversal above root" "400" \
"GET /../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "Path with null byte (invalid URI char)" "400" \
"$(printf 'GET /foo\x00bar HTTP/1.1\r\nHost: localhost\r\n\r\n')"

# ============================================================
#  3. URI TOO LONG (414)
# ============================================================
section "3. URI too long --> 414"

LONG=$(python3 -c "print('a' * 4097)")
run_raw_test "URI longer than 4096 bytes --> 414" "414" \
"GET /${LONG} HTTP/1.1\r\nHost: localhost\r\n\r\n"

# ============================================================
#  4. NOT IMPLEMENTED (501)
# ============================================================
section "4. Unknown methods --> 501"

run_raw_test "PATCH method --> 501" "501" \
"PATCH / HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "OPTIONS method --> 501" "501" \
"OPTIONS / HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "CONNECT method --> 501" "501" \
"CONNECT / HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "FOOBAR method --> 501" "501" \
"FOOBAR / HTTP/1.1\r\nHost: localhost\r\n\r\n"

# ============================================================
#  5. NOT FOUND (404)
# ============================================================
section "5. Missing resources --> 404"

run_raw_test "GET /nonexistent --> 404" "404" \
"GET /nonexistent HTTP/1.1\r\nHost: localhost\r\n\r\n"

run_raw_test "GET /a/b/c/d.html --> 404" "404" \
"GET /a/b/c/d.html HTTP/1.1\r\nHost: localhost\r\n\r\n"

# ============================================================
#  SUMMARY
# ============================================================
echo ""
echo -e "${CYAN}══════════════════════════════════════════${RESET}"
echo -e "${CYAN}  RESULTS${RESET}"
echo -e "${CYAN}══════════════════════════════════════════${RESET}"
echo -e "  Total : $TOTAL"
echo -e "  ${GREEN}Pass  : $PASS${RESET}"
echo -e "  ${RED}Fail  : $FAIL${RESET}"
echo ""
if [ "$FAIL" -eq 0 ]; then
    echo -e "  ${GREEN}All tests passed!${RESET}"
else
    echo -e "  ${RED}$FAIL test(s) failed.${RESET}"
fi
echo ""
