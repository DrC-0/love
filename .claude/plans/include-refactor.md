# インクルード改修: 手書き extern の解消とヘッダ構造の事故要因の除去

## 目的

各 `.cpp` が `extern int char_to_action(char c);` のような宣言を手書きでコピーしている
状態をやめ、共有宣言に置き場を作る。宣言と定義がずれても誰も気づかない構造が原因で、
`cfrorg` は `rph` を、`win` は `oph` を定義しておらず、`-O2` の定数畳み込みで参照が
消えるおかげで**たまたま**リンクが通っているだけの状態になっている。

背景と判断の根拠は `docs/adr/0004-shared-declarations-headers.md` を読むこと。

## この変更が触るモデル

**org と rnd の両方**。ただしゲームロジックには一切触らない。触るのは宣言の置き場と
ビルド設定だけで、`org_tree.hpp` / `visit_winlose.hpp` / `rnd_make_infset.hpp` /
`infset_dfs.hpp` / `infset_dfs_rnd.hpp` の中身（展開器と判定器のロジック）は変更しない。
`bf_position` の `_s` / `_e` 視点、推論フラグ (`lt5_*` `not7_*` `sol_*` `open_flag_*`)、
行動履歴のエンコードには**一切手を触れない**。履歴を伸ばす処理も追加しない。

したがって「org 側だけ直して rnd 側を忘れる」という対構造の事故は起きない。

### 左右対称な対（両方に同じ処理を施すこと。片方だけ直したら不合格）

- `org_action_sequense.cpp` / `rnd_action_sequense.cpp`
- `all_elements.hpp` / `all_elements_rnd.hpp`
- `infset_dfs.hpp` / `infset_dfs_rnd.hpp`

### 左右非対称な対（**揃えようとしてはいけない**）

`org_tree.hpp` と `rnd_make_infset.hpp` は org / rnd の対だが、扱いが違う。

| | `org_tree.hpp` | `rnd_make_infset.hpp` |
|---|---|---|
| インクルードガード | **すでに持っている** (`ORG_TREE_HPP`) | **無い。今回足す** |
| 統計カウンタの使用 | 使わない | `p1_points` / `p2_points` / `rand_points` / `end_points` を直接使う (35 箇所) |
| `table_infset` の使用 | 使わない | 使う (4 箇所) |
| 今回足す include | **無し。このファイルは触らない** | `analysis_points.hpp` と `loveletter.hpp` |

`org_tree.hpp` が何も使わないのは、展開器と判定器を分離した結果 (`docs/adr/0002`)
であって漏れではない。`org_tree.hpp` に include を足してはいけない。
逆に `rnd_make_infset.hpp` を「org 側と揃えて触らない」と判断するのも誤り。

---

## 変更 1: `action_code.hpp` / `action_code.cpp` を新設し `log_util.hpp` を削除

### 1-1. 現状

`loveletter.cpp:34-60` に変換6本の定義がある。同じ6本が `log_util.hpp:1-27` に
**バイト単位で同一の非 inline 定義**として重複しており、インクルードガードも無い。
そのため `log_util.hpp` を include すると `loveletter.cpp` と多重定義でリンクエラーに
なる。これを避けるために 8 つの `.cpp` が `extern` を手書きでコピーしている。
`log_util.hpp` を実際に include しているのは `test.cpp` だけで、`test.cpp` は Makefile 上
`loveletter.cpp` とリンクされない単独ビルド。

`rph` / `oph` は `bf_position.hpp:20-21` が `extern` 宣言する一方、9 ファイルが
`static` で各々のインスタンスを定義している。

### 1-2. `action_code.hpp` を新規作成

```cpp
#ifndef ACTION_CODE_HPP
#define ACTION_CODE_HPP

#include "rnd_action.hpp"
#include "org_action.hpp"

// 行動 <-> unsigned char 1 文字 の変換。定義は action_code.cpp に1つだけある。
char action_to_char(int action, int card);
int char_to_action(char c);
char wizard_to_char(int to, int trash, int draw);
int char_to_wizard(char c);
char twonum_to_char(int card1, int card2);
int char_to_twonum(char c);

// gperf の完全ハッシュ表。どちらも非静的データメンバを持たないステートレスな
// 関数オブジェクトなので、プログラム全体で1インスタンスにしてよい。
inline Rnd_Perfect_Hash rph;
inline Org_Perfect_Hash oph;

#endif
```

