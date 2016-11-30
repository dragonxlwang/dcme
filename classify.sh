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
  make classify/train NOEXEC=1 CFLAGS="$CFLAGS"
  make eval/eval_classify NOEXEC=1 CFLAGS="$CFLAGS"
  make eval/eval_fit_test NOEXEC=1 CFLAGS="$CFLAGS"
  # make eval/eval_word_distance NOEXEC=1
  # make eval/eval_question_accuracy NOEXEC=1 # CFLAGS="-DACCURACY"
  if [[ $1 == "make" ]]; then exit 0; fi
fi

train="./bin/classify/train"
classify="./bin/eval/eval_classify"
fit="./bin/eval/eval_fit_test"


# accuracy    = 0.211029
# probability = 0.165033
## 2nd run
# correct     = 3477
# total       = 16230
# accuracy    = 0.214233
# probability = 0.158673
# fitting
# accuracy    = 0.239952
# probability = 0.179323
dcme_1=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10"

# accuracy    = 0.176587
# probability = 0.176389
## 2nd run
# correct     = 3166
# total       = 16230
# accuracy    = 0.195071
# probability = 0.194763
# fitting
# accuracy    = 0.195878
# probability = 0.195747
dcme_2=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-20_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 20 \
  Q 10"

# accuracy    = 0.170918
# probability = 0.155897
## 2nd run
# correct     = 2857
# total       = 16230
# accuracy    = 0.176032
# probability = 0.158707
# fitting
# accuracy    = 0.189210
# probability = 0.173319
dcme_3=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-10_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 10 \
  Q 10"

# accuracy    = 0.191559
# probability = 0.191464
## 2nd run
# correct     = 2500
# total       = 16230
# accuracy    = 0.154036
# probability = 0.153990
# fitting
# accuracy    = 0.161767
# probability = 0.161607
dcme_4=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-10_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 10 \
  Q 10"

# accuracy    = 0.203266
# probability = 0.197433
## 2nd run
# correct     = 3298
# total       = 16230
# accuracy    = 0.203204
# probability = 0.197520
# fitting
# accuracy    = 0.215621
# probability = 0.209574
dcme_5=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-10_Q-0 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 10 \
  Q 0"

# accuracy    = 0.191559
# probability = 0.191488
## 2nd run
# correct     = 3015
# total       = 16230
# accuracy    = 0.185767
# probability = 0.185688
# fitting
# accuracy    = 0.193491
# probability = 0.193372
dcme_6=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-10_Q-0 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 10 \
  Q 0"

##################################

# correct     = 2751
# total       = 16230
# accuracy    = 0.169501
# probability = 0.169396
# fitting
# accuracy    = 0.176285
# probability = 0.176147
dcme_7=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-1_N-30K_K-5_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  K 5 \
  Q 0"

# correct     = 2784
# total       = 16230
# accuracy    = 0.171534
# probability = 0.171383
# fitting
# accuracy    = 0.178432
# probability = 0.178283
dcme_8=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-5_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 5 \
  Q 0"

# correct     = 2814
# total       = 16230
# accuracy    = 0.173383
# probability = 0.173238
# fitting
# accuracy    = 0.182385
# probability = 0.182168
dcme_9=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-5_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 5 \
  Q 0"

# correct     = 2863
# total       = 16230
# accuracy    = 0.176402
# probability = 0.174607
# fitting
# accuracy    = 0.183274
# probability = 0.180462
dcme_10=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-5_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 5 \
  Q 0"

# correct     = 3424
# total       = 16230
# accuracy    = 0.210967
# probability = 0.210835 ****
# fitting
# accuracy    = 0.220695
# probability = 0.220569
dcme_11=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-1_N-30K_K-20_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  K 20 \
  Q 0"

# correct     = 3243
# total       = 16230
# accuracy    = 0.199815
# probability = 0.199599
# fitting
# accuracy    = 0.212037
# probability = 0.211924
dcme_12=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-20_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 20 \
  Q 0"

# correct     = 3127
# total       = 16230
# accuracy    = 0.192668
# probability = 0.192267
# fitting
# accuracy    = 0.209671
# probability = 0.209489
dcme_13=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-20_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 20 \
  Q 0"

