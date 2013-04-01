#!/bin/bash

echo "Start Time: "
date


M2S_EXE=/home/ray/WORK/multi2sim/multi2sim-4.0.1/src/m2s

X86_CONFIG_8CORE="--x86-config x86-config-8core --x86-sim detailed"
X86_CONFIG_1CORE="--x86-config x86-config-1core --x86-sim detailed"

CKP_NAME="./mcf.ckp" ##!!CHANGE ME!!

CHECKPOINT_LOAD_8CORE="--x86-multiload-checkpoint 8 $CKP_NAME $CKP_NAME $CKP_NAME $CKP_NAME $CKP_NAME $CKP_NAME $CKP_NAME $CKP_NAME"
MEM_CONFIG_8CORE="--mem-config mem-config-8core"
MAX_INST_8CORE="--x86-max-inst 2400000000"

CHECKPOINT_LOAD_1CORE="--x86-load-checkpoint $CKP_NAME"
MEM_CONFIG_1CORE="--mem-config mem-config-1core"
MAX_INST_1CORE="--x86-max-inst 300000000"

REPORT_CONFIG="--mem-report ./report/mem.report --x86-report ./report/x86.report --net-report ./report/net.report"


#$M2S_EXE $X86_CONFIG_8CORE $CHECKPOINT_LOAD_8CORE $REPORT_CONFIG $MEM_CONFIG_8CORE $MAX_INST_8CORE

$M2S_EXE $X86_CONFIG_1CORE $CHECKPOINT_LOAD_1CORE $REPORT_CONFIG $MEM_CONFIG_1CORE $MAX_INST_1CORE

mv *.log ./report

echo "Finish Time: "
date
