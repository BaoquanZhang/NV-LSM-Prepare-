#include<iostream>
#include "../include/nvlsm_types.h"

using namespace std;
using namespace nv_lsm;

#define PATH "/mnt/mem/nvlsm/test.pool"
#define POOL_SIZE 10 * 1024 * 1024

int main(void) {
    PlsmStore pstore(PATH, POOL_SIZE, 4, 10);
    for (int i = 0; i < 10000; i++) {
        pstore.put(to_string(i), "456asdf");
    }

    cout << "display kv range in buffer: ";
    auto p_buffer_range = pstore.metaTable->buffer_range;
    cout << "<" << p_buffer_range->start_key << ",";
    cout << p_buffer_range->end_key << ">";
    cout << endl;

    usleep(3000 * 1000);
    cout << "Display level 0: ";
    auto p_level0_range = &(pstore.metaTable->level_range[0]);
    for (auto level0_it = p_level0_range->begin(); level0_it != p_level0_range->end(); level0_it++) {
        cout << "<" << level0_it->first.start_key << "," << level0_it->first.end_key << ">";
        auto local_array = level0_it->second->local_array;
        cout << "(" << local_array[0].key << "," << local_array[4095].key << ") ";

    }
    cout << endl;

    /*
    auto p_run = pstore.level_head->run_head;
    while (p_run) {
        for (int i = 0; i < RUN_SIZE; i++) {
            cout << "<" << p_run->local_array[i].key << "> ";
        }
        cout << "--->";
        p_run = p_run->next_run;
    }
    cout << endl;
    */
    return 0;
}
