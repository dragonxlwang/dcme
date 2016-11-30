#ifndef MODEL
#define MODEL

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../classify/constants.c"
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"

real *WeightCreate(int c, int n, real init_amp) {
  real *weight = NumNewHugeVec(c * n);
  if (init_amp >= 0) NumRandFillVec(weight, c * n, -init_amp, init_amp);
  return weight;
}

void WeightSave(real *w, int c, int n, int iter_num, char *fp) {
  // always rewrite the final model
  // intermediate models not written if files already exist
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
  fwrite(&c, sizeof(int), 1, fout);
  fwrite(&n, sizeof(int), 1, fout);
  fwrite(w, sizeof(real), c * n, fout);
  fclose(fout);
  free(mfp);
  if (iter_num == -1) {  // print only when saving final model
    LOGC(1, 'c', 'k', "[WEIGHT]: Save weight to %s\n", fp);
    LOGC(1, 'c', 'k', "[WEIGHT]: C = %d\n", c);
    LOGC(1, 'c', 'k', "[WEIGHT]: N = %d\n", n);
  }
  return;
}

real *WeightLoad(char *fp, int *cptr, int *nptr) {
  FILE *fin = fsopen(fp, "rb");
  int c, n;
  if (fread(&c, sizeof(int), 1, fin) != 1) {
    LOG(0, "WeightLoad Error c!\n");
    exit(1);
  }
  if (fread(&n, sizeof(int), 1, fin) != 1) {
    LOG(0, "WeightLoad Error n!\n");
    exit(1);
  }
  real *w = NumNewHugeVec(c * n);
  if (fread(w, sizeof(real), c * n, fin) != c * n) {
    LOG(0, "WeightLoad Error w!\n");
    exit(1);
  }
  *nptr = n;
  *cptr = c;
  LOGC(1, 'c', 'k', "[WEIGHT]: Load weight from %s\n", fp);
  LOGC(1, 'c', 'k', "[WEIGHT]: C = %d\n", c);
  LOGC(1, 'c', 'k', "[WEIGHT]: N = %d\n", n);
  fclose(fin);
  return w;
}

void WeightFree(real *w) {
  free(w);
  return;
}

void WeightVecRegularize(real *weight, int i, real proj_norm, real l2w, int n) {
  if (proj_norm >= 0) NumVecProjUnitBall(weight + i * n, proj_norm, n);
  if (l2w >= 0) NumVecMulC(weight + i * n, 1 - l2w, n);
  return;
}

real WeightTrainProgress(real *progress, int thread_num, int iter_num) {
  real p = NumSumVec(progress, thread_num);
  p /= (thread_num * iter_num);
  return p;
}

char *WeightDebugInfoStr(real *weight, int c, int n, real p, int tid,
                         clock_t start_clock_t, int thread_num, real gd_ss) {
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
  real nw = NumVecNorm(weight, c * n);
  real nwr = NumMatMaxRowNorm(weight, c, n);
  saprintf(str, "WEIGHT:%.2e=%.2e*", nw, nw / nwr);
  saprintfc(str, 'r', 'k', "%.2e", nwr);
#endif
  return str;
}

#endif /* ifndef MODEL */
