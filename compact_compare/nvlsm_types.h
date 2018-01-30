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
            int size;
            vector< pair<long, string> > kvArray;
            Run * next[2];
    };

    class Level {
        public:
            int run_count;
            vector<Run> runs;
    };

};
