# all_elements.hpp / all_elements_rnd.hpp を1本に畳む

## 目的

同じ木を歩く展開器が org / rnd の2ファイルに複製されている。これを
`template <class P>` のポリシー方式で1本にする。ADR 0002 が選んだ
「関数テンプレート＋ポリシー型」の方式をそのまま適用する。

- `all_elements.hpp` (org) 573行
- `all_elements_rnd.hpp` (rnd) 526行

## この変更が触るのは org と rnd の両方

両方を1つのテンプレートに畳むので、両側を同時に触る。片方だけ直すことはできない。

## 前提: 現状の呼ばれ方（調査済み・最重要）

`all_*` には**直接の呼び出し**と、`infset_dfs*.hpp` の `all_exp_reward` を
経由する**間接の呼び出し**の2系統がある。間接の方が本命。

| ファイル | include | 直接呼ぶか | 間接 (`all_exp_reward` 経由) |
|---|---|---|---|
| `compare_abscfr.cpp` (comp) | `all_elements_rnd.hpp` + `infset_dfs.hpp` | 呼ぶ (169行) | 呼ぶ (177行が `all_exp_reward`) |
| `br.cpp` (brorg) | `all_elements.hpp` + `infset_dfs.hpp` | 呼ばない (197行は `#ifdef ALL_ELEMENTS_TEST` の中) | **呼ぶ** |
| `br_rnd.cpp` (brrnd) | `all_elements_rnd.hpp` + `infset_dfs_rnd.hpp` | 呼ばない | 呼ぶ |

`br.cpp` の間接経路は `brorg` の実行時に実際に通る:

```
br.cpp:280  infset_dfs_put_hide_card
   → infset_dfs_draw / infset_dfs_play        (infset_dfs.hpp:141,189,286)
   → all_exp_reward                           (infset_dfs.hpp:480)
   → all_put_hide_card / all_soldior / all_play / all_wizard /
     all_wizard_self / all_draw               (infset_dfs.hpp:491〜687)
```

`objdump -d -C brorg` で `infset_dfs_draw -> all_exp_reward` と
`all_exp_reward -> all_put_hide_card` の呼び出しが実在することを確認済み。

→ **org 側も rnd 側も生きたコードである。**

### この構造が課す制約（設計の要）

`infset_dfs.hpp` は `br.cpp` と `compare_abscfr.cpp` の**両方**から include され、
その中の `all_put_hide_card(his_p, 0, n);` という**同じ1行**が、翻訳単位によって
別の関数に解決される。

- `br.cpp` の翻訳単位では → `all_elements.hpp` (org) が定義したもの
- `compare_abscfr.cpp` の翻訳単位では → `all_elements_rnd.hpp` (rnd) が定義したもの

切り替えは「どちらのヘッダを include したか」で**暗黙に**行われている。

したがって:

- `infset_dfs*.hpp` の呼び出しを `all_elements_walker<org_elements>::...` のように
  直書きすると、`br.cpp` と `compare_abscfr.cpp` のどちらかが必ず壊れる。
- `infset_dfs*.hpp` も同時にテンプレート化すれば解けるが、883行+759行あり
  今回の範囲外。
- **よって、旧シグネチャの自由関数を残し、その中身を翻訳単位ごとに選ぶ。**
  詳細は後述の「自由関数ラッパー」。

### `br.cpp` の `#ifdef ALL_ELEMENTS_TEST` について

`ALL_ELEMENTS_TEST` を定義する Makefile ターゲットは存在せず、
`br.cpp:181-218` のブロックは現在**コンパイルが通らない**
(`action_sequense` は `rnd_action_sequense` / `org_action_sequense` に改名済み、
`output_hash_history` は `bool rnd` 引数が増えている)。

**この計画では `br.cpp` を1行も編集しない。** 自由関数ラッパーを残すので
197行の `all_put_hide_card(history_private, 0, n_all_test);` もそのまま通る。
186行・206行・211行の腐りも**壊れたまま残す**。腐りの修復は別の変更として扱う。

## 設計

### 新しい `all_elements.hpp` の構造

