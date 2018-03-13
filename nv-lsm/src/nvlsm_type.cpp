#include "nvlsm_types.h"
#include <algorithm>
#include <mutex>
#include <iostream>

#define DO_LOG 0
#define LOG(msg) if (DO_LOG) std::cout << "[nv-lsm] " << msg << "\n"

using namespace std;
using namespace nv_lsm;

mutex g_persist_list_mutex;
mutex g_start_persist_mutex;

pool<LSM_Root> pmpool;
size_t pmsize;

/* This function will run in a sub-thread
 * 1. keep checking persist_queue with an infinit loop
 * 2. copy array from persist_queue to level0
 * */
void * persist_memTable(void * virtual_p) {
    PlsmStore * plsmStore = (PlsmStore *) virtual_p;
    while(true) {
    
        g_start_persist_mutex.lock();
        /* check if stop the persist thread */
        if (!plsmStore->start_persist && plsmStore->memTable->persist_queue.empty())
            break;
        g_start_persist_mutex.unlock();
        
        /* Do nothing if queue is empty */
        if (plsmStore->memTable->persist_queue.empty())
            continue;

        /* copy data from persist queue to level 0 */
        if (plsmStore->level0 == NULL) {
            make_persistent_atomic<Level>(pmpool, plsmStore->level0, 0);
        }
        g_persist_list_mutex.lock();
        vector<KVPair> * p_kv_vec = plsmStore->memTable->persist_queue.front();
        persistent_ptr<KVPair[]> level0_array = plsmStore->level0->runs_tail->kv_array;
        
        /* add a new empty run at tail */
        persistent_ptr<Run> new_tail;
        make_persistent_atomic<Run> (pmpool, new_tail);
        new_tail->pre_run = plsmStore->level0->runs_tail;
        plsmStore->level0->runs_tail->next_run = new_tail;
        plsmStore->level0->runs_tail = new_tail;
        /* delete front from persist queue */
        plsmStore->memTable->persist_queue.pop_front();

        g_persist_list_mutex.unlock();
    }

}

/* Implementations for PlsmStore */

PlsmStore::PlsmStore(const string &path, const size_t size, int level_base_val, int level_ratio_val) 
    : level_base(level_base_val), 
    level_ratio(level_ratio_val) {

    /* create memTable */
    memTable = new MemTable();

    /* open/create persist pool */
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

    // to-do recover

    LOG("Opened OK");

    /* start persist thread */
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
bool PlsmStore::compare(KVPair &kv1, KVPair &kv2) {
    return  strcmp(kv1.key, kv2.key);
}

/* insert a key value pair to memory buffer */
void PlsmStore::put(char * key, char *  value) {
    if (memTable->buffer == NULL) {
        memTable->buffer = new vector<KVPair>();
        memTable->buffer->reserve(RUN_SIZE);
    }
    
    /* put kv pair into memory buffer */
    KVPair kv_pair;
    strcpy(key, kv_pair.key);
    strcpy(value, kv_pair.value);
    memTable->buffer->insert(memTable->buffer->end(), kv_pair);

    int len = memTable->buffer->size();

    if (len == RUN_SIZE) {
        /* put memory buffer into persist queue (after sort) 
         * and allocate new buffer */
        sort(memTable->buffer->begin(), memTable->buffer->end(), compare);
        
        g_persist_list_mutex.lock();
        memTable->persist_queue.insert(memTable->persist_queue.end(), memTable->buffer);
        g_persist_list_mutex.unlock();

        memTable->buffer = new vector<KVPair>();
        memTable->buffer->reserve(RUN_SIZE);
    }
    return;
}

/* Implementations for MemTable */
MemTable::MemTable() {}
MemTable::~MemTable() {
    delete(buffer);
}

/* Implementations for Level */ 

Level::Level(int id) : level_id(id), run_count(0) {
    make_persistent_atomic<Run>(pmpool, runs);
    if (!runs) {
        LOG("allocate runs failed");
    }
    runs_head = runs;
    runs_tail = runs;
}
Level::~Level() {}

/* Implementations for Run */

Run::Run() : seg_count(0), ref_count(0) {
    make_persistent_atomic<KVPair[]>(pmpool, kv_array, RUN_SIZE);
}


Run::~Run() {}
