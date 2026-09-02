#ifndef MAX_NUM
const int max_num[8] = {5, 2, 2, 2, 2, 1, 1, 1};
#define MAX_NUM
#endif
#ifndef EXP_REWARD_HPP
#define EXP_REWARD_HPP

#include <algorithm>
#include <vector>
#include <cassert>
#include <cstddef>
#include <numeric>
#include <iterator>
#include <fstream>
#include <iostream>

// #include "bf_position.hpp"
// #include "log_util.hpp"
#include "bs_set.hpp"

enum Action {
  Sol_2,
  Sol_3,
  Sol_4,
  Sol_5,
  Sol_6,
  Sol_7,
  Sol_8,
  Crown,
  Knight,
  Priest,
  Wiz_self,
  Wiz_enemy,
  General,
  Minister
};

struct prob_endgame {
  double win, draw, lose;
};

const int ACTION_COUNT = 14;
const int BUCKET_COUNT = 11;
const int Total_hash_space = TRASH_MAX * HAND_MAX * COMPLEX_SELF_MAX * COMPLEX_ENEMY_MAX;
const std::string BUCKET_FILENAME = "buckets_data.bin";
const std::string REWARD_FILENAME = "reward_table.bin";
std::vector<double> reward_table(Total_hash_space, -2.0);
bool commentable = false;

void save_buckets(const std::vector<int> buckets[], int count) {
  std::ofstream out(BUCKET_FILENAME, std::ios::binary);
  if(!out) {
    std::cerr << "Error: Cannot open file for writing: " << BUCKET_FILENAME << std::endl;
    return;
  }

  // 各バケットについて「サイズ」と「データ」を順に書き込む
  for(int i = 0; i < count; ++i) {
    // 1. ベクトルの要素数 (size_t) を書き込む
    size_t size = buckets[i].size();
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));

    // 2. ベクトルの中身 (double配列) を書き込む
    if(size > 0) {
      out.write(reinterpret_cast<const char*>(buckets[i].data()), size * sizeof(int));
    }
  }

  out.close();
  std::cout << "Saved buckets to " << BUCKET_FILENAME << std::endl;
}

void load_buckets(std::vector<int> buckets[], int count) {
  std::ifstream in(BUCKET_FILENAME, std::ios::binary);
  if(!in) {
    std::cerr << "Error: Cannot open file for reading: " << BUCKET_FILENAME << std::endl;
    return;
  }

  for(int i = 0; i < count; ++i) {
    size_t size = 0;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));

    buckets[i].resize(size);

    if(size > 0) {
      in.read(reinterpret_cast<char*>(buckets[i].data()), size * sizeof(int));
    }
  }

  in.close();
  std::cout << "Loaded buckets from " << BUCKET_FILENAME << std::endl;
}

void save_reward_table() {
  std::ofstream out(REWARD_FILENAME, std::ios::binary);
  if(!out) {
    std::cerr << "Error: Cannot open file for writing: " << REWARD_FILENAME << std::endl;
    return;
  }

  // 1. ベクトルの要素数 (size_t) を書き込む
  size_t size = reward_table.size();
  out.write(reinterpret_cast<const char*>(&size), sizeof(size));

  // 2. ベクトルの中身 (double配列) を書き込む
  if(size > 0) {
    // std::vector<double> なので sizeof(double) を使用する
    out.write(reinterpret_cast<const char*>(reward_table.data()), size * sizeof(double));
  }

  out.close();
  std::cout << "Saved reward_table to " << REWARD_FILENAME << " (Size: " << size << ")" << std::endl;
}

void load_reward_table() {
  std::ifstream in(REWARD_FILENAME, std::ios::binary);
  if(!in) {
    std::cerr << "Error: Cannot open file for reading: " << REWARD_FILENAME << std::endl;
    return;
  }

  // 1. ベクトルの要素数を読み込む
  size_t size = 0;
  in.read(reinterpret_cast<char*>(&size), sizeof(size));

  // 2. メモリを確保 (読み込んだサイズに合わせてリサイズ)
  reward_table.resize(size);

  // 3. データを一気に読み込む
  if(size > 0) {
    in.read(reinterpret_cast<char*>(reward_table.data()), size * sizeof(double));
  }

  in.close();
  std::cout << "Loaded reward_table from " << REWARD_FILENAME << " (Size: " << size << ")" << std::endl;
}

bool is_ge1_lt8(int c) {
  return 1 <= c && c <= 8;
}