`rnd_action.hpp` / `org_action.hpp` は **すでにインクルードガードを持っている**
(`rnd_action.hpp:5` の `#ifndef RND_ACTION`、`org_action.hpp:5` の `#ifndef ORG_ACTION`。
先頭3行が gperf のコメントなのでガードは5行目にある)。したがってここから include して
二重インクルードになっても問題ない。

### 1-3. `action_code.cpp` を新規作成

`loveletter.cpp:34-60` の変換6本の定義をそのまま移す。中身は1文字も変えない。

```cpp
#include "action_code.hpp"

char action_to_char(int action, int card) {
  char c = (1 << 7) | (action << 3) | card;
  return c;
}
int char_to_action(char c) {
  int c2a1 = (c >> 3) & 7;
  int c2a2 = c & 7;
  return c2a1 * 10 + c2a2;
}
char wizard_to_char(int to, int trash, int draw) {
  char c = (1 << 7) | (to << 6) | (trash << 3) | draw;
  return c;
}
int char_to_wizard(char c) {
  int c2w1 = (c >> 6) & 1;
  int c2w2 = (c >> 3) & 7;
  int c2w3 = c & 7;
  return c2w1 * 100 + c2w2 * 10 + c2w3;
}
char twonum_to_char(int card1, int card2) {
  char c = (1 << 7) | (card1 << 3) | card2;
  return c;
}
int char_to_twonum(char c) {
  int c2t1 = (c >> 3) & 7;
  int c2t2 = c & 7;
  return c2t1 * 10 + c2t2;
}
```

### 1-4. `log_util.hpp` を削除

`git rm log_util.hpp`。include していたのは `test.cpp:7` (gitignore 済みのスクラッチ、
放置してよい) と、`bs_set.hpp:14` / `endgame.hpp:18` のコメントアウト行のみ。
コメントアウト行も削除する。

### 1-5. 各ファイルから手書き `extern` と `static rph/oph` を削除し `action_code.hpp` を include

削除する行（**行番号は変更前のもの。上から削ると番号がずれるので注意**）:

| ファイル | 削除する行 |
|---|---|
| `cfr.cpp` | 24-26 (extern 3本)、47 (`static Rnd_Perfect_Hash rph;`) |
| `cfr_zero.cpp` | 24-26 |
| `cfr_org.cpp` | 16-18、35 (`static Org_Perfect_Hash oph;`) |
| `br.cpp` | 26-28、55-56 |
| `br_rnd.cpp` | 26-28、53-54 |
| `infset_iswin.cpp` | 18 (`extern int char_to_action` 1本のみ)、30 (`static Rnd_Perfect_Hash rph;`) |
| `compare_abscfr.cpp` | 26-28、55-56 |
| `watch_cfr.cpp` | 20 (`static Rnd_Perfect_Hash rph;`) |
| `loveletter.cpp` | 31-32 (`static rph` / `static oph`)、34-60 (変換6本の定義。`action_code.cpp` へ移動済み) |
| `org_action_sequense.cpp` | 4 (`static Org_Perfect_Hash oph;`) |
| `rnd_action_sequense.cpp` | 4 (`static Rnd_Perfect_Hash rph;`) |

`test.cpp` は gitignore 済みのスクラッチなので**触らない**。

各ファイルの既存の `#include "org_action.hpp"` / `#include "rnd_action.hpp"` の直後に
`#include "action_code.hpp"` を足す。`org_action_sequense.cpp` と
`rnd_action_sequense.cpp` は `#include "org_action.hpp"` / `#include "rnd_action.hpp"` を
`#include "action_code.hpp"` に置き換えてよい。

