#include "online_gradient_boosting.hh"

//#include <iostream>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "debug/OGB.hh"
#include "debug/OGB2.hh"

using namespace boost;

OGBBP *OGBBPParams::create()
{
    return new OGBBP(this);
}

bool OGBBP::lookup(ThreadID tid, Addr branch_addr, void *&bp_bistory) {

    uint32_t index = computeIndex(branch_addr);
    dynamic_bitset<> &ghr = globalHistory[tid];
    OGBEntry &ogbe = table.at(index);

//    auto probe_old = ogbe.probing;
//    if (predictionID > probe_pred_id) {
//        ogbe.probing = true;
//    }

    if (ogbe.probing && Debug::OGB) {
        DPRINTF(OGB, "Inst[0x%llx] with Pred[%llu]\n",
                branch_addr, predictionID);
        std::cout << "Using local: " << ogbe.localHistory
                  << ", global: " << globalHistory[tid] << std::endl;
    }

    bool result = ogbe.predict(ghr) > 0;
    bp_bistory = new BPHistory(
            ghr, ogbe.localHistory, index, result, predictionID++);

//    if (predictionID > probe_pred_id) {
//        ogbe.probing = probe_old;
//    }
    updateGHR(tid, result);
    ogbe.localHistory <<= 1;
    ogbe.localHistory[0] = result;

    return result;
}

void OGBBP::uncondBranch(ThreadID tid, Addr pc, void *&bp_history) {
    bp_history = new BPHistory(globalHistory[tid],
            emptyLocalHistory, InvalidTableIndex, true, InvalidPredictionID);
    updateGHR(tid, true);
}

void OGBBP::btbUpdate(ThreadID tid, Addr branch_addr, void *&bp_history) {
    globalHistory[tid][0] = false;
    auto index = computeIndex(branch_addr);
    table[index].localHistory[0] = false;
}

void OGBBP::update(ThreadID tid, Addr branch_addr, bool taken,
        void *bp_history, bool squashed) {
    auto history = static_cast<BPHistory *>(bp_history);

    auto index = computeIndex(branch_addr);
    OGBEntry &ogbe = table.at(index);

//    auto probe_old = ogbe.probing;
//    if (predictionID > probe_pred_id) {
//        ogbe.probing = true;
//    }

    if (squashed) {
        globalHistory[tid] = history->globalHistory << 1;
        globalHistory[tid][0] = taken;
        if (history->tableIndex != InvalidTableIndex) {
            ogbe.localHistory = history->localHistory << 1;
            ogbe.localHistory[0] = taken;
        }
//        if (predictionID > probe_pred_id) {
//            ogbe.probing = probe_old;
//        }
        return;
    }

    if (ogbe.probing && Debug::OGB) {
        DPRINTF(OGB, "Inst[0x%llx] with Pred[%llu], ",
                branch_addr, history->predictionID);
        DPRINTFR(OGB, "correct:%d\n", history->predTaken == taken);
    }

    if (history->predTaken != taken) {
        if (misPredictions.count(branch_addr) > 0) {
            misPredictions[branch_addr] += 1;
        } else {
            misPredictions[branch_addr] = 1;
        }
        misses += 1;
        if (Debug::OGB2 && misses % 100000 == 0) {
            std::cout << "----------------------------------------------\n";
            for (const auto & e: misPredictions) {
                std::cout << e.first << ":" << e.second << "\n";
            }
        }
    }

    if (history->tableIndex != InvalidTableIndex &&
        history->localHistory.size() < localHistoryLen) {
        DPRINTF(OGB, "local history length of %llu is %d\n",
                history->predictionID, history->localHistory.size());
        panic("sanity check failed");
    }

    ogbe.gradient_descent(history, taken);
//    if (predictionID > probe_pred_id) {
//        ogbe.probing = probe_old;
//    }

    if (ogbe.probing && Debug::OGB) {
        std::cout << "New local: " << ogbe.localHistory
                  << ", global: " << globalHistory[tid] << std::endl;
    }

    delete history;
}

