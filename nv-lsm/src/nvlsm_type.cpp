#include "nvlsm_types.h"
#include <algorithm>
#include <mutex>
#include <iostream>

using namespace std;
using namespace nv_lsm;

mutex g_persist_list_mutex;
mutex g_start_persist_mutex;

/* This function will run in a sub-thread
 * 1. keep checking persist_queue with an infinite loop
 * 2. copy array from persist_queue to level0
 * */
void * persist_memTable(void * virtual_p) {
    PlsmStore * plsmStore = (PlsmStore *) virtual_p;
    while(true) {
    
        g_start_persist_mutex.lock();
        if (!plsmStore->start_persist && plsmStore->memTable->persist_queue.empty())
            break;
        g_start_persist_mutex.unlock();
        
        /* Do nothing if queue is empty */
        if (plsmStore->memTable->persist_queue.empty())
            continue;
        /* start to copy data */
        if (plsmStore->levels.empty()) {
            plsmStore->levels.emplace_back(0);
        }

        g_persist_list_mutex.lock();
        plsmStore->levels[0].runs.emplace_back(plsmStore->memTable->persist_queue.front());
        plsmStore->memTable->persist_queue.pop_front();
        g_persist_list_mutex.unlock();
    }

}

/* Implementations for PlsmStore */

PlsmStore::PlsmStore(int level_base_val, int level_ratio_val) 
    : level_base(level_base_val), 
    level_ratio(level_ratio_val) {

    memTable = new MemTable();

    g_start_persist_mutex.lock();
    start_persist = true;
    g_start_persist_mutex.unlock();
    
    if (pthread_create(&persist_thread_id, NULL, persist_memTable,  this) < 0) {
        cout << "create persisting thread error!" << endl;
        exit(-1);
    }
}

PlsmStore::~PlsmStore(){
    delete memTable;
}

/* compare two kv pairs */
bool PlsmStore::compare(pair<string, string> kv1, pair<string, string> kv2) {
    return kv1.first < kv2.first;
}

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
        /* put memory buffer into persist queue (after sort) 
         * and allocate new buffer */
        sort(memTable->buffer->begin(), memTable->buffer->end(), compare);
        
        g_persist_list_mutex.lock();
        memTable->persist_queue.insert(memTable->persist_queue.end(), memTable->buffer);
        g_persist_list_mutex.unlock();

        memTable->buffer = new vector< pair<string, string> >();
        memTable->buffer->reserve(RUN_SIZE);
    }
    return;
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

Level::Level(int id) : level_id(id), run_count(0) {}
Level::~Level() {}

/* Implementations for Run */

Run::Run() : array_count(0), ref_count(0) {}

Run::Run(vector< pair<string, string> > * array) 
    : kvArray(*array), 
    ref_count(0), 
    array_count(1), 
    start(array->front().first), 
    end(array->back().first) { }

Run::~Run() {}

/* Implementaions for Segment */

Seg::Seg(list<Run>::iterator old_run, int old_start, int old_end) 
    : run_it(old_run), 
    start(old_start), 
    end(old_end) { }

Seg::~Seg() {}
