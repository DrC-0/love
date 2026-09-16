# win の出力形式を wininf / loseinf に変える

## 目的

`win` (`infset_iswin.cpp`) が書き出す `abs/abs<部分ゲーム>.bin` を、
必勝側 `abs/wininf<部分ゲーム>.bin` と必敗側 `abs/loseinf<部分ゲーム>.bin` の
2 本に分け、**1 行 = 1 つの (情報集合, 行動) 対**の形式にする。

今の形式は「この情報集合は必勝」としか記録しておらず、**どちらの手札スロットが
必勝なのかを持っていない**。その情報は `infset_iswin.cpp` の中には存在するのに
捨てられている。読み手 (次の変更で作り直す `comp`) は CFR の平均戦略 σ̄ =
「手札スロット 0 を出す確率」と突き合わせたいので、スロットが要る。

**この変更は出力ファイルの形式だけを変える。判定そのものと統計は一切変えない。**

## この変更は rnd 側だけを触る

`infset_iswin.cpp` は `rnd_make_infset.hpp` で rnd の情報集合表を作り、
`belief_state` を既定の `rnd = true` で構築する。org 側 (`org_tree.hpp` /
`visit_winlose.hpp` / `cfr_org.cpp`) はこのファイル群を一切参照しないので触らない。
`save_load_abshistory.hpp` を include しているのも `infset_iswin.cpp` と
`compare_abscfr.cpp` の 2 つだけで、どちらも rnd 側。

確認コマンド:

```
grep -rn "save_load_abshistory\|load_bin_abs\|save_bin_abs" --include=*.cpp --include=*.hpp .
```

## 絶対に動かしてはいけないもの (最重要)

この変更が正しいことの証明は「**`win` の標準出力が 1 文字も変わらないこと**」である。
形式だけを変えたなら統計は動かないはずで、動いたら判定に触ってしまっている。

したがって以下は **1 行も変えない**:

- `infset_iswin.cpp:34-40` の `win_cnt` / `lose_cnt` / `win_move` / `lose_move` /
  `action_cnt` / `max_history` / `hist_max`
- `cnt_abs` の中の `win_move[...]++` / `lose_move[...]++` / `hist_max` 更新
- `infset_iswin.cpp:110-114` の走査ループと `action_cnt += action_count(bs)`
- `infset_iswin.cpp:116-117` の 2 本の `assert`
- `infset_iswin.cpp:119-124` の `win move:` / `lose move:` の出力
- `infset_iswin.cpp:129-154` の `infset_cnt` の計算と `infset size by win/lose:` の出力
- `infset_iswin.cpp:69-70` の `lose_action.first == 9` のときの
  `output_actions_history(history, true)` (標準出力に出るため)

`infset_cnt` の計算は今まで通りメモリ上の `abs_history` / `only_history` を使う。
**`abs_history` と `only_history` はメモリ上に残す。ファイルに書くのをやめるだけ。**

---

## 1. 新ファイル `save_load_winlose.hpp` (`save_load_abshistory.hpp` を改名)

`save_load_abshistory.hpp` を `git mv` で `save_load_winlose.hpp` にし、中身を
総入れ替えする。`save_bin_abs` / `load_bin_abs` は削除する (呼び出し元は
`infset_iswin.cpp` と、この変更で削除する `compare_abscfr.cpp` の 2 つだけ)。

### レコード型

```cpp
#ifndef SAVE_LOAD_WINLOSE_HPP
#define SAVE_LOAD_WINLOSE_HPP
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

// 意味の違う値を同じ欄に入れると読む側が必ず取り違えるので、slot の読み方を
// 別の欄で示す (ADR 0008 で Card / MaybeCard を分けたのと同じ理由)。
struct winlose_record {
  std::string history; // rnd の情報集合の履歴
  bool is_win; // true = 必勝、false = 必敗
  bool is_choice_node; // true = 魔術師の対象選択ノード
  unsigned char slot; // 0 か 1。
                      // is_choice_node が偽なら手札スロット (0 = hand_s[0]、1 = hand_s[1])
                      // is_choice_node が真なら魔術師の対象 (0 = 自分、1 = 相手)
};
#endif
```

