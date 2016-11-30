#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include "../vectors/variables.c"

Vocabulary* vcb;

void GetSentEmbd(int* wids, int wnum, real* embd) {
  int i;
  NumFillZeroVec(embd, N);
  for (i = 0; i < wnum; i++)
    NumVecAddCVec(embd, model->tar + wids[i] * N, 1.0 / wnum, N);
  return;
}

void EvalParaphrase() {
  char* EV_MSRPC_FILE_PATH =
      FilePathExpand("~/data/MSRPC/msr_paraphrase_train_3ln.txt");
  FILE* fin = fopen(EV_MSRPC_FILE_PATH, "r");
  int sent1[SUP], sent2[SUP], len1, len2, label, num1, num0;
  char str_label[SUP];
  real embd1[NUP], embd2[NUP], cos1, cos0, c;

  num1 = 0;
  num0 = 0;
  cos1 = 0;
  cos0 = 0;
  while (!feof(fin)) {
    fgets(str_label, SUP, fin);
    if (str_label[0] == '1')
      label = 1;
    else if (str_label[0] == '0')
      label = 0;
    else {
      printf("error!");
      exit(1);
    }
    len1 = TextReadSent(fin, vcb, sent1, V_TEXT_LOWER, V_TEXT_RM_TRAIL_PUNC, 1);
    len2 = TextReadSent(fin, vcb, sent2, V_TEXT_LOWER, V_TEXT_RM_TRAIL_PUNC, 1);
    GetSentEmbd(sent1, len1, embd1);
    GetSentEmbd(sent2, len2, embd2);
    c = NumVecCos(embd1, embd2, N);
    if (label == 0) {
      cos0 += c;
      num0++;
    } else {
      cos1 += c;
      num1++;
    }
  }
  LOG(0, "Class1: Average Cos = %lf / %d = %lf\n", cos1, num1, cos1 / num1);
  LOG(0, "Class0: Average Cos = %lf / %d = %lf\n", cos0, num0, cos0 / num0);
  return;
}

int main(int argc, char** argv) {
  VariableInit(argc, argv);
  NumInit();
  EvalParaphrase();
}