# correct     = 3594
# total       = 16230
# accuracy    = 0.221442
# probability = 0.209027
# fitting
# accuracy    = 0.241389
# probability = 0.227043
dcme_14=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 0"

# correct     = 2818
# total       = 16230
# accuracy    = 0.173629
# probability = 0.173512
# fitting
# accuracy    = 0.185421
# probability = 0.185282
dcme_15=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-1_N-30K_K-20_Q-10 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  K 20 \
  Q 10"

# correct     = 2932
# total       = 16230
# accuracy    = 0.180653
# probability = 0.180513
# fitting
# accuracy    = 0.195878
# probability = 0.195747
dcme_16=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-20_Q-10 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 20 \
  Q 10"

# correct     = 2951
# total       = 16230
# accuracy    = 0.181824
# probability = 0.181687
# fitting
# accuracy    = 0.206430
# probability = 0.205761
dcme_17=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-20_Q-10 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 20 \
  Q 10"

# correct     = 3386
# total       = 16230
# accuracy    = 0.208626
# probability = 0.155615
# fitting
# accuracy    = 0.239952
# probability = 0.179323
dcme_18=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10"

# correct     = 3260
# total       = 16230
# accuracy    = 0.200863
# probability = 0.200714
# fitting
# accuracy    = 0.214520
# probability = 0.214370
dcme_19=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-1_N-30K_K-20_Q-10_mme \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  K 20 \
  Q 10 \
  V_MICRO_ME 1"

# correct     = 3262
# total       = 16230
# accuracy    = 0.200986
# probability = 0.200937
# fitting
# accuracy    = 0.217255
# probability = 0.217112
dcme_20=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-20_Q-10_mme \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 20 \
  Q 10 \
  V_MICRO_ME 1"

# correct     = 3333
# total       = 16230
# accuracy    = 0.205360
# probability = 0.204888
# fitting
# accuracy    = 0.220599
# probability = 0.220402
dcme_21=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-20_Q-10_mme \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 20 \
  Q 10 \
  V_MICRO_ME 1"

# correct     = 3673
# total       = 16230
# accuracy    = 0.226309
# probability = 0.209654
# fitting
# accuracy    = 0.245553
# probability = 0.228005
dcme_22=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10_mme \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10 \
  V_MICRO_ME 1"

dcme_test=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_test \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10 \
  V_MICRO_ME 1"

# correct     = 3645
# total       = 16230
# accuracy    = 0.224584
# probability = 0.175574
# fitting
# accuracy    = 0.411006
# probability = 0.306718
w2v_1=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-10_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 10 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 2526
# total       = 16230
# accuracy    = 0.155638
# probability = 0.124181
# fitting
# accuracy    = 0.426673
# probability = 0.324966
w2v_2=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-20_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 0 \
  V_NS_WRH 1 "

##################################

# correct     = 3222
# total       = 16230
# accuracy    = 0.198521
# probability = 0.193660
# fitting
# accuracy    = 0.410076
# probability = 0.400650
w2v_3=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-5_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 4128
# total       = 16230
# accuracy    = 0.254344
# probability = 0.200751
# fitting
# accuracy    = 0.423787
# probability = 0.312175
w2v_4=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-5_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 4176
# total       = 16230
# accuracy    = 0.257301
# probability = 0.144779
# fitting
# accuracy    = 0.319102
# probability = 0.170844
w2v_5=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-3_N-30K_NEG-5_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 3412
# total       = 16230
# accuracy    = 0.210228
# probability = 0.055599
# fitting
# accuracy    = 0.227007
# probability = 0.059080
w2v_6=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-4_N-30K_NEG-5_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 3788
# total       = 16230
# accuracy    = 0.233395
# probability = 0.229537
# fitting
# accuracy    = 0.421230
# probability = 0.415184
w2v_7=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-5_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 1 \
  V_NS_WRH 1 "

# correct     = 3455
# total       = 16230
# accuracy    = 0.212877
# probability = 0.185776
# fitting
# accuracy    = 0.294524
# probability = 0.260857
w2v_8=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-5_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 1 \
  V_NS_WRH 1 "

