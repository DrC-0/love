# Belief State の interface をカード (1-origin) に統一する

## 目的

`belief_state` の interface は 0-origin の添字と 1-origin のカードを混在させて
いる。どちらも型は `int` で、名前にも区別が無い。

```
0-origin (添字) を取る : hand_e(i)  deck(i)  deck_or_hand_e(i)  hand_s_est(i)
1-origin (カード) を取る: have_s(card)  other_hand_s(card)  add_sol_s/e(card)
                          use_win(bs, card)  sol_win(bs, card)  use_lose(bs, card)
                          draw(bs, draw_card)  reset_flag_by_use(..., card)
```

このため `for(int i = 0; i < 8; i++)` の中で `bs.deck_or_hand_e(i)` と
`if(i + 1 == 3)` が隣り合う。**`i + 1` / `j + 1` が 49 箇所**ある。

添字を取る側をカードに揃えると、49 箇所のうち 38 箇所が消え、代わりに
`card - 1` が 6 箇所増える (`trash[]` と `max_num[]` の添字)。

**振る舞いは変えない。** 回帰ハーネスの 12 項目が一致することが成功条件。

用語は `CONTEXT.md`、Belief State の層構造は `docs/adr/0006-belief-state-layers.md`
を参照。

## org と rnd のどちらを触るか

**どちらも触らない。** 変更するのは `belief_state*.hpp` の 3 ファイルだけで、
`rnd` 引数を持つ関数 (履歴コンストラクタ、`action2char`、`get_actions_history`)
には手を入れない。org 側の展開器 (`org_tree.hpp`)、rnd 側の展開器
(`rnd_make_infset.hpp`)、対で存在するヘッダ (`org_action*.hpp` / `rnd_action*.hpp`)
も触らない。

## 行動履歴に手を入れるか

**入れない。** 履歴文字列を作る・解釈するコードには触らない。末尾の追記も
payload 付きへの置換も行わない。`belief_state_history.hpp` は 1 行も変えない。

ただし**行動コードの符号化**には注意が要る (下の `able_actions` の節を参照)。

## 変更 1: 引数の意味を添字からカードに変える関数 (7 本)

`belief_state.hpp` の以下 7 本。**引数名も `i` から `card` に変える** (名前で
どちらか分かるようにするのがこの変更の目的なので、名前を変えないと意味が無い)。

### 自由関数 3 本 (宣言 L77-79、定義 L108-135)

```cpp
int trash_and_hand_s(const int card, const int hand[2], const int trash[8]) {
  return trash[card - 1] + (hand[0] == card ? 1 : 0) + (hand[1] == card ? 1 : 0);
}

int deck_or_hand_e(const int card, const int hand[2], const int trash[8]) {
  CW_BUMP(deck_or_hand_e);
  return max_num[card - 1] - trash_and_hand_s(card, hand, trash);
}

bool hand_e(const int card, const int hand[2], const int trash[8], const int open_flag_e,
            const int sol_flag_e[2], const bool lt5_flag_e, const bool not7_flag_e) {
  CW_BUMP(hand_e);
  if(open_flag_e > 0) {
    if(open_flag_e == card) { return true; } else { return false; }
  } else if(sol_flag_e[0] > 1 && sol_flag_e[0] == card) {
    return false;
  } else if(sol_flag_e[1] > 1 && sol_flag_e[1] == card) {
    return false;
  } else if(lt5_flag_e && card >= 5) {
    return false;
  } else if(not7_flag_e && card == 7) {
    return false;
  } else {
    return deck_or_hand_e(card, hand, trash) > 0;
  }
}
```

**`- 1` が入るのは `trash[card - 1]` と `max_num[card - 1]` の 2 箇所だけ。**
`sol_flag_e[0]` `sol_flag_e[1]` `hand[0]` `hand[1]` は 2 要素配列で添字は 0/1
固定、カード添字ではないので**そのまま**。

### メンバ 4 本 (宣言 L57-62、定義 L137-160 付近)

