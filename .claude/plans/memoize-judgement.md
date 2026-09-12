# 判定をメモ化する (belief_state_win_checker / belief_state_lose_checker)

## 目的と根拠

判定は同じ Belief State を何度も評価し直している。測定済みの数字 (部分ゲーム557、
打ち切り深さ6、詳細は `archive/measure-memo-feasibility` タグ):

| 関数 | 呼び出し | 相異なる状態 | 比 |
|---|---|---|---|
| `enemy_turn_win` | 36,590,596 | 7,742 | 4,726 |
| `sol_win` | 27,616,571 | 4,316 | 6,399 |
| `use_win` | 24,560,516 | 13,975 | 1,757 |
| `draw_win` | 14,846,732 | 2,849 | 5,211 |
| `use_lose` | 1,230,588 | 6,552 | 188 |

使い捨てのメモ表で壁時計を実測した結果 (`archive/measure-memo-walltime` タグ)、
ADR 0003 の手順 (交互実行・中央値) で **4.5〜17.6 倍速**。出力は 8 ケースすべてで
基準と完全一致した。

| 条件 | 基準 | メモ化 | 倍率 |
|---|---|---|---|
| 557 n=7 | 0.92s | 0.07s | 13.1 |
| 446 n=7 | 1.41s | 0.08s | 17.6 |
| 557 n=6 | 4.67s | 0.66s | 7.1 |
| 446 n=6 | 7.87s | 0.75s | 10.5 |
| 557 n=5 | 25.70s | 5.66s | 4.5 |
| 446 n=5 | 42.66s | 6.73s | 6.3 |

## メモ化が安全である根拠

判定 7 本は `belief_state` と引数以外の可変状態を一切読まない。実測で確認済み。

- `end_deck_n` の参照: 0 (`belief_state*.hpp` すべて)
- `cfr_switch` / `br_switch` / `org_switch` / `br_player` / `table_infset` の参照: 0
- `commentable_bs` の参照: 3 箇所。いずれもデバッグ出力のみ (下記で対処)

したがって戻り値は (belief_state, card または to0p) の純関数であり、
同じ鍵に対して同じ値を返す。

## org と rnd のどちらを触るか

**どちらも触らない。** 判定は org / rnd を区別しない。`rnd` 引数を持つのは
履歴コンストラクタと `action2char` / `get_actions_history` だけで、
`belief_state_history.hpp` には 1 行も触れない。展開器 (`org_tree.hpp` /
`rnd_make_infset.hpp`) にも触れない。

## 行動履歴に手を入れるか

**入れない。** 履歴文字列の生成も解釈も変更しない。

## 変更 1: `belief_state_win_checker` を作る

`belief_state_win.hpp` に追加する。**判定のロジックは 1 行も変えない。**
既存の自由関数の本体をそのままメンバ関数の本体に移す。

```cpp
#include <unordered_map>

struct belief_state_win_checker {
  // belief_state の16フィールドと追加の鍵 (card 0..8 / to0p) を 59bit に詰める。
  // 値域: open_flag / sol_flag / hand_s は 0..8、trash[i] は max_num[i] 以下。
  static unsigned long long key(const belief_state& bs, int extra);

  std::pair<int, int> is_win(const belief_state& bs);          // メモ化しない (入口、再帰しない)
  std::pair<bool, int> use_win(const belief_state& bs, int card);
  std::pair<bool, int> enemy_turn_win(const belief_state& bs);
  std::pair<bool, int> draw_win(const belief_state& bs);
  std::pair<bool, int> sol_win(const belief_state& bs, int card);
  std::pair<bool, int> wiz_win(const belief_state& bs, bool to0p);

private:
  std::unordered_map<unsigned long long, std::pair<bool, int>> m_use_win, m_enemy_turn_win,
      m_draw_win, m_sol_win, m_wiz_win;
};
```

メモ化する 5 本は次の形にする (`enemy_turn_win` の例)。

```cpp
std::pair<bool, int> belief_state_win_checker::enemy_turn_win(const belief_state& bs) {
  const unsigned long long k = key(bs, 0);
  if(!commentable_bs) {
    auto it = m_enemy_turn_win.find(k);
    if(it != m_enemy_turn_win.end()) return it->second;
  }
  auto r = enemy_turn_win_impl(bs);   // 中身は現在の enemy_turn_win をそのまま
  if(!commentable_bs) m_enemy_turn_win.emplace(k, r);
  return r;
}
```

`_impl` は `private` のメンバ関数にする。

**本体は現在の自由関数から 1 文字も変えずに移す。** 中の
`enemy_turn_win(...)` `use_win(...)` `draw_win(...)` `sol_win(...)` `wiz_win(...)`
`is_terminated_win(...)` といった呼び出しは**書き換えなくてよい**。メンバ関数の
中では修飾なしの名前がまずメンバとして解決されるので、`enemy_turn_win(bs)` は
自動的にメモ化されたメンバ版を呼ぶ。`is_terminated_win` はメンバではないので
自由関数がそのまま見つかる。どちらも意図どおり。

