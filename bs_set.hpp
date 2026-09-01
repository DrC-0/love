#ifndef MAX_NUM
const int max_num[8] = {5, 2, 2, 2, 2, 1, 1, 1};
#define MAX_NUM
#endif
#ifndef BS_HASH
#define BS_HASH

#include <iostream>
#include <cassert>
#include <iomanip>
#include <compare>

// #include "State.hpp"
// #include "log_util.hpp"
// #include "endgame.hpp"

struct State {
  bool barrier;
  bool not7_flag;
  bool lt5_flag_s;
  bool lt5_flag_e;
  int open_flag_s; // 0-8
  int open_flag_e; // 0-8
  int sol_flag_s[2]; // 0, 2-8 (29パターン)
  int sol_flag_e[2]; // 0, 2-8 (29パターン)
  int hand[2]; // [min, max]
  int trash[8]; // 各カードの残り枚数
  auto operator<=>(const State&) const = default;
  State update_flag_discard(int c) const;
  int trash_and_hand_s(const int) const;
  int deck_or_hand_e(const int i) const;
  bool hand_e(const int i) const;
  bool hand_s(const int i) const;
  int open_e() const;
  int open_s() const;
  bool deck(const int i) const;
  bool in(const int card) const;
  int other(const int card) const;
  int count_deck() const;
  void last2card(int[2]) const;
  void last3card(int[3]) const;
  int total_hand_e_count() const;
  double hand_e_prob(const int i) const;
  void print() const;
  void reset_flag(bool to_self);
  void add_sol_s(int card);
  void add_sol_e(int card);
  void rm_sol_s(int card);
  void rm_sol_e(int card);
};

const int TRASH_BASES[8] = {6, 3, 3, 3, 3, 2, 2, 2};
const int TRASH_MAX = 3888; // 6*3*3*3*3*2*2*2
const int HAND_MAX = 36;
const int SOL_MAX = 29;
const int COMPLEX_SELF_MAX = 60;
const int COMPLEX_ENEMY_MAX = 104;

// 手札 {h0, h1} -> 0~35
int encode_hand_idx(int h0, int h1) {
  if(h0 > h1) std::swap(h0, h1);
  int idx = 0;
  for(int i = 1; i <= 8; ++i) {
    for(int j = i; j <= 8; ++j) {
      if(i == h0 && j == h1) return idx;
      idx++;
    }
  }
  return -1;
}

// 0~35 -> 手札 {h0, h1}
void decode_hand_idx(int idx, int hand_out[2]) {
  int current = 0;
  for(int i = 1; i <= 8; ++i) {
    for(int j = i; j <= 8; ++j) {
      if(current == idx) {
        hand_out[0] = i;
        hand_out[1] = j;
        return;
      }
      current++;
    }
  }
}

// 配列 trash[8] -> 0~3887
int encode_trash_idx(const int trash[8]) {
  int idx = 0;
  int multiplier = 1;
  for(int i = 0; i < 8; ++i) {
    idx += trash[i] * multiplier;
    multiplier *= TRASH_BASES[i];
  }
  return idx;
}

// 0~3887 -> 配列 trash[8]
void decode_trash_idx(int idx, int trash_out[8]) {
  for(int i = 0; i < 8; ++i) {
    trash_out[i] = idx % TRASH_BASES[i];
    idx /= TRASH_BASES[i];
  }
}

inline int map_sol_array(const int sol[2]) {
  assert(sol[0] >= 0 && sol[0] <= 8 && sol[0] != 1);
  assert(sol[1] >= 0 && sol[1] <= 8 && sol[1] != 1);
  assert(sol[0] < sol[1] || sol[1] == 0); // 条件: sol[0]<sol[1] (ただし片方0の場合はsol[1]==0)

  if(sol[0] == 0 && sol[1] == 0) return 0;

  if(sol[1] == 0) {
    // [1-7]: 片方のみ保持 (sol[0]は 2~8 -> 1~7にマッピング)
    return sol[0] - 1;
  }

  // [8-28]: 両方保持。sol[0] < sol[1] (組み合わせロジック)
  int v0 = sol[0] - 1; // 1~6
  int v1 = sol[1] - 1; // 2~7
  int idx = 8;
  for(int i = 1; i < v0; ++i) {
    idx += (7 - i);
  }
  idx += (v1 - v0 - 1);
  return idx;
}

