#ifndef _NVLSM_TYPE_H_
#define _NVLSM_TYPE_H_

#include<vector>
#include<list>
#include <unistd.h>
#include<utility>
#include<string>
#include <pthread.h>
/* pmdk headers */
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace std;
/* pmdk namespace */
using namespace pmem::obj;

#define RUN_SIZE 4096
#define MAX_ARRAY 3
#define LAYOUT "plsmStore"
#define KEY_SIZE 16
#define VALUE_SIZE 128
#define LEVEL_NUM 3

namespace nv_lsm {
    
    class PlsmStore;
    class Run;
    class Level;
    class Seg;
    class MemTable;

    struct LSM_Root {                                       // persistent root object
        persistent_ptr<Level> head;                         // head of the vector of levels
    };

    struct KVPair {
        char key[KEY_SIZE];
        char value[VALUE_SIZE];
    };

    class PlsmStore {
        private:
            pthread_t persist_thread_id;
            /* internal operations */
            void normal_compaction(Level * up_level, Level * bottom_level);
            void lazy_compaction(Level * up_level, Level * bottom_level);
            static bool compare(pair<string, string> kv1, pair<string, string> kv2);

        public:
            bool start_persist;
            p<int> level_base;
            p<int> level_ratio;
            p<int> level_num;
            persistent_ptr<Level> level_head;
            persistent_ptr<Level> level_tail;
            MemTable * memTable;

            /* interface */
            void put(string key, string value);
            string get(string key);
            vector< pair<string, string> > range(string start_key, string end_key);

            PlsmStore(const string &path, const size_t size, int level_base_val, int level_ratio_val);
            ~PlsmStore();
    };

    /* Data structure in DRAM only */
    class MemTable {
        public:
            vector< pair<string, string> > * buffer;
            list< vector< pair<string, string> > * > persist_queue;
            MemTable();
            ~MemTable();
    };

    /* Data structure in pmem */
    class Level {
        public:
            p<int> level_id;
            p<int> run_count;
            persistent_ptr<Run> run_head;
            persistent_ptr<Run> run_tail;
            persistent_ptr<Level> next_level;
            persistent_ptr<Level> pre_level;

            Level(int id);
            ~Level();
    };

    class Run {
        public:
            persistent_ptr<string> start;
            persistent_ptr<string>  end;
            persistent_ptr<KVPair[]> local_array;
            persistent_ptr<Run> next_run;
            persistent_ptr<Run> pre_run;
            Run();
            Run(vector< pair<string, string> > * array);
            ~Run();
    };

};
#endif // _NVLSM_TYPE_H_
