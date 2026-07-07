# 共通で使用するソースファイル（bf_position.cpp をここに追加してリンクエラーを回避）
COMMON_SRCS = loveletter.cpp rnd_action_sequense.cpp org_action_sequense.cpp

# 共通で使用するヘッダファイル（依存関係チェック用）
COMMON_HDRS = loveletter.hpp rnd_action_sequense.hpp org_action_sequense.hpp newcfr.hpp bf_position.hpp

# 共通のプリプロセッサ定義（-Dフラグ）
COMMON_DEFS = -DNDEBUG -DUSE_SAME_MOVE -DUSE_GOOD_MOVE -DNUSE_BAD_MOVE

.PHONY: clear clean all

all: cfr cfr0 brorg

# --- CFR系ターゲット (-DCFR を使用) ---

cfr: cfr.cpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 -Wall -O2 $(COMMON_DEFS) -DCFR $(COMMON_SRCS) cfr.cpp -o $@

cfr0: cfr_zero.cpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 -Wall -O2 $(COMMON_DEFS) -DCFR $(COMMON_SRCS) cfr_zero.cpp -o $@

cfrorg: cfr_org.cpp make_infset.hpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 -Wall -O2 $(COMMON_DEFS) -DCFR $(COMMON_SRCS) cfr_org.cpp -o $@

watch: cfr_org.cpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 -Wall -O2 $(COMMON_DEFS) -DCFR $(COMMON_SRCS) watch_cfr.cpp -o $@

# --- Best Response系ターゲット (-DBEST_RESPONSE を使用) ---

brrnd: br_rnd.cpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 -Wall -O2 $(COMMON_DEFS) -DBEST_RESPONSE $(COMMON_SRCS) br_rnd.cpp -o $@

brorg: br.cpp all_elements.hpp infset_dfs.hpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 -Wall -O2 $(COMMON_DEFS) -DBEST_RESPONSE $(COMMON_SRCS) br.cpp -o $@

win: infset_iswin.cpp save_load_abshistory.hpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 -Wall -O2 $(COMMON_DEFS)  $(COMMON_SRCS) infset_iswin.cpp -o $@


comp: compare_abscfr.cpp save_load_abshistory.hpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 -Wall -O2 $(COMMON_DEFS)  $(COMMON_SRCS) compare_abscfr.cpp -o $@

# --- その他 ---
test: test.cpp log_util.hpp bf_position.hpp endgame.hpp
	g++ -std=c++20 -Wall -O2 test.cpp -o $@

end: endgame.cpp bs_set.hpp endgame.hpp
	g++ -std=c++20 -Wall -O2 endgame.cpp -o $@

clear:
	rm -f cfr cfr0 brrnd brorg test win end