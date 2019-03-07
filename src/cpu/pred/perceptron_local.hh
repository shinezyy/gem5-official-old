#ifndef __PERCEPTRON_PRED_HH__
#define __PERCEPTRON_PRED_HH__

#include <vector>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "cpu/pred/perceptronlocal.hh"
#include "debug/PErceptron.hh"
#include "params/PerceptronLocalBP.hh"

/**
 * Implements a local predictor that uses the PC to index into a table of
 * counters.  Note that any time a pointer to the bp_history is given, it
 * should be NULL using this predictor because it does not have any branch
 * predictor state that needs to be recorded or updated; the update can be
 * determined solely by the branch being taken or not taken.
 */
class PerceptronLocalBP : public BPredUnit
{
  public:
    /**
     * Default branch predictor constructor.
     */
    PerceptronLocalBP(const PerceptronLocalBPParams *params);

    virtual void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);

    /**
     * Looks up the given address in the branch predictor and returns
     * a true/false value as to whether it is taken.
     * @param branch_addr The address of the branch to look up.
     * @param bp_history Pointer to any bp history state.
     * @return Whether or not the branch is taken.
     */
    bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);

    /**
     * Updates the branch predictor to Not Taken if a BTB entry is
     * invalid or not found.
     * @param branch_addr The address of the branch to look up.
     * @param bp_history Pointer to any bp history state.
     * @return Whether or not the branch is taken.
     */
    void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);

    /**
     * Updates the branch predictor with the actual result of a branch.
     * @param branch_addr The address of the branch to update.
     * @param taken Whether or not the branch was taken.
     */
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed);

    void squash(ThreadID tid, void *bp_history)
    { assert(bp_history == NULL); }

    void reset();

  private:
    /**
     *  Returns the taken/not taken prediction given the value of the
     *  counter.
     *  @param count The value of the counter.
     *  @return The prediction based on the counter value.
     */
    bool getPrediction(int count);

        /** Calculates the local index based on the PC. */
    size_t getLocalIndex(Addr &branch_addr);

    std::vector<Perceptron> localPerceps;
    /** Array of counters that make up the local predictor. */

    /** Size of the local perceptron. */
    size_t localPerceptronSize;

        /** Number of sets. */
    size_t localPredictorSets;

    /** Updates global history as taken. */
    void updateGlobalHistTaken(ThreadID tid);

    /** Updates global history as not taken. */
    void updateGlobalHistNotTaken(ThreadID tid);

    /**32 bit rolling history of previous branches*/
    size_t globalHistory;

        /** Mask to get index bits. */
        size_t indexMask;

};


#endif // __PERCEPTRON_PRED_HH__
