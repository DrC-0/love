# 必勝判定の戻り値を型にし、入口を再帰に組み込む (B1)

## 目的

`std::pair<bool,int>` と `std::pair<int,int>` が混在し、**暗黙変換で化ける**。
`is_terminated_win` が返す `{3,1}` が `pair<bool,int>` に変換されて `{true,1}` に
なるため、終局局面では `use_win(bs, 姫)` まで真を返す。`std::pair` を使っている
限りコンパイラは止めない。`Card` / `MaybeCard` を入れた ADR 0008 と同じ話。

判定ごとに別の struct を与えて、取り違えをコンパイル時に止める。あわせて
**入口 `is_win` を相互再帰に組み込み**、位置レベルと行動レベルの混線が
構造として起きないようにする。

**この変更は表現だけを変える。判定の意味は 1 つも変えない。**

## この変更で直さないもの (別の変更にする)

判定の意味に触るものは、ハーネスの「出力不変」という保護が効かなくなるので
分ける。**この計画では手を付けない。**

- `is_lose` の戻り値の形だけは `vector<std::pair<int,int>>` のまま。多目的コード
  `9` / `8` / `0`・`1` / カード の整理は次の変更 (B2)
- 姫(8) の必敗判定が `is_lose` の入口で早期 return しており、もう片方の
  カードが評価されない不具合
- `wizard_lose_uncached` の深さ 2 件 (`{true,0}` と `+1` 漏れ)
- `belief_state_lose.hpp` の `15` / `25` に言及する古いコメント

## 触るファイル

| ファイル | モデル | 変更 |
|---|---|---|
| `belief_state_win.hpp` | 共通 | 型の導入、改名、`is_win` の再帰組み込み、終端短絡の除去 |
| `belief_state_lose.hpp` | 共通 | 改名、`lose_result` への置換、win 側の型への追随 |
| `visit_winlose.hpp` | org (`cfrorg`) | 呼び出しの追随 |
| `infset_iswin.cpp` | rnd (`win`) | 呼び出しの追随 |
| `count_work.hpp` | 計測 | カウンタ名の追随 |

---

## 1. 新しい型 (`belief_state_win.hpp` の先頭)

```cpp
// 必勝が成立するか、成立までにプレイヤーの行動が何回続くか。
// turns は holds が真のときだけ意味を持つ。偽のときは 0 を入れる。
struct win_result {
  bool holds;
  int turns;
};

// 必敗側。win_result と同じ形だが別の型にして、取り違えをコンパイル時に止める。
// use_lose は win.draw_win の結果を受けるので、混ざる余地が実際にある。
struct lose_result {
  bool holds;
  int turns;
};

// 意思決定点の種類。行動番号 0〜7 の読み方がこれで決まる。
enum class decision_kind {
  play_card, // 行動番号 = カード − 1 (0〜7 が カード 1〜8)
  wizard_target, // 0 = 自分, 1 = 相手
  soldier_declaration, // 0 = 宣言なし, 1〜7 = カード 2〜8
};

// 魔術師の対象。bool の 0/1 では意味が読めず、
// visit_winlose.hpp と is_lose で 0/1 の意味が逆になっていた。
enum class wizard_target { self, opponent };

// どの行動で必勝が成立するか。bit i が行動番号 i に対応する。
struct win_decision {
  decision_kind kind;
  unsigned char holds; // 行動 i で必勝なら bit i が立つ
  unsigned char turns[8]; // bit i が立っているときだけ意味を持つ

  bool any() const {
    return holds != 0;
  }
  bool at(int action) const {
    return (holds >> action) & 1;
  }
};

// 探索を打ち切ってよいか。打ち切るなら、その時点の判定結果。
// decided が偽なら探索を続ける (今の {-1, 0})。
// decided が真で result.any() が偽なら「決着したが勝ちではない」(今の {0, 0})。
struct terminal_check {
  bool decided;
  win_decision result;
};
```

