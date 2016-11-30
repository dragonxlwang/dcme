#ifndef UTIL_NUM
#define UTIL_NUM

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util_misc.c"

///////////////////////
// Numeric Functions //
///////////////////////

// Parameters:
//  NUM_EXP_TABLE_LEN          : exp table size
//  NUM_EXP_HIGH               : exp table high end
//  NUM_EXP_LOW                : exp table low end
//  NUM_MAX_PRINT_ELEM         : number of printable element in an array
//  NUM_EPS                    : machine precision
// Init:
//  NumInit()

#define NUM_EXP_TABLE_LEN 0xFFFFF
real NUM_EXP_TABLE[NUM_EXP_TABLE_LEN];
real NUM_EXP_HIGH = 1.0;
real NUM_EXP_LOW = -15;
real NumExp(real x) {
  // output exp(x) for -NUM_EXP_LOW <= x <= NUM_EXP_HIGH
  int i = (x - NUM_EXP_LOW) / (NUM_EXP_HIGH - NUM_EXP_LOW) * NUM_EXP_TABLE_LEN;
  if (i < 0)
    return 0;
  else
    return NUM_EXP_TABLE[i];
}

real NumRandNext(unsigned long *seed) {
  *seed = (((*seed) * 0x5DEECE66DL + 0xBL) & 0xFFFFFFFFFFFFL);
  return ((real)((*seed) >> 16) / ((real)0xFFFFFFFFL));
}

unsigned long RANDOM_SEED = 0x0F0F0F0FL;
real NumRand() {
  // return a random number range from [0, 1)
  RANDOM_SEED = ((RANDOM_SEED * 0x5DEECE66DL + 0xBL) & 0xFFFFFFFFFFFFL);
  return ((real)(RANDOM_SEED >> 16) / ((real)0xFFFFFFFFL));
}

void NumRandFillVec(real *arr, int l, real lb, real ub) {
  int i;
  for (i = 0; i < l; i++) arr[i] = lb + NumRand() * (ub - lb);
  return;
}

void NumPermuteIntVec(int *arr, int l, real permute_ratio) {
  // pairwise permute array l * permute_ratio times
  int cnt = permute_ratio * l;
  int i, j;
  while (cnt-- > 0) {
    i = NumRand() * l;
    j = NumRand() * (l - 1);
    if (j >= i) j++;
    SWAP(arr[i], arr[j]);
  }
  return;
}

int NUM_MAX_PRINT_ELEM = 10;
real NUM_EPS = 1e-6;

int NumEqual(real x, real y) { return ABS(x - y) <= NUM_EPS; }

int NumIsNan(real x) { return (x != x); }
void NumPrintArr(char *name, real *arr, int l) {
  int i = 0;
  char name10[11] = {0};
  for (i = 0; i < 10; i++) {
    if (i < strlen(name))
      name10[i] = name[i];
    else
      name10[i] = ' ';
  }
  printfc('g', 'k', "%10s: ", name10);
  int abbrv = 0;
  for (i = 0; i < l; i++)
    if (i < NUM_MAX_PRINT_ELEM - 2 || l - i <= 2) {
      if (-NUM_EPS <= arr[i] && arr[i] <= NUM_EPS)
        printf("%12.6g ", 0.0);
      else
        printf("%12.6g ", arr[i]);
    } else if (!abbrv) {
      abbrv = 1;
      printf("     ...     ");
    }
  printf("\n");
  fflush(stdout);
  return;
}

void NumPrintAllArr(real *arr, int l) {
  int i = 0;
  for (i = 0; i < l; i++) printf("%12.6g ", arr[i]);
  printf("\n");
  fflush(stdout);
  return;
}

void NumPrintMatrix(char *name, real *arr, int m, int n) {
  int i;
  int abbrv = 0;
  for (i = 0; i < m; i++) {
    if (i < NUM_MAX_PRINT_ELEM - 2 || m - i <= 2) {
      if (i == 0)
        NumPrintArr(name, arr, n);
      else
        NumPrintArr("", arr + i * n, n);
    } else if (!abbrv) {
      abbrv = 1;
      printf(" ... \n");
    }
  }
  fflush(stdout);
  return;
}

