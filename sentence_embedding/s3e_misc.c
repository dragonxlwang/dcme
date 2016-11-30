////////////////////
// Misc Functions //
////////////////////
void PrintArr(char *name, real *arr, int l) {
  printf("%s", name);
  printf(" ");
  int i = 0;
  for(i = 0; i < l; i++)
    if(arr[i] == 0) printf("%10d ", 0);
    else printf("%10lf ", arr[i]);
  printf("\n");
  fflush(stdout);
  return;
}

void PrintMatrix(char *name, real *arr, int m, int n) {
  int i, j;
  for(i = 0; i < m; i++) {
    if(i == 0) printf("%-10s", name);
    else printf("%10s", "");
    for(j = 0; j < n; j++) {
      printf("%10lf ", arr[i * n + j]);
    }
    printf("\n");
  }
  fflush(stdout);
  return;
}

void PrintArrAbsMaxColor(char *name, real *arr, int l, char* color_code) {
  printf("%s", name);
  printf(" ");
  int i = 0;
  real x = ABSMAX(arr, l);
  for(i = 0; i < l; i++) {
    if(ABS(arr[i]) == x) printf("%s", color_code);
    if(arr[i] == 0) printf("0 ");
    else printf("%e ", arr[i]);
    if(ABS(arr[i]) == x) printf(ANSI_COLOR_RESET);
  }
  printf("\n");
  fflush(stdout);
}

void AllocAlignMem(char* str, void** var, long long int size) {
  if(posix_memalign(var, 128, size)) {
    LOG(0, "%s: memory allocation failed!", str);
    exit(1);
  }
  return;
}

void Sfread(real *ptr, long long int size, FILE *fin) {
  long long int read_size = fread(ptr, sizeof(real), size, fin);
  if(read_size != size) {printf("fread error!"); exit(1);}
  return;
}

void InitConstant() {
  seed = 0x0F0F0F0FL;
  SMN_SIZE =  3 * s3eM * s3eM * s3eN + 2 * s3eM * s3eM +     s3eN;
  DSMN_SIZE = 3 * s3eM * s3eM * s3eN +     s3eM * s3eM + 2 * s3eN;
  SYN_SIZE = s3eM * (s3eM + 1) * (s3eM + 1);
  return;
}

void Perm_Gen(int i, int j, int *word_ids, int *neg_word_ids, int l) {
  int k;
  for(k = 0; k < l; k++) neg_word_ids[k] = word_ids[k];
  SWAP(neg_word_ids[i], neg_word_ids[j]);
  return;
}

void Flip_Gen(int i, int w, int *word_ids, int *neg_word_ids, int l) {
  int k;
  for(k = 0; k < l; k++) neg_word_ids[k] = word_ids[k];
  neg_word_ids[i] = w;
  return;
}

/////////////
// Numeric //
/////////////
void InitSquashTable() {
  AllocAlignMem("s3eExpTable",  (void **)&s3eExpTable,
                (long long)(s3eSquashTableSize + 1) * sizeof(real));
  AllocAlignMem("s3eSigmTable", (void **)&s3eSigmTable,
                (long long)(s3eSquashTableSize + 1) * sizeof(real));
  AllocAlignMem("s3eTanhTable", (void **)&s3eTanhTable,
                (long long)(s3eSquashTableSize + 1) * sizeof(real));
  int i;
  for(i = 0; i <= s3eSquashTableSize; i++) {
    real r = exp(((real) i / s3eSquashTableSize * 2 - 1.0)
                      * s3eSquashTableRange);
    s3eExpTable[i] = r;
    s3eSigmTable[i] = r / (r + 1.0);
    s3eTanhTable[i] = (r * r - 1.0) / (r * r + 1.0);
  }
  return;
}

real Norm(real *arr, int l) {
  real s = 0;
  int i = 0;
  for(i = 0; i < l; i ++) s += arr[i] * arr[i];
  return sqrt(s);
}

real Dot(real *a, real *b, int l) {
  real s = 0;
  int i;
  for(i = 0; i < l; i++) s += a[i] * b[i];
  return s;
}

