# 必敗判定の戻り値を型にし、終端判定を1本にまとめる

`is_lose` が返す多目的の `std::vector<std::pair<int, int>>` を、行動ごとの
真偽と手数を持つ `lose_decision` に置き換える。あわせて位置レベルの終端判定を
`check_terminal` 1本にまとめ、`is_lose` のコード `9` を無くす。

**この変更は出力を動かす。** 必敗による削減が強くなる方向。詳細は「検証」を参照。

## 触るファイル

| ファイル | 役割 | 変更 |
|---|---|---|
| `belief_state_win.hpp` | 型定義・`check_terminal`・`able_actions` | 型と終端判定、`able_actions` の分割 |
| `belief_state_lose.hpp` | 必敗判定 | `is_lose` の戻り値 |
| `infset_iswin.cpp` | rnd 第2フェーズ | 呼び出し側の追随 |
| `CLAUDE.md` | 規約 | 戻り値の規約を更新 |

**org 側 (`visit_winlose.hpp`) は触らない。** `is_lose` / `able_actions` /
`action_count` を呼ぶのは `infset_iswin.cpp` (rnd) だけで、`visit_winlose.hpp`
は `use_lose` と `wizard_lose` を直接呼んでいるため (`enter_play:83-84`、
`enter_wizard:171-172`)。この2本の戻り値 `lose_result` は変えないので org 側に
影響しない。grep で確認できる:

```
$ grep -rn "is_lose(\|able_actions(\|action_count(" --include=*.hpp --include=*.cpp .
infset_iswin.cpp:105  infset_iswin.cpp:106  infset_iswin.cpp:123  infset_iswin.cpp:179
(ほかは belief_state_win.hpp / belief_state_lose.hpp の宣言と定義のみ)
```

**行動履歴のエンコードには一切手を入れない。** `cnt_abs` の
`history.substr(0, history.length() - 1)` による末尾1文字の置換も今のまま
残す (`infset_iswin.cpp:145-149`)。

---

## 1. `terminal_kind` を4値にして `check_terminal` を1本にする

### 今の問題

`belief_state_win.hpp:45-47` の `terminal_kind` は3値で、`lost` は実際には
**「決着したが必勝ではない」**を意味する。`belief_state_win.hpp:120-121`:

```cpp
    if(max.value() < bs.hand_s[0].value()) return terminal_kind::won;
    else return terminal_kind::lost;
```

`max` は `hand_e_max()`。`max >= hand_s[0]` は「負けるかもしれない」であって
「必ず負ける」ではない。一方 `belief_state_lose.hpp:35-38` の必敗側は

```cpp
    MaybeCard min = bs.hand_e_min();
    if(min.value() > bs.hand_s[0].value()) return {{9, 0}};
    else return {};
```

と `hand_e_min()` を使い、`min > hand_s[0]` (必ず負ける) で判定している。
同じ終局条件を2箇所で別の述語で書いている。

### 置き換え後

`belief_state_win.hpp:45-49` を次にする。

```cpp
// 終端判定。カードを問わない位置レベルの判定だけを見る。
// 山札が尽きたとき、相手の手札が候補集合でしか分からないので、
// 勝ち・負け・どちらとも言えない、の3つに分かれる。
enum class terminal_kind {
  not_terminal, // 終局条件に該当しない。探索を続ける
  certain_win, // 必ず勝つ
  certain_lose, // 必ず負ける
  uncertain, // 終局だが相手の手札が絞れず勝敗が確定しない
};

terminal_kind check_terminal(const belief_state& bs);
```

`belief_state_win.hpp:109-126` の本体を次にする。