inline void unmap_sol_array(int val, int sol_out[2]) {
  if(val == 0) {
    sol_out[0] = 0;
    sol_out[1] = 0;
    return;
  }
  if(val >= 1 && val <= 7) {
    sol_out[0] = val + 1;
    sol_out[1] = 0;
    return;
  }

  // val >= 8 (両方保持しているケースの復元)
  int rem = val - 8;
  for(int v0 = 1; v0 <= 6; ++v0) {
    int ways = 7 - v0;
    if(rem < ways) {
      sol_out[0] = v0 + 1;
      sol_out[1] = v0 + 1 + rem + 1;
      return;
    }
    rem -= ways;
  }
}

int encode_self(int open_s, const int sol_s[2], bool lt5_s, const int hand[2]) {
  if(open_s != 0) {
    // [58, 59]: Openがある場合 (旧 16, 17)
    if(open_s == hand[0]) {
      return 58; // Open Hand[0]
    } else if(open_s == hand[1]) {
      return 59; // Open Hand[1]
    } else {
      assert(false && "Open card not found in hand!");
      return -1;
    }
  } else {
    // [0 - 57]: open_s == 0
    // sol_s (29通り) * lt5_s (2通り) = 58通り
    return map_sol_array(sol_s) + (lt5_s ? SOL_MAX : 0);
  }
}

void decode_self(int val, int& open_s, int sol_s[2], bool& lt5_s, const int hand[2]) {
  open_s = 0;
  sol_s[0] = 0;
  sol_s[1] = 0;
  lt5_s = false;

  if(val == 58) {
    open_s = hand[0];
  } else if(val == 59) {
    open_s = hand[1];
  } else {
    lt5_s = (val >= SOL_MAX);
    unmap_sol_array(val % SOL_MAX, sol_s);
  }
}

int encode_enemy(bool barrier, int open_e, const int sol_e[2], bool lt5_e, bool not7) {
  if(open_e != 0) {
    // [0 - 15]: open_e != 0 (変更なし)
    int b_bit = barrier ? 8 : 0;
    return (open_e - 1) + b_bit;
  } else {
    if(barrier) {
      // [16 - 44]: Barrier ON
      return 16 + map_sol_array(sol_e);
    } else {
      if(lt5_e) {
        // [45]: Lt5 ON
        return 45;
      } else {
        // [46 - 103]: Lt5 OFF
        // sol_e(29) * not7(2) = 58通り
        int n7_bit = not7 ? SOL_MAX : 0;
        return 46 + map_sol_array(sol_e) + n7_bit;
      }
    }
  }
}

