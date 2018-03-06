#include<iostream>
#include "../include/nvlsm_types.h"

using namespace std;
using namespace nv_lsm;

int main(void) {
    PlsmStore pstore(4, 10);
    for (int i = 0; i < 10000; i++) {
        pstore.put(to_string(i), "456asdf");
    }

    cout << "display kv pair in buffer " << endl;
    auto memBuffer = pstore.memTable->buffer;
    for (auto it =  memBuffer->begin(); it != memBuffer->end(); it++) {
        cout << "<" << it->first << "," << it->second << "> ";
    }
    cout << endl;

    cout << "Display level 0 " << endl;
    for (auto run_it : pstore.levels[0].runs) {
        for (auto it = run_it.kvArray.begin(); it != run_it.kvArray.end(); it++) {
            cout << "<" << it->first << "," << it->second << "> ";
        }

        cout << "--->";
    }
    cout << endl;
    
    return 0;
}
