#ifndef VARIABLES
#define VARIABLES

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
#include "../vectors/constants.c"
#include "../vectors/model.c"
#include "../vectors/peek.c"

// ---------------------------- Config Variables ------------------------------
char *V_MODEL_DECOR_FILE_PATH = NULL;
char *V_TEXT_FILE_PATH = "~/data/gigaword/giga_nyt.txt";  // "text8/text8"
char *V_VOCAB_FILE_PATH = NULL;  // don't set it if can be inferred from above
char *V_MODEL_SAVE_PATH = NULL;
char *V_PEEK_FILE_PATH = NULL;

int V_THREAD_NUM = 20;
int V_ITER_NUM = 10;

// Initial grad descent step size
real V_INIT_GRAD_DESCENT_STEP_SIZE = 1e-4;
// Model Shrink: l-2 regularization:
real V_L2_REGULARIZATION_WEIGHT = -1;  // 1e-3;
// Model Shrink: if proj model to ball with specified norm, -1 if not proj
real V_MODEL_PROJ_BALL_NORM = -1;
// Vocab loading option: cut-off high frequent (stop) words
int V_VOCAB_HIGH_FREQ_CUTOFF = 80;
// if convert uppercase to lowercase
int V_TEXT_LOWER = 1;
// if remove trailing punctuation
int V_TEXT_RM_TRAIL_PUNC = 1;
// if cache model per iteration
int V_CACHE_INTERMEDIATE_MODEL = 0;
// if overwrite vocab file
int V_VOCAB_OVERWRITE = 0;
// if load model from file instead of random initiailization
int V_MODEL_LOAD = 0;
// if overwrite peek file
int V_PEEK_OVERWRITE = 0;
// Peek sampling rate
real V_PEEK_SAMPLE_RATE = 1e-6;
// Peek top K words
int V_PEEK_TOP_K = 200;
int N = 100;      // embedding dimension
int V = 1000000;  // vocabulary size cap, set to -1 if no limit
int C = 5;        // context length
// method
char *V_TRAIN_METHOD = "dcme";
// ----------------------------- DCME specific --------------------------------
// every this number times vocabulary online updates perform one offline update
real V_OFFLINE_INTERVAL_VOCAB_RATIO = 1;
// if using micro maximal entropy for top words plus target words
int V_MICRO_ME = 0;
// if also use micro-me adjusted distribution for updateing scr (w - ww)
int V_MICRO_ME_SCR_UPDATE = 0;
// Dual Reset option
int V_DUAL_RESET_OPT = 1;
// Dual update with hi not hh
int V_DUAL_HI = 0;
// Adjust the WW while online updating
real V_ADJUST_WW = -1;
int K = 20;  // number of dual cluster
int Q = 10;  // Number of top words in online update
// ----------------------------- W2V specific --------------------------------
int V_NS_WRH = 1;
real V_NS_POWER = 0.75;
int V_NS_NEG = 20;
int V_NCE = 0;
int V_NSME = 0;

// ---------------------------- global variables ------------------------------
Vocabulary *vcb;        // vocabulary
Model *model;           // model
PeekSet *ps = NULL;     // peek set
real gd_ss;             // gradient descent step
real *progress;         // thread progress
clock_t start_clock_t;  // start time for model training
// ------------------------ global private variables --------------------------
real model_init_amp = 1e-4;  // model init: small value for numeric stability
typedef void *(*ThreadTrain)(void *);
typedef void (*MainWork)();

void VariableFree() {
  ModelFree(model);  // <<
  VocabFree(vcb);    // <<
#ifdef DEBUG
  PeekSetFree(ps);  // <<
#endif
  free(progress);  // <<
}