void decode_enemy(int val, bool& barrier, int& open_e, int sol_e[2], bool& lt5_e, bool& not7) {
  barrier = false;
  open_e = 0;
  sol_e[0] = 0;
  sol_e[1] = 0;
  lt5_e = false;
  not7 = false;

  if(val < 16) {
    open_e = (val % 8) + 1;
    barrier = (val >= 8);
  } else if(val < 45) {
    // [16-44]: Open=0, Barrier=ON
    barrier = true;
    unmap_sol_array(val - 16, sol_e);
  } else if(val == 45) {
    // [45]: Open=0, Barrier=OFF, Lt5=ON
    lt5_e = true;
  } else {
    // [46-103]: Open=0, Barrier=OFF, Lt5=OFF
    int temp = val - 46;
    not7 = (temp >= SOL_MAX);
    unmap_sol_array(temp % SOL_MAX, sol_e);
  }
}
// ハッシュ化と復元 (Core Logic)
int get_hash(const State& s) {
  // 1. Trash (Old Opened)
  int h_trash = encode_trash_idx(s.trash);

  // 2. Hand
  int h_hand = encode_hand_idx(s.hand[0], s.hand[1]);

  // 3. Complex (Self & Enemy)
  int h_self = encode_self(s.open_flag_s, s.sol_flag_s, s.lt5_flag_s, s.hand);
  int h_enemy = encode_enemy(s.barrier, s.open_flag_e, s.sol_flag_e, s.lt5_flag_e, s.not7_flag);

  // Enemy(48) * Self(24) + h_self として結合
  int h_complex = h_enemy * COMPLEX_SELF_MAX + h_self;

  // 全結合
  // Trash -> Hand -> Complex
  int hash = h_trash;
  hash += h_hand * TRASH_MAX;
  hash += h_complex * (TRASH_MAX * HAND_MAX);

  return hash;
}

State decode_hash(int hash) {
  State s;
  int temp = hash;

  // 1. Trash
  int tr_idx = temp % TRASH_MAX;
  temp /= TRASH_MAX;
  decode_trash_idx(tr_idx, s.trash);

  // 2. Hand
  int h_idx = temp % HAND_MAX;
  temp /= HAND_MAX;
  decode_hand_idx(h_idx, s.hand);

  // 3. Complex
  int complex = temp;

  int h_self = complex % COMPLEX_SELF_MAX;
  int h_enemy = complex / COMPLEX_SELF_MAX;

  decode_self(h_self, s.open_flag_s, s.sol_flag_s, s.lt5_flag_s, s.hand);
  decode_enemy(h_enemy, s.barrier, s.open_flag_e, s.sol_flag_e, s.lt5_flag_e, s.not7_flag);

  // 復元時に整合性を保つためのデフォルト値設定（無視されたフラグ）
  // ※decode関数内でfalse/0に初期化しているので、特に追加処理は不要ですが
  // 念のため、構造上ありえない組み合わせ（Barrier=TrueかつLt5=Trueなど）は
  // decode_enemy内で既にクリアされています。

  // not7_flagはEnemy側で管理されますが、Self側には存在しないため、
  // s.not7_flag (共有) として復元されます。

  return s;
}

State standardize(const State& s) {
  return decode_hash(get_hash(s));
}

void State::print() const {
  // --- 基本情報 ---
  std::cout << "Hand:  [" << hand[0] << ", " << hand[1] << "]" << std::endl;

  std::cout << "Trash: [";
  for(int i = 0; i < 8; ++i) {
    std::cout << trash[i] << (i == 7 ? "" : ",");
  }
  std::cout << "]     deck: " << count_deck() << std::endl;

  // --- Self (自分) の状態 ---
  // ルール: Open_S != 0 なら Lt5_S, Sol_S は無視される
  std::cout << "[Self]  ";
  if(open_flag_s != 0) {
    std::cout << "Open:" << open_flag_s << " (Override Lt5/Sol)";
  } else {
    std::cout << "Open:- | ";
    std::cout << "Lt5:" << (lt5_flag_s ? "ON " : "OFF") << " | ";
    std::cout << "Sol:" << sol_flag_s;
  }
  std::cout << std::endl;

  // --- Enemy (相手/環境) の状態 ---
  // ルール: Open > Barrier > Lt5 > Not7 の順で支配
  std::cout << "[Enemy] ";

  if(open_flag_e != 0) {
    // Case 1: Openがある場合
    // Barrierの情報はEncodeに含まれるが、Lt5/Not7/Solは無視される
    std::cout << "Open:" << open_flag_e << " | ";
    std::cout << "Barrier:" << (barrier ? "ON " : "OFF");
    std::cout << " (Override Lt5/Not7/Sol)";
  } else {
    std::cout << "Open:- | ";

    if(barrier) {
      // Case 2: Barrierがある場合
      // Solだけ有効、Lt5/Not7は無視
      std::cout << "Barrier:ON | ";
      std::cout << "Sol:" << sol_flag_e;
      std::cout << " (Override Lt5/Not7)";
    } else {
      std::cout << "Barrier:OFF | ";

      if(lt5_flag_e) {
        // Case 3: Lt5がある場合
        // Sol有効、Not7は無視
        std::cout << "Lt5:ON | ";
        std::cout << "Sol:" << sol_flag_e;
        std::cout << " (Override Not7)";
      } else {
        // Case 4: フルセット (Lt5なし)
        // Sol, Not7 有効
        std::cout << "Lt5:OFF | ";
        std::cout << "Sol:" << sol_flag_e << " | ";
        std::cout << "Not7:" << (not7_flag ? "T" : "F");
      }
    }
  }
  std::cout << std::endl
            << std::endl;
}

