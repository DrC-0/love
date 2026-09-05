#include "org_action_sequense.hpp"
#include "action_code.hpp"

void org_action_sequense::push(const char *p, size_t len) noexcept {
  //行動を1つ追加
  assert(oph.get_hash(p, len) <= ORG_MAX_HASH_VALUE);
  m_s.push_back((unsigned char)oph.get_hash(p, len));
}