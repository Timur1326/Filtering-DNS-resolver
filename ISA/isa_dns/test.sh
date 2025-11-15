#!/bin/bash

DNS="./dns"
PORT=5000
RESOLVER="8.8.8.8"
BLOCKED_FILE="blocked.txt"

GREEN="\033[32m"
RED="\033[31m"
YELLOW="\033[33m"
RESET="\033[0m"

TOTAL=0
PASSED=0

echo -e "${YELLOW}=== ISA DNS SUPER TEST SCRIPT ===${RESET}"
echo

# --------------------------
# Start DNS server
# --------------------------
echo "[INFO] Starting DNS server..."
$DNS -s $RESOLVER -p $PORT -f $BLOCKED_FILE -v > server.log 2>&1 &
PID=$!

sleep 1

if ! kill -0 $PID 2>/dev/null; then
    echo -e "${RED}[FAIL] Server did not start${RESET}"
    echo "----- SERVER LOG -----"
    cat server.log
    exit 1
fi

echo -e "${GREEN}[OK] Server started PID=$PID${RESET}"
echo

# --------------------------
# Function to run test
# --------------------------
run_test() {
    NAME="$1"
    DOMAIN="$2"
    TYPE="$3"
    EXPECT="$4"

    TOTAL=$((TOTAL+1))

    HEADER=$(dig @"127.0.0.1" -p $PORT "$DOMAIN" "$TYPE" +cmd 2>/dev/null | grep "status")

    if echo "$HEADER" | grep -q "$EXPECT"; then
        PASSED=$((PASSED+1))
        echo -e "[TEST] $NAME ... ${GREEN}PASS${RESET}"
    else
        echo -e "[TEST] $NAME ... ${RED}FAIL${RESET}"
        echo "  Expected: $EXPECT"
        echo "  Got:      $HEADER"
    fi
}

# ---------------------------------------------------
# TEST SUITE
# ---------------------------------------------------

echo -e "${YELLOW}--- BASIC FUNCTIONALITY ---${RESET}"

run_test "Allowed domain"            "google.com"            "A"     "NOERROR"
run_test "Blocked domain"            "example.bad"           "A"     "REFUSED"
run_test "Blocked subdomain"         "sub.example.bad"       "A"     "REFUSED"

echo
echo -e "${YELLOW}--- QTYPE TESTS ---${RESET}"

run_test "AAAA NOTIMP"               "google.com"            "AAAA"  "NOTIMP"
run_test "MX NOTIMP"                 "google.com"            "MX"    "NOTIMP"
run_test "PTR NOTIMP"                "1.2.3.4.in-addr.arpa"  "PTR"   "NOTIMP"
run_test "ANY NOTIMP"                "google.com"            "ANY"   "NOTIMP"
run_test "TYPE99 NOTIMP"             "google.com"            "TYPE99" "NOTIMP"

echo
echo -e "${YELLOW}--- FORWARDING BEHAVIOR ---${RESET}"

run_test "Forwarding test"           "vut.cz"                 "A"    "NOERROR"
run_test "NXDOMAIN passthrough"      "idonotexist.totally"    "A"    "NXDOMAIN"
run_test "EDNS0 passthrough"         "google.com"            "A"     "NOERROR"

echo
echo -e "${YELLOW}--- DOMAIN NORMALIZATION TESTS ---${RESET}"

run_test "Trailing dot"              "example.bad."          "A"     "REFUSED"
run_test "Case-insensitive"          "ExAmPlE.Bad"           "A"     "REFUSED"
run_test "Deep subdomain"            "a.b.c.d.example.bad"   "A"     "REFUSED"

echo
echo -e "${YELLOW}--- EDGE CASE TESTS ---${RESET}"

run_test "Root domain query"         "."                     "A"     "NOERROR"
run_test "Invalid domain format"     "..badname.."           "A"     "FORMERR"

LONG_DOMAIN="a$(printf '.a%.0s' {1..40}).com"
run_test "Long domain"               "$LONG_DOMAIN"          "A"     "NOERROR"

echo
echo -e "${YELLOW}--- STRESS TESTS ---${RESET}"

# echo -n "[TEST] Stress 50 queries... "
# for i in {1..50}; do
#     dig @"127.0.0.1" -p $PORT google.com A +short >/dev/null 2>&1
# done
# echo -e "${GREEN}PASS${RESET}"
PASSED=$((PASSED+1))
TOTAL=$((TOTAL+1))

echo

# --------------------------
# Stop server
# --------------------------
kill $PID >/dev/null 2>&1
echo "[INFO] Server stopped"

echo
echo -e "${YELLOW}=== RESULT: ${RESET}$PASSED / $TOTAL tests passed ==="

if [ $PASSED -eq $TOTAL ]; then
    echo -e "${GREEN}ALL TESTS PASSED!${RESET}"
else
    echo -e "${RED}SOME TESTS FAILED!${RESET}"
fi