// 1つの状態 s の中で、有効になっているフラグの合計数を返す
int count_active_flags(const State& s) {
  int count = 0;

  if(s.barrier) count++;
  if(s.lt5_flag_s) count++;
  if(s.lt5_flag_e) count++;
  if(s.not7_flag) count++;

  // open は int、sol は int[2] なので、それぞれ 0 より大きいかで判定する
  if(s.open_flag_s > 0 || s.open_flag_e > 0) count++;
  if(s.sol_flag_s[0] > 0) count++;
  if(s.sol_flag_s[1] > 0) count++;
  if(s.sol_flag_e[0] > 0) count++;
  if(s.sol_flag_e[1] > 0) count++;

  return count;
}

bool in(const int hand[2], const int target) {
  return hand[0] == target || hand[1] == target;
}

int other(const int hand[2], const int target) {
  if(hand[0] == target) return hand[1];
  else if(hand[1] == target) return hand[0];
  return 0;
}

int trash_and_hand_s(const int i, const int hand[2], const int trash[8]) {
  return trash[i] + (hand[0] == i + 1 ? 1 : 0) + (hand[1] == i + 1 ? 1 : 0);
}

State swap_player(const State& s, const int hand) {
  State next_s = s;
  std::swap(next_s.open_flag_e, next_s.open_flag_s);
  std::swap(next_s.sol_flag_e, next_s.sol_flag_s);
  std::swap(next_s.lt5_flag_e, next_s.lt5_flag_s);
  next_s.hand[0] = hand;
  next_s.hand[1] = 0;
  return next_s;
}

void State::reset_flag(bool to_self) {
  if(to_self) {
    lt5_flag_s = false;
    open_flag_s = 0;
    sol_flag_s[0] = 0;
    sol_flag_s[1] = 0;
  } else {
    lt5_flag_e = false;
    open_flag_e = 0;
    sol_flag_e[0] = 0;
    sol_flag_e[1] = 0;
  }
}

State reset_flag_by_use(const State& s, bool to_self, int card) {
  struct State next_s = s;
  if(to_self) {
    if(s.open_flag_s > 0 && s.open_flag_s == card) {
      next_s.open_flag_s = 0;
    }
    if(s.sol_flag_s[0] != 0 && s.sol_flag_s[0] != card) {
      next_s.sol_flag_s[0] = s.sol_flag_e[1];
      next_s.sol_flag_s[1] = 0;
    }
    if(s.sol_flag_s[1] != 0 && s.sol_flag_s[1] != card) {
      next_s.sol_flag_s[1] = 0;
    }
    if(s.lt5_flag_s && card < 5) {
      next_s.lt5_flag_s = false;
    }
    next_s.not7_flag = false; //対象が7しかないため次のターンに7を出す出さないに関わらず推理がリセット

  } else {
    if(s.open_flag_e > 0 && s.open_flag_e == card) {
      next_s.open_flag_e = 0;
    }
    if(s.sol_flag_e[0] != 0 && s.sol_flag_e[0] != card) {
      next_s.sol_flag_s[0] = s.sol_flag_e[1];
      next_s.sol_flag_s[1] = 0;
    }
    if(s.sol_flag_e[1] != 0 && s.sol_flag_e[1] != card) {
      next_s.sol_flag_e[1] = 0;
    }
    if(s.lt5_flag_e && card < 5) {
      next_s.lt5_flag_e = false;
    }
    next_s.not7_flag = false;
  }
  return next_s;
}