`win_result` と `win_decision` は別の型なので、片方を返す場所でもう片方を
`return` するとコンパイルが止まる。`std::pair` では止まらなかった。

**`turns[8]` の初期化を忘れないこと。** `win_decision` を作るときは必ず
`{kind, 0, {}}` のように全体を初期化する。`holds` のビットが立っていない
要素の値は読まれないが、未初期化のまま比較や複製をしない。

## 2. 改名 (略語をやめる)

| 今 | 新 |
|---|---|
| `is_terminated_win` | `check_terminal` |
| `sol_win` / `sol_win_impl` | `soldier_win` / `soldier_win_uncached` |
| `wiz_win` / `wiz_win_impl` | `wizard_win` / `wizard_win_uncached` |
| `wiz_lose` / `wiz_lose_impl` | `wizard_lose` / `wizard_lose_uncached` |
| `use_win_impl` | `use_win_uncached` |
| `enemy_turn_win_impl` | `enemy_turn_win_uncached` |
| `draw_win_impl` | `draw_win_uncached` |
| `use_lose_impl` | `use_lose_uncached` |
| 引数 `bool to0p` | `wizard_target target` |
| メンバ `m_sol_win` / `m_wiz_win` / `m_wiz_lose` | `m_soldier_win` / `m_wizard_win` / `m_wizard_lose` |

`_impl` は「実装」以上の情報が無い。実際には「メモ表を引かない版」なので
`_uncached` と名乗らせる。**引数名 `bs` はそのまま残す** (この 2 ファイルで
1 行おきに出てくる支配的な引数で、綴ると読みにくくなる)。

`check_terminal` の名前の理由: `is_` で始まるのに真偽を返さず、`win` と
名乗るのに「決着したが勝ちではない」も返していた。役割は if-then による
DFS の終端判定なので、そう名乗らせる。

### `wizard_target` への置き換えで直る取り違え

- `is_win` の魔術師分岐 (`:113-114`): `wiz_win(bs, true)` = 自分 →
  `wizard_win(bs, wizard_target::self)`
- `visit_winlose.hpp:155-156`: `wc.wiz_win(bs, 0)` は `to0p` 偽 = **相手**、
  `wc.wiz_win(bs, 1)` は **自分**。`res_0win` が相手、`res_1win` が自分という
  順序なので、**`res_0win` に `opponent`、`res_1win` に `self` を渡す**。
  `is_lose` の 0/1 とは逆。**この変更では順序を入れ替えない。**
  `enter_wizard` の集約 (`visit_winlose.hpp:159-178`) は `res_0*` と `res_1*` に
  ついて対称 (真偽は OR、深さは min / max) なので、入れ替えても出力は動かない。
  それでも入れ替えないのは、0/1 の規約は `is_lose` の多目的コードを整理する
  B2 でまとめて決めるべきだからで、出力が動くからではない。
  逆であるという事実はコメントに書き残す。
- `visit_winlose.hpp:157-158` の `lc.wiz_lose(bs, 0)` / `(bs, 1)` も同様に
  `opponent` / `self` の順。

## 3. `check_terminal`

`belief_state_win.hpp:62-88` の `is_terminated_win` を `check_terminal` に改名し、
戻り値を `terminal_check` にする。**判定条件は 1 つも変えない。**

| 今の return | 新しい return |
|---|---|
| `{0, 0}` (`:65`, `:73`) | `{true, {decision_kind::play_card, 0, {}}}` |
| `{bs.hand_s[0].value().value(), 0}` (`:72`) | `decided_by(bs.hand_s[0].value(), 0)` |
| `{1, 1}` (`:78`) | `decided_by(Card{1}, 1)` |
| `{3, 1}` (`:81`) | `decided_by(Card{3}, 1)` |
| `{5, 1}` (`:84`) | `decided_by(Card{5}, 1)` |
| `{-1, 0}` (`:87`) | `{false, {decision_kind::play_card, 0, {}}}` |

`decided_by` は同じファイル内の小さなヘルパ:

```cpp
// 終端で、そのカードを出せば勝ちが確定する場合の結果を作る。
inline terminal_check decided_by(Card card, int turns) {
  win_decision d{decision_kind::play_card, 0, {}};
  d.holds = (unsigned char)(1u << card.index());
  d.turns[card.index()] = (unsigned char)turns;
  return {true, d};
}
```

## 4. `is_win` を再帰に組み込み、メモ化する

### 4.1 宣言

`belief_state_win.hpp:17` の

```cpp
  std::pair<int, int> is_win(const belief_state& bs); // メモ化しない (入口、再帰しない)
```

を

```cpp
  // 意思決定点で、自分のどの行動が必勝かを返す。draw_win から再帰的に呼ばれる
  // ため、他の 5 本と同じくメモ化する。
  win_decision is_win(const belief_state& bs);
```

にし、private に `win_decision is_win_uncached(const belief_state& bs);` を、
メンバに `std::unordered_map<unsigned long long, win_decision> m_is_win;` を足す。

メモの鍵は他と同じ `key(bs, extra)` を使う。**`extra` には 0 を渡す**
(`is_win` は追加の引数を取らないため)。他の判定とはメモ表が別なので、
`extra = 0` が `use_win` の `card = 0` と衝突することはない。

### 4.2 本体

`belief_state_win.hpp:90-156` の `is_win` を `is_win_uncached` に改名し、
`is_win` はメモ表を引くだけの薄い関数にする (他の 5 本と同じ形)。

`is_win_uncached` の各分岐の戻り値を `win_decision` に書き換える。**分岐条件は
1 つも変えない。**

```cpp
win_decision belief_state_win_checker::is_win_uncached(const belief_state& bs) {
  auto t = check_terminal(bs);
  if(t.decided) return t.result;

  if(!bs.hand_s[1].has_value() && bs.is_sol_choice) {
    win_decision d{decision_kind::soldier_declaration, 0, {}};
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bs, "soldier");
    for(int c = 2; c <= 8; c++) {
      Card card{c};
      if(bs.hand_e(card)) {
        auto res = soldier_win(bs, card);
        // 行動番号: 0 = 宣言なし、1〜7 = カード 2〜8
        if(res.holds) {
          const int action = c - 1;
          d.holds |= (unsigned char)(1u << action);
          d.turns[action] = (unsigned char)(res.turns + 1);
        }
      }
    }
    return d;

  } else if(!bs.hand_s[1].has_value() && bs.is_wiz_choice) {
    win_decision d{decision_kind::wizard_target, 0, {}};
    auto res_self = wizard_win(bs, wizard_target::self);
    auto res_opponent = wizard_win(bs, wizard_target::opponent);
    if(res_self.holds) {
      d.holds |= 1u << 0;
      d.turns[0] = (unsigned char)(res_self.turns + 1);
    }
    if(res_opponent.holds) {
      d.holds |= 1u << 1;
      d.turns[1] = (unsigned char)(res_opponent.turns + 1);
    }
    return d;

  } else if(bs.is_my_turn && bs.hand_s[1].has_value()) {
    win_decision d{decision_kind::play_card, 0, {}};
    // 手札の2枚が同じカードなら片方だけ評価する (無駄を省く)。
    // 行動番号はカード − 1 なので、同じカードなら同じビットになる。
    const Card c0 = bs.hand_s[0].value();
    auto res0 = use_win(bs, c0);
    if(res0.holds) {
      d.holds |= (unsigned char)(1u << c0.index());
      d.turns[c0.index()] = (unsigned char)res0.turns;
    }
    if(bs.hand_s[0] != bs.hand_s[1]) {
      const Card c1 = bs.hand_s[1].value();
      auto res1 = use_win(bs, c1);
      if(res1.holds) {
        d.holds |= (unsigned char)(1u << c1.index());
        d.turns[c1.index()] = (unsigned char)res1.turns;
      }
    }
    return d;
  }
  // 相手ターンの場合
  return win_decision{decision_kind::play_card, 0, {}};
}
```