```cpp
#ifndef ALL_ELEMENTS_HPP
#define ALL_ELEMENTS_HPP

#include "loveletter.hpp"
#include "run_mode.hpp"
#include "action_code.hpp"   // rnd_action.hpp と org_action.hpp の両方を含み、
                             // rph / oph を inline グローバルとして宣言している

// br.cpp と compare_abscfr.cpp が定義している統計。org 分岐から参照する。
// br_rnd.cpp は定義していないが、if constexpr で捨てられる側でも非依存名の
// 名前解決は起きるので、宣言だけは見えている必要がある。捨てられた分岐は
// コード生成されないので、定義が無くてもリンクは通る。
extern double soldior_prob[8];

struct org_elements {
  static Org_Perfect_Hash &ph() { return oph; }   // get_action(int) が非 const なので const を付けない
  static constexpr int max_hash_value = ORG_MAX_HASH_VALUE;
  static std::string his(const node &n) { return n.org_his.get_hash_value(); }
  static std::string his_p(const node &n, int turn) { return n.org_his_p[turn].get_hash_value(); }
  static constexpr bool soldior_is_decision = true;
};

struct rnd_elements {
  static Rnd_Perfect_Hash &ph() { return rph; }   // 同上
  static constexpr int max_hash_value = RND_MAX_HASH_VALUE;
  static std::string his(const node &n) { return n.rnd_his.get_hash_value(); }
  static std::string his_p(const node &n, int turn) { return n.rnd_his_p[turn].get_hash_value(); }
  static constexpr bool soldior_is_decision = false;
};

template <class P>
struct all_elements_walker {
  // 8本すべてクラス内で定義する。クラス内定義なら宣言順に関係なく
  // 相互に呼べるので、元ファイルにあった前方宣言8本は不要になる。
  static void put_hide_card(string &h, unsigned long int head, node &n) { ... }
  static void draw_p1_init(string &h, unsigned long int head, node &n) { ... }
  static void draw_p2_init(string &h, unsigned long int head, node &n) { ... }
  static void draw(string &h, unsigned long int head, node &n) { ... }
  static void play(string &h, unsigned long int head, node &n, int c) { ... }
  static void wizard(string &h, unsigned long int head, node &n) { ... }
  static void wizard_self(string &h, unsigned long int head, node &n) { ... }
  static void soldior(string &h, unsigned long int head, node &n) { ... }
};

// --- 自由関数ラッパー ---
// infset_dfs.hpp / infset_dfs_rnd.hpp は all_exp_reward の中からこれらを
// 無修飾の自由関数として呼ぶ。どちらのモデルに解決されるかは、これまで
// 「all_elements.hpp と all_elements_rnd.hpp のどちらを include したか」で
// 暗黙に決まっていた。畳んだ後はその切り替えを ALL_ELEMENTS_ORG で明示する。
// これがあるおかげで infset_dfs*.hpp と br.cpp は1行も変えずに済む。
#ifdef ALL_ELEMENTS_ORG
using all_walker = all_elements_walker<org_elements>;
#else
using all_walker = all_elements_walker<rnd_elements>;
#endif

inline void all_put_hide_card(string &h, unsigned long int head, node &n) { all_walker::put_hide_card(h, head, n); }
inline void all_draw_p1_init(string &h, unsigned long int head, node &n)  { all_walker::draw_p1_init(h, head, n); }
inline void all_draw_p2_init(string &h, unsigned long int head, node &n)  { all_walker::draw_p2_init(h, head, n); }
inline void all_draw(string &h, unsigned long int head, node &n)          { all_walker::draw(h, head, n); }
inline void all_play(string &h, unsigned long int head, node &n, int c)   { all_walker::play(h, head, n, c); }
inline void all_wizard(string &h, unsigned long int head, node &n)        { all_walker::wizard(h, head, n); }
inline void all_wizard_self(string &h, unsigned long int head, node &n)   { all_walker::wizard_self(h, head, n); }
inline void all_soldior(string &h, unsigned long int head, node &n)       { all_walker::soldior(h, head, n); }

#endif
```

