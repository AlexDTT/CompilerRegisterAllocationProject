#!/usr/bin/env bash

set -euo pipefail

BINARY="./register_alloc"
TMP_DIR="tests/output/generated"
EXPECTED_DIR="tests/output/expected"
PASS=0
FAIL=0

mkdir -p "$TMP_DIR"

run_case() {
    local name="$1"
    local ranges="$2"
    local config="$3"
    local expected="$4"
    local output="$TMP_DIR/${name}.txt"

    echo -n "  [$name] ... "

    if ! "$BINARY" -b "$ranges" "$config" "$output" >/dev/null 2>&1; then
        echo "FAIL (non-zero exit)"
        FAIL=$((FAIL + 1))
        return
    fi

    if ! cmp -s "$output" "$expected"; then
        echo "FAIL (output mismatch)"
        diff -u "$expected" "$output" || true
        FAIL=$((FAIL + 1))
        return
    fi

    echo "PASS"
    PASS=$((PASS + 1))
}

echo "=== Integration Tests ==="

run_case "basic_ranges1" \
    "inputs/basic/ranges/ranges1.txt" \
    "inputs/basic/registers/registers2.txt" \
    "$EXPECTED_DIR/basic_ranges1.txt"

run_case "basic_ranges2" \
    "inputs/basic/ranges/ranges2.txt" \
    "inputs/basic/registers/registers2.txt" \
    "$EXPECTED_DIR/basic_ranges2.txt"

run_case "basic_ranges3" \
    "inputs/basic/ranges/ranges3.txt" \
    "inputs/basic/registers/registers2.txt" \
    "$EXPECTED_DIR/basic_ranges3.txt"

run_case "basic_ranges4" \
    "inputs/basic/ranges/ranges4.txt" \
    "inputs/basic/registers/registers1.txt" \
    "$EXPECTED_DIR/basic_ranges4.txt"

run_case "basic_ranges5" \
    "inputs/basic/ranges/ranges5.txt" \
    "inputs/basic/registers/registers1.txt" \
    "$EXPECTED_DIR/basic_ranges5.txt"

run_case "basic_ranges6" \
    "inputs/basic/ranges/ranges6.txt" \
    "inputs/basic/registers/registers3.txt" \
    "$EXPECTED_DIR/basic_ranges6.txt"

run_case "spilling_triangle" \
    "tests/input/spilling_triangle_ranges.txt" \
    "tests/input/spilling_triangle_config.txt" \
    "$EXPECTED_DIR/spilling_triangle.txt"

run_case "splitting_bridge" \
    "tests/input/splitting_bridge_ranges.txt" \
    "tests/input/splitting_bridge_config.txt" \
    "$EXPECTED_DIR/splitting_bridge.txt"

run_case "free_triangle" \
    "tests/input/free_triangle_ranges.txt" \
    "tests/input/free_triangle_config.txt" \
    "$EXPECTED_DIR/free_triangle.txt"

echo "========================="
echo "Passed: $PASS  Failed: $FAIL"

[[ $FAIL -eq 0 ]]
