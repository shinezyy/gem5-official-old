#include "zperceptron.hh"

//#include <iostream>
#include <fstream>

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "debug/ZPerceptron.hh"

using namespace boost;

ZPerceptron *ZPerceptronParams::create()
{
    return new ZPerceptron(this);
}

void ZPerceptron::uncondBranch(ThreadID tid, Addr pc, void *&bp_history) {
    bp_history = new BPHistory(globalHistory[tid],
            emptyLocalHistory, InvalidTableIndex,
            true, InvalidPredictionID, 0);
    updateGHR(tid, true);
}

void ZPerceptron::btbUpdate(
        ThreadID tid, Addr branch_addr, void *&bp_history) {
    globalHistory[tid][0] = false;
    auto index = computeIndex(branch_addr);
    table[index].localHistory[0] = false;
}


void ZPerceptron::squash(ThreadID tid, void *bp_history) {
    auto history = static_cast<BPHistory *>(bp_history);
    globalHistory[tid] = history->globalHistory;

    if (history->tableIndex != InvalidTableIndex) {
        table[history->tableIndex].localHistory = history->localHistory;
    }

    delete history;
}

unsigned ZPerceptron::getGHR(ThreadID tid, void *bp_history) const {
    return 0;
}

uint32_t ZPerceptron::computeIndex(Addr addr) {
    return static_cast<uint32_t>((addr >> 2) % tableSize);
}

void ZPerceptron::updateGHR(ThreadID tid, bool taken) {
    globalHistory[tid] <<= 1;
    globalHistory[tid][0] = taken;
}

ZPerceptron::ZPerceptron(const ZPerceptronParams *params)
        : BPredUnit(params),
          globalHistoryLen(params->globalHistoryLen),
          tableSize(params->tableSize),
          emptyLocalHistory(1),
          globalHistory(params->numThreads,
                  dynamic_bitset<>(globalHistoryLen)),
          table(tableSize, NBEntry(params))
{
    uint32_t count = 0;
    for (auto &entry: table) {
        if (count++ == probeIndex) {
            entry.probing = true;
        }
    }
}

bool ZPerceptron::lookup(ThreadID tid, Addr branch_addr, void *&bp_bistory) {

    uint32_t index = computeIndex(branch_addr);
    dynamic_bitset<> &ghr = globalHistory[tid];
    NBEntry &entry = table.at(index);

    if (entry.probing && Debug::ZPerceptron) {
        DPRINTF(ZPerceptron, "Inst[0x%llx] with Pred[%llu]\n",
                branch_addr, predictionID);
        std::cout << "Using local: " << entry.localHistory
                  << ", global: " << globalHistory[tid] << std::endl;
    }


    int32_t prediction_val = entry.predict(ghr);
    bool result = prediction_val >= 0;
    bp_bistory = new BPHistory(ghr, entry.localHistory, index,
            result, predictionID++, prediction_val);

    updateGHR(tid, result);

    return result;
}


void ZPerceptron::update(ThreadID tid, Addr branch_addr, bool taken,
        void *bp_history, bool squashed) {
    auto history = static_cast<BPHistory *>(bp_history);

    auto index = computeIndex(branch_addr);
    NBEntry &entry = table.at(index);
    assert(entry.valid);

    if (squashed) {
        globalHistory[tid] = history->globalHistory << 1;
        globalHistory[tid][0] = taken;
        if (history->tableIndex != InvalidTableIndex) {
            entry.localHistory = history->localHistory << 1;
            entry.localHistory[0] = taken;
        }
        return;
    }

    if (entry.probing && Debug::ZPerceptron) {
        DPRINTF(ZPerceptron, "Inst[0x%llx] with Pred[%llu], ",
                branch_addr, history->predictionID);
        DPRINTFR(ZPerceptron, "correct:%d\n", history->predTaken == taken);
    }

    entry.fit(history, taken);

    if (entry.probing) {
        DPRINTF(ZPerceptron, "New prediction:\n");
    }
    entry.predict(history->globalHistory);
    if (entry.probing && Debug::ZPerceptron) {
        std::cout << "New local: " << entry.localHistory << std::endl;
    }

    delete history;
}




int32_t ZPerceptron::NBEntry::predict(boost::dynamic_bitset<> &ghr)
{
    int32_t sum = weights.back().read(); // bias
    for (int i = 0; i < globalHistoryLen; i++) {
        sum += b2s(ghr[i]) * weights[i].read();
    }
    return sum;
}

void ZPerceptron::NBEntry::fit(BPHistory *bp_history, bool taken) {


    if (taken == bp_history->predTaken &&
        abs(bp_history->predictionValue) > theta) {
        return;
    }

    if (taken) {
        weights.back().increment();
    } else {
        weights.back().decrement();
    }

    const auto &ghr = bp_history->globalHistory;

    for (int i = 0; i < globalHistoryLen; i++) {
        weights[i].add(b2s(taken) * b2s(ghr[i]));
    }
}

ZPerceptron::NBEntry::NBEntry(const ZPerceptronParams *params)
         : globalHistoryLen(params->globalHistoryLen),
         localHistory(params->localHistoryLen),
         weights(globalHistoryLen, SignedSatCounter(params->ctrBits, 0)),
         theta(static_cast<int32_t>(1.93 * globalHistoryLen + 14.0))

{
}

int ZPerceptron::NBEntry::b2s(bool taken) {
    // 1 -> 1; 0 -> -1
    return (taken << 1) - 1;
}

