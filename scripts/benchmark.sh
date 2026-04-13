#!/bin/bash
# Usage: ./scripts/benchmark.sh <master_bin> <build_dir> "<strategies>"
set -euo pipefail

MASTER_BIN="$1"
BUILD_DIR="$2"
STRATEGIES="$3"
ROUNDS="${4:-10}"
BOARD_W="${5:-20}"
BOARD_H="${6:-20}"
TIMEOUT="${7:-10}"

declare -A TOTAL_SCORES

for strat in $STRATEGIES; do
    TOTAL_SCORES["$strat"]=0
done

PLAYER_ARGS=""
for strat in $STRATEGIES; do
    PLAYER_ARGS="$PLAYER_ARGS -p ./$BUILD_DIR/$strat"
done

for round in $(seq 1 "$ROUNDS"); do
    OUTPUT=$("$MASTER_BIN" -w "$BOARD_W" -h "$BOARD_H" -t "$TIMEOUT" -s "$round" $PLAYER_ARGS 2>&1 >/dev/null || true)

    while IFS= read -r line; do
        if [[ "$line" =~ Player\ ([^\ ]+)\ \([0-9]+\):\ score=([0-9]+) ]]; then
            name="${BASH_REMATCH[1]}"
            score="${BASH_REMATCH[2]}"
            if [[ -v "TOTAL_SCORES[$name]" ]]; then
                TOTAL_SCORES["$name"]=$(( TOTAL_SCORES["$name"] + score ))
            fi
        fi
    done <<< "$OUTPUT"
done

for strat in $STRATEGIES; do
    echo "  $strat: ${TOTAL_SCORES[$strat]}" >&2
done

best_name=""
best_score=-1
for strat in $STRATEGIES; do
    if (( TOTAL_SCORES["$strat"] > best_score )); then
        best_score=${TOTAL_SCORES["$strat"]}
        best_name="$strat"
    fi
done

echo "$best_name"