**例外は lose 側だけ。** `belief_state_lose_checker` の中の `draw_win(...)` は
同じクラスのメンバではないので名前解決に失敗し、`win.draw_win(...)` に直す
必要がある (コンパイルエラーになるので見落とすことはない)。

### `commentable_bs` のときはメモを迂回する

`commentable_bs` は `belief_state.hpp` の `inline bool commentable_bs = false;`。
**通常は `false` のまま**で、判定がどのような順で実行されているのかを検査したい
ときに、`test.cpp` のようなスクラッチから手で `true` にして使う。`true` にすると
`belief_state_win.hpp` の 3 箇所 (`use_win` / `enemy_turn_win` / `draw_win`) が
実行の過程を標準出力に流す。

メモが当たると計算そのものが飛ぶので、その行が出なくなる。**実行順を見るのが
目的なのにトレースが欠けるのでは用を成さない。** そのため `commentable_bs` が
真のときは引きも書き込みもせず、毎回計算する。通常時は `false` なので分岐は
常に同じ側に落ち、分岐予測が当たる。

この分岐を省いて「どうせデバッグ用だから」と済ませてはいけない。省くと、
検査のために `true` にした人が**欠けたトレースを見て誤った結論を出す**。

### 鍵の詰め方

```cpp
unsigned long long belief_state_win_checker::key(const belief_state& bs, int extra) {
  unsigned long long k = 0;
  int b = 0;
  auto put = [&](unsigned long long v, int w) { k |= v << b; b += w; };
  put(bs.is_my_turn, 1);  put(bs.is_sol_choice, 1);  put(bs.is_wiz_choice, 1);
  put(bs.not7_flag_s, 1); put(bs.not7_flag_e, 1);
  put(bs.barrier_s, 1);   put(bs.barrier_e, 1);
  put(bs.lt5_flag_s, 1);  put(bs.lt5_flag_e, 1);
  put(bs.open_flag_s, 4); put(bs.open_flag_e, 4);
  put(bs.sol_flag_s[0], 4); put(bs.sol_flag_s[1], 4);
  put(bs.sol_flag_e[0], 4); put(bs.sol_flag_e[1], 4);
  put(bs.hand_s[0], 4);   put(bs.hand_s[1], 4);
  static const int tw[8] = {3, 2, 2, 2, 2, 1, 1, 1}; // max_num = {5,2,2,2,2,1,1,1}
  for(int i = 0; i < 8; i++) put(bs.trash[i], tw[i]);
  put(extra, 4);
  return k;
}
```

合計 59bit。**16 フィールドすべてを含めること。** 1 つでも落とすと違う局面が
同じ鍵になり、誤った結果を返す (回帰ハーネスが捕まえるが、原因の特定が難しい)。

メモ表は関数ごとに分ける。同じ (bs, extra) でも関数が違えば答えが違うため、
1 つの表にまとめてはいけない。

## 変更 2: `belief_state_lose_checker` を作る

`belief_state_lose.hpp` に追加する。`use_lose` と `wiz_lose` が
`draw_win` を呼ぶので、**win 側の checker への参照を持つ**。

```cpp
struct belief_state_lose_checker {
  explicit belief_state_lose_checker(belief_state_win_checker& w)
    : win(w) {}

  std::vector<std::pair<int, int>> is_lose(const belief_state& bs);  // メモ化しない
  std::pair<bool, int> use_lose(const belief_state& bs, int card);
  std::pair<bool, int> wiz_lose(const belief_state& bs, bool to0p);

private:
  belief_state_win_checker& win;
  std::unordered_map<unsigned long long, std::pair<bool, int>> m_use_lose, m_wiz_lose;
};
```

依存の向きは ADR 0006 のまま (lose → win の一方向)。**逆にしてはいけない。**
`is_lose` は `std::vector` を返し自己再帰もしないのでメモ化しない
(`use_lose` / `wiz_lose` を呼ぶだけ)。

本体の中の `draw_win(...)` はすべて `win.draw_win(...)` になる。

## 変更 3: メモ化しない関数はそのまま

次は**自由関数のまま残す**。

- `is_terminated_win` — 比較数個の安い述語。再帰しない。メモ化後の実測で
  24,566 回まで落ちており、表を引くほうが高くつく可能性が高い。
- `able_actions` / `action_count` — 判定を呼ばない。`belief_state` のアクセサのみ。
- `ef_wizard` / `draw` / `swap_player` / `reset_flag_by_use` — `belief_state.hpp` の遷移。

## 変更 4: 呼び出し側 (3 ファイル 11 箇所)

### `visit_winlose.hpp` (`cfrorg`)

`winlose_visitor` にメンバを 2 つ足す。**宣言の順序が重要** (`lc` が `wc` を
参照するので `wc` が先)。

```cpp
belief_state_win_checker wc;
belief_state_lose_checker lc{wc};
```

呼び出し 9 箇所を書き換える。

