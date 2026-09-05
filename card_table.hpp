#ifndef CARD_TABLE_HPP
#define CARD_TABLE_HPP

// カードごとの枚数。配列の添字は card - 1。
inline constexpr int max_num[8] = {5, 2, 2, 2, 2, 1, 1, 1};

inline constexpr double table_sign[2] = {1.0, -1.0};
inline constexpr char action_sign[8] = {'0', 'a', 'c', 'd', 'e', 'f', 'g', 'h'};
inline constexpr char card_sign[8][20] = {"兵士", "道化", "騎士", "僧侶", "魔術師", "将軍", "大臣", "姫"};

#endif