`bf_position.hpp:20-28` の `extern Rnd_Perfect_Hash rph;` 〜 `extern char twonum_to_char(...)` の
8 行を削除し、代わりに `#include "action_code.hpp"` を書く（`bf_position.hpp:14-15` の
`#include "rnd_action.hpp"` / `#include "org_action.hpp"` は `action_code.hpp` が
include するので削除してよい）。

### 1-6. Makefile に `action_code.cpp` を追加

`COMMON_SRCS` に `action_code.cpp` を、`COMMON_HDRS` に `action_code.hpp` を足す。

```make
COMMON_SRCS = loveletter.cpp rnd_action_sequense.cpp org_action_sequense.cpp action_code.cpp
COMMON_HDRS = loveletter.hpp rnd_action_sequense.hpp org_action_sequense.hpp action_code.hpp
```

`test` と `end` ターゲットは `COMMON_SRCS` を使っていない単独ビルドなので、
`test` は触らない。`end` は `endgame.cpp` のみで変換関数を使わないので触らない。

---

## 変更 2: `run_mode.hpp` を新設（宣言のみ）

`loveletter.cpp:26-29` の `extern int cfr_switch;` 〜 `extern bool org_switch;` を削除し、
新しいヘッダに移す。

```cpp
#ifndef RUN_MODE_HPP
#define RUN_MODE_HPP

// 実行モードの切替。初期値はプログラムごとに違うため、定義は各 main の .cpp に残す。
// 詳細は docs/adr/0004-shared-declarations-headers.md を参照。
extern int cfr_switch;
extern int cfr_player;
extern bool br_switch;
extern int br_player;
extern bool org_switch;

#endif
```

**定義は絶対に移動しない。** 各 `.cpp` の以下の定義行はそのまま残す:

| ファイル | 残す定義 |
|---|---|
| `cfr.cpp:33-37` | `cfr_switch = 0` / `cfr_player = 0` / `br_switch = false` / `br_player = 0` / `org_switch = false` |
| `cfr_zero.cpp:33-37` | 同上 |
| `cfr_org.cpp:22-25` | `cfr_switch = 0` / `br_switch = false` / `br_player = 0` / **`org_switch = true`** |
| `br.cpp:38-42` | `cfr_switch = 0` / `cfr_player = 0` / `br_switch = false` / `br_player = 0` / `org_switch = false` |
| `br_rnd.cpp:38-42` | 同上 |
| `infset_iswin.cpp:22-24` | `br_switch = false` / `br_player = 0` / `org_switch = false` (`cfr_switch` は無い) |
| `watch_cfr.cpp:12-14` | 同上 |
| `compare_abscfr.cpp:38,40-42` | `cfr_switch = 0` / `br_switch` / `br_player` / `org_switch = false` |

`cfr_org.cpp` の `org_switch = true` と、`br.cpp:273` / `br_rnd.cpp:212` の実行時の
書き換えを壊さないこと。**初期値を1つでも取り違えたら `loveletter.cpp` の `do_action` の
分岐が変わり、出力が静かに変わる。**

`cfr_switch` を定義していないファイル (`infset_iswin.cpp` / `watch_cfr.cpp`) に
定義を足してはいけない。これらは `-DCFR` なしでビルドされ、`loveletter.cpp` の
`cfr_switch` 参照は `#ifdef CFR` の中にあるので参照が発生しない。

`loveletter.cpp` と、上表の各 `.cpp` に `#include "run_mode.hpp"` を足す。

---

## 変更 3: `analysis_points.hpp` を新設（宣言のみ）

`visit_winlose.hpp:14-21` の `extern unsigned long int p1_points;` 〜
`extern std::map<std::string, infset> table_infset;` の 8 行を削除し、新しいヘッダに移す。
ただし `table_infset` は変更 5 で `loveletter.hpp` に移すので、ここには含めない。