double lastjudge(int hand_val, int enemy_val) {
  if(hand_val > enemy_val) return 1.0;
  if(hand_val == enemy_val) return 0.0;
  return -1.0;
}

bool is_legal_multiple_list(const int cards[8]) {
  for(int i = 0; i < 8; i++) {
    if(cards[i] < 0) return false;
    if(max_num[i] - cards[i] < 0) return false;
  }
  return true;
}

// 手札と場のカードがルール上正当かチェック
bool is_legal(const int hand[2], const int trash[8]) {
  int cards[8];
  for(int i = 0; i < 8; i++) {
    cards[i] = trash_and_hand_s(i, hand, trash);
  }
  if(hand[0] > hand[1]) return false;

  if(!is_legal_multiple_list(cards)) return false;
  return true;
}

bool is_legal_state(const State& s) {
  int deck = s.count_deck();
  if(deck > 10 || deck < 1) return false;

  int visible_card[8] = {0};
  for(int i = 0; i < 8; ++i) {
    visible_card[i] = s.trash[i];
  }
  if(s.hand[0] >= 1 && s.hand[0] <= 8) visible_card[s.hand[0] - 1]++;
  if(s.hand[1] >= 1 && s.hand[1] <= 8) visible_card[s.hand[1] - 1]++;
  if(!is_legal_multiple_list(visible_card)) return false;

  // フラグが立っているなら、それを発動させたカードがTrashにあるはず
  if(10 - deck - count_active_flags(s) < 0) return false;
  // if(commentable) std::cout << "B" << std::endl;
  if(s.barrier && s.trash[3] < 1) return false;
  if((s.lt5_flag_s || s.lt5_flag_e) && s.trash[6] < 1) return false;
  if((s.sol_flag_e[0] > 0 ? 1 : 0) + (s.sol_flag_e[1] > 0 ? 1 : 0) + (s.sol_flag_s[0] > 0 ? 1 : 0) + (s.sol_flag_s[1] > 0 ? 1 : 0) > s.trash[0]) return false;
  if((s.not7_flag ? 1 : 0) > s.trash[4]) return false;

  int active_opens = (s.open_flag_s > 0 ? 1 : 0) + (s.open_flag_e > 0 ? 1 : 0);
  int max_possible_opens = s.trash[1] * 1 + s.trash[2] * 2 + s.trash[5] * 2;
  if(active_opens > max_possible_opens) return false;

  //すでにすべて見えているカードに対して兵士宣言はしない
  if(s.sol_flag_e[0] > 0 && max_num[s.sol_flag_e[0] - 1] <= visible_card[s.sol_flag_e[0] - 1]) return false;
  if(s.sol_flag_e[1] > 0 && max_num[s.sol_flag_e[1] - 1] <= visible_card[s.sol_flag_e[1] - 1]) return false;
  if(s.sol_flag_s[0] > 0 && max_num[s.sol_flag_s[0] - 1] <= s.trash[s.sol_flag_s[0] - 1]) return false;
  if(s.sol_flag_s[1] > 0 && max_num[s.sol_flag_s[1] - 1] <= s.trash[s.sol_flag_s[1] - 1]) return false;
  if(s.open_flag_s > 0 && !in(s.hand, s.open_flag_s)) return false;

  //すでにすべて見えているカードに対して兵士宣言や兵士以外の宣言はしない
  // if(s.open_flag_e > 0 && max_num[s.open_flag_e-1] -1 < visible_card[s.open_flag_e-1] + ((s.sol_flag_s[0] == s.open_flag_e) || (s.sol_flag_s[1] == s.open_flag_e) ? 1 : 0 )) return false;
  // if(s.open_flag_s > 0 && max_num[s.open_flag_s-1] -1 < s.trash[s.open_flag_s-1] + ((s.sol_flag_e[0] == s.open_flag_s || s.sol_flag_e[1] == s.open_flag_s) ? 1 : 0 )) return false;

  bool p = false;
  for(int i = 0; i < 8; ++i) {
    p = p || s.hand_e(i);
  }
  if(!p) return false;

  // 合法でない手札は1枚まで
  int illegal = 0;
  if(s.lt5_flag_s && s.hand[0] >= 5) illegal++;
  if(s.lt5_flag_s && s.hand[1] >= 5) illegal++;
  if(s.sol_flag_s[0] > 1 && s.hand[0] == s.sol_flag_s[0]) illegal++;
  if(s.sol_flag_s[0] > 1 && s.hand[1] == s.sol_flag_s[0]) illegal++;
  if(s.sol_flag_s[1] > 1 && s.hand[0] == s.sol_flag_s[1]) illegal++;
  if(s.sol_flag_s[1] > 1 && s.hand[1] == s.sol_flag_s[1]) illegal++;
  if(illegal > 1) return false;

  if(s.sol_flag_s[0] == 1 || s.sol_flag_s[1] == 1 || s.sol_flag_e[0] == 1 || s.sol_flag_e[1] == 1) exit(1);
  return true;
}

