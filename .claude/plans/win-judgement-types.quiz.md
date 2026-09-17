# 設問: 必勝判定の戻り値を型にし、入口を再帰に組み込む

計画書 `.claude/plans/win-judgement-types.md` だけを読んで答えること。
分からないものは推測せず「不明」と書くこと。

---

**Q1.** `is_win_uncached` の兵士の宣言の分岐で、`soldier_win(bs, Card{5})` が
`{holds = true, turns = 2}` を返したとする。`win_decision` のどのビットが立ち、
`turns` のどの要素にいくつが入るか。行動番号の決め方も書け。

**Q2.** `win_decision` に深さを入れるとき、`+1` するのはどの分岐か。しないのは
どの分岐か。しない分岐では、なぜ `+1` が要らないのか。

**Q3.** `visit_winlose.hpp:155-156` の `wc.wiz_win(bs, 0)` と `wc.wiz_win(bs, 1)`
は、それぞれ `wizard_target` のどちらに置き換わるか。`is_lose` の 0/1 とはどういう
関係になるか。この変更で順序を揃えるか、揃えないか。

**Q4.** `use_win_uncached` から何行を消すか。消したあと、終端局面で
`use_win(bs, Card{8})` は何を返すようになるか。`CW_BUMP(use_win);` はどうするか。

**Q5.** `draw_win_uncached` の OR ノードを `is_win` に置き換えたとき、`or_second`
には何を入れるか。終端局面のときに今と同じ値になる理由を説明せよ。`next_bs` が
`is_win` の選択ノードの分岐に入る心配が無いのはなぜか。

**Q6.** `./regress.sh check` でどの項目が `OK`、どの項目が `DIFFER` になることを
期待するか。`DIFFER` になる項目について、それが想定どおりかをどうやって確かめるか。
具体的なコマンドを書け。

**Q7.** `is_win` のメモ表の鍵はどう作るか。`extra` に何を渡すか。それが
`use_win` の鍵と衝突しないのはなぜか。

**Q8.** 次のうち、この計画で直すものと直さないものを分けよ。直さないものは
なぜ直さないのか。

- 姫(8) を持っているともう片方のカードの必敗判定が行われない
- `wizard_lose_uncached` が `{true, 0}` を返す箇所
- `is_lose` が `9` や `0`/`1` という多目的のコードを返すこと
- `enemy_turn_win_uncached` の入口にある終端判定の短絡
