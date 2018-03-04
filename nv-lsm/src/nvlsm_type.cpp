#include "../include/nvlsm_types.h"

using namespace std;
using namespace nv_lsm;

/* Implementations for PlsmStore */

PlsmStore::PlsmStore(int level_base_val, int level_ratio_val) {
    level_base = level_base_val;
    level_ratio = level_ratio_val;
    memTable = new MemTable();
}

PlsmStore::~PlsmStore(){}

/* insert a key value pair to memory buffer */
void PlsmStore::put(string key, string  value) {
    if (memTable->buffer == NULL) {
        memTable->buffer = new vector< pair<string, string> >();
        memTable->buffer->reserve(RUN_SIZE);
    }
    
    /* put kv pair into memory buffer */
    memTable->buffer->insert(memTable->buffer->end(), make_pair(key, value));

    int len = memTable->buffer->size();

    if (len == RUN_SIZE) {
        /* put memory buffer into persist queue 
         * and allocate new buffer */
        memTable->persist_queue.insert(memTable->persist_queue.end(), memTable->buffer);
        memTable->buffer = new vector< pair<string, string> >();
        memTable->buffer->reserve(RUN_SIZE);
    }
    return;
}

void PlsmStore::persist_memTable(MemTable * memTable, list<Level *> levels) {
    
}

string PlsmStore::get(string key) {
}

vector< pair<string, string> > PlsmStore::range(string start, string end) {
}

void PlsmStore::normal_compaction(Level * up_level, Level * bottom_level) {
}

void PlsmStore::lazy_compaction(Level * up_level, Level * bottom_level) {
}

/* Implementations for MemTable */
MemTable::MemTable() {}
MemTable::~MemTable() {
    delete(buffer);
}

/* Implementations for Level */ 

Level::Level(int id) {
    run_count = 0;
    level_id = id;
}

Level::~Level() {}

/* Implementations for Run */

Run::Run() {
    array_count = 0;
    ref_count = 0;
    start = 0;
    end = 0;
}

Run::~Run() {}

/* Implementaions for Segment */

Seg::Seg(list<Run>::iterator old_run, int old_start, int old_end) {
    run_it = old_run;
    start = old_start;
    end = old_end;
}

Seg::~Seg() {}
