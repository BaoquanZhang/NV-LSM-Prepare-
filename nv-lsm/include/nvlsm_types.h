#ifndef _NVLSM_TYPE_H_
#define _NVLSM_TYPE_H_

#include<vector>
#include<list>
#include<utility>
#include<string>
#include <pthread.h>

using namespace std;

#define RUN_SIZE 4096
#define MAX_ARRAY 3
#define LAYOUT "plsmStore"

namespace nv_lsm {
    
    class PlsmStore;
    class Run;
    class Level;
    class Seg;
    class MemTable;

    class PlsmStore {
        private:
            pthread_t persist_thread_id;
            /* internal operations */
            void normal_compaction(Level * up_level, Level * bottom_level);
            void lazy_compaction(Level * up_level, Level * bottom_level);
            static bool compare(pair<string, string> kv1, pair<string, string> kv2);

        public:
            bool start_persist;
            int level_base;
            int level_ratio;
            vector<Level> levels;
            MemTable * memTable;

            /* interface */
            void put(string key, string value);
            string get(string key);
            vector< pair<string, string> > range(string start_key, string end_key);

            PlsmStore(int level_base_val, int level_ratio_val);
            ~PlsmStore();
    };

    class MemTable {
        public:
            vector< pair<string, string> > * buffer;
            list< vector< pair<string, string> > * > persist_queue;
            MemTable();
            ~MemTable();
    };

    class Seg {
        public:
            list<Run>::iterator run_it;
            int start;
            int end;

            Seg(list<Run>::iterator old_run, int old_start, int old_end);
            ~Seg();
    };

    class Run {
        public:
            string start;
            string  end;
            int array_count;
            int ref_count;
            vector< pair<string, string> > kvArray;
            list<Seg> next;

            Run();
            Run(vector< pair<string, string> > * array);
            ~Run();
    };

    class Level {
        public:
            int level_id;
            int run_count;
            list<Run> runs;

            Level(int id);
            ~Level();
    };

};
#endif // _NVLSM_TYPE_H_
