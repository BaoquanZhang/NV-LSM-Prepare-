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

    cout << "Display persist queue " << endl;
    for (auto queue_it : pstore.memTable->persist_queue) {
        for (auto it = queue_it->begin(); it != queue_it->end(); it++) {
            cout << "<" << it->first << "," << it->second << "> ";
        }

        cout << "--->";
    }
    cout << endl;
    
    return 0;
}
