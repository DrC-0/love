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

### 実施: `all_elements` の org/rnd を畳んだ (2026-09-13)

8本のうち最初の対を畳んだ。`all_elements.hpp` (573行) と
`all_elements_rnd.hpp` (526行) を `template <class P> struct all_elements_walker`
1本にし、合計 1,099行 → 626行。

ポリシー型が供給するのは4つだけだった。

| ポリシーの要素 | org | rnd |
|---|---|---|
| 完全ハッシュ表 `ph()` | `oph` | `rph` |
| `max_hash_value` | `ORG_MAX_HASH_VALUE` | `RND_MAX_HASH_VALUE` |
| 木の履歴 `his()` / `his_p()` | `org_his` / `org_his_p` | `rnd_his` / `rnd_his_p` |
| `soldior_is_decision` | `true` | `false` |

機械的な置換で消えない差は 573行中 **75行** で、その大半が兵士 (`case 1`) の扱い。
org は宣言を意思決定として列挙し、rnd は一様ランダムとみなして確率を計算するだけ。
これは `if constexpr(P::soldior_is_decision)` で分けた。

#### 学び1: ポリシーを通してはいけないものがある

`table_infset` を引く2箇所は、**org 版でも `rnd_his_p` と書かれている**。
`br.cpp` は org ビルドでも `rnd_make_infset.hpp` を include して rnd の
情報集合表を作るため。「ゲーム木の履歴は org、情報集合表の鍵は rnd」という
使い分けが意図的にある。ここをポリシー経由に「統一」すると org の挙動が変わる。

対をテンプレートに畳むとき、**2つのファイルの差分がすべてモデルの差とは限らない**。
同じに見えて意図的に片方に寄せてある箇所を先に洗い出すこと。

#### 学び2: 自由関数の暗黙の切り替えに依存していた

`infset_dfs.hpp` / `infset_dfs_rnd.hpp` は `all_exp_reward` の中から
`all_put_hide_card` などを**無修飾の自由関数として**呼んでいる。そして
`infset_dfs.hpp` は `br.cpp` と `compare_abscfr.cpp` の両方から include され、
**同じ1行が翻訳単位によって別の関数に解決されていた**。切り替えは
「どちらの `all_elements` を include したか」という暗黙の仕掛け。

そのため `infset_dfs*.hpp` 側に `all_elements_walker<org_elements>::` と
直書きすると、`br.cpp` か `compare_abscfr.cpp` のどちらかが必ず壊れる。

解決として、旧シグネチャの `inline` 自由関数ラッパーを8本残し、その中身を
`ALL_ELEMENTS_ORG` で選ぶ形にした。Makefile の `brorg` にだけ `-D` を付ける。
結果として `infset_dfs*.hpp` (計1,642行) と `br.cpp` は**1行も変えずに済んだ**。

暗黙の切り替え (どちらのファイルを include したか) が明示の切り替え
(フラグ1つ) になったので、残り3対を畳むときも同じ手が使える。

#### 学び3: `if constexpr` で捨てられる側でも名前解決は起きる

org 分岐は `soldior_prob` を参照するが、`br_rnd.cpp` はこの配列を定義していない。
`if constexpr` は**コード生成**を捨てるだけで、非依存名の**名前解決**は
テンプレート定義時に起きる。そのため `all_elements.hpp` に
`extern double soldior_prob[8];` の宣言が必要になった (定義は不要。捨てられた
分岐は odr-use しないのでリンクは通る)。

#### 検証

`brorg` は完走しない (`5 5 7` で40分打ち切り、`4 4 6` で10分打ち切り)。
実行による検証は `comp 4 4 6` (70秒、出力20行) だけで、これは rnd 側しか通らない。
**org 側はコンパイルとシンボルの確認までしか保証できていない。**

- `nm -C brorg | grep 'all_elements_walker<org_elements>'` が8本
- `nm -C comp` に org は0本、`nm -C brrnd` にも0本

`comp 5 5 7` は検証に使えない。`get_action` が範囲外の添字で gperf の語表を
読む未定義動作があり、実行のたびに正常終了 / abort / segfault が変わる。
`373d4ba` から再現する既存の不具合で、この変更とは無関係。
