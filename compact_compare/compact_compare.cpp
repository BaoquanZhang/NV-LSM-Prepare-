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
            run.array_count = 1;
            run.next.reserve(MAX_ARRAY - 1);
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
            run.array_count = 1;
            run.next.reserve(MAX_ARRAY - 1);
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

void split_write(vector<pair<long, string> > & buffer, Level & level) {
    int count = 0;
    while (count + RUN_SIZE <= buffer.size()) {
        level.runs.emplace(level.runs.end());
        auto last_run = &(level.runs.back());
        level.run_count++;
        last_run->kvArray.reserve(RUN_SIZE);
        last_run->kvArray.insert(last_run->kvArray.begin(), buffer.begin() + count, buffer.begin() + count + RUN_SIZE);
        last_run->start = last_run->kvArray.front().first;
        last_run->end = last_run->kvArray.back().first;
        count += RUN_SIZE;
    }

}

/* This is the function for normal compaction */
void norm_compact(Level & level0, Level & level1) {
    int count = 0;

    while(!level0.runs.empty()) {
        /* for each of the runs in level0
         * read our all runs in level1 and merge-sort-split-write
         * */
        auto l0_it = level0.runs.begin();
        vector<pair<long, string>> buffer;
        buffer.reserve(RUN_SIZE * (level1.run_count + 1));
        count = 0;
        
        /* merge and sort */
        for (auto l1_it = level1.runs.begin(); l1_it != level1.runs.end(); l1_it++) {
            buffer.insert(buffer.begin() + count, l1_it->kvArray.begin(), l1_it->kvArray.end());
            count += l1_it->kvArray.size();
        }
        buffer.insert(buffer.begin() + count, l0_it->kvArray.begin(), l0_it->kvArray.end());
        sort(buffer.begin(), buffer.end(), comparePair);

        /* split and write back to level1 */
        int erase_start = 0;
        int erase_end = level1.runs.size();
        split_write(buffer, level1);
        level0.runs.erase(level0.runs.begin());
        level0.run_count--;
        level1.runs.erase(level1.runs.begin() + erase_start, level1.runs.begin() + erase_end);
        level1.run_count -= erase_end - erase_start - 1;
        //checkRuns(level0);
        //checkRuns(level1);
    }
}

/* This is the function for lazy compaction */
void lazy_compact(Level & level0, Level & level1) { 
    auto l0_it = level0.runs.begin();
    while (l0_it != level0.runs.end()) {
        int offset = 0;
        int start = 0;
        auto l1_it = level1.runs.begin();
        auto l1_stop = level1.runs.end();
        while (l1_it != level1.runs.end()) {
            while (offset < l0_it->kvArray.size() && l0_it->kvArray[offset] <= l1_it.end()) {
                offset++;
            }
            if (l1_it->array_count <= MAX_ARRAY) {
                /* add segment to run */
                pair<int, int> range(start, offset - 1);
                pair<Run *, pair<int, int> > nextSeg(l0_it, range);    
                l1_it->next.insert(nextSeg);
                l1.it->array_count++;
                l0_it->ref_count++;
            } else {
                int total = offset - start - 1;
                total += l1_it->kvArray.size();
                for (auto next_it = l1_it->next.begin(); next_it != l1_it->next.end(); next_it++) {
                    total += next_it->second.second - next_it->second.first + 1;
                }
                vector<pair<long, string>> buffer(total);
                buffer.insert(buffer.begin() + buffer.size(), l0_it.kvArray.begin() + start, l0_it->kvArray.begin() + offset);
                for (auto next_it = l1_it->next.begin(); next_it != l1_it->next.end(); next_it++) {
                    auto seg_it = next_it->first;
                    int seg_start = next_it->second.first;
                    int seg_end = next_it->second.second;
                    auto seg_start_it = seg_it->kvArray.begin() + seg_start;
                    auto seg_end_it = seg_it->kvArray.begin() + seg_end;
                    buffer.insert(buffer.begin() + buffer.size(), seg_start_it, seg_end_it);
                } 
                sort(buffer.begin(), buffer.end(), comparePair);
                split_write(buffer, level1);
                level1.erase(l1_it);
                l0_it->ref_count--;
                if (l0_it->ref_count == 0) {
                    level0.erase(l0_it);
                }
            }
            l0_it = level0.runs.begin();
            l1_it = level1.runs.begin();
            start = offset;
        }
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
