# 位置レベルの判定を use_win の中から追い出す

## 目的

`use_win(bs, card)` は 2 つの違う問いに答えている。

```cpp
std::pair<bool, int> belief_state_win_checker::use_win_impl(const belief_state& bs, Card card) {
  CW_BUMP(use_win);
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;      // ← card を一切見ずに返す
  ...
  if(card == Card{8}) return {false, 0};   // ← ここまで来ない
```

`is_terminated_win` は `bs.hand_s[1].has_value()` が真のときにも発火し
(`belief_state_win.hpp:69-80` の `!bs.barrier_e && bs.hand_s[1].has_value()` の分岐)、
`{1,1}` `{3,1}` `{5,1}` を返す。`pair<int,int>` から `pair<bool,int>` への暗黙変換で
`1`/`3`/`5` はすべて `true` になるので、**終局判定が効く局面では
`use_win(bs, どのカードでも)` が `true` を返す**。手札が {兵士(1), 姫(8)} なら、
姫を捨てて即負けするスロットまで「必勝」と答える。

引数を取る判定 3 本のうち、この短絡を持っているのは `use_win_impl` だけである。
`sol_win_impl` (`belief_state_win.hpp:533`) と `wiz_win_impl` (`:556`) は持っていない。
引数を取らない `enemy_turn_win_impl` (`:305-308`) と `draw_win_impl` (`:464-467`) は
位置レベルの関数なので、短絡していて正しい。

**`use_win` を行動レベルの問いだけに答えさせ、位置レベルの判定は呼び出し側に出す。**

## この変更は org と rnd の両方に触る

判定 (`belief_state_win.hpp`) は org と rnd の共通コードなので、呼び出し側の
両方を同時に直す。

| ファイル | モデル | 役割 |
|---|---|---|
| `belief_state_win.hpp` | 共通 | 短絡を外し、手順をまとめるヘルパを足す |
| `visit_winlose.hpp` | org (`cfrorg`) | `use_win` を直に 2 回呼んでいる |

`infset_iswin.cpp` (rnd) は**触らない**。すでに `is_terminated_win` を先に見て
名指しのスロットだけを書いており (`infset_iswin.cpp:64-71`)、この変更で挙動が
変わらない。ヘルパに寄せたくなるが、そこの if 連鎖は終局を先に見ることに
依存している (山札 2 枚未満で手札 1 枚の終局局面が、順序を変えると
`winlose_unreachable` に落ちてしまう)。整理は符号化を変える次の変更でまとめて行う。

## 出力は 1 ビットも変わらない (最重要)

この変更の合否は **`./regress.sh check` が stdout と bin の全項目で一致すること**。

なぜ一致するのかを先に示す。`is_terminated_win(bs)` の戻り値 `t` で場合分けする。

| `t.first` | 意味 | 今の `use_win` の答え | 変更後 | 統計への影響 |
|---|---|---|---|---|
| `-1` | 終局判定に該当せず | 通常どおり評価 | 同じ | なし |
| `0` | 終局しているが勝ちではない | 両スロット `{false, 0}` | 同じ | なし |
| `1`〜`8` | 終局していて勝ち | **両スロット** `{true, t.second}` | 名指しされたカードのスロットだけ `{true, t.second}`、もう片方は `{false, 0}` | **なし** (下記) |

`win_points` / `lose_points` は `.first` が真のときしか `.second` を読まないので、
勝たない側の深さを 0 にしても統計は動かない。

`t.first` が 1〜8 のとき、`visit_winlose.hpp:94-99` は今こう動く。

```cpp
    if(res_0win.first && res_1win.first) {
      if(res_0win.second <= res_1win.second) win_points[res_0win.second]++;   // ← ここ
      else win_points[res_1win.second]++;
    } else if(res_0win.first) win_points[res_0win.second]++;                   // ← 変更後はここ
```

今は両方 true で深さも同じ `t.second` なので `win_points[t.second]++`。変更後は
片方だけ true で深さは `t.second` なので、やはり `win_points[t.second]++`。
`rm_bywin` は OR なのでどちらも true。`pf[n.depth].w_inc` も OR なので同じ。
**したがって `cfrorg` の標準出力は 1 文字も動かない。**