void NumPrintArrAbsMaxColor(char *name, real *arr, int l) {
  int i = 0;
  char name10[11] = {0};
  for (i = 0; i < 10; i++) {
    if (i < strlen(name))
      name10[i] = name[i];
    else
      name10[i] = ' ';
  }
  printf("%10s: ", name10);
  real x = ABSMAX(arr, l);
  for (i = 0; i < l; i++) {
    if (ABS(arr[i]) == x)
      printfc('r', 'k', "%12.6g ", arr[i]);
    else if (-NUM_EPS <= arr[i] && arr[i] <= NUM_EPS)
      printf("%12.6g ", 0.0);
    else
      printf("%12.6g ", arr[i]);
  }
  printf("\n");
  fflush(stdout);
}

real *NumNewHugeVec(long elem_num) {
  real *ptr;
  if (posix_memalign((void **)&ptr, 128, elem_num * sizeof(real))) {
    LOG(0, "[NumNewHugeVec]: memory allocation failed!\n");
    exit(1);
  }
  return ptr;
}

int *NumNewHugeIntVec(long elem_num) {
  int *ptr;
  if (posix_memalign((void **)&ptr, 128, elem_num * sizeof(int))) {
    LOG(0, "[NumNewHugeVec]: memory allocation failed!\n");
    exit(1);
  }
  return ptr;
}

real *NumNewVec(long elem_num) { return malloc(elem_num * sizeof(real)); }

real *NumNewIntVec(long elem_num) { return malloc(elem_num * sizeof(int)); }

void NumCopyVec(real *d, real *s, int l) {
  memcpy(d, s, l * sizeof(real));
  return;
}

void NumCopyIntVec(int *d, int *s, int l) {
  memcpy(d, s, l * sizeof(int));
  return;
}

real *NumCloneHugeVec(real *vec, long elem_num) {
  real *ptr = NumNewHugeVec(elem_num);
  NumCopyVec(ptr, vec, elem_num);
  return ptr;
}

int *NumCloneHugeIntVec(int *vec, long elem_num) {
  int *ptr = NumNewHugeIntVec(elem_num);
  NumCopyIntVec(ptr, vec, elem_num);
  return ptr;
}

real *NumCloneVec(real *vec, int elem_num) {
  real *ptr = malloc(elem_num * sizeof(real));
  NumCopyVec(ptr, vec, elem_num);
  return ptr;
}

int *NumCloneIntVec(int *vec, int elem_num) {
  int *ptr = malloc(elem_num * sizeof(int));
  NumCopyIntVec(ptr, vec, elem_num);
  return ptr;
}

void NumReadVec(real *ptr, long elem_num, FILE *fin) {
  long actual_read_size = fread(ptr, sizeof(real), elem_num, fin);
  if (actual_read_size != elem_num) {
    LOG(0, "[NumReadVec]: read error!");
    exit(1);
  }
  return;
}

real NumVecNorm(real *arr, int l) {
  real s = 0;
  int i = 0;
  for (i = 0; i < l; i++) s += arr[i] * arr[i];
  return sqrt(s);
}

real NumVecZeroNorm(real *arr, int l) {
  real s = 0;
  int i = 0;
  for (i = 0; i < l; i++) s += ((arr[i] == 0) ? 0 : 1);
  return s;
}

real NumVecPNorm(real *arr, int l, int p) {
  real s = 0;
  int i = 0;
  for (i = 0; i < l; i++) s += pow(arr[i], p);
  return pow(s, 1.0 / p);
}

real NumMatMaxRowNorm(real *mat, int m, int n) {
  int i;
  real x = 0, y;
  for (i = 0; i < m; i++) {
    y = NumVecNorm(mat + i * n, n);
    x = ((i == 0 || y > x) ? y : x);
  }
  return x;
}