**今との同値性**: 今の `is_win` は「勝てるカードのうち手数の短い方」を 1 つ
返していた (`:130-148`)。新しい形は両方をビットで持つので、**情報が増える方向**で
あり失われない。呼び出し側が「どれか勝てるか」を知りたいときは `any()`、
「最短の手数」が要るときは立っているビットの `turns` の最小値を取る。

### 4.3 `soldier_win` の `+1` の移動に注意

今の `is_win` の兵士分岐は `{has_true, min_t + 1}` と**外で `+1`** している
(`:110`)。上のコードでは `d.turns[action] = res.turns + 1` として**行動ごとに
`+1`** する。魔術師分岐 (`:122`) も同じ。値は変わらない。

手札 2 枚の分岐 (`:130-148`) は**今も `+1` していない** (`use_win` が内部で
足している) ので、上のコードでも足さない。**ここを間違えると深さが 1 ずれる。**

## 5. `use_win_uncached` から終端短絡を外す

`belief_state_win.hpp:171-172` の

```cpp
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;
```

の **2 行を削除する**。`CW_BUMP(use_win);` は残す。以降の本体は分岐を変えず、
戻り値だけ `win_result` にする。

これで `use_win` は「そのカードを出したら勝つか」だけに答える。

### 短絡を外すと終端局面で何が変わるか

これがこの変更の核心なので具体的に書く。`check_terminal` が「兵士(1)を出せば
勝ち」と答える局面で、手札が {兵士(1), 姫(8)} だったとする。

| 呼び出し | 今 | 変更後 |
|---|---|---|
| `use_win(bs, Card{1})` | `{true, 1}` (短絡がそのまま返す) | 兵士の分岐を実際に評価した結果 |
| `use_win(bs, Card{8})` | **`{true, 1}`** (短絡が card を見ずに返す) | **`{false, 0}`** |

`use_win_uncached` には `if(card == Card{8}) return {false, 0};`
(`belief_state_win.hpp:177`) が元からある。**短絡の 2 行がその手前にあったせいで
そこまで到達していなかった。** 姫を捨てれば即負けなので `{false, 0}` が正しい。

同じことがカード 2〜7 でも起きる。終端局面で `use_win` は今どのカードでも真を
返しており、変更後はカードごとに正しく評価される。

**この変更が `cfrorg` / `win` の出力を動かさないのは、`use_win` を直接呼ぶ
呼び出し側 (`visit_winlose.hpp`) を `is_win` 経由に変えるからである**
(7.1 節)。`is_win` は終端を `check_terminal` の結果で答えるので、今と同じ
「名指しされたカードだけが真」という形になる。7.1 節の同値性の説明を参照。

## 6. `draw_win_uncached` の OR ノードを `is_win` に置き換える

`belief_state_win.hpp:483-506` の OR ノード全体を置き換える。

置き換え前:

```cpp
      // --- 自分の手札の選択 (ORノード) ---
      auto res0 = use_win(next_bs, next_bs.hand_s[0].value());

      bool or_first = false;
      int or_second = 0;

      // 手札の2枚が違うカードなら、もう一方も評価する
      if(next_bs.hand_s[0] != card) {
        auto res1 = use_win(next_bs, next_bs.hand_s[1].value());

        if(res0.first) {
          or_first = true;
          or_second = res0.second;
        }
        if(res1.first) {
          if(!or_first) or_second = res1.second;
          else or_second = std::min(or_second, res1.second);
          or_first = true;
        }
      } else {
        // 同じカードなら片方の結果をそのまま使う
        or_first = res0.first;
        or_second = res0.second;
      }
```

置き換え後:

```cpp
      // --- 自分の手札の選択 (ORノード) ---
      // 「この局面で自分のどの行動が勝つか」はそのまま is_win の問い。
      // 終端判定も is_win の中で捌かれるので、ここで先に見る必要はない。
      const win_decision d = is_win(next_bs);
      const bool or_first = d.any();
      int or_second = 0;
      if(or_first) {
        or_second = 255;
        for(int action = 0; action < 8; action++) {
          if(d.at(action)) or_second = std::min(or_second, (int)d.turns[action]);
        }
      }
```