```cpp
terminal_kind check_terminal(const belief_state& bs) {
  CW_BUMP(check_terminal);
  // 大臣(7) を持っていて手札の合計が12以上ならルール上の負け。
  if(bs.have_s(Card{7}) && bs.hand_s[1].has_value() && bs.hand_s[0].value().value() + bs.hand_s[1].value().value() >= 12) {
    return terminal_kind::certain_lose;
  }
  // 山札が尽きたら手札の大きい方が勝ち。ここは手札1枚なので勝ちカードは自明。
  // 相手の手札は候補集合なので、最大が自分より小さければ必ず勝ち、最小が
  // 自分より大きければ必ず負け、またがっていればどちらとも言えない。
  // count_deck() は8要素ループなので、スカラの比較3つを先に評価する。
  // どれも副作用が無いので && の順序を入れ替えても意味は変わらない。
  if(!bs.hand_s[1].has_value() && !bs.is_wiz_choice && !bs.is_sol_choice && bs.count_deck() < 2) {
    if(bs.hand_e_max().value() < bs.hand_s[0].value()) return terminal_kind::certain_win;
    if(bs.hand_e_min().value() > bs.hand_s[0].value()) return terminal_kind::certain_lose;
    return terminal_kind::uncertain;
  }
  // 兵士(1) / 騎士(3) / 魔術師(5) を出した瞬間に勝つ判定は、どのカードを出すかが
  // 決まってからの問いなので use_win_uncached の先頭にある。
  return terminal_kind::not_terminal;
}
```

**`hand_e_max()` の後に `hand_e_min()` を呼ぶので、必勝側に8要素ループが1本
増える。** `certain_win` のときは `hand_e_min()` を呼ばずに返るので、増えるのは
`max >= hand_s[0]` の場合だけ。速度への影響は検証で測る。

### 呼び出し側3箇所の書き換え

`belief_state_win.hpp:152` (`is_win_uncached`):

```cpp
  if(check_terminal(bs) != terminal_kind::not_terminal) {
```

は**そのまま**でよい。決着していれば選べる行動が無いことに変わりはない。

`belief_state_win.hpp:388` (`enemy_turn_win_uncached`) と
`belief_state_win.hpp:558` (`draw_win_uncached`) の

```cpp
  if(t != terminal_kind::not_terminal) return {t == terminal_kind::won, 0};
```

を次にする。

```cpp
  if(t != terminal_kind::not_terminal) return {t == terminal_kind::certain_win, 0};
```

`certain_lose` と `uncertain` はどちらも「必勝ではない」なので `false` になり、
今の `lost` と同じ値になる。**必勝側の出力は動かない。**

念のため: **必勝側が真を返すのは `certain_win` のときだけ**である。
`t == terminal_kind::certain_lose` や `t != terminal_kind::certain_lose` と
書いてはいけない。今の3値での `t == terminal_kind::won` を、4値での
`certain_win` にそのまま読み替えるだけでよい。

---

## 2. `lose_decision` を作る

### 形

`belief_state_win.hpp` の `win_decision` (`:28-42`) の直後に足す。

```cpp
// どのスロットの行動で必敗が成立するか。
// win_decision と違い **スロットは2つしかない**。必敗判定が起きる意思決定点は
//   - カード使用ノード: スロット 0 = hand_s[0]、1 = hand_s[1]
//   - 魔術師の対象選択ノード: スロット 0 = 自分、1 = 相手
// の2種類だけで、行動が8通りある兵士の宣言ノードには必敗判定が無いため
// (is_lose は sol_choice で必ず空を返す。visit_winlose.hpp の enter_soldier が
//  無条件に lose_points[0] を増やしているのも同じ理由)。
struct lose_decision {
  decision_kind kind; // play_card か wizard_target。soldier_declaration は現れない
  lose_result slot[2];

  bool has_lose() const {
    return slot[0].is_lose || slot[1].is_lose;
  }
  int count() const {
    return (slot[0].is_lose ? 1 : 0) + (slot[1].is_lose ? 1 : 0);
  }
};
```

`lose_result` は `belief_state_win.hpp:15-18` の既存の型 (`{bool is_lose; int turns;}`)
をそのまま使う。既定構築で `{false, 0}` になるよう、空の `lose_decision` は
`lose_decision{decision_kind::play_card, {}}` と書く。

### `is_lose` の宣言

`belief_state_lose.hpp:11` を次にする。

```cpp
  lose_decision is_lose(const belief_state& bs); // メモ化しない
```

### `is_lose` の本体

`belief_state_lose.hpp:23-57` を丸ごと次に置き換える。

