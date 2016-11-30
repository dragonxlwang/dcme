#ifndef VARIABLES
#define VARIABLES

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../classify/constants.c"
#include "../classify/helper.c"
#include "../classify/weight.c"
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"

// ---------------------------- Config Variables ------------------------------
char *V_WEIGHT_DECOR_FILE_PATH = NULL;
char *V_TRAIN_FILE_PATH = "~/data/acm_corpus/proc.train";
char *V_TEST_FILE_PATH = NULL;
char *V_VOCAB_FILE_PATH = NULL;  // don't set it if can be inferred from above
char *V_WEIGHT_SAVE_PATH = NULL;

int V_THREAD_NUM = 20;
int V_ITER_NUM = 100;

// Initial grad descent step size
real V_INIT_GRAD_DESCENT_STEP_SIZE = 1e-4;
// Weight Shrink: l-2 regularization:
real V_L2_REGULARIZATION_WEIGHT = -1;  // 1e-3;
// Weight Shrink: if proj weight to ball with specified norm, -1 if not proj
real V_WEIGHT_PROJ_BALL_NORM = -1;
// Vocab loading option: cut-off high frequent (stop) words
int V_VOCAB_HIGH_FREQ_CUTOFF = 0;
// if convert uppercase to lowercase
int V_TEXT_LOWER = 1;
// if remove trailing punctuation
int V_TEXT_RM_TRAIL_PUNC = 1;
// if cache weight per V_CACHE_INTERMEDIATE_WEIGHT iteration
int V_CACHE_INTERMEDIATE_WEIGHT = 0;
// if overwrite vocab file
int V_VOCAB_OVERWRITE = 0;
// if load weight from file instead of random initiailization
int V_WEIGHT_LOAD = 0;

int N = 30000;   // (max) feature weight dimension
int C = 100000;  // (max) class number
// method
char *V_TRAIN_METHOD = "dcme";

// ----------------------------- DCME specific --------------------------------
// every this number times  online updates perform one offline update
real V_OFFLINE_INTERVAL_CLASS_RATIO = 1;
// if using micro maximal entropy for top words plus target words
int V_MICRO_ME = 0;
// Dual Reset option
int V_DUAL_RESET_OPT = 1;
// number of threads sharing one dual bookkeeping
int V_THREADS_PER_DUAL = 1;
int K = 20;  // number of dual cluster
int Q = 10;  // Number of top words in online update

// ----------------------------- W2V/NSME specific ----------------------------
int V_NS_WRH = 1;
real V_NS_POWER = 0.75;
int V_NS_NEG = 5;
int V_NCE = 0;
int V_ME_TOP = 0;

// ---------------------------- global variables ------------------------------
Vocabulary *vcb;  // vocabulary
real *weight;     // weight
real gd_ss;       // gradient descent step
real *progress;   // thread progress
long int *fpos_beg, *fpos_end, file_size;
Dictionary *classes;

clock_t start_clock_t;  // start time for weight training

// ------------------------ global private variables --------------------------
real weight_init_amp = 0.01;  // weight init: large value for random init
typedef void *(*ThreadTrain)(void *);
typedef void (*MainWork)();

void VariableFree() {
  free(weight);
  VocabFree(vcb);  // <<
  free(progress);  // <<
  free(fpos_beg);
  free(fpos_end);
}

