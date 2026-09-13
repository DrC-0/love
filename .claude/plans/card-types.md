# カードに型を与える (Card / MaybeCard)

## 目的

カードを表す値がすべて `int` で、「カードが必ずある」箇所と「カードが無いかも
しれない」箇所が型で区別できない。番兵の `0` が「無し / 不明」を意味するが、
その判定が場所によって `> 0` と `> 1` に分かれている。

実際にこれで間違えた箇所がある。`is_openhand_loveletter` は「双方の手札が互いに
既知」を判定するのに `open_e() > 1` と書かれていた。`> 1` は兵士の判定
(`is_terminated_win`) で「確定していて、かつカード1でない」を意味する形
(兵士はカード1を宣言できない) で、開手ラブレターでは意味を持たない。
既に修正済みだが、**型があれば書けなかった**。

もう一例。大臣のルールは 2 箇所でこう書かれている。

```cpp
if(bs.have_s(7) && bs.hand_s[0] + bs.hand_s[1] >= 12)
```

これは**カード同士の足し算**で、しかも手札が 1 枚のとき `hand_s[1] == 0` が
加法単位元として働くことに暗黙に依存している (合計が最大 8 で 12 未満だから
発火しない)。正しいが、読んで分かる人はいない。

**振る舞いは一切変えない。** 回帰ハーネス 12 項目すべて一致が成功条件。
ロジックを変えないので決定的カウンタも動かないはず。

## org と rnd のどちらを触るか

**org 固有のコードにも rnd 固有のコードにも触らない。**

`belief_state` はもともと org と rnd の両方から共有される型で、
org / rnd の切り替えは `rnd` 引数 1 つだけで行われる。今回変えるのは
**カードを表す値の型**であって、org / rnd の切り替えには一切関係しない。

したがって次のファイルは**1 行も変更しない**。

- 展開器: `org_tree.hpp` (org 側) / `rnd_make_infset.hpp` (rnd 側)
- 対で存在するヘッダ: `org_action*.hpp` / `rnd_action*.hpp` /
  `org_action_sequense.*` / `rnd_action_sequense.*`
- `all_elements.hpp` / `all_elements_rnd.hpp` /
  `infset_dfs.hpp` / `infset_dfs_rnd.hpp`

`rnd` 引数を持つ関数 (履歴コンストラクタ、`action2char`、`get_actions_history`、
`output_actions_history`) の `rnd` 引数の型も意味も変えない。

## 行動履歴に手を入れるか

**入れない。** ただし `belief_state_history.hpp` のコンストラクタは
`hand_s` / `open_flag_*` / `sol_flag_*` に代入するので、**代入の右辺だけ**
`Card` / `MaybeCard` の構築に変わる。履歴文字列の解釈も、末尾の追記も、
payload 付きへの置換も変更しない。

## 変更 1: 2 つの型を作る

`card_table.hpp` に置く (`max_num` と同じ、カードの定数の置き場)。
assert を使うので `#include <cassert>` を足すこと。

```cpp
// カード。必ず 1〜8 のいずれか。「無い」状態は表せない。
class Card {
public:
  constexpr explicit Card(int v)
    : v_(v) { assert(1 <= v && v <= 8); }
  constexpr int value() const { return v_; }   // 1..8
  constexpr int index() const { return v_ - 1; } // 0..7、trash[] / max_num[] の添字
  constexpr bool operator==(const Card&) const = default;
  constexpr auto operator<=>(const Card&) const = default;
private:
  int v_;
};

// カード、または「無い / 判明していない」。格納は 0 が「無い」。
class MaybeCard {
public:
  constexpr MaybeCard()
    : v_(0) {}
  constexpr explicit MaybeCard(int v)
    : v_(v) {}
  constexpr MaybeCard(Card c)
    : v_(c.value()) {}
  static constexpr MaybeCard none() { return MaybeCard(); }
  constexpr bool has_value() const { return v_ != 0; }
  constexpr Card value() const {                      // has_value() が真のときだけ
    assert(v_ != 0);
    return Card(v_);
  }
  constexpr int raw() const { return v_; }           // 0..8、メモ鍵の詰め込み用
  constexpr bool operator==(const MaybeCard&) const = default;
private:
  int v_;
};
```

**`MaybeCard` に順序比較を入れないこと。** `open_e() > 1` のような書き方を
封じるのがこの型の目的で、順序を入れると同じ間違いが書けてしまう。
比べたいときは `has_value()` で存在を確かめてから `value()` で `Card` にする。

