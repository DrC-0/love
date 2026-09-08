#!/bin/sh
# 指定した計算ジョブを停止する。
# 新しい対象は target_rules() の一覧に「対象名<TAB>pgrep -f の正規表現」を追加するだけでよい。

set -u

target_rules() {
  cat <<'RULES'
# target<TAB>pgrep -f 用の拡張正規表現
cfrorg	(^|[[:space:]/])cfrorg\.sh([[:space:]]|$)
cfrorg	(^|[[:space:]/])cfrorgupl\.sh([[:space:]]|$)
cfrorg	xargs[[:space:]].*-a[[:space:]]all_subgames\.txt
cfrorg	(^|[[:space:]])(\./|/[^[:space:]]*/)cfrorg([[:space:]]|$)
win	(^|[[:space:]/])win\.sh([[:space:]]|$)
win	(^|[[:space:]/])winupl\.sh([[:space:]]|$)
win	(^|[[:space:]])(\./|/[^[:space:]]*/)win([[:space:]]|$)
RULES
}

usage() {
  echo "Usage: $0 [--force] TARGET [TARGET ...]"
  echo "Targets:"
  target_rules | awk -F '\t' '!/^#/ && NF >= 2 && !seen[$1]++ { print "  " $1 }'
}

force=false
if [ "${1:-}" = "--force" ]; then
  force=true
  shift
fi

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ] || [ "$#" -eq 0 ]; then
  usage
  exit 0
fi

tmp_pids=$(mktemp "${TMPDIR:-/tmp}/stop_cfr_jobs.XXXXXX") || exit 1
trap 'rm -f "$tmp_pids"' 0 1 2 15

for target in "$@"; do
  patterns=$(target_rules | awk -F '\t' -v target="$target" '$1 == target { print $2 }')
  if [ -z "$patterns" ]; then
    echo "未知の対象です: $target" >&2
    usage >&2
    exit 2
  fi
  printf '%s\n' "$patterns" | while IFS= read -r pattern; do
    pgrep -f -- "$pattern" >> "$tmp_pids" || true
  done
done

# `sh stop_cfr_jobs.sh cfrorg` 自身のコマンド行が対象名を含んでも、自分自身は止めない。
grep -vx "$$" "$tmp_pids" > "$tmp_pids.filtered" || true
mv "$tmp_pids.filtered" "$tmp_pids"

if [ ! -s "$tmp_pids" ]; then
  echo "指定されたジョブは見つかりませんでした: $*"
  exit 0
fi

sort -nu "$tmp_pids" -o "$tmp_pids"
pids=$(tr '\n' ' ' < "$tmp_pids")
echo "停止対象: $pids"
ps -fp "$(tr '\n' ',' < "$tmp_pids" | sed 's/,$//')" || true

# PID のリストを意図的に展開する。
kill -TERM $pids 2>/dev/null || true
echo "SIGTERM を送信しました。"

if [ "$force" = true ]; then
  sleep 3
  remaining=''
  for pid in $pids; do
    if kill -0 "$pid" 2>/dev/null; then
      remaining="$remaining $pid"
    fi
  done
  if [ -n "$remaining" ]; then
    kill -KILL $remaining 2>/dev/null || true
    echo "残存プロセスへ SIGKILL を送信しました:$remaining"
  fi
fi