### ファイル形式

```
[size_t          行数]
以降、1 行ごとに:
  [uint8_t  履歴のバイト長]   1〜255
  [履歴のバイト列]
  [uint8_t  フラグ]
```

フラグの詰め方 (下位ビットから):

| ビット | 意味 |
|---|---|
| bit0 | `is_win` (1 = 必勝、0 = 必敗) |
| bit1 | `slot` (0 か 1) |
| bit2 | `is_choice_node` (1 = 魔術師の対象選択ノード) |
| bit3-7 | 予約。**書くときは 0、読むときは 0 でなければエラーにする** |

履歴の長さは `uint8_t` にする。rnd の履歴は最長でも 20 文字程度で、今の
`size_t` (8 バイト) は 1 行あたり 7 バイトの無駄になる (270 万行で約 19MB)。
**255 を超えたら書き込み時にエラーで落とす** (無言で切り詰めない)。

### API

```cpp
void save_bin_winlose(const std::string& filename,
                      const std::vector<winlose_record>& rows);
void load_bin_winlose(const std::string& filename,
                      std::vector<winlose_record>& rows);
```

- `save_bin_winlose` は今の `save_bin_abs` と同じく、CWD 相対の `abs/` が
  無ければ `fs::create_directories` で作り、`abs/<filename>` に書く。
  この挙動は `regress.sh` が作業用ディレクトリで `win` を走らせる前提
  (`regress.sh:129-135`) に依存しているので**変えない**。
- `load_bin_winlose` も `fs::path("abs") / filename` を開く。
- 書き込み・読み込みとも、`ofs` / `ifs` の失敗と、読み込み時の
  「行数ぶん読む前に EOF」「予約ビットが 0 でない」「長さが 0」を検出したら
  `std::fprintf(stderr, ...)` してから `std::abort()` する。
  `assert` は使わない (通常ビルドは `-DNDEBUG` で無効になるため。
  `card_table.hpp:11-14` の `card_range_error` と同じ方針)。

---

## 2. `infset_iswin.cpp`

### 2.1 include とグローバル

`infset_iswin.cpp:16` の

```cpp
#include "save_load_abshistory.hpp"
```

を

```cpp
#include "save_load_winlose.hpp"
```

にする。

`infset_iswin.cpp:32-33` の

```cpp
std::set<std::string> only_history;
std::map<std::string, bool> abs_history;
```

は**そのまま残す** (`infset_cnt` の計算に使うため)。その下に 2 本足す:

```cpp
std::vector<winlose_record> win_rows;
std::vector<winlose_record> lose_rows;
```

### 2.2 `cnt_abs` — 必勝側 (`infset_iswin.cpp:49-59`)

現状:

```cpp
  auto res_win = wc.is_win(bs);

  if(res_win.first > 0) {
    win_move[res_win.second]++;
    abs_history.insert({history, true});

    if(res_win.second > hist_max) {
      hist_max = res_win.second;
      max_history = history;
    }
  } else win_move[0]++;
```

`abs_history.insert({history, true});` の**行はそのまま残し**、その直後に
必勝行の追加を足す。`is_win` は手数の短い方のカードしか返さない
(`belief_state_win.hpp:129-146`) ので、**`is_win` の戻り値からスロットを
決めてはいけない**。`is_win` と同じ分岐構造をなぞって、スロットごとに
`use_win` / `wiz_win` を呼び直す。`use_win` と `wiz_win` はメモ化されている
(`belief_state_win.hpp:18,22`) ので、`is_win` が既に呼んだぶんは表から返り、
追加コストはほぼ無い。

`res_win.first > 0` が真のときだけ、以下を行う:

```cpp
    // 終局判定を最初に見る。is_win も先頭でこれを呼び、-1 以外なら他の分岐を
    // 通らずに返す (belief_state_win.hpp:91-92)。
    auto term = is_terminated_win(bs);
    if(term.first != -1) {
      // 終局判定は勝てるカードを 1 枚名指しする ({1,1} / {3,1} / {5,1}、
      // または残り 1 枚のときの {hand_s[0], 0})。そのスロットだけを書く。
      const int tslot =
          (bs.hand_s[0].has_value() && bs.hand_s[0].value().value() == term.first) ? 0 : 1;
      win_rows.push_back({history, true, false, (unsigned char)tslot});
    } else if(!bs.hand_s[1].has_value() && bs.is_sol_choice) {
      // rnd では兵士の宣言は自然手番なので、宣言ノードが情報集合表に入ることは
      // 無いはず。通ったら行動を特定できないので、誤った行を書く代わりに落とす。
      winlose_unreachable(history, "sol_choice");
    } else if(!bs.hand_s[1].has_value() && bs.is_wiz_choice) {
      // 0 = 自分 (to0p = true)、1 = 相手 (to0p = false)。
      // is_lose の wiz 分岐 (belief_state_lose.hpp:44-48) と同じ対応。
      if(wc.wiz_win(bs, true).first) win_rows.push_back({history, true, true, 0});
      if(wc.wiz_win(bs, false).first) win_rows.push_back({history, true, true, 1});
    } else if(bs.is_my_turn && bs.hand_s[1].has_value()) {
      if(bs.hand_s[0] == bs.hand_s[1]) {
        // is_win と同じく片方だけ評価する。行動は 1 つしかない。
        if(wc.use_win(bs, bs.hand_s[0].value()).first)
          win_rows.push_back({history, true, false, 0});
      } else {
        if(wc.use_win(bs, bs.hand_s[0].value()).first)
          win_rows.push_back({history, true, false, 0});
        if(wc.use_win(bs, bs.hand_s[1].value()).first)
          win_rows.push_back({history, true, false, 1});
      }
    } else {
      // is_win の分岐をすべてなぞった上で残るものは無いはず。
      winlose_unreachable(history, "no_branch");
    }
```

### 終局判定を先に見る理由 (最重要)

**`use_win` は先頭で `is_terminated_win` を呼んで短絡する。**

```cpp
std::pair<bool, int> belief_state_win_checker::use_win_impl(const belief_state& bs, Card card) {
  CW_BUMP(use_win);
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;          // ← card を一切見ずに返す
  ...
  if(card == Card{8}) return {false, 0};   // ← ここまで来ない
```

`is_terminated_win` は `bs.hand_s[1].has_value()` が真のときにも発火し
(`belief_state_win.hpp` の `!bs.barrier_e && bs.hand_s[1].has_value()` の分岐)、
`{1, 1}` `{3, 1}` `{5, 1}` を返す。戻り値は `pair<int,int>` から
`pair<bool,int>` に暗黙変換されるので、`1` / `3` / `5` はすべて `true` になる。

つまり**終局判定が効く局面では、`use_win(bs, どのカードでも)` が `true` を返す**。
2 枚手札の分岐で両スロットに `use_win` を呼ぶと、**両方が必勝として記録される**。
手札が {兵士(1), 姫(8)} なら、姫を捨てて即負けするスロットまで必勝になる。

終局判定は勝てるカードを 1 枚**名指ししている**ので、そのスロットだけを書く。
もう片方も勝てるかどうかは判定が評価していないので、**書かない**。今の
`abs_history` が「この情報集合は必勝」としか記録していないのと同じ保守性である。

`is_terminated_win` を呼び直すコストは問題にならない。メモ化されていないが
再帰せず、スカラ比較と `count_deck()` だけの浅い関数である。`CW_BUMP` は
`cfrorgcnt` (= `cfr_org.cpp`) の集計にしか出ず、`win` はカウンタを出力しないので
`regress.sh` の `cfrorg-*-counters` は動かない。

### `winlose_unreachable`

兵士の宣言ノード (`is_sol_choice`) は、**rnd の情報集合表には現れないはず**である。
rnd では兵士の宣言が自然手番だから。`belief_state` にこの分岐があるのは
org 側 (`cfrorg`) が同じ型を使うためで、rnd の走査が通る想定ではない。
最後の `else` も、`is_win` の分岐をすべてなぞった後なので通らないはず。