この下の「`--- 山札ドローの集約 (ANDノード) ---`」以降 (`:508-513`) は **1 行も
変えない**。ループ先頭の `if(!all_true) break;` (`:476`) も変えない。

**同値性**:

- 終端でない場合。今は `use_win` を 2 枚それぞれに呼び、真のものの深さの
  **最小**を取る。`is_win` の手札 2 枚の分岐はまったく同じ `use_win` を呼んで
  ビットに詰めるので、`or_first` は OR、`or_second` は最小で一致する。
  手札が同じカードなら `is_win` も片方だけ評価する。
- 終端の場合。今は `use_win` の短絡で両方が `t` を返し、`or_first = (t.first != 0)`、
  `or_second = t.second`。新しい形では `is_win` が `check_terminal` の結果を
  そのまま返すので、`or_first = d.any()`、`or_second` は立っている唯一のビットの
  `turns` = `t` の深さ。一致する。
- `next_bs` はドロー直後なので手札は必ず 2 枚あり、`is_win` の選択ノード分岐には
  入らない。

## 6.5 `belief_state_lose.hpp` の型置換

`use_lose` / `wizard_lose` とその `_uncached` 版の戻り値を
`std::pair<bool, int>` から `lose_result` にする。メンバのメモ表
`m_use_lose` / `m_wizard_lose` の値型も `lose_result` にする。

**`{a, b}` の形の return はそのまま通る** (集成体初期化)。`.first` / `.second` を
読んでいる箇所を `.holds` / `.turns` に置き換える。

`win.draw_win(...)` の結果は `win_result` になるので、そちらも `.first` /
`.second` を `.holds` / `.turns` にする。**`win_result` と `lose_result` は
別の型なので、取り違えるとコンパイルが止まる。** それがこの置換の目的である。

`is_lose` の戻り値 `std::vector<std::pair<int, int>>` は**変えない** (B2)。
`is_lose` の中で `use_lose` / `wizard_lose` の結果を読んでいる箇所
(`belief_state_lose.hpp:44-55`) は `.first` / `.second` を置換する。

**判定条件と `+1` の位置は 1 つも変えない。** `wizard_lose_uncached` の
`{true, 0}` (`:211`) と `+1` 漏れ (`:228`) は不具合と分かっているが、
**この計画では直さない** (変更 A)。

## 7. 呼び出し側

### 7.1 `visit_winlose.hpp`

`use_win` から短絡が消えるので、**ここで `use_win` を直に呼んではいけない**
(終端局面の答えが変わる)。`is_win` に切り替える。

`visit_winlose.hpp:78-88` の置き換え前:

```cpp
  void enter_play(node &n, int c1, int c2) {
    std::string key = n.org_his_p[n.turn].get_hash_value();
    belief_state bs(n.open, key, false);
    auto res_0win = wc.use_win(bs, Card{c1});
    auto res_1win = wc.use_win(bs, Card{c2});
    auto res_0lose = lc.use_lose(bs, Card{c1});
    auto res_1lose = lc.use_lose(bs, Card{c2});
    bool rm_bywin = res_0win.first || res_1win.first || cutting_w > 0;
    bool rm_bylose = res_0lose.first || res_1lose.first || cutting_l > 0;
    assert(0 <= res_0win.second && res_0win.second < 11);
    assert(0 <= res_1win.second && res_1win.second < 11);
```

置き換え後:

```cpp
  void enter_play(node &n, int c1, int c2) {
    std::string key = n.org_his_p[n.turn].get_hash_value();
    belief_state bs(n.open, key, false);
    // 行動ごとの必勝は is_win がまとめて返す。use_win を直に呼ぶと終端局面の
    // 扱いを自前で書くことになるので呼ばない。
    const win_decision dwin = wc.is_win(bs);
    const win_result res_0win{dwin.at(c1 - 1), dwin.turns[c1 - 1]};
    const win_result res_1win{dwin.at(c2 - 1), dwin.turns[c2 - 1]};
    auto res_0lose = lc.use_lose(bs, Card{c1});
    auto res_1lose = lc.use_lose(bs, Card{c2});
    bool rm_bywin = res_0win.holds || res_1win.holds || cutting_w > 0;
    bool rm_bylose = res_0lose.holds || res_1lose.holds || cutting_l > 0;
    assert(0 <= res_0win.turns && res_0win.turns < 11);
    assert(0 <= res_1win.turns && res_1win.turns < 11);
```

`c1` / `c2` はカード (1〜8) なので添字は `− 1`。ビットが立っていない行動の
`turns` は 0 (`win_decision` を `{kind, 0, {}}` で作るため) なので、
`win_result{false, 0}` になる。今の `use_win` が偽のときに `{false, 0}` を
返していたのと同じ。

**`:90-109` の `decision_points` / `win_points` / `lose_points` / `pf[]` の計算は
式を 1 つも変えない。** `.first` → `.holds`、`.second` → `.turns` の置換だけ。

#### 同値性

終端局面で今は `use_win` の短絡により `res_0win` も `res_1win` も
`{true, t.second}` になる。新しい形では `is_win` が `check_terminal` の結果を
返すので、名指しされたカードのビットだけが立ち、片方だけ真になる。

`win_points` は `:94-98` で「両方真なら深さの小さい方、片方だけ真ならその値」を
数える。今は両方が同じ深さ `t.second` なので `win_points[t.second]++`。
新しい形では片方だけ真で深さは同じ `t.second` なので、やはり
`win_points[t.second]++`。`rm_bywin` は OR なので不変。`pf[n.depth].w_inc` も
OR なので不変。

終端でない局面では、`is_win` の手札 2 枚の分岐が今と同じ `use_win` を同じ
2 枚に対して呼ぶだけなので、値は一致する。

#### 残り 2 箇所

- `:129` の `wc.sol_win(soldier_bs[n.depth], Card{i})` → `wc.soldier_win(...)`。
  戻り値は `win_result` になるので `.first` → `.holds`、`.second` → `.turns`。
  **ここは `use_win` ではなく `soldier_win` なので、`is_win` に切り替えない**
  (兵士の宣言という行動が引数で決まっている問い)。
- `:155-158` の `wc.wiz_win(bs, 0)` / `(bs, 1)` / `lc.wiz_lose(bs, 0)` / `(bs, 1)`
  を `wizard_target::opponent` / `wizard_target::self` の順に置き換える
  (2 節の注記のとおり **順序は変えない**)。`.first` / `.second` も置換する。

### 7.2 `infset_iswin.cpp`

- `:64` の `is_terminated_win(bs)` → `check_terminal(bs)`。`term.first != -1` は
  `t.decided`、`term.first` によるスロット決定は `t.result` のビットから取る
- `:78-79` の `wc.wiz_win(bs, true)` / `(bs, false)` →
  `wc.wizard_win(bs, wizard_target::self)` / `(bs, wizard_target::opponent)`。
  **スロット 0 が自分、1 が相手という今の対応は変えない**
- `:83-88` の `wc.use_win(...).first` → `.holds`

`win_rows` / `lose_rows` に積む内容は **1 行も変えない**。

### 7.3 `count_work.hpp`

カウンタ名 `is_terminated_win` / `sol_win` / `wiz_win` / `wiz_lose` を
`check_terminal` / `soldier_win` / `wizard_win` / `wizard_lose` に改名する。
`CW_BUMP` の引数も合わせる。

**これは `cfrorg-*-counters` の出力テキストを変える。** 数値は変わらない
(呼び出し回数は変わらないため)。検証の節を参照。

---

## 検証

### ビルド

```
make cfr cfr0 cfrorg cfrorgcnt brrnd brorg win
```

新しい警告を出さないこと。`make cppcheck` も無指摘であること。

