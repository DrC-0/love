# 共有宣言はヘッダに置き、定義は各プログラムに残す

各 `.cpp` が `extern int char_to_action(char c);` のような宣言を手書きでコピーして
いたのをやめ、`action_code.hpp` (行動 ↔ char の変換と `rph` / `oph`)、`run_mode.hpp`
(`cfr_switch` などの切替)、`analysis_points.hpp` (統計カウンタ)、`card_label.hpp`
(表示用の定数表) に集約する。手書きの `extern` は宣言と定義がずれても誰も気づかない。
実際 `cfrorg` は `rph` を、`win` は `oph` を定義しておらず、`-O2` が定数畳み込みで
参照を消すおかげで**たまたま**リンクが通っていただけで、どちらも `-O0` では
`undefined reference` で落ちていた。

一方、**定義は各 main の `.cpp` に現状の初期値のまま残す**。`org_switch` は cfrorg
だけ `true`、`br.cpp` は実行時に書き換え、`cfr.cpp` は `cfr_switch` を 0/1 で
切り替えるというように、これらの初期値はプログラムごとに違い、`loveletter.cpp` の
`do_action` を実行時に分岐させる。共通の定義に寄せると取り違えたときに出力が静かに
変わるうえ、得られるのは重複行の削減だけで割に合わない。宣言を共有した時点で
手書き `extern` の実害は消える。
