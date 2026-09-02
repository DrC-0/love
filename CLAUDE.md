# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

不完全情報ゲーム「ラブレター」(日本語版) の解析コード。CFR による戦略計算、最適反応による搾取量計算、必勝/必敗判定による情報集合の削減を行う。ソースはリポジトリ直下のフラット配置で、ロジックはほぼ `.hpp` に直接書かれ、`.cpp` は `main` とグローバル変数の定義だけを持つ。

## ゲームのルール（コード上の前提）

- 手札は常に1枚。手番で山札から1枚引き、2枚から1枚を出してその効果を適用する。山札が尽きたら手札の大きい方が勝ち。
- カードは 1〜8 の数値そのままで扱い、配列の添字は `card - 1`。枚数は `max_num[8] = {5,2,2,2,2,1,1,1}`。
- 1 兵士: 相手の手札を宣言する（1 は宣言できない）。的中なら勝ち。
- 2 道化: 相手の手札を見る。
- 3 騎士: 手札を比べ、小さい方が負け（同値なら何も起きない）。
- 4 僧侶: 次の自分の手番まで相手の効果の対象にならない（コード上は `barrier_s` / `barrier_e`）。
- 5 魔術師: 自分か相手の手札を捨てさせ、山札から引き直させる。姫(8)を捨てさせたら勝ち。
- 6 将軍: 相手と手札を交換する。
- 7 大臣: 手札の合計が12以上になったら負け（`have_s(7) && hand_s[0]+hand_s[1] >= 12`）。
- 8 姫: 捨てたら負け。
- 部分ゲームは「開始時に公開して取り除く3枚」で識別する。伏せ札1枚はそれとは別 (`do_action` の `put_hide_card`)。

## ビルドと実行

- C++20 / g++ / Makefile のみ。`make <target>`。
- 変更後は `make cfr cfr0 cfrorg brrnd brorg win` が通ることを確認する（実行までは不要）。
- `watch` は現状リンクエラー（`watch_cfr.cpp` が `cfr_switch` を定義していない）。`make all` はここで止まるので、上記6ターゲットを個別に指定する。
- 通常ビルドは `-DNDEBUG` なので `assert` は無効。assert を効かせたいときは `make cfrorgd`（出力名は `cfrorg` のまま）。
- 実行時は部分ゲームの3枚を引数で渡す: `./cfrorg 5 5 7`、`./win 4 4 6`。
- 自動テストは存在しない。`test.cpp` は gitignore 済みの手動デバッグ用スクラッチで、`main` を書き換えて使う。

## org と rnd の二重構造（最重要）

同じゲームに対して2つのモデルがあり、多くの関数・ヘッダが対で存在する。片方を直したらもう片方の整合性も確認すること。

- **org**: 元のルール。兵士の宣言がプレイヤーの意思決定。`org_action*.hpp` / `oph` / `make_infset.hpp` / `cfr_org.cpp` / `br.cpp`。
- **rnd**: 兵士の宣言を一様ランダムに抽象化した版。`rnd_action*.hpp` / `rph` / `rnd_make_infset.hpp` / `cfr.cpp` / `br_rnd.cpp`。

`bf_position` など共通コードは `bool rnd` 引数で切り替える（`bf_position(open, history, rnd)` の既定は `true` = rnd）。

## 行動履歴のエンコード（間違えやすい）

- 履歴は「1行動 = `unsigned char` 1文字」の文字列。gperf 完全ハッシュ (`rph` / `oph`) で 1〜2文字の文字列に戻してから解釈する（`log_util.hpp` の `char_to_action` / `char_to_twonum` / `char_to_wizard`）。
- `char_to_action` の上位桁: 1,2 = 初期手札、3 = ドロー、4 = カード使用。
- 兵士と魔術師の選択は**行動として追記されるのではなく、直前の1文字を payload 付きに書き換える**（`loveletter.cpp` の `do_action` case 6/7/8 が `erase()` してから `push`）。履歴末尾が payload なしの状態＝選択待ちノードで、`bf_position` では `is_sol_choice` / `is_wiz_choice` に対応する。履歴を伸ばす側のコードも同様に末尾を置換する必要がある（`infset_iswin.cpp` の `substr(0, length-1)`）。
- rnd の履歴には兵士宣言の payload が入らない（`do_action` case 8 は `org_his*` しか更新しない）。

