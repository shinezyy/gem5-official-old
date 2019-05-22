# Copyright (c) 2012 Mark D. Hill and David A. Wood
# Copyright (c) 2015 The University of Wisconsin
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Nilay Vaish and Dibakar Gope

from m5.SimObject import SimObject
from m5.params import *

class BranchPredictor(SimObject):
    type = 'BranchPredictor'
    cxx_class = 'BPredUnit'
    cxx_header = "cpu/pred/bpred_unit.hh"
    abstract = True

    numThreads = Param.Unsigned(1, "Number of threads")
    BTBEntries = Param.Unsigned(4096, "Number of BTB entries")
    BTBTagSize = Param.Unsigned(16, "Size of the BTB tags, in bits")
    RASSize = Param.Unsigned(16, "RAS size")
    instShiftAmt = Param.Unsigned(2, "Number of bits to shift instructions by")

    useIndirect = Param.Bool(True, "Use indirect branch predictor")
    indirectHashGHR = Param.Bool(True, "Hash branch predictor GHR")
    indirectHashTargets = Param.Bool(True, "Hash path history targets")
    indirectSets = Param.Unsigned(256, "Cache sets for indirect predictor")
    indirectWays = Param.Unsigned(2, "Ways for indirect predictor")
    indirectTagSize = Param.Unsigned(16, "Indirect target cache tag bits")
    indirectPathLength = Param.Unsigned(3,
        "Previous indirect targets to use for path history")



class LocalBP(BranchPredictor):
    type = 'LocalBP'
    cxx_class = 'LocalBP'
    cxx_header = "cpu/pred/2bit_local.hh"

    localPredictorSize = Param.Unsigned(2048, "Size of local predictor")
    localCtrBits = Param.Unsigned(2, "Bits per counter")

class PerceptronLocalBP(BranchPredictor):
    type = 'PerceptronLocalBP'
    cxx_class = 'PerceptronLocalBP'
    cxx_header = "cpu/pred/perceptron_local.hh"

    localPredictorSize = Param.Unsigned(2048, "Size of local predictor")
    localPercepSize = Param.Unsigned(64, "Bits per counter")

class Perceptron(BranchPredictor):
    type = 'Perceptron'
    cxx_class = 'Perceptron'
    cxx_header = "cpu/pred/perceptron.hh"

    globalPredictorSize = Param.Unsigned(64, "Size of global Perdictor")
    numberOfPerceptrons = Param.Unsigned(8192, "Number of Perceptrons")

class MyPerceptron(BranchPredictor):
    type = 'MyPerceptron'
    cxx_class = 'MyPerceptron'
    cxx_header = "cpu/pred/myperceptron.hh"

    globalPredictorSize = Param.Unsigned(256, "Size of global Perdictor")
    sizeOfPerceptrons   = Param.Unsigned(16, "Size of each Perceptron")
    pseudoTaggingBit    = Param.Unsigned(0, "Numeber of pseudo-tagging bits")
    indexMethod         = Param.String('MODULO', "Indexing method")
    bitsPerWeight       = Param.Unsigned(8, "Bits used to store each weight")
    lamda               = Param.Unsigned(1, "Learning rate")
    dynamicThresholdBit = Param.Unsigned(0, "Log of number of thresholds")
    thresholdCounterBit = Param.Unsigned(0, "Bits used to store TC")
    redundantBit        = Param.Unsigned(0,
                            "n-bits to represent a history bit")
    maxHisLen           = Param.Unsigned(128,
                            "max record length of global his")

class PathPerceptron(BranchPredictor):
    type = 'PathPerceptron'
    cxx_class = 'PathPerceptron'
    cxx_header = "cpu/pred/pathperceptron.hh"

    globalPredictorSize = Param.Unsigned(8192, "Size of global Perdictor")
    numberOfPerceptrons = Param.Unsigned(128, "Number of Perceptrons")

class TournamentBP(BranchPredictor):
    type = 'TournamentBP'
    cxx_class = 'TournamentBP'
    cxx_header = "cpu/pred/tournament.hh"

    localPredictorSize = Param.Unsigned(2048/2, "Size of local predictor")
    localCtrBits = Param.Unsigned(2, "Bits per counter")
    localHistoryTableSize = Param.Unsigned(2048/2,
                                     "size of local history table")
    globalPredictorSize = Param.Unsigned(8192/2, "Size of global predictor")
    globalCtrBits = Param.Unsigned(2, "Bits per counter")
    choicePredictorSize = Param.Unsigned(8192/2, "Size of choice predictor")
    choiceCtrBits = Param.Unsigned(2, "Bits of choice counters")


