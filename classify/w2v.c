#ifndef W2V
#define W2V

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

int *w2v_nswrh_a;    // WRH sample alias array
real *w2v_nswrh_p;   // WRH sample probability array
int *w2v_nstable;    // Table sample
real w2v_nstable_r;  // Table size ratio
real *w2v_neg_prob;
real *w2v_neg_prob_log;

int sid_w2v_ppb_lock = 0;
void W2vThreadPrintProgBar(int dbg_lvl, int tid, real p) {
  if (sid_w2v_ppb_lock) return;
  sid_w2v_ppb_lock = 1;
#ifdef DEBUG
  if (NumRand() > 0.1) {
    sid_w2v_ppb_lock = 0;
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
  sid_w2v_ppb_lock = 0;
  return;
}

void W2vCreateNegSampleInit() {
  LOG(2, "[NS]: Init\n");
  int i;
  real s = 0;
  w2v_neg_prob = NumNewHugeVec(C);
  w2v_neg_prob_log = NumNewHugeVec(C);
  for (i = 0; i < C; i++) {
    w2v_neg_prob[i] = pow(DictGetVal(classes, i), V_NS_POWER);
    s += w2v_neg_prob[i];
  }
  for (i = 0; i < C; i++) w2v_neg_prob[i] /= s;
  if (V_NS_WRH)
    NumMultinomialWRBInit(w2v_neg_prob, C, 1, &w2v_nswrh_a, &w2v_nswrh_p);
  else {
    w2v_nstable_r = SMALLER(((real)1e8) / C, 100);
    w2v_nstable = NumMultinomialTableInit(w2v_neg_prob, C, w2v_nstable_r);
  }
  for (i = 0; i < C; i++) w2v_neg_prob_log[i] = log(V_NS_NEG * w2v_neg_prob[i]);
  return;
}

int W2vNegSample(unsigned long *rs) {
  if (V_NS_WRH)
    return NumMultinomialWRBSample(w2v_nswrh_a, w2v_nswrh_p, C, rs);
  else
    return NumMultinomialTableSample(w2v_nstable, C, w2v_nstable_r, rs);
}

void W2vNegSampleFree() {
  if (V_NS_WRH) {
    free(w2v_nswrh_a);
    free(w2v_nswrh_p);
  } else
    free(w2v_nstable);
  return;
}

void W2vUpdate(int *fsv, int fn, int label, unsigned long *rs) {
  int j, k, b;
  real f;
  for (j = 0; j <= V_NS_NEG; j++) {
    k = (j == 0) ? label : W2vNegSample(rs);
    b = (j == 0) ? 1 : 0;
    if (V_NCE)  // NCE
      f = NumSigmoid(NumSvSum(fsv, fn, weight + k * N) - w2v_neg_prob_log[k]);
    else  // NS
      f = NumSigmoid(NumSvSum(fsv, fn, weight + k * N));
    NumVecAddCSvOnes(weight + k * N, fsv, fn, (b - f) * gd_ss);
    WeightVecRegularize(weight, k, V_WEIGHT_PROJ_BALL_NORM,
                        V_L2_REGULARIZATION_WEIGHT, N);
  }
  return;
}

void *W2vThreadTrain(void *arg) {
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
  while (iter_num < V_ITER_NUM) {
    HelperReadInstance(fin, vcb, classes, fsv, &fn, &label, V_TEXT_LOWER,
                       V_TEXT_RM_TRAIL_PUNC);
    fpos = ftell(fin);
    if (fn > 0 && label != -1) W2vUpdate(fsv, fn, label, &rs);
    if (i++ >= 10000) {
      i = 0;
      progress[tid] = iter_num + (double)(fpos - fbeg) / (fend - fbeg);  // prog
      p = WeightTrainProgress(progress, V_THREAD_NUM, V_ITER_NUM);       //
      gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE * (1 - p);                   // gdss
      W2vThreadPrintProgBar(2, tid, p);                                  // info
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

void W2vPrep() {
  W2vCreateNegSampleInit();
  return;
}

void W2vClean() {
  W2vNegSampleFree();
  return;
}
#endif /* ifndef W2V */
