#ifndef PEEK
#define PEEK

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
#include "../vectors/model.c"

typedef struct PeekSet {
  int **wids;
  int *wnum;
  int size;
  int top_k;
  int *top_w;
} PeekSet;

void PeekSetFree(PeekSet *ps) {
  int i;
  for (i = 0; i < ps->size; i++) free(ps->wids[i]);
  free(ps->wnum);
  free(ps->top_w);
  free(ps);
}

void PeekSave(char *fp, Vocabulary *vcb, PeekSet *ps) {
  // format: top_k top words
  //         size
  //         sentence words
  int i, j, ttl_word = 0;
  FILE *fout = fopen(fp, "wb");
  fprintf(fout, "%d", ps->top_k);
  for (i = 0; i < ps->top_k; i++)
    fprintf(fout, " %s", VocabGetWord(vcb, ps->top_w[i]));
  fprintf(fout, "\n");
  fprintf(fout, "%d\n", ps->size);
  for (i = 0; i < ps->size; i++) {
    fprintf(fout, "%d", ps->wnum[i]);
    for (j = 0; j < ps->wnum[i]; j++)
      fprintf(fout, " %s", VocabGetWord(vcb, ps->wids[i][j]));
    fprintf(fout, "\n");
    ttl_word += ps->wnum[i];
  }
  fclose(fout);
  LOG(1, "[Peek] Save   :\n");
  LOG(1, "|- File       : %s\n", fp);
  LOG(1, "|- Top Words  : %d\n", ps->top_k);
  LOG(1, "|- Sentences  : %d\n", ps->size);
  LOG(1, "\\- Words      : %d\n", ttl_word);
  return;
}

PeekSet *PeekLoad(char *fp, Vocabulary *vcb) {
  PeekSet *ps = (PeekSet *)malloc(sizeof(PeekSet));
  int i, j, k, unk_term = 0, unk_word = 0, unk_sent = 0, ttl_word = 0, wnum,
               wids[SUP];
  char word[WUP];
  FILE *fin = fopen(fp, "rb");
  fscanf(fin, "%d", &k);
  ps->top_w = (int *)malloc(k * sizeof(int));
  ps->top_k = 0;
  while (k-- > 0) {
    fscanf(fin, " %s", word);
    j = VocabGetId(vcb, word);
    if (j != -1)
      ps->top_w[ps->top_k++] = j;
    else
      unk_term++;
  }
  ps->top_w = (int *)realloc(ps->top_w, ps->top_k * sizeof(int));
  fscanf(fin, "%d\n", &k);
  ps->wnum = (int *)malloc(k * sizeof(int));
  ps->wids = (int **)malloc(k * sizeof(int *));
  ps->size = 0;
  while (k-- > 0) {
    fscanf(fin, "%d", &i);
    wnum = 0;
    while (i-- > 0) {
      fscanf(fin, " %s", word);
      j = VocabGetId(vcb, word);
      if (j != -1)
        wids[wnum++] = j;
      else
        unk_word++;
    }
    if (wnum > 1) {
      ps->wids[ps->size] = NumCloneIntVec(wids, wnum);
      ps->wnum[ps->size] = wnum;
      ps->size++;
    } else
      unk_sent++;
    ttl_word += wnum;
  }
  ps->wids = (int **)realloc(ps->wids, ps->size * sizeof(int *));
  fclose(fin);
  LOG(1, "[Peek] Load   :\n");
  LOG(1, "|- File       : %s\n", fp);
  LOG(1, "|- Top Words  : %d (missing %d)\n", ps->top_k, unk_term);
  LOG(1, "|- Sentences  : %d (missing %d)\n", ps->size, unk_sent);
  LOG(1, "\\- Words      : %d (missing %d)\n", ttl_word, unk_word);
  return ps;
}

