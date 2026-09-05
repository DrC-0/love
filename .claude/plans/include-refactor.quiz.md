# 設問: インクルード改修

計画書 `.claude/plans/include-refactor.md` を読んで、以下に答えてください。
分からないものは推測せず「不明」と書いてください。
最後に総評として「この計画書のまま実装したら、自分がやらかしそうなこと」を挙げてください。

## Q1

この変更は org 側・rnd 側のどちらを触りますか。
片方だけなら、もう片方を触らなくてよい理由は何ですか。

## Q2

このリポジトリには対で存在するファイル群があります。
そのうち、この改修で**両方に同じ処理を施す必要がある**のはどれとどれですか。
また、対に見えるが**片方しか触らない**ものがあれば、それはどれで、なぜ片方だけでよいのですか。

## Q3

この改修は行動履歴 (`org_his_p` / `rnd_his_p` が持つ文字列) に手を入れますか。
入れる場合、末尾への追記ですか、末尾1文字の payload 付きへの置換ですか。

## Q4

`org_switch` の初期値は、`cfr_org.cpp` と `br.cpp` と `cfr.cpp` でそれぞれ何ですか。
これらの定義を新しいヘッダ 1 箇所にまとめてよいですか。よくないなら、その理由は何ですか。

## Q5

`infset_iswin.cpp` は `cfr_switch` を定義していません。
`cfr_org.cpp` は `soldior_points` を定義していません。
新しいヘッダはどちらも `extern` 宣言します。
実装時、これらのファイルに定義を足すべきですか。足すべきでないなら、それでもリンクが通る理由は何ですか。

## Q6

`action_code.hpp` は `rnd_action.hpp` と `org_action.hpp` を include します。
一方、`cfr.cpp` などの多くの `.cpp` はもともと `rnd_action.hpp` / `org_action.hpp` を
直接 include しています。両方が有効なまま二重インクルードになりますが、
これが問題にならないのはなぜですか。根拠となるファイルと行を挙げてください。

## Q7

Makefile の `cfrorg` ターゲットは現在こう書かれています。

```make
cfrorg: cfr_org.cpp org_tree.hpp visit_winlose.hpp bf_position.hpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 $(COMMON_WARN) -O2 $(COMMON_DEFS) -DCFR $(COMMON_SRCS) bf_position.hpp cfr_org.cpp -o $@
```

改修後はどうなりますか。`bf_position.hpp` という文字列は 2 箇所に現れますが、
それぞれ残しますか消しますか。

## Q8

検証手順に `-O0` でのビルドが含まれています。
改修**前**に `-O0` で `cfrorg` と `win` をビルドすると何が起きますか。
また、`all_elements.hpp` にインクルードガードを足すとき、
このファイルのどんな特徴に注意する必要がありますか。