```cpp
int  belief_state::deck_or_hand_e(int card) const;   // ::deck_or_hand_e(card, ...) に渡すだけ
bool belief_state::hand_e(int card) const;           // ::hand_e(card, ...) に渡すだけ
bool belief_state::hand_s_est(int card) const;       // ::hand_e(card, enemy_hand, ...) に渡すだけ
bool belief_state::deck(int card, int open_card) const {
  CW_BUMP(deck);
  return deck_or_hand_e(card) > (card == open_card ? 1 : 0);
}
bool belief_state::deck(int card) const { return deck(card, open_e()); }
```

`deck` の第 2 引数 `open_card` は**元からカード**で、変更しない
(`open_e()` の戻り値を受けている)。

## 変更 2: カードを回すループを 1..8 にする

下表のループを `for(int card = 1; card <= 8; card++)` に変える。
中の `i + 1` は `card` に、`i` が配列添字なら `card - 1` にする。

### `belief_state.hpp` (14 本)

| 行 | 関数 | 備考 |
|---|---|---|
| 154 | `open_e` | 結果変数が既に `card` という名前。**衝突するので結果側を `found` に改名する** |
| 168 | `open_s` | 同上。結果変数名を確認して衝突を避ける |
| 206 | `count_deck` | |
| 215 | `hand_e_max` | |
| 226 | `hand_e_min` | |
| 239 | `deck_or_hand_e_min` | |
| 346, 368, 392, 410 | `ef_wizard` | |
| 434, 438, 443 | `print` | 434 は `trash[i]` なので `trash[card - 1]` になる |
| 457 | `validate_hand_e_candidate` | |

### `belief_state_win.hpp` (7 本)

| 行 | 関数 | 範囲 |
|---|---|---|
| 161, 209 | `use_win` | 1..8 |
| 239 | `enemy_turn_win` | **`i < 7` なので 1..7** |
| 251, 325, 352 | `enemy_turn_win` | `j` のループ。1..8 |
| 385 | `draw_win` | 1..8 |

### `belief_state_lose.hpp` (7 本)

| 行 | 関数 | 範囲 |
|---|---|---|
| 74, 96, 118, 138, 152 | `use_lose` | 1..8 |
| 176 | `wiz_lose` | 1..8 |

## `- 1` が必要になる配列アクセスは 6 箇所だけ (全部ここに挙げる)

ループをカードで回すと、**ループ変数を配列の添字に直接使っている箇所**だけが
`card - 1` になる。該当は次の 6 箇所で、これ以外に `- 1` を足してはいけない。

| 場所 | 関数 | 現在 | 変更後 |
|---|---|---|---|
| `belief_state.hpp:109` | `trash_and_hand_s` | `trash[i]` | `trash[card - 1]` |
| `belief_state.hpp:114` | `deck_or_hand_e` | `max_num[i]` | `max_num[card - 1]` |
| `belief_state.hpp:372` | `ef_wizard` | `next_bs.trash[i] += 1` | `next_bs.trash[card - 1] += 1` |
| `belief_state.hpp:415` | `ef_wizard` | `next_bs.trash[i] += 1` | `next_bs.trash[card - 1] += 1` |
| `belief_state.hpp:435` | `print` | `trash[i]` | `trash[card - 1]` |
| `belief_state_win.hpp:275` | `enemy_turn_win` | `next_bs.trash[i] += 1` | `next_bs.trash[card - 1] += 1` |

**「ループをカードにしたから配列は全部 `[card - 1]` に」と一括で置き換えては
いけない。** カード添字の配列は `trash[8]` と `max_num[8]` の 2 つだけで、
`sol_flag_s[2]` `sol_flag_e[2]` `hand_s[2]` `hand[2]` は**手札や宣言の位置**を
表す 2 要素配列であり、添字は `[0]` `[1]` 固定。ここに `- 1` を足すと
添字が `-1` になって範囲外アクセスになる。

`able_actions` の中の添字は変更 5 のとおり対象外なので、この表には含まれない。

## 変更 3: 1 から始まるループは「添字の 1..7」＝「カードの 2..8」

**最も間違えやすい箇所。** 次の 4 本は `for(int i = 1; i < 8; i++)` という形を
していて、一見すでにカードで回っているように読めるが、**添字の 1..7、つまり
カード 2〜8** を回している。