class BiModeBP(BranchPredictor):
    type = 'BiModeBP'
    cxx_class = 'BiModeBP'
    cxx_header = "cpu/pred/bi_mode.hh"

    globalPredictorSize = Param.Unsigned(8192, "Size of global predictor")
    globalCtrBits = Param.Unsigned(2, "Bits per counter")
    choicePredictorSize = Param.Unsigned(8192, "Size of choice predictor")
    choiceCtrBits = Param.Unsigned(2, "Bits of choice counters")

class LTAGE(BranchPredictor):
    type = 'LTAGE'
    cxx_class = 'LTAGE'
    cxx_header = "cpu/pred/ltage.hh"

    logSizeBiMP = Param.Unsigned(13, "Log size of Bimodal predictor in bits")
    logRatioBiModalHystEntries = Param.Unsigned(2,
        "Log num of prediction entries for a shared hysteresis bit " \
        "for the Bimodal")
    logSizeTagTables = Param.Unsigned(10, "Log size of tag table in LTAGE")
    logSizeLoopPred = Param.Unsigned(6, "Log size of the loop predictor")
    nHistoryTables = Param.Unsigned(5, "Number of history tables")
    tagTableCounterBits = Param.Unsigned(3, "Number of tag table counter bits")
    histBufferSize = Param.Unsigned(2097152,
            "A large number to track all branch histories(2MEntries default)")
    minHist = Param.Unsigned(4, "Minimum history size of LTAGE")
    maxHist = Param.Unsigned(640, "Maximum history size of LTAGE")
    minTagWidth = Param.Unsigned(8, "Minimum tag size in tag tables")

class OGBBP(BranchPredictor):
    type = 'OGBBP'
    cxx_class = 'OGBBP'
    cxx_header = "cpu/pred/online_gradient_boosting.hh"

    globalHistoryLenLog = Param.Unsigned(7, "global history length log")
    globalHistoryLen = Param.Unsigned(128, "global history length")
    treeHeight = Param.Unsigned(1, "base tree model height")
    localHistoryLen = Param.Unsigned(10, "local history length")
    tableSize = Param.Unsigned(512,
            "table size for local history and predictor")
    factorBits = Param.Unsigned(6, "bits for sigma")
    nTrees = Param.Unsigned(16, "number of weak tree learners")
    nLocal = Param.Unsigned(5, "number of weak tree learners")
    ctrBits = Param.Unsigned(2, "width of saturating counter")

class NBBP(BranchPredictor):
    type = 'NBBP'
    cxx_class = 'NBBP'
    cxx_header = "cpu/pred/naive_bayes.hh"

    globalHistoryLenLog = Param.Unsigned(8, "global history length log")
    globalHistoryLen = Param.Unsigned(256, "global history length")
    localHistoryLen = Param.Unsigned(10, "local history length")

    nLocalCounter = Param.Unsigned(10, "local history counter numbert")
    nGlobalCounter = Param.Unsigned(20, "global history counter numbert")

    tableSize = Param.Unsigned(512,
            "table size for local history and predictor")
    ctrBits = Param.Unsigned(5, "width of saturating counter")

class ZPerceptron(BranchPredictor):
    type = 'ZPerceptron'
    cxx_class = 'ZPerceptron'
    cxx_header = "cpu/pred/zperceptron.hh"

    tableSize = Param.Unsigned(256, "Size of global Perdictor")
    globalHistoryLen = Param.Unsigned(64, "global history length")
    localHistoryLen = Param.Unsigned(1, "local history length")
    ctrBits = Param.Unsigned(8, "width of saturating counter")

    pseudoTaggingBit    = Param.Unsigned(0, "Numeber of pseudo-tagging bits")
    lamda               = Param.Unsigned(1, "Learning rate")
    # thresholdCounterBit = Param.Unsigned(0, "Bits used to store TC")

class SNN(BranchPredictor):
    type = 'SNN'
    cxx_class = 'SNN'
    cxx_header = "cpu/pred/snn.hh"

    tableSize = Param.Unsigned(256, "Size of global Perdictor")
    localHistoryLen = Param.Unsigned(1, "local history length")
    ctrBits = Param.Unsigned(8, "width of saturating counter")

    denseGlobalHistoryLen = Param.Unsigned(16, "global history length")
    sparseGHSegLen = Param.Unsigned(8,
            "sparse global history length per segment")
    sparseGHNSegs = Param.Unsigned(8,
            "number of sparse global history segments")
    activeTerm = Param.Unsigned(16, "number of updates to observe")

    pseudoTaggingBit    = Param.Unsigned(0, "Numeber of pseudo-tagging bits")
    lamda               = Param.Unsigned(1, "Learning rate")
    # thresholdCounterBit = Param.Unsigned(0, "Bits used to store TC")