`infset_dfs*.hpp` が実際に呼ぶのは
`all_put_hide_card` `all_draw` `all_play` `all_wizard` `all_wizard_self` `all_soldior`
の6本だが、**8本すべてラッパーを用意する**。対称性のため、および
`br.cpp:197` (`#ifdef` 内) が `all_put_hide_card` を呼んでいるため。

`all_elements_rnd.hpp` は**削除する**。

### 本文の作り方

`all_elements_rnd.hpp` (rnd 版) を土台にして、次の置換を機械的に行う。
`git mv all_elements_rnd.hpp` はしない。既存の `all_elements.hpp` を
新内容で上書きし、`all_elements_rnd.hpp` を `git rm` する。

| 元 (rnd 版) | 畳んだ後 |
|---|---|
| `rph.get_action(...)` (15箇所) | `P::ph().get_action(...)` |
| `RND_MAX_HASH_VALUE` (1箇所, `all_draw` 内) | `P::max_hash_value` |
| 関数名 `all_put_hide_card` → | `put_hide_card` (以下同様に `all_` を外す) |
| 内部の相互再帰呼び出し `all_draw(h, head, n)` など | `draw(h, head, n)` (同一クラススコープなので修飾不要) |

**例外が1箇所ある。** `play()` の `case 5` (魔術師) には `int draw = c2w % 10;`
というローカル変数があり、同名の静的メンバ関数 `draw` を隠す。この分岐の中の
呼び出しだけは `all_elements_walker::draw(h, head, n);` と修飾すること
(元の `all_elements_rnd.hpp:392` に当たる1箇所のみ。`case 8` の後にある
もう1つの `all_draw` はこのスコープの外なので修飾不要)。

### 履歴メンバの扱い（最重要・間違えやすい）

`node` は `rnd_his` / `rnd_his_p[2]` と `org_his` / `org_his_p[2]` を**常に両方持つ**
(`loveletter.hpp:167-170`)。使い分けは2種類あり、**片方はポリシーを通してはいけない**。

**(A) 木の履歴 — ポリシーを通す**

`h` (探索中の私的履歴) との照合と `all_history` への記録。
rnd 版の `all_elements_rnd.hpp:175,176,265,266` と
org 版の `all_elements.hpp:175,176,231,232,296,297`。

```cpp
  if(P::his_p(n, n.turn) == h) {
    all_history.emplace_back(P::his(n));
    return;
  }
```

**(B) `table_infset` の鍵 — 常に rnd 固定。ポリシーを通さない**

`all_elements_rnd.hpp:186,278` と `all_elements.hpp:186,309` の2箇所は、
**org 版でも `rnd_his_p` と書かれている**。`br.cpp` は org ビルドでも
`rnd_make_infset.hpp` を include して rnd の情報集合表を作るため。
つまり「ゲーム木の履歴は org、情報集合表の鍵は rnd」という使い分けが意図的にある。

畳んだ後も**そのまま `n.rnd_his_p[n.turn].get_hash_value()` と書く**:

```cpp
    w.infset_it = table_infset.find(n.rnd_his_p[n.turn].get_hash_value());
```

ここを `P::his_p(n, n.turn)` にすると org 側の挙動が変わる。**してはいけない。**

### 兵士 (case 1) の分岐 — org と rnd で本質的に違う

機械的置換で消えない差は75行あり、その大半がこれ。`if constexpr` で分ける。

**`play()` の `case 1`**

```cpp
  case 1:
    if(n.open2 > 1 && n.barrier2 == false) {
      return;
    }
    if constexpr(P::soldior_is_decision) {
      // org: all_elements.hpp:226-252 の中身をそのまま。
      // n.turn == br_player のとき宣言を意思決定として列挙し soldior() を呼ぶ。
      // rph → P::ph(), org_his* → P::his/P::his_p に置換すること。
      // all_soldior(h, head, n) → soldior(h, head, n)
    } else {
      // rnd: all_elements_rnd.hpp:226-243 の中身をそのまま。
      // prob_win を計算するだけ。宣言は一様ランダムとして扱う。
      // ここで計算した prob_win は play() の末尾で w.randsol_wprob に入る
      // (下記参照)。
    }
    break;
```

**`prob_win` の置き場所が org と rnd で違う**