```cpp
// どのスロットの行動で必敗するかを返す。スロットの意味は lose_decision を参照。
// 「ルール上すでに敗北」は行動ではなく位置レベルの事実なので、この関数では
// 扱わない。呼び出し側が check_terminal() == certain_lose で見ること。
lose_decision belief_state_lose_checker::is_lose(const belief_state& bs) {
  CW_BUMP(is_lose);

  if(!bs.hand_s[1].has_value() && bs.is_sol_choice) {
    // 兵士の宣言ノードには必敗判定が無い。
    return lose_decision{decision_kind::play_card, {}};
  }
  if(!bs.hand_s[1].has_value() && bs.is_wiz_choice) {
    lose_decision d{decision_kind::wizard_target, {}};
    d.slot[0] = wizard_lose(bs, true); // 0 = 自分
    d.slot[1] = wizard_lose(bs, false); // 1 = 相手
    return d;
  }
  if(bs.is_my_turn && bs.hand_s[1].has_value()) {
    lose_decision d{decision_kind::play_card, {}};
    d.slot[0] = use_lose(bs, bs.hand_s[0].value());
    // 手札の2枚が同じカードなら行動は1つしかない。スロット1は立てない。
    // is_win_uncached が同じ場合に片方だけ評価するのと合わせる。
    if(bs.hand_s[0] != bs.hand_s[1]) d.slot[1] = use_lose(bs, bs.hand_s[1].value());
    return d;
  }
  return lose_decision{decision_kind::play_card, {}};
}
```

**大臣の判定と山札 `< 2` の判定 (`belief_state_lose.hpp:29-38`) はここから消える。**
`check_terminal` の `certain_lose` が同じことを答える (1 節)。条件が完全に一致
することを確認した:

| | 旧 `is_lose` | 新 `check_terminal` |
|---|---|---|
| 大臣 | `have_s(7) && hand_s[1].has_value() && 合計 >= 12` → `{{9,0}}` | 同じ条件 → `certain_lose` |
| 山札 `< 2` | `hand_e_min() > hand_s[0]` → `{{9,0}}`、そうでなければ `{}` | `hand_e_min() > hand_s[0]` → `certain_lose`、`hand_e_max() < hand_s[0]` → `certain_win`、それ以外 → `uncertain` |

**3つの結果が重ならないことの確認。** `hand_e_min() <= hand_e_max()` は常に
成り立つ (同じ候補集合の最小と最大で、候補が1枚なら等しい) ので、
`hand_e_max() < hand_s[0]` と `hand_e_min() > hand_s[0]` が同時に成り立つことは
無い。したがって上の3分岐は排他で、順序を入れ替えても結果は変わらない。
必勝側は `certain_win` を、必敗側は `certain_lose` を見るので、
**どちらの側も今とまったく同じ述語で判定することになる**。

**`hand_e_max()` / `hand_e_min()` が「無し」を返す場合。** 今のコードも
`max.value()` / `min.value()` を直接呼んでおり (候補集合が空なら値域検査で
落ちる)、その挙動は変えない。`certain_win` の分岐で先に返るときは
`hand_e_min()` を呼ばないので、ループが増えるのは `max >= hand_s[0]` の場合だけ。

**`use_lose` と `wizard_lose` の中身は1行も変えない。** 戻り値の `lose_result` も
そのまま。したがって `visit_winlose.hpp` (org) は影響を受けない。

---

## 3. `able_actions` を2本に分ける

今の `able_actions(bs, int card, bool)` (`belief_state_win.hpp:667-708`) は
`card` にカード 1〜8 と、魔術師の対象選択フラグ 0/1 の両方を受ける多目的の
`int` になっている。`lose_decision` がスロットを持つようになるので、
呼び出し側でどちらのノードか分かる。2本に分ける。

`belief_state_win.hpp:50` の宣言を次の2本にする。

```cpp
std::vector<int> able_actions_play(const belief_state& bs, Card card, bool is_second_player);
std::vector<int> able_actions_wizard(const belief_state& bs, bool to_self, bool is_second_player);
```

第2引数の型が `Card` と `bool` で違うので、呼び分けを取り違えるとコンパイルが
通らない (`Card` のコンストラクタは明示的で、`bool` からは作れない)。
`int` の多目的コードをやめる目的の半分はこれである。

`able_actions_wizard` は今の `:671-687` の魔術師分岐をそのまま移したもの。
`card == 0` が `to_self == true`、`card == 1` が `to_self == false` に対応する
(`is_lose` のスロット 0 = 自分、1 = 相手と同じ)。

