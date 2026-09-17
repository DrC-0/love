# 設問: 必敗判定の戻り値を型にし、終端判定を1本にまとめる

計画書 `.claude/plans/lose-decision.md` だけを読んで答えること。
分からないものは推測せず「不明」と書くこと。

---

**Q1.** この変更は org 側と rnd 側のどちらを触るか。触らない側を触らなくてよい
理由は何か。対で存在する関数のうち、同時に直す必要があるものはあるか。

**Q2.** `lose_decision` のスロットは 2 つしかない。なぜ `win_decision` のように
8 つ要らないのか。スロット 0 と 1 はそれぞれ何を指すか、意思決定点の種類ごとに
答えよ。

**Q3.** 手札が {騎士(3), 騎士(3)} で、騎士を出すと必敗だとする。
`is_lose` はどのスロットを立てるか。`count()` はいくつになるか。
`action_count(bs)` はいくつか。`able_act` はいくつになるか。
**今のコードでは `able_act` はいくつだったか。**

**Q4.** `check_terminal` が `uncertain` を返すのはどういう局面か。
必勝側 (`enemy_turn_win_uncached` / `draw_win_uncached`) は `uncertain` を
受け取ったとき何を返すか。それが今と同じ値になる理由を説明せよ。

**Q5.** `infset_iswin.cpp` で「ルール上すでに敗北」を判定するのはどの関数の
どの戻り値か。そのとき `able_act` と `only_history` と
`output_actions_history` はそれぞれどうなるか。

**Q6.** 今の `pushed[2]` は何のためにあったか。新しい形でそれが要らなくなる
理由を、`is_lose` の側と `infset_iswin.cpp` の側の両方から説明せよ。

**Q7.** `able_actions_wizard` の第2引数 `to_self` は、今の `able_actions` の
`card` のどの値に対応するか。`is_lose` のスロット番号とはどう対応するか。
「自分を対象とする場合」に入る条件を、今のコードと新しいコードの両方の式で
書け。

**Q8.** `./regress.sh check` でどの項目が `OK` で、どの項目が `DIFFER` して
よいか。`DIFFER` してはいけないのにした場合、どうするか。速度について測る
ものと、報告が必要になる基準は何か。
