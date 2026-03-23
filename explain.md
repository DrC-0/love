# ラブレターCFR関連ファイルの依存関係

このドキュメントでは、CFRと最適反応戦略の計算に用いる各ファイルの依存関係をMermaid記法で視覚化しています。シェルスクリプト・C++ファイル・ヘッダファイル間の関係を明示し、全体の構造を理解しやすくします。

```mermaid
graph TD

%% シェルスクリプト
%%cfrbrh.sh --> cfrbr.sh
%%cfrbrh.sh --> cfr_zero.cpp
cfrbrh.sh --> cfr.cpp
%%cfrbrh.sh --> br_rnd.cpp
%%cfrbrh.sh --> br.cpp

%% cfrbr.sh 依存関係（内部でcfrやbr系を使う）
cfrbr.sh --> cfr_zero.cpp
cfrbr.sh --> cfr.cpp
%%cfrbr.sh --> br_rnd.cpp
%%cfrbr.sh --> br.cpp

%% cfr_zero
cfr_zero.cpp --> loveletter.hpp
cfr_zero.cpp --> loveletter.cpp
cfr_zero.cpp --> cfr_exp_reward.hpp
cfr_zero.cpp --> rnd_make_infset.hpp
cfr_zero.cpp --> cfr.hpp

%% cfr
cfr.cpp --> cfr.hpp
cfr.cpp --> cfr_exp_reward.hpp
cfr.cpp --> rnd_make_infset.hpp
cfr.cpp --> loveletter.hpp
cfr.cpp --> loveletter.cpp

%% br_rnd
%%br_rnd.cpp --> infset_dfs_rnd.hpp
%%br_rnd.cpp --> all_elements_r_