```cpp
// 行動コードは 0-origin のカード添字をそのまま桁に埋め込むシリアライズ形式
// なので、この関数の中だけは添字 (0..7) で通す。
std::vector<int> able_actions_wizard(const belief_state& bs, bool to_self, bool is_second_player) {
  std::vector<int> actions;
  if(bs.hand_s[1].has_value() || !bs.is_wiz_choice) return actions;

  // --- 自分を対象とする場合 ---
  if(to_self != is_second_player) {
    const int other_raw = to_self ? bs.hand_s[0].raw() : 0;
    for(int j = 0; j < 8; j++)
      if(bs.deck(Card{j + 1})) actions.push_back(44000 + is_second_player * 100 + (other_raw - 1) * 10 + j);
  }
  // --- 相手を対象とする場合 ---
  else {
    if(!bs.barrier_e) {
      for(int i = 0; i < 7; i++) {
        if(bs.hand_e(Card{i + 1})) { // 相手が捨てさせられるカード
          actions.push_back(44000 + !is_second_player * 100 + i * 10 + 0);
        }
      }
    } else actions.push_back(44000 + !is_second_player * 100);
  }
  return actions;
}
```

**分岐条件の同値性**: 今は
`(card == 0 && !is_second_player) || (card == 1 && is_second_player)` が自分側。
`card == 0` が `to_self == true` なので、これは
`(to_self && !is_second_player) || (!to_self && is_second_player)`、つまり
`to_self != is_second_player`。相手側はその否定で、今の
`(card == 0 && is_second_player) || (card == 1 && !is_second_player)` と一致する。

真理値表で確かめる。**ここを反転させると出力が壊れるので、実装後に必ず見直すこと。**

| `card` (旧) | `to_self` (新) | `is_second_player` | 旧の分岐 | `to_self != is_second_player` | 新の分岐 |
|---|---|---|---|---|---|
| 0 | true | false | 自分 | true | 自分 |
| 0 | true | true | 相手 | false | 相手 |
| 1 | false | false | 相手 | false | 相手 |
| 1 | false | true | 自分 | true | 自分 |

4行とも旧と新が一致する。
**今の `card` が 0/1 以外 (例: 旧コードの `9`) のときは両方の分岐に該当せず空を
返していたが、新しい形では必ずどちらかに入る。** これは `9` を渡す経路自体が
無くなるので問題にならない (2 節)。

`other_raw` は今のコードと同じく、`card == 0` (自分対象) なら `hand_s[0].raw()`、
そうでなければ `0`。

`able_actions_play` は今の `:691-707` をそのまま移し、`card` を `Card` にする。

```cpp
std::vector<int> able_actions_play(const belief_state& bs, Card card, bool is_second_player) {
  (void)is_second_player; // カード使用側は先手後手を見ない (今と同じ)
  const int base = 40 + card.index();
  std::vector<int> actions;

  if(card == Card{4} || card == Card{7} || bs.barrier_e || card == Card{1}) {
    actions.push_back(base);
  } else if(card == Card{2}) {
    // 相手の判明するカード
    for(int i = 0; i < 8; i++)
      if(bs.hand_e(Card{i + 1})) actions.push_back(base * 10 + i);
  } else if(card == Card{3}) {
    // 相手の判明するカード
    int other_i = bs.other_hand_s(card).value().index();
    if(bs.hand_e(Card{other_i + 1})) actions.push_back(base * 100 + other_i * 11);
  } else if(card == Card{6}) {
    // 相手と交換するカード
    for(int i = 0; i < 8; i++)
      if(bs.hand_e(Card{i + 1})) actions.push_back(base * 100 + (bs.other_hand_s(card).value().index()) * 10 + i);
  }
  return actions;
}
```

`base` は今の `40 + card - 1` と同じ (`card.index()` が `card - 1`)。
**カード 5 と 8 はどの分岐にも該当せず空を返す。今と同じ。**

`action_count` (`belief_state_win.hpp:711-723`) は**変えない**。

---

## 4. `infset_iswin.cpp` の必敗側

`infset_iswin.cpp:104-157` を次に置き換える。

置き換え前 (`:104-112`):

```cpp
  auto lose_actions = lc.is_lose(bs);
  int act_cnt = action_count(bs);
  int able_act = act_cnt - lose_actions.size();
  lose_move[0] += able_act;
  if(able_act == 1 && act_cnt > 1) only_history.insert(history);

  bool pushed[2] = {false, false}; // 同じスロットを 2 回積まないための印

  for(const auto& lose_action : lose_actions) {
    lose_move[lose_action.second]++;

    if(lose_action.first == 9) {
```

