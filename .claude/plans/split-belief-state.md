# Belief State (bf_position.hpp) を層で分割する

## 目的

`bf_position.hpp` は 1489 行ある。中身を調べると依存が一方向の層になっており、
機械的に分割できる。**振る舞いは一切変えない。** 回帰ハーネスの diff が空である
ことが唯一の成功条件。

用語は `CONTEXT.md` を参照 (Belief State / 物理状態 / 展開器 / 判定器)。

## org と rnd のどちらを触るか

**どちらも触らない。** `bf_position` は org と rnd の両方から使われる共有の型で、
org/rnd の切り替えは `bf_position(open, history, rnd)` の第3引数と
`action2char(x, rnd)` / `get_actions_history(s, rnd)` の `rnd` 引数で行われる。
この分割はコードの置き場所を変えるだけで、`rnd` 引数の意味にも、org 側の展開器
(`org_tree.hpp`) にも、rnd 側の展開器 (`rnd_make_infset.hpp`) にも触らない。
対で存在するヘッダ (`org_action*.hpp` / `rnd_action*.hpp` など) も触らない。

## 行動履歴に手を入れるか

**入れない。** 履歴文字列を解釈するコード (コンストラクタ L199-339、
`action2char`、`get_actions_history` など) は**1文字も変えずにファイルを移すだけ**。
末尾の追記も、payload 付きへの置換も行わない。

## 分割後のファイル構成

依存は上から下への一方向のみ。逆流が無いことを確認済み。

| 新ファイル | 中身 | include するもの |
|---|---|---|
| `belief_state.hpp` ① | 型・アクセサ・フラグ更新・遷移・表示 | 標準ヘッダ + `action_code.hpp` `card_table.hpp` `count_work.hpp` |
| `belief_state_history.hpp` ② | 履歴コンストラクタと符号化・復号 | ① |
| `belief_state_win.hpp` ③ | 必勝判定 + `able_actions` `action_count` | ① |
| `belief_state_lose.hpp` ④ | 必敗判定 | ③ |
| `first_hand_open.hpp` ⑤ | 開手ラブレターの述語 3 本 | ② |
| `bf_position.hpp` | ①②③④ を include するだけの**互換 umbrella** | ①②③④ |

`bf_position.hpp` を umbrella として残すのは、**利用者側の `#include` を1行も
変えないため**。これで回帰ハーネスに差が出たら原因は分割のミスに限定できる。
umbrella は次の作業 (改名) で撤去する。⑤ は umbrella に含めない
(外部から誰も呼んでいないため)。

## 変更 1: `belief_state.hpp` を新規作成

`bf_position.hpp` から以下を**そのまま**移す。行番号は現在の `bf_position.hpp`。
順序は下表のとおりにする (前方参照が生じないため)。

| 現在の行 | 内容 |
|---|---|
| L15 | `inline bool commentablebfp = false;` |
| L17-75 | `struct bf_position` の宣言と2つのコンストラクタ (既定・全項目) |
| L77-79 | 自由関数 `trash_and_hand_s` `deck_or_hand_e` `hand_e` の**宣言** |
| L86 | `reset_flag_by_use` の宣言 |
| L97-113 | `struct ef_wizard_preds` |
| L114-117 | `ef_wizard` `draw` `swap_player` の宣言 |
| L125-127 | `exit_with_print` `validate_hand_e_candidate` の宣言 |
| L132-161 | 自由関数 `trash_and_hand_s` `deck_or_hand_e` `hand_e` の**定義** |
| L340-492 | メンバ関数 16 本 (`deck_or_hand_e` 〜 `reset_flag`) |
| L493-530 | `reset_flag_by_use` |
| L948-954 | `draw` |
| L991-1080 | `ef_wizard` |
| L1081-1104 | `bf_position::print` |
| L1105-1110 | `exit_with_print` |
| L1111-1117 | `validate_hand_e_candidate` |
| L1230-1244 | `swap_player` |

先頭は:

```cpp
#ifndef BELIEF_STATE_HPP
#define BELIEF_STATE_HPP
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <utility>
#include <cstdlib>
#include <cassert>

#include "action_code.hpp"
#include "card_table.hpp"
#include "count_work.hpp"
```

`<cassert>` を明示的に足す (`ef_wizard_preds::push` が `assert` を使うが、現在は
他のヘッダ経由で間接的に入っている)。それ以外の include は現在と同じ。

`bf_position` の履歴コンストラクタの**宣言** (L56
`bf_position(int open[3], std::string history, bool rnd);`) は①に残す。
定義 (L199-339) は②に置く。**①の宣言に既定引数を足してはいけない** (下の
「既定引数の扱い」を参照)。

## 変更 2: `belief_state_history.hpp` を新規作成

| 現在の行 | 内容 |
|---|---|
| L199-339 | `bf_position::bf_position(int open[3], std::string history, bool rnd = true)` |
| L118-120 | `action2char` `actions_to_string` `get_actions_history` `output_actions_history` の宣言 |
| L1118-1157 | `action2char` |
| L1158-1167 | `actions_to_string` |
| L1168-1225 | `get_actions_history` |
| L1226-1229 | `output_actions_history` |

```cpp
#ifndef BELIEF_STATE_HISTORY_HPP
#define BELIEF_STATE_HISTORY_HPP
#include "belief_state.hpp"
```

### 既定引数の扱い (最も間違えやすい)

このコードベースは**宣言側に既定引数を書かず、定義側に書いている**。
3 箇所すべてこの形になっている。

| 宣言 (既定引数なし) | 定義 (既定引数あり) |
|---|---|
| L56 `bf_position(int open[3], std::string history, bool rnd);` | L199 `bf_position::bf_position(int open[3], std::string history, bool rnd = true)` |
| L119 `std::string get_actions_history(std::string s, bool rnd);` | L1168 `std::string get_actions_history(std::string s, bool rnd = true) {` |
| L120 `void output_actions_history(std::string s, bool rnd);` | L1226 `void output_actions_history(std::string s, bool rnd = true) {` |

**この非対称を現状のまま、1 文字も変えずに保つこと。**
「宣言と定義で揃っていないから片方に寄せよう」と判断してはいけない。
`= true` を①の宣言側へ移す、あるいは①にも足して両方に書く、のどちらも駄目。
両方に書くと C++ の規則 (同じ既定引数を 2 回宣言できない) に触れてコンパイルが
通らなくなり、宣言側だけに移すと意図せず①単体で既定引数が見えるようになる。

**実際にこの既定引数へ依存している呼び出しが 3 箇所ある** (2 引数で呼んでいる)。

| 呼び出し元 | ターゲット |
|---|---|
| `infset_iswin.cpp:46` `bf_position bfp(open, history);` | `win` |
| `infset_iswin.cpp:107` `bf_position bfp(open, it->first);` | `win` |
| `watch_cfr.cpp:43` `bf_position bfp(open, it->first);` | `watch` |

これらが動くのは、既定引数が**②の定義を通過した後**にしか見えないため、
umbrella が①の次に②を include しているから。**umbrella の include 順を
①→②→③→④ から変えてはいけない。** ②を①より先に置くと②が①を include して
いるので結果は同じだが、③や④を②より前に置くと `win` のビルドが壊れる。

なお `cfrorg` はこの既定引数に依存しない。`visit_winlose.hpp:74` と `:145` が
`bf_position bfp(n.open, key, false);` と第 3 引数を明示しているため。

## 変更 3: `belief_state_win.hpp` を新規作成

| 現在の行 | 内容 |
|---|---|
| L87-93 | `is_win` `is_terminated_win` `use_win` `draw_win` `enemy_turn_win` `sol_win` `wiz_win` の宣言 |
| L124, L127 | `able_actions` `action_count` の宣言 |
| L531-557 | `is_terminated_win` |
| L558-624 | `is_win` |
| L625-746 | `use_win` |
| L747-890 | `enemy_turn_win` |
| L891-947 | `draw_win` |
| L955-966 | `sol_win` |
| L967-990 | `wiz_win` |
| L1422-1460 | `able_actions` |
| L1461-1474 | `action_count` |