## bf_position の規約

- `_s` = 視点プレイヤー自身、`_e` = 相手。`hand_s[2]` が自分の手札で、相手の手札は `trash` と推論フラグ (`open_flag_e`, `sol_flag_e`, `lt5_flag_e`, `not7_flag_e`) から `hand_e(i)` で候補集合として復元する。
- 推論フラグの意味: `lt5_*` = 大臣(7)を出したので残りの手札は5未満、`not7_*` = 魔術師(5)を出したので大臣(7)は持っていない、`sol_*` = 兵士で宣言されて外れたカード。
- 選択ノード (`is_sol_choice` / `is_wiz_choice`) では必ず `hand_s[1] == 0`。また相手が `barrier_e` のときは宣言・対象選択自体が発生しないので選択ノードにならない。
- 返り値の規約: `is_win` / `is_terminated_win` は `{使用カード(または真偽), 勝利までの手数}`。`is_terminated_win` の `{-1, 0}` は「終局判定に該当せず」。`is_lose` の `9` は「ルール上すでに敗北」。

## コード整形と静的解析

- 整形は clang-format 14（Ubuntu 22.04 の apt 版）＋直下の `.clang-format`。バージョンが違うと結果が変わるので 14 系を使う。
- 全体整形は一度きり済み。以後は**変更した行だけ**整形する: `git add -p && git clang-format`（`-i` 相当の上書きになる）。
- `org_action.hpp` / `rnd_action.hpp` は gperf の生成物なので整形対象外。
- `git blame` から整形コミットを除外するには一度だけ `git config blame.ignoreRevsFile .git-blame-ignore-revs` を実行する。
- 警告フラグは Makefile の `COMMON_WARN`（`-Wall -Wextra -Wshadow=local`）。`-Wshadow=local` はコンストラクタ引数がメンバを隠す書き方を許しつつ、ローカル同士のシャドーイングだけを警告する。新しい警告を出したまま放置しない。
- `make cppcheck` で静的解析（cppcheck 2.7）。抑制は理由を添えて `.cppcheck-suppressions` に、1箇所だけなら該当行の直前に `// cppcheck-suppress <id>` を書く。

## 実装の進め方

- 複数ファイルにまたがる変更、および org / rnd の対構造に触る変更は、直接実装せず `/plan-gate` を通す。
- `/plan-gate` は「計画書を書く → Haiku サブエージェント (`plan-quiz`) への一問一答で穴を検出 → 埋まるまで書き直す → Sonnet サブエージェント (`impl`) に実装させる」というゲート。詳細は `.claude/skills/plan-gate/SKILL.md`。
- 1ファイル内で完結する小さな修正はゲート不要。そのまま実装してよい。

## Git

- master に直接積まず、作業ブランチを切ってコミットし、確認後に `git merge --ff-only` で master に取り込む。
- コミットメッセージは日本語。
- `*.txt` `*.csv` `*.bin` `*.sh` と各バイナリ、`test.cpp` は gitignore 済み。実験結果のファイルをコミットしようとしないこと。

## 参考

- ファイル間の依存関係: `explain.md`（mermaid の図）
- 各ファイルの役割: `説明書.txt`（gitignore 済みでローカルのみ）

## Agent skills

### Issue tracker

GitHub Issues (`DrC-0/love`) を `gh` CLI 経由で使う。See `docs/agents/issue-tracker.md`.

### Triage labels

デフォルトの5ラベル（`needs-triage` / `needs-info` / `ready-for-agent` / `ready-for-human` / `wontfix`）をそのまま使う。See `docs/agents/triage-labels.md`.

### Domain docs

単一コンテキスト（ルートの `CONTEXT.md` + `docs/adr/`）。用語や決定が固まった時点で `domain-modeling` が遅延生成する。See `docs/agents/domain.md`.