置き換え後:

```cpp
  const int act_cnt = action_count(bs);
  if(check_terminal(bs) == terminal_kind::certain_lose) {
    // ルール上すでに敗北。どの行動も選ぶ意味が無いので able_act は 0。
    lose_move[0] += 0;
    if(act_cnt > 1) only_history.insert(history);
    output_actions_history(history, true);
  } else {
    const lose_decision d = lc.is_lose(bs);
    const int able_act = act_cnt - d.count();
    lose_move[0] += able_act;
    if(able_act <= 1 && act_cnt > 1) only_history.insert(history);

    for(int slot = 0; slot < 2; slot++) {
      if(!d.slot[slot].is_lose) continue;
      lose_move[d.slot[slot].turns]++;

      string action = rph.get_action((unsigned char)history[0]);
      int firstp = char_to_action(action[0]) / 10;
      const bool is_second_player = (firstp == 2);

      std::vector<int> actions;
      if(d.kind == decision_kind::wizard_target) {
        actions = able_actions_wizard(bs, slot == 0, is_second_player);
      } else {
        actions = able_actions_play(bs, bs.hand_s[slot].value(), is_second_player);
      }

      // able_actions が空なら abs_history に何も入らないので、行も書かない
      if(!actions.empty()) {
        const bool is_choice_node = (d.kind == decision_kind::wizard_target);
        lose_rows.push_back({history, false, is_choice_node, (unsigned char)slot});
      }

      for(int act : actions) {
        string new_hist;
        if(bs.is_wiz_choice) {
          new_hist = history.substr(0, history.length() - 1) + string(1, action2char(act, true));
        } else {
          new_hist = history + string(1, action2char(act, true));
        }
        abs_history.insert({new_hist, false});
      }
    }
  }
```

### `pushed[2]` が要らなくなる理由

今の `pushed[2]` は、`is_lose` が同じ履歴に2件返したとき (手札の2枚が同じ
カードで両方とも必敗) に同じスロットを2回積まないための印だった。今のコードは
`slot` を `hand_s[0].value() == lose_action.first ? 0 : 1` で決めるので、
2枚が同じカードなら両方ともスロット0になり、重複していた。

新しい形ではスロットが添字そのもので、`is_lose` は2枚が同じカードなら
スロット1を立てない (2 節)。**同じスロットが2回来ることが構造的に無いので
`pushed` は不要**になる。

### `able_actions_wizard` の `slot == 0` が `to_self` になる理由

`is_lose` のスロット 0 は「自分を対象」(`wizard_lose(bs, true)`) なので、
`slot == 0` がそのまま `to_self == true` になる。

### `lose_move` の合計について

`CLAUDE.md` の規約では `win_points` / `lose_points` の合計が
`decision_points[0]` と一致するとあるが、これは org 側 (`visit_winlose.hpp`) の
話。`infset_iswin.cpp` の `lose_move` は `lose_move[0]` に「必敗でない行動の
本数」を足しこむ別の量で、合計は一致しない (今も一致していない)。**この変更で
規約を壊すことはない。**

---

## 5. `CLAUDE.md` の更新

`belief_state` の規約から次を直す。

- `able_actions` の `card` 引数が多目的の `int` だという記述 (`0 / 9` を取る)
  を消し、`able_actions_play` (`Card`) と `able_actions_wizard` (`bool to_self`)
  の2本になったことを書く。
- 返り値の規約に `lose_decision` を足し、`terminal_kind` を4値に直す。
- 「`is_lose` の `9` は『ルール上すでに敗北』」を消し、位置レベルの判定は
  `check_terminal` の `certain_lose` が答えると書く。

---

## 検証

### ビルド

```
make cfr cfr0 cfrorg cfrorgcnt brrnd brorg win
make cppcheck
git add -u && git clang-format
```

新しい警告を出さないこと。cppcheck 無指摘であること。整形差分が無いこと。

### 不変条件

```
for a in "4 4 6 7" "1 1 1 7" "5 5 7 7" "2 2 8 7" "1 2 3 7" "6 7 8 7"; do
  ./cfrorg $a 2>&1 >/dev/null | head -2
done
```