int sid_peek_pb_lock = 0;
PeekSet *sid_peek_build(char *text_file_path, int if_lower,
                        int if_rm_trail_punc, long int fbeg, long int fend,
                        real sample_rate, int peek_top_k, Vocabulary *vcb,
                        long int *fpass, int *size) {
  int i;
  FILE *fin = fopen(text_file_path, "rb");
  if (!fin) {
    LOG(0, "Error\n");
    exit(1);
  }
  PeekSet *ps = (PeekSet *)malloc(sizeof(PeekSet));
  ps->top_k = peek_top_k;
  ps->top_w = (int *)malloc(ps->top_k * sizeof(int));
  for (i = 0; i < peek_top_k; i++) ps->top_w[i] = i;
  fseek(fin, 0, SEEK_END);
  long int fsz = ftell(fin);
  fseek(fin, fbeg, SEEK_SET);
  if (!fin) {
    LOG(0, "Error\n");
    exit(1);
  }
  ps->size = 0;
  int cap = 10000;
  ps->wids = (int **)malloc(cap * sizeof(int *));
  ps->wnum = (int *)malloc(cap * sizeof(int));
  int wids[SUP], wnum;
  long int sid = 0, fpos1, fpos2;
  while (!feof(fin) && (fend < 0 || ftell(fin) < fend)) {
    fpos1 = ftell(fin);
    wnum = TextReadSent(fin, vcb, wids, if_lower, if_rm_trail_punc, 1);
    fpos2 = ftell(fin);
    *fpass += fpos2 - fpos1;
    if (sid++ > 100 && NumRand() < sample_rate && wnum >= 5) {
      ps->wids[ps->size] = NumCloneIntVec(wids, wnum);
      ps->wnum[ps->size] = wnum;
      ps->size++;
      (*size)++;
      if (ps->size == cap) {
        cap <<= 1;  // double resize
        ps->wids = (int **)realloc(ps->wids, cap * sizeof(int *));
        ps->wnum = (int *)realloc(ps->wnum, cap * sizeof(int));
      }
      if (!sid_peek_pb_lock) {
        sid_peek_pb_lock = 1;
        LOGCLR(2);
        LOG(2, "[PEEK]: completed %lf%% (%d sentence)",
            (double)(*fpass) * 100 / fsz, *size);
        sid_peek_pb_lock = 0;
      }
    }
  }
  ps->wids = (int **)realloc(ps->wids, ps->size * sizeof(int *));  // shrink
  ps->wnum = (int *)realloc(ps->wnum, ps->size * sizeof(int));
  return ps;
}

void *sid_peek_build_thread(void *param) {
  void **p = (void **)param;
  int i = 0;
  char *text_file_path = p[i++];
  int if_lower = *((int *)p[i++]);
  int if_rm_trail_punc = *((int *)p[i++]);
  long int fbeg = *((long int *)p[i++]);
  long int fend = *((long int *)p[i++]);
  real sample_rate = *((real *)p[i++]);
  int peek_top_k = *((int *)p[i++]);
  Vocabulary *vcb = p[i++];
  long int *fpass = p[i++];
  int *size = p[i++];
  PeekSet *ps = sid_peek_build(text_file_path, if_lower, if_rm_trail_punc, fbeg,
                               fend, sample_rate, peek_top_k, vcb, fpass, size);
  p[i++] = ps;
  return NULL;
}