```cpp
#ifndef BELIEF_STATE_WIN_HPP
#define BELIEF_STATE_WIN_HPP
#include "belief_state.hpp"
```

この 7 本は相互再帰の強連結成分なので、**必ず全部の宣言を定義より前に置く**
(`use_win` ↔ `enemy_turn_win` ↔ `draw_win` ↔ `use_win`、`sol_win` → `enemy_turn_win`、
`wiz_win` → `enemy_turn_win`)。分割してはいけない。

## 変更 4: `belief_state_lose.hpp` を新規作成

| 現在の行 | 内容 |
|---|---|
| L121-123 | `is_lose` `use_lose` `wiz_lose` の宣言 |
| L1245-1278 | `is_lose` |
| L1279-1399 | `use_lose` |
| L1400-1421 | `wiz_lose` |

```cpp
#ifndef BELIEF_STATE_LOSE_HPP
#define BELIEF_STATE_LOSE_HPP
#include "belief_state_win.hpp"
```

③ を include するのは、`use_lose` と `wiz_lose` が `draw_win` を呼ぶため。
**必勝側は必敗側を一度も呼ばない** (依存は ④→③ の一方向)。

## 変更 5: `first_hand_open.hpp` を書き換える

現在の `first_hand_open.hpp` の中身 (16 行) を**全部消す**。消す理由は、
`node::first_hand_open` に戻り値型が無く、`this.` を `this->` の代わりに使って
おり、`node` にその宣言も無いため**コンパイルが通らない草案**だから。どこからも
include されていないので誰も気づいていなかった。

代わりに `bf_position.hpp` L128-130 の宣言と L1475-1488 の定義
(`is_openhand_loveletter` `is_openhand_loveletter_hisp` `is_openhand_loveletter_his`)
を移す。

```cpp
#ifndef FIRST_HAND_OPEN_HPP
#define FIRST_HAND_OPEN_HPP
#include "belief_state_history.hpp"
```

`_hisp` と `_his` が `bf_position(open, history, rnd)` を使うので②が要る。
現在のガードは `FIRRST_HAND_OPEN_HPP` と R が 1 つ多い綴り間違いなので、
`FIRST_HAND_OPEN_HPP` に直す。

**この3本は現在どこからも呼ばれていない。** Makefile のどのターゲットも
`first_hand_open.hpp` を include しないので、ビルド対象には入らない。

## 変更 6: `bf_position.hpp` を umbrella にする

中身を全部捨てて、これだけにする。

```cpp
#ifndef BF_POSITION_HPP
#define BF_POSITION_HPP
// 互換のための umbrella。中身は belief_state*.hpp に分割済み。
// 次の作業 (bf_position -> belief_state の改名) で撤去する。
#include "belief_state.hpp"
#include "belief_state_history.hpp"
#include "belief_state_win.hpp"
#include "belief_state_lose.hpp"
#endif
```

## 変更 7: 死んでいる自由関数 5 本を削除

以下は定義も宣言も**削除する**。対応するメンバ関数が全部自前で実装しており、
`bf_position.hpp` の中からも外からも呼ばれていないことを確認済み。

| 宣言 | 定義 | 関数 |
|---|---|---|
| L80 | L162-174 | `open_e(const int hand[2], ...)` |
| L81 | L176-179 | `deck(const int i, const int hand[2], ...)` |
| L82 | L181-183 | `in(const int hand[2], const int target)` |
| L83 | L185-189 | `other(const int hand[2], const int target)` |
| L84 | L191-197 | `count_deck(const int hand[2], const int trash[8])` |

**残す 3 本と混同しないこと。** `trash_and_hand_s` / `deck_or_hand_e` / `hand_e` の
生配列版は**消してはいけない**。理由:

- `bf_position::deck_or_hand_e` (L341) が `::deck_or_hand_e` を呼ぶ
- `bf_position::hand_e` (L345) が `::hand_e` を呼ぶ
- `bf_position::hand_s_est` (L350) が `::hand_e` を呼ぶ。しかも `hand_s` ではなく
  その場で組んだ `int enemy_hand[2] = {open_e(), 0}` を渡し、`_e` ではなく `_s` 側の
  フラグ (`open_flag_s` `sol_flag_s` `lt5_flag_s` `not7_flag_s`) を使う。
  **役割を入れ替えて「相手から見て自分の手札に card i がありうるか」を計算して
  いる**ので、生配列を取る汎用形でないと書けない。
- `::deck_or_hand_e` が内部で `::trash_and_hand_s` を呼ぶ

なお `bs_set.hpp` L387 付近にも `trash_and_hand_s` と `in` の**別の実装**があるが、
`endgame.hpp` の `#include "bf_position.hpp"` は L14 でコメントアウトされており、
`end` ターゲットは `bs_set.hpp` 側の実装を使う。**`bs_set.hpp` と `endgame.hpp` には
一切触らない。**

## 利用者側の変更

**無い。** umbrella を残すので `#include "bf_position.hpp"` はそのまま動く。
以下の 5 ファイルはいずれも変更しない。

| ファイル | ターゲット |
|---|---|
| `visit_winlose.hpp` | `cfrorg` |
| `infset_iswin.cpp` | `win` |
| `compare_abscfr.cpp` | `comp` |
| `watch_cfr.cpp` | `watch` (元からリンクできない既知の壊れ) |
| `test.cpp` | `test` (gitignore 済みスクラッチ。触らない) |

## Makefile の変更

`cfrorg` `win` `comp` `test` の依存に `bf_position.hpp` が書かれている。
新しい 4 ファイルを依存に足す。

```make
cfrorg: cfr_org.cpp org_tree.hpp visit_winlose.hpp bf_position.hpp \
        belief_state.hpp belief_state_history.hpp belief_state_win.hpp belief_state_lose.hpp \
        $(COMMON_SRCS) $(COMMON_HDRS)
```

同様に `cfrorgd` `cfrorgcnt` `win` `comp` `test` にも足す。
**コンパイルコマンド本体 (`g++ ...` の行) は変えない。**

## 検証

### 手順 1: ビルド

```
make cfr cfr0 cfrorg brrnd brorg win
make comp cfrorgcnt
```

警告が 1 つも出ないこと。`-Wall -Wextra -Wshadow=local` が有効。

### 手順 2: 回帰ハーネス

```
./regress.sh check full
```

**12 項目すべて `OK` であること。** 1 つでも `DIFFER` が出たら分割のミス。

内訳: `cfrorg` の 557/446 × 打ち切り深さ 7/6 について標準出力と決定的カウンタ
(8 項目)、`win` の 557/446 について標準出力と `abs/abs<部分ゲーム>.bin` の
sha256 (4 項目)。所要 3 分 30 秒。

### 手順 3: 行数の確認

分割後の 5 ファイルの合計行数が、元の `bf_position.hpp` (1489 行) から
削除した 5 関数ぶん (宣言 5 行 + 定義 36 行 = 41 行) を引いた値と、
ガードと include のぶんを除いておおむね一致すること。
**大きくずれていたら、移し忘れか二重に移している。**

## やらないこと

- `bf_position` の**改名**はしない。次の作業で別コミットとしてやる。
- 引数の規約 (0-origin の `i` と 1-origin の `card` の混在) は**直さない**。
  その次の作業でやる。
- 関数の中身、フラグの意味、判定のロジックには**一切触らない**。
  移すだけ。1 文字も変えない。
- `node` / `loveletter.cpp` / `org_tree.hpp` / `rnd_make_infset.hpp` /
  `visit_winlose.hpp` には触らない。
- `bs_set.hpp` / `endgame.hpp` / `end` ターゲットには触らない。
- `test.cpp` には触らない (gitignore 済みスクラッチ)。
- 性能を変える変更はしない。