| 行 | 現在 | 変更後 |
|---|---|---|
| 76, 77 | `use_win(bs, c1)` / `use_win(bs, c2)` | `wc.use_win(...)` |
| 78, 79 | `use_lose(bs, c1)` / `use_lose(bs, c2)` | `lc.use_lose(...)` |
| 124 | `sol_win(soldier_bs[n.depth], i)` | `wc.sol_win(...)` |
| 147, 148 | `wiz_win(bs, 0)` / `wiz_win(bs, 1)` | `wc.wiz_win(...)` |
| 149, 150 | `wiz_lose(bs, 0)` / `wiz_lose(bs, 1)` | `lc.wiz_lose(...)` |

**`winlose_visitor` は 1 つだけ作られる** (`cfr_org.cpp` の `cfr_org` が
`winlose_visitor v;` を 1 回)。したがってメモ表は部分ゲーム 1 回の実行を通して
共有され、これが 4.5〜17.6 倍の源になる。

### `infset_iswin.cpp` (`win`)

`infset_iswin.cpp:48` の `is_win(bs)` と `:60` の `is_lose(bs)`。どちらも
`cnt_abs(int open[3], string history)` (`:46`) の中にある。

`cnt_abs` は `infset_iswin` (`:92`) が `:110` のループから**情報集合ごとに
呼んでいる**。したがって checker を `cnt_abs` のローカル変数にすると、
**呼ばれるたびに表が作られて捨てられ、メモがまったく効かない**。

**`infset_iswin` (`:92`) で 2 つの checker を作り、`cnt_abs` に参照で渡す。**
`cnt_abs` のシグネチャを

```cpp
void cnt_abs(int open[3], string history,
             belief_state_win_checker& wc, belief_state_lose_checker& lc)
```

に変え、`:110` の呼び出しも合わせる。`cnt_abs` の中では `wc.is_win(bs)` と
`lc.is_lose(bs)` になる。

`infset_iswin.cpp` はファイル内に `win_cnt` などのグローバルを持っているが、
**checker をそこに足してはいけない。** モード切替のグローバルと同じ形になり、
ADR 0004 が問題にしている構造を増やすことになる。

### `compare_abscfr.cpp` (`comp`)

`:159` の `is_win(bs)` 1 箇所。ここは 1 回しか呼ばないのでローカルに作ってよい。

### `watch_cfr.cpp`

判定を呼んでいないので**変更不要**。

## 変更しないもの

- 判定のロジック。`_impl` に移すだけで 1 文字も変えない。
- `belief_state.hpp` / `belief_state_history.hpp` / `first_hand_open.hpp`。
- `count_work.hpp` のカウンタ名。**絶対に変えない。**
- `CW_BUMP` の位置。`_impl` の側に置く (実計算の回数を数えるため)。
- `loveletter.cpp` / `org_tree.hpp` / `rnd_make_infset.hpp` / `bs_set.hpp` /
  `endgame.hpp` / `test.cpp`。

## 検証

### 手順 1: ビルド

```
make cfr cfr0 cfrorg brrnd brorg win comp cfrorgcnt
```

警告が 1 つも出ないこと。`make test` は通らない (既知)。

### 手順 2: 回帰ハーネス — 契約が変わる

```
./regress.sh check full
```

**この変更では 12 項目すべて一致にはならない。**

| 項目 | 期待 |
|---|---|
| `cfrorg` の stdout 4 項目 | **完全一致でなければならない** (正しさ) |
| `win` の stdout 2 項目 | **完全一致でなければならない** (正しさ) |
| `win` の abs bin 2 項目 | **完全一致でなければならない** (正しさ) |
| `cfrorg` のカウンタ 4 項目 | **DIFFER になる。判定の回数が大幅に減っていること** (成果) |

stdout か abs bin が 1 つでも違ったら実装のミス。カウンタが減っていなければ
メモが効いていない (鍵の作り方か、checker の寿命が短すぎる)。

参考値 (557 n=6、使い捨て実装での実測):

```
enemy_turn_win  36,590,596 ->  7,742
sol_win         27,616,571 ->  4,316
use_win         24,560,516 -> 13,975
draw_win        14,846,732 ->  2,849
use_lose         1,230,588 ->  6,552
```

### 手順 3: 壁時計

ADR 0003 の手順で測る。交互実行の中央値、10% 未満の差は根拠にしない。

```
for r in 1 2 3 4 5; do /usr/bin/time -f "%e" ./cfrorg 5 5 7 6 >/dev/null; done
```

**基準 4.67s に対して 1s 前後になること。** 使い捨て実装では 0.66s だった。

### 手順 4: 基準の取り直し

手順 2 の stdout と abs bin が完全一致し、カウンタの減少を確認したうえで

```
./regress.sh save full
```

**これは 2 回目の例外的な取り直し。** 1 回目は ADR 0006 のカウンタ名変更。
理由を ADR に書くこと。

## やらないこと

- `is_terminated_win` のメモ化。安い述語なので別途測ってから判断する。
- 履歴からの `belief_state` 再構築の最適化。これはメモ化後に次のボトルネックに
  なる (`belief_state_from_history` が 910,276 回で判定より桁が大きい) が、
  **この変更では触らない**。ADR 0003 の「ここを速くしようとしてはいけない」は
  メモ化前の測定に基づく判断なので、追記して測り直すのは別の作業。
- 性能のためにロジックを書き換えること。移すだけ。
