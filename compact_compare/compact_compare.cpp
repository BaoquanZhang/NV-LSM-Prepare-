#include "nvlsm_types.h"
#include<algorithm>
#include<chrono>

using namespace std;
using namespace chrono;
using namespace nv_lsm;

void init(Level & level, int num);
void checkRuns(Level level);
void norm_compact(Level & level0, Level & level1);

void init(Level & level, int num) {
    int i;
    long j;
    string value = "ald989898sofasdfasdfasdfdsfajflkasjdfsakkakandkjnkajndkjoiakdjnckadlkflasdlcmaklsdmlkmcalsdklclskdkmc";
    cout << "key/value size is " << sizeof(long) << "/" << value.length() << endl;

    level.run_count = 0;
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
            level.run_count++;
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
            level.run_count++;
        }
    }
}

bool comparePair(const pair<long, string> lr, const pair<long, string> rr) {
    return lr.first < rr.first ? true : false;
}

void outputArray(vector< pair<long, string> > kvArray, int count) {
    cout << "-------------display array------------" << endl;
    cout << "size " << kvArray.size() << endl;
    for (int i = 0; i < count && i < kvArray.size(); i++) {
        cout << kvArray[i].first << " ";
    }
    cout << endl;
}

void norm_compact(Level & level0, Level & level1) {
    int count = 0;

    while(!level0.runs.empty()) {
        /* for each of the runs in level0
         * read our all runs in level1 and merge-sort-split-write
         * */
        //cout << "########## compacting #########" << endl;
        auto l0_it = level0.runs.begin();
        vector<pair<long, string>> buffer;
        buffer.reserve(RUN_SIZE * (level1.run_count + 1));
        count = 0;
        
        /* merge and sort */
        for (auto l1_it = level1.runs.begin(); l1_it != level1.runs.end(); l1_it++) {
            //cout << "copy runs to buffer at offset " << count << endl;
            buffer.insert(buffer.begin() + count, l1_it->kvArray.begin(), l1_it->kvArray.end());
            count += l1_it->kvArray.size();
        }
        //cout << "buffer size befor merge level 0 " << buffer.size() << endl;
        buffer.insert(buffer.begin() + count, l0_it->kvArray.begin(), l0_it->kvArray.end());
        sort(buffer.begin(), buffer.end(), comparePair);

        //cout << "buffer size " << buffer.size() << endl;
        /* split and write back to level1 */
        int erase_start = 0;
        int erase_end = level1.runs.size();
        count = 0;
        while (count + RUN_SIZE <= buffer.size()) {
            level1.runs.emplace(level1.runs.end());
            auto last_run = &(level1.runs.back());
            level1.run_count++;
            last_run->kvArray.reserve(RUN_SIZE);
            last_run->kvArray.insert(last_run->kvArray.begin(), buffer.begin() + count, buffer.begin() + count + RUN_SIZE);
            last_run->start = last_run->kvArray.front().first;
            last_run->end = last_run->kvArray.back().first;
            count += RUN_SIZE;
        }
        level0.runs.erase(level0.runs.begin());
        level0.run_count--;
        level1.runs.erase(level1.runs.begin() + erase_start, level1.runs.begin() + erase_end);
        level1.run_count -= erase_end - erase_start - 1;
        //checkRuns(level0);
        //checkRuns(level1);
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
    init(level1, 1);

    cout << "############### Before compaction ############" << endl;
    cout << "Level 0:";
    checkRuns(level0);
    cout << "Level 1:";
    checkRuns(level1);

    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    norm_compact(level0, level1);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>( t2 - t1 ).count();
    cout << "Time used by normal compaction: " << duration << endl;
    cout << "############### After compaction #############" << endl;

    cout << "Level 0:";
    checkRuns(level0);
    cout << "Level 1:";
    checkRuns(level1);

    return 0;
}
