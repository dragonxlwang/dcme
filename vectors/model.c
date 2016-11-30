#ifndef MODEL
#define MODEL

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

typedef struct Model {  // model parameter is shared across all threads
  real *scr;            // source vector;
  real *tar;            // target vector;
  int v;                // vocabulary size;
  int n;                // vocabulary dimension;
} Model;

Model *ModelCreate(int v, int n, real init_amp) {
  Model *model = (Model *)malloc(sizeof(Model));
  model->scr = NumNewHugeVec(v * n);
  model->tar = NumNewHugeVec(v * n);
  model->v = v;
  model->n = n;
  if (init_amp >= 0) {
    NumRandFillVec(model->scr, v * n, -init_amp, init_amp);
    // instead of using random inititalization for model->tar,
    // set all values to 0 the same as word2vec
    /* NumRandFillVec(model->tar, v * n, -init_amp, init_amp); */
    NumFillZeroVec(model->tar, v * n);
  }
  real scr = NumVecNorm(model->scr, model->v * model->n);
  real ss = NumMatMaxRowNorm(model->scr, model->v, model->n);
  real tar = NumVecNorm(model->tar, model->v * model->n);
  real tt = NumMatMaxRowNorm(model->tar, model->v, model->n);
  LOG(2, "[Model]: Init ");                 // scr
  LOG(2, "SCR:%.2e=%.2e*", scr, scr / ss);  // scr
  LOGC(0, 'r', 'k', "%.2e", ss);            // ss
  LOG(2, " ");                              //
  LOG(2, "TAR=%.2e=%.2e*", tar, tar / tt);  // tar
  LOGC(2, 'r', 'k', "%.2e", tt);            // tt
  LOGCR(2);
  return model;
}

void ModelSave(Model *model, int iter_num, char *fp) {
  char *mfp;
  if (iter_num == -1) {  // save final model, call by master
    mfp = sclone(fp);    // so that free(mfp) can work
  } else {               // avoid thread racing
    char *mdp = sformat("%s.dir", fp);
    if (!direxists(mdp)) dirmake(mdp);
    mfp = sformat("%s/%d.iter", mdp, iter_num);
    free(mdp);
    if (fexists(mfp)) return;
  }
  FILE *fout = fopen(mfp, "wb");
  if (!fout) {
    LOG(0, "Error!\n");
    exit(1);
  }
  fwrite(&model->v, sizeof(int), 1, fout);
  fwrite(&model->n, sizeof(int), 1, fout);
  fwrite(model->scr, sizeof(real), model->v * model->n, fout);
  fwrite(model->tar, sizeof(real), model->v * model->n, fout);
  fclose(fout);
  free(mfp);
  if (iter_num == -1) {  // print only when saving final model
    LOGC(1, 'c', 'k', "[MODEL]: Save model to %s\n", fp);
    LOGC(1, 'c', 'k', "[MODEL]: Model V = %d, N = %d\n", model->v, model->n);
  }
  return;
}

Model *ModelLoad(char *fp) {
  FILE *fin = fopen(fp, "rb");
  if (!fin) {
    LOG(0, "Error!\n");
    exit(1);
  }
  int v, n;
  if (fread(&v, sizeof(int), 1, fin) != 1) {
    LOG(0, "Error!\n");
    exit(1);
  }
  if (fread(&n, sizeof(int), 1, fin) != 1) {
    LOG(0, "Error!\n");
    exit(1);
  }
  Model *model = ModelCreate(v, n, -1);
  if (fread(model->scr, sizeof(real), v * n, fin) != v * n) {
    LOG(0, "Error!\n");
    exit(1);
  }
  if (fread(model->tar, sizeof(real), v * n, fin) != v * n) {
    LOG(0, "Error!\n");
    exit(1);
  }
  LOGC(1, 'c', 'k', "[MODEL]: Load model from %s\n", fp);
  LOGC(1, 'c', 'k', "[MODEL]: Model V = %d, N = %d ", model->v, model->n);
  real scr = NumVecNorm(model->scr, model->v * model->n);
  real ss = NumMatMaxRowNorm(model->scr, model->v, model->n);
  real tar = NumVecNorm(model->tar, model->v * model->n);
  real tt = NumMatMaxRowNorm(model->tar, model->v, model->n);
  LOG(2, "SCR:%.2e=%.2e*", scr, scr / ss);  // scr
  LOGC(0, 'r', 'k', "%.2e", ss);            // ss
  LOG(2, " ");                              //
  LOG(2, "TAR=%.2e=%.2e*", tar, tar / tt);  // tar
  LOGC(2, 'r', 'k', "%.2e", tt);            // tt
  LOGCR(2);
  return model;
}

void ModelFree(Model *model) {
  free(model->scr);
  free(model->tar);
  free(model);
  return;
}

void ModelGradUpdate(Model *model, int p, int i, real c, real *g) {
  // model minimization update: p=0: scr; p=1: tar
  if (p == 0)
    NumVecAddCVec(model->scr + i * model->n, g, -c, model->n);  // update scr
  else
    NumVecAddCVec(model->tar + i * model->n, g, -c, model->n);  // update tar
  return;
}