void OGBBP::squash(ThreadID tid, void *bp_history) {
    auto history = static_cast<BPHistory *>(bp_history);
    globalHistory[tid] = history->globalHistory;

    if (history->tableIndex != InvalidTableIndex) {
        table[history->tableIndex].localHistory = history->localHistory;
        if (history->localHistory.size() < localHistoryLen) {
            DPRINTF(OGB, "local history length of %llu is %d\n",
                    history->predictionID, history->localHistory.size());
            panic("sanity check failed");
        }
    }

    delete history;
}

unsigned OGBBP::getGHR(ThreadID tid, void *bp_history) const {
    return 0;
}

OGBBP::OGBBP(const OGBBPParams *params)
        :
        BPredUnit(params),

        beginning(myClock::now()),

        globalHistoryLenLog(params->globalHistoryLenLog),
        globalHistoryLen(params->globalHistoryLen),
        treeHeight(params->treeHeight),
        localHistoryLen(params->localHistoryLen),
        tableSize(params->tableSize),
        factorBits(params->factorBits),
        nTrees(params->nTrees),
        ctrBits(params->ctrBits),
        emptyLocalHistory(1),

        globalHistory(params->numThreads,
                boost::dynamic_bitset<>(globalHistoryLen)),

        table(tableSize,
                OGBEntry(localHistoryLen, globalHistoryLen, nTrees,
                        params->nLocal, factorBits, treeHeight, ctrBits))
         {
    uint32_t count = 0;
    for (auto &ogbe: table) {
        if (count++ == probeIndex) {
            ogbe.probing = true;
        }
        ogbe.init(beginning);
    }
}

uint32_t OGBBP::computeIndex(Addr addr) {
    return static_cast<uint32_t>((addr >> 2) % tableSize);
}

void OGBBP::updateGHR(ThreadID tid, bool taken) {
    globalHistory[tid] <<= 1;
    globalHistory[tid][0] = taken;
}

void OGBBP::OGBEntry::init(myClock::time_point beginning) {
    valid = true;
    assert(nTrees > nLocal);

    // init for local
    std::vector<uint32_t> cards(localHistoryLen);
    std::iota(cards.begin(), cards.end(), 0);
    std::shuffle(cards.begin(), cards.end(),
            std::default_random_engine(
                    static_cast<unsigned long>(
                            (myClock::now() - beginning).count())));

    uint32_t cursor = 0;
    for (uint32_t i = 0; i < nLocal; i++) {
        for (auto &node: trees.at(i).treeNodes) {
            if (i < 2) {
                node = i;
            } else {
                node = cards[cursor];
                cursor = (cursor+1) % localHistoryLen;
            }
        }
        if (probing) {
//            DPRINTF(OGB, "Tree[%d] (local): %d, %d, %d\n",
//                    i, trees[i].treeNodes[0],
//                    trees[i].treeNodes[1], trees[i].treeNodes[2]);
            DPRINTF(OGB, "Tree[%d] (local): %d\n",
                    i, trees[i].treeNodes[0]);
        }
    }

    uint32_t r = globalHistoryLen;
    for (uint32_t i = nTrees - 1; i >= nLocal; i--) {
        r = r*25/32;
        cards.resize(r);
        std::iota(cards.begin(), cards.end(), 0);
        std::shuffle(cards.begin(), cards.end(),
                     std::default_random_engine(
                             static_cast<unsigned long>(
                                     (myClock::now() - beginning).count())));
        cursor = 0;
        for (auto &node: trees.at(i).treeNodes) {
            node = cards[cursor];
            cursor = (cursor+1) % r;
        }

        if (probing) {
//            DPRINTF(OGB, "Tree[%d] (global): %d, %d, %d\n",
//                    i, trees[i].treeNodes[0],
//                    trees[i].treeNodes[1], trees[i].treeNodes[2]);
            DPRINTF(OGB, "Tree[%d] (global): %d\n",
                    i, trees[i].treeNodes[0]);
        }
    }

}

