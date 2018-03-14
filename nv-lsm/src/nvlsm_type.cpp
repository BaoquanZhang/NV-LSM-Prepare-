#include "nvlsm_types.h"
#include <algorithm>
#include <mutex>
#include <iostream>

using namespace std;
using namespace nv_lsm;

mutex g_persist_list_mutex;
mutex g_start_persist_mutex;

pool<LSM_Root> pmpool;
size_t pmsize;

#define DO_LOG true
#define LOG(msg) if (DO_LOG) std::cout << "[nvlsm] " << msg << "\n"


/* This function will run in a sub-thread
 * 1. keep checking persist_queue with an infinite loop
 * 2. copy array from persist_queue to level0
 * */
void * persist_memTable(void * virtual_p) {
    PlsmStore * plsmStore = (PlsmStore *) virtual_p;
    auto persist_queue = &(plsmStore->memTable->persist_queue);
    LOG("Persist thread start");
    while(true) {
        g_start_persist_mutex.lock();
        if (!plsmStore->start_persist && persist_queue->empty())
            break;
        g_start_persist_mutex.unlock();
        
        /* Do nothing if queue is empty */
        if (persist_queue->empty())
            continue;
        /* start to copy data */
        if (!persist_queue->empty() && plsmStore->level_head == NULL) {
            /* Create Level 0 */
            make_persistent_atomic<Level>(pmpool, plsmStore->level_head, 0);
            plsmStore->level_tail = plsmStore->level_head;
        }
        /* Create a new run at the end of Level 0 */
        LOG("persist thread copying kv pairs");
        auto level0 = plsmStore->level_head;
        g_persist_list_mutex.lock();
        if (level0->run_head == NULL) {
            make_persistent_atomic<Run>(pmpool, level0->run_head, persist_queue->front());
            level0->run_tail = level0->run_head;
        } else {
            make_persistent_atomic<Run>(pmpool, level0->run_tail->next_run, persist_queue->front());
            /* move run_tail in level0 to next position */
            level0->run_tail->next_run->pre_run = level0->run_tail;
            level0->run_tail = level0->run_tail->next_run;
        }
        /* pop the first memtable from persist queue */
        persist_queue->pop_front();
        g_persist_list_mutex.unlock();
    }
    LOG("Persist thread exit");

}

/* Implementations for PlsmStore */

PlsmStore::PlsmStore (const string& path, const size_t size, int level_base_val, int level_ratio_val) 
    : level_base(level_base_val), 
    level_ratio(level_ratio_val) {

    /* Create mem tables */
    memTable = new MemTable();
    /* Create/Open pmem pool */
    if (access(path.c_str(), F_OK) != 0) {
        LOG("Creating filesystem pool, path=" << path << ", size=" << to_string(size));
        pmpool = pool<LSM_Root>::create(path.c_str(), LAYOUT, size, S_IRWXU);
        pmsize = size;
    } else {
        LOG("Opening filesystem pool, path=" << path);
        pmpool = pool<LSM_Root>::open(path.c_str(), LAYOUT);
        struct stat st;
        stat(path.c_str(), &st);
        pmsize = (size_t) st.st_size;
    }
    LOG("Create/open pool done");
    /* Start persist thread for copy kv pairs from memtable to persistent levels */
    g_start_persist_mutex.lock();
    start_persist = true;
    g_start_persist_mutex.unlock();
    
    if (pthread_create(&persist_thread_id, NULL, persist_memTable,  this) < 0) {
        cout << "create persisting thread error!" << endl;
        exit(-1);
    }
}

PlsmStore::~PlsmStore(){
    g_start_persist_mutex.lock();
    start_persist = false;
    g_start_persist_mutex.unlock();
    delete memTable;
    LOG("Closing persistent pool");
    pmpool.close();
    LOG("Closed ok");
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

Level::Level(int id) : level_id(id), run_count(0) {
}
Level::~Level() {}

/* Implementations for Run */

Run::Run() {
    make_persistent_atomic<KVPair[]>(pmpool, local_array, RUN_SIZE);
}

Run::Run(vector< pair<string, string> > * array) {
    make_persistent_atomic<KVPair[]>(pmpool, local_array, RUN_SIZE);
    for (int i = 0; i < array->size(); i++) {
        array->at(i).first.copy(local_array[i].key, array->at(i).first.length(), 0);
        array->at(i).second.copy(local_array[i].value, array->at(i).second.length(), 0);
    }
}

Run::~Run() {}