```cpp
#ifndef ANALYSIS_POINTS_HPP
#define ANALYSIS_POINTS_HPP

// 解析中に数える統計カウンタ。定義は各 main の .cpp に残す。
// 詳細は docs/adr/0004-shared-declarations-headers.md を参照。
extern unsigned long int p1_points;
extern unsigned long int p2_points;
extern unsigned long int rand_points;
extern unsigned long int end_points;
extern unsigned long int win_points[11];
extern unsigned long int lose_points[11];
extern unsigned long int decision_points[4];
extern unsigned long int opengame;
extern unsigned long int soldior_points;
extern unsigned long int soldior_infsets;

#endif
```

**注意**: `win_points` / `lose_points` / `decision_points` / `opengame` を定義しているのは
`cfr_org.cpp:31-34` だけ。`soldior_points` は `br.cpp:54` と `br_rnd.cpp:52`、
`soldior_infsets` は `br.cpp:53` だけ。**宣言はあるが定義が無いプログラムが存在する**が、
参照が発生しないのでリンクは通る。定義を足してはいけない。

`visit_winlose.hpp` に `#include "analysis_points.hpp"` を足す。
`cfr.cpp` / `cfr_zero.cpp` / `br.cpp` / `br_rnd.cpp` / `infset_iswin.cpp` /
`compare_abscfr.cpp` / `watch_cfr.cpp` にも足す。
`rnd_make_infset.hpp` / `newcfr.hpp` / `infset_dfs.hpp` にも必要（変更 6 の表を参照）。

---

## 変更 4: `card_table.hpp` を新設

**計画時の名称からの変更**: ユーザーと合意した名前は `card_label.hpp` だったが、
`max_num` (カードの枚数) もここに入れるため、表示名だけを示す `label` より
`card_table.hpp` の方が内容に合う。実装時はこの名前を使うこと。

```cpp
#ifndef CARD_TABLE_HPP
#define CARD_TABLE_HPP

// カードごとの枚数。配列の添字は card - 1。
inline constexpr int max_num[8] = {5, 2, 2, 2, 2, 1, 1, 1};

inline constexpr double table_sign[2] = {1.0, -1.0};
inline constexpr char action_sign[8] = {'0', 'a', 'c', 'd', 'e', 'f', 'g', 'h'};
inline constexpr char card_sign[8][20] = {"兵士", "道化", "騎士", "僧侶", "魔術師", "将軍", "大臣", "姫"};

#endif
```

削除する重複:

| ファイル | 削除する行 |
|---|---|
| `bf_position.hpp:1-4` | `#ifndef MAX_NUM` 〜 `#endif` の4行 |
| `bs_set.hpp:1-4` | 同上 |
| `endgame.hpp:1-4` | 同上 |
| `cfr.cpp:30-32` | `table_sign` / `action_sign` / `card_sign` |
| `cfr_zero.cpp:30-32` | 同上 |
| `br.cpp:35-37` | 同上 |
| `br_rnd.cpp:35-37` | 同上 |

`bf_position.hpp` / `bs_set.hpp` / `endgame.hpp` / `cfr.cpp` / `cfr_zero.cpp` /
`br.cpp` / `br_rnd.cpp` に `#include "card_table.hpp"` を足す。

`bf_position.hpp` は削除後、1行目が `#ifndef BF_POSITION_HPP` になる。
`bs_set.hpp` は `#ifndef BS_HASH`、`endgame.hpp` は `#ifndef EXP_REWARD_HPP` になる。

---

## 変更 5: `loveletter.hpp` に `table_infset` の宣言を足す

`loveletter.hpp` の `class infset` 定義の直後（`struct work_do_action` の直前）に:

```cpp
extern std::map<std::string, infset> table_infset;
```

`loveletter.hpp` は `<map>` と `<string>` を include していないので、ファイル冒頭の
`#ifndef LOVELETTER` / `#define LOVELETTER` の直後に `#include <map>` と
`#include <string>` を足す。

`loveletter.cpp:24` と `visit_winlose.hpp:21` の `extern std::map<std::string, infset> table_infset;`
を削除する。`loveletter.cpp:25` のコメントアウト行 (`// extern std::map<size_t, infset> ...`)
も削除してよい。