void VariableInit(int argc, char **argv) {
  int i;
  char c = 'w';

  i = getoptpos("V_WEIGHT_DECOR_FILE_PATH", argc, argv);
  if (i != -1) {
    V_WEIGHT_DECOR_FILE_PATH = sclone(argv[i + 1]);
    LOGC(1, 'g', 'k', "%s",
         sformat("== Config: %s ==\n", V_WEIGHT_DECOR_FILE_PATH));
  } else
    LOGC(1, 'g', 'k', "== Config ==\n");

  i = getoptpos("V_TRAIN_FILE_PATH", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_TRAIN_FILE_PATH = sclone(argv[i + 1]);
  V_TRAIN_FILE_PATH = FilePathExpand(V_TRAIN_FILE_PATH);
  LOGC(1, c, 'k', "Training File ----------------------------------- : %s\n",
       V_TRAIN_FILE_PATH);

  i = getoptpos("V_TEST_FILE_PATH", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1)
    V_TEST_FILE_PATH = sclone(argv[i + 1]);
  else
    V_TEST_FILE_PATH = sreplace(V_TRAIN_FILE_PATH, "train", "test");
  V_TEST_FILE_PATH = FilePathExpand(V_TEST_FILE_PATH);
  LOGC(1, c, 'k', "Test File --------------------------------------- : %s\n",
       V_TEST_FILE_PATH);

  i = getoptpos("V_VOCAB_FILE_PATH", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_VOCAB_FILE_PATH = sclone(argv[i + 1]);
  i = getoptpos("V_TEXT_LOWER", argc, argv);
  if (i != -1) {
    V_TEXT_LOWER = atoi(argv[i + 1]);
    c = 'r';
  }
  i = getoptpos("V_TEXT_RM_TRAIL_PUNC", argc, argv);
  if (i != -1) {
    V_TEXT_RM_TRAIL_PUNC = atoi(argv[i + 1]);
    c = 'r';
  }
  if (!V_VOCAB_FILE_PATH)
    V_VOCAB_FILE_PATH = FilePathSubExtension(
        V_TRAIN_FILE_PATH,
        sformat("l%dr%d.vcb", V_TEXT_LOWER, V_TEXT_RM_TRAIL_PUNC));
  else
    V_VOCAB_FILE_PATH = FilePathExpand(V_VOCAB_FILE_PATH);
  LOGC(1, c, 'k', "Vocab File -------------------------------------- : %s\n",
       V_VOCAB_FILE_PATH);

  i = getoptpos("V_WEIGHT_SAVE_PATH", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_WEIGHT_SAVE_PATH = sclone(argv[i + 1]);
  if (!V_WEIGHT_SAVE_PATH) {
    if (V_WEIGHT_DECOR_FILE_PATH) {
      V_WEIGHT_SAVE_PATH = FilePathSubExtension(
          V_TRAIN_FILE_PATH, sformat("%s.wgt", V_WEIGHT_DECOR_FILE_PATH));
      c = 'r';
    } else
      V_WEIGHT_SAVE_PATH = FilePathSubExtension(V_TRAIN_FILE_PATH, "wgt");
  } else
    V_WEIGHT_SAVE_PATH = FilePathExpand(V_WEIGHT_SAVE_PATH);
  LOGC(1, c, 'k', "Weight File ------------------------------------- : %s\n",
       V_WEIGHT_SAVE_PATH);

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

  i = getoptpos("V_WEIGHT_PROJ_BALL_NORM", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_WEIGHT_PROJ_BALL_NORM = atof(argv[i + 1]);
  LOGC(1, c, 'k', "Project Weight Inside Ball with Norm ------------ : %lf\n",
       (double)V_WEIGHT_PROJ_BALL_NORM);

  i = getoptpos("V_VOCAB_HIGH_FREQ_CUTOFF", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_VOCAB_HIGH_FREQ_CUTOFF = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Vocabulary High-Freq Words Cut-Off -------------- : %d\n",
       V_VOCAB_HIGH_FREQ_CUTOFF);

  i = getoptpos("V_TEXT_LOWER", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_TEXT_LOWER = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Convert Uppercase to Lowercase Letters ---------- : %d\n",
       V_TEXT_LOWER);

  i = getoptpos("V_TEXT_RM_TRAIL_PUNC", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_TEXT_RM_TRAIL_PUNC = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Remove Trailing Punctuation --------------------- : %d\n",
       V_TEXT_RM_TRAIL_PUNC);

  i = getoptpos("V_CACHE_INTERMEDIATE_WEIGHT", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_CACHE_INTERMEDIATE_WEIGHT = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Cache Intermediate Weight ----------------------- : %d\n",
       V_CACHE_INTERMEDIATE_WEIGHT);

  i = getoptpos("V_VOCAB_OVERWRITE", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_VOCAB_OVERWRITE = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Overwrite Vocabulary File ----------------------- : %d\n",
       V_VOCAB_OVERWRITE);

  i = getoptpos("V_WEIGHT_LOAD", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_WEIGHT_LOAD = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "Load Weight from File for Initialization -------- : %d\n",
       V_WEIGHT_LOAD);

  i = getoptpos("N", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) N = atoi(argv[i + 1]);
  LOGC(1, c, 'k', "N -- Feature Dimension -------------------------- : %d\n",
       N);

  i = getoptpos("V_TRAIN_METHOD", argc, argv);
  c = (i == -1) ? 'w' : 'r';
  if (i != -1) V_TRAIN_METHOD = sclone(argv[i + 1]);
  LOGC(1, c, 'k', "Train Mehod ------------------------------------- : %s\n",
       V_TRAIN_METHOD);

  if (!strcmp(V_TRAIN_METHOD, "dcme")) {
    LOGC(1, 'g', 'k', "== DCME specific ==\n");

    i = getoptpos("V_OFFLINE_INTERVAL_CLASS_RATIO", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_OFFLINE_INTERVAL_CLASS_RATIO = atof(argv[i + 1]);
    LOGC(1, c, 'k', "Offline Interval / Online Update / Class Number - : %lf\n",
         (double)V_OFFLINE_INTERVAL_CLASS_RATIO);

    i = getoptpos("V_MICRO_ME", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_MICRO_ME = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Micro ME for Top Words -------------------------- : %d\n",
         V_MICRO_ME);

    i = getoptpos("V_DUAL_RESET_OPT", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_DUAL_RESET_OPT = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Dual Reset Option ------------------------------- : %d\n",
         V_DUAL_RESET_OPT);

    i = getoptpos("V_THREADS_PER_DUAL", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_THREADS_PER_DUAL = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Number of Threads Sharing a Dual Clustering ----- : %d\n",
         V_THREADS_PER_DUAL);

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
  } else if (!strcmp(V_TRAIN_METHOD, "w2v") ||
             !strcmp(V_TRAIN_METHOD, "nsme")) {
    LOGC(1, 'g', 'k', "== W2V/NSME specific ==\n");

    i = getoptpos("V_NS_WRH", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_NS_WRH = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Use Walker's Robin Hood Sample method ----------- : %d\n",
         V_NS_WRH);

    i = getoptpos("V_NS_POWER", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_NS_POWER = atof(argv[i + 1]);
    LOGC(1, c, 'k', "Skew the Unigram Distribution with Power -------- : %lf\n",
         (double)V_NS_POWER);

    i = getoptpos("V_NS_NEG", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_NS_NEG = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Negative Sample Number of Words ----------------- : %d\n",
         V_NS_NEG);

    i = getoptpos("V_NCE", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_NCE = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Use Negative Contrastive Estimation ------------- : %d\n",
         V_NCE);

    i = getoptpos("V_ME_TOP", argc, argv);
    c = (i == -1) ? 'w' : 'r';
    if (i != -1) V_ME_TOP = atoi(argv[i + 1]);
    LOGC(1, c, 'k', "Maximal Entropy Constrained to Top Classes ------ : %d\n",
         V_ME_TOP);
  }

  LOGC(1, 'g', 'k', "== Sanity Checks ==\n");
  int x = 0;
  x = (NUP > N);
  LOG(1, "        NUP > N: %s (%d > %d)\n", x == 1 ? "yes" : "no", NUP, N);
  if (x == 0) {
    LOG(0, "fail!");
    exit(1);
  }
  if (!strcmp(V_TRAIN_METHOD, "dcme")) {
    x = (KUP > K);
    LOG(1, "        KUP > K: %s (%d > %d)\n", x == 1 ? "yes" : "no", KUP, K);
    if (x == 0) {
      LOG(0, "fail!");
      exit(1);
    }
    x = (QUP > Q);
    LOG(1, "        QUP > Q: %s (%d > %d)\n", x == 1 ? "yes" : "no", QUP, Q);
    if (x == 0) {
      LOG(0, "fail!");
      exit(1);
    }
  }
  if (!strcmp(V_TRAIN_METHOD, "nsme")) {
    x = (QUP > V_NS_NEG);
    LOG(1, "        QUP > V_NS_NEG: %s (%d > %d)\n", x == 1 ? "yes" : "no", QUP,
        V_NS_NEG);
    if (x == 0) {
      LOG(0, "fail!");
      exit(1);
    }
    x = (QUP > V_ME_TOP);
    LOG(1, "        QUP > V_ME_TOP: %s (%d > %d)\n", x == 1 ? "yes" : "no", QUP,
        V_ME_TOP);
    if (x == 0) {
      LOG(0, "fail!");
      exit(1);
    }
    x = (CUP > C);
    LOG(1, "        CUP > C: %s (%d > %d)\n", x == 1 ? "yes" : "no", CUP, C);
    if (x == 0) {
      LOG(0, "fail!");
      exit(1);
    }
  }

  // build vocab if necessary, load, and set V by smaller actual size
  if (!fexists(V_VOCAB_FILE_PATH) || V_VOCAB_OVERWRITE) {
    vcb = HelperBuildVocab(V_TRAIN_FILE_PATH, V_TEXT_LOWER,
                           V_TEXT_RM_TRAIL_PUNC, -1);
    TextSaveVocab(V_VOCAB_FILE_PATH, vcb);
    VocabFree(vcb);
  }
  vcb = TextLoadVocab(V_VOCAB_FILE_PATH, N, V_VOCAB_HIGH_FREQ_CUTOFF);  // >>
  if (vcb->size != N) {
    LOGC(0, 'r', 'k', "[VARIABLES]: overwrite N from %d to %d\n", N, vcb->size);
    N = vcb->size;
  }
  // load weight if necessary,  otherwise init weight with random values

  classes = HelperBuildClasses(V_TRAIN_FILE_PATH);
  if (classes->size != C) {
    LOGC(0, 'r', 'k', "[VARIABLES]: overwrite C from %d to %d\n", C,
         classes->size);
    C = SMALLER(C, classes->size);
  }

  int weightc, weightn;
  if (V_WEIGHT_LOAD && fexists(V_WEIGHT_SAVE_PATH)) {
    weight = WeightLoad(V_WEIGHT_SAVE_PATH, &weightc, &weightn);
    if (weightc != C) {
      LOGC(0, 'r', 'k', "[WEIGHT]: overwrite C from %d to %d\n", C, weightc);
      C = weightc;
    }
    if (weightn != N) {
      LOGC(0, 'r', 'k', "[WEIGHT]: overwrite N from %d to %d\n", N, weightn);
      N = weightn;
    }
  } else
    weight = WeightCreate(C, N, weight_init_amp);  // >>

  gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE;                   // gd_ss
  progress = (real *)malloc(V_THREAD_NUM * sizeof(real));  // >>
  fpos_beg = (long int *)malloc(V_THREAD_NUM * sizeof(long int));
  fpos_end = (long int *)malloc(V_THREAD_NUM * sizeof(long int));
  file_size = FileSize(V_TRAIN_FILE_PATH);
  for (i = 0; i < V_THREAD_NUM; i++) {
    fpos_beg[i] = file_size / V_THREAD_NUM * i;
    fpos_end[i] = file_size / V_THREAD_NUM * (i + 1);
    if (i != 0)
      fpos_beg[i] = HelperLocateNextInstance(V_TRAIN_FILE_PATH, fpos_beg[i]);
    if (i != V_THREAD_NUM - 1)
      fpos_end[i] = HelperLocateNextInstance(V_TRAIN_FILE_PATH, fpos_end[i]);
  }
  NumFillZeroVec(progress, V_THREAD_NUM);
  start_clock_t = clock();
  return;
}

#endif /* ifndef VARIABLES */
