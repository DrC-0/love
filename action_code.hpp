#ifndef ACTION_CODE_HPP
#define ACTION_CODE_HPP

// rnd_action.hpp / org_action.hpp (gperf生成物) は std::cout / std::endl を
// 使うが <iostream> を自分では include していない。今までは include 元の
// .cpp が先に <iostream> を include していたので気づかれなかった。
#include <iostream>
#include "rnd_action.hpp"
#include "org_action.hpp"

// 行動 <-> unsigned char 1文字 の変換。定義は action_code.cpp に1つだけある。
char action_to_char(int action, int card);
int char_to_action(char c);
char wizard_to_char(int to, int trash, int draw);
int char_to_wizard(char c);
char twonum_to_char(int card1, int card2);
int char_to_twonum(char c);

// gperf の完全ハッシュ表。どちらも非静的データメンバを持たないステートレスな
// 関数オブジェクトなので、プログラム全体で1インスタンスにしてよい。
inline Rnd_Perfect_Hash rph;
inline Org_Perfect_Hash oph;

#endif
