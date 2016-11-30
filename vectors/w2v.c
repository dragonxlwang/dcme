#ifndef W2V
#define W2V

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include "../vectors/model.c"
#include "../vectors/peek.c"
#include "../vectors/variables.c"

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
  if (NumRand() > 0.01) {
    sid_w2v_ppb_lock = 0;
    return;
  }
  char *mdis =
      ModelDebugInfoStr(model, p, tid, start_clock_t, V_THREAD_NUM, gd_ss);
#ifdef DEBUGPEEK
  real avgp = PeekEvalSingleThread(model, ps, C);
  LOGCR(dbg_lvl);
  if (V_MODEL_DECOR_FILE_PATH) LOG(dbg_lvl, "[%s]: ", V_MODEL_DECOR_FILE_PATH);
  LOG(dbg_lvl, "%s", mdis);
  LOG(dbg_lvl, " ");
  LOGC(dbg_lvl, 'c', 'r', "PEEK:%.2e", avgp);
  LOGCR(dbg_lvl);
#else
  LOGCR(dbg_lvl);
  if (V_MODEL_DECOR_FILE_PATH) LOG(dbg_lvl, "[%s]: ", V_MODEL_DECOR_FILE_PATH);
  LOG(dbg_lvl, "%s", mdis);
#endif
  free(mdis);
#else
  char *mdis =
      ModelDebugInfoStr(model, p, tid, start_clock_t, V_THREAD_NUM, gd_ss);
  LOGCLR(dbg_lvl);
  if (V_MODEL_DECOR_FILE_PATH) LOG(dbg_lvl, "[%s]: ", V_MODEL_DECOR_FILE_PATH);
  LOG(dbg_lvl, "%s", mdis);
  free(mdis);
#endif
  sid_w2v_ppb_lock = 0;
  return;
}

void W2vCreateNegSampleInit() {
  LOG(2, "[NS]: Init\n");
  int i;
  real s = 0;
  w2v_neg_prob = NumNewHugeVec(V);
  w2v_neg_prob_log = NumNewHugeVec(V);
  for (i = 0; i < V; i++) {
    w2v_neg_prob[i] = pow(VocabGetCount(vcb, i), V_NS_POWER);
    s += w2v_neg_prob[i];
  }
  for (i = 0; i < V; i++) w2v_neg_prob[i] /= s;
  if (V_NS_WRH)
    NumMultinomialWRBInit(w2v_neg_prob, V, 1, &w2v_nswrh_a, &w2v_nswrh_p);
  else {
    w2v_nstable_r = SMALLER(((real)1e8) / V, 100);
    w2v_nstable = NumMultinomialTableInit(w2v_neg_prob, V, w2v_nstable_r);
  }
  for (i = 0; i < V; i++) w2v_neg_prob_log[i] = log(V_NS_NEG * w2v_neg_prob[i]);
  return;
}

int W2vNegSample(unsigned long *rs) {
  if (V_NS_WRH)
    return NumMultinomialWRBSample(w2v_nswrh_a, w2v_nswrh_p, V, rs);
  else
    return NumMultinomialTableSample(w2v_nstable, V, w2v_nstable_r, rs);
}

void W2vNegSampleFree() {
  if (V_NS_WRH) {
    free(w2v_nswrh_a);
    free(w2v_nswrh_p);
  } else
    free(w2v_nstable);
  return;
}

