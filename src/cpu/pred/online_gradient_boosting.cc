#include "online_gradient_boosting.hh"

//#include <iostream>
#include <fstream>

#include "base/bitfield.hh"
#include "base/intmath.hh"

using namespace boost;

OGBBP *OGBBPParams::create()
{
    return new OGBBP(this);
}

bool OGBBP::lookup(ThreadID tid, Addr branch_addr, void *&bp_bistory) {

    uint32_t index = computeIndex(branch_addr);
    dynamic_bitset<> &ghr = globalHistory[tid];
    OGBEntry &ogbe = table.at(index);

    bool result = ogbe.predict(ghr) > 0;
    bp_bistory = new BPHistory(ghr, ogbe.localHistory, index, result);
    return result;
}

void OGBBP::uncondBranch(ThreadID tid, Addr pc, void *&bp_history) {
    bp_history = new BPHistory(globalHistory[tid],
            emptyLocalHistory, InvalidTableIndex, true);
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
    assert(ogbe.valid);

    if (squashed) {
        globalHistory[tid] = history->globalHistory << 1;
        globalHistory[tid][0] = taken;
        ogbe.localHistory = history->localHistory << 1;
        ogbe.localHistory[0] = taken;
        return;
    }

    ogbe.gradient_descent(history->globalHistory, taken);

    delete history;
}

void OGBBP::squash(ThreadID tid, void *bp_history) {
    auto history = static_cast<BPHistory *>(bp_history);
    globalHistory[tid] = history->globalHistory;

    if (history->tableIndex != InvalidTableIndex) {
        table[history->tableIndex].localHistory = history->localHistory;
    }

    delete history;
}

unsigned OGBBP::getGHR(ThreadID tid, void *bp_history) const {
    return 0;
}

OGBBP::OGBBP(const OGBBPParams *params)
        :
        BPredUnit(params),

        mt(rd()),

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
                OGBEntry(localHistoryLen, globalHistoryLen,
                        nTrees, factorBits, treeHeight, ctrBits))
         {
    for (auto &ogbe: table) {
        ogbe.init(mt);
    }
}

uint32_t OGBBP::computeIndex(Addr addr) {
    return static_cast<uint32_t>((addr >> 2) % tableSize);
}

void OGBBP::updateGHR(ThreadID tid, bool taken) {
    globalHistory[tid] <<= 1;
    globalHistory[tid][0] = taken;
}

void OGBBP::OGBEntry::init(std::mt19937 &mt) {
    valid = true;
    auto n_trees = static_cast<uint32_t>(sigma.size());
    assert(n_trees > 4);
    assert(n_trees == trees.size());
    for (uint32_t i = 0; i < nLocal; i++) {
        trees[i].reRand(mt, 0, localHistoryLen);
    }
    uint32_t r = globalHistoryLen;
    for (uint32_t i = nLocal; i < n_trees; i++) {
        trees[i].reRand(mt, 0, r);
        r /= 2;
    }
}

float OGBBP::OGBEntry::predict(boost::dynamic_bitset<> &ghr) {
    float pred = baseValue.read();
    for (uint32_t i = 0; i < nTrees; i++) {
        dynamic_bitset<> &history = i < nLocal ? localHistory : ghr;
        Tree &tree = trees.at(i);

        uint32_t tree_path = 0;
        uint32_t p = 0;

        for (uint32_t d = 0; d < treeHeight; d++) {
            bool taken = history[tree.treeNodes.at(p)];
            tree_path = tree_path << 1 | taken; // endian does not matter
            if (!taken) {
                p = 2*(p + 1) - 1; // left
            } else {
                p = 2*(p + 1); // right
            }
        }
        float value = tree.leaves.at(tree_path).read();
        pred = static_cast<float>(
                (value * 64 + (
                        64.0 * eta - sigma[i].read()) * pred) / (64 * eta));
    }
    return pred;
}

void OGBBP::OGBEntry::gradient_descent(
        boost::dynamic_bitset<> &ghr, bool taken) {
    int32_t residual = taken ? zoomFactor : -zoomFactor;

    if (taken) {
        baseValue.increment();
    } else {
        baseValue.decrement();
    }

    residual -= baseValue.read();

    for (uint32_t i = 0; i < nTrees; i++) {
        dynamic_bitset<> &history = i < nLocal ? localHistory : ghr;
        Tree &tree = trees.at(i);

        uint32_t tree_path = 0;
        uint32_t p = 0;

        for (uint32_t d = 0; d < treeHeight; d++) {
            bool taken_1 = history[tree.treeNodes.at(p)];
            tree_path = tree_path << 1 | taken_1; // endian does not matter
            if (!taken_1) {
                p = 2*(p + 1) - 1; // left
            } else {
                p = 2*(p + 1); // right
            }
        }

        int32_t contribution = tree.leaves.at(tree_path).read();

        if (contribution * residual < 0) {
            tree.leaves[tree_path].decrement();
            sigma[i].decrement();
        } else {
            tree.leaves[tree_path].increment();
            sigma[i].increment();
        }

        residual -= contribution;
    }
}

void OGBBP::Tree::reRand(std::mt19937 &mt, uint32_t l, uint32_t r) {
    auto dist = std::uniform_int_distribution<uint32_t>(l, r);
    for (auto &node : treeNodes) {
        node = dist(mt);
    }
}