- rnd: `all_play` の先頭で `double prob_win = 0.0;` を宣言し、`case 1` で計算し、
  関数末尾の `w.randsol_wprob = prob_win;` (2箇所:
  `all_elements_rnd.hpp:408,420`) で work に入れる。
- org: `all_play` に `prob_win` は無い。代わりに `all_soldior` の中で
  `n.turn != br_player` のときだけ計算し、`w.randsol_wprob = prob_win;`
  (`all_elements.hpp:556,565`) で入れる。

畳んだ後もこの違いを保つ。`play()` 側は

```cpp
    double prob_win = 0.0;   // org では常に 0.0 のまま使われない
    ...
      work_do_action w;
      if constexpr(!P::soldior_is_decision) w.randsol_wprob = prob_win;
```

`soldior()` 側は

```cpp
    double prob_win = 0.0;
    if constexpr(P::soldior_is_decision) {
      if(n.turn != br_player && n.hand2 > 1 && n.barrier2 == false) {
        ...  // all_elements.hpp:532-544 の中身
      }
    }
    ...
      work_do_action w;
      if constexpr(P::soldior_is_decision) w.randsol_wprob = prob_win;
```

`-Wunused-variable` を避けるため、`prob_win` は `if constexpr` の外で
宣言したうえで両側から参照される形にする (上の書き方なら、使わない側でも
`w.randsol_wprob` への代入が消えるだけで宣言は残るので警告は出ない。
出たら `[[maybe_unused]]` を付ける)。

## 変更するファイルと箇所

### 1. `all_elements.hpp` — 全面書き換え

上記の設計どおり。土台は `all_elements_rnd.hpp`。

### 2. `all_elements_rnd.hpp` — 削除

`git rm all_elements_rnd.hpp`

### 3. `compare_abscfr.cpp:53` — include の差し替えのみ

```diff
-#include "all_elements_rnd.hpp"
+#include "all_elements.hpp"
```

`ALL_ELEMENTS_ORG` は定義しないので、ラッパーは rnd に解決される。
**169行の `all_put_hide_card(key, 0, n);` は変更しない。** ラッパーがあるので
そのまま通り、かつ rnd に解決されるので挙動が変わらない。

### 4. `br_rnd.cpp:51` — include の差し替えのみ

```diff
-#include "all_elements_rnd.hpp"
+#include "all_elements.hpp"
```

`ALL_ELEMENTS_ORG` は定義しない。rnd に解決される。

### 5. `br.cpp` — **1行も変更しない**

`br.cpp:53` の `#include "all_elements.hpp"` はそのまま。
197行も `#ifdef ALL_ELEMENTS_TEST` 内の腐った行もそのまま。
org を選ぶのは Makefile の `-DALL_ELEMENTS_ORG` (次項)。

### 6. `infset_dfs.hpp` / `infset_dfs_rnd.hpp` — **1行も変更しない**

自由関数ラッパーがあるので無修飾呼び出しがそのまま通る。

### 7. `Makefile`

`brorg` に `-DALL_ELEMENTS_ORG` を足す。`#define` を `br.cpp` に書くのではなく
コンパイル行で与えるのは、include の順序に左右されないため
(`all_elements.hpp` にはインクルードガードがあるので、もし他のヘッダが先に
include していたら `#define` が効かない)。

```diff
 brorg: br.cpp all_elements.hpp infset_dfs.hpp $(COMMON_SRCS) $(COMMON_HDRS)
-	g++ -std=c++20 $(COMMON_WARN) -O2 $(COMMON_DEFS) -DBEST_RESPONSE $(COMMON_SRCS) br.cpp -o $@
+	g++ -std=c++20 $(COMMON_WARN) -O2 $(COMMON_DEFS) -DBEST_RESPONSE -DALL_ELEMENTS_ORG $(COMMON_SRCS) br.cpp -o $@
```

**`brrnd` と `comp` には付けない。** 付けると rnd が org に化けて挙動が変わる。

あわせて依存の漏れを埋める (既存の漏れ。このままだと `all_elements.hpp` を
編集しても `comp` が再ビルドされず、検証が空振りする)。

