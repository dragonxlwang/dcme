#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../classify/test.c"
#include "../classify/variables.c"
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"

void EvalClassify() {
  int fsv[LUP], fn, label, correct = 0, total = 0, x;
  real p, totalp = 0;
  long int fsz = FileSize(V_TEST_FILE_PATH);
  FILE* fin = fsopen(V_TEST_FILE_PATH, "rb");
  int* detail_correct = NumNewHugeIntVec(C);
  int* detail_total = NumNewHugeIntVec(C);
  NumFillZeroIntVec(detail_correct, C);
  NumFillZeroIntVec(detail_total, C);
  while (HelperReadInstance(fin, vcb, classes, fsv, &fn, &label, V_TEXT_LOWER,
                            V_TEXT_RM_TRAIL_PUNC)) {
    if (label < 0) continue;  // filtering classes not shown in training data
    x = TestClassify(fsv, fn, weight, C, N, label, &p);
    detail_total[label]++;
    detail_correct[label] += x;
    correct += x;
    totalp += p;
    total++;
    if (total % 10 == 0) {
      LOGCLR(2);
      LOG(2, "Completing %.2lf%%", (double)ftell(fin) / fsz * 100);
    }
  }
  LOGCR(2);
  printf("correct     = %d\n", correct);
  printf("total       = %d\n", total);
  printf("accuracy    = %lf\n", (double)correct / total);
  printf("probability = %lf\n", totalp / total);

#ifdef DEBUG
  int i, j;
  pair* detail_tuple = sortedi(detail_total, C, 1);
  for (j = 0; j < C; j++) {
    i = detail_tuple[j].key;
    if (detail_total[i] >= 50)
      printf("%20s: %15d / %-15d = %-15lf\n", DictGetKey(classes, i),
             detail_correct[i], detail_total[i],
             (double)detail_correct[i] / detail_total[i]);
  }
  free(detail_tuple);
#endif
  free(detail_correct);
  free(detail_total);
  fclose(fin);
}

int main(int argc, char** argv) {
  V_WEIGHT_LOAD = 1;
  NumInit();
  VariableInit(argc, argv);
  EvalClassify();
  return 0;
}
