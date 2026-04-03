using namespace std;
#include "endgame.hpp"
void win_percent_check(){
    int match_count = 0;
    int match_by_depth[16] = {};
    for(int hash=0; hash< Total_hash_space; hash++){
        State s = decode_hash(hash);
        if(!is_legal_state(s)) continue;
        int deck = s.count_deck();
        match_by_depth[deck]++;
        match_count++;
        if(deck == 10) s.print();
    }
    cout << "match_by_depth" << endl;
    for(int x: match_by_depth){
        cout << x << " ";
    }
    cout << endl;
    cout << "match:" << match_count << endl;
}

void make_buckets(){// データ構造
    vector<int> depth_buckets[BUCKET_COUNT];
    int total_hash_space = TRASH_MAX * HAND_MAX * COMPLEX_SELF_MAX * COMPLEX_ENEMY_MAX;
    int cnt = 0;
    for (int h = 0; h < total_hash_space; ++h) {
        State s = decode_hash(h);
        if (!is_legal_state(s)) continue;

        int d = s.count_deck();
        if (d >= 0 && d < BUCKET_COUNT) {
            depth_buckets[d].push_back(h);
        }
        cnt++;
    }
    save_buckets(depth_buckets, BUCKET_COUNT);
    cout << "count : " << cnt << endl;
}

int depth = 10;
void make_reward_table(){
    vector<int> depth_buckets[BUCKET_COUNT];
    load_buckets(depth_buckets, BUCKET_COUNT);
    for(int d=1; d<=depth; d++){
        for (int h : depth_buckets[d]) {
            reward_table[h] = exp_reward(h);
        }
        cout << "end: " << d << endl;
    }
    save_reward_table();
}

bool is_searching(const State& s){
    if( s.count_deck() == 6 &&
        s.barrier &&
        s.in(4) &&
        s.open_e() == 8 &&
        s.in(5)
    ) return true;
    return false;
}

void search_hash(){
    load_reward_table();
    for(int hash=0; hash< Total_hash_space; hash++){
        State s = decode_hash(hash);
        if(!is_legal_state(s)) continue;
        if(is_searching(s)){
            if(exp_reward(hash) != 1)  cout << hash << " ";
        }
    }
    cout << endl;
}

void debug_hash(){
    load_reward_table();
    commentable = true;
    vector<int> hashs = {38298009, 42718719, 17668336};
    for (int h : hashs) {
        State s = decode_hash(h);
        cout << "hash:" << h << " deck:" << s.count_deck() << endl;
        s.print();
        cout << "legal:" << is_legal_state(s) << endl;
        // print_game_result_prob(s,Sol_8);
        cout << exp_reward(h) << endl;
    }
}

void abswin_check(){
    int hash = 62983619;
    State s = decode_hash(hash);
    s.print();
    abs_win(s);
}

int main() {
    // win_percent_check();
    // make_buckets();
    // make_reward_table();
    // search_hash();
    // debug_hash();
    abswin_check();
    return 0;
}