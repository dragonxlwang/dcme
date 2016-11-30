#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include "../vectors/variables.c"

void EvalWordDistance(real* e, Vocabulary* vcb, char* sim_method) {
  int i;
  char word[WUP];
  int wid;
  real* sim = (real*)malloc(V * sizeof(real));
  pair* p;
  while (1) {
    LOG(0, "Enter a word (EXIT to break): ");
    scanf("%s", word);
    if (!strcmp(word, "EXIT")) return;
    for (i = 0; i < strlen(word); i++) word[i] = LOWER(word[i]);
    wid = VocabGetId(vcb, word);
    if (wid == -1) {
      LOGC(0, 'r', 'k', "word %s not in vocabulary\n", word);
      continue;
    }
    for (i = 0; i < V; i++) {
      if (i != wid) {
        if (!strcmp(sim_method, "cosine"))
          sim[i] = NumVecCos(e + i * N, e + wid * N, N);
        else if (!strcmp(sim_method, "l2-dist"))
          sim[i] = -NumVecL2Dist(e + i * N, e + wid * N, N);
      } else
        sim[i] = -1e35;
    }
    p = sorted(sim, V, 1);
    LOGC(0, 'c', 'k', "\t\t\t\tword\t%s\t\tid\tfreq\tnorm\n", sim_method);
    for (i = 0; i < 80; i++) LOGC(0, 'r', 'k', "=");
    LOG(0, "\n");
    LOGC(0, 'c', 'k', "----  %30s\t----------\t%d\t%d\t%.2e\n", word, wid,
         vcb->id2cnt[wid], NumVecNorm(e + wid * N, N));
    for (i = 0; i < 40; i++)
      LOG(0, "[%02d]: %30s\t%.4e\t%d\t%d\t%.2e\n", i,
          VocabGetWord(vcb, p[i].key), p[i].val, p[i].key,
          vcb->id2cnt[p[i].key], NumVecNorm(e + p[i].key * N, N));
    LOG(0, "\n");
    free(p);
  }
  free(sim);
  return;
}

int main(int argc, char** argv) {
  V_MODEL_LOAD = 1;
  NumInit();
  VariableInit(argc, argv);
  EvalWordDistance(model->scr, vcb, "cosine");
  /* EvalWordDistance(model->tar, vcb, "l2-dist"); */
  return 0;
}
