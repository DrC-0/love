# 設問: fold-all-elements

計画書 `.claude/plans/fold-all-elements.md` を読んで答えること。
答えが計画書から読み取れない場合は、推測せず `不明` と書くこと。

---

**Q1.**
この変更は org 側・rnd 側のどちらを触るか。片方だけなら、もう片方を触らなくてよい
理由は何か。

---

**Q2.**
`infset_dfs.hpp` の中の `all_put_hide_card(his_p, 0, n);` という1行は、
`br.cpp` の翻訳単位と `compare_abscfr.cpp` の翻訳単位で、それぞれ何に解決されるか。
その切り替えは、変更前は何によって行われていたか。変更後は何によって行われるか。

---

**Q3.**
`infset_dfs.hpp` と `infset_dfs_rnd.hpp` は何行変更するか。
またその中の `all_put_hide_card(...)` の呼び出しを
`all_elements_walker<org_elements>::put_hide_card(...)` に書き換えてよいか。
理由も答えること。

---

**Q4.**
`br.cpp` は何行変更するか。`br.cpp:197` の
`all_put_hide_card(history_private, 0, n_all_test);` はどう扱うか。

---

**Q5.**
畳んだ後のコードには、`node` の履歴メンバを読む箇所が2種類ある。
一方はポリシー (`P::his` / `P::his_p`) を経由し、もう一方は経由しない。

(a) ポリシーを経由しないのはどちらの用途で、そこには具体的に何と書くか。
(b) そこをポリシー経由にすると何が起きるか。

---

**Q6.**
`Makefile` の3つのターゲット `brorg` / `brrnd` / `comp` について、
`-DALL_ELEMENTS_ORG` を付けるのはどれか。付け間違えると何が起きるか。
また、なぜ `br.cpp` の中に `#define` を書くのではなくコンパイル行で与えるのか。

---

**Q7.**
`double prob_win` の扱いは org と rnd で違う。
org 側ではどの関数の中で計算され、rnd 側ではどの関数の中で計算されるか。
それぞれ `w.randsol_wprob` への代入はどちらの関数で行われるか。

---

**Q8.**
この変更は行動履歴 (`unsigned char` 1文字＝1行動の文字列) に手を入れるか。
入れるなら、末尾への追記か、末尾1文字の payload 付き置換か。
入れないなら、その理由は何か。

---

**Q9.**
実装後に `grep -rn '\ball_put_hide_card\b' --include=*.cpp --include=*.hpp .` を
実行したら、何行かヒットした。これは実装が誤っている証拠か。理由も答えること。

---

**Q10.**
変更後に走らせるコマンドを挙げ、それぞれ期待される出力は何かを答えること。
`comp` はどの部分ゲームで走らせるか。走らせない部分ゲームがあるなら、
それはどれで、なぜ走らせないのか。

---

**Q11.**
`nm -C brorg | grep 'all_elements_walker<org_elements>'` の期待される結果は何か。
`nm -C comp` で同じことをしたときの期待される結果は何か。
期待と違ったとき、それぞれ何が起きていることを意味するか。

---

**総評**
計画書のとおりに実装するとして、自分がやらかしそうなことを挙げること。
