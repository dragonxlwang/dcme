#!/usr/bin/env bash

function check_cmd_arg  {
  argarr=($(echo $1))
  xarg=$2
  for x in "${argarr[@]}"; do
    if [[ $x == $xarg ]]; then
      echo 1
      return 0
    fi
  done
  echo 0
  return 1
}

train_debug_opt=$(check_cmd_arg "$*" DEBUG)
train_peek_opt=$(check_cmd_arg "$*" PEEK)
CFLAGS=""
[[ $train_debug_opt -eq 1  ]] && { CFLAGS="-DDEBUG "; }
[[ $train_peek_opt -eq 1  ]] && { CFLAGS+="-DDEBUGPEEK "; }

nomake=$(check_cmd_arg "$*" nomake)

if [[ $nomake -eq 0  || $1 == "make" ]]; then
  make vectors/train NOEXEC=1 CFLAGS="$CFLAGS"
  make eval/eval_peek NOEXEC=1
  make eval/eval_word_distance NOEXEC=1
  make eval/eval_question_accuracy NOEXEC=1 # CFLAGS="-DACCURACY"
  if [[ $1 == "make" ]]; then exit 0; fi
fi

train="./bin/vectors/train"
peek="./bin/eval/eval_peek"
wd="./bin/eval/eval_word_distance"
qa="./bin/eval/eval_question_accuracy"
if [[ $1 == "train" ]]; then bin=$train
elif [[ $1 == "peek" ]]; then bin=$peek
elif [[ $1 == "wd" ]]; then bin=$wd
elif [[ $1 == "qa" ]]; then bin=$qa
else bin=$train
fi

dcme=" \
  V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_no-cutoff \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me \
  # V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  # V_MODEL_PROJ_BALL_NORM 1e2 \
  # V_MICRO_ME 1"

# 28.69 %   Semantic accuracy: 22.35 %   Syntactic accuracy: 30.68 %
# PEEK:8.22e-03
# PEEK:9.22e-03
# 0.015602 %   Semantic accuracy: 0.015575 %   Syntactic accuracy: 0.015611 %
w2v_1=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_CACHE_INTERMEDIATE_MODEL 1 \
  V_ITER_NUM 100"


# 47.41 %   Semantic accuracy: 32.52 %   Syntactic accuracy: 52.08 %
# PEEK:1.72e-03
# PEEK:2.14e-03
# 0.015336 %   Semantic accuracy: 0.015305 %   Syntactic accuracy: 0.015346 %
w2v_2=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 0 \
  V_NS_WRH 1 "

# 29.92 %   Semantic accuracy: 24.66 %   Syntactic accuracy: 31.57 %
# PEEK:8.36e-03
# PEEK:9.15e-03
# 0.015564 %   Semantic accuracy: 0.015528 %   Syntactic accuracy: 0.015575 %
w2v_3=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  V_NCE 1 \
  V_NS_WRH 1 "

# 45.17 %   Semantic accuracy: 27.04 %   Syntactic accuracy: 50.86 %
# PEEK:7.51e-03
# PEEK:8.55e-03
# 0.011536 %   Semantic accuracy: 0.011491 %   Syntactic accuracy: 0.011550 %
w2v_4=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 1 \
  V_NS_WRH 1 "

# 27.42 %   Semantic accuracy: 19.83 %   Syntactic accuracy: 29.80 %
# PEEK:8.10e-03
# PEEK:9.47e-03
# 0.015729 %   Semantic accuracy: 0.015642 %   Syntactic accuracy: 0.015756 %
w2v_5=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_ns_wrh_pow-1 \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_NS_POWER 1"

# 52.27 %   Semantic accuracy: 33.60 %   Syntactic accuracy: 58.13 %
# PEEK:7.87e-04
# PEEK:1.07e-03
# 0.017462 %   Semantic accuracy: 0.017230 %   Syntactic accuracy: 0.017535 %
w2v_6=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_ns_wrh_pow-1 \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_NS_POWER 1"

# 28.55 %   Semantic accuracy: 22.28 %   Syntactic accuracy: 30.53 %
# PEEK:8.86e-03
# PEEK:9.71e-03
# 0.015792 %   Semantic accuracy: 0.015731 %   Syntactic accuracy: 0.015811 %
w2v_7=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_nce_wrh_pow-1 \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_NS_POWER 1"

