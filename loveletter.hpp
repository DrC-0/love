#ifndef LOVELETTER
#define LOVELETTER

#ifdef USE_GOOD_MOVE //必勝検出
// g++ -DUSE_GOOD_MOVE ...
constexpr bool use_good_move = true;
#else
// g++ ...
constexpr bool use_good_move = false;
#endif

#ifdef NUSE_BAD_MOVE //必敗検出
// g++ -DNUSE_BAD_MOVE ...
constexpr bool nuse_bad_move = true;
#else
// g++ ...
constexpr bool nuse_bad_move = false;
#endif

#ifdef USE_SAME_MOVE //同手札検出
// g++ -DUSE_SAME_MOVE ...
constexpr bool use_same_move = true;
#else
// g++ ...
constexpr bool use_same_move = false;
#endif

#ifdef ALL_ELEMENTS_TEST //情報集合の全節点列挙テスト
constexpr bool all_elements_test = true;
#else
// g++ ...
constexpr bool all_elements_test = false;
#endif

enum { n1 = 5, n2 = 2, n3 = 2, n4 = 2, n5 = 2, n6 = 1, n7 = 1, n8 = 1};
//enum { n1 = 1, n2 = 1, n3 = 1, n4 = 1, n5 = 2, n6 = 1, n7 = 1, n8 = 1}; //mini
class infset{
  private:
    float prob_action;
    float pi_i; //can be removed
#ifdef CFR
    float sum_i[2];
    float regret[2]; /**/
#endif
#ifdef BEST_RESPONSE
    float exp_reward[2]; //can be 半分
#endif
#ifdef CFR_SELF_PLAY
    unsigned int sum_i[2];
    float value[2];
#endif
  public:
    void set_prob_action(double v) noexcept { prob_action = (float)v; }
    double get_prob_action() const noexcept { return (double)prob_action; }
    void set_pi_i(double v) noexcept { pi_i = (float)v; }
    double get_pi_i() const noexcept { return (double)pi_i; }
#ifdef CFR
    void set_sum_i(int i, double v) noexcept { sum_i[i] = (float)v; }
    double get_sum_i(int i) const noexcept { return (double)sum_i[i]; }
    void set_regret(int i, double v) noexcept { regret[i] = (float)v; } /**/
    double get_regret(int i) const noexcept { return (double)regret[i]; } /**/
#endif
#ifdef BEST_RESPONSE
    void set_exp_reward(int i, double v) noexcept { exp_reward[i] = (float)v; }
    double get_exp_reward(int i) const noexcept { return (double)exp_reward[i]; }
#endif
#ifdef CFR_SELF_PLAY
    void add_sum_i(int i) noexcept { sum_i[i]++; }
    void set_sum_i(int i, unsigned int v) noexcept { sum_i[i] = v; }
    unsigned int get_sum_i(int i) const noexcept { return sum_i[i]; }
    void set_value(int i, double v) noexcept { value[i] = (float)v; } /**/
    double get_value(int i) const noexcept { return (double)value[i]; } /**/
#endif
#ifdef ALL_ELEMENTS_TEST
    std::vector<std::string> history; //can be removed
#endif
    int depth; //can be removed
    //int player; //can be removed
    bool play; //can be removed
    bool wizard; //can be removed
#ifdef CHECK_STR
    bool wizard_with_general;
    bool wizard_with_princess;
    bool general0;
    bool general1;
    bool minister0;
    bool minister1;
#endif
#ifdef CFR
     infset()
      : prob_action (0.5), pi_i (-1.0), sum_i {0.0, 0.0}, regret {0.0, 0.0}, depth (0), play (false), wizard (false) {}
#elif BEST_RESPONSE
     infset()
      : prob_action (0.5), pi_i (-1.0), exp_reward {0.0, 0.0}, depth (0), play (false), wizard (false) {}
#elif CFR_SELF_PLAY
     infset()
      : prob_action (0.5), pi_i (-1.0), sum_i {0, 0}, value {0.0, 0.0}, depth (0), play (false), wizard (false) {}
#else
     infset()
      : prob_action (0.5), pi_i(-1.0), depth (0), play (false), wizard (false) {}
#endif
} ;

struct work_do_action{
    std::map<std::string, infset>::iterator infset_it;
    double randsol_wprob;
    int prev_play;
    int prev_hand1;
    int prev_hand2;
    int used_open1;
    int prev_open1;
    int prev_open2;
    bool prev_barrier1;
    bool prev_barrier2;
    work_do_action()
      : randsol_wprob (0.0),  used_open1 (0) {}
} ;

struct node{
    std::vector<int> flow;
    rnd_action_sequense rnd_his;
    rnd_action_sequense rnd_his_p[2];
    org_action_sequense org_his;
    org_action_sequense org_his_p[2];
    double npi[3][32];
    int next_f;
    int depth;
    int hand1[2];
    int hand2;
    int open1;
    int open2;
    int deck[8];
    int open[3];
    int hide;
    int turn;
    int count_turn;
    bool barrier1;
    bool barrier2;
    node()
      : next_f (0), depth (0), hand1 {0, 0}, hand2 (0), open1 (0), open2 (0), deck {n1, n2, n3, n4, n5, n6, n7, n8}, open {0, 0, 0}, hide (0), turn (0), count_turn (0), barrier1 (false), barrier2 (false) {}

    node(const int input_open[3]);
    node(const std::string &h, bool rnd, const int input_open[3]);
    int dsum()const noexcept{
      int a = std::accumulate(std::begin(deck), std::end(deck), 0);
      return a;
    }
    double dsum_rec()const noexcept{
      return 1.0 / this->dsum();
    }
    void do_action(int a, int c, work_do_action &w);
    void undo_action(int a, int c, work_do_action &w);
    bool valid_data()const noexcept;
    bool operator == (const node &n)const noexcept;
    void print_history()const noexcept;
    void print_history_rnd()const noexcept;
    void print_node();
} ;

#endif
