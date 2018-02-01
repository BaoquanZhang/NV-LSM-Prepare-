#include<iostream>
#include<vector>
#include<list>
#include<utility>

using namespace std;

#define RUN_SIZE 4096
#define MAX_ARRAY 3


namespace nv_lsm{
    
    class Run;
    class Level;
    class Seg;

    class Seg {
        
        public:
            list<Run>::iterator run_it;
            int start;
            int end;

            Seg(list<Run>::iterator old_run, int old_start, int old_end);
    };

    class Run {
        public:
            long start;
            long end;
            int array_count;
            int ref_count;
            vector< pair<long, string> > kvArray;
            list<Seg> next;

            Run();
    };

    class Level {
        public:
            int run_count;
            list<Run> runs;

            Level();
    };

    Seg::Seg(list<Run>::iterator old_run, int old_start, int old_end) {
        run_it = old_run;
        start = old_start;
        end = old_end;
    }

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
