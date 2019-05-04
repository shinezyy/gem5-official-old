#include "myperceptron.hh"

//#include <iostream>
#include <fstream>

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "debug/MYperceptron.hh"

#define SPECULATIVE_UPDATE 1

#define COUNT 0
#define ALIASING 0
#define NU_RATIO 0
#define TABLE_USAGE 0

MyPerceptron::MyPerceptron(const MyPerceptronParams *params)
    : BPredUnit(params),
    globalPredictorSize(params->globalPredictorSize),
    threadHistory(params->numThreads),
    globalHistoryBits(ceilLog2(params->globalPredictorSize)),
    sizeOfPerceptrons(params->sizeOfPerceptrons),
    pseudoTaggingBit(params->pseudoTaggingBit),
    indexMethod(params->indexMethod),
    bitsPerWeight(params->bitsPerWeight),
    lambda(params->lamda),
    thresholdBits(params->dynamicThresholdBit),
    thresholdCounterBits(params->thresholdCounterBit),
    redundantBit(params->redundantBit),
    maxHisLen(params->maxHisLen)
{

    for (auto& history : threadHistory){
        history.globalHistory = new uint8_t[maxHisLen];
    }
    if (redundantBit > 0)
        weights.assign(globalPredictorSize, std::vector<int>\
            (sizeOfPerceptrons*redundantBit + 1, 0));
    else
        weights.assign(globalPredictorSize, std::vector<int>\
            (sizeOfPerceptrons + 1, 0));


    // weights could be positive or negative
    maxWeight = 1 << (bitsPerWeight - 1);

    // Dynamic threshold
    if (thresholdBits > 0){
        // Multiple thetas
        if (redundantBit > 0)
            thetas.assign(1 << thresholdBits, 1.93 * redundantBit *
                    sizeOfPerceptrons + 14);
        else
            thetas.assign(1 << thresholdBits, 1.93 * sizeOfPerceptrons + 14);

        // Init TCs with medium value
        unsigned TC_initialValue = 1 << (thresholdCounterBits - 1);

        // 2**thresholdBits TCs
        SatCounter tc(thresholdCounterBits, TC_initialValue);
        TC.assign(1 << thresholdBits, tc);

        std::vector<SatCounter>::iterator sat_iter;

        for (sat_iter = TC.begin(); sat_iter != TC.end(); sat_iter++)
            (*sat_iter).reset();
        DPRINTFR(MYperceptron, "counter init is %d, max is %d\n",
            TC_initialValue, TC[0].readMax());
    }
    else{
        if (redundantBit > 0)
            thetas.assign(1, 1.93 * redundantBit * sizeOfPerceptrons + 14);
        else
            thetas.assign(1, 1.93 * sizeOfPerceptrons + 14);
    }


    // Set indexing methods
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

    // Set pseudo tagging weights
    if (pseudoTaggingBit > 0){
        pweights.assign(globalPredictorSize, std::vector<int>\
            (pseudoTaggingBit, 0));
        std::vector<unsigned>::iterator u_iter;
        // Weights are added, so thetas should also be larger
        for (u_iter = thetas.begin(); u_iter != thetas.end(); u_iter++)
            *u_iter += 1.93 * pseudoTaggingBit;
    }

    DPRINTFR(MYperceptron, "globalPredictorSize is %d, globalHistoryBits is\
%d, size of perceptrons is %d, historyRegisterMask is %d,\
redundant bit is %d, maxHisLen is %d\n",
globalPredictorSize, globalHistoryBits, sizeOfPerceptrons,
historyRegisterMask, redundantBit, maxHisLen);

    DPRINTFR(MYperceptron, "maxWeight is %d, lambda is %d, theta is %d\n",
            maxWeight, lambda, thetas[0]);


#if ALIASING
    addr_record.assign(globalPredictorSize, 0);
    taken_record.assign(globalPredictorSize, false);
#endif

#if TABLE_USAGE
    index_count.assign(globalPredictorSize, 0);
#endif
}

inline void
MyPerceptron::updateGlobalHist(ThreadID tid, bool taken)
{
    for (int i = maxHisLen - 1; i > 0; i--){
        threadHistory[tid].globalHistory[i] =
            threadHistory[tid].globalHistory[i-1];
    }
    threadHistory[tid].globalHistory[0] = taken ? 1 : 0;
}

