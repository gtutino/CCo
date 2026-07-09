output=$(CCO_THREADS=1 "./test/src/$1")
expected_output=$(cat "test/output/$1.txt")

if [[ "$output" == "$expected_output" ]]; then
    echo "[TEST PASS]: $1"
else
    echo "[TEST FAIL]: $1"
fi