# 40.43 %   Semantic accuracy: 24.80 %   Syntactic accuracy: 45.34%
# PEEK:8.04e-03
# PEEK:5.00e-06
# 0.010087 %   Semantic accuracy: 0.010140 %   Syntactic accuracy: 0.010070 %
w2v_8=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_nce_wrh_pow-1 \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_NS_POWER 1"

# no cutoff
# PEEK:1.74e-02
# PEEK:3.68e-02
# 28.98 %   Semantic accuracy: 17.00 %   Syntactic accuracy: 32.84 %
# 0.017875 %   Semantic accuracy: 0.017514 %   Syntactic accuracy: 0.017991 %
w2v_9=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_ns_wrh_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 28.71 %   Semantic accuracy: 15.89 %   Syntactic accuracy: 32.84 %
# PEEK:1.74e-02
# PEEK:3.68e-02
# 0.017882 %   Semantic accuracy: 0.017525 %   Syntactic accuracy: 0.017997 %
w2v_10=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_nce_wrh_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 26.60 %   Semantic accuracy: 15.54 %   Syntactic accuracy: 30.16 %
# PEEK:1.63e-02
# PEEK:2.66e-02
# 0.017696 %   Semantic accuracy: 0.017417 %   Syntactic accuracy: 0.017785 %
w2v_11=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_ns_wrh_pow-1_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_NS_POWER 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 26.91 %   Semantic accuracy: 16.24 %   Syntactic accuracy: 30.34 %
# PEEK:1.62e-02
# PEEK:2.70e-02
# 0.017723 %   Semantic accuracy: 0.017457 %   Syntactic accuracy: 0.017809 %
w2v_12=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_nce_wrh_pow-1_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_NS_POWER 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 9.66 %   Semantic accuracy: 11.38 %   Syntactic accuracy: 9.10 %
# PEEK:1.09e-02
# PEEK:6.16e-02 (newly tested without cut off)
# 0.016166 %   Semantic accuracy: 0.015974 %   Syntactic accuracy: 0.016228 %
dcme_2=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 13.34 %   Semantic accuracy: 10.96 %   Syntactic accuracy: 14.10 %
# PEEK:1.64e-02
# PEEK:3.59e-02
# 0.016231 %   Semantic accuracy: 0.015777 %   Syntactic accuracy: 0.016377 %
dcme_4=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ub \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_CACHE_INTERMEDIATE_MODEL 1 \
  V_ITER_NUM 100"

# 5.59 %   Semantic accuracy: 6.87 %   Syntactic accuracy: 5.18 %
# PEEK:6.02e-03
# PEEK:2.00e-02
# 0.017287 %   Semantic accuracy: 0.016449 %   Syntactic accuracy: 0.017556 %
dcme_6=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ub_mm \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1"

# 5.38 %   Semantic accuracy: 6.38 %   Syntactic accuracy: 5.06 %
# PEEK:6.72e-03
# PEEK:2.36e-02
# 0.016973 %   Semantic accuracy: 0.016447 %   Syntactic accuracy: 0.017142 %
dcme_8=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ub_mm_mmsu \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1 \
  V_MICRO_ME_SCR_UPDATE 1"

# 12.00 %   Semantic accuracy: 11.94 %   Syntactic accuracy: 12.02 %
# PEEK:9.56e-03
# PEEK:3.68e-02
# 0.017240 %   Semantic accuracy: 0.017047 %   Syntactic accuracy: 0.017302 %
dcme_9=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 11.73 %   Semantic accuracy: 13.12 %   Syntactic accuracy: 11.29 %
# PEEK:8.57e-03
# PEEK:4.12e-02
# 0.017353 %   Semantic accuracy: 0.017214 %   Syntactic accuracy: 0.017398 %
dcme_10=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_ub \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2"

# 11.93 %   Semantic accuracy: 12.63 %   Syntactic accuracy: 11.71 %
# PEEK:1.05e-02
# PEEK:2.94e-02
# 0.016621 %   Semantic accuracy: 0.016560 %   Syntactic accuracy: 0.016640 %
dcme_11=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_ub_mm \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1"

# 13.86 %   Semantic accuracy: 11.52 %   Syntactic accuracy: 14.61 %
# PEEK:1.30e-02
# PEEK:3.02e-02
# 0.016314 %   Semantic accuracy: 0.015877 %   Syntactic accuracy: 0.016454 %
dcme_12=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_ub_mm_mmsu \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1 \
  V_MICRO_ME_SCR_UPDATE 1"