void
MyPerceptron::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    threadHistory[tid].globalHistory[0] = 0;
}

inline int
MyPerceptron::getIndex(hash_type type, Addr branch_addr,
        uint8_t *global_history)
{
    if (type == MODULO)
        //return (branch_addr >> 2) & historyRegisterMask;
        return (branch_addr >> 2) % globalPredictorSize;
    else if (type == BITWISE_XOR){
        uint64_t x = branch_addr >> 2;
        uint64_t y = branch_addr >> (2 + globalHistoryBits);
        //uint64_t g = global_history;
        //g ^= global_history >> globalHistoryBits;
        //g ^= global_history >> (globalHistoryBits * 2);
        return (x ^ y) % globalPredictorSize;
    }
    else if (type == PRIME_DISPLACEMENT){
        uint64_t T = branch_addr >> (2 + globalHistoryBits);
        uint64_t x = (branch_addr >> 2) & historyRegisterMask;
        // A prime number
        uint64_t p = 17;

        return (T * p + x) % globalPredictorSize;

        //uint64_t g = global_history;
        //g ^= global_history >> globalHistoryBits;
        //g ^= global_history >> (2 * globalHistoryBits);

        //return ((T * p + x) ^ g) & historyRegisterMask;
    }
    else if (type == PRIME_MODULO){
        uint64_t p = 8209;
        return ((branch_addr >> 2) % p) % globalPredictorSize;
    }
    else
        fatal("Not implemented indexing method!\n");
}

inline int
MyPerceptron::getIndexTheta(Addr branch_addr)
{
    return (branch_addr >> 2) & mask(thresholdBits);
}


uint8_t *
MyPerceptron::redundantHistory(uint8_t *history)
{
    uint8_t *res;
    if (redundantBit > 0){
        res = new uint8_t[sizeOfPerceptrons*redundantBit];
        //uint8_t **H = new uint8_t*[redundantBit];
        uint8_t *H[redundantBit];
        // res layout:
        // H[rBit-1], H[rBit-2], ..., H[1], H[0]
        for (int i = 0; i < redundantBit; i++)
            H[i] = &(res[i * sizeOfPerceptrons]);
        for (int i = 0; i < sizeOfPerceptrons; i++){
            for (int j = 0; j < redundantBit; j++){
                H[j][i] = history[i];
                if (j == 0)
                    continue;
                else
                    H[j][i] ^= history[i + j];
            }
        }
        //delete[] H;
    }
    // If not redundant, return history itself
    else{
        res = history;
    }
    return res;
}

int
MyPerceptron::computeOutput(uint8_t *history, int index, Addr addr)
{
    std::vector<int> perceptron = weights[index];

    // Bias term
    int out = perceptron[0];

    uint8_t *input = redundantHistory(history);

    // Weights

    for (int i = 0; i < sizeOfPerceptrons; i++){
        if (input[i]){
            out += perceptron[i+1];
        }
        else{
            out -= perceptron[i+1];
        }
    }

    if (redundantBit > 0){
        for (int i = 0; i < sizeOfPerceptrons; i++){
            for (int j = 1; j < redundantBit; j++){
                int ptr = i + j * sizeOfPerceptrons;
                if (input[ptr]){
                    out += perceptron[ptr+1];
                }
                else{
                    out -= perceptron[ptr+1];
                }
            }
        }
        // Recycle memory if newed
        delete[] input;
    }

    // Pseudo-tagging weights
    if (pseudoTaggingBit > 0){
        for (int j = 0; j < pseudoTaggingBit; j++)
            if ((addr >> (globalHistoryBits + (j + 1) + 2)) & 0x1)
                out += pweights[index][j];
            else
                out -= pweights[index][j];
    }
    return out;
}

