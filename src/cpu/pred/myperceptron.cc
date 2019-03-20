#include "myperceptron.hh"

//#include <iostream>
#include <fstream>

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "debug/MYperceptron.hh"

#define PSEUDOTAGGING false

#define DYNAMIC_THRESHOLD true

#define DEBUG 0
#define COUNT 0
#define ALIASING 1
#define NU_RATIO 1


MyPerceptron::MyPerceptron(const MyPerceptronParams *params)
    : BPredUnit(params),
    globalPredictorSize(params->globalPredictorSize),
    globalHistory(params->numThreads, 0),
    globalHistoryBits(ceilLog2(params->globalPredictorSize)),
    sizeOfPerceptrons(params->sizeOfPerceptrons),
    pseudoTaggingBit(params->pseudoTaggingBit),
    indexMethod(params->indexMethod),
    bitsPerWeight(params->bitsPerWeight),
    lambda(params->lamda)
{
    if (!isPowerOf2(globalPredictorSize)) {
        fatal("Invalid global predictor size, should be power of 2.\n");
    }

    historyRegisterMask = mask(globalHistoryBits);

    // theta = 1.93 * sizeOfPerceptrons + 14;

    weights.assign(globalPredictorSize, std::vector<int>\
            (sizeOfPerceptrons + 1, 0));

    // weights could be positive or negative
    maxWeight = 1 << (bitsPerWeight - 1);

#if DYNAMIC_THRESHOLD
    thresholdBits = 10;
    thetas.assign(1 << thresholdBits, 1.93 * sizeOfPerceptrons + 14);

    thresholdCounterBits = 5;
    unsigned TC_initialValue = 1 << (thresholdCounterBits - 1);

    SatCounter tc(thresholdCounterBits, TC_initialValue);
    TC.assign(1 << thresholdBits, tc);
    //SatCounter TC(thresholdCounterBits, TC_initialValue);

    std::vector<SatCounter>::iterator sat_iter;

    for (sat_iter = TC.begin(); sat_iter != TC.end(); sat_iter++)
        (*sat_iter).reset();
    DPRINTFR(MYperceptron, "counter init is %d, max is %d\n",
        TC_initialValue, TC[0].readMax());
#endif


    DPRINTFR(MYperceptron, "maxWeight is %d, lambda is %d \n",
            maxWeight, lambda);

    if (indexMethod == "MODULO")
        hType = MODULO;
    else if (indexMethod == "BITWISE_XOR")
        hType = BITWISE_XOR;
    else if (indexMethod == "IPOLY")
        hType = IPOLY;
    else if (indexMethod == "PRIME_MODULO")
        hType = PRIME_MODULO;
    else if (indexMethod == "PRIME_DISPLACEMENT")
        hType = PRIME_DISPLACEMENT;
    else
        fatal("Invalid indexing method!\n");

    DPRINTFR(MYperceptron, "Using %s\n", indexMethod);

#if PSEUDOTAGGING
    pweights.assign(globalPredictorSize, std::vector<int>\
            (pseudoTaggingBit, 0));
    std::vector<unsigned>::iterator u_iter;
    for (u_iter = thetas.begin(); u_iter != thetas.end(); u_iter++)
        *u_iter += 1.93 * pseudoTaggingBit;
#endif


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

int
MyPerceptron::getIndex(hash_type type, Addr branch_addr,
        uint64_t global_history)
{
    if (type == MODULO)
        return (global_history ^ (branch_addr >> 2)) & historyRegisterMask;
    else if (type == BITWISE_XOR){
        uint64_t x = branch_addr >> 2;
        uint64_t y = branch_addr >> (2 + globalHistoryBits);
        return (x ^ y ^ global_history) & historyRegisterMask;
    }
    else if (type == PRIME_DISPLACEMENT){
        uint64_t T = branch_addr >> (2 + globalHistoryBits);
        uint64_t x = branch_addr >> 2;
        // A prime number
        uint64_t p = 8209;
        return (T * p + x) & historyRegisterMask;
    }
    else if (type == PRIME_MODULO){
        uint64_t p = 8209;
        return ((branch_addr >> 2) % p) & historyRegisterMask;
    }
    else
        fatal("Not implemented indexing method!\n");
}

int
MyPerceptron::getIndexTheta(Addr branch_addr)
{
    return (branch_addr >> 2) & mask(thresholdBits);
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

    // Indexing
    index = getIndex(hType, branch_addr, thread_history);
    assert(index < globalPredictorSize);

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

#if PSEUDOTAGGING
    for (int j = 0; j < pseudoTaggingBit; j++)
        if ((branch_addr >> (2 * (j + 1))) & 0x1)
            out += pweights[index][j];
        else
            out -= pweights[index][j];
#endif


    // Use the sign bit as the result
    bool taken = (out >= 0);

#if COUNT
    int t_index = getIndexTheta(branch_addr);
    unsigned theta = thetas[t_index];

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
    static uint64_t count = 0;
    int interval = 10000;

    count++;

    assert(bp_history);

    // Get the global history of this thread
    BPHistory *history = static_cast<BPHistory *>(bp_history);
    unsigned global_history = history->globalHistory;

    // Index of the perceptron to visit
    int index;

    // Indexing
    index = getIndex(hType, branch_addr, global_history);
    assert(index < globalPredictorSize);


    // Calculate the output again
    int out = weights[index][0];

    for (int i = 0; i < sizeOfPerceptrons; i++)
        if ((global_history >> i) & 0x1)
            out += weights[index][i+1];
        else
            out -= weights[index][i+1];

#if PSEUDOTAGGING
    for (int j = 0; j < pseudoTaggingBit; j++)
        if ((branch_addr >> (2 * (j + 1))) & 0x1)
            out += pweights[index][j];
        else
            out -= pweights[index][j];
#endif


    //DPRINTFR(MYperceptron,"theta is %d\n", theta);
    int t_index = getIndexTheta(branch_addr);
    unsigned theta = thetas[t_index];

    // Updates if predicted incorrectly(squashed) or the output <= theta
    if (squashed || (abs(out) <= theta)){
#if DEBUG
        if (count % interval == 0){
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
            if (weights[index][0] + lambda <= maxWeight)
                weights[index][0] += lambda;
            else
                weights[index][0] = maxWeight;
        else
            if (weights[index][0] - lambda >= -maxWeight)
                weights[index][0] -= lambda;
            else
                weights[index][0] = -maxWeight;

        for (int i = 0; i < sizeOfPerceptrons; i++){
            if (((global_history >> i) & 1) == taken)
                if (weights[index][i+1] + lambda <= maxWeight)
                    weights[index][i+1] += lambda;
                else
                    weights[index][i+1] = maxWeight;
            else
                if (weights[index][i+1] - lambda >= maxWeight)
                    weights[index][i+1] -= lambda;
                else
                    weights[index][i+1] = -maxWeight;
        }

#if PSEUDOTAGGING
        for (int j = 0; j < pseudoTaggingBit; j++){
            if (((branch_addr >> (2 * (j + 1))) & 0x1) == taken)
                pweights[index][j] += 1;
            else
                pweights[index][j] -= 1;
        }
#endif

    }

#if NU_RATIO

    static uint64_t NU_miss = 0;
    static uint64_t NU_miss_temp = 0;
    static uint64_t NU_correct = 0;
    static uint64_t NU_correct_temp = 0;

    if (squashed)
        NU_miss++;
    else if (abs(out) <= theta)
        NU_correct++;

    if (count % interval == 0){
        DPRINTFR(MYperceptron, "NU_RATIO: At %lluth update, NU_miss/NU_correct\
=%f, theta=%u\n", count, float(NU_miss - NU_miss_temp) / \
(NU_correct - NU_correct_temp), theta);
        NU_miss_temp = NU_miss;
        NU_correct_temp = NU_correct;
    }
#endif

#if DYNAMIC_THRESHOLD
    if (squashed){
        if (TC[t_index].increment()){
            thetas[t_index] += 1;
            TC[t_index].reset();
        }
    }
    else if (abs(out) <= theta){
        if (TC[t_index].decrement()){
            thetas[t_index] -= 1;
            TC[t_index].reset();
        }
    }
#endif

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
