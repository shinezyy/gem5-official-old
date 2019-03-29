def modifyO3CPUConfig(options, cpu):
    print('modifying O3 cpu config')
    if options.num_ROB:
        cpu.numROBEntries = options.num_ROB
    if options.num_IQ:
        cpu.numIQEntries = options.num_IQ
    if options.num_LQ:
        cpu.LQEntries = options.num_LQ
    if options.num_SQ:
        cpu.SQEntries = options.num_SQ
    if options.num_PhysReg:
        cpu.numPhysIntRegs = options.num_PhysReg
        cpu.numPhysFloatRegs = options.num_PhysReg
        cpu.numPhysVecRegs = options.num_PhysReg
        cpu.numPhysCCRegs = 0

    # TODO: check whether is perceptronBP
    #if cpu.branchPred == MyPerceptron:
    if options.bp_size:
        cpu.branchPred.globalPredictorSize = options.bp_size
    if options.bp_index_type:
        cpu.branchPred.indexMethod = options.bp_index_type
    if options.bp_history_len:
        cpu.branchPred.sizeOfPerceptrons = options.bp_history_len
    if options.bp_learning_rate:
        cpu.branchPred.lamda = options.bp_learning_rate
    if options.bp_pseudoTagging:
        cpu.branchPred.pseudoTaggingBit = options.bp_pseudo_tagging

    #if opt.bp_dyn_thres:
     #   cpu.branchPred.dynamicThresholdBit = options.bp_dyn_thres
      #  if opt.bp_tc_bit:
       #     cpu.branchPred.thresholdCounterBit = options.bp_tc_bit