どちらも `int` 1 個なので、`belief_state` のレイアウトは変わらない。
メンバ関数はすべて `constexpr` のインラインで、`-O2` では `int` と同じに落ちる
はず (性能は検証の手順 3 で確認する)。

## 変更 2: `Card` を取るようにする関数

引数の型を `int` から `Card` に変える。**引数名 `card` はそのまま。**

`belief_state.hpp`
- 自由関数 `trash_and_hand_s` / `deck_or_hand_e` / `hand_e`
- メンバ `deck_or_hand_e` / `hand_e` / `hand_s_est` / `deck`(2 種) / `have_s` /
  `other_hand_s` / `add_sol_s` / `add_sol_e`
- `reset_flag_by_use` / `draw` / `swap_player`

`belief_state_win.hpp` / `belief_state_lose.hpp`
- `use_win` / `sol_win` / `use_lose` の `card` 引数

中の `trash[card - 1]` `max_num[card - 1]` は `trash[card.index()]`
`max_num[card.index()]` になる。カードを回すループは

```cpp
for(int c = 1; c <= 8; c++) { Card card{c}; ... }
```

## 変更 3: `MaybeCard` にするフィールドと戻り値

### フィールド (`belief_state.hpp` の `struct belief_state`)

```cpp
MaybeCard open_flag_s, open_flag_e;
MaybeCard sol_flag_s[2], sol_flag_e[2];
MaybeCard hand_s[2];
```

`trash[8]` は**カードではなく枚数**なので `int` のまま。
`lt5_flag_*` `not7_flag_*` `barrier_*` `is_*` も `bool` のまま。

### 戻り値

| 関数 | 現在 | 変更後 |
|---|---|---|
| `open_e()` `open_s()` | `int` (0 = 判明せず) | `MaybeCard` |
| `other_hand_s(Card)` | `int` (0 = もう一枚が無い) | `MaybeCard` |
| `hand_e_max()` `hand_e_min()` | `int` (0 = 候補なし) | `MaybeCard` |
| `deck_or_hand_e_min()` | `int` (0 = 候補なし) | `MaybeCard` |

### 書き換えの型

| 現在 | 変更後 | 意味 |
|---|---|---|
| `open_flag_e > 0` | `open_flag_e.has_value()` | 判明しているか |
| `hand_s[1] == 0` | `!hand_s[1].has_value()` | 手札が 1 枚か |
| `hand_s[1] > 0` | `hand_s[1].has_value()` | 手札が 2 枚か |
| `open_e() > 1` | `open_e().has_value() && open_e().value() != Card{1}` | 兵士の判定 |
| `sol_flag_e[0] == 0` | `!sol_flag_e[0].has_value()` | 空きスロット |
| `hand_s[0] < 3` | `hand_s[0].value() < Card{3}` | カード同士の比較 |
| `max < bs.hand_s[0]` | `max.value() < bs.hand_s[0].value()` | 同上 |

`hand_s[0]` は手番中であれば必ず存在するので `.value()` を直に呼んでよい。
**`hand_s[1]` は存在しないことがある。** `.value()` を呼ぶ前に必ず
`has_value()` を確かめること。

## 変更 4: 大臣のルール (2 箇所)

`belief_state_win.hpp` の `is_terminated_win` と `belief_state_lose.hpp` の
`is_lose`。

```cpp
// 現在
if(bs.have_s(7) && bs.hand_s[0] + bs.hand_s[1] >= 12) {

// 変更後
if(bs.have_s(Card{7}) && bs.hand_s[1].has_value()
   && bs.hand_s[0].value().value() + bs.hand_s[1].value().value() >= 12) {
```

**`hand_s[1].has_value()` の判定を必ず足すこと。** 現在は `hand_s[1] == 0` が
加法単位元として働き、手札 1 枚のときは合計が 12 未満になるので結果的に発火
しない。省くと `MaybeCard::value()` を無い状態で呼ぶことになる。
**足しても結果は変わらない** (回帰ハーネスで確認できる)。

`.value().value()` が読みにくいので、`Card` に `value()` があることを踏まえ
`bs.hand_s[0].value().value()` の代わりに局所変数を置いてよい。

## 変更 5: メモ鍵 (`belief_state_win_checker::key`)

`put()` に渡す 8 箇所を `.raw()` にする。**ビット幅は変えない** (0..8 の 4bit)。