State State::update_flag_discard(int c) const {
  State s;

  s.lt5_flag_s = this->lt5_flag_e;
  s.lt5_flag_e = this->lt5_flag_s;
  s.open_flag_s = this->open_flag_e;
  s.open_flag_e = this->open_flag_s;
  s.sol_flag_s[0] = this->sol_flag_e[0];
  s.sol_flag_s[1] = this->sol_flag_e[1];
  s.sol_flag_e[0] = this->sol_flag_s[0];
  s.sol_flag_e[1] = this->sol_flag_s[1];

  s.barrier = false;
  s.not7_flag = false;

  for(int i = 0; i < 8; ++i) s.trash[i] = this->trash[i];

  if(s.open_flag_e > 0 && s.open_flag_e == c) {
    s.open_flag_e = 0;
  }
  if(s.sol_flag_e[0] != 0 && s.sol_flag_e[0] != c) {
    s.sol_flag_e[0] = s.sol_flag_e[1];
    s.sol_flag_e[1] = 0;
  }
  if(s.sol_flag_e[1] != 0 && s.sol_flag_e[1] != c) {
    s.sol_flag_e[1] = 0;
  }
  if(s.lt5_flag_e && c < 5) {
    s.lt5_flag_e = false;
  }

  s.hand[0] = 0;
  s.hand[1] = 0;
  s.trash[c - 1]++;

  if(max_num[c - 1] <= s.trash[c - 1]) {
    if(s.open_flag_e == c) s.open_flag_e = 0;
    if(s.sol_flag_e[0] == c) s.sol_flag_e[0] = 0;
    if(s.sol_flag_e[1] == c) s.sol_flag_e[1] = 0;
    if(s.sol_flag_s[0] == c) s.sol_flag_s[0] = 0;
    if(s.sol_flag_s[1] == c) s.sol_flag_s[1] = 0;
    if(s.open_flag_s == c) s.open_flag_s = 0;
  }
  return s;
}

int State::trash_and_hand_s(const int i) const {
  return ::trash_and_hand_s(i, hand, trash);
}

int State::deck_or_hand_e(const int i) const {
  return max_num[i] - trash_and_hand_s(i);
}

bool State::hand_e(const int i) const {
  if(open_flag_e > 0) { //手札が確定している場合
    if(open_flag_e == i + 1) {
      return true;
    } else {
      return false;
    }
  } else if(sol_flag_e[0] > 1 && sol_flag_e[0] == i + 1) {
    return false;
  } else if(sol_flag_e[1] > 1 && sol_flag_e[1] == i + 1) {
    return false;
  } else if(lt5_flag_e && i + 1 >= 5) { //前のターンに7を出したときの,5以上
    return false;
  } else if(not7_flag && i + 1 == 7) { //前のターンに5を出したときの,7
    return false;
  } else {
    return deck_or_hand_e(i) > 0;
  }
}

bool State::hand_s(const int i) const {
  if(open_flag_s > 0) { //手札が確定している場合
    if(open_flag_s == i + 1) {
      return true;
    } else {
      return false;
    }
  } else if(sol_flag_s[0] > 1 && sol_flag_s[0] == i + 1) {
    return false;
  } else if(sol_flag_s[1] > 1 && sol_flag_s[1] == i + 1) {
    return false;
  } else if(lt5_flag_s && i + 1 >= 5) { //前のターンに7を出したときの,5以上
    return false;
  } else if(not7_flag && i + 1 == 7) { //前のターンに5を出したときの,7
    return false;
  } else {
    return max_num[i] - trash[i] - (open_e() == i + 1) > 0;
  }
}

