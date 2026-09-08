#!/bin/bash
# 集計した csv を Google スプレッドシートへ上げる。
# シート ID は環境ごとに違うので .env に置く (gitignore 済み)。

ENV_FILE="$(dirname "$0")/.env"
if [ -f "$ENV_FILE" ]; then . "$ENV_FILE"; fi
: "${SPREADSHEET_ID:?.env に SPREADSHEET_ID を設定してください (.env.example を参照)}"

./upload_csv.sh "win_count.csv" "$SPREADSHEET_ID" "win_count"
./upload_csv.sh "lose_count.csv" "$SPREADSHEET_ID" "lose_count"
./upload_csv.sh "infset_wl.csv" "$SPREADSHEET_ID" "infset_wl"

./upload_csv.sh "org_win_points.csv" "$SPREADSHEET_ID" "org_win_points"
./upload_csv.sh "org_lose_points.csv" "$SPREADSHEET_ID" "org_lose_points"
./upload_csv.sh "org_wl_points.csv" "$SPREADSHEET_ID" "org_wl_points"