```cpp
put(bs.open_flag_s.raw(), 4);   put(bs.open_flag_e.raw(), 4);
put(bs.sol_flag_s[0].raw(), 4); put(bs.sol_flag_s[1].raw(), 4);
put(bs.sol_flag_e[0].raw(), 4); put(bs.sol_flag_e[1].raw(), 4);
put(bs.hand_s[0].raw(), 4);     put(bs.hand_s[1].raw(), 4);
```

`extra` は `card.value()` (1..8) または `to0p ? 1 : 0`。**現在と同じ値を詰める
こと。** 鍵が変わるとメモの当たり方が変わり、決定的カウンタが動いて
「振る舞いを変えていない」ことを示せなくなる。

## 変更 6: `able_actions` は対象外のまま

行動コードは 0-origin の添字を桁に埋め込むシリアライズ形式なので、
`belief_state_win.hpp` の `able_actions` (`468-505` 付近) の中は**添字のまま**。
アクセサが `Card` を取るようになるので、呼び出し 5 箇所を

```cpp
if(bs.hand_e(Card{i + 1}))
```

にする。`push_back` の中身、`int base = 40 + card - 1;`、
`int other_i = bs.other_hand_s(card) - 1;` は変える必要がある
(`card` が `Card`、`other_hand_s` が `MaybeCard` を返すため) が、
**埋め込む数値は現在と同じでなければならない**。

```cpp
int base = 40 + card.index();
int other_i = bs.other_hand_s(card).value().index();
```

`other_hand_s` はここで必ず値を返す。`card == 3` の分岐は手札 2 枚が前提で、
`other_hand_s(Card{3})` は「3 でないほうの手札」を返すため。
**`has_value()` の判定は足さないこと。** 足すと、偽のときに何をするかという
分岐が新しく生まれ、現在のコードに無い振る舞いを決めることになる。
この変更は振る舞いを変えないのが条件なので、`.value()` を直に呼ぶ。

## 変更 7: 外部の 3 箇所

| 場所 | 現在 | 変更後 |
|---|---|---|
| `first_hand_open.hpp:14` | `bs.hand_s[1] == 0` | `!bs.hand_s[1].has_value()` |
| `first_hand_open.hpp:27` | `bs0.hand_s[1] == 0` (と bs1) | 同上 |
| `watch_cfr.cpp:44` | `bs.hand_s[1] == 0` | 同上 |

同じ行の `open_e() > 0` は `open_e().has_value()` になる。

## 変更 7b: `visit_winlose.hpp` の判定呼び出し 5 箇所

`use_win` / `use_lose` / `sol_win` が `Card` を取るようになるので、
`visit_winlose.hpp` の呼び出しを包む。**`Card` のコンストラクタは `explicit`
なので `int` からの暗黙変換は起きず、包まないとコンパイルが通らない。**

| 行 | 現在 | 変更後 |
|---|---|---|
| 81 | `wc.use_win(bs, c1)` | `wc.use_win(bs, Card{c1})` |
| 82 | `wc.use_win(bs, c2)` | `wc.use_win(bs, Card{c2})` |
| 83 | `lc.use_lose(bs, c1)` | `lc.use_lose(bs, Card{c1})` |
| 84 | `lc.use_lose(bs, c2)` | `lc.use_lose(bs, Card{c2})` |
| 129 | `wc.sol_win(soldier_bs[n.depth], i)` | `wc.sol_win(soldier_bs[n.depth], Card{i})` |

**フックの署名 `enter_play(node &n, int c1, int c2)` と
`enter_soldier(node &n, int i)` は `int` のまま変えないこと。**
これらは `org_tree.hpp:153` と `:187` から呼ばれており、`org_tree.hpp` は
1 行も変更しないため。

`c1` / `c2` / `i` は**すでにカード (1-origin)** なので、`Card{c1}` のように
そのまま包めばよい。`- 1` も `+ 1` も足さないこと。

- `c1` / `c2` は `org_tree.hpp` で `n.hand1[0]` / `n.hand1[1]` (手札のカード)
- `i` は `org_tree.hpp:187` 付近の `for(int i = 2; i < 9; i++)` の宣言カード
  (兵士はカード 1 を宣言できないので 2 から始まる)

`wc.wiz_win(bs, 0)` / `lc.wiz_lose(bs, 1)` は `bool` 引数なので**変更不要**。

## 変更 8: 使われていない全項目コンストラクタを削除する

`belief_state.hpp:37-52` の

