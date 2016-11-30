#ifndef CASSIFY_TEST
#define CASSIFY_TEST

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

void TestCompDist(int *fsv, int fn, real *weight, int c, int n, real *prob) {
  int i;
  for (i = 0; i < c; i++) prob[i] = NumSvSum(fsv, fn, weight + i * n);
  NumSoftMax(prob, 1, c);
  return;
}

int TestClassify(int *fsv, int fn, real *weight, int c, int n, int label,
                 real *p_ptr) {
  real *p = (real *)malloc(c * sizeof(real));
  TestCompDist(fsv, fn, weight, c, n, p);
  *p_ptr = p[label];
  int i;
  for (i = 0; i < c; i++)
    if (i != label && p[label] < p[i]) {
      free(p);
      return 0;
    }
  free(p);
  return 1;
}

#endif /* ifndef CASSIFY_TEST */
