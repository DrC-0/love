# CFR の平均戦略と必勝・必敗判定を突き合わせる comp を作り直す

削除した `compare_abscfr.cpp` の代わりに `compare_strategy.cpp` を新設する。
`win` が書いた `abs/wininf<部分ゲーム>.bin` / `abs/loseinf<部分ゲーム>.bin` と、
`cfr` が書いた `str<部分ゲーム>64.bin` を突き合わせ、**belief_state の必勝・必敗
判定が CFR の平均戦略 σ̄ と大きく食い違っていないか**を調べる。

**これは読み取り専用の調査ツール。既存のコードの挙動は1バイトも変えない。**
`rnd` 側だけを見る (情報集合表は rnd のものしか無いため)。org 側には触らない。

## 呼び方

```
./comp 4 4 6
```

引数は部分ゲームの3枚のみ。反復回数は **64 固定**で `str<部分ゲーム>64.bin` を読む。
終了コードは**常に 0**。違反を見つけても失敗にしない (調査ツールであり、
CI の判定に使うものではないため)。

---

## 1. `str<部分ゲーム>64.bin` の形式 (既存。変更しない)

`cfr.cpp:85-97` が読み書きしている形式。`table_infset`
(`std::map<std::string, infset>` なので履歴文字列の昇順) を先頭から回し、
情報集合1つにつき `float` 4 本を順に並べる。

```
float sum_i0;    // 行動0 の累積戦略 (分子)
float sum_i1;    // 累積戦略の総和 (分母)
float regret0;
float regret1;
```

**σ̄(行動0) は `sum_i0 / sum_i1`。和ではなく比である**
(`cfr_exp_reward.hpp:140-141`)。`sum_i1 == 0` のとき CFR 本体は `p = 0.5` に
落としているので、こちらも同じにする。σ̄(行動1) は `1 - σ̄(行動0)`。

ファイルサイズは `4 * sizeof(float) * table_infset.size()` になる。
**起動時にこれを検査し、合わなければメッセージを出して終了する**
(部分ゲームと str ファイルの取り違えを弾くため)。実測値:

| ファイル | サイズ | ÷16 | `table_infset.size()` |
|---|---|---|---|
| `str44664.bin` | 100,783,552 | 6,298,972 | 6,298,972 (446) |
| `str55764.bin` | 55,945,856 | 3,496,616 | 3,496,616 (557) |

---

## 2. 行動0 / 行動1 が何を指すか (**最重要**)

ノードの種類で意味が変わり、**片方は `winlose_record` のスロットと逆になる**。

### カード使用ノード: 一致する

`cfr_exp_reward.hpp:141-148`:

```cpp
  c1 = n.hand1[0];
  c2 = n.hand1[1];
  n.do_action(5, 0, w);  r1 = ut_play(n, c1);   // 行動0
  n.do_action(5, 1, w2); r2 = ut_play(n, c2);   // 行動1
  r = r1 * p + r2 * (1 - p);
```

行動0 = `hand1[0]`、行動1 = `hand1[1]`。`winlose_record.slot` も
0 = `hand_s[0]`、1 = `hand_s[1]` なので**そのまま対応する**。

### 魔術師の対象選択ノード: **逆になる**

`cfr_exp_reward.hpp:213-292`:

```cpp
    double p = ... get_sum_i(0) / get_sum_i(1);
    // reward1 は do_action(6, ...) = 相手を対象
    // reward2 は do_action(7, ...) = 自分を対象
    reward = reward1 * p + reward2 * (1 - p);
```

**CFR の行動0 = 相手を対象、行動1 = 自分を対象。**

一方 `winlose_record` のスロットは `infset_iswin.cpp` が
`is_lose` / `wizard_win` の並びに合わせており、**0 = 自分、1 = 相手**
(`belief_state_lose.hpp` の `d.slot[0] = wizard_lose(bs, true)`、
`infset_iswin.cpp` の `wizard_win(bs, true)` を slot 0 に積む箇所)。

したがって**対象選択ノードでは、行の `slot` を反転してから σ̄ を引く**。

