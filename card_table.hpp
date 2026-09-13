#ifndef CARD_TABLE_HPP
#define CARD_TABLE_HPP
#include <cassert>
#include <compare>
#include <cstdio>
#include <cstdlib>

// 値域違反の報告。constexpr の中から呼ぶので、定数式の文脈で違反すると
// コンパイルエラーになる (リテラルからの構築はそこで捕まる)。実行時に踏むと
// NDEBUG の有無に関係なく落ちる。
[[noreturn]] inline void card_range_error(int v, const char* what) {
  std::fprintf(stderr, "%s: value %d is out of range\n", what, v);
  std::abort();
}

// カード。必ず 1〜8 のいずれか。「無い」状態は表せない。
class Card {
public:
  constexpr explicit Card(int v)
    : v_(v) {
    if(v < 1 || v > 8) card_range_error(v, "Card");
  }
  constexpr int value() const {
    return v_;
  } // 1..8
  constexpr int index() const {
    return v_ - 1;
  } // 0..7、trash[] / max_num[] の添字
  constexpr bool operator==(const Card&) const = default;
  constexpr auto operator<=>(const Card&) const = default;

private:
  int v_;
};

// カード、または「無い / 判明していない」。格納は 0 が「無い」。
class MaybeCard {
public:
  constexpr MaybeCard()
    : v_(0) {}
  constexpr explicit MaybeCard(int v)
    : v_(v) {}
  constexpr MaybeCard(Card c)
    : v_(c.value()) {}
  static constexpr MaybeCard none() {
    return MaybeCard();
  }
  constexpr bool has_value() const {
    return v_ != 0;
  }
  constexpr Card value() const { // has_value() が真のときだけ
    if(v_ == 0) card_range_error(v_, "MaybeCard::value");
    return Card(v_);
  }
  constexpr int raw() const {
    return v_;
  } // 0..8、メモ鍵の詰め込み用
  constexpr bool operator==(const MaybeCard&) const = default;

private:
  int v_;
};

// カードごとの枚数。配列の添字は card - 1。
inline constexpr int max_num[8] = {5, 2, 2, 2, 2, 1, 1, 1};

inline constexpr double table_sign[2] = {1.0, -1.0};
inline constexpr char action_sign[8] = {'0', 'a', 'c', 'd', 'e', 'f', 'g', 'h'};
inline constexpr char card_sign[8][20] = {"兵士", "道化", "騎士", "僧侶", "魔術師", "将軍", "大臣", "姫"};

#endif