real Dist(real *a, real *b, int l) {
  real s = 0, x;
  int i;
  for(i = 0; i < l; i++) { x = a[i] - b[i]; s += x * x;}
  return s;
}

real Cos(real *a, real *b, int l) {
  return Dot(a, b, l) / (Norm(a, l) + 1e-9) / (Norm(b, l) + 1e-9);
}

//////////////////
// Heap, SpanID //
//////////////////
struct Heap {
  int size;
  int key[s3eMaxHeapSize];
  real val[s3eMaxHeapSize];
};

void ResolveSpanId(int id, int l, int *b, int *e) {
  int ub = l - 1, lb = 0, m, beg, end;
  while(1) {
    m = (ub + lb) / 2;
    beg = (2 * l - m + 1) * m / 2;
    end = beg + l - m;
    if(beg <= id && id < end) break;
    else if(beg > id) ub = m - 1;
    else lb = m + 1;
  }
  *b = id - beg;
  *e = (*b) + m + 1;
  return;
}

void HeapSiftUp(struct Heap *heap) {
  int c = heap->size - 1, p = (heap->size - 2) >> 1;
  while(p >= 0 && heap->val[c] < heap->val[p]) {
    SWAP(heap->key[c], heap->key[p]);
    SWAP(heap->val[c], heap->val[p]);
    c = p--; p >>= 1;
  }
  return;
}

void HeapSiftDown(struct Heap *heap) {
  int p = 0, c = 1;
  while(1) {
    if(c + 1 < heap->size && heap->val[c] > heap->val[c + 1]) c++;
    if(c >= heap->size || heap->val[p] <= heap->val[c]) break;
    SWAP(heap->key[c], heap->key[p]);
    SWAP(heap->val[c], heap->val[p]);
    p = c; c <<= 1; c++;
  }
  return;
}

int HeapInsert(struct Heap *heap, real val) {
  int k = -1;
  if(heap->size < s3eBestK) {
    k = heap->key[heap->size];
    heap->val[heap->size++] = val;
    HeapSiftUp(heap);
  }
  else if(val > heap->val[0]) {
    k = heap->key[0];
    heap->val[0] = val;
    HeapSiftDown(heap);
  }
  return k;
}

int HeapPop(struct Heap *heap) {
  int p = heap->key[0];
  SWAP(heap->key[0], heap->key[heap->size]);
  SWAP(heap->val[0], heap->val[heap->size]);
  heap->size--;
  HeapSiftDown(heap);
  return p;
}

void HeapSort(struct Heap *heap) {
  int size = heap->size;
  while(heap->size > 0) HeapPop(heap);
  heap->size = size;
  return;
}

////////////////
// Structures //
////////////////
struct Param {
  real *smn, *dsmn, *syn, *smnlut, *synlut;
};

void AllocParam(struct Param** p, int lut_size) {
  *p = (struct Param *) malloc(sizeof(struct Param));
  AllocAlignMem("SMN", (void**)&((*p)->smn), (long long)SMN_SIZE*sizeof(real));
  AllocAlignMem("DSMN",(void**)&((*p)->dsmn),(long long)DSMN_SIZE*sizeof(real));
  AllocAlignMem("SYN", (void**)&((*p)->syn), (long long)SYN_SIZE*sizeof(real));
  AllocAlignMem("SMNLUT", (void**)&((*p)->smnlut),
                      (long long)lut_size*s3eN*sizeof(real));
  AllocAlignMem("SYNLUT", (void**)&((*p)->synlut),
                      (long long)lut_size*s3eM*sizeof(real));
  return;
}

void FreeParam(struct Param* p) {
  free(p->smn); free(p->dsmn); free(p->syn); free(p->smnlut); free(p->synlut);
  free(p);
  return;
}

