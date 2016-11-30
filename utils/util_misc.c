#ifndef UTIL_MISC
#define UTIL_MISC

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wordexp.h>

// util_misc.c shall only include system include files
// sid_module_name prefix indicate functions/variables shall not be accessed
//    directly; for variables, a suffix '_' is used.
// Parameters:
//  log_debug_mode        : debug mode 0 print only error, 2 print all

/***
 *      #####
 *     #     # ##### #####  # #    #  ####
 *     #         #   #    # # ##   # #    #
 *      #####    #   #    # # # #  # #
 *           #   #   #####  # #  # # #  ###
 *     #     #   #   #   #  # #   ## #    #
 *      #####    #   #    # # #    #  ####
 *
 */
// w k r g y b m c l
#define ANSI_COLOR_WHITE "\x1b[00m"
#define ANSI_COLOR_BLACK "\x1b[30m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_LGRAY "\x1b[37m"
#define ANSI_COLOR_BG_BLACK "\x1b[40m"
#define ANSI_COLOR_BG_RED "\x1b[41m"
#define ANSI_COLOR_BG_GREEN "\x1b[42m"
#define ANSI_COLOR_BG_YELLOW "\x1b[43m"
#define ANSI_COLOR_BG_BLUE "\x1b[44m"
#define ANSI_COLOR_BG_MAGENTA "\x1b[45m"
#define ANSI_COLOR_BG_CYAN "\x1b[46m"
#define ANSI_COLOR_BG_LGRAY "\x1b[47m"
#define ANSI_COLOR_RESET "\x1b[0m"

int log_debug_mode = 2;

// print only if log_dbg_level is small enough -- 0 being always printed
#define LOG(log_dbg_level, ...)            \
  ({                                       \
    if (log_debug_mode >= log_dbg_level) { \
      printf(__VA_ARGS__);                 \
      fflush(stdout);                      \
    }                                      \
  })
#define LOGC(log_dbg_level, ...)           \
  ({                                       \
    if (log_debug_mode >= log_dbg_level) { \
      printfc(__VA_ARGS__);                \
      fflush(stdout);                      \
    }                                      \
  })
#define LOGCLR(log_dbg_level)              \
  ({                                       \
    if (log_debug_mode >= log_dbg_level) { \
      printf("\33[2K\r");                  \
      fflush(stdout);                      \
    }                                      \
  })
#define LOGCR(log_dbg_level)               \
  ({                                       \
    if (log_debug_mode >= log_dbg_level) { \
      printf("\n");                        \
      fflush(stdout);                      \
    }                                      \
  })
#define LOWER(c) (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))
#define UPPER(c) (((c) >= 'a' && (c) <= 'z') ? (c) - 'a' + 'A' : (c))

int cisspace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
int cisdigit(char c) { return c >= '0' && c <= '9'; }

void slower(char *s) {
  int i;
  int l = strlen(s);
  for (i = 0; i < l; i++) s[i] = LOWER(s[i]);
  return;
}

void suppwer(char *s) {
  int i;
  int l = strlen(s);
  for (i = 0; i < l; i++) s[i] = UPPER(s[i]);
  return;
}

char *sconcat(char *sa, char *sb, int la, int lb) {
  // if la = -1 (or lb), use whole string for concatenating
  int i;
  if (la == -1) la = strlen(sa);
  if (lb == -1) lb = strlen(sb);
  char *s = (char *)malloc(la + lb);
  for (i = 0; i < la; i++) s[i] = sa[i];
  for (i = 0; i < lb; i++) s[i + la] = sb[i];
  return s;
}

void srstrip(char *s) {
  int len = strlen(s);
  int i = len - 1;
  while (i >= 0 && cisspace(s[i])) s[i--] = 0;
  return;
}

void slstrip(char *s) {
  int i = 0, j = 0;
  while (cisspace(s[i++]))
    ;
  if (--i == 1) return;
  while ((s[j++] = s[i++]) != '\0')
    ;
}

void sstrip(char *s) {
  srstrip(s);
  slstrip(s);
  return;
}

char *sreplace(char *str, char *from, char *to) {
  char *ret = (char *)malloc(strlen(str) + strlen(from) + strlen(to) + 1);
  char *pp = strstr(str, from), *ps = str, *pt = ret, *p1 = to;
  while (ps != pp && *ps != 0) *(pt++) = *(ps++);
  if (pp) {
    while (*p1 != 0) *(pt++) = *(p1++);
    ps += strlen(from);
    while (*ps != 0) *(pt++) = *(ps++);
  }
  *pt = 0;
  return ret;
}