```cpp
// 行の slot を CFR の行動番号に直す
int cfr_action(const winlose_record& row) {
  return row.is_choice_node ? (1 - row.slot) : row.slot;
}
```

**ここを反転し忘れると、対象選択ノードの結果がすべて逆になる。**
`visit_winlose.hpp` の `wf[]` はスロット0 = 相手で CFR と同じ並びだが、
**comp が読むのは `winlose_record` のほうなので、そちらに合わせること。**

### `is_choice_node` が真になるのは魔術師の対象選択ノードだけ

`winlose_record.is_choice_node` (`save_load_winlose.hpp` のフラグ bit2) を
`infset_iswin.cpp` が真にするのは、次の2箇所だけである。

- 必勝の行: `bs.is_wiz_choice` の分岐で `wizard_win` を呼んで積むとき
  (`{history, true, true, 0}` と `{history, true, true, 1}`)
- 必敗の行: `is_choice_node = (d.kind == decision_kind::wizard_target)`

**兵士の宣言ノードでは真にならない。** rnd では兵士の宣言は自然手番に
抽象化されていて情報集合表に入らず、`cnt_abs` は該当したら
`winlose_unreachable(history, "sol_choice")` で落ちるため。
したがって `is_choice_node` は「魔術師の対象選択ノードか」と読んでよい。

裏取りとして、comp 側でも履歴から組んだ `belief_state` の
`bs.is_wiz_choice` と一致することを確かめる。**食い違ったらメッセージを出して
その行を飛ばす** (行と情報集合表の対応が壊れている証拠なので、黙って数えない)。

### 手札2枚が同じカードのノードは情報集合表に無い

`rnd_make_infset.hpp:120-128` が `use_same_move` で
`n.hand1[0] == n.hand1[1]` のとき情報集合を作らずに返す。
したがって `table_infset` に入っている2枚のノードは必ず違うカードで、
`hand_s[0]` と `hand_s[1]` の区別が意味を持つ。**除外処理は要らない。**

---

## 3. 調べる2方向

閾値は `EPS = 1e-6` を定数で持ち、**出力の先頭に必ず印字する**。
ただし「どの値が妥当か分からない」ので、**σ̄ の分布も併せて出す**
(下の「出力」を参照)。先頭 `N = 10` 件を例示する。

### 方向1: 記録のある (履歴, スロット) の σ̄ が期待と食い違う

- **必勝の行** (`wininf`): そのスロットを出せば勝てるのだから、σ̄ はそちらに
  寄るはず。`σ̄(そのスロット) < EPS` なら「CFR が必勝手を捨てている」= 違反候補。
- **必敗の行** (`loseinf`): そのスロットを出せば負けるのだから、σ̄ はそちらを
  捨てるはず。`σ̄(そのスロット) > 1 - EPS` なら「CFR が必敗手に寄せている」=
  違反候補。

**同じ履歴に両方のスロットの行がある場合は、そのスロットだけを見る**
(必勝が2つあるなら、どちらに寄っていてもおかしくないため、必勝側は
「両方のスロットに行があるなら数えない」)。必敗側は両方あっても
「どちらも負ける」なので σ̄ に関係なく数えない。

### 方向2: σ̄ が偏っているのに記録が無い

`wininf` にも `loseinf` にも1行も無い情報集合で、
`σ̄(行動0) < EPS` または `σ̄(行動0) > 1 - EPS` のもの。
CFR が片方をほぼ捨てているのに判定が何も言えていない局面で、
**判定の取りこぼしの候補**になる。

---

## 4. 実装

### ファイルと Makefile

`compare_strategy.cpp` を新設する。ビルド対象名は `comp`。

```make
comp: compare_strategy.cpp save_load_winlose.hpp rnd_make_infset.hpp \
      belief_state.hpp belief_state_history.hpp \
      $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 $(COMMON_WARN) -O2 $(COMMON_DEFS) $(COMMON_SRCS) compare_strategy.cpp -o $@
```

**`-DALL_ELEMENTS_ORG` は付けない** (付けると rnd が org に化ける)。
`-DCFR` も `-DBEST_RESPONSE` も付けない (`win` と同じ)。
`all:` と `main:` の行には**足さない** (`all` は `watch` で止まるため)。

