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

    cout << "display kv pair in buffer " << endl;
    auto memBuffer = pstore.memTable->buffer;
    for (auto it =  memBuffer->begin(); it != memBuffer->end(); it++) {
        cout << "<" << it->first << "> ";
    }
    cout << endl;

    usleep(3000 * 1000);
    cout << "Display level 0 " << endl;
    auto p_run = pstore.level_head->run_head;
    while (p_run) {
        for (int i = 0; i < RUN_SIZE; i++) {
            cout << "<" << p_run->local_array[i].key << "> ";
        }
        cout << "--->";
        p_run = p_run->next_run;
    }
    cout << endl;
    
    return 0;
}
