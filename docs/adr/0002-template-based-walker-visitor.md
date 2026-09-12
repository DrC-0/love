# 展開器と判定器を関数テンプレートで接続する

展開器から判定器を呼ぶ機構として、仮想関数ではなく関数テンプレート + policy 型を
選んだ。実測では判定が実行時間の 66% を占め、仮想呼び出しのコストは 0.1% 未満
だったので、性能は選定理由ではない。理由は、将来 cfr.hpp / infset_dfs.hpp など
double を返す 5 本の展開部分を同じ展開器に畳むときに、戻り値型を型パラメータ
(typename V::result_type) にできる必要があるため。仮想関数の基底クラスでは
void を返す版と double を返す版を同じ型で表せず、そこで詰む。

なお本文中の「判定が実行時間の 66%」は `-pg` ビルドの gprof による数字で、
その比率は信用できない (docs/adr/0003 参照)。判定が支配的であること自体は
決定的カウンタでも裏づけが取れているが、66% という値は根拠として使わないこと。
この決定の理由は性能ではないので、結論は変わらない。

## 追記: 展開器を1〜2本に畳む

同じゲーム木を歩く展開器が (`newcfr.hpp` を削除した時点で) 8本ある。すべて同じ
7〜8関数の形 (`put_hide_card` / `draw_p1_init` / `draw_p2_init` / `draw` / `play` /
`wizard` / `wizard_self` / `soldior`) をしていて、合計 4,224 行が1つの木の8重写しに
なっている。`org_tree.hpp` (362行) だけが展開器と判定器に分離済みで、残る7本は
測る内容が本体に埋め込まれたまま。

| 展開器 | 行数 | 戻り値 | 追加引数 |
|---|---|---|---|
| `org_tree.hpp` | 362 | void | `V&` (判定器) |
| `rnd_make_infset.hpp` | 362 | void | — |
| `cfr.hpp` | 380 | double | — |
| `cfr_exp_reward.hpp` | 379 | double | — |
| `infset_dfs.hpp` | 883 | double | `vector<string>&, int` |
| `infset_dfs_rnd.hpp` | 759 | double | 同上 |
| `all_elements.hpp` | 573 | void | `string&, unsigned long` |
| `all_elements_rnd.hpp` | 526 | void | 同上 |

重複の度合いは実測で、`cfr.hpp` と `cfr_exp_reward.hpp` は `ut_` 接頭辞を落とすと
380行中71行しか違わない (しかも約半分はコメントアウトされた死んだコード)。
`all_elements` の org/rnd は 573行中117行差、`infset_dfs` の org/rnd は 883行中308行差。

**目標**: `org_tree.hpp` を発展させ、8本を1〜2本の展開器に畳む。本文で述べた
「戻り値型を `typename V::result_type` にできる必要がある」はこのための条件。

### 却下した代替案: 実行時 enum による分岐

削除した `newcfr.hpp` (533行) が、`rnd_make_infset` / `cfr` / `cfr_exp_reward` の
3本を**実行時の enum 分岐**で1本に畳んでいた。

```cpp
enum game_tree_mode { MRND_DS, MCFR, MCFR_EXP_REWARD };
extern game_tree_mode g;
```

`if(g == MRND_DS)` の類が展開器の本体に **44箇所**散在する形になっていた。これは
`loveletter.cpp` の `do_action` が既に抱えている病 (`cfr_switch` / `br_switch` /
`br_player` による分岐が30箇所超) とまったく同じ形で、ゲームのルールと解析
アルゴリズムが1つの関数に同居する。9本を畳むときにこの方式を採ると、44箇所が
より大きな数になって再現する。

この実装はどこからも include されておらず動いていなかったため削除したが、
「実行時 enum で畳む案は一度試されて、同じ絡まり方をした」という事実は
テンプレート + policy を選ぶ理由の実物なので、ここに残す。中身は
`git log -- newcfr.hpp` で辿れる。
