#include "nvlsm_types.h"
#include "mem_structure.h"
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
void * persist_memTable(void * virtual_p) 
{
    PlsmStore * plsmStore = (PlsmStore *) virtual_p;
    auto persist_queue = &(plsmStore->memTable->persist_queue);
    auto level0 = plsmStore->level_head;
    auto p_level0_range = &(plsmStore->metaTable->level_range[0]);
    LOG("Persist thread start");
    while(true) {
        g_start_persist_mutex.lock();
        if (persist_queue->empty()) {
            if (!plsmStore->start_persist) {
                /* break out if start_persist is false */
                g_start_persist_mutex.unlock();
                break;
            } else {
                /* Do nothing if queue is empty */
                g_start_persist_mutex.unlock();
                continue;
            }
        }
        g_start_persist_mutex.unlock();
        /* start to copy data
         * 1. Create a new run at the end of Level 0 
         * 2. copy kvs into new run 
         * */
        LOG("persist thread copying kv pairs");
        g_persist_list_mutex.lock();
        auto p_queue_range = &(plsmStore->metaTable->queue_range);
        LOG("Persisting <" + p_queue_range->front()->start_key + "," + p_queue_range->front()->end_key + ">");
        if (level0->run_head == NULL) {
            make_persistent_atomic<Run>(pmpool, level0->run_head, persist_queue->front());
            level0->run_tail = level0->run_head;
        } else {
            make_persistent_atomic<Run>(pmpool, level0->run_tail->next_run, persist_queue->front());
            /* move run_tail in level0 to next position */
            level0->run_tail->next_run->pre_run = level0->run_tail;
            level0->run_tail = level0->run_tail->next_run;
        }
        p_level0_range->emplace_back(make_pair(*(p_queue_range->front()),level0->run_tail));
        /* pop the first memtable from persist queue */
        persist_queue->pop_front();
        p_queue_range->pop_front();
        g_persist_list_mutex.unlock();
    }
    LOG("Persist thread exit");

}

/* Implementations for PlsmStore */

PlsmStore::PlsmStore (const string& path, const size_t size, int level_base_val, int level_ratio_val) 
    : level_base(level_base_val), 
    level_ratio(level_ratio_val) 
{

    /* Create mem structures */
    memTable = new MemTable();
    metaTable = new MetaTable();
    metaTable->level_range.reserve(7); // we reserve 7 levels here
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
    /* Create Level 0 */
    make_persistent_atomic<Level>(pmpool, level_head, 0);
    level_tail = level_head;
    /* Start persist thread for copy kv pairs from memtable to persistent levels */
    g_start_persist_mutex.lock();
    start_persist = true;
    g_start_persist_mutex.unlock();
    
    if (pthread_create(&persist_thread_id, NULL, persist_memTable,  this) < 0) {
        cout << "create persisting thread error!" << endl;
        exit(-1);
    }
}

PlsmStore::~PlsmStore()
{
    g_start_persist_mutex.lock();
    start_persist = false;
    g_start_persist_mutex.unlock();
    delete memTable;
    delete metaTable;
    LOG("Closing persistent pool");
    pmpool.close();
    LOG("Closed ok");
}

/* compare two kv pairs */
bool PlsmStore::compare(pair<string, string> kv1, pair<string, string> kv2) 
{
    return kv1.first < kv2.first;
}

/* insert a key value pair to memory buffer */
void PlsmStore::put(string key, string  value) 
{
    if (memTable->buffer == NULL) {
        memTable->buffer = new vector< pair<string, string> >();
        memTable->buffer->reserve(RUN_SIZE);
        metaTable->buffer_range = new KeyRange(KEY_SIZE);
    }
    
    /* put kv pair into memory buffer */
    memTable->buffer->insert(memTable->buffer->end(), make_pair(key, value));
    // update key range for mem buffer
    int len = memTable->buffer->size();
    auto p_buffer_range = metaTable->buffer_range;
    if (len == 1) {
        p_buffer_range->start_key = key;
        p_buffer_range->end_key = key;
    } else {
        if (p_buffer_range->start_key.compare(key) > 0)
            p_buffer_range->start_key = key;
        else if (p_buffer_range->end_key.compare(key) < 0)
            p_buffer_range->end_key = key;
    }

    if (len == RUN_SIZE) {
        /* put memory buffer into persist queue (after sort) 
         * and allocate new buffer */
        sort(memTable->buffer->begin(), memTable->buffer->end(), compare);
        
        g_persist_list_mutex.lock();
        memTable->persist_queue.insert(memTable->persist_queue.end(), memTable->buffer);
        metaTable->queue_range.insert(metaTable->queue_range.end(), metaTable->buffer_range);
        g_persist_list_mutex.unlock();

        memTable->buffer = new vector< pair<string, string> >();
        memTable->buffer->reserve(RUN_SIZE);
        metaTable->buffer_range = new KeyRange(KEY_SIZE);
    }
    return;
}

vector< pair<string, string> > PlsmStore::range(string start, string end) 
{
    // to-do
}

void PlsmStore::normal_compaction(Level * up_level, Level * bottom_level) 
{
    // to-do
}

void PlsmStore::lazy_compaction(Level * up_level, Level * bottom_level) 
{
    // to-do
}

/* Implementations for Level */ 
Level::Level(int id) : level_id(id), run_count(0) {}
Level::~Level() {}

/* Implementations for Run */
Run::Run() 
{
    make_persistent_atomic<KVPair[]>(pmpool, local_array, RUN_SIZE);
}

Run::Run(vector< pair<string, string> > * array) 
{
    make_persistent_atomic<KVPair[]>(pmpool, local_array, RUN_SIZE);
    for (int i = 0; i < array->size(); i++) {
        array->at(i).first.copy(local_array[i].key, array->at(i).first.length(), 0);
        array->at(i).second.copy(local_array[i].value, array->at(i).second.length(), 0);
    }
}

Run::~Run() {}