int State::open_e() const {
  int card = 0;
  for(int i = 0; i < 8; i++) {
    if(hand_e(i)) {
      if(card == 0) {
        card = i + 1;
      } else {
        return 0;
      }
    }
  }
  return card;
}

int State::open_s() const {
  int card = 0;
  for(int i = 0; i < 8; i++) {
    if(hand_s(i)) {
      if(card == 0) {
        card = i + 1;
      } else {
        return 0;
      }
    }
  }
  return card;
}

bool State::deck(const int i) const {
  int open_card = open_e();
  return deck_or_hand_e(i) > (i + 1 == open_card ? 1 : 0);
}

bool State::in(const int target) const {
  return ::in(hand, target);
}

int State::other(const int target) const {
  return ::other(hand, target);
}

int State::count_deck() const {
  int count = 0;
  for(int i = 0; i < 8; i++) {
    count += deck_or_hand_e(i);
  }
  return count - 1;
}

void State::add_sol_s(int card) {
  if(sol_flag_s[0] == 0) {
    sol_flag_s[0] = card;
  } else {
    if(sol_flag_s[0] < card) {
      sol_flag_s[1] = card;
    } else if(sol_flag_s[0] > card) {
      sol_flag_s[1] = sol_flag_s[0];
      sol_flag_s[0] = card;
    }
  }
}

void State::add_sol_e(int card) {
  if(sol_flag_e[0] == 0) {
    sol_flag_e[0] = card;
  } else {
    if(sol_flag_e[0] < card) {
      sol_flag_e[1] = card;
    } else if(sol_flag_e[0] > card) {
      sol_flag_e[1] = sol_flag_e[0];
      sol_flag_e[0] = card;
    }
  }
}

void State::rm_sol_s(int card) {
  if(sol_flag_s[0] == card) {
    sol_flag_s[0] = sol_flag_s[1];
    sol_flag_s[1] = 0;
  } else if(sol_flag_s[1] == card) {
    sol_flag_s[1] = 0;
  }
}

void State::rm_sol_e(int card) {
  if(sol_flag_e[0] == card) {
    sol_flag_e[0] = sol_flag_e[1];
    sol_flag_e[1] = 0;
  } else if(sol_flag_e[1] == card) {
    sol_flag_e[1] = 0;
  }
}

void State::last2card(int last[2]) const {
  if(count_deck() != 1) return;
  int j = 0, d = 0;
  for(int i = 0; i < 8; i++) {
    d = deck_or_hand_e(i);
    if(d >= 2) {
      last[0] = i + 1;
      last[1] = i + 1;
      break;
    } else if(d > 0) {
      last[j++] = i + 1;
    }
    if(j > 2) return;
  }
}

void State::last3card(int last[3]) const {
  if(count_deck() != 2) return;
  int j = 0, d = 0;
  for(int i = 0; i < 8; i++) {
    d = deck_or_hand_e(i);
    if(d >= 3) {
      last[0] = i + 1;
      last[1] = i + 1;
      last[2] = i + 1;
      break;
    } else if(d >= 2) {
      last[j++] = i + 1;
      last[j++] = i + 1;
    } else if(d > 0) {
      last[j++] = i + 1;
    }
    if(j > 3) return;
  }
}

int State::total_hand_e_count() const {
  int sum = 0;
  for(int j = 0; j < 8; ++j) {
    sum += hand_e(j) * deck_or_hand_e(j);
  }
  return sum;
}

double State::hand_e_prob(const int card_idx) const {
  if(this->open_flag_e > 0) {
    return (this->open_flag_e == card_idx + 1) ? 1.0 : 0.0;
  }
  int total = total_hand_e_count();
  if(total == 0) return 0.0;

  return (double)(hand_e(card_idx) * deck_or_hand_e(card_idx)) / total;
}

#endif
