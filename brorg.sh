#!/bin/bash
# 部分ゲーム91本に対して最適反応 (brorg) を 12 並列で流す。
#
#   ./brorg 1 <反復> <3枚>       P1 が最適反応 (上界)
#   ./brorg 2 <反復> <3枚>       P2 が最適反応 (下界)
#
# CFR は回さない。str<部分ゲーム><反復>.bin が既にあることが前提で、
# 無い部分ゲームは実行せず終了状態 127 として記録する
# (作るのは ./cfr0 <3枚> と ./cfr 0 <反復> <3枚>)。
#
# brorg の2本はこの順に走らせること。utilities<部分ゲーム>.csv の1行を
# brorg 1 が "<反復>,<上界>" まで、brorg 2 が ",<下界>" と書き足して閉じる。
# cfrbrh.sh と違い cfr0 が csv を作り直さないので、同じ部分ゲームを
# 流し直すと行が増えていく。logs 側にはこの実行が書き足した分だけを控える。
#
# 結果の置き場:
#   logs/brorg/log/<部分ゲーム>.log        brorg 2本の標準出力と標準エラー
#   logs/brorg/time/<部分ゲーム>.time      本ごとに 本名 経過秒 CPU秒 最大RSS(KB)
#   logs/brorg/status/<部分ゲーム>.status  終了状態 (0 なら完走、127 なら str 不在)
#   logs/brorg/utilities/<部分ゲーム>.csv  この実行が書き足した "<反復>,<上界>,<下界>"
#   logs/brorg/summary.txt                 所要時間の降順にまとめたもの
#   logs/brorg/started_at                  開始時刻 (epoch)
#   logs/brorg/finished_at                 終了時刻 (epoch)
#   logs/brorg/run.log                     このスクリプト自身の出力
#
# 普段は summary.txt だけ見れば済む。個別のログが要るときだけ log/ に降りる。
#
# summary.txt の「差」は上界 − 下界で、CFR の平均戦略がナッシュ均衡からどれだけ
# 離れているか (双対ギャップ)。理屈の上では必ず 0 以上なので、負の値が出たら
# どこかがおかしい。「終了状態」が 0 以外の行は途中で落ちているので、
# 上界・下界も信用できない。
#
# 環境変数:
#   PARALLEL  並列数 (既定 12)。brorg は情報集合表を丸ごと持つので1本でも
#             数GB使う。メモリの小さい機器では必ず下げる。
#   ITER      読む str の反復番号 (既定 64)
#   SUBGAMES  部分ゲームの一覧ファイル (既定 all_subgames.txt)

LOG_DIR=logs/brorg
PARALLEL=${PARALLEL:-12}
ITER=${ITER:-64}
SUBGAMES=${SUBGAMES:-all_subgames.txt}
export ITER

summarize() {
    local start end sg
    start=$(cat "$LOG_DIR/started_at" 2>/dev/null)
    end=$(cat "$LOG_DIR/finished_at" 2>/dev/null)
    {
        echo "# 部分ゲーム 経過秒 CPU秒 最大RSS(KB) 上界 下界 差 終了状態"
        for f in "$LOG_DIR"/time/*.time; do
            [ -e "$f" ] || continue
            sg=$(basename "$f" .time)
            # 経過秒と CPU 秒は2本の合計、RSS は2本の最大。上界・下界は
            # 控えた "<反復>,<上界>,<下界>" から取る。
            awk -v sg="$sg" \
                -v util="$(cat "$LOG_DIR/utilities/$sg.csv" 2>/dev/null)" \
                -v st="$(cat "$LOG_DIR/status/$sg.status" 2>/dev/null)" '
                # 異常終了すると /usr/bin/time が
                # "Command exited with non-zero status N" を混ぜるので、
                # 数字4列になっている行だけ足す
                $2 ~ /^[0-9]+(\.[0-9]+)?$/ && $4 ~ /^[0-9]+$/ {
                    elapsed += $2; cpu += $3; if($4 > rss) rss = $4
                }
                END {
                    if(split(util, u, ",") >= 3) {
                        up = u[2]; lo = u[3]; gap = u[2] - u[3];
                    } else {
                        up = "-"; lo = "-"; gap = "-";
                    }
                    printf "%s %.2f %.2f %d %s %s %s %s\n", \
                        sg, elapsed, cpu, rss, up, lo, gap, (st == "" ? "?" : st)
                }' "$f"
        done | sort -k2 -rn
        if [ -n "$start" ] && [ -n "$end" ]; then
            echo "# 全体 $((end - start)) 秒 ($(date -d "@$start" '+%F %T') 〜 $(date -d "@$end" '+%F %T'))"
        fi
        echo "# 反復 $ITER, 並列 $PARALLEL, 一覧 $SUBGAMES"
    } > "$LOG_DIR/summary.txt"
}

run_brorg() {
    mkdir -p "$LOG_DIR/log" "$LOG_DIR/time" "$LOG_DIR/status" "$LOG_DIR/utilities"
    date +%s > "$LOG_DIR/started_at"

    # $0 に LOG_DIR を渡し、$1 $2 $3 が部分ゲームの3枚になる。
    # 反復番号は ITER を export して渡す。
    xargs -a "$SUBGAMES" -n 3 -P "$PARALLEL" sh -c '
        sg="$1$2$3"
        if [ ! -f "str$sg$ITER.bin" ]; then
            echo "str$sg$ITER.bin が無いので飛ばした" > "$0/log/$sg.log"
            : > "$0/time/$sg.time"
            : > "$0/utilities/$sg.csv"
            echo 127 > "$0/status/$sg.status"
            exit 0
        fi
        # cfr0 が utilities を作り直さないので、追記される前の大きさを覚えて
        # おき、この実行が書き足した分だけを控える
        if [ -f "utilities$sg.csv" ]; then
            before=$(wc -c < "utilities$sg.csv")
        else
            before=0
        fi
        : > "$0/time/$sg.time"
        {
            /usr/bin/time -f "brorg1 %e %U %M" -a -o "$0/time/$sg.time" \
                ./brorg 1 "$ITER" "$@" &&
            /usr/bin/time -f "brorg2 %e %U %M" -a -o "$0/time/$sg.time" \
                ./brorg 2 "$ITER" "$@"
        } > "$0/log/$sg.log" 2>&1
        echo "$?" > "$0/status/$sg.status"
        tail -c "+$((before + 1))" "utilities$sg.csv" > "$0/utilities/$sg.csv" 2>/dev/null
    ' "$LOG_DIR"

    date +%s > "$LOG_DIR/finished_at"
    summarize
}

if [ ! -f "$SUBGAMES" ]; then
    echo "部分ゲームの一覧が無い: $SUBGAMES" >&2
    exit 1
fi

# ソースを直接叩く cfrbrh.sh と違い、ビルドは Makefile に任せる
if ! make brorg; then
    echo "ビルドに失敗した" >&2
    exit 1
fi

mkdir -p "$LOG_DIR"

if [ "${1:-}" = "--foreground" ]; then
    run_brorg
else
    (
        trap '' HUP
        run_brorg
    ) > "$LOG_DIR/run.log" 2>&1 &
fi
