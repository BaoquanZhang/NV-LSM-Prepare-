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
    while (count < buffer.size()) {
        int end1 = count + RUN_SIZE;
        int end2 = buffer.size();
        int copy_end = end1 <= end2 ? end1: end2;
        level.runs.emplace_back();
        auto last_run = &(level.runs.back());
        level.run_count++;
        last_run->kvArray.reserve(RUN_SIZE);
        last_run->kvArray.insert(last_run->kvArray.begin(), buffer.begin() + count, buffer.begin() + copy_end);
        last_run->start = last_run->kvArray.front().first;
        last_run->end = last_run->kvArray.back().first;
        count += RUN_SIZE;
    }
}

/* This is the function for normal compaction */
void norm_compact(Level & level0, Level & level1) {
    int count = 0;
    int write_count = 0;
    int read_count = 0;

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
        
        /* read to buffer */
        read_count += buffer.size();

        /* split and write back to level1 */
        int end_count = level1.runs.size();
        split_write(buffer, level1);

        /* write to level */
        write_count += buffer.size();

        level0.runs.erase(level0.runs.begin());
        level0.run_count--;
        while (end_count-- > 0) {
            level1.runs.erase(level1.runs.begin());
        }
        level1.run_count -= end_count;
        //checkRuns(level0);
        //checkRuns(level1);
    }

    cout << "Read/Write counts for normal compaction is " << read_count << "/" << write_count << endl;
}

/* This is the function for lazy compaction */

void lazy_compact(Level & level0, Level & level1) { 
    auto l0_it = level0.runs.begin();
    int l1_stop = level1.runs.size();
    int write_count = 0;
    int read_count = 0;

    while (l0_it != level0.runs.end()) {
        int offset = 0;
        int start = 0;
        int index_l1 = 0;
        auto l1_it = level1.runs.begin();
        while (index_l1++ < l1_stop) {
            /* find the overlap scale to next level runs */
            while (offset < l0_it->kvArray.size() && l0_it->kvArray[offset].first <= l1_it->end) {
                offset++;
            }
            /* add read counts for scan */
            read_count += offset - start;

            if (offset > start && l1_it->array_count < MAX_ARRAY) {
                /* smaller than MAX_ARRAY link the scale only */
                l1_it->next.emplace_back(l0_it, start, offset - 1);
                l1_it->start = l1_it->start < l0_it->kvArray[start].first ? l1_it->start : l0_it->kvArray[start].first;
                l1_it->end = l1_it->end > l0_it->kvArray[offset - 1].first ? l1_it->end : l0_it->kvArray[offset - 1].first; 
                l1_it->array_count++;
                l0_it->ref_count++;
                l1_it++;
            } else {
                /* reach MAX_ARRAY -- merge and rewrite*/
                int total = offset - start;
                total += l1_it->kvArray.size();
                for (auto next_it = l1_it->next.begin(); next_it != l1_it->next.end(); next_it++) {
                    total += next_it->end - next_it->start + 1;
                }

                /* allocate buffer and copy data from level0, level1 run and level1 run-seg */
                vector<pair<long, string>> buffer;
                buffer.reserve(total);
                buffer.insert(buffer.begin() + buffer.size(), l0_it->kvArray.begin() + start, l0_it->kvArray.begin() + offset);
                buffer.insert(buffer.begin() + buffer.size(), l1_it->kvArray.begin(), l1_it->kvArray.end());
                for (auto next_it = l1_it->next.begin(); next_it != l1_it->next.end(); next_it++) {
                    auto array_begin = next_it->run_it->kvArray.begin();
                    int seg_start = next_it->start;
                    int seg_end = next_it->end;
                    buffer.insert(buffer.begin() + buffer.size(), array_begin + seg_start, array_begin + seg_end);
                    next_it->run_it->ref_count--;
                } 
                /* add read counts for reading data to buffer */
                read_count += buffer.size();

                sort(buffer.begin(), buffer.end(), comparePair);
                split_write(buffer, level1);
                /* add write counts for writing to next level */
                write_count += buffer.size();
                level1.runs.erase(l1_it);
                l1_it = level1.runs.begin();
            }
            start = offset;
        }
        l0_it++;
    }
    
    l0_it = level0.runs.begin();
    while (l0_it != level0.runs.end()) {
        if (l0_it->ref_count == 0) {
            level0.runs.erase(l0_it);
            l0_it = level0.runs.begin();
        } else {
            l0_it++;
        }
    }
    cout << "Read/Write counts for lazy compaction is " << read_count << "/" << write_count << endl;
}

void checkRuns(Level level) {
    for (auto it : level.runs) {
        cout << "<" << it.start << "," << it.end << ">(" << it.kvArray.size() << ") ";
    }
    cout << endl;
}

int main(void) {

    cout << "############### Initializing Level 0 and 1 ############" << endl;
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
    //lazy_compact(level0, level1);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>( t2 - t1 ).count();
    cout << "Time used by normal compaction: " << duration << endl;
    cout << "############### After compaction #############" << endl;

    cout << "Level 0:";
    checkRuns(level0);
    cout << "Level 1:";
    checkRuns(level1);

    cout << "############### Initializing Level 0 and 1 ############" << endl;
    Level level0_lazy;
    Level level1_lazy;
    
    init(level0_lazy, 0);
    init(level1_lazy, 1);

    cout << "############### Before compaction ############" << endl;
    cout << "Level 0:";
    checkRuns(level0_lazy);
    cout << "Level 1:";
    checkRuns(level1_lazy);

    high_resolution_clock::time_point t3 = high_resolution_clock::now();
    lazy_compact(level0_lazy, level1_lazy);
    high_resolution_clock::time_point t4 = high_resolution_clock::now();
    auto duration2 = duration_cast<microseconds>( t4 - t3 ).count();
    cout << "Time used by lazy compaction: " << duration2 << endl;
    cout << "############### After compaction #############" << endl;

    cout << "Level 0:";
    checkRuns(level0_lazy);
    cout << "Level 1:";
    checkRuns(level1_lazy);
    return 0;
}