`exit_with_print` の検査 (`use_win` / `is_win` / `draw_win` / `soldier_win` /
`wizard_win` / `wizard_lose`) が1つも落ちないこと。出力が空であること。

### 回帰

```
./regress.sh check
```

**期待する結果**:

**`OK` でなければならない項目 (8本)**:

- `cfrorg-*-stdout` 4本と `cfrorg-*-counters` 4本。org 側は `use_lose` /
  `wizard_lose` しか使わず、その2本を変えないため。`check_terminal` を4値に
  したぶんも、必勝側は `certain_win` かどうかしか見ないので値が動かない
  (1 節)。カウンタ名も変えないので counters のテキストも動かない。
- **`win-557-wininf` と `win-446-wininf`。** `win_rows` は必勝側だけで作られ、
  `lose_rows` とも `abs_history` とも独立しているため。

**`DIFFER` してよい項目 (4本)**:

- `win-557-stdout` `win-446-stdout` `win-557-loseinf` `win-446-loseinf`。

**557 も動きうる。** 出力が動く3つの原因 (次節) は、どれも魔術師とは無関係
だからである。

| 原因 | 魔術師が要るか | 557 でも起きるか |
|---|---|---|
| すでに敗北のとき `able_act` が 0 | 要らない (大臣・山札 `< 2`) | **起きる** |
| `only_history` が `able_act <= 1` | 要らない | **起きる** |
| 同じカード2枚で両方必敗 | 要らない | **起きる** |

前の変更 (A) で 557 が動かなかったのは、A の中身が魔術師の対象選択ノード
だけだったからで、今回は当てはまらない。**「557 だから動かないはず」という
理由づけを今回は使わない。**

`OK` でなければならない8本のどれかが `DIFFER` したら、**原因を突き止めるまで
先に進まない**。特に `cfrorg-*-counters` が動いたら、必勝側に手が入っている
証拠なので 1 節の書き換えを見直すこと。

### 出力が動く3つの原因を分けて確認する

`DIFFER` した項目について、次の3つ以外の変化が無いことを確かめる。

1. **すでに敗北のとき `able_act` が 0 になる。**
   今は `act_cnt - 1`。`lose_move[0]` が減る方向。
2. **`only_history` の条件が `able_act == 1` から `able_act <= 1` になる。**
   全部の行動が必敗の情報集合が新たに削減対象になる。
   `infset size by win/lose` の `[2]` と `[3]` が減る方向。
3. **手札の2枚が同じカードで両方必敗のとき、今は `able_act` が `-1` になる。**
   `act_cnt` は1なのに `lose_actions.size()` が2だったため。新しい形では
   スロット1を立てないので `count()` が1になり `able_act` は0。
   `lose_move[0]` が増える方向。

3 が実際に起きているかは、実装前に旧コードへ一時的な計数を入れて測る:

```cpp
// belief_state_lose.hpp の is_lose、2枚分岐の末尾に置く (計測後に消す)
if(bs.hand_s[0] == bs.hand_s[1] && res.size() == 2) probe.same_card_both++;
```

`./win 4 4 6` で件数を出し、`lose_move[0]` の差と突き合わせる。

### 速度

`hand_e_min()` が必勝側に増えるので測る。

```
/usr/bin/time -f "%e s %M KB" ./cfrorg 4 4 6 6
/usr/bin/time -f "%e s %M KB" ./win 4 4 6
```

直前のコミットでの実測値は `cfrorg 0.76 s / 5268 KB`、`win 80.89 s / 1080260 KB`。
`cfrorgcnt` の `hand_e_min` と `hand_e` のカウンタ増加も記録する。
**`win` が 10% 以上遅くなったら報告して相談する。**

### 基準の取り直し

上がすべて通ったら `./regress.sh save`。**通る前に取り直さない。**

---

## やらないこと

- `use_lose` / `wizard_lose` の中身と戻り値 (`lose_result`)
- `visit_winlose.hpp` (org 側)
- `is_lose` のメモ化 (今もしていない)
- `win_decision` を2スロットにすること。必勝側は兵士の宣言ノードがあり、
  行動が8通り必要なので8ビットのまま
- `action_count` の中身
- `infset_iswin.cpp:179` の `action_cnt += action_count(bs)`
- ADR 0007 の更新 (形が固まってから別途)
