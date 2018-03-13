#include<iostream>
#include "nvlsm_types.h"

using namespace std;
using namespace nv_lsm;

#define POOL_SIZE 1 * 1024 * 1024 * 1024
int main(void) {
    PlsmStore pstore("/mnt/mem/nvlsm/pstore.layout", POOL_SIZE, 4, 10);
    for (int i = 0; i < 10000; i++) {
        pstore.put(const_cast<char*>(to_string(i).c_str()), "456asdf");
    }

    cout << "display kv pair in buffer " << endl;
    auto memBuffer = pstore.memTable->buffer;
    for (auto it =  memBuffer->begin(); it != memBuffer->end(); it++) {
        cout << "<" << it->key << "," << it->value << "> ";
    }
    cout << endl;

    cout << "Display level 0 " << endl;
    auto current_run = pstore.level0->runs_head;
    while(current_run != pstore.level0->runs_tail) {
        for (int i = 0; i < RUN_SIZE; i++) {
            cout << "<" << current_run->kv_array[i].key << ">";
        }
        current_run = current_run->next_run;
        cout << "--->";
    }
    cout << endl;
    
    return 0;
}