char *sclone(char *s) {
  char *d = (char *)malloc((strlen(s) + 1) * sizeof(char));
  strcpy(d, s);
  return d;
}

char *sformat(char *fmt, ...) {
  // sort of like sprintf, but allocate string with malloc. max length 4096
  char s[0x1000] = {0};
  char *ss;
  va_list al;
  va_start(al, fmt);
  vsprintf(s, fmt, al);
  va_end(al);
  ss = sclone(s);
  return ss;
}

void vsprintfc(char *str, char fg_color_code, char bg_color_code,
               const char *fmt, va_list al) {
  char f = LOWER(fg_color_code);
  char b = LOWER(bg_color_code);
  char *fg, *bg;
  switch (f) {
    case 'w':
      fg = ANSI_COLOR_WHITE;
      break;
    case 'k':
      fg = ANSI_COLOR_BLACK;
      break;
    case 'r':
      fg = ANSI_COLOR_RED;
      break;
    case 'g':
      fg = ANSI_COLOR_GREEN;
      break;
    case 'y':
      fg = ANSI_COLOR_YELLOW;
      break;
    case 'b':
      fg = ANSI_COLOR_BLUE;
      break;
    case 'm':
      fg = ANSI_COLOR_MAGENTA;
      break;
    case 'c':
      fg = ANSI_COLOR_CYAN;
      break;
    case 'l':
      fg = ANSI_COLOR_LGRAY;
      break;
    default:
      printf("Error: color code\n");
      exit(1);
  }
  switch (b) {
    case 'k':
      bg = ANSI_COLOR_BG_BLACK;
      break;
    case 'r':
      bg = ANSI_COLOR_BG_RED;
      break;
    case 'g':
      bg = ANSI_COLOR_BG_GREEN;
      break;
    case 'y':
      bg = ANSI_COLOR_BG_YELLOW;
      break;
    case 'b':
      bg = ANSI_COLOR_BG_BLUE;
      break;
    case 'm':
      bg = ANSI_COLOR_BG_MAGENTA;
      break;
    case 'c':
      bg = ANSI_COLOR_BG_CYAN;
      break;
    case 'l':
      bg = ANSI_COLOR_BG_LGRAY;
      break;
    default:
      printf("Error: color code\n");
      exit(1);
  }
  sprintf(str, "%s%s", fg, bg);
  vsprintf(str + strlen(str), fmt, al);
  sprintf(str + strlen(str), "%s", ANSI_COLOR_RESET);
}

char *vsformatc(char fg_color_code, char bg_color_code, const char *fmt,
                va_list al) {
  char s[0x1000];
  char *ss;
  vsprintfc(s, fg_color_code, bg_color_code, fmt, al);
  ss = sclone(s);
  return ss;
}

char *sformatc(char fg_color_code, char bg_color_code, const char *fmt, ...) {
  va_list al;
  va_start(al, fmt);
  char *s = vsformatc(fg_color_code, bg_color_code, fmt, al);
  va_end(al);
  return s;
}

void printfc(char fg_color_code, char bg_color_code, const char *fmt, ...) {
  va_list al;
  va_start(al, fmt);
  char *s = vsformatc(fg_color_code, bg_color_code, fmt, al);
  va_end(al);
  printf("%s", s);
  free(s);
  return;
}

void saprintf(char *str, const char *fmt, ...) {
  va_list al;
  va_start(al, fmt);
  vsprintf(str + strlen(str), fmt, al);
  va_end(al);
  return;
}

void sprintfc(char *str, char fg_color_code, char bg_color_code,
              const char *fmt, ...) {
  va_list al;
  va_start(al, fmt);
  vsprintfc(str, fg_color_code, bg_color_code, fmt, al);
  va_end(al);
  return;
}

void saprintfc(char *str, char fg_color_code, char bg_color_code,
               const char *fmt, ...) {
  va_list al;
  va_start(al, fmt);
  vsprintfc(str + strlen(str), fg_color_code, bg_color_code, fmt, al);
  va_end(al);
  return;
}