### グローバル変数

`infset_iswin.cpp:23-29` と同じものを定義する。`rnd_make_infset.hpp` と
`loveletter.hpp` が `extern` で参照するため、**1つでも欠けるとリンクが通らない**。

```cpp
bool br_switch = false;
int br_player = 0;
bool org_switch = false;
map<std::string, infset> table_infset{};
unsigned long int p1_points = 0;
unsigned long int p2_points = 0;
unsigned long int rand_points = 0;
unsigned long int end_points = 0;
```

include の並びも `infset_iswin.cpp:1-19` に合わせる。ただし
`belief_state_lose.hpp` は要らない (判定を呼ばないため)。
`belief_state_history.hpp` は要る (`bs.is_wiz_choice` を見るため)。

### 情報集合表の構築

`infset_iswin.cpp:152-163` と同じ手順。

**並び順は構築の呼び出し順ではない。** `cfr.cpp:85` も comp も
`table_infset` を `std::map` として先頭から舐めるので、`str` ファイルの
並びは**履歴文字列の昇順**であり、どの順に `do_action` したかには依存しない。

危ないのは順序ではなく**集合が違うこと**である。`table_infset` に入る情報集合は
枝刈りのマクロで変わるので、`str` を書いた `cfr` と comp の
`-DUSE_SAME_MOVE` `-DUSE_GOOD_MOVE` `-DNUSE_BAD_MOVE` が**揃っていないと
件数がずれる**。Makefile の `$(COMMON_DEFS)` をそのまま使うこと。
ずれた場合は起動時のサイズ検査が弾く。

```cpp
  node n_rnd_ds(open);
  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n_rnd_ds.deck[i - 1] == 0) continue;
    work_do_action ds_w;
    n_rnd_ds.do_action(1, i, ds_w);
    rnd_ds_put_hide_card(n_rnd_ds);
    n_rnd_ds.undo_action(1, i, ds_w);
  }
```

### σ̄ の読み込み

```cpp
  // table_infset と同じ順に読む。cfr.cpp:85-97 と同じ形式。
  std::map<std::string, double> sigma0; // 履歴 -> σ̄(行動0)
  for(auto it = table_infset.begin(); it != table_infset.end(); ++it) {
    float sum_i0, sum_i1, regret0, regret1;
    input.read((char *)&sum_i0, sizeof(float));
    input.read((char *)&sum_i1, sizeof(float));
    input.read((char *)&regret0, sizeof(float));
    input.read((char *)&regret1, sizeof(float));
    // cfr_exp_reward.hpp:140-141 と同じ。分母が 0 なら 0.5。
    double p = (sum_i1 == 0.0f) ? 0.5 : (double)sum_i0 / (double)sum_i1;
    sigma0.emplace(it->first, p);
  }
```

`regret0` / `regret1` は読み飛ばすだけで使わない (読まないとずれるので
読むこと)。

### 行の読み込み

```cpp
  std::vector<winlose_record> win_rows, lose_rows;
  load_bin_winlose("wininf" + subgame + ".bin", win_rows);
  load_bin_winlose("loseinf" + subgame + ".bin", lose_rows);
```

`load_bin_winlose` は `abs/` を前置する (`save_load_winlose.hpp`)。
**`win` と同じ作業ディレクトリから走らせること**を `--help` 相当の
使い方メッセージに書く。

---

## 5. 出力

すべて標準出力。終了コードは常に 0。

```
comp 4 4 6
  str44664.bin          (6,298,972 情報集合)
  abs/wininf446.bin     (2,025,016 行)
  abs/loseinf446.bin    (   ...    行)
  EPS = 1e-06   例示件数 N = 10

--- σ̄ の分布 (記録のある行のスロット側) ---
必勝の行:  σ̄ = 0 : ...  <1e-6 : ...  <1e-4 : ...  <1e-2 : ...  <0.5 : ...  >=0.5 : ...
必敗の行:  (同じバケット)

--- 方向1: 記録と σ̄ が食い違う ---
必勝なのに σ̄ < EPS      : <件数>
  <履歴> slot=<0/1> choice=<0/1> σ̄=<値>     (先頭 10 件)
必敗なのに σ̄ > 1 - EPS  : <件数>
  (同上)

--- 方向2: σ̄ が偏っているのに記録が無い ---
記録の無い情報集合            : <件数>
  うち σ̄ < EPS または > 1-EPS : <件数>
  <履歴> σ̄=<値>                (先頭 10 件)
```