float OGBBP::OGBEntry::predict(boost::dynamic_bitset<> &ghr) {
    float y = baseValue.read();
    if (probing) {
        DPRINTFR(OGB, "bias: %.2f\n", y);
    }
    for (uint32_t i = 0; i < nTrees; i++) {
        if (probing) {
            DPRINTFR(OGB, "T[%d] ", i);
        }
        dynamic_bitset<> &history = i < nLocal ? localHistory : ghr;
        Tree &tree = trees.at(i);

        uint32_t tree_path = 0;
        uint32_t p = 0;

        for (uint32_t d = 0; d < treeHeight; d++) {
            bool taken = history[tree.treeNodes.at(p)];
            if (probing) {
                DPRINTFR(OGB, "%s[%d]: %d, ", i<nLocal ? "local": "global",
                        tree.treeNodes[p], taken);
            }
            tree_path = tree_path << 1 | taken; // endian does not matter
            if (!taken) {
                p = 2*(p + 1) - 1; // left
            } else {
                p = 2*(p + 1); // right
            }
        }
        float value = tree.leaves.at(tree_path).read();
        if (probing) {
            DPRINTFR(OGB, "prediction: %.2f, ", value);
        }
        y = static_cast<float>(
                (1.0 - eta * sigma[i].read() / sigma_deno) * y + eta * value);
        if (probing) {
            DPRINTFR(OGB, "cumulative prediction: %.2f\n", y);
        }
    }
    return y;
}

void OGBBP::OGBEntry::gradient_descent(BPHistory *bp_history, bool taken) {
    int32_t residual = taken ? zoomFactor : -zoomFactor;

    if (probing) {
        DPRINTFR(OGB, "bias from %d ", baseValue.read());
    }

    if (taken) {
        baseValue.increment();
    } else {
        baseValue.decrement();
    }

    if (probing) {
        DPRINTFR(OGB, "to %d \n", baseValue.read());
    }

    residual -= baseValue.read();

    float y = baseValue.read();

    for (uint32_t i = 0; i < nTrees; i++) {
        if (probing) {
            DPRINTFR(OGB, "T[%d] ", i);
        }
        dynamic_bitset<> &history = i < nLocal ?
                bp_history->localHistory : bp_history->globalHistory;
        Tree &tree = trees.at(i);

        uint32_t tree_path = 0;
        uint32_t p = 0;

        for (uint32_t d = 0; d < treeHeight; d++) {
            bool taken_1 = history[tree.treeNodes.at(p)];
            if (probing) {
                DPRINTFR(OGB, "%s[%d]: %d, ",
                        i<nLocal ? "local": "global",
                        tree.treeNodes[p], taken_1);
            }
            tree_path = tree_path << 1 | taken_1; // endian does not matter
            if (!taken_1) {
                p = 2*(p + 1) - 1; // left
            } else {
                p = 2*(p + 1); // right
            }
        }

        if (probing) {
            float value = tree.leaves.at(tree_path).read();
            DPRINTFR(OGB, "old prediction: %.2f, residual: %d, ",
                    value, residual);
        }

        if (abs(residual) > 1) {
            bool sensitive = i < nLocal ||
                    abs(tree.leaves[tree_path].read()) < 2;
            if (residual > 0) {
                if (sensitive) {
                    tree.leaves[tree_path].increment(
                            std::max(residual / 4, 2));
                } else {
                    tree.leaves[tree_path].increment();
                }
                sigma[i].increment();
            } else {
                if (sensitive) {
                    tree.leaves[tree_path].decrement(
                            std::max(-residual / 4, 2));
                } else {
                    tree.leaves[tree_path].decrement();
                }
                sigma[i].decrement();
            }
        }

        int32_t contribution = tree.leaves.at(tree_path).read();

        if (probing) {
            float value = tree.leaves.at(tree_path).read();
            DPRINTFR(OGB, "new prediction: %.2f, ", value);
            y = static_cast<float>(
                    (1.0 - eta * sigma[i].read()
                    / sigma_deno) * y + eta * value);
            DPRINTFR(OGB, "new cumulative: %.2f\n", y);

        }
//        if (abs(residual) <= 2) {
//            break;
//        }
        residual -= contribution;
    }

    if (probing) {
        DPRINTFR(OGB, "New final prediction: %f\n", y);
    }
}