void VariableInit(int argc, char **argv) {
  int i;
  char c = 'w';

  i = getoptpos("V_MODEL_DECOR_FILE_PATH", argc, argv);
  if (i != -1) {
    V_MODEL_DECOR_FILE_PATH = sclone(argv[i + 1]);
    LOGC(1, 'g', 'k', "%s",
         sformat("== Config: %s ==\n", V_MODEL_DECOR_FILE_PATH));
  } else
    LOGC(1, 'g', 'k', "== Config ==\n");

  i = getoptpos("V_TEXT_FILE_PATH", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_TEXT_FILE_PATH = sclone(argv[i + 1]);
  V_TEXT_FILE_PATH = FilePathExpand(V_TEXT_FILE_PATH);
  LOGC(1, c, 'k', "Input File -------------------------------------- : %s\n",
       V_TEXT_FILE_PATH);

  i = getoptpos("V_TEXT_LOWER", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_TEXT_LOWER = atoi(argv[i + 1]);
  i = getoptpos("V_TEXT_RM_TRAIL_PUNC", argc, argv);
  if (c == 'w') c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_TEXT_RM_TRAIL_PUNC = atoi(argv[i + 1]);
  i = getoptpos("V_VOCAB_FILE_PATH", argc, argv);
  if (c == 'w') c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_VOCAB_FILE_PATH = sclone(argv[i + 1]);
  if (!V_VOCAB_FILE_PATH)
    V_VOCAB_FILE_PATH = FilePathSubExtension(
        V_TEXT_FILE_PATH,
        sformat("l%dr%d.vcb", V_TEXT_LOWER, V_TEXT_RM_TRAIL_PUNC));
  else
    V_VOCAB_FILE_PATH = FilePathExpand(V_VOCAB_FILE_PATH);
  LOGC(1, c, 'k', "Vocab File -------------------------------------- : %s\n",
       V_VOCAB_FILE_PATH);

  i = getoptpos("V_MODEL_SAVE_PATH", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_MODEL_SAVE_PATH = sclone(argv[i + 1]);
  if (!V_MODEL_SAVE_PATH) {
    if (V_MODEL_DECOR_FILE_PATH) {
      V_MODEL_SAVE_PATH = FilePathSubExtension(
          V_TEXT_FILE_PATH, sformat("%s.mdl", V_MODEL_DECOR_FILE_PATH));
      c = 'r';
    } else
      V_MODEL_SAVE_PATH = FilePathSubExtension(V_TEXT_FILE_PATH, "mdl");
  } else
    V_MODEL_SAVE_PATH = FilePathExpand(V_MODEL_SAVE_PATH);
  LOGC(1, c, 'k', "Model File -------------------------------------- : %s\n",
       V_MODEL_SAVE_PATH);

  i = getoptpos("V_PEEK_SAMPLE_RATE", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_PEEK_SAMPLE_RATE = atof(argv[i + 1]);
  i = getoptpos("V_PEEK_FILE_PATH", argc, argv);
  if (c == 'w') c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_PEEK_FILE_PATH = sclone(argv[i + 1]);
  if (!V_PEEK_FILE_PATH)
    V_PEEK_FILE_PATH = FilePathSubExtension(
        V_TEXT_FILE_PATH, sformat("l%dr%ds%.2e.pek", V_TEXT_LOWER,
                                  V_TEXT_RM_TRAIL_PUNC, V_PEEK_SAMPLE_RATE));
  else
    V_PEEK_FILE_PATH = FilePathExpand(V_PEEK_FILE_PATH);
  LOGC(1, c, 'k', "Peek File --------------------------------------- : %s\n",
       V_PEEK_FILE_PATH);

  i = getoptpos("V_THREAD_NUM", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_THREAD_NUM = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Thread Num -------------------------------------- : %d\n",
       V_THREAD_NUM);

  i = getoptpos("V_ITER_NUM", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_ITER_NUM = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Iterations -------------------------------------- : %d\n",
       V_ITER_NUM);

  i = getoptpos("V_INIT_GRAD_DESCENT_STEP_SIZE", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_INIT_GRAD_DESCENT_STEP_SIZE = atof(argv[i + 1]);
  LOGC(1, c, 'k', "Initial Grad Descent Step Size ------------------ : %lf\n",
       (double)V_INIT_GRAD_DESCENT_STEP_SIZE);

  i = getoptpos("V_L2_REGULARIZATION_WEIGHT", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_L2_REGULARIZATION_WEIGHT = atof(argv[i + 1]);
  LOGC(1, c, 'k', "L2 Regularization Weight ------------------------ : %lf\n",
       (double)V_L2_REGULARIZATION_WEIGHT);

  i = getoptpos("V_MODEL_PROJ_BALL_NORM", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_MODEL_PROJ_BALL_NORM = atof(argv[i + 1]);
  LOGC(1, c, 'k', "Project Model Inside Ball with Norm ------------- : %lf\n",
       (double)V_MODEL_PROJ_BALL_NORM);

  i = getoptpos("V_VOCAB_HIGH_FREQ_CUTOFF", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_VOCAB_HIGH_FREQ_CUTOFF = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Vocabulary high-freq words cut-off -------------- : %d\n",
       V_VOCAB_HIGH_FREQ_CUTOFF);

  i = getoptpos("V_TEXT_LOWER", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_TEXT_LOWER = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Convert uppercase to lowercase letters ---------- : %d\n",
       V_TEXT_LOWER);

  i = getoptpos("V_TEXT_RM_TRAIL_PUNC", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_TEXT_RM_TRAIL_PUNC = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Remove trailing punctuation --------------------- : %d\n",
       V_TEXT_RM_TRAIL_PUNC);

  i = getoptpos("V_CACHE_INTERMEDIATE_MODEL", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_CACHE_INTERMEDIATE_MODEL = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Cache intermediate models ----------------------- : %d\n",
       V_CACHE_INTERMEDIATE_MODEL);

  i = getoptpos("V_VOCAB_OVERWRITE", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_VOCAB_OVERWRITE = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Overwrite vocabulary file ----------------------- : %d\n",
       V_VOCAB_OVERWRITE);

  i = getoptpos("V_MODEL_LOAD", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_MODEL_LOAD = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Overwrite model file ---------------------------- : %d\n",
       V_MODEL_LOAD);

  i = getoptpos("V_PEEK_OVERWRITE", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_PEEK_OVERWRITE = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Overwrite peek file ----------------------------- : %d\n",
       V_PEEK_OVERWRITE);

  i = getoptpos("V_PEEK_SAMPLE_RATE", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_PEEK_SAMPLE_RATE = atof(argv[i + 1]);
  LOGC(1, c, 'k', "Peek Sampling Rate ------------------------------ : %lf\n",
       (double)V_PEEK_SAMPLE_RATE);

  i = getoptpos("V_PEEK_TOP_K", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_PEEK_TOP_K = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Peek Top K Words -------------------------------- : %d\n",
       V_PEEK_TOP_K);

  i = getoptpos("N", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) N = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "N -- word embedding dim ------------------------- : %d\n",
       N);

  i = getoptpos("V", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "V -- vocabulary size cap ------------------------ : %d\n",
       V);

  i = getoptpos("C", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) C = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "C -- context length ----------------------------- : %d\n",
       C);

  i = getoptpos("V_TRAIN_METHOD", argc, argv);
  if (i != -1) V_TRAIN_METHOD = sclone(argv[i + 1]);

  if (!strcmp(V_TRAIN_METHOD, "dcme")) {
    LOGC(1, 'g', 'k', "== DCME specific ==\n");

    i = getoptpos("V_OFFLINE_INTERVAL_VOCAB_RATIO", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_OFFLINE_INTERVAL_VOCAB_RATIO = atof(argv[i + 1]);
    LOGC(1, c, 'k', "Offline interval / online update / vocabulary size: %lf\n",
         (double)V_OFFLINE_INTERVAL_VOCAB_RATIO);

    i = getoptpos("V_MICRO_ME", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_MICRO_ME = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Micro ME for top words -------------------------- : %d\n",
         V_MICRO_ME);

    i = getoptpos("V_MICRO_ME_SCR_UPDATE", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_MICRO_ME_SCR_UPDATE = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Micro ME adjusted distribution for updateing scr  : %d\n",
         V_MICRO_ME_SCR_UPDATE);

    i = getoptpos("V_DUAL_RESET_OPT", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_DUAL_RESET_OPT = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Dual reset option ------------------------------- : %d\n",
         V_DUAL_RESET_OPT);

    i = getoptpos("V_DUAL_HI", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_DUAL_HI = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Dual update with hi not hh ---------------------- : %d\n",
         V_DUAL_HI);

    i = getoptpos("V_ADJUST_WW", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_ADJUST_WW = atof(argv[i + 1]);
    LOGC(1, c, 'k', "Adjust ww while online updating ----------------- : %lf\n",
         V_ADJUST_WW);

    i = getoptpos("K", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) K = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "K -- dual cluster number ------------------------ : %d\n",
         K);

    i = getoptpos("Q", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) Q = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Q -- micro maximal entropy size ----------------- : %d\n",
         Q);
  } else if (!strcmp(V_TRAIN_METHOD, "w2v")) {
    LOGC(1, 'g', 'k', "== W2V specific ==\n");

    i = getoptpos("V_NS_WRH", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_NS_WRH = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Use Walker's Robin Hood Sample method ----------- : %d\n",
         V_NS_WRH);

    i = getoptpos("V_NS_POWER", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_NS_POWER = atof(argv[i + 1]);
    LOGC(1, c, 'k', "Skew the unigram distribution with power -------- : %lf\n",
         (double)V_NS_POWER);

    i = getoptpos("V_NS_NEG", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_NS_NEG = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Negative Sample number of words ----------------- : %d\n",
         V_NS_NEG);

    i = getoptpos("V_NCE", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_NCE = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Use Negative Contrastive Estimation ------------- : %d\n",
         V_NCE);

    i = getoptpos("V_NSME", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_NSME = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Use Maximum Entropy ----------------------------- : %d\n",
         V_NCE);
  }

  LOGC(1, 'g', 'k', "== Sanity Checks ==\n");
  int x = 0;
  x = (NUP > N);
  LOG(1, "        NUP > N: %s (%d > %d)\n", x == 1 ? "yes" : "no", NUP, N);
  if (x == 0) {
    LOG(0, "fail!");
    exit(1);
  }
  x = (KUP > K);
  LOG(1, "        KUP > K: %s (%d > %d)\n", x == 1 ? "yes" : "no", KUP, K);
  if (x == 0) {
    LOG(0, "fail!");
    exit(1);
  }
  x = (VUP > V);
  LOG(1, "        VUP > V: %s (%d > %d)\n", x == 1 ? "yes" : "no", VUP, V);
  if (x == 0) {
    LOG(0, "fail!");
    exit(1);
  }
  if (!strcmp(V_TRAIN_METHOD, "dcme")) {
    x = (QUP > Q);
    LOG(1, "        QUP > Q: %s (%d > %d)\n", x == 1 ? "yes" : "no", QUP, Q);
    if (x == 0) {
      LOG(0, "fail!");
      exit(1);
    }
  }

  // use perm file instead of original
  /* V_TEXT_FILE_PATH = sformat("%s.perm", V_TEXT_FILE_PATH); */

  // build vocab if necessary, load, and set V by smaller actual size
  if (!fexists(V_VOCAB_FILE_PATH) || V_VOCAB_OVERWRITE) {
    vcb = TextBuildVocab(V_TEXT_FILE_PATH, V_TEXT_LOWER, V_TEXT_RM_TRAIL_PUNC,
                         -1);
    TextSaveVocab(V_VOCAB_FILE_PATH, vcb);
    VocabFree(vcb);
  }
  vcb = TextLoadVocab(V_VOCAB_FILE_PATH, V, V_VOCAB_HIGH_FREQ_CUTOFF);  // >>
  if (V != vcb->size) {
    LOGC(0, 'r', 'k', "[VOCAB]: overwrite V from %d to %d\n", V, vcb->size);
    V = vcb->size;
  }
  // load model if necessary,  otherwise init model with random values
  if (V_MODEL_LOAD && fexists(V_MODEL_SAVE_PATH)) {
    model = ModelLoad(V_MODEL_SAVE_PATH);
    if (N != model->n) {
      LOGC(0, 'r', 'k', "[MODEL]: overwrite N from %d to %d\n", N, model->n);
      N = model->n;
    }
    if (V != model->v) {
      LOGC(0, 'r', 'k', "[MODEL]: overwrite V from %d to %d\n", V, model->v);
      V = model->v;
    }
  } else
    model = ModelCreate(V, N, model_init_amp);  // >>
#ifdef DEBUG
  // build peek set if necessary, and load
  if (!fexists(V_PEEK_FILE_PATH) || V_PEEK_OVERWRITE) {
    ps = PeekBuild(V_TEXT_FILE_PATH, V_TEXT_LOWER, V_TEXT_RM_TRAIL_PUNC,
                   V_PEEK_SAMPLE_RATE, V_PEEK_TOP_K, vcb, V_THREAD_NUM);
    PeekSave(V_PEEK_FILE_PATH, vcb, ps);
    PeekSetFree(ps);
  }
  ps = PeekLoad(V_PEEK_FILE_PATH, vcb);  // >>
#endif
  gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE;                   // gd_ss
  progress = (real *)malloc(V_THREAD_NUM * sizeof(real));  // >>
  NumFillZeroVec(progress, V_THREAD_NUM);
  start_clock_t = clock();
  return;
}

#endif /* ifndef VARIABLES */