PeekSet *PeekBuild(char *text_file_path, int if_lower, int if_rm_trail_punc,
                   real sample_rate, int peek_top_k, Vocabulary *vcb,
                   int thread_num) {
  if (peek_top_k >= vcb->size) {
    LOG(0, "[error]: peek top_k must be smaller than vocabulary size\n");
    exit(1);
  }
  // sample by sample_rate amount of sentences
  long int fsz = FileSize(text_file_path);
  int size = 0, stride = 11, i, j, k = 0;
  long int fpass = 0;
  void **parameters = (void **)malloc(thread_num * stride * sizeof(void *));
  long int *fbeg = (long int *)malloc(thread_num * sizeof(long int));
  long int *fend = (long int *)malloc(thread_num * sizeof(long int));
  for (i = 0; i < thread_num; i++) {
    j = 0;
    fbeg[i] = ((real)fsz) / thread_num * i;
    fend[i] = ((real)fsz) / thread_num * (i + 1);
    parameters[i * stride + j++] = text_file_path;
    parameters[i * stride + j++] = &if_lower;
    parameters[i * stride + j++] = &if_rm_trail_punc;
    parameters[i * stride + j++] = fbeg + i;
    parameters[i * stride + j++] = fend + i;
    parameters[i * stride + j++] = &sample_rate;
    parameters[i * stride + j++] = &peek_top_k;
    parameters[i * stride + j++] = vcb;
    parameters[i * stride + j++] = &fpass;
    parameters[i * stride + j++] = &size;
    parameters[i * stride + j++] = NULL;
  }
  // schedule thread jobs
  pthread_t *pt = (pthread_t *)malloc(thread_num * sizeof(pthread_t));  // >>
  for (i = 0; i < thread_num; i++)
    pthread_create(&pt[i], NULL, sid_peek_build_thread,
                   (void *)(parameters + i * stride));
  for (i = 0; i < thread_num; i++) pthread_join(pt[i], NULL);
  PeekSet *ps = (PeekSet *)malloc(sizeof(PeekSet));
  PeekSet *tps;
  ps->size = 0;
  for (i = 0; i < thread_num; i++) {
    tps = (PeekSet *)(parameters[(i + 1) * stride - 1]);
    ps->size += tps->size;
  }
  ps->wids = (int **)malloc(ps->size * sizeof(int *));
  ps->wnum = (int *)malloc(ps->size * sizeof(int));
  for (i = 0; i < thread_num; i++) {
    tps = (PeekSet *)(parameters[(i + 1) * stride - 1]);
    for (j = 0; j < tps->size; j++) {
      ps->wids[k] = NumCloneIntVec(tps->wids[j], tps->wnum[j]);
      ps->wnum[k] = tps->wnum[j];
      k++;
    }
    PeekSetFree(tps);
  }
  ps->top_k = peek_top_k;
  ps->top_w = (int *)malloc(ps->top_k * sizeof(int));
  for (i = 0; i < peek_top_k; i++) ps->top_w[i] = i;
  LOGCR(1);
  LOG(1, "[Peek] Build %d Sentence (%d)\n", ps->size, size);
  free(parameters);
  free(fbeg);
  free(fend);
  free(pt);
  return ps;
}

real sid_peek_log_likelihood(int k, real *s, int n) {
  // used for computing perplexity, since softmax does not work
  real lpf = 0, ll;
  int i;
  real m = MAX(s, n);
  for (i = 0; i < n; i++) lpf += exp(s[i] - m);
  lpf = log(lpf);
  ll = s[k] - m - lpf;
  return ll;
}