```diff
-brrnd: br_rnd.cpp $(COMMON_SRCS) $(COMMON_HDRS)
+brrnd: br_rnd.cpp all_elements.hpp infset_dfs_rnd.hpp $(COMMON_SRCS) $(COMMON_HDRS)
```

```diff
 comp: compare_abscfr.cpp save_load_abshistory.hpp \
+      all_elements.hpp infset_dfs.hpp \
       belief_state.hpp belief_state_history.hpp belief_state_win.hpp belief_state_lose.hpp \
       $(COMMON_SRCS) $(COMMON_HDRS)
```

## 触らないもの

- `infset_dfs.hpp` / `infset_dfs_rnd.hpp` (883/759行)。これも org/rnd の対だが、
  今回は触らない。自由関数ラッパーを残すので**1行も変える必要がない**。
  `comp` は `all_elements_rnd.hpp` (rnd) と `infset_dfs.hpp` (org) を
  **混ぜて**使っている (`all_exp_reward` を org 版から呼ぶ)。この混在は
  今回の変更で壊してはいけない。
- `br.cpp`。**1行も変える必要がない**。
- `rnd_make_infset.hpp` / `org_tree.hpp`
- 行動履歴のエンコード。**履歴を伸ばす処理は一切含まない**
  (この展開器は既存の履歴文字列 `h` を読むだけで、追記も末尾置換もしない)。
- `belief_state*` 一式。

## 検証

### 0. 機械的な検査 (ビルドの前に実行すること)

罠は目視ではなく grep で確かめる。**どれか1つでも期待どおりでなければ、
そこで止めてやり直すこと。**

```sh
# (1) table_infset の鍵はポリシーを通さず rnd 固定のまま、ちょうど2箇所
grep -c 'table_infset.find(n.rnd_his_p\[n.turn\].get_hash_value())' all_elements.hpp
#   期待: 2
grep -n 'table_infset.find(P::' all_elements.hpp
#   期待: 何も出ない

# (2) 自由関数ラッパーが8本あること
grep -c '^inline void all_' all_elements.hpp
#   期待: 8

# (3) ALL_ELEMENTS_ORG で切り替えていること
grep -n 'ALL_ELEMENTS_ORG' all_elements.hpp Makefile
#   期待: all_elements.hpp に #ifdef が1つ、Makefile の brorg の行に -D が1つ。
#         Makefile の brrnd / comp の行には出てはいけない

# (4) br.cpp と infset_dfs*.hpp を変えていないこと
git diff --stat br.cpp infset_dfs.hpp infset_dfs_rnd.hpp
#   期待: 何も出ない (差分ゼロ)

# (5) .cpp の差分は include の1行ずつだけ
git diff compare_abscfr.cpp br_rnd.cpp | grep '^[+-]' | grep -v '^[+-][+-]'
#   期待: all_elements_rnd.hpp が - で all_elements.hpp が + の、計4行だけ

# (6) 古いファイルが消えていること
ls all_elements_rnd.hpp
#   期待: No such file or directory

# (7) 旧名の自由関数が呼び出し側に残っていること (ラッパーがあるので残ってよい)
grep -rn '\ball_put_hide_card\b' --include=*.cpp --include=*.hpp .
#   期待: all_elements.hpp のラッパー定義、infset_dfs.hpp:491,687、
#         infset_dfs_rnd.hpp:458,620、compare_abscfr.cpp:169、br.cpp:197 が出る。
#         **これは正常。** 消してはいけない
```

### ビルド

```
make clear || true
make cfr cfr0 cfrorg brrnd brorg win comp
```

`make` は最新のターゲットを飛ばすので、**必ず一度消してから**ビルドすること。
新しい警告を出さないこと (既存の警告として `cfr_exp_reward.hpp` の
`unused variable 'sum_exist_card'` と `prob_win may be used uninitialized` が
master 時点で出るが、これは今回の対象外)。

### rnd 側の挙動 (これが本番)

**使えるのは `comp 4 4 6` だけ。`comp 5 5 7` は検証に使ってはいけない。**

`comp 5 5 7` は**実行するたびに出力が変わる**。実測で3回とも違い (30行 / 29行 / 39行)、
うち2回は `terminate called after throwing an instance of 'std::logic_error'
what(): basic_string::_M_construct null not valid` で abort した。
原因は `rnd_action.hpp` / `org_action.hpp` の