char *strprogbar(double p, int len, int bar_only) {
  char *str = malloc(0x1000);
  int i;
  int bar = p * len;
  if (!bar_only) sprintf(str, "[%08.4lf%%]: ", p * 100);  // pct
  for (i = 0; i < bar; i++) saprintf(str, "+");           // past
  if (i < len) saprintf(str, "~");                        // current
  for (i = bar + 1; i < len; i++) saprintf(str, "=");     // left
  return str;
}

char *strprogbarc(double p, int len, int bar_only) {
  char *str = malloc(0x1000);
  int i;
  int bar = p * len;
  if (!bar_only) sprintfc(str, 'y', 'k', "[%08.4lf%%]: ", p * 100);  // pct
  for (i = 0; i < bar; i++) saprintfc(str, 'r', 'k', "+");           // past
  if (i < len) saprintfc(str, 'w', 'k', "~");                        // current
  for (i = bar + 1; i < len; i++) saprintfc(str, 'c', 'k', "=");     // left
  return str;
}

char *strtime(int s) {
  int ss = s;
  int mm = ss / 60;
  ss %= 60;
  int hh = mm / 60;
  mm %= 60;
  int dd = hh / 24;
  hh %= 24;
  return sformat("%02d:%02d:%02d:%02d", dd, hh, mm, ss);
}

char *strclock(clock_t t1, clock_t t2, int thread_num) {
  return strtime((double)(t2 - t1) / CLOCKS_PER_SEC / thread_num);
}

int getoptpos(char *str, int argc, char **argv) {
  // return the position of command line argument of str; return -1 if not found
  int i;
  for (i = 1; i < argc; i++)
    if (!strcmp(str, argv[i])) return i;
  return -1;
}

/***
 *     #######
 *     #       # #      ######
 *     #       # #      #
 *     #####   # #      #####
 *     #       # #      #
 *     #       # #      #
 *     #       # ###### ######
 *
 */
int fexists(const char *filename) {
  // however, this also return true if filename is a directory
  // which might not be desired
  FILE *file = fopen(filename, "r");
  if (file) fclose(file);
  return (file != 0);
}

FILE *fsopen(const char *filename, const char *mode) {
  FILE *fin = fopen(filename, mode);
  if (!fin) {
    LOG(0, "Error! Can't Open File: %s\n", filename);
    exit(1);
  }
  return fin;
}

char *FilePathExpand(char *fp) {
  wordexp_t ep;
  wordexp(fp, &ep, 0);
  char *nfp = malloc(strlen(ep.we_wordv[0]) + 1);
  strcpy(nfp, ep.we_wordv[0]);
  wordfree(&ep);
  return nfp;
}

char *FilePathSubExtension(char *fp, char *ext) {
  int i = strlen(fp) - 1;
  while (i >= 0 && fp[i] != '.') i--;
  if (i < 0) i = strlen(fp);
  char *nfp = (char *)malloc(i + 1 + strlen(ext) + 1);
  memcpy(nfp, fp, i);
  memcpy(nfp + i, ".", 1);
  memcpy(nfp + i + 1, ext, strlen(ext));
  nfp[i + 1 + strlen(ext)] = '\0';
  return nfp;
}

long int FileSize(char *fp) {
  FILE *fin = fopen(fp, "rb");
  if (!fin) {
    LOG(0, "Error\n");
    exit(1);
  }
  fseek(fin, 0, SEEK_END);
  long int fsz = ftell(fin);
  fclose(fin);
  return fsz;
}

int direxists(const char *dirname) {
  struct stat s;
  if (stat(dirname, &s) == 0 && S_ISDIR(s.st_mode)) return 1;
  return 0;
}

int dirmake(const char *dirname) { return mkdir(dirname, 0700); }

int dirdelete(const char *dirname) { return rmdir(dirname); }

/***
 *        #
 *       # #   #       ####   ####  #####  # ##### #    # #    #
 *      #   #  #      #    # #    # #    # #   #   #    # ##  ##
 *     #     # #      #      #    # #    # #   #   ###### # ## #
 *     ####### #      #  ### #    # #####  #   #   #    # #    #
 *     #     # #      #    # #    # #   #  #   #   #    # #    #
 *     #     # ######  ####   ####  #    # #   #   #    # #    #
 *
 */
#ifndef real
#define real double
#endif

#define SMALLER(a, b) ((a) <= (b) ? (a) : (b))
#define LARGER(a, b) ((a) >= (b) ? (a) : (b))
#define MAX(arr, l)                                        \
  ({                                                       \
    __typeof__(arr[0]) x = arr[0];                         \
    int i;                                                 \
    for (i = 1; i < l; i++) x = (x < arr[i]) ? arr[i] : x; \
    x;                                                     \
  })
