#!/bin/bash
# ============================================================
# test_webserv.sh
# Test suite dyal webserv (bla CGI, bla timeout)
# Usage: ./test_webserv.sh [host] [port]
# Default: 127.0.0.1 8081
# ============================================================

HOST=${1:-127.0.0.1}
PORT=${2:-8081}
BASE="http://$HOST:$PORT"

PASS=0
FAIL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

# ------------------------------------------------------------
# helper: check(desc, expected_code, actual_code)
# ------------------------------------------------------------
check() {
    local desc="$1"
    local expected="$2"
    local actual="$3"
    if [ "$expected" == "$actual" ]; then
        echo -e "${GREEN}[PASS]${NC} $desc (got $actual)"
        PASS=$((PASS+1))
    else
        echo -e "${RED}[FAIL]${NC} $desc (expected $expected, got $actual)"
        FAIL=$((FAIL+1))
    fi
}

section() {
    echo ""
    echo -e "${YELLOW}==== $1 ====${NC}"
}

# ------------------------------------------------------------
# 0. Server reachability
# ------------------------------------------------------------
section "0. Connectivity"
code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 3 "$BASE/" 2>/dev/null)
if [ -z "$code" ] || [ "$code" == "000" ]; then
    echo -e "${RED}Server unreachable at $BASE — check it's running.${NC}"
    exit 1
fi
echo "Server reachable at $BASE"

# ------------------------------------------------------------
# 1. Basic GET
# ------------------------------------------------------------
section "1. GET basique"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/")
check "GET / (index or autoindex)" "200" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/index.html")
check "GET /index.html" "200" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/does-not-exist-xyz")
check "GET fichier inexistant -> 404" "404" "$code"

# ------------------------------------------------------------
# 2. Redirections (location /home -> return 301 /)
# ------------------------------------------------------------
section "2. Redirection"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/home")
check "GET /home -> 301" "301" "$code"

# ------------------------------------------------------------
# 3. Method not allowed
# ------------------------------------------------------------
section "3. Method Not Allowed"

code=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE/")
check "DELETE / (methods GET only) -> 405" "405" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" -X POST -d "test=data" "$BASE/static/")
check "POST /static (methods GET only) -> 405" "405" "$code"

# ------------------------------------------------------------
# 4. Path traversal (sécurité)
# ------------------------------------------------------------
section "4. Path Traversal"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/../../../../etc/passwd")
check "GET path traversal simple -> 400/403/404" "404" "$code" || true

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/static/../../../../etc/passwd")
check "GET path traversal via /static -> 400/403/404" "404" "$code" || true

echo -e "${YELLOW}(Note: si le status differe de 404 mais reste 400 ou 403, c'est aussi acceptable — verifier manuellement)${NC}"

# ------------------------------------------------------------
# 5. Autoindex
# ------------------------------------------------------------
section "5. Autoindex"

body=$(curl -s "$BASE/static/images/")
if echo "$body" | grep -qi "Index of"; then
    echo -e "${GREEN}[PASS]${NC} Autoindex genere une page 'Index of ...'"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${NC} Autoindex ne genere pas de page correcte"
    FAIL=$((FAIL+1))
fi

# ------------------------------------------------------------
# 6. Upload (POST multipart)
# ------------------------------------------------------------
section "6. POST Upload"

echo "Ceci est un fichier de test webserv." > "$TMPDIR/small.txt"

code=$(curl -s -o /dev/null -w "%{http_code}" -F "file=@$TMPDIR/small.txt" "$BASE/upload")
check "POST upload petit fichier -> 200" "200" "$code"

# ------------------------------------------------------------
# 7. Upload fichier volumineux + verification integrite (test du fix POLLOUT/send loop)
# ------------------------------------------------------------
section "7. Fichier volumineux (test POLLOUT loop)"

BIGFILE="$TMPDIR/big.bin"
# genere ~15MB de donnees aleatoires
dd if=/dev/urandom of="$BIGFILE" bs=1M count=15 status=none

