#ifndef RND_ACTION_SEQUENSE
#define RND_ACTION_SEQUENSE

#include <string>
#include <cassert>
#include <tuple>
#include <iostream>

class rnd_action_sequense {
    private :
      std::string m_s;
    public :
      rnd_action_sequense() noexcept : m_s ("") {}
      void push(const char *p, size_t len) noexcept;
      void erase() noexcept {
        //行動を1つ削除
        assert(!m_s.empty());
        m_s.pop_back();
      }
      void set(const std::string &s) noexcept { m_s = s; }
      std::string get_hash_value() const noexcept { return m_s; }
};

#endif