| 行 | 関数 | 根拠 |
|---|---|---|
| `belief_state_win.hpp:52` | `is_win` | 中で `sol_win(bs, i + 1)` とカードに直して渡している |
| `belief_state_win.hpp:147` | `use_win` | 同上 |
| `belief_state_win.hpp:510` | `action_count` | 兵士の宣言候補を数える。カード 1 は宣言できない |
| `belief_state_lose.hpp:86` | `use_lose` | 直後の `bs.hand_e(0)` (カード 1) を別扱いしている |

カード 1 を飛ばしているのは**兵士 (1) がカード 1 を宣言できない**というルール
のため。変換後のループ見出しは**この 4 本とも次の 1 種類だけ**で、導出しないこと。

```cpp
for(int card = 2; card <= 8; card++) {
```

`card = 1` でも `card < 8` でもない。中身はこうなる。

```cpp
for(int card = 2; card <= 8; card++) {
  if(bs.hand_e(card)) {
    auto res_sol = sol_win(bs, card);   // i + 1 が card になる
```

**`for(int card = 1; card < 8; ...)` と書くと、カード 1 を誤って含めたうえで
カード 8 (姫) を落とす。** 姫は必勝・必敗判定で決定的に効くので、出力が確実に
変わる。

## 変更 4: リテラルの添字を渡している 3 箇所

| 行 | 現在 | 変更後 | 意味 |
|---|---|---|---|
| `belief_state_lose.hpp:89` | `bs.hand_e(0)` | `bs.hand_e(1)` | カード 1 (兵士) |
| `belief_state_lose.hpp:168` | `bs.hand_e(7)` | `bs.hand_e(8)` | **カード 8 (姫)** |
| `belief_state_win.hpp:253` | `bs.deck_or_hand_e(2)` | `bs.deck_or_hand_e(3)` | カード 3 (騎士) |

`hand_e(7)` を 7 のまま残すと**姫のつもりで大臣を見る**ことになる。

## 変更 5: `able_actions` は添字のまま残す (対象外)

`belief_state_win.hpp:468-505` の `able_actions` は**変換しない**。

理由。この関数の `i` / `j` は配列添字であると同時に**行動コードの桁**として
使われている。

```cpp
actions.push_back(44000 + !is_second_player * 100 + i * 10 + 0);
actions.push_back(base * 10 + i);
actions.push_back(base * 100 + other_i * 11);
actions.push_back(base * 100 + (bs.other_hand_s(card) - 1) * 10 + i);
```

行動コードは**0-origin のカード添字をそのまま桁に埋め込むシリアライズ形式**で、
カードを表す数ではない。この関数には `i + 1` が**1 つも無く**、元から添字で
一貫している。カードで回すと `(card - 1)` が 6 箇所増えるだけで、消える
`i + 1` はゼロ。

さらにこの符号化は `infset_iswin.cpp` が `new_hist` を組むのに使われ、
`abs/abs<部分ゲーム>.bin` の中身に直結する。触らないのが安全。

**ただしアクセサはカードを取るようになるので、呼び出し側だけ `+ 1` する。**

| 行 | 現在 | 変更後 |
|---|---|---|
| 476 | `if(bs.deck(j))` | `if(bs.deck(j + 1))` |
| 482 | `if(bs.hand_e(i))` | `if(bs.hand_e(i + 1))` |
| 493 | `if(bs.hand_e(i))` | `if(bs.hand_e(i + 1))` |
| 497 | `if(bs.hand_e(other_i))` | `if(bs.hand_e(other_i + 1))` |
| 501 | `if(bs.hand_e(i))` | `if(bs.hand_e(i + 1))` |

(行番号は目安。`able_actions` の中の `hand_e` / `deck` 呼び出し 5 箇所すべて。)

`push_back` の中身、`int base = 40 + card - 1;`、
`int other_i = bs.other_hand_s(card) - 1;` は**1 文字も変えない**。

関数の直前にこのコメントを置く。

```cpp
// 行動コードは 0-origin のカード添字をそのまま桁に埋め込むシリアライズ形式
// なので、この関数の中だけは添字 (0..7) で通す。Belief State のアクセサは
// カード (1..8) を取るため、呼び出しでだけ +1 する。
```