分布のバケットは `0 / <1e-6 / <1e-4 / <1e-2 / <0.5 / >=0.5` の6つ。
必勝の行は「そのスロットの σ̄」、必敗の行も「そのスロットの σ̄」を入れる
(必勝は高いほど、必敗は低いほど期待どおり)。

履歴はそのままでは読めないので `get_actions_history(history, true)`
(`belief_state_history.hpp` が宣言、`log_util` 相当) で人が読める形にして出す。
**`get_actions_history` が使えない場合は、生のバイト列を16進で出す**
(履歴は `unsigned char` 列なのでそのままでは端末に出せない)。

---

## 6. `CLAUDE.md` への追記

`belief_state` の規約の「魔術師の対象の 0/1 は、層によって基準が違う」に、
**CFR 側の並び**を足す。

- `cfr_exp_reward.hpp` の魔術師ノードは `reward1 * p + reward2 * (1 - p)` で、
  `reward1` が `do_action(6)` = 相手、`reward2` が `do_action(7)` = 自分。
  つまり **CFR の行動0 = 相手、行動1 = 自分**で、`visit_winlose.hpp` の
  `wf[]` と同じ並び、`winlose_record.slot` (0 = 自分) とは逆。
- `str<部分ゲーム><反復>.bin` と `wininf` / `loseinf` を突き合わせるときは
  対象選択ノードだけスロットを反転させること。

---

## 検証

### ビルド

```
make comp
make cfr cfr0 cfrorg cfrorgcnt brrnd brorg win
make cppcheck
git add -u && git clang-format
```

`comp` が新しい警告なしで通ること。**既存6ターゲットも引き続き通ること**
(Makefile を触るため)。cppcheck 無指摘、整形差分なし。

### 既存の出力が動いていないこと

```
./regress.sh check
```

**12 項目すべて `OK`**。`compare_strategy.cpp` は新規ファイルで、既存の
ヘッダには手を入れないため。**1つでも `DIFFER` したら、既存コードに
手が入っている。**

### 実行

```
./comp 4 4 6
./comp 5 5 7
```

`abs/wininf446.bin` `abs/loseinf446.bin` `str44664.bin` が揃っている 446 と、
`abs/wininf557.bin` `abs/loseinf557.bin` `str55764.bin` が揃っている 557 で走る。

**確かめること**:

- 情報集合数がファイルサイズ検査を通る (446 で 6,298,972、557 で 3,496,616)
- 方向1・方向2の件数が出る。**0 件でも異常ではない** (判定と CFR が一致して
  いるということなので、それが期待でもある)
- わざと違う部分ゲームの str を読ませるとサイズ検査で弾かれる
  (`./comp 5 5 7` のときに `str44664.bin` を置くなど、手で1回試す)

### 出力の妥当性を1件だけ手で追う

方向1で件数が出たら、**先頭の1件について履歴から `belief_state` を組み直し、
必勝・必敗判定を呼び直して、行の内容と一致することを確かめる**。
一致しなければ comp 側のバグ (スロットの取り違えが最有力)。

---

## やらないこと

- 既存のヘッダ・`.cpp` の変更 (`CLAUDE.md` と `Makefile` を除く)
- 終了コードで失敗を表すこと
- 反復回数を引数で受けること (64 固定)
- 情報集合の除外 (訪問回数が少ないもの、手札2枚が同じカード、対象選択ノードの
  いずれも除外しない)
- `action_code.hpp` の `get_action` に範囲検査を足すこと。`comp` はこの関数を
  使わないので、CLAUDE.md の「やり残し」のまま別途扱う
- ADR の新設 (形が固まってから)