# correct     = 3680
# total       = 16230
# accuracy    = 0.226741
# probability = 0.160707
# fitting
# accuracy    = 0.264763
# probability = 0.189046
w2v_9=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-3_N-30K_NEG-5_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 1 \
  V_NS_WRH 1 "

# correct     = 3261
# total       = 16230
# accuracy    = 0.200924
# probability = 0.091971
# fitting
# accuracy    = 0.213904
# probability = 0.098154
w2v_10=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-4_N-30K_NEG-5_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 1 \
  V_NS_WRH 1 "

###############################################################################

# correct     = 3630
# total       = 16230
# accuracy    = 0.223660
# probability = 0.219594
# fitting
# accuracy    = 0.475986
# probability = 0.465888
w2v_11=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-10_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 10 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 3554
# total       = 16230
# accuracy    = 0.218977
# probability = 0.172061
# fitting
# accuracy    = 0.411006
# probability = 0.306718
w2v_12=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-10_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 10 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 4390
# total       = 16230
# accuracy    = 0.270487
# probability = 0.156876
# fitting
# accuracy    = 0.352044
# probability = 0.195026
w2v_13=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-3_N-30K_NEG-10_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  V_NS_NEG 10 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 3671
# total       = 16230
# accuracy    = 0.226186
# probability = 0.072026
# fitting
# accuracy    = 0.248337
# probability = 0.079032
w2v_14=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-4_N-30K_NEG-10_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  V_NS_NEG 10 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 3664
# total       = 16230
# accuracy    = 0.225755
# probability = 0.221270
# fitting
# accuracy    = 0.446368
# probability = 0.439641
w2v_15=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-10_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 10 \
  V_NCE 1 \
  V_NS_WRH 1 "

# correct     = 3122
# total       = 16230
# accuracy    = 0.192360
# probability = 0.171779
# fitting
# accuracy    = 0.286475
# probability = 0.252982
w2v_16=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-10_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 10 \
  V_NCE 1 \
  V_NS_WRH 1 "

# correct     = 4135
# total       = 16230
# accuracy    = 0.254775
# probability = 0.185350
# fitting
# accuracy    = 0.298826
# probability = 0.221684
w2v_17=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-3_N-30K_NEG-10_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  V_NS_NEG 10 \
  V_NCE 1 \
  V_NS_WRH 1 "

# correct     = 3464
# total       = 16230
# accuracy    = 0.213432
# probability = 0.108237
# fitting
# accuracy    = 0.227397
# probability = 0.117558
w2v_18=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-4_N-30K_NEG-10_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  V_NS_NEG 10 \
  V_NCE 1 \
  V_NS_WRH 1 "

# correct     = 3161
# total       = 16230
# accuracy    = 0.194763
# probability = 0.191483
# fitting
# accuracy    = 0.472888
# probability = 0.463870
w2v_19=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-20_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 3462
# total       = 16230
# accuracy    = 0.213309
# probability = 0.169641
# fitting
# accuracy    = 0.426673
# probability = 0.324966
w2v_20=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-20_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 3888
# total       = 16230
# accuracy    = 0.239556
# probability = 0.134584
# fitting
# accuracy    = 0.351661
# probability = 0.189985
w2v_21=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-3_N-30K_NEG-20_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 3801
# total       = 16230
# accuracy    = 0.234196
# probability = 0.079348
# fitting
# accuracy    = 0.264688
# probability = 0.088137
w2v_22=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-4_N-30K_NEG-20_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 0 \
  V_NS_WRH 1 "

# correct     = 3358
# total       = 16230
# accuracy    = 0.206901
# probability = 0.204257
# fitting
# accuracy    = 0.464374
# probability = 0.457812
w2v_23=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-20_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 1 \
  V_NS_WRH 1 "

# correct     = 3801
# total       = 16230
# accuracy    = 0.234196
# probability = 0.205024
# fitting
# accuracy    = 0.378482
# probability = 0.329503
w2v_24=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-20_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 1 \
  V_NS_WRH 1 "