real NumVecDot(real *a, real *b, int l) {
  real s = 0;
  int i;
  for (i = 0; i < l; i++) s += a[i] * b[i];
  return s;
}

real NumVecL2Dist(real *a, real *b, int l) {
  real s = 0;
  int i;
  for (i = 0; i < l; i++) s += (a[i] - b[i]) * (a[i] - b[i]);
  return sqrt(s);
}

real NumVecCos(real *a, real *b, int l) {
  return NumVecDot(a, b, l) / (NumVecNorm(a, l) + NUM_EPS) /
         (NumVecNorm(b, l) + NUM_EPS);
}

void NumVecAddCVec(real *a, real *b, real c, int l) {
  // in place a <- a + c * b
  int i;
  for (i = 0; i < l; i++) a[i] += c * b[i];
  return;
}

void NumVecMulC(real *a, real c, int l) {
  // in place a <- c * a
  int i;
  for (i = 0; i < l; i++) a[i] *= c;
  return;
}

void NumMulCVec(real *a, real c, int l, real *x) {
  // x <- c * a
  int i;
  for (i = 0; i < l; i++) x[i] = c * a[i];
  return;
}

void NumMulMatVec(real *m, real *a, int l, int r, real *x) {
  // x <- m * a; x: l, m: l * r, a: r
  int i, j, k = 0;
  for (i = 0; i < l; i++) x[i] = 0;
  for (i = 0; i < l; i++)
    for (j = 0; j < r; j++) x[i] += m[k++] * a[j];
  return;
}

void NumMulVecMat(real *a, real *m, int l, int r, real *x) {
  // x <- a * m; x: r, m: l * r, a: l
  int i, j, k = 0;
  for (j = 0; j < r; j++) x[j] = 0;
  for (i = 0; i < l; i++)
    for (j = 0; j < r; j++) x[j] += m[k++] * a[i];
  return;
}

void NumAddCVecDVec(real *a, real *b, real c, real d, int l, real *x) {
  // x <- c * a + d * b
  int i;
  for (i = 0; i < l; i++) x[i] = c * a[i] + d * b[i];
  return;
}

real NumSoftMax(real *a, real d, int l) {
  // return entropy, discounted by d; propto exp(a/d)
  int i;
  real c = MAX(a, l);
  real s = 0, e = 0, t;
  for (i = 0; i < l; i++) {
    a[i] = (a[i] - c) / d;  // discount by d
    t = NumExp(a[i]);
    e += t * a[i];
    s += t;
    a[i] = t;
  }
  for (i = 0; i < l; i++) a[i] /= s;
  e = -e / s + log(s);
  return (e > 0) ? e : 0;
}

real NumSigmoid(real x) {
  real y;
  if (x >= -NUM_EXP_LOW)
    return 1;
  else if (x <= NUM_EXP_LOW)
    return 0;
  else if (x < 0) {
    y = NumExp(x);
    return y / (1 + y);
  } else {
    y = NumExp(-x);
    return 1 / (1 + y);
  }
}

void NumVecProjUnitSphere(real *a, real s, int l) {
  // project to unit sphere
  NumVecMulC(a, s / NumVecNorm(a, l), l);
  return;
}

void NumVecProjUnitBall(real *a, real s, int l) {
  // project to unit ball
  real c = s / NumVecNorm(a, l);
  if (c < 1) NumVecMulC(a, c, l);
  return;
}

void NumFillValVec(real *a, int l, real v) {
  int i;
  for (i = 0; i < l; i++) a[i] = v;
  return;
}

void NumFillValIntVec(int *a, int l, int v) {
  int i;
  for (i = 0; i < l; i++) a[i] = v;
  return;
}

void NumFillZeroVec(real *a, int l) {
  memset(a, 0, l * sizeof(real));
  return;
}

void NumFillZeroIntVec(int *a, int l) {
  memset(a, 0, l * sizeof(int));
  return;
}