void InitParam(struct Param* p, int lut_size, int flag) {
  int i;
  for(i = 0; i < SMN_SIZE; i++)
    p->smn[i] = (flag == 0) ? 0 : (RAND(seed) - 0.5);
  for(i = 0; i < DSMN_SIZE; i++)
    p->dsmn[i] = (flag == 0) ? 0 : (RAND(seed) - 0.5);
  for(i = 0; i < SYN_SIZE; i++)
    p->syn[i] = (flag == 0) ? 0 : (RAND(seed) - 0.5);
  for(i = 0; i < lut_size * s3eN; i++)
    p->smnlut[i] = (flag == 0) ? 0 : (RAND(seed) - 0.5);
  for(i = 0; i < lut_size * s3eM; i++)
    p->synlut[i] = (flag == 0) ? 0 : (RAND(seed) - 0.5);
  return;
}

void ChunkUpdate(real *src, real *tar, real *var, int size, real lr, real l2w) {
  // set lr = -1, l2w = 0, to add src to tar
  int i;
  if(var) {
    for(i = 0; i < size; i++) {
      var[i] += SQ(src[i]);
      tar[i] -= lr*(src[i] + l2w*tar[i]) / (sqrt(var[i]) + adag_eps + lr*l2w);
    }
  } else {
    for(i = 0; i < size; i++) tar[i] -= lr * (src[i] + l2w * tar[i]);
  }
  return;
}

void BatchUpdateParam(struct Param *src, struct Param *tar, struct Param *var,
                      real lr, real l2w, int *qs, int *qt, int *is, int *it){
  int i, j;
  ChunkUpdate(src->smn, tar->smn, var?var->smn:0L, SMN_SIZE, lr, l2w);
  ChunkUpdate(src->dsmn, tar->dsmn, var?var->dsmn:0L, DSMN_SIZE, lr, l2w);
  ChunkUpdate(src->syn, tar->syn, var?var->syn:0L, SYN_SIZE, lr, l2w);
  j = *is;
  if(it) {
    for(i = 0; i < j; i++) {
      ChunkUpdate(src->synlut + i * s3eM, tar->synlut + (*it) * s3eM,
          var?(var->synlut + qs[i] * s3eM):0L, s3eM, lr, l2w);
      ChunkUpdate(src->smnlut + i * s3eN, tar->smnlut + (*it) * s3eN,
          var?(var->smnlut + qs[i] * s3eN):0L, s3eN, lr, l2w);
      qt[(*it)++] = qs[i];
    }
  } else {
    for(i = 0; i < j; i++) {
      ChunkUpdate(src->synlut + i * s3eM, tar->synlut + qs[i] * s3eM,
          var?(var->synlut + qs[i] * s3eM):0L, s3eM, lr, l2w);
      ChunkUpdate(src->smnlut + i * s3eN, tar->smnlut + qs[i] * s3eN,
          var?(var->smnlut + qs[i] * s3eN):0L, s3eN, lr, l2w);
    }
  }
  return;
}

char* ToStringParam(struct Param * p) {
    static char str[1000];
    sprintf(str, "W %7e,%7e V %7e, L %7e,%7e",
            Norm(p->smn, SMN_SIZE), Norm(p->dsmn, DSMN_SIZE),
            Norm(p->syn, SYN_SIZE), Norm(p->smnlut, vocab.size * s3eN),
            Norm(p->synlut, vocab.size * s3eM));
    return str;
}

void SaveParam(struct Param *p, int size, FILE *fout) {
  fwrite(p->smn, sizeof(real), SMN_SIZE, fout);
  fwrite(p->dsmn, sizeof(real), DSMN_SIZE, fout);
  fwrite(p->syn, sizeof(real), SYN_SIZE, fout);
  fwrite(p->smnlut, sizeof(real), size * s3eN, fout);
  fwrite(p->synlut, sizeof(real), size * s3eM, fout);
  return;
}

