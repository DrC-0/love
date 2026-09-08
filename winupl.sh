#!/bin/bash
mkdir -p logs

# 全体の処理（計算＋集計）をまとめてバックグラウンドで実行
(
    # ターミナルを閉じても処理が中断されないようにする（nohupと同等の効果）
    trap '' HUP

    # 1. win.sh の12並列処理が完了するまで待つ
    ./win.sh --foreground

    # 2. 上記のxargs（12並列計算）がすべて完了したら、順次集計スクリプトを実行

    ./count_wl_depinf.sh
    ./upl.sh

) > logs/winupl.log 2>&1 &