### 回帰

```
./regress.sh check
```

**期待する結果**: `cfrorg-*-stdout` 4 項目・`win-*-stdout` 2 項目・
`win-*-wininf` 2 項目・`win-*-loseinf` 2 項目の **10 項目がすべて `OK`**。
`cfrorg-*-counters` の 4 項目は**カウンタ名を変えたので `DIFFER`**。

- `-stdout` が 1 つでも `DIFFER` → 不合格
- `wininf` / `loseinf` が `DIFFER` → 不合格

### カウンタの数値が動いていないことの確認

名前だけが変わり数値は同じであることを、名前を戻して突き合わせて示す。

```
sed -e 's/check_terminal=/is_terminated_win=/' \
    -e 's/soldier_win=/sol_win=/' \
    -e 's/wizard_win=/wiz_win=/' \
    -e 's/wizard_lose=/wiz_lose=/' \
    logs/regress/mismatch/cfrorg-557-6.cnt > /tmp/renamed.cnt
diff /tmp/renamed.cnt logs/regress/base/cfrorg-557-6.cnt
```

**差分が出たら不合格。** 446 の 6 と 7、557 の 7 も同じ手順で確認する。
`is_win` をメモ化しても `CW_BUMP` は `is_win` に無いので、カウンタは動かない。

### 基準の取り直し

上の 2 つが通ったら `./regress.sh save` で基準を取り直す。**通る前に取り直さない。**

---

## やらないこと

- `is_lose` の戻り値の形 (B2)
- 姫の不具合、`wizard_lose_uncached` の深さ 2 件、古いコメント (A)
- `enemy_turn_win_uncached` / `draw_win_uncached` の入口の `check_terminal` 短絡。
  どちらも引数を取らない位置レベルの関数なので、短絡していて正しい
- 深さ (turns) の意味や値を変えること
- `bs` という引数名
- ADR 0007 の更新 (B2 と A が終わって形が固まってから 1 度に書く)

---

## 実装中に判明した2点 (計画書の訂正)

### 訂正1: `is_win` の `bs.is_my_turn` ガードを外す必要がある

6 節で `draw_win_uncached` の OR ノードを `is_win` に置き換えると、`is_win_uncached`
の手札2枚の分岐 `else if(bs.is_my_turn && bs.hand_s[1].has_value())` が素通りし、
「勝てる行動なし」を返してしまう。`draw_win_uncached` は
`next_bs.is_my_turn = !next_bs.is_my_turn;` を通した `next_bs` を渡すためである。
`is_my_turn` は判定の再帰が1段ごとに機械的に反転させる簿記で、`hand_s` が
「今から手を選ぶ側の手札」であることとは独立している。

実測 (HEAD に計測コードを入れて計数):

| | OR ノード到達 | `is_my_turn == false` | かつ手札2枚 |
|---|---|---|---|
| cfrorg 446 深さ6 | 2899 | 1568 | 1568 |
| cfrorg 557 深さ6 | 4745 | 3951 | 3951 |
| win 446 | 10067 | 6479 | 6479 |
| win 557 | 18091 | 16150 | 16150 |

`is_my_turn` が偽のケースは**すべて手札2枚**だった。

**対処**: ガードから `bs.is_my_turn` を外し、`bs.hand_s[1].has_value()` だけにする。
外部から `is_win` を呼ぶ側がこの条件を踏まないことを実測で確認した
(HEAD では `is_win` は再帰に入っていないので、HEAD での計数が外部呼び出しの
答えそのものになる):

| | `is_win` 外部呼び出し | `!is_my_turn && 手札2枚` |
|---|---|---|
| win 446 | 6,298,972 | **0** |
| win 557 | 3,496,616 | **0** |
| cfrorg 446 深さ6 | 0 | 0 |

踏み数が 0 なので、ガードを外しても外部から見た `is_win` の答えは変わらない。

### 訂正2: `cfrorg-*-counters` は数値も動く (すべて減る方向)

