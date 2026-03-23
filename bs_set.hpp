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

// #include "bf_position.hpp"
// #include "log_util.hpp"
// #include "endgame.hpp"


struct State {
    bool barrier;
    bool not7_flag;
    bool lt5_flag_s;
    bool lt5_flag_e;
    int open_flag_s; // 0-8
    int open_flag_e; // 0-8
    int sol_flag_s;  // 0, 2-8
    int sol_flag_e;  // 0, 2-8
    int hand[2];     // [min, max]
    int trash[8];   // 各カードの残り枚数
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
    void print()const;
};

const int TRASH_BASES[8] = {6, 3, 3, 3, 3, 2, 2, 2};
const int TRASH_MAX = 3888; // 6*3*3*3*3*2*2*2
const int HAND_MAX = 36;
const int COMPLEX_SELF_MAX = 18;
const int COMPLEX_ENEMY_MAX = 41;

// 手札 {h0, h1} -> 0~35
int encode_hand_idx(int h0, int h1) {
    if (h0 > h1) std::swap(h0, h1);
    int idx = 0;
    for (int i = 1; i <= 8; ++i) {
        for (int j = i; j <= 8; ++j) {
            if (i == h0 && j == h1) return idx;
            idx++;
        }
    }
    return -1;
}

// 0~35 -> 手札 {h0, h1}
void decode_hand_idx(int idx, int hand_out[2]) {
    int current = 0;
    for (int i = 1; i <= 8; ++i) {
        for (int j = i; j <= 8; ++j) {
            if (current == idx) {
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
    for (int i = 0; i < 8; ++i) {
        idx += trash[i] * multiplier;
        multiplier *= TRASH_BASES[i];
    }
    return idx;
}

// 0~3887 -> 配列 trash[8]
void decode_trash_idx(int idx, int trash_out[8]) {
    for (int i = 0; i < 8; ++i) {
        trash_out[i] = idx % TRASH_BASES[i];
        idx /= TRASH_BASES[i];
    }
}

inline int map_sol(int sol_flag) {
    assert(sol_flag != 1 && sol_flag >= 0 && sol_flag <= 8);
    return (sol_flag == 0) ? 0 : (sol_flag - 1);
}

inline int unmap_sol(int val) {
    return (val == 0) ? 0 : (val + 1);
}

int encode_self(int open_s, int sol_s, bool lt5_s, const int hand[2]) {
    if (open_s != 0) {
        // Openがある場合: 値そのものではなく、Handのどちらと一致するかを見る
        if (open_s == hand[0]) {
            return 16; // Open Hand[0]
        } else if (open_s == hand[1]) {
            return 17; // Open Hand[1]
        } else {
            // エラー: Openカードが手札にない
            assert(false && "Open card not found in hand!");
            return -1;
        }
    } else {
        // [0 - 15]: open_s == 0
        // lt5_s (2通り) * sol_s (8通り) = 16通り
        return map_sol(sol_s) + (lt5_s ? 8 : 0);
    }
}

void decode_self(int val, int& open_s, int& sol_s, bool& lt5_s, const int hand[2]) {
    // 初期化
    open_s = 0; sol_s = 0; lt5_s = false;

    if (val == 16) {
        // Case: Open Hand[0]
        open_s = hand[0];
    }
    else if (val == 17) {
        // Case: Open Hand[1]
        open_s = hand[1];
    }
    else {
        // Case: Open == 0 (0~15)
        lt5_s = (val >= 8); // 上位ビット
        sol_s = unmap_sol(val % 8);
    }
}

int encode_enemy(bool barrier, int open_e, int sol_e, bool lt5_e, bool not7) {
    if (open_e != 0) {
        // [0 - 15]: open_e != 0 (lt5_e, not7, sol_e無視)
        // barrier(2) * open_e(8) = 16通り
        int b_bit = barrier ? 8 : 0;
        return (open_e - 1) + b_bit;
    }
    else {
        // open_e == 0
        if (barrier) {
            // [16 - 23]: Barrier ON (lt5_e=F, not7=F 固定/無視)
            // sol_e(8)
            return 16 + map_sol(sol_e);
        }
        else {
            // Barrier OFF
            if (lt5_e) {
                // [24]: Lt5 ON (sol_e, not7 無視) ★変更箇所
                // ここは sol_e を保存せず、単一のIDになります
                return 24;
            }
            else {
                // [25 - 40]: Lt5 OFF
                // not7(2) * sol_e(8) = 16通り
                int n7_bit = not7 ? 8 : 0;
                return 25 + map_sol(sol_e) + n7_bit;
            }
        }
    }
}

void decode_enemy(int val, bool& barrier, int& open_e, int& sol_e, bool& lt5_e, bool& not7) {
    // 初期化
    barrier = false; open_e = 0; sol_e = 0; lt5_e = false; not7 = false;

    if (val < 16) {
        // [0-15] Case: Open != 0
        open_e = (val % 8) + 1;
        barrier = (val >= 8);
    }
    else if (val < 24) {
        // [16-23] Case: Open=0, Barrier=ON
        barrier = true;
        sol_e = unmap_sol(val - 16);
    }
    else if (val == 24) {
        // [24] Case: Open=0, Barrier=OFF, Lt5=ON ★変更箇所
        lt5_e = true;
        // sol_e, not7 は初期値(0, false)のまま
    }
    else {
        // [25-40] Case: Open=0, Barrier=OFF, Lt5=OFF
        int temp = val - 25; // オフセットが25になります
        not7 = (temp >= 8);
        sol_e = unmap_sol(temp % 8);
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

State standardize(const State& s){
    return decode_hash(get_hash(s));
}

void State::print() const {
    // --- 基本情報 ---
    std::cout << "Hand:  [" << hand[0] << ", " << hand[1] << "]" << std::endl;

    std::cout << "Trash: [";
    for(int i=0; i<8; ++i) {
        std::cout << trash[i] << (i==7 ? "" : ",");
    }
    std::cout << "]     deck: " << count_deck() << std::endl;

    // --- Self (自分) の状態 ---
    // ルール: Open_S != 0 なら Lt5_S, Sol_S は無視される
    std::cout << "[Self]  ";
    if (open_flag_s != 0) {
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

    if (open_flag_e != 0) {
        // Case 1: Openがある場合
        // Barrierの情報はEncodeに含まれるが、Lt5/Not7/Solは無視される
        std::cout << "Open:" << open_flag_e << " | ";
        std::cout << "Barrier:" << (barrier ? "ON " : "OFF");
        std::cout << " (Override Lt5/Not7/Sol)";
    }
    else {
        std::cout << "Open:- | ";

        if (barrier) {
            // Case 2: Barrierがある場合
            // Solだけ有効、Lt5/Not7は無視
            std::cout << "Barrier:ON | ";
            std::cout << "Sol:" << sol_flag_e;
            std::cout << " (Override Lt5/Not7)";
        }
        else {
            std::cout << "Barrier:OFF | ";

            if (lt5_flag_e) {
                // Case 3: Lt5がある場合
                // Sol有効、Not7は無視
                std::cout << "Lt5:ON | ";
                std::cout << "Sol:" << sol_flag_e;
                std::cout << " (Override Not7)";
            }
            else {
                // Case 4: フルセット (Lt5なし)
                // Sol, Not7 有効
                std::cout << "Lt5:OFF | ";
                std::cout << "Sol:" << sol_flag_e << " | ";
                std::cout << "Not7:" << (not7_flag ? "T" : "F");
            }
        }
    }
    std::cout << std::endl << std::endl;
}

// 1つの状態 s の中で、有効になっているフラグの合計数を返す
int count_active_flags(const State& s) {
    int count = 0;

    if (s.barrier) count++;
    if (s.lt5_flag_s) count++;
    if (s.lt5_flag_e) count++;
    if (s.not7_flag) count++;

    // open, sol は int なので 0 より大きいか判定
    if (s.open_flag_s > 0 || s.open_flag_e > 0) count++;
    if (s.sol_flag_s > 0) count++;
    if (s.sol_flag_e > 0) count++;

    return count;
}

bool in(const int hand[2], const int target) {
  return hand[0] == target || hand[1] == target;
}

int other(const int hand[2], const int target) {
  if (hand[0] == target) return hand[1];
  else if (hand[1] == target) return hand[0];
  return 0;
}

int trash_and_hand_s(const int i, const int hand[2], const int trash[8]){
  return trash[i] + (hand[0]  == i + 1 ? 1 : 0) + (hand[1]  == i + 1 ? 1 : 0);
}

State State::update_flag_discard(int c) const {
    State s;

    s.lt5_flag_s  = this->lt5_flag_e;
    s.lt5_flag_e  = this->lt5_flag_s;
    s.open_flag_s = this->open_flag_e;
    s.open_flag_e = this->open_flag_s;
    s.sol_flag_s  = this->sol_flag_e;
    s.sol_flag_e  = this->sol_flag_s;

    s.barrier   = false;
    s.not7_flag = false;

    for(int i = 0; i < 8; ++i) s.trash[i] = this->trash[i];


    if(s.open_flag_e > 0 && s.open_flag_e == c){
        s.open_flag_e = 0;
    }
    if(s.sol_flag_e > 1 && s.sol_flag_e != c){
        s.sol_flag_e = 0;
    }
    if(s.lt5_flag_e && c < 5){
        s.lt5_flag_e = false;
    }


    s.hand[0] = 0;
    s.hand[1] = 0;
    s.trash[c-1]++;

    if(max_num[c-1] <= s.trash[c-1]){
        if(s.open_flag_e == c) s.open_flag_e = 0;
        if(s.sol_flag_e == c) s.sol_flag_e = 0;
    }
    if(max_num[c-1] <= s.trash[c-1]){
        if(s.sol_flag_s == c) s.sol_flag_s = 0;
        if(s.open_flag_s == c) s.open_flag_s = 0;
    }
    return s;
}

int State::trash_and_hand_s(const int i) const{
    return ::trash_and_hand_s(i, hand, trash);
}

int State::deck_or_hand_e(const int i) const {
  return max_num[i] - trash_and_hand_s(i);
}

bool State::hand_e(const int i) const {
  if(open_flag_e > 0){//手札が確定している場合
    if(open_flag_e == i + 1){
      return true;
    }else{
      return false;
    }
  }else if(sol_flag_e > 1 && sol_flag_e == i + 1){
    return false;
  }else if(lt5_flag_e && i + 1 >= 5){//前のターンに7を出したときの,5以上
    return false;
  }else if(not7_flag && i + 1 == 7){//前のターンに5を出したときの,7
    return false;
  }else{
    return deck_or_hand_e(i) > 0;
  }
}

bool State::hand_s(const int i) const {
  if(open_flag_s > 0){//手札が確定している場合
    if(open_flag_s == i + 1){
      return true;
    }else{
      return false;
    }
  }else if(sol_flag_s > 1 && sol_flag_s == i + 1){
    return false;
  }else if(lt5_flag_s && i + 1 >= 5){//前のターンに7を出したときの,5以上
    return false;
  }else if(not7_flag && i + 1 == 7){//前のターンに5を出したときの,7
    return false;
  }else{
    return max_num[i] - trash[i] - (open_e() == i+1) > 0;
  }
}

int State::open_e() const {
  int card = 0;
  for (int i = 0; i < 8; i++){
    if(hand_e(i)){
      if(card == 0){
        card = i+1;
      } else {
        return 0;
      }
    }
  }
  return card;
}

int State::open_s() const {
  int card = 0;
  for (int i = 0; i < 8; i++){
    if(hand_s(i)){
      if(card == 0){
        card = i+1;
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

int State::count_deck() const{
  int count = 0;
  for(int i = 0; i < 8; i++){
    count += deck_or_hand_e(i);
  }
  return count -1;
}

void State::last2card(int last[2]) const {
    if(count_deck() != 1) return;
    int j = 0, d = 0;
    for(int i = 0; i < 8; i++){
        d = deck_or_hand_e(i);
        if(d >= 2){
            last[0] = i + 1;
            last[1] = i + 1;
            break;
        }else if (d > 0){
            last[j++] = i + 1;
        }
        if(j > 2) return;
    }
}

void State::last3card(int last[3]) const {
    if(count_deck() != 2) return;
    int j = 0, d = 0;
    for(int i = 0; i < 8; i++){
        d = deck_or_hand_e(i);
        if(d >= 3){
            last[0] = i + 1;
            last[1] = i + 1;
            last[2] = i + 1;
            break;
        }else if(d >= 2){
            last[j++] = i + 1;
            last[j++] = i + 1;
        }else if (d > 0){
            last[j++] = i + 1;
        }
        if(j > 3) return;
    }
}

int State::total_hand_e_count() const {
    int sum = 0;
    for (int j = 0; j < 8; ++j) {
        sum += hand_e(j) * deck_or_hand_e(j);
    }
    return sum;
}

double State::hand_e_prob(const int card_idx) const {
    if (this->open_flag_e > 0) {
        return (this->open_flag_e == card_idx + 1) ? 1.0 : 0.0;
    }
    int total = total_hand_e_count();
    if (total == 0) return 0.0;

    return (double)(hand_e(card_idx) * deck_or_hand_e(card_idx)) / total;
}

#endif
