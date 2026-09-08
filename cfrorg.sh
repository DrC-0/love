#!/bin/bash
# 部分ゲーム92本を12並列で流す。
#
# 結果の置き場:
#   logs/org/log/<部分ゲーム>.log    標準出力と標準エラー
#   logs/org/time/<部分ゲーム>.time  経過秒 CPU秒 最大RSS(KB)
#   logs/org/summary.txt             所要時間の降順にまとめたもの
#   logs/org/started_at              開始時刻 (epoch)
#   logs/org/finished_at             終了時刻 (epoch)
#   logs/org/run.log                 このスクリプト自身の出力
#
# 普段は summary.txt だけ見れば済む。個別のログが要るときだけ log/ に降りる。

LOG_DIR=logs/org

summarize() {
    local start end
    start=$(cat "$LOG_DIR/started_at" 2>/dev/null)
    end=$(cat "$LOG_DIR/finished_at" 2>/dev/null)
    {
        echo "# 部分ゲーム 経過秒 CPU秒 最大RSS(KB)"
        for f in "$LOG_DIR"/time/*.time; do
            [ -e "$f" ] || continue
            printf '%s %s\n' "$(basename "$f" .time)" "$(tr '\n' ' ' < "$f")"
        done | sort -k2 -rn
        if [ -n "$start" ] && [ -n "$end" ]; then
            echo "# 全体 $((end - start)) 秒 ($(date -d "@$start" '+%F %T') 〜 $(date -d "@$end" '+%F %T'))"
        fi
    } > "$LOG_DIR/summary.txt"
}

run_cfrorg() {
    mkdir -p "$LOG_DIR/log" "$LOG_DIR/time"
    date +%s > "$LOG_DIR/started_at"

    # $0 に LOG_DIR を渡し、$1 $2 $3 が部分ゲームの3枚になる
    xargs -a all_subgames.txt -n 3 -P 12 sh -c '
        /usr/bin/time -f "%e %U %M" -o "$0/time/$1$2$3.time" \
            ./cfrorg "$@" > "$0/log/$1$2$3.log" 2>&1
    ' "$LOG_DIR"

    date +%s > "$LOG_DIR/finished_at"
    summarize
}

mkdir -p "$LOG_DIR"

if [ "${1:-}" = "--foreground" ]; then
    run_cfrorg
else
    (
        trap '' HUP
        run_cfrorg
    ) > "$LOG_DIR/run.log" 2>&1 &
fi