# 7.950709 %   Semantic accuracy: 9.021513 %   Syntactic accuracy: 7.606513 %
# PEEK:2.32e-02
# 0.015662 %   Semantic accuracy: 0.016250 %   Syntactic accuracy: 0.015474 %
dcme_13=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_ovr_0.1 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_OFFLINE_INTERVAL_VOCAB_RATIO 0.1"

# 9.199865 %   Semantic accuracy: 10.131853 %   Syntactic accuracy: 8.900290 %
# PEEK:4.24e-02
# 0.017426 %   Semantic accuracy: 0.017061 %   Syntactic accuracy: 0.017543 %
dcme_14=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_ovr-0.1_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_OFFLINE_INTERVAL_VOCAB_RATIO 0.1 \
  Q 0"

################################################################

dcme_15=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-4_nc \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

dcme_16=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-4_nc_ovr-0.1 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_OFFLINE_INTERVAL_VOCAB_RATIO 0.1"

dcme_17=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-4_nc_ovr-0.1_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_OFFLINE_INTERVAL_VOCAB_RATIO 0.1 \
  Q 0"

dcme_18=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ovr-0.1 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_OFFLINE_INTERVAL_VOCAB_RATIO 0.1"

dcme_19=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ovr-0.1_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_OFFLINE_INTERVAL_VOCAB_RATIO 0.1 \
  Q 0"

dcme_20=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_hi_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0"

dcme_21=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# this is the baseline!
# PEEK:6.43e-02 Took time 00:01:04:27
# 0.016017 %   Semantic accuracy: 0.016294 %   Syntactic accuracy: 0.015928 %
# SCR:3.22e+02=2.00e+00*1.61e+02 TAR=8.77e+02=1.97e+00*4.44e+02
dcme_22=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-5_nc_hi_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0"

# varing gradient step 2e-5
# PEEK:3.93e-02 TIME:7.70e+04/00:01:04:11
# 0.017079 %   Semantic accuracy: 0.017125 %   Syntactic accuracy: 0.017064 %
# SCR:1.38e+07=2.53e+00*5.45e+06 TAR=1.81e+08=3.54e+00*5.11e+07
dcme_23=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_hi_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0"

# varing gradient step 5e-5
# PEEK:3.54e-02 TIME:8.24e+04/00:01:08:41
# 0.016432 %   Semantic accuracy: 0.016566 %   Syntactic accuracy: 0.016388 %
# SCR:2.11e+112=4.67e+00*4.52e+111 TAR=3.69e+113=3.29e+00*1.12e+113
dcme_24=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-5e-5_nc_hi_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 5e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0"

# varing gradient step 1e-4
# NAN
dcme_25=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_hi_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0"

# repeat 22-25 but with adjusted ww
# TIME:7.88e+04/00:01:05:41 GDSS:2.1983e-08
# SCR:1.19e+02=1.13e+01*1.05e+01
# TAR=3.65e+02=5.03e+00*7.27e+01
# ENT:5.39e+00±4.89e+00 PEEK:1.04e-01
# PEEK:5.30e-02
# 0.014715 %   Semantic accuracy: 0.015115 %   Syntactic accuracy: 0.014587 %
dcme_26=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-5_nc_hi_Q-0_ww \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 10"

# TIME:8.52e+04/00:01:10:58 GDSS:5.2764e-08
# SCR:1.79e+02=3.26e+00*5.48e+01
# TAR=5.85e+02=3.15e+00*1.86e+02
# ENT:2.55e+00±3.74e+00 PEEK:9.83e-02
# PEEK:5.13e-02
# 0.017528 %   Semantic accuracy: 0.016882 %   Syntactic accuracy: 0.017735 %
dcme_27=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_hi_Q-0_ww \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 10"

# TIME:7.94e+04/00:01:06:10 GDSS:6.6070e-08
# SCR:6.75e+02=5.14e+00*1.31e+02
# TAR=1.77e+03=2.52e+00*7.03e+02
# ENT:5.92e-01±2.50e+00 PEEK:8.61e-02
# PEEK:4.61e-02
# 0.017963 %   Semantic accuracy: 0.017470 %   Syntactic accuracy: 0.018121 %
dcme_28=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-5e-5_nc_hi_Q-0-ww \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 5e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 10"

