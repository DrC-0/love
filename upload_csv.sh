#!/bin/bash

# 引数の数をチェック (3つ必要)
if [ "$#" -ne 3 ]; then
  echo "エラー: 引数が足りません、または多すぎます。"
  echo "使い方: $0 <CSVファイルパス> <スプレッドシートID> <シート名>"
  echo "例: $0 data.csv 1AbC_dEfG...hIjKlMn シート1"
  exit 1
fi

# ==================== 引数から入力を受け取る ====================
CSV_FILE="$1"
SPREADSHEET_ID="$2"
SHEET_NAME="$3"
KEY_FILE="credentials.json"  # サービスアカウントの鍵ファイル
# =============================================================

# ファイルの存在チェック
if [ ! -f "$CSV_FILE" ]; then
  echo "エラー: CSVファイル '$CSV_FILE' が見つかりません。"
  exit 1
fi

if [ ! -f "$KEY_FILE" ]; then
  echo "エラー: サービスアカウントの鍵ファイル '$KEY_FILE' が見つかりません。"
  exit 1
fi

# 1. サービスアカウントのJSONから必要な情報を抽出
CLIENT_EMAIL=$(jq -r '.client_email' "$KEY_FILE")
PRIVATE_KEY=$(jq -r '.private_key' "$KEY_FILE")

# 2. JWT（認証用トークン）のヘッダーとペイロードを作成
HEADER_BASE64=$(echo -n '{"alg":"RS256","typ":"JWT"}' | openssl base64 -e | tr -d '\r\n=' | tr '/+' '_-')
IAT=$(date +%s)
EXP=$((IAT + 3600))
PAYLOAD=$(printf '{"iss":"%s","scope":"https://www.googleapis.com/auth/spreadsheets","aud":"https://oauth2.googleapis.com/token","iat":%d,"exp":%d}' "$CLIENT_EMAIL" "$IAT" "$EXP")
PAYLOAD_BASE64=$(echo -n "$PAYLOAD" | openssl base64 -e | tr -d '\r\n=' | tr '/+' '_-')

# 3. 秘密鍵で署名してJWTを完成させる
SIGNATURE=$(printf '%s.%s' "$HEADER_BASE64" "$PAYLOAD_BASE64" | openssl dgst -sha256 -sign <(echo "$PRIVATE_KEY") | openssl base64 -e | tr -d '\r\n=' | tr '/+' '_-')
JWT="${HEADER_BASE64}.${PAYLOAD_BASE64}.${SIGNATURE}"

# 4. Googleの認証サーバーからアクセストークンを取得
RESPONSE=$(curl -s -X POST https://oauth2.googleapis.com/token \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=urn:ietf:params:oauth:grant-type:jwt-bearer&assertion=$JWT")

ACCESS_TOKEN=$(echo "$RESPONSE" | jq -r '.access_token')

if [ "$ACCESS_TOKEN" == "null" ] || [ -z "$ACCESS_TOKEN" ]; then
  echo "エラー: アクセストークンの取得に失敗しました。"
  echo "$RESPONSE"
  exit 1
fi

# 5. CSVファイルの中身をGoogle Sheets API用のJSON（2次元配列）に変換
JSON_DATA=$(jq -Rs 'split("\n") | map(select(length > 0) | split(",")) | {values: .}' "$CSV_FILE")

# 6. スプレッドシートの既存データをクリア
curl -s -X POST "https://sheets.googleapis.com/v4/spreadsheets/$SPREADSHEET_ID/values/$SHEET_NAME:clear" \
  -H "Authorization: Bearer $ACCESS_TOKEN" \
  -H "Content-Type: application/json" > /dev/null

# 7. 新しいCSVデータを書き込み
UPLOAD_RESPONSE=$(curl -s -X PUT "https://sheets.googleapis.com/v4/spreadsheets/$SPREADSHEET_ID/values/$SHEET_NAME?valueInputOption=USER_ENTERED" \
  -H "Authorization: Bearer $ACCESS_TOKEN" \
  -H "Content-Type: application/json" \
  -d "$JSON_DATA")

# 結果確認
if echo "$UPLOAD_RESPONSE" | grep -q "updatedCells"; then
  echo "成功: '$CSV_FILE' のインポートが完了しました！"
else
  echo "エラー: アップロードに失敗しました。"
  echo "$UPLOAD_RESPONSE"
  exit 1
fi