`action_count` (L507-515) は符号化を含まない数え上げなので**変換の対象に含める**
(変更 3 の表を参照)。

## 変更しないもの

- **格納形式**。`trash[8]` は 8 幅のまま、`hand_s[1] == 0` の「カード無し」も
  そのまま。`trash[9]` にも `-1` 番兵にもしない。
- `sol_flag_s[2]` `sol_flag_e[2]` `hand_s[2]` `hand[2]` の添字。これらは
  2 要素配列で `[0]` `[1]` 固定。カード添字ではない。
- `deck(card, open_card)` の第 2 引数。元からカード。
- `belief_state_history.hpp` / `first_hand_open.hpp` / `visit_winlose.hpp` /
  `infset_iswin.cpp` / `compare_abscfr.cpp` / `watch_cfr.cpp`。
  これらはアクセサを添字で呼んでいないので**変更不要**
  (grep で `hand_e(` `deck_or_hand_e(` `hand_s_est(` `deck(` が無いことを確認する)。
- `count_work.hpp` のカウンタ名。**絶対に変えない**。回帰ハーネスが
  `COUNT_WORK` 行を比較しているので、名前を変えると基準を取り直す羽目になる。
- `loveletter.cpp` の `node::deck[i - 1]` など。`node` 側は別の型で、
  今回の対象外。

## 検証

### 手順 1: ビルド

```
make cfr cfr0 cfrorg brrnd brorg win comp cfrorgcnt
```

警告が 1 つも出ないこと。`make test` は通らない (`test.cpp` が
`bf_position.hpp` を include したままなのは既知)。

### 手順 2: 回帰ハーネス

```
./regress.sh check full
```

**12 項目すべて `OK` であること。** 1 つでも `DIFFER` が出たら変換のミス。
決定的カウンタも一致しなければならない。`hand_e` や `deck_or_hand_e` の
呼び出し回数が変わったら、ループの範囲を間違えている。所要 3 分 30 秒。

### 手順 3: 残った変換の数を数える

```
grep -c "i + 1\|j + 1" belief_state.hpp belief_state_win.hpp belief_state_lose.hpp
```

`able_actions` の呼び出し 5 箇所と、数箇所の例外だけが残るはず。
**49 箇所から 11 箇所前後に減っていること。** 減っていなければ変換漏れ、
増えていれば向きを間違えている。

### 手順 4: 添字で呼んでいる箇所が残っていないか

```
grep -n "hand_e(0)\|hand_e(7)\|deck_or_hand_e(2)" belief_state*.hpp
```

何も出ないこと (変更 4 で 1 / 8 / 3 になっているはず)。

### 手順 5: 難所 3 つを機械的に潰す

この変換で壊しやすい 3 点は、それぞれ grep で検出できる。**回帰ハーネスの前に
この 3 つを通すこと。** ハーネスは壊れたことは教えてくれるが、どこが原因かは
教えてくれない。

**(a) カード 8 (姫) を落としていないか**

```
grep -n "for(int card = 1; card < 8\|for(int card = 2; card < 8" belief_state*.hpp
```

何も出ないこと。カードのループの上限は必ず `card <= 8` または `card <= 7`
(変更 2 の `enemy_turn_win` L239 のみ)。`card < 8` と書いた時点で姫が落ちている。

**(b) 2 要素配列に `- 1` を足していないか**

```
grep -n "sol_flag_s\[card\|sol_flag_e\[card\|hand_s\[card\|hand\[card" belief_state*.hpp
```

何も出ないこと。これらは `[0]` `[1]` 固定で、`card` や `card - 1` を添字に
してはいけない。1 つでも出たら範囲外アクセスになっている。

**(c) `able_actions` の符号化を触っていないか**

```
git diff belief_state_win.hpp | grep "^[-+].*push_back"
git diff belief_state_win.hpp | grep "^[-+].*base = 40\|^[-+].*other_i ="
```

**どちらも何も出ないこと。** `able_actions` で変えてよいのは
`hand_e(...)` / `deck(...)` の引数に `+ 1` を足す 5 箇所だけで、
`push_back` の行も `base` と `other_i` の計算も 1 文字も変わらない。
ここに差分が出ていたら行動コードの形式を壊している。
