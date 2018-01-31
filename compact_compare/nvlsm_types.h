#include<iostream>
#include<vector>
#include<utility>

using namespace std;

#define RUN_SIZE 4096

namespace nv_lsm{
    
    class Run {
        public:
            long start;
            long end;
            vector< pair<long, string> > kvArray;
    };

    class Level {
        public:
            int run_count;
            vector<Run> runs;
    };

};


