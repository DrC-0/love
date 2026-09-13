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

- **org**: 元のルール。兵士の宣言がプレイヤーの意思決定。`org_action*.hpp` / `oph` / `org_tree.hpp` + `visit_winlose.hpp` / `cfr_org.cpp` / `br.cpp`。
- **rnd**: 兵士の宣言を一様ランダムに抽象化した版。`rnd_action*.hpp` / `rph` / `rnd_make_infset.hpp` / `cfr.cpp` / `br_rnd.cpp`。

`belief_state` など共通コードは `bool rnd` 引数で切り替える（`belief_state(open, history, rnd)` の既定は `true` = rnd）。

org 側だけ、ゲーム木の展開と必勝・必敗判定が分離してある。`org_tree.hpp` が展開器
（`loveletter.hpp` しか include せず、判定も統計も持たない `template <class V>` の DFS）、
`visit_winlose.hpp` が判定器（`winlose_visitor`）。判定を差し替えるときは
`visit_winlose.hpp` の隣に同じフックを持つ struct を書く。展開部分は複製しない。
rnd 側は `rnd_make_infset.hpp` が情報集合表を作り、`infset_iswin.cpp` が第2フェーズで
判定する二相構成のまま。org を二相にしてはいけない理由は `docs/adr/0001` を参照。

## 行動履歴のエンコード（間違えやすい）

- 履歴は「1行動 = `unsigned char` 1文字」の文字列。gperf 完全ハッシュ (`rph` / `oph`) で 1〜2文字の文字列に戻してから解釈する（`log_util.hpp` の `char_to_action` / `char_to_twonum` / `char_to_wizard`）。
- `char_to_action` の上位桁: 1,2 = 初期手札、3 = ドロー、4 = カード使用。
- 兵士と魔術師の選択は**行動として追記されるのではなく、直前の1文字を payload 付きに書き換える**（`loveletter.cpp` の `do_action` case 6/7/8 が `erase()` してから `push`）。履歴末尾が payload なしの状態＝選択待ちノードで、`belief_state` では `is_sol_choice` / `is_wiz_choice` に対応する。履歴を伸ばす側のコードも同様に末尾を置換する必要がある（`infset_iswin.cpp` の `substr(0, length-1)`）。
- rnd の履歴には兵士宣言の payload が入らない（`do_action` case 8 は `org_his*` しか更新しない）。

## belief_state (Belief State) の規約

Belief State は依存が一方向の4ファイルに分かれている。umbrella は無いので、
使う側は必要な層を直接 include する。

| ファイル | 中身 | 依存 |
|---|---|---|
| `belief_state.hpp` | 型・アクセサ・フラグ更新・遷移・表示 | — |
| `belief_state_history.hpp` | 履歴コンストラクタと符号化・復号 | `belief_state.hpp` |
| `belief_state_win.hpp` | 必勝判定7本 + `able_actions` `action_count` | `belief_state.hpp` |
| `belief_state_lose.hpp` | 必敗判定3本 | `belief_state_win.hpp` |

必勝判定の7本 (`is_win` `is_terminated_win` `use_win` `enemy_turn_win` `draw_win`
`sol_win` `wiz_win`) は相互再帰の強連結成分なので分割できない。必敗側は
`draw_win` を呼ぶが、必勝側は必敗側を一度も呼ばない。

**判定はメモ化されており、`belief_state_win_checker` / `belief_state_lose_checker`
のメンバとして呼ぶ** (自由関数ではない)。

```cpp
belief_state_win_checker wc;
belief_state_lose_checker lc{wc};   // wc を先に宣言すること
auto r = wc.use_win(bs, card);
auto l = lc.use_lose(bs, card);
```

**checker は部分ゲーム1回の実行を通して使い回すこと。** 局所に作ると呼ばれる
たびにメモ表が捨てられ、効果が完全に消える。`cfrorg` は `winlose_visitor` が
1つ持ち、`win` は `infset_iswin` が作って `cnt_abs` に参照で渡している。

メモ化されているのは `use_win` `enemy_turn_win` `draw_win` `sol_win` `wiz_win`
`use_lose` `wiz_lose` の7本。`is_terminated_win` `is_win` `is_lose` はしていない。
判定に**グローバル変数を読むコードを足してはいけない** (純関数でなくなると
メモ化が壊れる)。詳細は `docs/adr/0007-memoize-judgement.md`。

2引数のコンストラクタ `belief_state(open, history)` を使うなら
`belief_state_history.hpp` を include すること。既定引数 `rnd = true` は
**宣言側ではなく定義側**に書かれているので、`belief_state.hpp` だけでは見えない。

