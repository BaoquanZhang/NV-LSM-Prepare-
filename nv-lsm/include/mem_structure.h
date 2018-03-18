#ifndef _MEM_STRUCTURE_H_
#define _MEM_STRUCTURE_H_
#include <vector>
#include <list>
#include <unistd.h>
#include <utility>
#include <string>
#include "global_conf.h"
#include <libpmemobj++/persistent_ptr.hpp>

using namespace std;
using namespace pmem::obj;


namespace nv_lsm {

    /* Data structure in DRAM only */
    class Run;

    class MemTable {
        public:
            vector< pair<string, string> > * buffer;
            list< vector< pair<string, string> > * > persist_queue;
            MemTable();
            ~MemTable();
    };

    struct KeyRange {
        string start_key;
        string end_key;
        KeyRange(string key1, string key2) {
            start_key = key1;
            end_key = key2;
        }

        KeyRange(int size) {
            start_key.reserve(size);
            end_key.reserve(size);
        }
    };

    class MetaTable {
        public:
            KeyRange * buffer_range; // key ranges for buffer
            list<KeyRange *> queue_range; // key ranges for persist list
            vector< vector< pair<KeyRange, persistent_ptr<Run> > > > level_range; // key ranges for level
            MetaTable();
            ~MetaTable();

    };

};

#endif // _MEM_STRUCTURE_H_

