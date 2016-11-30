//////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Xiaolong Wang All Rights Reserved.        //
// Code for ``Dual Clustered Maximal Entropy for Many Classes'' //
//////////////////////////////////////////////////////////////////
#ifndef DCME
#define DCME

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../classify/helper.c"
#include "../classify/variables.c"
#include "../classify/weight.c"
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"

typedef struct DcmeBookkeeping {  // each thread worker maintains a bookkeeping
  real* dd;                       // list(K):vector(C) dual distribution
  real* ent;                      // list(K):
  real* ww;                       // list(K):vector(N) W * dd, synthesis word
  real* hh;                       // list(K):vector(N) avg(h_k)
  real* hi;                       // list(K):vector(C)
  real* hn;                       // list(K): |h_k|
  int* tw;                        // list(K):list(Q) top w (labels)
  real* twps;                     // list(K): top w probability sum
  real* ow;                       // list(K):vector(N)
  int* freeze;                    // flag to show if currently being updated
  int last_updated_zz;
} DcmeBookkeeping;
DcmeBookkeeping** blst;
heap** hlst;

char* DcmeDualModelDebugInfoStr(DcmeBookkeeping* b) {
  char* ddis = malloc(0x1000);
  real eem = NumVecMean(b->ent, K);
  real ees = NumVecStd(b->ent, K);
  sprintfc(ddis, 'g', 'k', "ENT:%.2e\u00b1%.2e", eem, ees);
  saprintf(ddis, " ");
  saprintf(ddis, "twps: %.3e zz: %02d", b->twps[b->last_updated_zz],
           b->last_updated_zz);
  if (NumIsNan(b->twps[b->last_updated_zz])) {
    saprintf(ddis, "NAN");
  } else {
    char* pb = strprogbarc(b->twps[b->last_updated_zz], 80, 0);
    saprintf(ddis, "%s", pb);
    free(pb);
  }
  return ddis;
}

int sid_dcme_ppb_lock = 0;
void DcmeThreadPrintProgBar(int dbg_lvl, int tid, real p, DcmeBookkeeping* b) {
  if (sid_dcme_ppb_lock) return;
  sid_dcme_ppb_lock = 1;
#ifdef DEBUG
  if (NumRand() > 0.01) {
    sid_dcme_ppb_lock = 0;
    return;
  }
  char* wdis = WeightDebugInfoStr(weight, C, N, p, tid, start_clock_t,
                                  V_THREAD_NUM, gd_ss);
  char* ddis = DcmeDualModelDebugInfoStr(b);
  LOGCR(dbg_lvl);
  if (V_WEIGHT_DECOR_FILE_PATH)
    LOG(dbg_lvl, "[%s]: ", V_WEIGHT_DECOR_FILE_PATH);
  LOG(dbg_lvl, "%s", wdis);
  LOG(dbg_lvl, " ");
  LOG(dbg_lvl, "%s", ddis);
  free(wdis);
  free(ddis);
#else
  char* wdis = WeightDebugInfoStr(weight, C, N, p, tid, start_clock_t,
                                  V_THREAD_NUM, gd_ss);
  LOGCLR(dbg_lvl);
  if (V_WEIGHT_DECOR_FILE_PATH)
    LOG(dbg_lvl, "[%s]: ", V_WEIGHT_DECOR_FILE_PATH);
  LOG(dbg_lvl, "%s", wdis);
  free(wdis);
#endif
  sid_dcme_ppb_lock = 0;
  return;
}

DcmeBookkeeping* DcmeBookkeepingCreate() {
  DcmeBookkeeping* b = (DcmeBookkeeping*)malloc(sizeof(DcmeBookkeeping));
  b->dd = NumNewHugeVec(C * K);
  b->ent = NumNewHugeVec(K);
  b->ww = NumNewHugeVec(N * K);
  b->hh = NumNewHugeVec(N * K);
  b->hi = NumNewHugeVec(C * K);
  b->hn = NumNewHugeVec(K);
  b->tw = NumNewHugeIntVec(Q * K);
  b->twps = NumNewHugeVec(K);
  b->ow = NumNewHugeVec(N * K);
  b->freeze = NumNewHugeIntVec(K);
  NumRandFillVec(b->hh, N * K, 0, weight_init_amp);
  NumFillValVec(b->hn, K, 1);
  NumFillZeroIntVec(b->freeze, K);
  return b;
}

void DcmeBookkeepingFree(DcmeBookkeeping* b) {
  free(b->dd);
  free(b->ent);
  free(b->ww);
  free(b->hh);
  free(b->hi);
  free(b->hn);
  free(b->tw);
  free(b->twps);
  free(b->ow);
  free(b->freeze);
  free(b);
  return;
}

int DcmeDualDecode(int* hsvk, int hsvn, DcmeBookkeeping* b) {
  // fit the sentence with the best dual distribution (z)
  // which maximizes h * w(z) + entropy(z)
  real s = 0, t;
  int z = 0, k;
  for (k = 0; k < K; k++) {
    t = NumSvSum(hsvk, hsvn, b->ww + k * N) + b->ent[k];
    if (k == 0 || t > s) {
      s = t;
      z = k;
    }
  }
  return z;
}