「数値は変わらない」は誤り。実測 (557 深さ6):

| | 基準 | 変更後 |
|---|---|---|
| `check_terminal` | 24566 | 18231 (−26%) |
| `use_win` | 13975 | 9942 (−29%) |
| `open_e` | 14519 | 11320 |
| `hand_e_max` | 3003 | 1610 |
| `hand_e` | 171738 | 141618 |
| `deck_or_hand_e` | 182117 | 172333 |
| `enemy_turn_win` / `draw_win` / `soldier_win` / `use_lose` / その他 | — | **すべて同一** |

増えた項目は1つも無い。判定の中核4本が不変のまま `check_terminal` と `use_win`
だけが減るのは、(a) `is_win` をメモ化したので同じ局面の `check_terminal` +
`use_win` を繰り返さなくなった、(b) `enter_play` が `use_win` を2回でなく
`is_win` を1回呼ぶようになり、終端局面では `check_terminal` だけで返る、
の2つで説明がつく。

したがって検証手順の「名前を戻して `diff`、差分が出たら不合格」は使えない。
**判定の同値性は `-stdout` 6 本と `wininf` / `loseinf` 4 本の一致で見る。**

### 実測した速度とメモリ (`is_win` のメモ表が1本増えたぶん)

| | HEAD | 変更後 |
|---|---|---|
| `cfrorg 4 4 6 6` | 0.78 s / 5236 KB | 0.82 s / 5448 KB |
| `win 4 4 6` | 81.61 s / 1077824 KB | 81.47 s / 1078824 KB |

実質変わらない。

---

## 型の作り直し (レビューを受けての変更)

`wizard_target` enum は廃止し `bool to_self` に戻した (`ef_wizard` が `bool` を
取るので詰め替えが増えるだけだった)。`terminal_check` 構造体と `decided_by` /
`terminal_as_win_result` の2つの補助関数も廃止し、

```cpp
enum class terminal_kind { not_terminal, lost, won };
terminal_kind check_terminal(const belief_state& bs);
```

の3値だけにした。`check_terminal` の第3ブロック (兵士(1)/騎士(3)/魔術師(5) を
出した瞬間の勝ち) は `use_win_uncached` の先頭へ移した。出すカードが決まって
からの問いであり、これを移したことで `check_terminal` は勝ちカードを名指しする
責任を失う。残る `won` は山札が尽きての勝ちだけで、そのとき手札は1枚なので
どのカードかは自明になる。

名前も変えた: `win_result::holds` → `is_win`、`lose_result::holds` → `is_lose`、
`win_decision::holds` → `win_bits`、`any()` → `has_win()`、`at()` → `wins_with()`。

### この変更で出力が動く (`win` の 446 の wininf のみ)

`./regress.sh check`: `cfrorg-*-stdout` 4 本、`win-*-stdout` 2 本、
`win-557-wininf` / `win-*-loseinf` 3 本が `OK`。**`win-446-wininf` だけ `DIFFER`**。

行を突き合わせた結果:

| | |
|---|---|
| 旧の行数 | 1,969,948 |
| 新の行数 | 2,025,016 (+55,068) |
| **消えた行** | **0** |
| 増えた行 | 55,068 |
| 増えた行のうち、同じ履歴が旧に1行だけあったもの | 55,068 (全件) |
| 増えた行が旧行と同じスロットだったもの | 0 (全件もう一方のスロット) |
| 増えた行のうち選択ノード | 0 (全件カード使用ノード) |

**純粋な追加**である。旧は終端判定が名指しした1枚だけを書いていたが、新は
もう一方のカードでも勝てる場合にその行も書く。短絡が隠していた事実が出た形で、
`use_win` に問い直した結果なので、判定そのものは従来と同じ関数に依っている。

`win-446-stdout` が動かないのは `win_move[]` が情報集合ごとの最短手数を数えて
おり、行数ではないため。557 が動かないのは 5 が2枚とも取り除かれる部分ゲームで、
移した第3ブロックに該当する局面が無いため。
