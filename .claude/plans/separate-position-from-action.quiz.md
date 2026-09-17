# 設問: 位置レベルの判定を use_win の中から追い出す

計画書 `.claude/plans/separate-position-from-action.md` だけを読んで答えること。
分からないものは推測せず「不明」と書くこと。

---

**Q1.** この変更は org 側と rnd 側のどちらを触るか。触らない側があるなら、
そのファイル名と、触らなくてよい理由を書け。

**Q2.** `use_win_impl` から具体的に何行を消すか。消したあと `CW_BUMP(use_win);`
はどうなるか。`use_win_impl` の本体（`card == Card{8}` の判定など）は変えるか。

**Q3.** `win_actions(bs, c0, c1)` は `is_terminated_win(bs)` が次を返したとき
それぞれ何を返すか。`c0` はカード 3、`c1` はカード 8 とする。

- `{-1, 0}`
- `{0, 0}`
- `{3, 1}`
- `{5, 1}`

**Q4.** `draw_win_impl` の OR ノードでは、なぜ `win_actions` を使わずに
`is_terminated_win` を直接呼ぶのか。そこで `or_first` と `or_second` に何を入れるか。

**Q5.** `enemy_turn_win_impl` と `draw_win_impl` の入口にも同じ短絡がある。
これらは外すか、残すか。そう判断した理由は何か。`sol_win_impl` と `wiz_win_impl`
はどうか。

**Q6.** `./regress.sh check` を実行したとき、どの項目が `OK` で、どの項目が
`DIFFER` になることを期待するか。項目数も書け。`cfrorg-557-6-stdout` が
`DIFFER` になったらどう判断するか。`cfrorg-557-6-counters` が `OK` だったら
どう判断するか。

**Q7.** `visit_winlose.hpp` で変更するのは何行か。`win_points` や `lose_points`
を数えている部分は変えるか。変えないなら、統計が動かない理由を
`is_terminated_win` が `{3, 1}` を返す場合について具体的に説明せよ。

**Q8.** `win_actions_mismatch` はどういうときに呼ばれるか。なぜ `assert` では
いけないのか。呼ばずに素通りさせると何が起きるか。