void DcmeDualUpdate(int zz, DcmeBookkeeping* b, heap* twh) {
  // quick update: dd, ww, ow, twps,
  int j, k, tw[QUP];
  real ent, twps, dd[CUP], ow[NUP], ww[NUP];
  NumMulMatVec(weight, b->hh + zz * N, C, N, dd);  // dd
  ent = NumSoftMax(dd, b->hn[zz], C);              // ent (sm)
  NumCopyVec(b->dd + zz * C, dd, C);
  b->ent[zz] = ent;
  // dual reset distribution (hh and hn) of zz --------------------------------
  if (V_DUAL_RESET_OPT == 1) {  // ------------------- 1) clean reset
    b->hn[zz] = 0;
    NumFillZeroVec(b->hh + zz * N, N);
  } else if (V_DUAL_RESET_OPT == 2) {  // ------------ 2) half reset
    b->hn[zz] *= 0.5;
    NumVecMulC(b->hh + zz * N, 0.5, N);
  } else if (V_DUAL_RESET_OPT == 3) {  // ------------ 3) decay/overshoot reset
    real v = 0.1;
    b->hn[zz] *= v;
    NumVecMulC(b->hh + zz * N, v, N);
  }  // ---------------------------------------------- 4) no reset
  // --------------------------------------------------------------------------
  if (Q > 0) {                                                    // top class
    HeapEmpty(twh);                                               // tw reset
    for (j = 0; j < C; j++) HeapPush(twh, j, b->dd[zz * C + j]);  // tw add
    for (j = 0; j < Q; j++) tw[j] = twh->d[j].key;                // tw load
    qsort(tw, Q, sizeof(int), cmp_int);                           // tw sort
    NumCopyIntVec(b->tw + zz * Q, tw, Q);
    twps = 0;
    for (j = 0; j < Q; j++) {
      k = b->tw[zz * Q + j];
      twps += b->dd[zz * C + k];
    }
    b->twps[zz] = twps;
  }
  if (V_MICRO_ME) {  // Q > 0 required
    NumFillZeroVec(ow, N);
    NumFillZeroVec(ww, N);
    for (j = 0, k = 0; j < C; j++) {  // ww, ow: two way merge of tw and ow
      if (j == b->tw[zz * Q + k]) {   // tw
        NumVecAddCVec(ww, weight + j * N, b->dd[zz * C + j], N);
        k++;
      } else
        NumVecAddCVec(ow, weight + j * N, b->dd[zz * C + j], N);
    }
    NumVecAddCVec(b->ww + zz * N, b->ow + zz * N, 1, N);  // ww: adding ow
    NumCopyVec(b->ww + zz * N, ww, N);
    NumCopyVec(b->ow + zz * N, ow, N);
  } else {
    NumMulVecMat(b->dd + zz * C, weight, C, N, ww);  // ww
    NumCopyVec(b->ww + zz * N, ww, N);
  }
  b->last_updated_zz = zz;
  return;
}

void DcmeMicroME(int zz, int label, int* fsv, int fn, DcmeBookkeeping* b) {
  int j, k, tw[QUP], twlen;
  real twps, twp[QUP];
  twlen = Q + 1;
  twps = b->twps[zz];
  for (k = 0; k < Q; k++) {
    j = b->tw[zz * Q + k];
    twp[k] = NumSvSum(fsv, fn, weight + j * N);
    tw[k] = j;
    if (j == label) twlen--;
  }
  if (twlen == Q + 1) {
    twp[Q] = NumSvSum(fsv, fn, weight + label * N);
    tw[Q] = label;
    twps += b->dd[zz * C + label];
  }
  NumSoftMax(twp, 1.0, twlen);
  NumVecMulC(twp, twps, twlen);
  if (twlen == Q + 1) twp[Q] -= b->dd[zz * C + label];
  for (k = 0; k < twlen; k++) {
    NumVecAddCSvOnes(weight + tw[k] * N, fsv, fn,
                     ((tw[k] == label ? 1 : 0) - twp[k]) * gd_ss);
    WeightVecRegularize(weight, tw[k], V_WEIGHT_PROJ_BALL_NORM,
                        V_L2_REGULARIZATION_WEIGHT, N);
  }
  return;
}

int DcmeDualFreeze(int zz, DcmeBookkeeping* b) {
  int i = 0;
  for (i = 0; i < 5e2; i++)
    if (b->freeze[zz]) return 0;
  b->freeze[zz] = 1;
  return 1;
}

void DcmeDualRelease(int zz, DcmeBookkeeping* b) { b->freeze[zz] = 0; }