`infset_iswin.cpp` は触らないが、そこから呼ばれる `use_win` の中身が変わるので
確認が要る。`win_rows` を作る 2 枚手札の分岐は `term.first == -1` のときにしか
到達しない (その手前で終局を捌いている) ので、**そこで呼ぶ `use_win` は
もともと短絡していない**。`win_move` は `is_win` 経由で、`is_win` も先頭で
終局を見て早期に返す (`belief_state_win.hpp:91-92`)。**`win` の標準出力も
`wininf`/`loseinf` の sha256 も動かない。**

### 動いてよいもの

**`cfrorgcnt` のカウンタは動く。** `is_terminated_win` の呼び出しが「`use_win` の
メモ不命中ごとに1回」から「意思決定点ごとに1回」に変わり、`use_win` は終局局面で
呼ばれなくなる。`regress.sh` の `cfrorg-*-counters` が `DIFFER` になるのは想定内で、
**この変更ではそれだけが差分であること**を確認する。`-stdout` が1つでも
`DIFFER` になったら不合格。

---

## 1. `belief_state_win.hpp`

### 1.1 `use_win_impl` から短絡を外す

`belief_state_win.hpp:169-173` の

```cpp
std::pair<bool, int> belief_state_win_checker::use_win_impl(const belief_state& bs, Card card) {
  CW_BUMP(use_win);
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;

```

から `auto t = ...;` と `if(t.first != -1) return t;` の **2 行を削除する**。
`CW_BUMP(use_win);` は残す。以降の本体は 1 行も変えない。

### 1.2 プロトコルをまとめるヘルパを足す

「位置レベルを先に見て、そのあと行動ごとに訊く」という手順を呼び出し側に
書き写すと必ずどこかで忘れる。1 本にまとめる。`belief_state_win_checker` の public メンバに
足す (宣言は `belief_state_win.hpp:17` の `use_win` の隣、定義は `use_win` の定義の
直後)。

```cpp
  // 手札 2 枚それぞれについて必勝かを返す。{スロット0の答え, スロット1の答え}。
  // 位置レベルの終局判定をここで捌いてから use_win に降りる。use_win は
  // 行動レベルの問いにしか答えないので、この順序を呼び出し側で崩してはいけない。
  std::pair<std::pair<bool, int>, std::pair<bool, int>>
  win_actions(const belief_state& bs, Card c0, Card c1);
```

```cpp
std::pair<std::pair<bool, int>, std::pair<bool, int>>
belief_state_win_checker::win_actions(const belief_state& bs, Card c0, Card c1) {
  auto t = is_terminated_win(bs);
  if(t.first != -1) {
    // t.first == 0 は「終局しているが勝ちではない」。1〜8 は勝てるカードの名指し。
    if(t.first == 0) return {{false, 0}, {false, 0}};
    if(c0.value() != t.first && c1.value() != t.first)
      win_actions_mismatch(t.first, c0.value(), c1.value());
    // 勝たない側の深さは 0。is_win が res.first == 0 のとき res.second = 0 を
    // 返すのと同じ慣習に揃える (belief_state_win.hpp:147-148)。
    const bool w0 = (c0.value() == t.first), w1 = (c1.value() == t.first);
    return {{w0, w0 ? t.second : 0}, {w1, w1 ? t.second : 0}};
  }
  return {use_win(bs, c0), use_win(bs, c1)};
}
```

`c0` と `c1` が同じカードなら両方 true になる。それが正しい (どちらを出しても
同じ手を出したことになる)。

上のコードにある `win_actions_mismatch` は、**`t.first` が 1〜8 なのに `c0` とも
`c1` とも一致しない場合**の検査である。`is_terminated_win` が名指しするカードは
`have_s(...)` か `hand_s[0]` から来るので手札に必ずあり、この状態は起きないはず。
だが検査を置かずに素通りさせると `{false,0}` が 2 つ返り、**`rm_bywin` が false に
落ちて統計が変わる**。黙って間違えるより落とす (`assert` は `-DNDEBUG` で無効
なので使わない。`card_table.hpp:11-14` の `card_range_error` と同じ方針)。
`belief_state_win.hpp` の先頭、`is_terminated_win` の宣言の手前に置く。

```cpp
[[noreturn]] inline void win_actions_mismatch(int named, int c0, int c1) {
  std::fprintf(stderr,
               "win_actions: 終局判定のカード %d が手札 {%d, %d} に無い\n",
               named, c0, c1);
  std::abort();
}
```

