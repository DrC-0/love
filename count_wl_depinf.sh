#!/bin/bash

set -euo pipefail

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

tmp_files=()
trap 'rm -f "${tmp_files[@]}"' EXIT

OUTPUT_WIN="win_count.csv"
OUTPUT_LOSE="lose_count.csv"
OUTPUT_INFSET="infset_wl.csv"

make_tmp TMP_WIN
make_tmp TMP_LOSE
make_tmp TMP_INFSET

echo "必勝情報集合数,0,1,2,3,4,5,6,7,8,9,10" > "$TMP_WIN"
echo "必敗行動数,0,1,2,3,4,5,6,7,8,9,10" > "$TMP_LOSE"
echo "めぐる情報集合数,なし,必勝のみ,必敗のみ,両方" > "$TMP_INFSET"

while read -r p1 p2 p3 || [ -n "$p1" ]; do
    [ -z "$p1" ] && continue
    [[ "$p1" =~ ^# ]] && continue

    log_file="logs/win/log/${p1}${p2}${p3}.log"
    [ -f "$log_file" ] || continue

    id="${p1}${p2}${p3}"
    res=$(awk '
        /win move:/ {
            getline
            win_val = sprintf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)
        }
        /lose move:/ {
            getline
            lose_val = sprintf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)
        }
        /infset size by win\/lose:/ {
            getline
            infset_val = $1 "," $2 "," $3 "," $4
        }
        END {
            print win_val "|" lose_val "|" infset_val
        }
    ' "$log_file")

    win_res="${res%%|*}"
    rest="${res#*|}"
    lose_res="${rest%%|*}"
    infset_res="${rest#*|}"
    echo "$id,$win_res" >> "$TMP_WIN"
    echo "$id,$lose_res" >> "$TMP_LOSE"
    echo "$id,$infset_res" >> "$TMP_INFSET"
done < subgames.txt

write_total_csv "$TMP_WIN" "$OUTPUT_WIN" 12
write_total_csv "$TMP_LOSE" "$OUTPUT_LOSE" 12
write_total_csv "$TMP_INFSET" "$OUTPUT_INFSET" 5

show_csv "$OUTPUT_WIN"
echo ""
show_csv "$OUTPUT_LOSE"
echo ""
show_csv "$OUTPUT_INFSET"