各 `.cpp` の `map<std::string, infset> table_infset{};` という**定義は残す**
(`cfr.cpp:38` / `cfr_zero.cpp:38` / `cfr_org.cpp:26` / `br.cpp:43` / `br_rnd.cpp:43` /
`infset_iswin.cpp:25` / `compare_abscfr.cpp` / `watch_cfr.cpp`)。

---

## 変更 6: ガードの無いヘッダ 8 本にインクルードガードを足す

対象と使うマクロ名:

| ファイル | マクロ名 |
|---|---|
| `all_elements.hpp` | `ALL_ELEMENTS_HPP` |
| `all_elements_rnd.hpp` | `ALL_ELEMENTS_RND_HPP` |
| `cfr.hpp` | `CFR_HPP` |
| `cfr_exp_reward.hpp` | `CFR_EXP_REWARD_HPP` |
| `infset_dfs.hpp` | `INFSET_DFS_HPP` |
| `infset_dfs_rnd.hpp` | `INFSET_DFS_RND_HPP` |
| `newcfr.hpp` | `NEWCFR_HPP` |
| `rnd_make_infset.hpp` | `RND_MAKE_INFSET_HPP` |

**`all_elements.hpp` / `all_elements_rnd.hpp` / `infset_dfs.hpp` / `infset_dfs_rnd.hpp` は
ファイル末尾に改行が無い**。`#endif` を足す前に改行を入れること。

`org_action.hpp` / `rnd_action.hpp` は gperf の生成物で、**すでにガードを持っている**
ので触らない。

### ガードと同時に、各ヘッダに必要な include を足す

これらのヘッダは現在 include を1つも持たず（`all_elements.hpp` /
`all_elements_rnd.hpp` を除く）、「各 `.cpp` の決まった順序の前置きの後に include
される」ことに依存している。その暗黙の順序依存こそが今回潰す対象なので、
ガードを足すのと同時に、実際に使っているものを自分で include させる。

各ヘッダが実際に使っている名前を数えた結果:

| ヘッダ | 統計カウンタ | `table_infset` | モード切替 | 変換6本 / `rph`・`oph` | `table_sign` |
|---|---|---|---|---|---|
| `all_elements.hpp` | 0 | 4 | 4 | 16 | 0 |
| `all_elements_rnd.hpp` | 0 | 4 | 2 | 15 | 0 |
| `cfr.hpp` | 0 | 2 | 0 | 0 | 29 |
| `cfr_exp_reward.hpp` | 0 | 2 | 0 | 0 | 29 |
| `infset_dfs.hpp` | 2 | 8 | 8 | 7 | 44 |
| `infset_dfs_rnd.hpp` | 0 | 8 | 8 | 5 | 43 |
| `newcfr.hpp` | 36 | 4 | 0 | 0 | 29 |
| `rnd_make_infset.hpp` | 35 | 4 | 0 | 0 | 0 |

したがって足す include は:

| ヘッダ | 足す include |
|---|---|
| `all_elements.hpp` | `loveletter.hpp`, `run_mode.hpp`, `action_code.hpp` |
| `all_elements_rnd.hpp` | `loveletter.hpp`, `run_mode.hpp`, `action_code.hpp` |
| `cfr.hpp` | `loveletter.hpp`, `card_table.hpp` |
| `cfr_exp_reward.hpp` | `loveletter.hpp`, `card_table.hpp` |
| `infset_dfs.hpp` | `loveletter.hpp`, `analysis_points.hpp`, `run_mode.hpp`, `action_code.hpp`, `card_table.hpp` |
| `infset_dfs_rnd.hpp` | `loveletter.hpp`, `analysis_points.hpp`, `run_mode.hpp`, `action_code.hpp`, `card_table.hpp` |
| `newcfr.hpp` | `loveletter.hpp`, `analysis_points.hpp`, `card_table.hpp` |
| `rnd_make_infset.hpp` | `loveletter.hpp`, `analysis_points.hpp` |