`<cstdio>` と `<cstdlib>` が必要。`belief_state_win.hpp` が既に include して
いなければ足す。

### 1.3 `draw_win_impl` の OR ノードに位置検査を足す

`belief_state_win.hpp:484` の `use_win(next_bs, ...)` は、ドロー後の `next_bs` に
対する呼び出しで、`next_bs` の終局判定は誰も見ていない。今は `use_win` の短絡が
拾っていたので、外に出す。

ここは「`next_bs` から勝てるか」という**位置レベル**の問いなので、ヘルパではなく
直接の短絡でよい。`belief_state_win.hpp:483-506` の OR ノード全体を、以下で
**丸ごと置き換える**。`auto res0 = use_win(...)` の行は `else` の中に移動する
(外に残してはいけない)。

置き換え前 (`:483-506`):

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
      bool or_first = false;
      int or_second = 0;

      // next_bs が終局しているかは位置レベルの問い。use_win は行動レベルの
      // 問いにしか答えないので、ここで先に捌く。
      auto term = is_terminated_win(next_bs);
      if(term.first != -1) {
        or_first = (term.first != 0);
        or_second = term.second;
      } else {
        auto res0 = use_win(next_bs, next_bs.hand_s[0].value());

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
      }
```

この下の「`--- 山札ドローの集約 (ANDノード) ---`」以降 (`:508-513`) は**1 行も
変えない**。ループ先頭の `if(!all_true) break;` (`:476`) も変えない。

今との同値性: 終局のとき、今は `res0` も `res1` も `t` を返すので
`or_first = (t.first != 0)`、`or_second = min(t.second, t.second) = t.second`。
置き換え後も同じ値になる。終局でないとき (`term.first == -1`) は、今と
まったく同じコードが走る。

---

## 2. `visit_winlose.hpp`

`visit_winlose.hpp:81-82` の

```cpp
    auto res_0win = wc.use_win(bs, Card{c1});
    auto res_1win = wc.use_win(bs, Card{c2});
```

を

```cpp
    auto [res_0win, res_1win] = wc.win_actions(bs, Card{c1}, Card{c2});
```

にする。**他の行は 1 行も変えない。** `res_0lose` / `res_1lose` も、
`decision_points` / `win_points` / `lose_points` の計算も、`pf[]` も触らない。

構造化束縛にすると `assert(0 <= res_0win.second && ...)` (`:87-88`) がそのまま
使えるので、そこも変えない。

---

## 検証

### ビルド

```
make cfr cfr0 cfrorg cfrorgcnt brrnd brorg win
```

新しい警告を出さないこと。

### 回帰

```
./regress.sh check
```

**期待する結果**: `cfrorg-*-counters` の 4 項目だけが `DIFFER`、それ以外の
`cfrorg-*-stdout` 4 項目・`win-*-stdout` 2 項目・`win-*-wininf` 2 項目・
`win-*-loseinf` 2 項目の**計 10 項目がすべて `OK`**。

- `-stdout` が 1 つでも `DIFFER` → 不合格。統計か判定を動かしている
- `wininf` / `loseinf` の sha256 が `DIFFER` → 不合格。記録の中身が変わっている
- `-counters` が `OK` → 変更が効いていない。`use_win` の短絡を外せていない

カウンタが動く理由は「絶対に動かしてはいけないもの」の節に書いたとおり。
**`DIFFER` になったカウンタの実数を報告に残すこと** (どのカウンタがどれだけ
減ったかが、短絡を外した効果の実測になる)。

### 静的解析

```
make cppcheck
```

無指摘であること。

---

## やらないこと

- 0/1/2/3 の符号化 (次の変更)。`win_actions` は今は 2 つの `pair` を返すだけ
- 深さ (手数) を落とすこと。早期脱出は実測で効かないと分かっている
- `is_win` / `is_lose` の整理と `infset_iswin.cpp` のヘルパへの集約 (次の変更)
- `enemy_turn_win_impl` / `draw_win_impl` の入口の短絡 (`:307-308`, `:466-467`)。
  どちらも引数を取らない位置レベルの関数なので、短絡していて正しい
- `sol_win_impl` / `wiz_win_impl` (`:533`, `:556`)。もともと短絡を持っていない
- 必敗側 (`belief_state_lose.hpp`)。`use_lose_impl` は `is_terminated_win` を
  呼んでいないので、同じ問題は無い
