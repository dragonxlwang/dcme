#ifndef NSME
#define NSME

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

int *nsme_nswrh_a;    // WRH sample alias array
real *nsme_nswrh_p;   // WRH sample probability array
int *nsme_nstable;    // Table sample
real nsme_nstable_r;  // Table size ratio
real *nsme_neg_prob;
real *nsme_neg_prob_log;

int sid_nsme_ppb_lock = 0;
void NsmeThreadPrintProgBar(int dbg_lvl, int tid, real p) {
  if (sid_nsme_ppb_lock) return;
  sid_nsme_ppb_lock = 1;
#ifdef DEBUG
  if (NumRand() > 0.1) {
    sid_nsme_ppb_lock = 0;
    return;
  }
  char *wdis = WeightDebugInfoStr(weight, C, N, p, tid, start_clock_t,
                                  V_THREAD_NUM, gd_ss);
  LOGCR(dbg_lvl);
  if (V_WEIGHT_DECOR_FILE_PATH)
    LOG(dbg_lvl, "[%s]: ", V_WEIGHT_DECOR_FILE_PATH);
  LOG(dbg_lvl, "%s", wdis);
  free(wdis);
#else
  char *wdis = WeightDebugInfoStr(weight, C, N, p, tid, start_clock_t,
                                  V_THREAD_NUM, gd_ss);
  LOGCLR(dbg_lvl);
  if (V_WEIGHT_DECOR_FILE_PATH)
    LOG(dbg_lvl, "[%s]: ", V_WEIGHT_DECOR_FILE_PATH);
  LOG(dbg_lvl, "%s", wdis);
  free(wdis);
#endif
  sid_nsme_ppb_lock = 0;
  return;
}

void NsmeCreateNegSampleInit() {
  LOG(2, "[NS]: Init\n");
  int i;
  real s = 0;
  nsme_neg_prob = NumNewHugeVec(C);
  nsme_neg_prob_log = NumNewHugeVec(C);
  for (i = 0; i < C; i++) {
    nsme_neg_prob[i] = pow(DictGetVal(classes, i), V_NS_POWER);
    s += nsme_neg_prob[i];
  }
  for (i = 0; i < C; i++) nsme_neg_prob[i] /= s;
  if (V_NS_WRH)
    NumMultinomialWRBInit(nsme_neg_prob, C, 1, &nsme_nswrh_a, &nsme_nswrh_p);
  else {
    nsme_nstable_r = SMALLER(((real)1e8) / C, 100);
    nsme_nstable = NumMultinomialTableInit(nsme_neg_prob, C, nsme_nstable_r);
  }
  for (i = 0; i < C; i++)
    nsme_neg_prob_log[i] = log(V_NS_NEG * nsme_neg_prob[i]);
  return;
}

int NsmeNegSample(unsigned long *rs) {
  if (V_NS_WRH)
    return NumMultinomialWRBSample(nsme_nswrh_a, nsme_nswrh_p, C, rs);
  else
    return NumMultinomialTableSample(nsme_nstable, C, nsme_nstable_r, rs);
}

void NsmeNegSampleFree() {
  if (V_NS_WRH) {
    free(nsme_nswrh_a);
    free(nsme_nswrh_p);
  } else
    free(nsme_nstable);
  return;
}

void sid_nsme_me_update(int *fsv, int fn, int label, real *prob, int *ids,
                        int num) {
  NumSoftMax(prob, 1, num);
  int j, k, b;
  for (j = 0; j < num; j++) {
    k = ids[j];
    b = (k == label) ? 1 : 0;
    NumVecAddCSvOnes(weight + k * N, fsv, fn, (b - prob[j]) * gd_ss);
    WeightVecRegularize(weight, k, V_WEIGHT_PROJ_BALL_NORM,
                        V_L2_REGULARIZATION_WEIGHT, N);
  }
  return;
}

void NsmeMeUpdate(int *fsv, int fn, int label, unsigned long *rs, heap *h) {
  int k, b, num, flag = 0;
  real prob[CUP];
  int ids[CUP];

  for (k = 0; k < C; k++) prob[k] = NumSvSum(fsv, fn, weight + k * N);
  if (V_ME_TOP) {
    HeapEmpty(h);
    for (k = 0; k < C; k++) HeapPush(h, k, prob[k]);
    for (k = 0; k < V_ME_TOP; k++) {
      HeapPop(h);
      ids[k] = HeapTopKey(h);
      prob[k] = HeapTopVal(h);
      if (ids[k] == label) flag = 1;
    }
    if (!flag) {
      ids[V_ME_TOP] = label;
      prob[V_ME_TOP] = NumSvSum(fsv, fn, weight + label * N);
    }
    num = V_ME_TOP + 1 - flag;
  } else {
    for (k = 0; k < C; k++) ids[k] = k;
    num = C;
  }
  sid_nsme_me_update(fsv, fn, label, prob, ids, num);
  return;
}

void NsmeUpdate(int *fsv, int fn, int label, unsigned long *rs) {
  int j, k;
  real prob[QUP];
  int ids[QUP];
  for (j = 0; j <= V_NS_NEG; j++) {
    k = (j < V_NS_NEG) ? NsmeNegSample(rs) : label;
    ids[j] = k;
    if (V_NCE)
      prob[j] = NumSvSum(fsv, fn, weight + k * N) - nsme_neg_prob_log[k];
    else  // NS
      prob[j] = NumSvSum(fsv, fn, weight + k * N);
  }
  sid_nsme_me_update(fsv, fn, label, prob, ids, V_NS_NEG + 1);
  return;
}

void *NsmeThreadTrain(void *arg) {
  int tid = (long)arg;
  FILE *fin = fopen(V_TRAIN_FILE_PATH, "rb");
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
  int i = 0;
  unsigned long rs = tid;
  heap *hp;
  if (V_ME_TOP) hp = HeapCreate(V_ME_TOP);
  while (iter_num < V_ITER_NUM) {
    HelperReadInstance(fin, vcb, classes, fsv, &fn, &label, V_TEXT_LOWER,
                       V_TEXT_RM_TRAIL_PUNC);
    fpos = ftell(fin);
    if (fn > 0 && label != -1) {
      if (V_NS_NEG)
        NsmeUpdate(fsv, fn, label, &rs);
      else
        NsmeMeUpdate(fsv, fn, label, &rs, hp);
    }
    if (i++ >= 10000) {
      i = 0;
      progress[tid] = iter_num + (double)(fpos - fbeg) / (fend - fbeg);  // prog
      p = WeightTrainProgress(progress, V_THREAD_NUM, V_ITER_NUM);       //
      gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE * (1 - p);                   // gdss
      NsmeThreadPrintProgBar(2, tid, p);                                 // info
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

void NsmePrep() {
  if (V_NS_NEG) NsmeCreateNegSampleInit();
  return;
}

void NsmeClean() {
  if (V_NS_NEG) NsmeNegSampleFree();
  return;
}
#endif /* ifndef NSME */