とはいえ「はず」なので、**通ったら黙って誤った行を書くのではなく落とす**。
`assert` は使わない (通常ビルドは `-DNDEBUG` で無効)。`save_load_winlose.hpp` に
置く:

```cpp
[[noreturn]] inline void winlose_unreachable(const std::string& history, const char* what) {
  std::fprintf(stderr,
               "winlose: rnd に無いはずの情報集合に当たった (%s), history length %zu\n",
               what, history.size());
  std::abort();
}
```

これは検証も兼ねている。`win 5 5 7` と `win 4 4 6` が最後まで走り切れば、
この 2 つが rnd に現れないことが実際に確かめられたことになる。

現状のループ本体はそのまま残し (`lose_move[...]++` と `abs_history.insert` と
`output_actions_history` は動かさない)、**`false` を 1 件挿入するのと同じ条件で**
`lose_rows` に 1 行足す。

今のコードは `able_actions` が返したアクションコードの**本数だけ** `abs_history` に
挿入している (`infset_iswin.cpp:78-88`)。道化(2) や将軍(6) は相手の手札の候補ごとに
別のコードになるため、同じカードに対して複数件入る。新しい形式は行動 = スロットなので
**カードあたり 1 行**にまとめる。`able_actions` が空を返したときは今も
`abs_history` に何も入らないので、**新形式でも行を書かない**。

`lose_action.first` の値の意味 (`belief_state_lose.hpp:22-24` のコメント):

| 値 | 意味 | 新形式の行 |
|---|---|---|
| `9` | ルール上すでに敗北 | **行を書かない** (今も `abs_history` に入らない) |
| `0` / `1` (`is_wiz_choice` のとき) | 魔術師の対象。0 = 自分、1 = 相手 | `is_choice_node` = 真、`slot` = その値 |
| `1`〜`8` (上記以外) | 必敗のカード | `is_choice_node` = 偽、スロットは下記で決める |

`15` / `25` は `is_lose` のコメントにあるが `belief_state_lose.hpp:24` の
本体では `wiz_lose` の結果を `0` / `1` に詰め替えて返しており、
`is_lose` の戻り値としては現れない。**`15` / `25` の分岐は書かない。**

**`lose_action.first` が `0` / `1` のときにカードとして扱ってはいけない。**
`bs.is_wiz_choice` が真のとき、`0` / `1` は魔術師の対象であって手札のスロットでは
ない。このとき `hand_s[1]` は空で `hand_s[0]` に 1 枚だけ残っているので、
カードとして照合すると `hand_s[0]` がたまたま兵士(1)だった場合に誤って
スロット 0 になる。**`bs.is_wiz_choice` で先に分岐すること。**

**重複の除去が要る。** `hand_s[0] == hand_s[1]` のとき `is_lose` は同じカードを
2 回 push する (`belief_state_lose.hpp:51-55` が `use_lose` を両方に呼び、
どちらも同じ結果を返すため)。カードからスロットを引くとどちらもスロット 0 に
なるので、同じ行が 2 本できる。今は `abs_history` が `map` なので勝手に 1 件に
まとまっており、`vector` に変えると重複がそのまま残る。1 つの情報集合あたり
最大 2 行なので、`cnt_abs` のローカルに `bool pushed[2] = {false, false}` を
置いて `slot` で引けばよい (`is_choice_node` は 1 つの情報集合の中では混ざらない。
`is_lose` は魔術師の分岐とカード 2 枚の分岐のどちらか一方しか通らないため)。

### 変更後の `infset_iswin.cpp:61-90` の全体

追加する行に `// 追加` を付けた。**それ以外の行は 1 文字も変えない。**