void ModelVecRegularize(Model *model, int p, int i, real proj_norm, real l2w) {
  if (p == 0) {  // scr
    if (proj_norm >= 0)
      NumVecProjUnitBall(model->scr + i * model->n, proj_norm, model->n);
    if (l2w >= 0) NumVecMulC(model->scr + i * model->n, 1 - l2w, model->n);
  } else {  // tar
    if (proj_norm >= 0)
      NumVecProjUnitBall(model->tar + i * model->n, proj_norm, model->n);
    if (l2w >= 0) NumVecMulC(model->tar + i * model->n, 1 - l2w, model->n);
  }
  return;
}

void ModelShrink(Model *model, real l2w) {
  NumVecMulC(model->scr, 1 - l2w, model->v * model->n);
  NumVecMulC(model->tar, 1 - l2w, model->v * model->n);
  return;
}

real ModelTrainProgress(real *progress, int thread_num, int iter_num) {
  real p = NumSumVec(progress, thread_num);
  p /= (thread_num * iter_num);
  return p;
}

char *ModelDebugInfoStr(Model *model, real p, int tid, clock_t start_clock_t,
                        int thread_num, real gd_ss) {
  char *str = malloc(0x1000);
  int i, bar_len = 10;
  clock_t cur_clock_t = clock();
  real pct = p * 100;
  int bar = p * bar_len;
  double st = (double)(cur_clock_t - start_clock_t) / CLOCKS_PER_SEC;
  char *ht = strtime(st / thread_num);
  sprintfc(str, 'y', 'k', "[%7.4lf%%]: ", pct);             // percentage
  for (i = 0; i < bar; i++) saprintfc(str, 'r', 'k', "+");  // bar: past
  saprintfc(str, 'w', 'k', "~");                            // bar: current
  for (i = bar + 1; i < bar_len; i++)                       //
    saprintfc(str, 'c', 'k', "=");                          // bar: left
  saprintf(str, " ");                                       //
  saprintfc(str, 'g', 'k', "(tid=%02d)", tid);              // tid
  saprintf(str, " ");                                       //
  saprintf(str, "TIME:%.2e/%s ", st, ht);                   // time
  saprintf(str, "GDSS:%.4e ", gd_ss);                       // gdss
  free(ht);
#ifdef DEBUG
  real scr = NumVecNorm(model->scr, model->v * model->n);
  real ss = NumMatMaxRowNorm(model->scr, model->v, model->n);
  real tar = NumVecNorm(model->tar, model->v * model->n);
  real tt = NumMatMaxRowNorm(model->tar, model->v, model->n);
  saprintf(str, "SCR:%.2e=%.2e*", scr, scr / ss);  // scr
  saprintfc(str, 'r', 'k', "%.2e", ss);            // ss
  saprintf(str, " ");                              //
  saprintf(str, "TAR=%.2e=%.2e*", tar, tar / tt);  // tar
  saprintfc(str, 'r', 'k', "%.2e", tt);            // tt
  saprintf(str, " ");                              //
#endif
  return str;
}

// For compatibility, load w2v model (and vocabulary) from word2vec code
void ModelLoadW2v(char *fp, Model **mptr, Vocabulary **vptr) {
  // fp: "~/workspace/w2v/giga_vectors.bin";
  int i, j, k;
  char *file_name = FilePathExpand(fp);
  char word[WUP];
  FILE *fin1 = fopen(sformat("%s.syn0", file_name), "rb");
  FILE *fin2 = fopen(sformat("%s.syn1neg", file_name), "rb");
  if (fin1 == NULL || fin2 == NULL) {
    printf("Input file not found\n");
    exit(1);
  }
  int v, n;
  fscanf(fin1, "%d", &v);
  fscanf(fin1, "%d", &n);
  fscanf(fin2, "%d", &i);
  fscanf(fin2, "%d", &i);
  Vocabulary *vcb = VocabCreate(v);
  Model *model = ModelCreate(v, n, -1);
  *mptr = model;
  *vptr = vcb;
  float vec[NUP];
  int dup_cnt = 0;
  for (i = 0; i < v; i++) {
    j = 0;  // read words
    while (1) {
      word[j] = fgetc(fin1);
      fgetc(fin2);
      if (feof(fin1) || (word[j] == ' ')) break;
      if ((j < WUP) && (word[j] != '\n')) j++;
    }
    word[j] = 0;
    slower(word);
    if (VocabGetId(vcb, word) != -1) {  // ignore case, take lemma with larger f
      dup_cnt++;
      fread(vec, sizeof(float), n, fin1);
      fread(vec, sizeof(float), n, fin2);
      continue;
    } else
      j = VocabAdd(vcb, word, 1);
    fread(vec, sizeof(float), n, fin1);
    for (k = 0; k < n; k++) model->scr[j * n + k] = vec[k];
    fread(vec, sizeof(float), n, fin2);
    for (k = 0; k < n; k++) model->tar[j * n + k] = vec[k];
  }
  model->v = vcb->size;
  fclose(fin1);
  fclose(fin2);
  LOGC(1, 'c', 'k', "[MODEL-W2V]: Load model from %s\n", file_name);
  LOGC(1, 'c', 'k', "[MODEL-W2V]: Model V = %d (duplicate %d), N = %d\n",
       model->v, dup_cnt, model->n);
}

#endif /* ifndef MODEL */