`loveletter.hpp` は `table_infset` の宣言（変更 5）と `node` / `infset` の定義のため。
`table_sign` は `card_table.hpp` に入る（変更 4）。

`newcfr.hpp` はどこからも include されていないので、ここで include を足しても
ビルドには影響しない。それでも足すのは、将来 include されたときに壊れないようにする
ため。`newcfr.hpp:6` の `extern game_tree_mode g;` は `newcfr.hpp` 自身が
`game_tree_mode` を定義しているので、そのままでよい。

---

## 変更 7: `bf_position.hpp` の `using namespace std;` と `commentablebfp`

`bf_position.hpp:18` の `using namespace std;` を削除する。修飾が必要になるのは:

- `cout` 27 箇所、`cerr` 3 箇所、`endl` 17 箇所 → `std::` を付ける
- 190 行目 `bf_position::bf_position(int open[3], string history, bool rnd = true)` の
  `string` → `std::string`
- 200 行目 / 1129 行目 / 1131 行目 の `string` → `std::string`

`std::max` / `std::min` はすでに修飾済み。

`bf_position.hpp:30` の `bool commentablebfp = false;` を `inline bool commentablebfp = false;`
にする。読んでいるのは同ファイル内の 610 / 733 / 877 行の `if(commentablebfp)` だけで、
`true` を代入している箇所はリポジトリに存在しない。**削除はしない**（デバッグ用に
残す）。

---

## 変更 8: `visit_winlose.hpp` の冗長ガードを削除

`visit_winlose.hpp:4-6` の

```cpp
#ifndef BF_POSITION_HPP
#include "bf_position.hpp"
#endif
```

を

```cpp
#include "bf_position.hpp"
```

にする。`bf_position.hpp` 自身が `BF_POSITION_HPP` ガードを持っているので二重に囲む
必要が無い。

---

## 変更 9: Makefile から `bf_position.hpp` のソース指定を削除

`cfrorg` / `cfrorgd` / `watch` / `win` / `comp` の 5 ターゲットで、g++ の引数から
`bf_position.hpp` を削除する。例:

```make
cfrorg: cfr_org.cpp org_tree.hpp visit_winlose.hpp bf_position.hpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 $(COMMON_WARN) -O2 $(COMMON_DEFS) -DCFR $(COMMON_SRCS) cfr_org.cpp -o $@
```

**依存関係の行 (`:` の右側) からは消さない**。消すのはコマンド行の g++ 引数だけ。

---

## 変更 10: `cfrorg.sh` に計測を足す

本番実行の所要時間が記録されていないので、`/usr/bin/time` で残す。

`cfrorg.sh` の `run_cfrorg` を次のように変える:

```bash
run_cfrorg() {
    xargs -a all_subgames.txt -n 3 -P 12 sh -c '
        /usr/bin/time -f "%e %U %M" -o "log/org$1$2$3.time" ./cfrorg "$@" > "log/org$1$2$3.log" 2>&1
    ' sh
}
```

`log/org<部分ゲーム>.time` に「経過秒 CPU秒 最大RSS(KB)」の1行が残る。

**`log/org*.status` は書かない。** `check_cfrorg_status.sh` がこのファイルを読むが、
今回は**そこには手を触れない**。`.status` を書き出す変更はユーザーが承認していない。
`check_cfrorg_status.sh` も `cfrorg.sh` の他の部分も変更しないこと。

---

## やらないこと（明示）

- **`brrnd` は触らない。** `br_rnd.cpp:154` が読む `ave_prob_action<部分ゲーム><t>.bin` を
  書き出すコードはリポジトリに存在せず、`./brrnd 1 2 5 5 7` は `file read error` で
  即死する。既に壊れているものとして扱う。ただし**ビルドは通さなければならない**。
- `newcfr.hpp` / `first_hand_open.hpp` はどこからも include されていないが、**削除しない**。
  ガードを足すだけ (`first_hand_open.hpp` はすでにガードを持つので触らない)。
- モード切替と統計の**定義**の共通化はしない。宣言だけを共有する。
- `bf_position` のフィールド、推論フラグ、行動履歴のエンコード、`do_action` /
  `undo_action`、展開器・判定器のロジックには一切触らない。
