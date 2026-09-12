#!/bin/bash
# 回帰ハーネス。改修の前後で cfrorg と win の出力が変わっていないことを示す。
#
# 使い方:
#   ./regress.sh save [quick]    改修前に基準を取る
#   ./regress.sh check [quick]   改修後に基準と突き合わせる
#
#   quick を付けると cfrorg の打ち切り深さ 7 の2本だけ (約3秒)。
#   付けなければ cfrorg の 7 と 6、および win の2本も回す (約3分30秒)。
#
# 結果の置き場:
#   logs/regress/base/       基準。save で作り、check で読む
#   logs/regress/mismatch/   不一致だったときの実際の出力
#   logs/regress/report.txt  check の結果
#
# 比較の仕方:
#   cfrorg    標準出力の全文
#   cfrorgcnt 決定的カウンタ (判定関数の呼び出し回数 22 個)。stdout より判定の
#             内部動作に近く、CW_BUMP は関数入口にあるのでファイル分割や
#             引数の規約変更では値が動かないはず。統計が偶然一致する変更も
#             ここで捕まる。
#   win       標準出力の全文 + 生成される abs/abs<部分ゲーム>.bin の sha256
#           (bin は 46〜104MB あるので基準には sha256 だけ置く。不一致のときだけ
#            実物を mismatch/ に残す)
#
# 注意:
#   - cfrorg は打ち切り深さが必須。end_deck_n=0 は 830 秒で伏せ札の分岐 8 本の
#     うち 1 本も終わらない。win には打ち切りの手段が無いのでコストは固定。
#   - 部分ゲーム 446 は外せない。557 は開示3枚が {5,5,7} で魔術師 2 枚が
#     どちらも取り除かれるため、wiz_win / wiz_lose / ef_wizard を一度も通らない。
#   - win は abs/abs<部分ゲーム>.bin を上書きする。このスクリプトは実行前に
#     退避し、終了時 (中断時も) に必ず戻す。

set -u

LOG_DIR=logs/regress
BASE_DIR=$LOG_DIR/base
MISMATCH_DIR=$LOG_DIR/mismatch
REPORT=$LOG_DIR/report.txt

# 打ち切り深さつき cfrorg: "<3枚> <打ち切り深さ>"
CFRORG_QUICK=("5 5 7 7" "4 4 6 7")
CFRORG_SLOW=("5 5 7 6" "4 4 6 6")
# win: "<3枚>"
WIN_CASES=("5 5 7" "4 4 6")

ABS_STASH=""

restore_abs() {
    [ -n "$ABS_STASH" ] || return 0
    [ -d "$ABS_STASH" ] || return 0
    for f in "$ABS_STASH"/*.bin; do
        [ -e "$f" ] || continue
        mv -f "$f" "abs/$(basename "$f")"
    done
    rmdir "$ABS_STASH" 2>/dev/null
    ABS_STASH=""
}
trap restore_abs EXIT INT TERM

usage() {
    echo "usage: $0 {save|check} [quick]" >&2
    exit 2
}

mode=${1:-}
scope=${2:-full}
case "$mode" in
    save|check) ;;
    *) usage ;;
esac
case "$scope" in
    quick|full) ;;
    *) usage ;;
esac

# --- 比較するバイナリを必ず作り直す ---
if ! make cfrorg cfrorgcnt win >/dev/null; then
    echo "ビルドに失敗した" >&2
    exit 1
fi

mkdir -p "$LOG_DIR"
outdir=$BASE_DIR
[ "$mode" = check ] && outdir=$(mktemp -d "$LOG_DIR/cur.XXXXXX")
mkdir -p "$outdir"

fail=0
report=""

note() {
    report="$report$1"$'\n'
    echo "$1"
}

compare() {  # compare <名前> <今回のファイル> <基準のファイル>
    local name=$1 cur=$2 base=$3
    if [ ! -e "$base" ]; then
        note "MISSING  $name  (基準が無い。先に save を実行する)"
        fail=1
    elif cmp -s "$cur" "$base"; then
        note "OK       $name"
    else
        note "DIFFER   $name"
        mkdir -p "$MISMATCH_DIR"
        cp "$cur" "$MISMATCH_DIR/$(basename "$cur")"
        fail=1
    fi
}

# --- cfrorg ---
cases=("${CFRORG_QUICK[@]}")
[ "$scope" = full ] && cases+=("${CFRORG_SLOW[@]}")

for c in "${cases[@]}"; do
    set -- $c
    name="cfrorg-$1$2$3-$4"
    ./cfrorg "$1" "$2" "$3" "$4" > "$outdir/$name.out" 2>/dev/null
    ./cfrorgcnt "$1" "$2" "$3" "$4" 2>&1 >/dev/null | grep '^COUNT_WORK' > "$outdir/$name.cnt"
    if [ "$mode" = check ]; then
        compare "$name-stdout" "$outdir/$name.out" "$BASE_DIR/$name.out"
        compare "$name-counters" "$outdir/$name.cnt" "$BASE_DIR/$name.cnt"
    else
        echo "saved    $name"
    fi
done

# --- win ---
if [ "$scope" = full ]; then
    ABS_STASH=$(mktemp -d "$LOG_DIR/absstash.XXXXXX")
    mkdir -p abs
    for c in "${WIN_CASES[@]}"; do
        set -- $c
        sg="$1$2$3"
        name="win-$sg"
        binfile="abs/abs$sg.bin"

        # 既存の実験結果を退避してから走らせる
        [ -e "$binfile" ] && mv "$binfile" "$ABS_STASH/abs$sg.bin"

        ./win "$1" "$2" "$3" > "$outdir/$name.out" 2>/dev/null

        if [ -e "$binfile" ]; then
            sha256sum < "$binfile" > "$outdir/$name.abs.sha256"
            # 不一致のときに実物が要るので、比較まで消さない
            mv "$binfile" "$outdir/abs$sg.bin"
        else
            echo "(abs bin was not produced)" > "$outdir/$name.abs.sha256"
        fi

        if [ "$mode" = check ]; then
            compare "$name-stdout" "$outdir/$name.out" "$BASE_DIR/$name.out"
            compare "$name-absbin" "$outdir/$name.abs.sha256" "$BASE_DIR/$name.abs.sha256"
        else
            echo "saved    $name"
        fi
        # 基準に 100MB 級の bin は置かない。sha256 だけ残す
        rm -f "$outdir/abs$sg.bin"
    done
    restore_abs
fi

if [ "$mode" = save ]; then
    date +%s > "$BASE_DIR/taken_at"
    git rev-parse HEAD > "$BASE_DIR/git_head" 2>/dev/null
    echo
    echo "基準を $BASE_DIR に取った ($scope)。"
    exit 0
fi

{
    echo "# 回帰ハーネス $(date '+%F %T')  scope=$scope"
    echo "# 基準: $(cat "$BASE_DIR/git_head" 2>/dev/null) $(date -d "@$(cat "$BASE_DIR/taken_at" 2>/dev/null)" '+%F %T' 2>/dev/null)"
    echo "# 現在: $(git rev-parse HEAD 2>/dev/null)"
    printf '%s' "$report"
} > "$REPORT"

rm -rf "$outdir"

echo
if [ "$fail" -eq 0 ]; then
    echo "一致。出力は変わっていない。"
else
    echo "不一致がある。実際の出力は $MISMATCH_DIR/ に残した。"
fi
exit "$fail"
