#!/bin/bash

set -euo pipefail

tmp_files=()

make_tmp() {
    local var_name="$1"
    local tmp
    tmp=$(mktemp)
    tmp_files+=("$tmp")
    printf -v "$var_name" '%s' "$tmp"
}

write_total_csv() {
    local input_file="$1"
    local output_file="$2"
    local last_col="$3"

    awk -F, -v last_col="$last_col" '
        NR == 1 { print $0; next }
        {
            print $0
            for (i = 2; i <= last_col; i++) total[i] += $i
        }
        END {
            printf "合計"
            for (i = 2; i <= last_col; i++) printf ",%d", total[i]
            print ""
        }
    ' "$input_file" > "$output_file"
}

show_csv() {
    local output_file="$1"

    echo "=== $output_file ==="
    cat "$output_file"
}

trap 'rm -f "${tmp_files[@]}"' EXIT

OUTPUT_WIN_ACT="org_win_points.csv"
OUTPUT_LOSE_MOVE="org_lose_points.csv"
OUTPUT_DECISION_POINTS="org_wl_points.csv"

make_tmp TMP_WIN_ACT
make_tmp TMP_LOSE_MOVE
make_tmp TMP_DECISION_POINTS

echo "必勝意思決定点数,0,1,2,3,4,5,6,7,8,9,10" > "$TMP_WIN_ACT"
echo "必敗意思決定点数,0,1,2,3,4,5,6,7,8,9,10" > "$TMP_LOSE_MOVE"
echo "めぐる意思決定点数,0,1,2,3" > "$TMP_DECISION_POINTS"

while read -r p1 p2 p3 || [ -n "$p1" ]; do
    [ -z "$p1" ] && continue
    [[ "$p1" =~ ^# ]] && continue

    log_file="logs/org/log/${p1}${p2}${p3}.log"
    [ -f "$log_file" ] || continue

    id="${p1}${p2}${p3}"
    res=$(awk '
        /win points:[[:space:]]*$/ {
            getline
            win_act = $1 "," $2 "," $3 "," $4 "," $5 "," $6 "," $7 "," $8 "," $9 "," $10 "," $11
        }
        /lose points:[[:space:]]*$/ {
            getline
            lose_move = $1 "," $2 "," $3 "," $4 "," $5 "," $6 "," $7 "," $8 "," $9 "," $10 "," $11
        }
        /decision_points[[:space:]]*:[[:space:]]*$/ {
            getline
            decision_points = $1 "," $2 "," $3 "," $4
        }
        END {
            print win_act "|" lose_move "|" decision_points
        }
    ' "$log_file")

    win_act="${res%%|*}"
    rest="${res#*|}"
    lose_move="${rest%%|*}"
    decision_points="${rest#*|}"

    [ -n "$win_act" ] || continue
    [ -n "$lose_move" ] || continue
    [ -n "$decision_points" ] || continue

    echo "$id,$win_act" >> "$TMP_WIN_ACT"
    echo "$id,$lose_move" >> "$TMP_LOSE_MOVE"
    echo "$id,$decision_points" >> "$TMP_DECISION_POINTS"
done < subgames.txt

write_total_csv "$TMP_WIN_ACT" "$OUTPUT_WIN_ACT" 12
write_total_csv "$TMP_LOSE_MOVE" "$OUTPUT_LOSE_MOVE" 12
write_total_csv "$TMP_DECISION_POINTS" "$OUTPUT_DECISION_POINTS" 5

show_csv "$OUTPUT_WIN_ACT"
echo ""
show_csv "$OUTPUT_LOSE_MOVE"
echo ""
show_csv "$OUTPUT_DECISION_POINTS"