real NumSumVec(real *a, int l) {
  int i;
  real s = 0;
  for (i = 0; i < l; i++) s += a[i];
  return s;
}

int NumSumIntVec(int *a, int l) {
  int i;
  int s = 0;
  for (i = 0; i < l; i++) s += a[i];
  return s;
}

int NumIsNanVec(real *a, int l) {
  int i;
  for (i = 0; i < l; i++)
    if (a[i] != a[i]) return 1;
  return 0;
}

real NumVecMean(real *a, int l) { return NumSumVec(a, l) / l; }

real NumVecVar(real *a, int l) {
  real x = NumVecNorm(a, l);
  real y = NumVecMean(a, l);
  real s = x * x / l - y * y;
  return (s > 0) ? s : 0;
}

real NumVecStd(real *a, int l) { return sqrt(NumVecVar(a, l)); }

real NumSvSum(int *svk, int svn, real *svv) {
  int i;
  real x = 0;
  for (i = 0; i < svn; i++) x += svv[svk[i]];
  return x;
}

void NumVecAddCSv(real *arr, int *svk, int svn, real *svv, real c, int l) {
  int i;
  for (i = 0; i < svn; i++) arr[svk[i]] += c * svv[svk[i]];
  return;
}

void NumVecAddCSvOnes(real *arr, int *svk, int svn, real c) {
  int i;
  for (i = 0; i < svn; i++) arr[svk[i]] += c;
  return;
}
void NumMultinomialWRBInit(real *m, int l, int if_sorted, int **a_ptr,
                           real **p_ptr) {
  pair *tl;
  int i;
  real q;
  if (if_sorted) {
    tl = array2tuples(m, l);
  } else {
    tl = sorted(m, l, 1);
  }
  int f = 0;
  int r = l - 1;
  int *a = NumNewHugeIntVec(l);
  real *p = NumNewHugeVec(l);
  for (i = 0; i < l; i++) p[tl[i].key] = tl[i].val * l;
  while (1) {
    if (p[tl[f].key] > 1 && p[tl[r].key] < 1) {  // rear underflow
      q = 1 - p[tl[r].key];                      // underflow probability
      a[tl[r].key] = tl[f].key;                  // alias
      p[tl[f].key] -= q;                         // probability
      r--;
    } else if (p[tl[f].key] < 1 && p[tl[f + 1].key] > 1) {  // front underflow
      q = 1 - p[tl[f].key];
      a[tl[f].key] = tl[f + 1].key;
      p[tl[f + 1].key] -= q;
      f++;
    } else
      break;
  }
  free(tl);
  *p_ptr = p;
  *a_ptr = a;
  return;
}

int NumMultinomialWRBSample(int *a, real *p, int l, unsigned long *rs) {
  if (!rs) rs = &RANDOM_SEED;
  int r1 = NumRandNext(rs) * l;
  real r2 = NumRandNext(rs);
  if (r2 < p[r1])
    return r1;
  else
    return a[r1];
}

int *NumMultinomialTableInit(real *m, int l, real r) {
  int *x = NumNewHugeIntVec(r * l);
  int i = 0, j = 0;
  real c = 0;
  while (1) {
    c += m[i] * r * l;
    while (j < c) x[j++] = i;
    if (++i == l) break;
  }
  return x;
}

int NumMultinomialTableSample(int *x, int l, real r, unsigned long *rs) {
  if (!rs) rs = &RANDOM_SEED;
  int j = NumRandNext(rs) * l * r;
  return x[j];
}

void NumInit() {
  // Initialize exp table;
  int i;
  real x;
  for (i = 0; i < NUM_EXP_TABLE_LEN; i++) {
    x = NUM_EXP_LOW + i * (NUM_EXP_HIGH - NUM_EXP_LOW) / NUM_EXP_TABLE_LEN;
    NUM_EXP_TABLE[i] = exp(x);
  }
}

#endif /* end of include guard: UTIL_NUM */