#define MIN(arr, l)                                        \
  ({                                                       \
    __typeof__(arr[0]) x = arr[0];                         \
    int i;                                                 \
    for (i = 1; i < l; i++) x = (x > arr[i]) ? arr[i] : x; \
    x;                                                     \
  })
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define ABSMAX(arr, l)                                               \
  ({                                                                 \
    __typeof__(arr[0]) x = ABS(arr[0]);                              \
    int i;                                                           \
    for (i = 1; i < l; i++) x = (x < ABS(arr[i])) ? ABS(arr[i]) : x; \
    x;                                                               \
  })
#define SWAP(x, y)       \
  ({                     \
    __typeof__(x) z = x; \
    x = y;               \
    y = z;               \
  })

int sign(real x) { return (x == 0 ? 0 : (x < 0 ? -1 : 1)); }
int signi(int x) { return (x == 0 ? 0 : (x < 0 ? -1 : 1)); }

int *range(int size) {
  int *idx = (int *)malloc(size * sizeof(int));
  int i;
  for (i = 0; i < size; i++) idx[i] = i;
  return idx;
}

typedef struct pair {
  int key;
  real val;
} pair;
pair *array2tuples(real *arr, int size) {
  int i;
  pair *p = (pair *)malloc(sizeof(pair) * size);
  for (i = 0; i < size; i++) {
    p[i].key = i;
    p[i].val = arr[i];
  }
  return p;
}
pair *array2tuplesi(int *arr, int size) {
  int i;
  pair *p = (pair *)malloc(sizeof(pair) * size);
  for (i = 0; i < size; i++) {
    p[i].key = i;
    p[i].val = arr[i];
  }
  return p;
}
int cmp(const void *x, const void *y) {
  return sign(((real *)x) - ((real *)y));
}
int cmp_reverse(const void *x, const void *y) {
  return sign(((real *)y) - ((real *)x));
}
int cmp_int(const void *x, const void *y) {
  return sign(((int *)x) - ((int *)y));
}
int cmp_int_reverse(const void *x, const void *y) {
  return sign(((int *)y) - ((int *)x));
}
int cmp_pair(const void *x, const void *y) {
  return sign(((pair *)x)->val - ((pair *)y)->val);
}
int cmp_pair_reverse(const void *x, const void *y) {
  return sign(((pair *)y)->val - ((pair *)x)->val);
}
pair *sort_tuples(pair *p, int size, int reverse) {
  if (reverse)
    qsort(p, size, sizeof(pair), cmp_pair_reverse);
  else
    qsort(p, size, sizeof(pair), cmp_pair);
  return p;
}
pair *sorted(real *arr, int size, int reverse) {
  return sort_tuples(array2tuples(arr, size), size, reverse);
}
pair *sortedi(int *arr, int size, int reverse) {
  return sort_tuples(array2tuplesi(arr, size), size, reverse);
}

typedef struct heap {
  pair *d;  // hold data pairsof keys and vals
  int k;    // only retain max k elements
  int size;
} heap;
// min heap: select k max number from an array
void sid_misc_heap_sift_up(pair *h, int size) {
  int c = size - 1;
  int p = (size - 2) >> 1;
  while (p >= 0 && h[c].val < h[p].val) {
    SWAP(h[c].key, h[p].key);
    SWAP(h[c].val, h[p].val);
    c = p--;
    p >>= 1;
  }
  return;
}
void sid_misc_heap_sift_down(pair *h, int size) {
  int c = 1;
  int p = 0;
  while (1) {
    if (c + 1 < size && h[c].val > h[c + 1].val) c++;
    if (c >= size || h[p].val <= h[c].val) return;
    SWAP(h[c].key, h[p].key);
    SWAP(h[c].val, h[p].val);
    p = c;
    c <<= 1;
    c++;
  }
}
heap *HeapCreate(int k) {
  heap *h = (heap *)malloc(sizeof(heap));
  h->d = (pair *)malloc(k * sizeof(pair));
  h->k = k;
  h->size = 0;
  return h;
}
void HeapFree(heap *h) {
  free(h->d);
  free(h);
  return;
}
void HeapEmpty(heap *h) {
  h->size = 0;
  return;
}
int HeapPush(heap *h, int key, real val) {
  if (h->size < h->k) {
    h->d[h->size].key = key;
    h->d[h->size].val = val;
    h->size++;
    sid_misc_heap_sift_up(h->d, h->size);
  } else if (val > h->d[0].val) {
    h->d[0].key = key;
    h->d[0].val = val;
    sid_misc_heap_sift_down(h->d, h->size);
  } else
    return 0;
  return 1;
}
int HeapPop(heap *h) {
  h->size--;
  SWAP(h->d[h->size].key, h->d[0].key);
  SWAP(h->d[h->size].val, h->d[0].val);
  sid_misc_heap_sift_down(h->d, h->size);
  return h->d[h->size].key;
}
int HeapTopKey(heap *h) { return h->d[h->size].key; }
int HeapTopVal(heap *h) { return h->d[h->size].val; }
int HeapSort(heap *h) {
  int size = h->size;
  while (h->size > 0) HeapPop(h);
  return size;
}