# correct     = 3876
# total       = 16230
# accuracy    = 0.238817
# probability = 0.177456
# fitting
# accuracy    = 0.278290
# probability = 0.210715
w2v_25=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-3_N-30K_NEG-20_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 1 \
  V_NS_WRH 1 "

# correct     = 3255
# total       = 16230
# accuracy    = 0.200555
# probability = 0.109845
# fitting
# accuracy    = 0.215190
# probability = 0.118963
w2v_26=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-4_N-30K_NEG-20_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 1 \
  V_NS_WRH 1 "

###############################
# test intermediate models

dcme_1i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000"

dcme_2i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10_mme \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10 \
  V_MICRO_ME 1 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000"

dcme_3i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-1_N-30K_K-20_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  K 20 \
  Q 0 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000"

dcme_4i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10_sd \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10 \
  V_THREAD_DUAL 0 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000"

dcme_5i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10_sd_4 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10 \
  V_THREADS_PER_DUAL 4 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000 \
  V_THREAD_NUM 20"

dcme_6i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10_sd_10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10 \
  V_THREADS_PER_DUAL 10 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000 \
  V_THREAD_NUM 20"

dcme_7i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-20_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 20 \
  Q 10 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000"

dcme_8i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-20_Q-10_sd_2 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 20 \
  Q 10 \
  V_THREADS_PER_DUAL 2 \
  V_CACHE_INTERMEDIATE_WEIGHT 10 \
  V_ITER_NUM 1000 \
  V_THREAD_NUM 20"

dcme_8i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-20_Q-10_sd_2 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 20 \
  Q 10 \
  V_THREADS_PER_DUAL 2 \
  V_CACHE_INTERMEDIATE_WEIGHT 10 \
  V_ITER_NUM 1000 \
  V_THREAD_NUM 20"

dcme_9i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-80_Q-10_sd_4 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 80 \
  Q 10 \
  V_THREADS_PER_DUAL 4 \
  V_CACHE_INTERMEDIATE_WEIGHT 10 \
  V_ITER_NUM 1000 \
  V_THREAD_NUM 20"

dcme_10i=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-80_Q-10_sd_4 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 80 \
  Q 10 \
  V_THREADS_PER_DUAL 4 \
  V_CACHE_INTERMEDIATE_WEIGHT 10 \
  V_ITER_NUM 1000 \
  V_THREAD_NUM 20"

w2v_1i=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-20_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_CACHE_INTERMEDIATE_WEIGHT 10 \
  V_ITER_NUM 1000"

w2v_2i=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-20_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_CACHE_INTERMEDIATE_WEIGHT 10 \
  V_ITER_NUM 1000"

w2v_3i=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-20_ns_wrh_pow-1 \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_NS_POWER 1 \
  V_CACHE_INTERMEDIATE_WEIGHT 10 \
  V_ITER_NUM 1000"

w2v_4i=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-20_nce_wrh_pow-1 \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_NS_POWER 1 \
  V_CACHE_INTERMEDIATE_WEIGHT 10 \
  V_ITER_NUM 1000"


nsme_1i=" \
  V_WEIGHT_DECOR_FILE_PATH nsme_gd-1e-1_N-30K_NEG-20_nce_wrh_pow-1 \
  V_TRAIN_METHOD nsme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_NS_POWER 1 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000"

nsme_2i=" \
  V_WEIGHT_DECOR_FILE_PATH nsme_gd-1e-1_N-30K_NEG-20_nce_wrh \
  V_TRAIN_METHOD nsme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000"

me_1i=" \
  V_WEIGHT_DECOR_FILE_PATH me_gd-1e-1_N-30K \
  V_TRAIN_METHOD nsme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 0 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000"

met20_1i=" \
  V_WEIGHT_DECOR_FILE_PATH me-top-20_gd-1e-1_N-30K \
  V_TRAIN_METHOD nsme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 0 \
  V_ME_TOP 20 \
  V_CACHE_INTERMEDIATE_WEIGHT 1 \
  V_ITER_NUM 1000"

eval "bin=\$$1"
eval "model=\$$2"
model=$model
echo $model
$bin $model