struct Param *LoadParam(FILE *fin) {
  InitConstant();
  struct Param *p;
  AllocParam(&p, vocab.size);
  Sfread(p->smn, SMN_SIZE, fin);
  Sfread(p->dsmn, DSMN_SIZE, fin);
  Sfread(p->syn, SYN_SIZE, fin);
  Sfread(p->smnlut, vocab.size * s3eN, fin);
  Sfread(p->synlut, vocab.size * s3eM, fin);
  if(fgetc(fin) == EOF)
    printf("%sLoad Done!%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
  return p;
}

struct Bookkeeping {
  real *v, *w, *s, *aw1, *aw2;      // forward
  int *l, *r, *num;
  real sentence_score;
  int sentence_bkkp_entry;
  int *q;                           // backward
  real *gv, *gw, *gs;
  int hd, tl;
};

void InitBkkp(struct Bookkeeping *bkkp) {
  int bkkp_size = s3eMaxSentenceLength * (s3eMaxSentenceLength + 1) / 2;
  AllocAlignMem("bkkp->v",        (void **) &(bkkp->v),
                (long long) bkkp_size * s3eBestK * s3eM * sizeof(real));
  AllocAlignMem("bkkp->w",        (void **) &(bkkp->w),
                (long long) bkkp_size * s3eBestK * s3eN * sizeof(real));
  AllocAlignMem("bkkp->s",        (void **) &(bkkp->s),
                (long long) bkkp_size * s3eBestK * sizeof(real));
  AllocAlignMem("bkkp->aw1",      (void **) &(bkkp->aw1),
                (long long) bkkp_size * s3eBestK * s3eN * sizeof(real));
  AllocAlignMem("bkkp->aw2",      (void **) &(bkkp->aw2),
                (long long) bkkp_size * s3eBestK * s3eN * sizeof(real));
  AllocAlignMem("bkkp->l",        (void **) &(bkkp->l),
                (long long) bkkp_size * s3eBestK * sizeof(int));
  AllocAlignMem("bkkp->r",        (void **) &(bkkp->r),
                (long long) bkkp_size * s3eBestK * sizeof(int));
  AllocAlignMem("bkkp->num",      (void **) &(bkkp->num),
                (long long) bkkp_size * sizeof(int));
  AllocAlignMem("bkkp->q",        (void **) &(bkkp->q),
                (long long) s3eMaxSentenceLength * 2 * sizeof(int));
  AllocAlignMem("bkkp->gv",       (void **) &(bkkp->gv),
                (long long) s3eMaxSentenceLength * 2 * s3eM * sizeof(real));
  AllocAlignMem("bkkp->gw",       (void **) &(bkkp->gw),
                (long long) s3eMaxSentenceLength * 2 * s3eN * sizeof(real));
  AllocAlignMem("bkkp->gs",       (void **) &(bkkp->gs),
                (long long) s3eMaxSentenceLength * 2 * sizeof(real));
  bkkp->hd = 0;
  bkkp->tl = 0;
  return;
}

void ZeroBkkp(struct Bookkeeping *bkkp) {
  memset(bkkp->gv, 0, bkkp->hd * s3eM * sizeof(real));
  memset(bkkp->gw, 0, bkkp->hd * s3eN * sizeof(real));
  bkkp->hd = 0;
  bkkp->tl = 0;
  return;
}

void FreeBkkp(struct Bookkeeping *bkkp) {
  free(bkkp->v); free(bkkp->w); free(bkkp->s); free(bkkp->aw1); free(bkkp->aw2);
  free(bkkp->l); free(bkkp->r); free(bkkp->num);
  free(bkkp->q); free(bkkp->gv); free(bkkp->gw); free(bkkp->gs);
  free(bkkp);
  return;
}

struct Grad {
  struct Param *gm;
  int *idx;
  int idx_size, cnt;
};

void InitGrad(struct Grad *g) {
  AllocParam(&(g->gm), s3eMaxSentenceLength * s3eBsz);
  AllocAlignMem("g->idx",  (void **) &(g->idx),
                (long long) s3eMaxSentenceLength * s3eBsz * sizeof(int));
  g->idx_size = 0;
  g->cnt = 0;
  return;
}

void ZeroGrad(struct Grad *g) {
  InitParam(g->gm, g->idx_size, 0);
  g->idx_size = 0;
  g->cnt = 0;
  return;
}

void FreeGrad(struct Grad *g) {
  FreeParam(g->gm);
  free(g->idx);
  free(g);
  return;
}