int DcmeOfflineUpdate(int zz, DcmeBookkeeping* b, heap* twh) {
  if (!DcmeDualFreeze(zz, b)) return 0;
  int j, k;
  for (j = 0, k = 0; j < C; j++) {
    if (k < Q && j == b->tw[zz * Q + k])
      k++;
    else {
      NumVecAddCVec(weight + j * N, b->hh + zz * N, -b->dd[zz * C + j] * gd_ss,
                    N);
      WeightVecRegularize(weight, j, V_WEIGHT_PROJ_BALL_NORM, -1, N);
    }
  }
  if (V_L2_REGULARIZATION_WEIGHT >= 0)
    NumVecMulC(weight, 1 - V_L2_REGULARIZATION_WEIGHT, N * C);
  DcmeDualUpdate(zz, b, twh);
  DcmeDualRelease(zz, b);
  return 1;
}

int DcmeUpdate(int* fsv, int fn, int label, DcmeBookkeeping* b, heap* twh) {
  int j, k, zz, flag;
  real c;
  zz = DcmeDualDecode(fsv, fn, b);
  b->hn[zz] += 1;
  NumVecAddCSvOnes(b->hh + zz * N, fsv, fn, 1);
  if (V_MICRO_ME) {
    DcmeMicroME(zz, label, fsv, fn, b);
  } else {
    flag = 1;
    for (k = 0; k < Q; k++) {
      j = b->tw[zz * Q + k];
      if (j == label) flag = 0;
      c = (j == label ? 1 : 0) - b->dd[zz * C + j];
      NumVecAddCSvOnes(weight + j * N, fsv, fn, c * gd_ss);
      WeightVecRegularize(weight, j, V_WEIGHT_PROJ_BALL_NORM,
                          V_L2_REGULARIZATION_WEIGHT, N);
    }
    if (flag) {
      NumVecAddCSvOnes(weight + label * N, fsv, fn, gd_ss);
      WeightVecRegularize(weight, label, V_WEIGHT_PROJ_BALL_NORM,
                          V_L2_REGULARIZATION_WEIGHT, N);
    }
  }
  return (b->hn[zz] >= C * V_OFFLINE_INTERVAL_CLASS_RATIO &&
          DcmeOfflineUpdate(zz, b, twh));
}

void* DcmeThreadTrain(void* arg) {
  int tid = (long)arg;
  FILE* fin = fopen(V_TRAIN_FILE_PATH, "rb");
  if (!fin) {
    LOG(0, "Error!\n");
    exit(1);
  }
  fseek(fin, 0, SEEK_END);
  long int fbeg = fpos_beg[tid];
  long int fend = fpos_end[tid];
  long int fpos = fbeg;
  int fsv[LUP], fn, label;
  real p = 0;
  int iter_num = 0;
  fseek(fin, fbeg, SEEK_SET);  // training
  ///////////////////////////////////////////////////////////////////////////
  DcmeBookkeeping* b = blst[tid / V_THREADS_PER_DUAL];
  heap* twh = hlst[tid / V_THREADS_PER_DUAL];
  while (iter_num < V_ITER_NUM) {
    HelperReadInstance(fin, vcb, classes, fsv, &fn, &label, V_TEXT_LOWER,
                       V_TEXT_RM_TRAIL_PUNC);
    fpos = ftell(fin);
    if (fn > 0 && label != -1 && DcmeUpdate(fsv, fn, label, b, twh)) {
      progress[tid] = iter_num + (double)(fpos - fbeg) / (fend - fbeg);  // prog
      p = WeightTrainProgress(progress, V_THREAD_NUM, V_ITER_NUM);       //
      gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE * (1 - p);                   // gdss
      DcmeThreadPrintProgBar(2, tid, p, b);                              // info
    }
    if (feof(fin) || fpos >= fend) {
      fseek(fin, fbeg, SEEK_SET);
      if (V_CACHE_INTERMEDIATE_WEIGHT &&
          iter_num % V_CACHE_INTERMEDIATE_WEIGHT == 0)
        WeightSave(weight, C, N, iter_num, V_WEIGHT_SAVE_PATH);
      iter_num++;
    }
  }
  ///////////////////////////////////////////////////////////////////////////
  fclose(fin);
  pthread_exit(NULL);
  return 0;
}

void DcmePrep() {
  int d = V_THREAD_NUM / V_THREADS_PER_DUAL + 1;
  blst = (DcmeBookkeeping**)malloc(d * sizeof(DcmeBookkeeping*));
  hlst = (heap**)malloc(d * sizeof(heap*));
  int i, k;
  for (i = 0; i < d; i++) {
    blst[i] = DcmeBookkeepingCreate();
    hlst[i] = HeapCreate(Q);
  }
  LOGCR(2);
  for (i = 0; i < d; i++)
    for (k = 0; k < K; k++) {
      LOGCLR(2);
      LOG(2, "Initializing dual cluster %d @ %d", k, i);
      DcmeDualUpdate(k, blst[i], hlst[i]);
    }
  LOGCLR(2);
  LOG(2, "dual clusters initialized\n");
  return;
}

void DcmeClean() {
  int d = V_THREAD_NUM / V_THREADS_PER_DUAL + 1;
  int i;
  for (i = 0; i < d; i++) {
    DcmeBookkeepingFree(blst[i]);
    HeapFree(hlst[i]);
  }
  free(blst);
  free(hlst);
  return;
}

#endif /* ifndef DCME */
