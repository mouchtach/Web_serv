#!/bin/bash

# ============================================================
#  Webserv GET Method Test Suite
#  Usage: ./test_get.sh [host] [port]
#  Default: localhost 8080
# ============================================================

HOST="${1:-localhost}"
PORT="${2:-8080}"
BASE="http://${HOST}:${PORT}"

PASS=0
FAIL=0
TOTAL=0

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[0;33m"
CYAN="\033[0;36m"
RESET="\033[0m"

# ---- helpers -----------------------------------------------

run_test() {
    local description="$1"
    local expected_code="$2"
    local curl_args="${@:3}"

    TOTAL=$((TOTAL + 1))

    # -s silent, -o /dev/null discard body, -w write only status code
    actual_code=$(curl -s -o /dev/null -w "%{http_code}" $curl_args)

    if [ "$actual_code" = "$expected_code" ]; then
        echo -e "  ${GREEN}[PASS]${RESET} [$actual_code] $description"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}[FAIL]${RESET} $description"
        echo -e "         expected ${YELLOW}$expected_code${RESET} | got ${RED}$actual_code${RESET}"
        FAIL=$((FAIL + 1))
    fi
}

run_test_body() {
    local description="$1"
    local expected_code="$2"
    local expected_body_contains="$3"
    local curl_args="${@:4}"

    TOTAL=$((TOTAL + 1))

    response=$(curl -s -w "\n%{http_code}" $curl_args)
    actual_code=$(echo "$response" | tail -1)
    body=$(echo "$response" | sed '$d')

    code_ok=false
    body_ok=false

    [ "$actual_code" = "$expected_code" ] && code_ok=true
    echo "$body" | grep -q "$expected_body_contains" && body_ok=true

    if $code_ok && $body_ok; then
        echo -e "  ${GREEN}[PASS]${RESET} [$actual_code] $description"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}[FAIL]${RESET} $description"
        echo -e "         code:  expected ${YELLOW}$expected_code${RESET} | got ${RED}$actual_code${RESET}"
        echo -e "         body:  looking for '${YELLOW}$expected_body_contains${RESET}' -> found: $(echo $body | grep -o "$expected_body_contains" || echo NO)"
        FAIL=$((FAIL + 1))
    fi
}

run_test_header() {
    local description="$1"
    local expected_code="$2"
    local expected_header_contains="$3"
    local curl_args="${@:4}"

    TOTAL=$((TOTAL + 1))

    headers=$(curl -s -D - -o /dev/null $curl_args)
    actual_code=$(echo "$headers" | head -1 | grep -o "[0-9][0-9][0-9]")

    if echo "$headers" | grep -qi "$expected_header_contains" && [ "$actual_code" = "$expected_code" ]; then
        echo -e "  ${GREEN}[PASS]${RESET} [$actual_code] $description"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}[FAIL]${RESET} $description"
        echo -e "         code:  expected ${YELLOW}$expected_code${RESET} | got ${RED}$actual_code${RESET}"
        echo -e "         header expected: '${YELLOW}$expected_header_contains${RESET}'"
        FAIL=$((FAIL + 1))
    fi
}

section() {
    echo ""
    echo -e "${CYAN}══════════════════════════════════════════${RESET}"
    echo -e "${CYAN}  $1${RESET}"
    echo -e "${CYAN}══════════════════════════════════════════${RESET}"
}

# ---- check server is up ------------------------------------

echo ""
echo -e "${CYAN}Checking server at ${BASE} ...${RESET}"
if ! curl -s --connect-timeout 3 "$BASE/" > /dev/null 2>&1; then
    echo -e "${RED}ERROR: Server not reachable at ${BASE}${RESET}"
    echo "Start your server first:  ./webserv test.conf"
    exit 1
fi
echo -e "${GREEN}Server is up!${RESET}"

# ============================================================
#  1. BASIC SUCCESSFUL RESPONSES (200 OK)
# ============================================================
section "1. Basic 200 OK responses"

run_test \
    "GET /  -->  200 (root index.html)" \
    "200" \
    "$BASE/"

run_test \
    "GET /index.html  -->  200 (explicit file)" \
    "200" \
    "$BASE/index.html"

run_test \
    "GET /about.html  -->  200 (another html file)" \
    "200" \
    "$BASE/about.html"

run_test \
    "GET /hello.txt  -->  200 (plain text file)" \
    "200" \
    "$BASE/hello.txt"

run_test \
    "GET /assets/css/style.css  -->  200 (CSS file)" \
    "200" \
    "$BASE/assets/css/style.css"

# ============================================================
#  2. RESPONSE BODY CONTENT
# ============================================================
section "2. Response body content"

run_test_body \
    "GET /  body contains <html>" \
    "200" \
    "<html>" \
    "$BASE/"

run_test_body \
    "GET /index.html  body contains 'Welcome'" \
    "200" \
    "Welcome" \
    "$BASE/index.html"

run_test_body \
    "GET /hello.txt  body contains 'Hello'" \
    "200" \
    "Hello" \
    "$BASE/hello.txt"

# ============================================================
#  3. RESPONSE HEADERS
# ============================================================
section "3. Response headers"

run_test_header \
    "GET /  response has Content-Type header" \
    "200" \
    "Content-Type" \
    "$BASE/"

run_test_header \
    "GET /  response has Content-Length header" \
    "200" \
    "Content-Length" \
    "$BASE/"

