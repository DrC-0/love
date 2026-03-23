#!/bin/bash
g++ -std=c++11 -DNDEBUG -DUSE_SAME_MOVE -DUSE_GOOD_MOVE -DNUSE_BAD_MOVE -DCFR -Wall -O2 loveletter.cpp org_action_sequense.cpp rnd_action_sequense.cpp cfr.cpp -o cfr
g++ -std=c++11 -DNDEBUG -DUSE_SAME_MOVE -DUSE_GOOD_MOVE -DNUSE_BAD_MOVE -DCFR -Wall -O2 loveletter.cpp org_action_sequense.cpp rnd_action_sequense.cpp cfr_zero.cpp -o cfr0
g++ -std=c++11 -DNDEBUG -DUSE_SAME_MOVE -DUSE_GOOD_MOVE -DNUSE_BAD_MOVE -DBEST_RESPONSE -Wall -O2 loveletter.cpp org_action_sequense.cpp rnd_action_sequense.cpp br_rnd.cpp -o brrnd
g++ -std=c++11 -DNDEBUG -DUSE_SAME_MOVE -DUSE_GOOD_MOVE -DNUSE_BAD_MOVE -DBEST_RESPONSE -Wall -O2 loveletter.cpp org_action_sequense.cpp rnd_action_sequense.cpp br.cpp -o brorg
cat all_subgames.txt | xargs -n 3 -P 12 ./cfrbr.sh