unsigned long DictBkdrHash(char *str) {
  unsigned long h = 0;
  char ch;
  while ((ch = *(str++))) h = (h << 7) + (h << 1) + (h) + ch;
  return h;
}

typedef struct Dictionary {
  char **id2key;
  real *id2val;
  int *id2next;
  int *hash2head;
  int size;
  int cap;
} Dictionary;

Dictionary *DictCreate(int cap) {
  Dictionary *d = (Dictionary *)malloc(sizeof(Dictionary));
  if (cap < 0) cap = 0xFFFFF;  // default 1M
  d->id2key = (char **)malloc(cap * sizeof(char *));
  d->id2val = (real *)malloc(cap * sizeof(real));
  d->id2next = (int *)malloc(cap * sizeof(int));
  d->hash2head = (int *)malloc(cap * sizeof(int));
  d->size = 0;
  d->cap = cap;
  if (!d->id2key || !d->id2val || !d->id2next || !d->hash2head) {
    LOG(0, "[Dictionary]: allocation error\n");
    exit(1);
  }
  memset(d->hash2head, 0xFF, cap * sizeof(int));
  return d;
}

void DictFree(Dictionary *d) {
  free(d->id2key);
  free(d->id2val);
  free(d->id2next);
  free(d->hash2head);
  free(d);
}

void DictResize(Dictionary *d, int cap) {
  int i, h;
  d->cap = cap;
  d->id2key = (char **)realloc(d->id2key, d->cap * sizeof(char *));
  d->id2val = (real *)realloc(d->id2val, d->cap * sizeof(real));
  free(d->id2next);
  free(d->hash2head);
  d->id2next = (int *)malloc(d->cap * sizeof(int));
  d->hash2head = (int *)malloc(d->cap * sizeof(int));
  if (!d->id2key || !d->id2val || !d->id2next || !d->hash2head) {
    LOG(0, "[Dictionary]: resize error\n");
    exit(1);
  }
  memset(d->hash2head, 0xFF, d->cap * sizeof(int));
  for (i = 0; i < d->size; i++) {
    h = DictBkdrHash(d->id2key[i]) % d->cap;
    d->id2next[i] = d->hash2head[h];
    d->hash2head[h] = i;
  }
  return;
}

void DictInsert(Dictionary *d, char *k, real v) {
  int h = DictBkdrHash(k) % d->cap;
  int i = d->hash2head[h];
  while (i != -1 && strcmp(d->id2key[i], k) != 0) i = d->id2next[i];
  if (i == -1) {
    if (d->size == d->cap) {
      printf("resize: %d => %d\n", d->size, d->cap);
      DictResize(d, d->cap * 2);
    }
    i = d->size++;
    d->id2key[i] = sclone(k);
    d->id2next[i] = d->hash2head[h];
    d->hash2head[h] = i;
  }
  d->id2val[i] = v;
  return;
}

int DictLocate(Dictionary *d, char *k) {
  int h = DictBkdrHash(k) % d->cap;
  int i = d->hash2head[h];
  while (i != -1 && strcmp(d->id2key[i], k) != 0) i = d->id2next[i];
  return i;
}
char *DictGetKey(Dictionary *d, int i) { return d->id2key[i]; }
real DictGetVal(Dictionary *d, int i) { return d->id2val[i]; }
real DictGet(Dictionary *d, char *k, real default_val) {
  int i = DictLocate(d, k);
  return (i == -1 ? default_val : DictGetVal(d, i));
}

#endif /* ifndef UTIL_MISC */