bool
MyPerceptron::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{
#if COUNT || NU_RATIO || ALIASING || TABLE_USAGE
    static uint64_t count = 0;
    count++;
#endif
#if COUNT
    static uint64_t less_than_theta = 0;
    static uint64_t less_than_half_of_theta = 0;
    static uint64_t less_than_quater_of_theta = 0;

    static uint64_t c_temp = 0;
    static uint64_t less_temp = 0;
    static uint64_t half_temp = 0;
    static uint64_t quater_temp = 0;
#endif

    // Get the global history of this thread
    uint8_t *global_history = threadHistory[tid].globalHistory;

    // Index of the perceptron to visit
    int index;

    // Indexing
    index = getIndex(hType, branch_addr, global_history);
    assert(index < globalPredictorSize);



    int out = computeOutput(global_history, index, branch_addr);

    // Use the sign bit as the result
    bool taken = (out >= 0);

#if COUNT
    int t_index;

    if (thresholdBits > 0)
        t_index = getIndexTheta(branch_addr);
    else
        t_index = 0;

    unsigned theta = thetas[t_index];

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
                            101 * less_than_quater_of_theta / count, theta/4,
                            100 * less_temp / c_temp, theta,\
                            100 * half_temp / c_temp, theta/2,\
                            100 * quater_temp / c_temp, theta/4);
        c_temp = 0;
        less_temp = 0;
        half_temp = 0;
        quater_temp = 0;
    }
#endif



    BPHistory *history = new BPHistory(maxHisLen);
    for (int i = 0; i < maxHisLen; i++)
        history->globalHistory[i] = threadHistory[tid].globalHistory[i];
    history->globalPredTaken = taken;
    bp_history = (void *)history;

    // Speculative updates the GHR because of OoO
    updateGlobalHist(tid, taken);

    return taken;
}

void
MyPerceptron::uncondBranch(ThreadID tid, Addr pc, void * &bp_history)
{
    BPHistory *history = new BPHistory(maxHisLen);
    for (int i = 0; i < maxHisLen; i++)
        history->globalHistory[i] = threadHistory[tid].globalHistory[i];
    history->globalPredTaken = true;
    history->globalUsed = true;
    bp_history = static_cast<void *>(history);
    updateGlobalHist(tid, true);
}

void
MyPerceptron::train(std::vector<int>& perceptron,
        std::vector<int>& pperceptron,
        bool taken, uint8_t *global_history, Addr branch_addr)
{
    if (taken){
        perceptron[0] += lambda;
        if (perceptron[0] > maxWeight)
            perceptron[0] = maxWeight;
    }
    else{
        perceptron[0] -= lambda;
        if (perceptron[0] < -maxWeight)
            perceptron[0] = -maxWeight;
    }

    // Get input used to calculate the output
    uint8_t *input = redundantHistory(global_history);

    // First lower bits, using original history
    for (int i = 0; i < sizeOfPerceptrons; i++){
        if (input[i] == taken){
            perceptron[i+1] += lambda;
            if (perceptron[i+1] > maxWeight)
                perceptron[i+1] = maxWeight;
        }
        else{
            perceptron[i+1] -= lambda;
            if (perceptron[i+1] < -maxWeight)
                perceptron[i+1] = -maxWeight;
        }
    }

    // Then redundant bits
    if (redundantBit > 0){
        for (int i = 0; i < sizeOfPerceptrons; i++){
            for (int j = 1; j < redundantBit; j++){
                int ptr = i + j * sizeOfPerceptrons;
                if (input[ptr] == taken){
                    perceptron[ptr+1] += lambda;
                    if (perceptron[ptr+1] > maxWeight)
                        perceptron[ptr+1] = maxWeight;
                }
                else{
                    perceptron[ptr] -= lambda;
                    if (perceptron[ptr+1] < -maxWeight)
                        perceptron[ptr+1] = -maxWeight;
                }
            }
        }
        // Recycle memory if newed
        delete[] input;
    }

    if (pseudoTaggingBit > 0){
        for (int j = 0; j < pseudoTaggingBit; j++){
            if (((branch_addr >> (globalHistoryBits+(j+2)+1)) & 0x1)
                    == taken){
                pperceptron[j] += lambda;
                if (pperceptron[j] > maxWeight)
                    pperceptron[j] = maxWeight;
            }
            else{
                pperceptron[j] -= lambda;
                if (pperceptron[j] < -maxWeight)
                    pperceptron[j] = -maxWeight;
            }
        }
    }

}

void
MyPerceptron::updateThreshold(int t_index, bool incorrect, bool unconfident)
{
    if (incorrect){
        if (TC[t_index].increment()){
            thetas[t_index] += 1;
            TC[t_index].reset();
        }
    }
    else if (unconfident){
        if (TC[t_index].decrement()){
            thetas[t_index] -= 1;
            TC[t_index].reset();
        }
    }

}

