# Belief State を層に分割し、bf_position から改名する

`bf_position.hpp` は 1489 行あり、型の定義・履歴文字列からの復元・必勝判定・
必敗判定・表示が 1 ファイルに同居していた。内部の呼び出し関係を全部辿ったところ
依存は一方向の層になっており、機械的に分割できると分かったので分割した。

あわせて `bf_position` という名前を `belief_state` に改めた。この型は
「ありえる物理状態の集合」であって「位置 (position)」ではない。`bf` が何の略かは
コード中のどこにも書かれておらず、`bs_set.hpp` (belief state set) のほうが
正しい名前を使っていた。語彙は `CONTEXT.md` を参照。

## 層の構造

```
belief_state_lose.hpp   必敗判定3本                     → belief_state_win.hpp
belief_state_win.hpp    必勝判定7本 + able_actions 他   → belief_state.hpp
belief_state_history.hpp 履歴コンストラクタと符号化・復号 → belief_state.hpp
belief_state.hpp        型・アクセサ・フラグ更新・遷移・表示
```

`first_hand_open.hpp` が開手ラブレターの述語 3 本を持ち、
`belief_state_history.hpp` に依存する。

### 必勝判定は分割できない

`is_win` / `is_terminated_win` / `use_win` / `enemy_turn_win` / `draw_win` /
`sol_win` / `wiz_win` の 7 本は相互再帰の強連結成分
(`use_win` ↔ `enemy_turn_win` ↔ `draw_win` ↔ `use_win`、`sol_win` と `wiz_win` は
`enemy_turn_win` を呼ぶ) で、1 つの探索アルゴリズムであって 7 つの独立した関数では
ない。これを別ファイルに割ると前方宣言だらけになって悪化する。

### 必敗 → 必勝は一方向

`use_lose` と `wiz_lose` は `draw_win` を呼ぶが、**必勝側は必敗側を一度も
呼ばない**。だから必敗判定は必勝判定の上に乗る薄い層として切り出せた。

### 表示は型と同じファイルに置く

`print` / `exit_with_print` / `validate_hand_e_candidate` は
`belief_state.hpp` に置いた。`belief_state_history.hpp` に置くと
`validate_hand_e_candidate` → `exit_with_print` → `print` の経路で
「型 ⇄ 履歴」の循環ができる。`print` は型のアクセサしか使わず、
フラグを 1 つ足したら `print` も直すという意味で型と一緒に変わる。

## 生配列を取る自由関数のうち 3 本は残す

生配列 (`const int hand[2], const int trash[8], ...`) を取る自由関数が 8 本あった。
うち 5 本 (`open_e` / `deck` / `in` / `other` / `count_deck`) は対応するメンバ関数が
全部自前で実装しており、ヘッダの内外どちらからも呼ばれていなかったので削除した。

残る 3 本 (`trash_and_hand_s` / `deck_or_hand_e` / `hand_e`) は**消せない**。

```cpp
bool belief_state::hand_s_est(int i) const {
  int enemy_hand[2] = {open_e(), 0};
  return ::hand_e(i, enemy_hand, trash, open_flag_s, sol_flag_s, lt5_flag_s, not7_flag_s);
}
```

`hand_s_est` は `hand_s` ではなく**その場で組んだ `enemy_hand` を渡し、`_e` では
なく `_s` 側のフラグを使う**ことで、「相手から見て自分の手札に card i がありうるか」
を役割を入れ替えて計算している。メンバ関数の形 (暗黙の `this` から `hand_s` と
`_e` フラグを読む) では書けないので、生配列を取る汎用形が必要。

## `bs_set.hpp` の `State` とは統合しない

`bs_set.hpp` の `struct State` は Belief State の 2 つ目の実装で、フィールドも
メンバ関数の名前もほぼ同じ。それでも統合しない。

`State` は `hand[2]` を `[min, max]` に**正規化**し、`operator<=>` と 8.7 億状態の
完全ハッシュを持つ (終盤の表引き用)。`belief_state` の `hand_s[2]` は
`hand_s[0]` が場に出す側で**順序が意味を持つ** (木探索用)。無理に共通の型へ寄せると、
正規化された型とされていない型が同じ名前になり、**2 実装あることが目に見えている
今より危険**になる。`State` 側の完全ハッシュは開手ラブレターの開発で使う予定。

## umbrella を経由した

分割のコミットでは `bf_position.hpp` を「4 つを include するだけ」の互換
umbrella として残し、**利用者側の `#include` を 1 行も変えなかった**。こうすると
回帰ハーネスに差が出たときに原因が分割のミスに限定できる。改名のコミットで
umbrella を撤去し、利用者 4 ファイルの include を必要な層へ張り替えた。

内容が移っただけであることは、元ファイルと新 5 ファイルを行単位で突き合わせて
確認した。**新ファイルに元ファイルへ無い行は 1 行も無く**、消えたのは削除した
5 本の 37 行と死んだコメント 1 行だけだった。

## 既定引数は定義側にある

`belief_state(int open[3], std::string history, bool rnd)` の既定引数 `= true` は
**宣言側ではなく定義側**に書かれている。`get_actions_history` と
`output_actions_history` も同じ形。分割でこの 2 つが別ファイルに分かれたので、
**2 引数で呼ぶコードは `belief_state_history.hpp` を include しないとコンパイル
できない**。`infset_iswin.cpp` の 2 箇所と `watch_cfr.cpp` の 1 箇所が該当する
(`visit_winlose.hpp` は第 3 引数を明示しているので該当しない)。
宣言側に既定引数を足して「揃える」と、同じ既定引数の二重宣言でコンパイルが通らない。

## 決定的カウンタの名前を変えたので基準を取り直した

`count_work.hpp` のフィールド `bfp_from_history` は名前がそのまま標準エラーに
出力され、回帰ハーネスがその行を比較している。改名すると 12 項目のうちカウンタ
4 項目が必ず `DIFFER` になる。

**名前以外に差が無いことを確認してから**基準を取り直した。確認は、新しい出力の
`belief_state_from_history` を旧名に置換し戻して基準とバイト比較する方法で行い、
4 ファイルとも完全一致した (数値は 1 つも動いていない)。

この「基準を取り直す」は例外的な操作で、常用してはいけない。ハーネスの価値は
「diff が空であること」に全部乗っており、「たぶん名前だけだから」で取り直す前例を
作ると本物のバグを通す。取り直すときは必ず、差が意図したものだけであることを
機械的に示してから行う。

## `test.cpp` は壊れたままにした

`test.cpp` は gitignore 済みの手動デバッグ用スクラッチで、`main` を書き換えて使う
運用になっている。改名の対象から外したので `#include "bf_position.hpp"` が残り、
`make test` は通らない。使うときに
`sed -i 's/bf_position/belief_state/g' test.cpp` を 1 回走らせれば直る。
こちらから触らないのは、作業中の内容を壊しうるため。