code=$(curl -s -o /dev/null -w "%{http_code}" -F "file=@$BIGFILE;filename=big.bin" "$BASE/upload")
check "POST upload fichier 15MB -> 200" "200" "$code"

# telecharge le fichier depuis /download (ajuster le path si besoin)
curl -s -o "$TMPDIR/big_downloaded.bin" "$BASE/upload/big.bin"

if [ -f "$TMPDIR/big_downloaded.bin" ]; then
    size_dl=$(stat -c%s "$TMPDIR/big_downloaded.bin" 2>/dev/null || stat -f%z "$TMPDIR/big_downloaded.bin")
    size_orig=$(stat -c%s "$BIGFILE" 2>/dev/null || stat -f%z "$BIGFILE")
    if [ "$size_dl" == "$size_orig" ]; then
        echo -e "${GREEN}[PASS]${NC} Taille fichier telecharge == taille originale ($size_dl bytes)"
        PASS=$((PASS+1))
    else
        echo -e "${RED}[FAIL]${NC} Taille differente (original=$size_orig, telecharge=$size_dl) -> send() loop probablement casse"
        FAIL=$((FAIL+1))
    fi
else
    echo -e "${RED}[FAIL]${NC} Impossible de telecharger le fichier uploade"
    FAIL=$((FAIL+1))
fi

# ------------------------------------------------------------
# 8. Client max body size (413)
# ------------------------------------------------------------
section "8. Client Max Body Size"

HUGEFILE="$TMPDIR/huge.bin"
# suppose client_max_body_size est < 25MB dans la config (ajuster si besoin)
dd if=/dev/urandom of="$HUGEFILE" bs=1M count=25 status=none

code=$(curl -s -o /dev/null -w "%{http_code}" -F "file=@$HUGEFILE;filename=huge.bin" "$BASE/upload")
check "POST upload > client_max_body_size -> 413" "413" "$code"

# ------------------------------------------------------------
# 9. DELETE
# ------------------------------------------------------------
section "9. DELETE"

code=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE/upload/small.txt")
check "DELETE fichier existant -> 200" "200" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE/upload/inexistant.txt")
check "DELETE fichier inexistant -> 404" "404" "$code"

# ------------------------------------------------------------
# 10. Requetes malformees (400)
# ------------------------------------------------------------
section "10. Requetes malformees"

resp=$(printf "GARBAGE REQUEST\r\n\r\n" | timeout 3 nc "$HOST" "$PORT" 2>/dev/null | head -n1)
if echo "$resp" | grep -q "400"; then
    echo -e "${GREEN}[PASS]${NC} Requete malformee -> 400 ($resp)"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${NC} Requete malformee -> reponse inattendue: $resp"
    FAIL=$((FAIL+1))
fi

resp=$(printf "GET / HTTP/9.9\r\nHost: localhost\r\n\r\n" | timeout 3 nc "$HOST" "$PORT" 2>/dev/null | head -n1)
if echo "$resp" | grep -q "400"; then
    echo -e "${GREEN}[PASS]${NC} Version HTTP invalide -> 400 ($resp)"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${NC} Version HTTP invalide -> reponse inattendue: $resp"
    FAIL=$((FAIL+1))
fi

# ------------------------------------------------------------
# 11. Concurrence (plusieurs clients simultanes)
# ------------------------------------------------------------
section "11. Requetes concurrentes"

for i in $(seq 1 10); do
    curl -s -o /dev/null "$BASE/" &
done
wait
echo -e "${GREEN}[INFO]${NC} 10 requetes GET simultanees envoyees (verifier server ne crash pas / logs)"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/")
check "Serveur toujours vivant apres charge concurrente" "200" "$code"

# ------------------------------------------------------------
# Resume
# ------------------------------------------------------------
section "RESUME"
echo -e "${GREEN}PASS: $PASS${NC}"
echo -e "${RED}FAIL: $FAIL${NC}"

if [ "$FAIL" -eq 0 ]; then
    echo -e "${GREEN}Tous les tests sont passes.${NC}"
    exit 0
else
    echo -e "${RED}Certains tests ont echoue, verifier les logs ci-dessus.${NC}"
    exit 1
fi