void
MyPerceptron::update(ThreadID tid, Addr branch_addr, bool taken,
                     void *bp_history, bool squashed)
{
    static uint64_t count = 0;
#if NU_RATIO
    int interval = 10000;
#endif
    assert(bp_history);

    // Called during squash, update GHR with correct result and return
    if (squashed){
        BPHistory *history = static_cast<BPHistory *> (bp_history);
        for (int i = 0; i < maxHisLen - 1; i++)
            threadHistory[tid].globalHistory[i+1] = history->globalHistory[i];
        threadHistory[tid].globalHistory[0] = taken ? 1 : 0;
        return;
    }

    count++;

    // Get the prediction
    BPHistory *history = static_cast <BPHistory *>(bp_history);

    // Get the global history of this thread
    uint8_t *global_history = history->globalHistory;

    bool prediction = history->globalPredTaken;

    bool incorrect = taken != prediction;

    // Indexing
    int index = getIndex(hType, branch_addr, global_history);
    assert(index < globalPredictorSize);


    // Calculate the output again
    int out = computeOutput(global_history, index, branch_addr);

    // Get theta
    int t_index;

    // if used dynamic threshold
    if (thresholdBits > 0)
        t_index = getIndexTheta(branch_addr);
    else
        t_index = 0;

    unsigned theta = thetas[t_index];

    bool unconfident = abs(out) <= theta;

    // Updates if predicted incorrectly(squashed) or the output <= theta
    if (incorrect || unconfident){
        train(weights[index], pweights[index],
                taken, global_history, branch_addr);
    }

    // Dynamic threshold training
    if (thresholdBits > 0)
        updateThreshold(t_index, incorrect, unconfident);

#if ALIASING
    static uint64_t alias = 0;
    static uint64_t dalias = 0;

    if (branch_addr != addr_record[index]){
        alias++;
        if (taken != taken_record[index])
            dalias++;
    }

    addr_record[index] = branch_addr;
    taken_record[index] = taken;

    if (count % 100000 == 0){
        DPRINTFR(MYperceptron, "%lluth Lookup: %llu! aliases\n", count, alias);
        DPRINTFR(MYperceptron, "%lluth Lookup: %llu@ daliases\n",count,dalias);
        alias = 0;
        dalias = 0;
    }
#endif


#if NU_RATIO

    static uint64_t NU_miss = 0;
    static uint64_t NU_miss_temp = 0;
    static uint64_t NU_correct = 0;
    static uint64_t NU_correct_temp = 0;

    if (incorrect)
        NU_miss++;
    else if (unconfident)
        NU_correct++;

    if (count % interval == 0){
        DPRINTFR(MYperceptron, "NU_RATIO: At %lluth ratio\
=%f, theta=%u\n", count, float(NU_miss - NU_miss_temp) / \
(NU_correct - NU_correct_temp), theta);
        NU_miss_temp = NU_miss;
        NU_correct_temp = NU_correct;
    }
#endif


#if TABLE_USAGE
    index_count[index] += 1;
    DPRINTFR(MYperceptron, "branch_addr: 0x%x, index: %d, taken: %d\n"
            , branch_addr, index, taken ? 1 : 0);
    if (count % 100000 == 0){
        DPRINTFR(MYperceptron, "At %lluth update\n", count);
    }
#endif

    if (!squashed){
        delete history;
    }

}

// Recovers global history register while squashing
void
MyPerceptron::squash(ThreadID tid, void * bp_history)
{
    BPHistory *history = static_cast<BPHistory *>(bp_history);
    for (int i = 0; i < sizeOfPerceptrons; i++)
        threadHistory[tid].globalHistory[i] = history->globalHistory[i];
    delete history;
}

unsigned
MyPerceptron::getGHR(ThreadID tid, void *bp_history) const
{
    BPHistory *history = static_cast<BPHistory *> (bp_history);
    unsigned val = 0;
    for (int i = 0; i < 32; i++)
        val |= (history->globalHistory)[i] << i;
    return val;
}

MyPerceptron *MyPerceptronParams::create()
{
    return new MyPerceptron(this);
}