```cpp
  auto lose_actions = lc.is_lose(bs);
  int act_cnt = action_count(bs);
  int able_act = act_cnt - lose_actions.size();
  lose_move[0] += able_act;
  if(able_act == 1 && act_cnt > 1) only_history.insert(history);

  bool pushed[2] = {false, false}; // 追加: 同じスロットを 2 回積まないための印

  for(const auto& lose_action : lose_actions) {
    lose_move[lose_action.second]++;

    if(lose_action.first == 9) {
      output_actions_history(history, true);
    } else {
      // 特定のアクションの先が必敗の場合（元コードのロジックを忠実に再現）
      string action = rph.get_action((unsigned char)history[0]);

      int firstp = char_to_action(action[0]) / 10;
      auto actions = able_actions(bs, lose_action.first, firstp == 2);

      // 追加: able_actions が空なら今も abs_history に何も入らないので、行も書かない
      if(!actions.empty()) {
        bool is_choice_node;
        int slot;
        if(bs.is_wiz_choice) {
          // lose_action.first は 0 = 自分 / 1 = 相手。カードではない
          is_choice_node = true;
          slot = lose_action.first;
        } else {
          is_choice_node = false;
          slot = (bs.hand_s[0].has_value()
                  && bs.hand_s[0].value().value() == lose_action.first)
                     ? 0
                     : 1;
        }
        if(!pushed[slot]) {
          pushed[slot] = true;
          lose_rows.push_back({history, false, is_choice_node, (unsigned char)slot});
        }
      }

      for(int act : actions) {
        string new_hist;
        if(bs.is_wiz_choice) {
          new_hist = history.substr(0, history.length() - 1) + string(1, action2char(act, true));
        } else {
          new_hist = history + string(1, action2char(act, true));
        }

        // それぞれの new_hist を挿入
        abs_history.insert({new_hist, false});
      }
    }
  }
```

`able_actions` の呼び出しと `abs_history.insert` のループは**そのまま残す**
(`infset_cnt` の計算がこの `abs_history` を使うため)。

### 2.4 書き出し (`infset_iswin.cpp:155-156`)

現状:

```cpp
  string filename = "abs" + to_string(open[0] * 100 + open[1] * 10 + open[2]) + ".bin";
  save_bin_abs(filename, abs_history, only_history);
```

これを置き換える:

```cpp
  string subgame = to_string(open[0] * 100 + open[1] * 10 + open[2]);
  save_bin_winlose("wininf" + subgame + ".bin", win_rows);
  save_bin_winlose("loseinf" + subgame + ".bin", lose_rows);
```

`abs<部分ゲーム>.bin` は**もう書かない**。`only_history` は**ファイルに書かない**
(`infset_cnt` の計算に使うだけ)。

---

## 3. `compare_abscfr.cpp` と Makefile の `comp` を削除

`compare_abscfr.cpp` は `save_load_abshistory.hpp` の `load_bin_abs` を
呼んでいる (`compare_abscfr.cpp:56,147`) ので、この変更でビルドが通らなくなる。
次の変更で `compare_strategy.cpp` として新規に作り直すことが決まっているため、
追随はさせず削除する。

```
git rm compare_abscfr.cpp
```

`Makefile:55-59` の `comp` ターゲットを削除し、`Makefile:17` の

```
all: cfr cfr0 cfrorg watch brrnd brorg test win comp end
```

から `comp` を外す。

`.gitignore` の `comp` の行は**残す** (次の変更で同じ名前のバイナリを作るため)。

---

## 4. `regress.sh`

`regress.sh:144-153` が `abs/abs<部分ゲーム>.bin` を 1 本だけ想定している。
2 本に増やす。

```sh
        for kind in wininf loseinf; do
            binfile="$WIN_WORK/abs/$kind$sg.bin"
            if [ -e "$binfile" ]; then
                sha256sum < "$binfile" > "$outdir/$name.$kind.sha256"
            else
                echo "($kind bin was not produced)" > "$outdir/$name.$kind.sha256"
            fi
        done

        if [ "$mode" = check ]; then
            compare "$name-stdout" "$outdir/$name.out" "$base_abs/$name.out"
            compare "$name-wininf" "$outdir/$name.wininf.sha256" "$base_abs/$name.wininf.sha256"
            compare "$name-loseinf" "$outdir/$name.loseinf.sha256" "$base_abs/$name.loseinf.sha256"
        else
            echo "saved    $name"
        fi
```

