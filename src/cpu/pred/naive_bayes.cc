#include "naive_bayes.hh"

//#include <iostream>
#include <fstream>

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "debug/NBBP.hh"

using namespace boost;

NBBP *NBBPParams::create()
{
    return new NBBP(this);
}

void NBBP::uncondBranch(ThreadID tid, Addr pc, void *&bp_history) {
    bp_history = new BPHistory(globalHistory[tid],
            emptyLocalHistory, InvalidTableIndex, true, InvalidPredictionID);
    updateGHR(tid, true);
}

void NBBP::btbUpdate(ThreadID tid, Addr branch_addr, void *&bp_history) {
    globalHistory[tid][0] = false;
    auto index = computeIndex(branch_addr);
    table[index].localHistory[0] = false;
}


void NBBP::squash(ThreadID tid, void *bp_history) {
    auto history = static_cast<BPHistory *>(bp_history);
    globalHistory[tid] = history->globalHistory;

    if (history->tableIndex != InvalidTableIndex) {
        table[history->tableIndex].localHistory = history->localHistory;
    }

    delete history;
}

unsigned NBBP::getGHR(ThreadID tid, void *bp_history) const {
    return 0;
}

uint32_t NBBP::computeIndex(Addr addr) {
    return static_cast<uint32_t>((addr >> 2) % tableSize);
}

void NBBP::updateGHR(ThreadID tid, bool taken) {
    globalHistory[tid] <<= 1;
    globalHistory[tid][0] = taken;
}

NBBP::NBBP(const NBBPParams *params)
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

bool NBBP::lookup(ThreadID tid, Addr branch_addr, void *&bp_bistory) {

    uint32_t index = computeIndex(branch_addr);
    dynamic_bitset<> &ghr = globalHistory[tid];
    NBEntry &entry = table.at(index);

    if (entry.probing && Debug::NBBP) {
        DPRINTF(NBBP, "Inst[0x%llx] with Pred[%llu]\n",
                branch_addr, predictionID);
        std::cout << "Using local: " << entry.localHistory
                  << ", global: " << globalHistory[tid] << std::endl;
    }


    bool result = entry.predict(ghr) > 1.0;
    bp_bistory = new BPHistory(ghr, entry.localHistory, index,
            result, predictionID++);

    updateGHR(tid, result);
    entry.localHistory <<= 1;
    entry.localHistory[0] = result;

    return result;
}


void NBBP::update(ThreadID tid, Addr branch_addr, bool taken,
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

    if (entry.probing && Debug::NBBP) {
        DPRINTF(NBBP, "Inst[0x%llx] with Pred[%llu], ",
                branch_addr, history->predictionID);
        DPRINTFR(NBBP, "correct:%d\n", history->predTaken == taken);
    }

    entry.fit(history, taken);

    if (entry.probing) {
        DPRINTF(NBBP, "New prediction:\n");
    }
    entry.predict(history->globalHistory);
    if (entry.probing && Debug::NBBP) {
        std::cout << "New local: " << entry.localHistory << std::endl;
    }

    delete history;
}




double NBBP::NBEntry::predict(boost::dynamic_bitset<> &ghr) {
    double y_t = ((double) prioriProb) / counterMax;
    double y_nt = 1.0 - ((double) prioriProb) / counterMax;

    auto f = [&](const boost::dynamic_bitset<> &history,
            const std::vector<NBBP::NBEntry::NBCounter> &probs,
            uint32_t index) {
        double prob = history[index] ? probs[index].takenOnT :
                      probs[index].takenOnNT;
        prob /= counterMax;
        if (probing) {
            DPRINTFR(NBBP, "%.2f/%.2f, ", prob, 1.0 - prob);
        }
        y_t *= prob;
        y_nt *= 1.0 - prob;
//        if (probing) {
//            DPRINTFR(NBBP, "(%.6f/%.6f), ", y_t, y_nt);
//        }
    };

    if (probing) {
        DPRINTFR(NBBP, "bias: %.2f/%.2f, Local ::", y_t, y_nt);
    }

    for (uint32_t i = 0; i < nLocal; i++) {
        f(localHistory, localPosteriorProb, i);
    }

    if (probing) {
        DPRINTFR(NBBP, "\nGlobal ::");
    }
    for (uint32_t i = 0; i < nGlobal; i++) {
        f(ghr, globalPosteriorProb, i);
    }
    if (probing) {
        DPRINTFR(NBBP, "Final prediction: %f/%f\n", y_t, y_nt);
    }
    return y_t/y_nt;
}

bool NBBP::NBEntry::neg_contrib(bool taken, int32_t prob) {
    return (taken && prob < origin) || (!taken && prob > origin);
}

void NBBP::NBEntry::fit(BPHistory *bp_history, bool taken) {

    // shrink priori prob
    const auto &ghr = bp_history->globalHistory;

    if (bp_history->predTaken != taken && neg_contrib(taken, prioriProb)) {
        int32_t diff = prioriProb - origin;
        prioriProb = counterMax / 2 +
                static_cast<int32_t>(shrinkFactor * diff);
    }

    if (taken) {
        prioriProb = std::min(counterMax, prioriProb + 1);
    } else {
        prioriProb = std::max(1, prioriProb - 1);
    }

    auto f = [&](std::vector<NBCounter> &post_probs,
            const dynamic_bitset<> &history_reg, uint32_t index) {

        int32_t &v = history_reg[index] ? post_probs[index].takenOnT :
                     post_probs[index].takenOnNT;

        if (bp_history->predTaken != taken && neg_contrib(taken ,v)) {
            post_probs[index].shrink(history_reg[index], shrinkFactor);
        }

        if (taken) {
            v = std::min(counterMax - 1, v + 1);
        } else {
            v = std::max(1, v - 1);
        }
    };

    for (uint32_t i = 0; i < nLocal; i++) {
        f(localPosteriorProb, localHistory, i);
    }
    for (uint32_t i = 0; i < nGlobal; i++) {
        f(globalPosteriorProb, ghr, i);
    }
}

NBBP::NBEntry::NBEntry(const NBBPParams *params)
        :localHistoryLen(params->localHistoryLen),
         globalHistoryLen(params->globalHistoryLen),

         nLocal(params->nLocalCounter),
         nGlobal(params->nGlobalCounter),

         localHistory(params->localHistoryLen),

         counterMax(static_cast<const uint32_t>(power(2, params->ctrBits))),
         origin(counterMax/2),

         prioriProb(origin),
         localPosteriorProb(nLocal, NBCounter(counterMax)),
         globalPosteriorProb(nGlobal, NBCounter(counterMax))
{
}

void NBBP::NBEntry::init() {

}

void NBBP::NBEntry::NBCounter::shrink(bool taken, float shrink_factor) {
    int32_t &v = taken ? takenOnT : takenOnNT;
    int32_t diff = v - origin;
    v = origin + static_cast<int32_t>(shrink_factor * diff);
}