# TIME:8.49e+04/00:01:10:43 GDSS:1.8035e-07
# SCR:7.19e+12=2.80e+00*2.57e+12
# TAR=3.86e+13=3.24e+00*1.19e+13
# ENT:0.00e+00±0.00e+00 PEEK:4.68e-02
# PEEK:3.42e-02
# 0.016666 %   Semantic accuracy: 0.015606 %   Syntactic accuracy: 0.017006 %
dcme_29=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_hi_Q-0-ww \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 10"

# jul 9
dcme_30=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-5_nc_hi_Q-0_ww-5 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 5"

dcme_31=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_hi_Q-0_ww-5 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 5"

dcme_32=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-5e-5_nc_hi_Q-0_ww-5 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 5e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 5"

dcme_33=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_hi_Q-0_ww-5 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 5"

dcme_34=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-5_nc_hi_Q-0_ww-2 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 2"

dcme_35=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_hi_Q-0_ww-2 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 2"

dcme_36=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-5e-5_nc_hi_Q-0_ww-2 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 5e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 2"

dcme_37=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_hi_Q-0_ww-2 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_DUAL_HI 1 \
  Q 0 \
  V_ADJUST_WW 2"

dcme_38=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_CACHE_INTERMEDIATE_MODEL 1 \
  V_ITER_NUM 100"

w2v_13=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-5e-2_ns_wrh_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 5e-2 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

w2v_14=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-5e-2_nce_wrh_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 5e-2 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

w2v_15=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_ns_wrh_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_CACHE_INTERMEDIATE_MODEL 1 \
  V_ITER_NUM 100"

w2v_16=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_nce_wrh_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_CACHE_INTERMEDIATE_MODEL 1 \
  V_ITER_NUM 100"


dcme_40=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ub_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  Q 0 \
  V_CACHE_INTERMEDIATE_MODEL 1 \
  V_ITER_NUM 100"

dcme_41=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ub_Q-10 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  Q 10 \
  V_CACHE_INTERMEDIATE_MODEL 1 \
  V_ITER_NUM 100"

dcme_42=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ub_mm \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1 \
  V_CACHE_INTERMEDIATE_MODEL 1 \
  V_ITER_NUM 100"

dcme_43=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_CACHE_INTERMEDIATE_MODEL 1 \
  V_ITER_NUM 10"

dcme_44=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_ub_1e2 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_CACHE_INTERMEDIATE_MODEL 1 \
  V_ITER_NUM 10"

dcme_45=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-2e-5_nc_ub_1 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 2e-5 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1 \
  V_CACHE_INTERMEDIATE_MODEL 5 \
  V_ITER_NUM 10"

eval "model=\$$2"
$bin $model


################################################################

# 13.53 %   Semantic accuracy: 9.88 %   Syntactic accuracy: 14.67 %
# PEEK:2.01e-02
# PEEK:2.27e-02 (newly tested without cut off)
# 0.016752 %   Semantic accuracy: 0.016283 %   Syntactic accuracy: 0.016899 %
dcme_1=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4"

# 9.66 %   Semantic accuracy: 11.38 %   Syntactic accuracy: 9.10 %
# PEEK:2.11e-02 *
# PEEK:2.20e-02
# 0.017776 %   Semantic accuracy: 0.018489 %   Syntactic accuracy: 0.017552 %
dcme_3=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_ub \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_MODEL_PROJ_BALL_NORM 1e2"

# 15.77 %   Semantic accuracy: 11.18 %   Syntactic accuracy: 17.21 %
# PEEK:2.04e-02 *
# PEEK:2.22e-02
# 0.017642 %   Semantic accuracy: 0.018675 %   Syntactic accuracy: 0.017317 %
dcme_5=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_ub_mm \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1"

# 17.85 %   Semantic accuracy: 10.09 %   Syntactic accuracy: 20.29 %
# PEEK:1.95e-02
# PEEK:2.15e-02
# 0.017402 %   Semantic accuracy: 0.017550 %   Syntactic accuracy: 0.017355 %
dcme_7=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_ub_mm_mmsu \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1 \
  V_MICRO_ME_SCR_UPDATE 1"

################################################################

# V_MODEL_DECOR_FILE_PATH gd-1e-4 \
  # V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4

# # P: 8.09%  Q: 13.44%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4 \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4

# # P: 3.77%  Q: 13.89%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0

# # P: 8.13%  Q: 17.73%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1

# # P:6.18%  Q: 18.66%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me-scr \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_MICRO_ME_SCR_UPDATE 1
