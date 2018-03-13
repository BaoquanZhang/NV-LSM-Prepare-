#ifndef _NVLSM_TYPE_H_
#define _NVLSM_TYPE_H_

#include <vector>
#include <list>
#include <utility>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace std;

using namespace pmem::obj;

#define RUN_SIZE 4096
#define MAX_ARRAY 3
#define LAYOUT "plsmStore"
#define KEY_SIZE 16
#define VALUE_SIZE 128
 
namespace nv_lsm {
    
    class PlsmStore;
    class Run;
    class Level;
    class Seg;
    class MemTable;
    struct KVPair;
    struct KVPair;

    struct LSM_Root {
        persistent_ptr<Level> p_start_level;
    };

    struct KVPair { // kv pairs in DRAM
        char key[KEY_SIZE];
        char value[VALUE_SIZE];
    };

    class PlsmStore {
        private:
            pthread_t persist_thread_id;
            /* internal operations */
            void normal_compaction(Level * up_level, Level * bottom_level);
            void lazy_compaction(Level * up_level, Level * bottom_level);
            static bool compare(KVPair &kv1, KVPair &kv2);

        public:
            bool start_persist;
            p<int> level_base;
            p<int> level_ratio;
            persistent_ptr<Level> level0;
            MemTable * memTable;

            /* interface */
            void put(char * key, char * value);
            string get(char * key);
            vector<KVPair> range(char * start_key, char * end_key);

            PlsmStore(const string &path, size_t size, int level_base_val, int level_ratio_val);
            ~PlsmStore();
    };

    /* MemTable is structure in DRAM */


    class MemTable { 
        public:
            vector<KVPair> * buffer;
            list< vector<KVPair> * > persist_queue;
            MemTable();
            ~MemTable();
    };

    /* Following are in Persistent Memory */

    /* Level includes multiple runs */
    class Level {
        public:
            int level_id;
            int run_count;
            persistent_ptr<Run> runs;         // list of runs in current level
            persistent_ptr<Run> runs_head;    // head of the list of runs in current level
            persistent_ptr<Run> runs_tail;    // tail of the list of runs in current level
            persistent_ptr<Level> next_level; // pointer to next level
            Level(int id);
            ~Level();
    };

    /* Run includes one kv array and one or more memory segmemts */
    class Run {
        public:
            persistent_ptr<char> start;       // start key of current run
            persistent_ptr<char> end;         // end key of current run
            p<int> seg_count;                 // the number of segments included
            p<int> ref_count;              
            persistent_ptr<KVPair[]> kv_array;  
            persistent_ptr<Seg> seg_list;     // head of segment list,
                                              // segment list will be only used in lazy compaction
            persistent_ptr<Run> next_run;
            persistent_ptr<Run> pre_run;
            Run();
            ~Run();
    };

    /* A memory segment includes one kv array 
     * Seg will be only used for lazy compaction*/
    class Seg {
        public:
            persistent_ptr<Run> run_it;      // points to the run that it belongs to
            p<int> offset_start;             // offset in the kv array of the run
            p<int> offset_end;
            persistent_ptr<Seg> next_seg;
            Seg(Run * src_run, int offset_start, int offset_end);
            ~Seg();
    };



};
#endif // _NVLSM_TYPE_H_
