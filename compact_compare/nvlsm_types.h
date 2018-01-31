#include<iostream>
#include<vector>
#include<utility>

using namespace std;

#define RUN_SIZE 4096
#define MAX_ARRAY 3

namespace nv_lsm{
    
    class Run {
        public:
            long start;
            long end;
            int array_count;
            int ref_count;
            vector< pair<long, string> > kvArray;
            vector< pair<Run*, pair<int, int> > > next;

            Run();
    };

    class Level {
        public:
            int run_count;
            vector<Run> runs;

            Level();
    };

    Run::Run() {
        array_count = 1;
        ref_count = 0;
        start = 0;
        end = 0;
    }

    Level::Level() {
        run_count = 0;
    }
};