- **カードには型がある。** `Card` は 1〜8 で「無い」を表せない。`MaybeCard` は
  「カード または 無し」で、`hand_s[2]` `open_flag_s/e` `sol_flag_s/e[2]` の格納と、
  `open_e()` `open_s()` `other_hand_s()` `hand_e_max/min()` `deck_or_hand_e_min()`
  の戻り値がこれ。**`MaybeCard` に順序比較は無い** (`open_e() > 1` のような、
  存在確認と値の比較を混ぜた書き方を封じるため)。`has_value()` で確かめてから
  `value()` で `Card` にする。`trash[]` は枚数なので `int` のまま。
  値域 (`Card` は 1〜8、`MaybeCard::value()` は「無し」で呼ばない) は
  **NDEBUG でも検査される**。`assert` ではなく常時有効の検査にしてあるのは、
  assert 有効ビルドが 16〜17 倍遅くて実際には回されないため
  (原因は `node::valid_data()` で、`Card` の検査は実測 +1% 未満)。
  **常に成り立つべき不変条件を `assert` で書かないこと。**
  詳細は `docs/adr/0008-card-types.md`。
- **カードでないものに `Card` を当てないこと。** `able_actions` の `card` 引数は
  `is_lose` が返す多目的のコードで、カード 1〜8 のほかに 0 / 9 / 15 / 25 を取り、
  `is_wiz_choice` の分岐では 0/1 の対象選択フラグになる。`int` のまま。
- **アクセサはカード (1〜8) を取る。** `hand_e(card)` `deck(card)` `deck_or_hand_e(card)`
  `hand_s_est(card)` `have_s(card)` すべて 1-origin で揃えてある。カードを回すループは
  `for(int card = 1; card <= 8; card++)`。`- 1` が要るのは `trash[]` と `max_num[]` の
  添字だけで、`sol_flag_*[2]` `hand_s[2]` は `[0]` `[1]` 固定なので足してはいけない。
  例外は `belief_state_win.hpp` の `able_actions` で、行動コードが 0-origin の添字を
  そのまま桁に埋め込むシリアライズ形式のため、この関数の中だけ添字で通している。
- `_s` = 視点プレイヤー自身、`_e` = 相手。`hand_s[2]` が自分の手札で、相手の手札は `trash` と推論フラグ (`open_flag_e`, `sol_flag_e`, `lt5_flag_e`, `not7_flag_e`) から `hand_e(i)` で候補集合として復元する。
- 推論フラグの意味: `lt5_*` = 大臣(7)を出したので残りの手札は5未満、`not7_*` = 魔術師(5)を出したので大臣(7)は持っていない、`sol_*` = 兵士で宣言されて外れたカード。
- 選択ノード (`is_sol_choice` / `is_wiz_choice`) では必ず `hand_s[1] == 0`。また相手が `barrier_e` のときは宣言・対象選択自体が発生しないので選択ノードにならない。
- 統計の規約: `win_points` と `lose_points` は**どちらも合計が `decision_points[0]` と
  一致する**。意思決定点ごとに必ず1つ数えるため、該当なしは添字0に落とす。兵士の
  宣言ノードには必敗判定が無いので `enter_soldier` は無条件に `lose_points[0]` を
  増やす。合計が合わなくなったら、どこかの意思決定点で数え漏らしている。
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

- **どんなに小さな変更でも作業ブランチを切る。** シェルスクリプトを1本 gitignore から
  外すような変更でも master に直接積まない。
- 確認後に master へ取り込む。取り込み方は2通りあり、**後始末が違う**。
  - **`git merge --ff-only`**: git がマージを認識するので `git branch -d` が素直に通る。
    枝の履歴を残したいときはこちら。
  - **スカッシュマージ**: git はマージとして記録しない。ブランチは永久に「未マージ」の
    ままになり、`git branch --merged` にも出ず、再度マージすると**取り消したはずの
    変更まで復活する**。そのため、**明言がない限りスカッシュ後はタグにして削除する**。
    ```
    git tag archive/<ブランチ名> <ブランチ名>
    git push origin archive/<ブランチ名>
    git branch -D <ブランチ名>
    git push origin --delete <ブランチ名>
    ```
    タグはマージの候補に出ないので、誤って取り込む経路が無くなる。中身は
    `git log archive/<ブランチ名>` で辿れる。
- コミットメッセージは日本語。
- gitignore 済み: `*.txt` `*.csv` `*.bin` `*.out`、`logs/`、各ターゲットのバイナリ
  (`cfrorg` `cfrorgcnt` `win` など)、`test.cpp`、`scp.sh`、`.env`、`credentials.json`。
  実験結果のファイルをコミットしようとしないこと。**`git add -A` はバイナリを拾うので、
  コミット前に `git status` で中身を確認する**。
- 環境ごとに違う値 (スプレッドシートの ID、scp の送信先) は `.env` に置く。雛形は
  `.env.example`。**このリポジトリは public なので、秘密や実ホスト名をコミットしない**。

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