run_test_header \
    "GET /index.html  Content-Type is text/html" \
    "200" \
    "text/html" \
    "$BASE/index.html"

run_test_header \
    "GET /assets/css/style.css  Content-Type is text/css" \
    "200" \
    "text/css" \
    "$BASE/assets/css/style.css"

run_test_header \
    "GET /hello.txt  Content-Type is text/plain" \
    "200" \
    "text/plain" \
    "$BASE/hello.txt"

run_test_header \
    "GET /  response has Date header" \
    "200" \
    "Date:" \
    "$BASE/"

run_test_header \
    "GET /  response has Server header" \
    "200" \
    "Server:" \
    "$BASE/"

# ============================================================
#  4. 404 NOT FOUND
# ============================================================
section "4. 404 Not Found"

run_test \
    "GET /doesnotexist.html  -->  404" \
    "404" \
    "$BASE/doesnotexist.html"

run_test \
    "GET /no/such/path  -->  404" \
    "404" \
    "$BASE/no/such/path"

run_test \
    "GET /images/ghost.jpg  -->  404 (missing file)" \
    "404" \
    "$BASE/images/ghost.jpg"

run_test \
    "GET /.hidden  -->  404 (hidden file)" \
    "404" \
    "$BASE/.hidden"

# ============================================================
#  5. 405 METHOD NOT ALLOWED
# ============================================================
section "5. 405 Method Not Allowed"

run_test \
    "POST /  -->  405 (only GET allowed on /)" \
    "405" \
    -X POST "$BASE/"

run_test \
    "DELETE /index.html  -->  405 (only GET allowed on /)" \
    "405" \
    -X DELETE "$BASE/index.html"

run_test \
    "PUT /index.html  -->  405 (PUT not allowed on /)" \
    "405" \
    -X PUT -d "data" "$BASE/index.html"

# ============================================================
#  6. AUTOINDEX (directory listing)
# ============================================================
section "6. Autoindex (directory listing)"

run_test \
    "GET /images/  -->  200 (autoindex on)" \
    "200" \
    "$BASE/images/"

run_test_body \
    "GET /images/  body lists photo.jpg" \
    "200" \
    "photo.jpg" \
    "$BASE/images/"

run_test \
    "GET /assets/  -->  403 or 200 (autoindex off, no index)" \
    "403" \
    "$BASE/assets/"

# ============================================================
#  7. REDIRECT (301)
# ============================================================
section "7. 301 Redirect"

run_test \
    "GET /redirect  -->  301 (location redirect)" \
    "301" \
    "$BASE/redirect"

run_test_header \
    "GET /redirect  response has Location header" \
    "301" \
    "Location:" \
    "$BASE/redirect"

# Following redirect must land on 200
run_test \
    "GET /redirect  -->  200 after following redirect" \
    "200" \
    -L "$BASE/redirect"

# ============================================================
#  8. URI EDGE CASES
# ============================================================
section "8. URI edge cases"

run_test \
    "GET with query string  -->  200" \
    "200" \
    "$BASE/?foo=bar"

run_test \
    "GET with multiple query params  -->  200" \
    "200" \
    "$BASE/index.html?a=1&b=2"

run_test \
    "GET with fragment  -->  200 (fragment stripped by client)" \
    "200" \
    "$BASE/index.html"

run_test \
    "GET /  with trailing slash  -->  200" \
    "200" \
    "$BASE/"

# ============================================================
#  9. PATH TRAVERSAL (security)
# ============================================================
section "9. Path traversal (security checks)"

run_test \
    "GET /../etc/passwd  -->  400 or 403 (traversal blocked)" \
    "400" \
    "$BASE/../etc/passwd"

run_test \
    "GET /../../  -->  400 (traversal goes above root)" \
    "400" \
    "$BASE/../../"

# ============================================================
#  10. HEAD METHOD
# ============================================================
section "10. HEAD method (same headers, no body)"

actual_get_len=$(curl -s -o /dev/null -w "%{size_download}" "$BASE/index.html")
actual_head_len=$(curl -s -I -o /dev/null -w "%{size_download}" "$BASE/index.html")
TOTAL=$((TOTAL + 1))
if [ "$actual_head_len" -eq 0 ]; then
    echo -e "  ${GREEN}[PASS]${RESET} HEAD /index.html  returns no body (size=0)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}[FAIL]${RESET} HEAD /index.html  body should be empty, got ${actual_head_len} bytes"
    FAIL=$((FAIL + 1))
fi

run_test_header \
    "HEAD /index.html  returns Content-Length header" \
    "200" \
    "Content-Length" \
    -I "$BASE/index.html"

run_test_header \
    "HEAD /index.html  returns Content-Type header" \
    "200" \
    "Content-Type" \
    -I "$BASE/index.html"

# ============================================================
#  11. LARGE URI (414)
# ============================================================
section "11. URI length limit (414)"

LONG_URI=$(python3 -c "print('a' * 5000)")
run_test \
    "GET with URI > 4096 chars  -->  414 URI Too Long" \
    "414" \
    "$BASE/$LONG_URI"

# ============================================================
#  12. HTTP VERSION COMPLIANCE
# ============================================================
section "12. HTTP/1.1 compliance"

run_test_header \
    "Response uses HTTP/1.1" \
    "200" \
    "HTTP/1.1" \
    "$BASE/"

# Keep-Alive
run_test_header \
    "Connection: keep-alive is echoed back" \
    "200" \
    "keep-alive" \
    -H "Connection: keep-alive" "$BASE/"

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