```cpp
belief_state(bool is_my_turn, ..., int sol_flag_s[2], int sol_flag_e[2],
             const int hand_s[2], const int trash[8])
```

は**どこからも呼ばれていない** (`belief_state(true` / `belief_state(false` /
`belief_state(bs.` のいずれも該当なし)。型を入れると署名を書き換えることに
なるが、使われていないので**削除する**。既定コンストラクタと履歴コンストラクタ
の 2 つは残す。

## 変更しないもの

- **判定の戻り値の規約。** `is_win` / `is_terminated_win` は `std::pair<int, int>`
  のまま。第 1 要素はカード (1..8) だけでなく `0` と `-1` (終局判定に該当せず) も
  取る **3 状態**で、`MaybeCard` (無し + 1..8) では表せない。`is_lose` の
  `std::vector<std::pair<int, int>>` も同じ。
- `trash[8]` と `max_num[8]`。カードではなく枚数。
- `able_actions` の行動コードの符号化。
- `count_work.hpp` のカウンタ名。**絶対に変えない。**
- メモ鍵のビット幅と詰める順序。
- `bs_set.hpp` / `endgame.hpp` / `loveletter.cpp` / `org_tree.hpp` /
  `rnd_make_infset.hpp` / `test.cpp`。

## 検証

### 手順 1: ビルド

```
make cfr cfr0 cfrorg brrnd brorg win comp cfrorgcnt
```

警告ゼロ。`make test` は通らない (既知)。

### 手順 2: 回帰ハーネス

```
./regress.sh check full
```

**12 項目すべて `OK`。** 今回はロジックを変えないので**決定的カウンタも一致
しなければならない**。カウンタが動いたらメモ鍵の詰め方を変えてしまっている。

### 手順 3: 壁時計

型が `int` と同じに落ちているかを確認する。ADR 0003 の手順。

```
for r in 1 2 3 4 5; do /usr/bin/time -f "%e" ./cfrorg 4 4 6 5 >/dev/null; done
```

**基準は 6.83s (メモ化後の実測)。10% 以上遅くなっていたら型が零コストに
なっていない**ので報告すること。

### 手順 4: assert を有効にして回す

`Card` のコンストラクタと `MaybeCard::value()` の assert が、**無い値から
`Card` を作ろうとする間違いを捕まえる**。通常ビルドは `-DNDEBUG` なので効かない。

```
make cfrorgd            # assert 有効。出力名は cfrorg のまま
./cfrorg 5 5 7 7
./cfrorg 4 4 6 7
```

**どちらも assert で落ちずに完走すること。** 落ちたら `has_value()` の確認を
飛ばして `.value()` を呼んでいる箇所がある。落ちた時点のスタックでその場所が
分かる。

確認が済んだら通常ビルドに戻すこと (`make cfrorg`)。**手順 2 と 3 は通常ビルドで
行う** (assert 有効のまま壁時計を測ると遅くなる)。

### 手順 5: 型が変わった関数を `int` のまま呼んでいる箇所が無いか

`Card` のコンストラクタが `explicit` なのでコンパイルエラーになるが、
**どこを直すべきかを先に洗い出しておく**。

```
grep -n "use_win(\|use_lose(\|sol_win(\|hand_e(\|deck(\|deck_or_hand_e(\|hand_s_est(\|have_s(\|other_hand_s(\|add_sol_[se](\|reset_flag_by_use(\|draw(\|swap_player(" \
  visit_winlose.hpp infset_iswin.cpp compare_abscfr.cpp watch_cfr.cpp first_hand_open.hpp
```

出てきた箇所の引数がすべて `Card` になっていること。
`belief_state*.hpp` の外から呼んでいるのは
**`visit_winlose.hpp` の 5 箇所 (変更 7b) だけ**のはずで、
`first_hand_open.hpp` / `watch_cfr.cpp` / `infset_iswin.cpp` /
`compare_abscfr.cpp` は `open_e()` `hand_s[1]` `is_win` `is_lose`
`able_actions` `action_count` しか使っていない (変更 7 を参照)。

### 手順 6: 番兵の書き方が残っていないか

```
grep -n "hand_s\[[01]\] == 0\|hand_s\[[01]\] > 0\|open_flag_[se] > 0\|open_e() > [01]\|sol_flag_[se]\[[01]\] == 0" belief_state*.hpp first_hand_open.hpp watch_cfr.cpp
```

何も出ないこと。`MaybeCard` に順序比較が無いので、残っていればコンパイルが
通らないはずだが、念のため確認する。