冒頭コメント `regress.sh:22-24` と `regress.sh:31-35` の `abs<部分ゲーム>.bin`
への言及も、2 本の新しい名前に直す。`mktemp` の作業用ディレクトリで走らせる
仕組み (`regress.sh:50-58,129-135`) は**変えない**。

---

## 5. `Makefile` の `win` ターゲット

`Makefile:49-52` の依存にある `save_load_abshistory.hpp` を
`save_load_winlose.hpp` に直す。

```
win: infset_iswin.cpp save_load_winlose.hpp \
     belief_state.hpp belief_state_history.hpp belief_state_win.hpp belief_state_lose.hpp \
     $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 $(COMMON_WARN) -O2 $(COMMON_DEFS)  $(COMMON_SRCS) infset_iswin.cpp -o $@
```

---

## 6. `CLAUDE.md`

「既知の未修正の不具合」の項 (`./comp 5 5 7` が実行のたびに結果が変わる) を
**削除する**。`comp` を消したので不具合ごと無くなる。

ただし `rnd_action.hpp` / `org_action.hpp` の `get_action` が範囲外を `assert`
任せにしている件は残るので、その 1 文だけ「やり残し」として残す:

```
- **やり残し**: `rnd_action.hpp` / `org_action.hpp` の `get_action` は範囲外を
  `assert`（NDEBUG で無効）に任せて `wordlist[v]` を返す。常時有効の検査に
  すべきだが未着手。
```

---

## 7. `CONTEXT.md`

「解析コードの語彙」の節に 1 項足す。実装の詳細は書かない (用語集なので)。

```
**必勝・必敗の記録**:
必勝判定・必敗判定の結果を、情報集合と行動の対ごとに 1 行として残したもの。
行動は手札のどちらのスロットを出すかで表す。必勝と必敗で別のファイルに分ける。
_Avoid_: abs、削減データ、判定ログ
```

---

## 検証

### ビルド

```
make cfr cfr0 cfrorg brrnd brorg win
```

`comp` を消したのでビルド対象から外れる。`make all` は `watch` のリンクエラーで
止まるので使わない (既知)。

### 統計が動いていないことの確認 (この変更の合否そのもの)

変更前に基準を取ってあること。変更後:

```
mkdir -p /tmp/winchk/abs && cd /tmp/winchk && /home/yukiya_linux/love/win 5 5 7 > new-557.out
mkdir -p /tmp/winchk2/abs && cd /tmp/winchk2 && /home/yukiya_linux/love/win 4 4 6 > new-446.out
diff new-557.out logs/regress/base/win-557.out
diff new-446.out logs/regress/base/win-446.out
```

**差分が 1 行でもあったら不合格。** 判定か統計に触ってしまっている。

`assert` は通常ビルドでは無効なので、`infset_iswin.cpp:116-117` の 2 本を
効かせたいときは `-DNDEBUG` を外してビルドし直す。

### 出力ファイルの確認

```
ls -la /tmp/winchk/abs/
```

`wininf557.bin` と `loseinf557.bin` の 2 本だけがあること。`abs557.bin` が
作られていないこと。

### 回帰ハーネスの基準の取り直し

上の diff が一致したら:

```
./regress.sh save
```

この変更は基準を無効にするので、`check` ではなく `save` を実行する。
**基準を取り直す前に必ず上の diff を通すこと。** 取り直してしまうと、
統計が動いていたかどうかを後から確かめる手段が無くなる。

---

## やらないこと

- prefix-minimal な統合ファイルは**作らない**。削減統計は今まで通り
  `infset_iswin.cpp:129-150` がメモリ上で計算する
- `only_history` を**ファイルに書かない**
- `comp` の作り直し (次の変更)
- `get_action` の範囲検査 (次の変更)
- 判定 (`belief_state_win.hpp` / `belief_state_lose.hpp`) の変更。
  `use_win` / `wiz_win` / `use_lose` を**呼ぶ**だけで、中身は触らない
- 既存の `abs/abs*.bin` 5 本の削除。読めなくなるが消さない