int get_num_by_action(const Action a) {
  if(Sol_2 <= a && a <= Sol_8) return 1;
  else if(a == Crown) return 2;
  else if(a == Knight) return 3;
  else if(a == Priest) return 4;
  else if(a == Wiz_self || a == Wiz_enemy) return 5;
  else if(a == General) return 6;
  else if(a == Minister) return 7;
  else return -1;
}

std::array<bool, 14> get_legal_action(const State& s) {
  std::array<bool, 14> actions = {false};
  bool sol, wiz;
  sol = s.in(1);
  wiz = s.in(5);
  for(int i = 0; i < 7; i++) { // 1行動
    actions[i] = sol;
  }
  for(int i = 0; i < 3; i++) { // 2~4行動
    actions[7 + i] = s.in(i + 2);
  }
  actions[10] = wiz;
  actions[11] = wiz;
  actions[12] = s.in(6);
  actions[13] = s.in(7);
  return actions;
}

void print_legal_action(bool actions[14]) {
  for(int i = 0; i < 14; i++) {
    if(actions[i]) std::cout << Action(i) << " ";
  }
  std::cout << std::endl;
}

prob_endgame game_result_prob(const State& s, const Action a) {
  prob_endgame res;
  res.win = 0;
  res.draw = 1;
  res.lose = 0;
  if(a == Wiz_self && s.other(5) == 8) {
    res.lose = 1;
    res.draw = 0;
    return res;
  }
  if(a == Crown || a == Priest || a == General || a == Minister || s.barrier) {
    return res;
  }
  if(Sol_2 <= a && a <= Sol_8) {
    res.win = s.hand_e_prob(int(a) + 1);
    res.draw -= res.win;
    return res;
  }
  if(a == Knight) {
    int hand = s.other(3);
    int last_g = 0;
    int last_d = 0;
    int last_l = 0;
    for(int i = 0; i < 8; i++) {
      if(s.hand_e(i)) {
        if(i + 1 < hand) {
          last_g += s.deck_or_hand_e(i);
        } else if(i + 1 > hand) {
          last_l += s.deck_or_hand_e(i);
        } else {
          last_d += s.deck_or_hand_e(i);
        }
      }
    }
    res.win = (double)last_g / (last_g + last_d + last_l);
    res.draw = (double)last_d / (last_g + last_d + last_l);
    res.lose = (double)last_l / (last_g + last_d + last_l);
    return res;
  }
  if(a == Wiz_enemy) {
    res.win = s.hand_e_prob(7);
    res.draw -= res.win;
    return res;
  }
  return res;
}