void W2vUpdate(int *ids, int l, unsigned long *rs) {
  int i, j, k, lt, rt, md, h0n = 0, label;
  real h0[NUP], h[NUP], hw[SUP], wd[NUP * SUP], w0[NUP], w[NUP], f;
  int samp[QUP];
  real prob[QUP];
  int window = C;  // int window = NumRand() * C + 1;
  NumFillZeroVec(h0, N);
  NumFillZeroVec(w0, N);
  for (i = 0; i < SMALLER(l, window); i++) {
    NumVecAddCVec(h0, model->scr + ids[i] * N, 1, N);  // h0++
    h0n++;                                             // h0n++
  }
  for (i = 0; i < l; i++) {
    lt = i - window - 1;
    rt = i + window;
    md = i;
    if (rt < l) {
      NumVecAddCVec(h0, model->scr + ids[rt] * N, 1, N);   // h0++
      h0n++;                                               // h0n++
    }                                                      //
    if (lt >= 0) {                                         //
      NumVecAddCVec(h0, model->scr + ids[lt] * N, -1, N);  // h0--
      h0n--;                                               // h0n--
    }                                                      //
    hw[md] = 1.0 / (h0n - 1.0);                            // hw
    NumAddCVecDVec(h0, model->scr + ids[md] * N, hw[md], -hw[md], N, h);  // h
    ///////////////////////////////////////////////////////////////////////////
    NumFillZeroVec(wd + i * N, N);  // wd
    if (!V_NSME) {
      for (j = 0; j <= V_NS_NEG; j++) {
        k = (j == 0) ? ids[i] : W2vNegSample(rs);
        label = (j == 0) ? 1 : 0;
        if (V_NCE)  // NCE
          f = NumSigmoid(NumVecDot(h, model->tar + k * N, N) -
                         w2v_neg_prob_log[k]);
        else  // NS
          f = NumSigmoid(NumVecDot(h, model->tar + k * N, N));
        NumVecAddCVec(wd + i * N, model->tar + k * N, label - f, N);
        ModelGradUpdate(model, 1, k, -(label - f) * gd_ss, h);  // up m->tar
        ModelVecRegularize(model, 1, k, V_MODEL_PROJ_BALL_NORM,
                           V_L2_REGULARIZATION_WEIGHT);
      }
    } else {
      for (j = 0; j <= V_NS_NEG; j++) {
        k = (j < V_NS_NEG) ? W2vNegSample(rs) : ids[i];
        samp[j] = k;
        if (V_NCE)  // NCE
          prob[j] = NumVecDot(h, model->tar + k * N, N) - w2v_neg_prob_log[k];
        else  // NS
          prob[j] = NumVecDot(h, model->tar + k * N, N);
      }
      NumSoftMax(prob, 1, V_NS_NEG + 1);
      for (j = 0; j <= V_NS_NEG; j++) {
        k = samp[j];
        f = (k == ids[i]) ? 1 : 0 - prob[j];
        NumVecAddCVec(wd + i * N, model->tar + k * N, f, N);
        ModelGradUpdate(model, 1, k, -f * gd_ss, h);
        ModelVecRegularize(model, 1, k, V_MODEL_PROJ_BALL_NORM,
                           V_L2_REGULARIZATION_WEIGHT);
      }
    }
    ///////////////////////////////////////////////////////////////////////////
    lt = i - 2 * window - 1;
    rt = i;
    md = i - window;
    NumVecAddCVec(w0, wd + rt * N, hw[rt], N);                  // w0++
    while (md < l) {                                            //
      if (lt >= 0) NumVecAddCVec(w0, wd + lt * N, -hw[lt], N);  // w0--
      if (md >= 0) {                                            //
        NumAddCVecDVec(w0, wd + md * N, 1, -hw[md], N, w);      // w
        ModelGradUpdate(model, 0, ids[md], -gd_ss, w);          // up m->scr
        ModelVecRegularize(model, 0, ids[md], V_MODEL_PROJ_BALL_NORM,
                           V_L2_REGULARIZATION_WEIGHT);
      }
      if (i == l - 1) {
        md++;
        lt++;
      } else
        break;
    }
  }
  return;
}

void *W2vThreadTrain(void *arg) {
  int tid = (long)arg;
  FILE *fin = fopen(V_TEXT_FILE_PATH, "rb");
  if (!fin) {
    LOG(0, "Error!\n");
    exit(1);
  }
  fseek(fin, 0, SEEK_END);
  long int fsz = ftell(fin);
  long int fbeg = fsz / V_THREAD_NUM * tid;
  long int fend = fsz / V_THREAD_NUM * (tid + 1);
  long int fpos = fbeg;
  int wids[SUP], wnum;
  real p = 0;
  int iter_num = 0;
  fseek(fin, fbeg, SEEK_SET);  // training
  ///////////////////////////////////////////////////////////////////////////
  int i = 0;
  unsigned long rs = tid;
  while (iter_num < V_ITER_NUM) {
    wnum = TextReadSent(fin, vcb, wids, V_TEXT_LOWER, V_TEXT_RM_TRAIL_PUNC, 1);
    fpos = ftell(fin);
    if (wnum > 1) W2vUpdate(wids, wnum, &rs);
    if (i++ >= 10000) {
      i = 0;
      progress[tid] = iter_num + (double)(fpos - fbeg) / (fend - fbeg);  // prog
      p = ModelTrainProgress(progress, V_THREAD_NUM, V_ITER_NUM);        //
      gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE * (1 - p);                   // gdss
      W2vThreadPrintProgBar(2, tid, p);                                  // info
    }
    if (feof(fin) || fpos >= fend) {
      fseek(fin, fbeg, SEEK_SET);
      if (V_CACHE_INTERMEDIATE_MODEL &&
          iter_num % V_CACHE_INTERMEDIATE_MODEL == 0)
        ModelSave(model, iter_num, V_MODEL_SAVE_PATH);
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
