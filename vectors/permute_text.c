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
#include "variables.c"

Vocabulary* vcb;
int** lwids;
int* lwnum;
long int num;
int cap = 0xFFFFF;
void permute() {
  int i, j, k;
  int wids[SUP], wnum;
  FILE* fin = fopen(V_TEXT_FILE_PATH, "rb");
  if (!fin) {
    LOG(0, "Error!\n");
    exit(1);
  }
  lwids = (int**)malloc(cap * sizeof(int*));
  lwnum = (int*)malloc(cap * sizeof(int));
  while (!feof(fin)) {
    wnum = TextReadSent(fin, vcb, wids, V_TEXT_LOWER, V_TEXT_RM_TRAIL_PUNC, 1);
    lwids[num] = NumCloneIntVec(wids, wnum);
    lwnum[num] = wnum;
    num++;
    if (num == cap) {
      cap *= 2;
      lwids = (int**)realloc(lwids, cap * sizeof(int*));
      lwnum = (int*)realloc(lwnum, cap * sizeof(int));
    }
  }
  int* idx = range(num);
  NumPermuteIntVec(idx, num, 10.0);
  printf("reading %ld sentences\n", num);
  fclose(fin);
  FILE* fout = fopen(sformat("%s.perm", V_TEXT_FILE_PATH), "wb");
  for (i = 0; i < num; i++) {
    j = idx[i];
    for (k = 0; k < lwnum[j]; k++)
      fprintf(fout, "%s ", VocabGetWord(vcb, lwids[j][k]));
  }
  fclose(fout);
  return;
}

int main(int argc, char** argv) {
  VariableInit(argc, argv);
  NumInit();
  permute();
}