std::vector<int> unique(std::vector<int> values) {
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

std::vector<State> unique(std::vector<State> values) {
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

std::vector<int> get_hashes(const std::vector<State>& states) {
  std::vector<int> hashes;
  hashes.reserve(states.size()); // メモリ確保で効率化

  // 各Stateをハッシュ値に変換
  for(const auto& s : states) {
    hashes.push_back(get_hash(s));
  }
  return hashes;
}

std::vector<State> set_hand(const State s, const int hand) {
  std::vector<State> legal_next_states;
  const int temp_op = s.open_flag_e;
  State s2 = s;
  s2.open_flag_e = hand;
  // if(commentable) s2.print();
  for(int i = 0; i < 8; i++) {
    if(s2.hand_s(i)) {
      State s3 = s2;
      s3.hand[0] = i + 1;
      if(s3.trash[i] + 1 == max_num[i] && in(s3.sol_flag_e, i + 1)) s3.rm_sol_e(i + 1);
      for(int j = 0; j < 8; j++) {
        if(s3.deck(j)) {
          State s4 = s3;
          s4.hand[1] = j + 1;

          if(s4.hand[0] > s4.hand[1]) std::swap(s4.hand[0], s4.hand[1]);

          s4.open_flag_e = temp_op;
          s4 = standardize(s4);
          // if(commentable) s4.print();
          if(is_legal_state(s4)) {
            legal_next_states.push_back(s4);
          }
        }
      }
    }
  }
  return legal_next_states;
}

std::vector<State> get_state_by_action(const State& s, const Action a) {
  int c = get_num_by_action(a);
  assert(s.in(c));
  int hand = s.other(c);
  State s2 = s.update_flag_discard(c);
  std::vector<State> states;

  if(s.barrier) {
    if(a == Priest) {
      s2.barrier = true;
    } else if(a == Wiz_self) {
      s2.trash[hand - 1]++;
      s2.lt5_flag_e = false;
      s2.sol_flag_e[0] = 0;
      s2.sol_flag_e[1] = 0;
      s2.open_flag_e = 0;
    } else if(a == Wiz_enemy) {
      s2.not7_flag = true;
    } else if(a == Minister) {
      s2.lt5_flag_e = true;
    }
    states = set_hand(s2, hand);
    return unique(states);
  }

  if(Sol_2 <= a && a <= Sol_8) {
    if(s2.trash[a + 1] < max_num[a + 1]) s2.add_sol_s(a + 2);
    states = set_hand(s2, hand);
  } else if(a == Priest) {
    s2.barrier = true;
    states = set_hand(s2, hand);
  } else if(a == Wiz_self) {
    s2.trash[hand - 1]++;
    s2.lt5_flag_e = false;
    s2.sol_flag_e[0] = 0;
    s2.sol_flag_e[1] = 0;
    s2.open_flag_e = 0;
    states = set_hand(s2, hand);
  } else if(a == Minister) {
    s2.lt5_flag_e = true;
    states = set_hand(s2, hand);
  } else if(a == Knight) {
    if(s.hand_e(hand - 1)) {
      s2.open_flag_s = hand;
      s2.open_flag_e = hand;
      states = set_hand(s2, hand);
    }
  } else if(a == Crown) {
    for(int i = 0; i < 8; i++) {
      if(s.hand_e(i)) {
        State s3 = s2;
        s3.open_flag_s = i + 1;
        states = set_hand(s3, hand);
      }
    }
  } else if(a == Wiz_enemy) {
    for(int i = 0; i < 7; i++) { //ゲームが進行する場合のみ処理するので姫を考えない
      if(s.hand_e(i)) {
        State s3 = s2;
        s3.trash[i]++;
        s3.lt5_flag_s = false;
        s3.sol_flag_s[0] = 0;
        s3.sol_flag_s[1] = 0;
        s3.open_flag_s = 0;
        s3.not7_flag = true;
        states = set_hand(s3, hand);
      }
    }
  } else if(a == General) {
    for(int i = 0; i < 8; i++) {
      if(s.hand_e(i)) {
        State s3 = s2;
        s3.open_flag_s = hand;
        s3.open_flag_e = i + 1;
        states = set_hand(s3, i + 1);
      }
    }
  } else {
    exit(1);
  }
  return unique(states);
}

void print_game_result_prob(const State& s, const Action a) {
  prob_endgame result = game_result_prob(s, a);
  std::cout << a << " " << result.win << ":" << result.draw << ":" << result.lose << std::endl;
}

// 完全情報下の期待報酬計算
double exp_reward_perfect(const int hand[2], int enemyhand, int last, bool barrier) {
  assert(is_ge1_lt8(enemyhand));
  assert(is_ge1_lt8(last));
  int lasts[8] = {};
  lasts[enemyhand - 1]++;
  lasts[last - 1]++;
  assert(is_legal(hand, lasts));

  if(in(hand, 7) && (hand[0] + hand[1] >= 12)) return -1.0;

  // バリアがある場合
  if(barrier) {
    if(!in(hand, 5)) {
      return lastjudge(hand[1], enemyhand);
    }
    return std::max(lastjudge(hand[1], enemyhand), lastjudge(last, enemyhand));
  }

  std::vector<double> exp_list;

  // 1を持っている場合
  if(in(hand, 1)) {
    if(enemyhand == 1) {
      exp_list.push_back(lastjudge(hand[1], enemyhand));
    }
    return 1.0;
  }

  // 6を持っている場合
  if(in(hand, 6)) {
    exp_list.push_back(lastjudge(enemyhand, other(hand, 6)));
  }

  // 5を持っている場合
  if(in(hand, 5)) {
    if(enemyhand == 8) return 1.0;
    exp_list.push_back(lastjudge(last, enemyhand));
    exp_list.push_back(lastjudge(other(hand, 5), last));
  }

  exp_list.push_back(lastjudge(hand[1], enemyhand));

  if(exp_list.empty()) return -2.0; // Error value
  return *std::max_element(begin(exp_list), end(exp_list));
}

double exp_reward2(const int hash) {
  State s = decode_hash(hash);
  assert(is_legal(s.hand, s.trash));
  assert(s.count_deck() == 1);
  int last[2] = {};
  s.last2card(last);
  int enemyhand = s.open_e();
  if(enemyhand > 0) {
    if(enemyhand == last[0]) return exp_reward_perfect(s.hand, enemyhand, last[1], s.barrier);
    else if(enemyhand == last[1]) return exp_reward_perfect(s.hand, enemyhand, last[0], s.barrier);
    else {
      decode_hash(hash).print();
      std::cout << hash << std::endl;
      exit(1);
    }
  }
  double val1 = exp_reward_perfect(s.hand, last[0], last[1], s.barrier);
  double val2 = exp_reward_perfect(s.hand, last[1], last[0], s.barrier);
  return (val1 + val2) / 2.0;
}

double exp_reward(const int hash) {
  State s = decode_hash(hash);
  int deck = s.count_deck();
  if(deck == 1) return exp_reward2(hash);
  assert(deck > 1);
  assert(is_legal_state(s));
  if(in(s.hand, 7) && (s.hand[0] + s.hand[1] >= 12)) return -1.0;
  std::array<bool, ACTION_COUNT> actions = get_legal_action(s);
  // print_legal_action(actions);
  double reward[ACTION_COUNT] = {};
  for(int i = 0; i < ACTION_COUNT; ++i) {
    if(!actions[i]) {
      reward[i] = -2;
      continue;
    }
    prob_endgame result = game_result_prob(s, Action(i));
    double w, d, l;
    w = result.win;
    d = result.draw;
    l = result.lose;
    if(commentable) print_game_result_prob(s, Action(i));
    if(w + l == 1.0) {
      reward[i] = w - l;
      if(w == 1) break;
      continue;
    }
    if(deck == 2 && Action(i) == Wiz_self) {
      int last[3] = {};
      s.last3card(last);
      double dreward = 0;
      dreward += s.hand_e_prob(last[0] - 1) * (lastjudge(last[1], last[0]) + lastjudge(last[2], last[0])) / 2;
      dreward += s.hand_e_prob(last[1] - 1) * (lastjudge(last[2], last[1]) + lastjudge(last[0], last[1])) / 2;
      dreward += s.hand_e_prob(last[2] - 1) * (lastjudge(last[0], last[2]) + lastjudge(last[1], last[2])) / 2;
      dreward /= (s.hand_e_prob(last[0] - 1) + s.hand_e_prob(last[1] - 1) + s.hand_e_prob(last[2] - 1));
      reward[i] = w + d * dreward - l;
    } else if(deck == 2 && Action(i) == Wiz_enemy && !s.barrier) {
      int hand = s.other(5);
      int last[3] = {};
      s.last3card(last);
      double dreward = 0;
      dreward += s.hand_e_prob(last[0] - 1) * (lastjudge(hand, last[1]) + lastjudge(hand, last[2])) / 2;
      dreward += s.hand_e_prob(last[1] - 1) * (lastjudge(hand, last[2]) + lastjudge(hand, last[0])) / 2;
      dreward += s.hand_e_prob(last[2] - 1) * (lastjudge(hand, last[0]) + lastjudge(hand, last[1])) / 2;
      dreward /= (s.hand_e_prob(last[0] - 1) + s.hand_e_prob(last[1] - 1) + s.hand_e_prob(last[2] - 1));
      reward[i] = w + d * dreward - l;
    } else if(s.barrier && Sol_3 <= Action(i) && Action(i) <= Sol_8) {
      reward[i] = reward[0];
    } else {
      std::vector<int> pred = get_hashes(get_state_by_action(s, Action(i)));
      if(pred.size() == 0) {
        if(commentable) std::cout << Action(i) << " size0" << std::endl;
        reward[i] = -2;
        continue;
      }
      double dreward = 0;
      for(int h : pred) {
        double r = reward_table[h];
        if(commentable) std::cout << Action(i) << " " << h << " : " << r << std::endl;
        // if(r < -1 || 1 < r) exit(1);
        dreward -= r;
      }
      dreward /= pred.size();
      reward[i] = w + d * dreward - l;
    }
  }
  for(double r : reward) {
    if(commentable) std::cout << r << " ";
  }
  if(commentable) std::cout << std::endl;

  double max = *std::max_element(std::begin(reward), std::end(reward));
  if(max > 1 || -1 > max) {
    std::cout << "err:hash = " << hash << ", max = " << max << std::endl;
    // if(!commentable) exit(1);
  }
  return max;
}

bool abs_win(const State& s) {
  int deck = s.count_deck();
  if(deck == 1) return exp_reward2(get_hash(s)) == 1.0;
  assert(deck > 1);
  assert(is_legal_state(s));
  if(in(s.hand, 7) && (s.hand[0] + s.hand[1] >= 12)) return false;
  std::array<bool, ACTION_COUNT> actions = get_legal_action(s);
  // print_legal_action(actions);
  for(int i = 0; i < ACTION_COUNT; ++i) {
    if(!actions[i]) continue;
    prob_endgame result = game_result_prob(s, Action(i));
    if(commentable) print_game_result_prob(s, Action(i));
    if(result.win + result.lose == 1.0) {
      if(result.win == 1.0) return true;
      continue;
    } else if(deck == 2 && Action(i) == Wiz_self) {
      int last[3] = {};
      s.last3card(last);
      double dreward = 0;
      dreward += s.hand_e_prob(last[0] - 1) * (lastjudge(last[1], last[0]) + lastjudge(last[2], last[0])) / 2;
      dreward += s.hand_e_prob(last[1] - 1) * (lastjudge(last[2], last[1]) + lastjudge(last[0], last[1])) / 2;
      dreward += s.hand_e_prob(last[2] - 1) * (lastjudge(last[0], last[2]) + lastjudge(last[1], last[2])) / 2;
      dreward /= (s.hand_e_prob(last[0] - 1) + s.hand_e_prob(last[1] - 1) + s.hand_e_prob(last[2] - 1));
      if(result.win + result.draw * dreward >= 1.0) return true;
    } else if(deck == 2 && Action(i) == Wiz_enemy && !s.barrier) {
      int hand = s.other(5);
      int last[3] = {};
      s.last3card(last);
      double dreward = 0;
      dreward += s.hand_e_prob(last[0] - 1) * (lastjudge(hand, last[1]) + lastjudge(hand, last[2])) / 2;
      dreward += s.hand_e_prob(last[1] - 1) * (lastjudge(hand, last[2]) + lastjudge(hand, last[0])) / 2;
      dreward += s.hand_e_prob(last[2] - 1) * (lastjudge(hand, last[0]) + lastjudge(hand, last[1])) / 2;
      dreward /= (s.hand_e_prob(last[0] - 1) + s.hand_e_prob(last[1] - 1) + s.hand_e_prob(last[2] - 1));
      if(result.win + result.draw * dreward >= 1.0) return true;
    } else if(s.barrier && Sol_3 <= Action(i) && Action(i) <= Sol_8) {
      continue;
    } else {
      std::vector<State> pred_e = get_state_by_action(s, Action(i));
      if(pred_e.size() == 0) {
        if(commentable) std::cout << Action(i) << " size0" << std::endl;
        continue;
      }
      bool my_action_abswin = true;
      for(const State& s2 : pred_e) {
        // std::cout << "pred_e: " << std::endl;
        // s2.print();
        std::array<bool, ACTION_COUNT> enum_actions = get_legal_action(s2);
        bool enemy_can_win = false;
        for(int j = 0; j < ACTION_COUNT; ++j) {
          if(!enum_actions[j]) continue;
          prob_endgame result_e = game_result_prob(s2, Action(j));
          // 相手が勝つ（＝自分が負ける）ルートがあるなら、今の自分の手は必勝ではない
          if(result_e.win > 0) break;
          if(result_e.draw > 0) {
            std::vector<State> pred_s = get_state_by_action(s2, Action(j));
            for(const State& s3 : pred_s) {
              if(!abs_win(s3)) {
                enemy_can_win = true;
                break;
              }
            }
          }
          // 相手がこちらの必勝を潰すルートを1つでも見つけたら、他の相手の手は調べなくてよい
          if(enemy_can_win) break;
        }
        // このstateにおいて相手が必勝を潰せるなら、最初のactionは失敗
        if(enemy_can_win) {
          my_action_abswin = false;
          break;
        }
      }
      // 相手があらゆる手を尽くしてもこちらの必勝を潰せなかったなら、この手は必勝
      if(my_action_abswin) return true;
    }
  }
  // 全ての可能な行動を試したが、どれも必勝ではなかった
  return false;
}

#endif
