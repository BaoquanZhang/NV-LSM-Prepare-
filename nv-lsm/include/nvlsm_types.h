#ifndef _NVLSM_TYPE_H_
#define _NVLSM_TYPE_H_

#include<vector>
#include<list>
#include<utility>
#include<string>

using namespace std;

#define RUN_SIZE 4096
#define MAX_ARRAY 3

namespace nv_lsm{
    
    class PlsmStore;
    class Run;
    class Level;
    class Seg;
    class MemTable;

    class PlsmStore {
        public:
            int level_base;
            int level_ratio;
            list<Level *> levels;
            MemTable * memTable;
            
            /* internal operations */
            void persist_memTable(MemTable* memTable, list<Level *> levels);
            void normal_compaction(Level * up_level, Level * bottom_level);
            void lazy_compaction(Level * up_level, Level * bottom_level);

            /* interface */
            void put(string key, string value);
            string get(string key);
            vector< pair<string, string> > range(string start, string end);

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
            long start;
            long end;
            int array_count;
            int ref_count;
            vector< pair<string, string> > kvArray;
            list<Seg> next;

            Run();
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