```cpp
  const char *get_action (int v){
    if(v > RND_MAX_HASH_VALUE) std::cout << v << " > RND_MAX_HASH_VALUE" << std::endl;
    assert(!(v > RND_MAX_HASH_VALUE));   // -DNDEBUG なので効かない
    return rnd_wordlist[v];              // 範囲外参照 (未定義動作)
  }
```

で、範囲外の添字で語表を読み、そこに在ったメモリを `std::string` に構築している。
実行ごとに違う値 (238/223/209、240/229/234/207、249/220 …) が出るのはそのため。
**これは既存の不具合で、この計画では直さない。別の変更として扱う。**

`comp 4 4 6` はこの経路に入らず、2回の実行がバイト単位で一致することを確認済み。

基準は `logs/regress/manual/comp-446.out` に取ってある (20行)。
`logs/` は gitignore 済みなのでコミットされない。経緯は同じディレクトリの
`README.txt` にある。

comp は CWD 相対で `str<部分ゲーム>64.bin` と `abs/abs<部分ゲーム>.bin` を読むので、
**リポジトリ直下では走らせず**、リンクを張った作業用ディレクトリで走らせる:

```sh
REPO=$(pwd)
W=$(mktemp -d /tmp/compcheck.XXXXXX); mkdir -p "$W/abs"
ln -s "$REPO/str44664.bin" "$REPO/str55764.bin" "$W/"
ln -s "$REPO/abs/abs446.bin" "$REPO/abs/abs557.bin" "$W/abs/"
( cd "$W" && "$REPO/comp" 4 4 6 > comp-446.out 2>&1 )
diff "$W/comp-446.out" "$REPO/logs/regress/manual/comp-446.out"
rm -rf "$W"
```

**期待される出力: 差分なし (20行が一致)。所要時間は約72秒。**

`comp 5 5 7` は走らせない (上記の理由)。

**もし念のためと思って走らせてしまい、異常終了したり出力が基準と食い違ったりしても、
それは実装が誤っている証拠にはならない。** 変更前から毎回違う結果になるので、
判断材料として使わずに捨てること。

### org 側

実行経路が無いので、コンパイルが通ることだけを確認する。加えて:

`brorg` は `-DALL_ELEMENTS_ORG` 付きでビルドされ、`all_exp_reward` 経由で
org 側のテンプレートが実体化される。したがって**コンパイルは org 側の本体まで
到達する** (以前の「実体化されないので構文検査できない」は誤り)。

実体化されたことは次で確かめる:

```
nm -C brorg | grep 'all_elements_walker<org_elements>'
```

**何本かシンボルが出ること。** 1本も出なければ `-DALL_ELEMENTS_ORG` が
効いておらず、rnd 版が org のつもりで使われている。その場合は止めて報告すること。

逆に comp 側:

```
nm -C comp | grep 'all_elements_walker<org_elements>'
```

**何も出ないこと。** 出たら `comp` に `-DALL_ELEMENTS_ORG` が漏れている。

実行による検証は `brorg` が10分以上かかり完走を確認できていないため、
この計画では行わない。`brorg` の完走時間は別途計測中。

### 巻き込み事故が無いこと

```
./regress.sh check full
```

cfrorg と win は触らないので **12項目すべて OK** であること。

## やってはいけないこと

- `table_infset.find(...)` の引数をポリシー経由にすること (上記 (B))
- `compare_abscfr.cpp:169` の呼び出しを `all_elements_walker<...>::` 形式に書き換えること
  (ラッパーのまま残す。書き換えるとしても org を渡してはいけない)
- `infset_dfs.hpp` / `infset_dfs_rnd.hpp` に手を入れること
- `br.cpp` に手を入れること (腐りの修復も含めて、別の変更として扱う)
- `brrnd` や `comp` のビルド行に `-DALL_ELEMENTS_ORG` を付けること
- リポジトリ直下で `comp` や `brorg` を走らせること
  (`brorg` は `utilities<部分ゲーム>.csv` に追記する)
