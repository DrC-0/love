#include "rnd_action_sequense.hpp"
#include "rnd_action.hpp"

static Rnd_Perfect_Hash rph;

void rnd_action_sequense::push(const char *p, size_t len) noexcept {
    //行動を1つ追加
    assert(rph.get_hash(p, len) <= RND_MAX_HASH_VALUE);
    m_s.push_back((unsigned char)rph.get_hash(p, len));
    //m_s += (unsigned char)ph.get_hash(p, len);
}