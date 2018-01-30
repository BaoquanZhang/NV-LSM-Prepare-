#include "nvlsm_types.h"
#include<algorithm>

using namespace std;
using namespace nv_lsm;

void init(Level & level, int num) {
    int i;
    long j;
    string value = "ald989898dsfajflkasjdfsakkakandkjnkajndkjoiakdjnckadlkflasdlcmaklsdmlkmcalsdklclskdkmc";

    cout << "key/value size is " << sizeof(long) << "/" << value.length() << endl;

    if (num == 0) {
        for (i = 0; i < 3; i++) {
            Run run;
            run.kvArray.reserve(RUN_SIZE);
            run.start = 0;
            for (j = 0; j < RUN_SIZE * 10; j+= 10) {
                pair<long, string> kvPair(j, value);
                run.kvArray.push_back(kvPair);
            }
            run.end = j - 1;
            level.runs.push_back(run);
        }
    } else if (num == 1) {
        for (i = 0; i < 10; i++) {
            Run run;
            run.kvArray.reserve(RUN_SIZE);
            run.start = i * RUN_SIZE;
            for (j = i * RUN_SIZE; j < RUN_SIZE * (i + 1); j++) {
                pair<long, string> kvPair(j, value);
                run.kvArray.push_back(kvPair);
            }
            run.end = j - 1;
            level.runs.push_back(run);
        }
    }
}

bool comparePair(const pair<long, string> lr, const pair<long, string> rr) {
    return lr.first < rr.first ? true : false;
}

void norm_compact(Level & level0, Level & level1) {
    vector<pair<long, string>> buffer;
    int count = 0;

    for (auto l0_it : level0.runs) {
        /* for each of the runs in level0
         * read our all runs in level1 and merge-sort-split-write
         * */
        buffer.reserve(RUN_SIZE * (level1.run_count + 1));
        count = 0;
        /* merge and sort */
        for (auto l1_it : level1.runs) {
            copy(l1_it.kvArray.begin(), l1_it.kvArray.end(), buffer.begin() + count);
            count += l1_it.kvArray.size();
        }
        copy(l0_it.kvArray.begin(), l0_it.kvArray.end(), buffer.begin() + count);
        sort(buffer.begin(), buffer.end(), comparePair);
        /* split and write back to level1 */
        
    }
}

void checkRuns(Level level) {
    for (auto it : level.runs) {
        cout << "<" << it.start << "," << it.end << "> ";
    }
    cout << endl;
}

int main(void) {
    Level level0;
    Level level1;

    init(level0, 0);
    checkRuns(level0);

    init(level1, 1);
    checkRuns(level1);

    return 0;
}