- `test.cpp` は gitignore 済みのスクラッチなので触らない。
- 性能を変える変更はしない（それは別ブランチ）。
- `check_cfrorg_status.sh` は変更しない。`log/org*.status` を書き出す変更は
  ユーザーが明示的に却下したので、実装してはいけない。

---

## 検証

### 手順 1: ビルド

```
make cfr cfr0 cfrorg brrnd brorg win
```

が警告なしで通ること。`COMMON_WARN` は `-Wall -Wextra -Wshadow=local`。
**新しい警告を1つでも出したら不合格。**

### 手順 2: `-O0` でもリンクが通ること

改修前は `cfrorg` が `undefined reference to 'rph'`、`win` が
`undefined reference to 'oph'` で落ちる。改修後は両方通らなければならない。

```
g++ -std=c++20 -O0 -DNDEBUG -DUSE_SAME_MOVE -DUSE_GOOD_MOVE -DNUSE_BAD_MOVE -DCFR \
  loveletter.cpp rnd_action_sequense.cpp org_action_sequense.cpp action_code.cpp cfr_org.cpp -o /tmp/o0_cfrorg
g++ -std=c++20 -O0 -DNDEBUG -DUSE_SAME_MOVE -DUSE_GOOD_MOVE -DNUSE_BAD_MOVE \
  loveletter.cpp rnd_action_sequense.cpp org_action_sequense.cpp action_code.cpp infset_iswin.cpp -o /tmp/o0_win
```

**これがこの改修の主目的なので、通らなければ不合格。**

### 手順 3: 出力の完全一致

`cfrorg` は改修前の md5 が分かっている。**この値と一致しなければ不合格。**

```
./cfrorg 5 5 7 7 | md5sum   -> 174ad7bb550748eafcc4ec0c921057e7
./cfrorg 4 4 6 7 | md5sum   -> e997f3ec7a80ec99bcc5f9afffbcb82f
```

`win` / `cfr0` / `cfr` は作業用ディレクトリで改修前後の両方を走らせて突き合わせる。
`abs/` 配下に書くので、リポジトリ直下では走らせないこと。

```
win 5 5 7        57秒   標準出力の md5 -> bebea24ab404452abadeadaee30f4fca
win 4 4 6        89秒   標準出力の md5 -> f014f65e2ed63c16eebba9d2a80a45b7
cfr0 5 5 7       49秒   標準出力 + str5570.bin + utilities557.csv
cfr 0 2 5 5 7   172秒   標準出力 + str5572.bin
```

### 手順 4: 静的解析

```
make cppcheck
```

新しい指摘を増やさないこと。

### 手順 5: 整形

変更した行だけを整形する。`git add -p && git clang-format`。clang-format は 14 系。
`org_action.hpp` / `rnd_action.hpp` は整形対象外（今回は触らないので該当しない）。

### ブランチ最後に1回だけ

```
brorg 1 2 5 5 7   3580秒 (約60分)
```

改修前後で標準出力と `utilities557.csv` を突き合わせる。

---

## コミットの分け方

1. `action_code.hpp` / `.cpp` の新設、`log_util.hpp` 削除、手書き extern と
   `static rph/oph` の除去 (変更 1)
2. `run_mode.hpp` の新設 (変更 2)
3. `analysis_points.hpp` の新設 (変更 3)
4. `card_table.hpp` の新設と `max_num` の一本化 (変更 4)
5. `table_infset` の宣言を `loveletter.hpp` へ (変更 5)
6. ガードの追加 (変更 6)
7. `using namespace std;` の除去と `commentablebfp` の `inline` 化 (変更 7)
8. `visit_winlose.hpp` の冗長ガード削除と Makefile の整理 (変更 8, 9)
9. `cfrorg.sh` の計測追加 (変更 10)。`.status` は書かない

各コミットで `make cfr cfr0 cfrorg brrnd brorg win` が通ることを確認する。
コミットメッセージは日本語。
