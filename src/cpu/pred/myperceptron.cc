#include "myperceptron.hh"

//#include <iostream>
#include <fstream>

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "debug/MYperceptron.hh"

#define DEBUG 0
#define COUNT 0
#define ALIASING 1


MyPerceptron::MyPerceptron(const MyPerceptronParams *params)
    : BPredUnit(params),
    globalPredictorSize(params->globalPredictorSize),
    globalHistory(params->numThreads, 0),
    globalHistoryBits(ceilLog2(params->globalPredictorSize)),
    sizeOfPerceptrons(params->sizeOfPerceptrons)
{
    if (!isPowerOf2(globalPredictorSize)) {
        fatal("Invalid global predictor size, should be power of 2.\n");
    }

    historyRegisterMask = mask(globalHistoryBits);

    theta = 1.93 * sizeOfPerceptrons + 14;

    weights.assign(globalPredictorSize, std::vector<int>\
            (sizeOfPerceptrons + 1, 0));

#if DEBUG
    stat_perceptrons.assign(globalPredictorSize, false);
#endif

#if ALIASING
    addr_record.assign(globalPredictorSize, 0);
    history_record.assign(globalPredictorSize, 0);
#endif
}

void
MyPerceptron::updateGlobalHistTaken(ThreadID tid)
{
    globalHistory[tid] = (globalHistory[tid] << 1) | 1;
    globalHistory[tid] &= historyRegisterMask;
}

void
MyPerceptron::updateGlobalHistNotTaken(ThreadID tid)
{
    globalHistory[tid] = (globalHistory[tid] << 1);
    globalHistory[tid] &= historyRegisterMask;
}

void
MyPerceptron::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    globalHistory[tid] &= (historyRegisterMask & ~ULL(1));
}

bool
MyPerceptron::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{
#if COUNT
    static uint64_t count = 0;
    static uint64_t less_than_theta = 0;
    static uint64_t less_than_half_of_theta = 0;
    static uint64_t less_than_quater_of_theta = 0;

    static uint64_t c_temp = 0;
    static uint64_t less_temp = 0;
    static uint64_t half_temp = 0;
    static uint64_t quater_temp = 0;
#endif

    // Get the global history of this thread
    unsigned thread_history = globalHistory[tid];

    // Index of the perceptron to visit
    int index;

    // Hash the global history bits
    index = (thread_history ^ (branch_addr >> 2)) % globalPredictorSize;

    // Bias term
    int out = weights[index][0];

    for (int i = 0; i < sizeOfPerceptrons; i++){
        if ((thread_history >> i) & 0x1){
            out += weights[index][i+1];
        }
        else{
            out -= weights[index][i+1];
        }
    }

    // Use the sign bit as the result
    bool taken = (out >= 0);

#if COUNT
    count++;
    c_temp++;
    if (abs(out) <= theta){
        ++less_than_theta;
        ++less_temp;
        if (abs(out) <= theta / 2){
            ++less_than_half_of_theta;
            ++half_temp;
            if (abs(out) <= theta / 2){
                ++less_than_quater_of_theta;
                ++quater_temp;
            }
        }
    }
    if (count % 100000 == 0){

DPRINTFR(MYperceptron, "At the %lluth lookup, %d%% less than theta(%d),\
 %d%% less than theta/2(%d), %d%% less than theta/4(%d)\nDuring this period, \
 %d%% less than theta(%d), %d%% less than theta/2(%d), %d%% less than theta/4\
 (%d)\n",
                            count,100 * less_than_theta / count, theta,\
                            100 * less_than_half_of_theta / count, theta/2,\
                            100 * less_than_quater_of_theta / count, theta/4,
                            100 * less_temp / c_temp, theta,\
                            100 * half_temp / c_temp, theta/2,\
                            100 * quater_temp / c_temp, theta/4);
        c_temp = 0;
        less_temp = 0;
        half_temp = 0;
        quater_temp = 0;
    }
#endif

#if ALIASING
    static uint64_t count = 0;
    static uint64_t alias = 0;
    count++;
    if (thread_history != history_record[index] ||
           branch_addr != addr_record[index])
    {
        alias++;
    }
    history_record[index] = thread_history;
    addr_record[index] = branch_addr;

    if (count % 100000 == 0)
        DPRINTFR(MYperceptron, "%lluth Lookup: %llu! aliases\n", count, alias);
#endif


    BPHistory *history = new BPHistory;
    history->globalHistory = globalHistory[tid];
    history->globalPredTaken = taken;
    bp_history = (void *)history;

    // Speculative updates the GHR because of OoO
    if (taken)
        updateGlobalHistTaken(tid);
    else
        updateGlobalHistNotTaken(tid);

    return taken;
}

void
MyPerceptron::uncondBranch(ThreadID tid, Addr pc, void * &bp_history)
{
    BPHistory *history = new BPHistory;
    history->globalHistory = globalHistory[tid];
    history->globalPredTaken = true;
    history->globalUsed = true;
    bp_history = static_cast<void *>(history);
    updateGlobalHistTaken(tid);
}

void
MyPerceptron::update(ThreadID tid, Addr branch_addr, bool taken,
                     void *bp_history, bool squashed)
{
#if DEBUG
    static uint64_t count = 0;
    int interval = 1000;
#endif

    assert(bp_history);

    // Get the global history of this thread
    BPHistory *history = static_cast<BPHistory *>(bp_history);
    unsigned global_history = history->globalHistory;

    // Index of the perceptron to visit
    int index;

    // Hash the global history bits
    index = (global_history ^ (branch_addr >> 2)) % globalPredictorSize;

    // Calculate the output again
    int out = weights[index][0];

    for (int i = 0; i < sizeOfPerceptrons; i++){
        if ((global_history >> i) & 0x1)
            out += weights[index][i+1];
        else
            out -= weights[index][i+1];
    }

    // Updates if predicted incorrectly(squashed) or the output <= theta
    if (squashed || (abs(out) <= theta)){
#if DEBUG
        if (count++ % interval == 0){
            DPRINTFR(MYperceptron, "Updated for %llu times, weight is:\n",
                    count);
            for (int i = 0; i < globalPredictorSize; i++)
            {
                if (stat_perceptrons[i]){
                    DPRINTFR(MYperceptron, "Index %d: ", i);
                    for (int j = 0; j <= sizeOfPerceptrons; j++)
                    {
                        DPRINTFR(MYperceptron, "%3d ", weights[i][j]);
                    }
                    DPRINTFR(MYperceptron,"\n");
                }
            }
        }
        stat_perceptrons[index] = true;
#endif
        if (taken)
            weights[index][0] += 1;
        else
            weights[index][0] -= 1;

        for (int i = 0; i < sizeOfPerceptrons; i++){
            if (((global_history >> i) & 1) == taken)
                weights[index][i+1] += 1;
            else
                weights[index][i+1] -= 1;
        }
    }
}

// Recovers global history register while squashing
void
MyPerceptron::squash(ThreadID tid, void * bp_history)
{
    BPHistory *history = static_cast<BPHistory *>(bp_history);
    globalHistory[tid] = history->globalHistory;
    delete history;
}

unsigned
MyPerceptron::getGHR(ThreadID tid, void *bp_history) const
{
    return static_cast<BPHistory *>(bp_history)->globalHistory;
}

MyPerceptron *MyPerceptronParams::create()
{
    return new MyPerceptron(this);
}