real sid_peek_eval(Model *m, PeekSet *ps, int c, int beg, int end, int *pn,
                   int *npass) {
  // evaluate on peek set
  // to speed up, we constrain competitive target words from top 10k words
  // evaluation on perplexity
  int i, j, k, s, lt, rt, md, tpass = 0;
  int *ids, l, h0n;
  real *p = (real *)malloc(ps->top_k * sizeof(real));
  real h0[NUP], h[NUP], hw, all = 0;
  *pn = 0;
  for (i = beg; i < end; i++) {
    tpass++;
    if (tpass == 5 || i == end - 1) {
      *npass += tpass;
      tpass = 0;
      LOGCLR(2);
      LOG(2, "[PEEK]: Eval completed %.2lf%% (sentences %d / %d, words %d)",
          (real)(*npass) * 100 / ps->size, *npass, ps->size, *pn);
    }
    ids = ps->wids[i];
    l = ps->wnum[i];
    h0n = 0;
    NumFillZeroVec(h0, m->n);
    for (j = 0; j < SMALLER(l, c); j++) {
      NumVecAddCVec(h0, m->scr + ids[j] * m->n, 1, m->n);
      h0n++;
    }
    for (j = 0; j < l; j++) {
      lt = j - c - 1;
      rt = j + c;
      md = j;
      if (rt < l) {
        NumVecAddCVec(h0, m->scr + ids[rt] * m->n, 1, m->n);
        h0n++;
      }
      if (lt >= 0) {
        NumVecAddCVec(h0, m->scr + ids[lt] * m->n, -1, m->n);
        h0n--;
      }
      hw = 1.0 / (h0n - 1.0);
      NumAddCVecDVec(h0, m->scr + ids[md] * m->n, hw, -hw, m->n, h);
      for (k = 0; k < ps->top_k; k++)
        if (ids[j] == ps->top_w[k]) break;
      if (k != ps->top_k) {  // only measure for the top_k words
        for (s = 0; s < ps->top_k; s++)
          p[s] = NumVecDot(h, m->tar + ps->top_w[s] * m->n, m->n);
        NumSoftMax(p, 1, ps->top_k);
        /* all -= sid_peek_log_likelihood(k, p, ps->top_k); */
        /* NumSoftMax(p, 1, ps->top_k); */
        /* printf("%e \n", p[k]); */
        all += p[k];
        (*pn)++;
      }
    }
  }
  all /= (*pn);
  free(p);
  return all;
}

void *sid_peek_eval_thread(void *param) {
  void **p = (void **)param;
  int i = 0;
  Model *m = p[i++];
  PeekSet *ps = p[i++];
  int c = *((int *)p[i++]);
  int beg = *((int *)p[i++]);
  int end = *((int *)p[i++]);
  int *pn = p[i++];
  int *npass = p[i++];
  real avgp = sid_peek_eval(m, ps, c, beg, end, pn, npass);
  *((real *)p[i++]) = avgp;
  return NULL;
}

real PeekEval(Model *m, PeekSet *ps, int c, int thread_num) {
  int i, j, num = 0, stride = 8, npass = 0;
  real prob = 0;
  void **parameters = (void **)malloc(thread_num * stride * sizeof(void *));
  int *beg = (int *)malloc(thread_num * sizeof(int));
  int *end = (int *)malloc(thread_num * sizeof(int));
  real *avgp = (real *)malloc(thread_num * sizeof(real));
  int *pn = (int *)malloc(thread_num * sizeof(int));
  for (i = 0; i < thread_num; i++) {
    j = 0;
    beg[i] = ((real)ps->size) / thread_num * i;
    end[i] = ((real)ps->size) / thread_num * (i + 1);
    parameters[i * stride + j++] = m;
    parameters[i * stride + j++] = ps;
    parameters[i * stride + j++] = &c;
    parameters[i * stride + j++] = beg + i;
    parameters[i * stride + j++] = end + i;
    parameters[i * stride + j++] = pn + i;
    parameters[i * stride + j++] = &npass;
    parameters[i * stride + j++] = avgp + i;
  }
  // schedule thread jobs
  pthread_t *pt = (pthread_t *)malloc(thread_num * sizeof(pthread_t));  // >>
  for (i = 0; i < thread_num; i++)
    pthread_create(&pt[i], NULL, sid_peek_eval_thread,
                   (void *)(parameters + i * stride));
  for (i = 0; i < thread_num; i++) pthread_join(pt[i], NULL);
  printf("\n");
  for (i = 0; i < thread_num; i++) {
    num += pn[i];
    prob += pn[i] * avgp[i];
    printf("i=%d %.2e prob=%.2e, num=%d \n", i, avgp[i], prob, num);
  }
  free(parameters);
  free(beg);
  free(end);
  free(avgp);
  free(pt);
  /* return exp(prob / num); */
  real rtavgp = prob / num;
  real rtavgll = log(rtavgp);
  return rtavgll;
}

real PeekEvalSingleThread(Model *m, PeekSet *ps, int c) {
  int pn, npass = 0;
  real avgp = sid_peek_eval(m, ps, c, 0, ps->size, &pn, &npass);
  return avgp;
}

#endif /* ifndef PEEK */
