///////////////////////////////////////////////////////////////////////////////
// Copyright 2014- Facebook. All rights reserved.                            //
// (Xiaolong Wang, xlwang@fb.com)                                            //
//  Licensed under the Apache License, Version 2.0 (the "License");          //
//  you may not use this file except in compliance with the License.         //
//  You may obtain a copy of the License at                                  //
//                                                                           //
//      http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                           //
//  Unless required by applicable law or agreed to in writing, software      //
//  distributed under the License is distributed on an "AS IS" BASIS,        //
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
//  See the License for the specific language governing permissions and      //
//  limitations under the License.                                           //
///////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define NO_MAIN_BLOCK

///////////////////////////////////////////////////////
// Precision (for atomic read/write use float 32bit) //
///////////////////////////////////////////////////////
#define real double

//////////////////////////
// constants and macros //
//////////////////////////
#define s3eVocabHashSize (0x3FFFFFL)  // ~4M (22 bits of 1)
#define s3eSquashTableRange 10
#define s3eSquashTableSize 2000000  // spanned over [-10 - 10]
#define s3eMaxDimension 1024
#define s3eMaxHeapSize 16
#define s3eMaxSentenceLength 64
#define s3eMaxStringLength 256
#define s3eL2PenaltyWeight 0

#define EXP(x)                                                             \
  ((fabs((x)) < s3eSquashTableRange)                                       \
       ? s3eExpTable[(int)(((real)(x) / s3eSquashTableRange / 2.0 + 0.5) * \
                           s3eSquashTableSize)]                            \
       : 0)
#define SIGM(x)                                                            \
  (((x) <= -s3eSquashTableRange)                                           \
       ? 0                                                                 \
       : (((x) >= s3eSquashTableRange)                                     \
              ? 1                                                          \
              : s3eSigmTable                                               \
                    [(int)(((real)(x) / s3eSquashTableRange / 2.0 + 0.5) * \
                           s3eSquashTableSize)]))
#define TANH(x)                                                            \
  (((x) <= -s3eSquashTableRange)                                           \
       ? -1                                                                \
       : (((x) >= s3eSquashTableRange)                                     \
              ? 1                                                          \
              : s3eTanhTable                                               \
                    [(int)(((real)(x) / s3eSquashTableRange / 2.0 + 0.5) * \
                           s3eSquashTableSize)]))
#define MAX(arr, l)                                                          \
  ({                                                                         \
    __typeof__(arr[0]) tmp_max_x = arr[0];                                   \
    int tmp_max_i;                                                           \
    for (tmp_max_i = 1; tmp_max_i < l; tmp_max_i++)                          \
      tmp_max_x = (tmp_max_x < arr[tmp_max_i]) ? arr[tmp_max_i] : tmp_max_x; \
    tmp_max_x;                                                               \
  })
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define ABSMAX(arr, l)                                                         \
  ({                                                                           \
    __typeof__(arr[0]) tmp_max_x = ABS(arr[0]);                                \
    int tmp_max_i;                                                             \
    for (tmp_max_i = 1; tmp_max_i < l; tmp_max_i++)                            \
      tmp_max_x =                                                              \
          (tmp_max_x < ABS(arr[tmp_max_i])) ? ABS(arr[tmp_max_i]) : tmp_max_x; \
    tmp_max_x;                                                                 \
  })
#define ABSSUM(arr, l)                           \
  ({                                             \
    __typeof__(arr[0]) tmp_ss_x = 0;             \
    int tmp_ss_i;                                \
    for (tmp_ss_i = 0; tmp_ss_i < l; tmp_ss_i++) \
        tmp_ss_x += ABS(arr[tmp_ss_i]);          \
    tmp_ss_x;                                    \
  })
#define CADD(arr, l, x)                                                      \
  ({                                                                         \
    int tmp_cadd_i;                                                          \
    for (tmp_cadd_i = 0; tmp_cadd_i < l; tmp_cadd_i++) arr[tmp_cadd_i] += x; \
  })
#define SUM(arr, l)                                 \
  ({                                                \
    __typeof__(arr[0]) tmp_sum_s = 0;               \
    int tmp_sum_i;                                  \
    for (tmp_sum_i = 0; tmp_sum_i < l; tmp_sum_i++) \
        tmp_sum_s += arr[tmp_sum_i];                \
    tmp_sum_s;                                      \
  })
#define SWAP(x, y)                \
  ({                              \
    __typeof__(x) tmp_swap_z = x; \
    x = y;                        \
    y = tmp_swap_z;               \
  })
#define SPAN_ID(l, b, e) \
  ((int)((b) + (2 * (l) + 2 - (e) + (b)) * ((e) - (b) - 1) / 2))
#define STR_CLONE(d, s)                                \
  ({                                                   \
    d = (char*)malloc((strlen(s) + 1) * sizeof(char)); \
    strcpy(d, s);                                      \
  })
#define LOG(dbg_level, ...)        \
  ({                               \
    if (s3eDbgMode >= dbg_level) { \
      printf(__VA_ARGS__);         \
      fflush(stdout);              \
    }                              \
  })
#define RAND(s)                                        \
  ({                                                   \
    s = ((s * 0x5DEECE66DL + 0x0A) & 0xFFFFFFFFFFFFL); \
    ((real)s / ((real)0xFFFFFFFFFFFFL));               \
  })
#define SQ(x) ((x) * (x))
#define RAND_PAIR(x, y, s, l) \
  ({                          \
    x = RAND(s) * l;          \
    y = RAND(s) * (l - 1);    \
    if (y >= x) y++;          \
  })

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

/////////////////////////
// User defined config //
/////////////////////////
int s3eVocabCapacity = 1000000;  // upperbound of input text unique words
int s3eVocabSize = 1000000;      // training vocabulary size
int s3eN = 100;                  // semantic embedding size
int s3eM = 10;                   // syntactic embedding size
int s3eR = 5;
int s3eBestK = 5;          // CKY best-k
int s3eDbgMode = 2;        // debug print mode: 2:full, 1:brief, 0:none
int s3eNormWord = 1;       // 1 if normalize word, 0 if already tokenized
int s3eThreadNum = 16;     // threads number
int s3eMaxIterNum = 10;    // training iteration number
int s3eLoadVocab = 0;      // 1 if loading vocabulary from file
real s3eSampleRate = 0.2;  // sample rate (words permuted per sentence)
real s3eInitLr = 1.0;      // Initial learning rate
real s3eDecay = 0.5;       // scoring decay
int s3eDynDecay = 0;       // scoring dynamic decay
int s3eAdaGrad = 0;        // use adagrad vs grad descent
int s3eLrShrink = 1;       // learning rate shrink by iterations
int s3eIntModelSave = 0;   // save intermediate model
int s3eBsz = 100;          // mini batch size for gradient descent
// File
char text_file_path[s3eMaxStringLength] = "train.txt";   // training file
char model_file_path[s3eMaxStringLength] = "s3e.model";  // save model to file
char vocab_file_path[s3eMaxStringLength] = "s3e.vocab";  // vocabulary to file

/////////////////////
// Global Constant //
/////////////////////
// Mode size
int SMN_SIZE, DSMN_SIZE, SYN_SIZE;

real adag_eps = 1e-6;
// Preprint Squashing table
real* s3eExpTable, *s3eSigmTable, *s3eTanhTable;
// File size, parsed num, progress, effective corpus words count
long long int file_size = -1, eff_corpus_w_cnt = 0;
long long int prd_sent_num = 0, prd_word_num = 0;
real progress = 0, speed = 0, accuracy = 0.5;
// Intermediate model save schedule
real* save_thread_start, *save_thread_cnt;
real save_thread_interval;
// time
clock_t t0;
// debug
int g_sent_cnt = 0;
int t_sent_cnt[100] = {0};
// random seed
unsigned long long int seed = 0x0F0F0F0FL;
// negative unigram distribution
int* neg_unigram;

#include "s3e_misc.c"
#include "s3e_text.c"
struct Param* MDL;
struct Param* VAR;
#include "s3e_debug.c"
#include "s3e_num.c"

//////////////////
// CKY Decoding //
//////////////////

real CKYDecodeEvalOneSentence(int* word_ids, int word_num, real dc,
                              struct Bookkeeping* bkkp, struct Heap* heap) {
  int i, k, b, m, e, p, pl, pr, c, cl, cr, nl, nr, kl, kr;
  real v[s3eMaxDimension], w[s3eMaxDimension], aw1[s3eMaxDimension],
      aw2[s3eMaxDimension];
  real score;
  for (i = 0; i < word_num; i++) {
    p = SPAN_ID(word_num, i, (i + 1));
    c = p * s3eBestK;
    LookupTableEval(word_ids[i], bkkp->v + c * s3eM, bkkp->w + c * s3eN);
    bkkp->s[c] = 0;
    bkkp->l[c] = -1;
    bkkp->r[c] = word_ids[i];
    bkkp->num[p] = 1;
  }
  for (i = 2; i <= word_num; i++)
    for (b = 0; b <= word_num - i; b++) {
      e = b + i;
      p = SPAN_ID(word_num, b, e);
      heap->size = 0;
      for (k = 0; k < s3eBestK; k++) heap->key[k] = k;
      for (m = b + 1; m < e; m++) {
        pl = SPAN_ID(word_num, b, m);
        pr = SPAN_ID(word_num, m, e);
        nl = bkkp->num[pl];
        nr = bkkp->num[pr];
        for (kl = 0; kl < nl; kl++)
          for (kr = 0; kr < nr; kr++) {  // v, w, s, score, l, r, num
            cl = pl * s3eBestK + kl;
            cr = pr * s3eBestK + kr;
            score = CompositionEval(bkkp->v + cl * s3eM, bkkp->v + cr * s3eM,
                                    bkkp->w + cl * s3eN, bkkp->w + cr * s3eN, v,
                                    w, aw1, aw2) +
                    dc * (bkkp->s[cl] + bkkp->s[cr]);
            if ((c = HeapInsert(heap, score)) != -1) {
              c += p * s3eBestK;
              memcpy(bkkp->v + c * s3eM, v, s3eM * sizeof(real));
              memcpy(bkkp->w + c * s3eN, w, s3eN * sizeof(real));
              memcpy(bkkp->aw1 + c * s3eN, aw1, s3eN * sizeof(real));
              memcpy(bkkp->aw2 + c * s3eN, aw2, s3eN * sizeof(real));
              bkkp->s[c] = score;
              bkkp->l[c] = cl;
              bkkp->r[c] = cr;
            }
          }
      }
      bkkp->num[p] = heap->size;
    }
  p = SPAN_ID(word_num, 0, word_num);
  bkkp->sentence_bkkp_entry = p * s3eBestK;
  bkkp->sentence_score = bkkp->s[p * s3eBestK];
  for (k = 1; k < bkkp->num[p]; k++)
    if (bkkp->s[k + p * s3eBestK] > bkkp->sentence_score) {
      bkkp->sentence_score = bkkp->s[k + p * s3eBestK];
      bkkp->sentence_bkkp_entry = k + p * s3eBestK;
    }
  return bkkp->sentence_score;
}

void CKYDecodeAccGradOneSentence(int* word_ids, int word_num, real gs, real lr,
                                 real dc, struct Bookkeeping* bkkp,
                                 struct Grad* gd, struct Grad* bgd) {
  int c, cl, cr, hl, hr;
  ZeroBkkp(bkkp);
  ZeroGrad(gd);
  bkkp->gs[bkkp->hd] = gs;
  bkkp->q[bkkp->hd++] = bkkp->sentence_bkkp_entry;
  while (bkkp->hd != bkkp->tl) {
    c = bkkp->q[bkkp->tl];
    cl = bkkp->l[c];
    cr = bkkp->r[c];
    if (cl != -1) {  // composition
      hl = bkkp->hd++;
      hr = bkkp->hd++;
      bkkp->q[hl] = cl;
      bkkp->q[hr] = cr;
      CompositionAccGrad(
          bkkp->v + cl * s3eM, bkkp->v + cr * s3eM, bkkp->w + cl * s3eN,
          bkkp->w + cr * s3eN, bkkp->v + c * s3eM, bkkp->w + c * s3eN,
          bkkp->aw1 + c * s3eN, bkkp->aw2 + c * s3eN, bkkp->s[c],
          bkkp->gv + bkkp->tl * s3eM, bkkp->gw + bkkp->tl * s3eN,
          bkkp->gs[bkkp->tl], gd->gm, bkkp->gv + hl * s3eM,
          bkkp->gv + hr * s3eM, bkkp->gw + hl * s3eN, bkkp->gw + hr * s3eN);
      bkkp->gs[hl] = dc * bkkp->gs[bkkp->tl];
      bkkp->gs[hr] = dc * bkkp->gs[bkkp->tl];
    } else {  // lookuptable
      gd->idx[gd->idx_size] = cr;
      LookupTableAccGrad(gd->idx[gd->idx_size], bkkp->v + c * s3eM,
                         bkkp->w + c * s3eN, bkkp->gv + bkkp->tl * s3eM,
                         bkkp->gw + bkkp->tl * s3eN,
                         gd->gm->synlut + gd->idx_size * s3eM,
                         gd->gm->smnlut + gd->idx_size * s3eN);
      gd->idx_size++;
    }
    bkkp->tl++;
  }
  BatchUpdateParam(gd->gm, bgd->gm, VAR, lr, s3eL2PenaltyWeight, gd->idx,
                   bgd->idx, &(gd->idx_size), &(bgd->idx_size));
  // LOG(2, "%s\n", ToStringParam(bgd->gm));
  // LOG(2, "%s\n", ToStringParam(gd->gm));
  if ((++(bgd->cnt)) == s3eBsz) {
    BatchUpdateParam(bgd->gm, MDL, 0L, -1.0, 0.0, bgd->idx, 0L,
                     &(bgd->idx_size), 0L);
    ZeroGrad(bgd);
  }
  return;
}

void CKYTrainOneSentence(int* words, int* words2, int wn, real lr, real dc,
                         struct Bookkeeping* bkkp, struct Bookkeeping* bkkp2,
                         struct Heap* heap, struct Heap* heap2, struct Grad* gd,
                         struct Grad* bgd, long long int* correct_num,
                         long long int* wrong_num) {
  int i, j, k, perm_num, flip_num;
  real score, score2, plr, nlr;
  plr = 0.0;
  nlr = lr;
  score = CKYDecodeEvalOneSentence(words, wn, dc, bkkp, heap);
  perm_num = wn * s3eSampleRate;
  flip_num = wn * s3eSampleRate;
  if (perm_num == 0) perm_num = 1;
  if (flip_num == 0) flip_num = 1;
  for (i = 0; i < perm_num + flip_num; i++) {
    if (i < perm_num) {
      RAND_PAIR(j, k, seed, wn);
      Perm_Gen(j, k, words, words2, wn);
    } else {
      j = RAND(seed) * wn;
      k = SampleNegUnigram(seed);
      Flip_Gen(j, k, words, words2, wn);
    }
    score2 = CKYDecodeEvalOneSentence(words2, wn, dc, bkkp2, heap2);
    if (score < score2 + 1.0) {
      CKYDecodeAccGradOneSentence(words2, wn, 1.0, nlr, dc, bkkp2, gd, bgd);
      plr += nlr;
    }
    if (score <= score2)
      (*wrong_num)++;
    else
      (*correct_num)++;
  }
  if (plr != 0)
    CKYDecodeAccGradOneSentence(words, wn, -1.0, plr, dc, bkkp, gd, bgd);
  return;
}

//////////////////////////////
// Multi-Threaded Training  //
//////////////////////////////
void InitModel() {
  InitConstant();
  AllocParam(&MDL, vocab.size);
  InitParam(MDL, vocab.size, 1);
  if (s3eAdaGrad) {
    AllocParam(&VAR, vocab.size);
    InitParam(VAR, vocab.size, 0);
  } else
    VAR = 0L;
  return;
}

void SaveModel(int id) {
  FILE* fout;
  char txt_m_file[s3eMaxStringLength], bin_m_file[s3eMaxStringLength];
  strcpy(txt_m_file, model_file_path);
  strcpy(bin_m_file, model_file_path);
  if (id != 0) {
    sprintf(txt_m_file + strlen(txt_m_file), ".p%d", id);
    sprintf(bin_m_file + strlen(bin_m_file), ".p%d", id);
  }
  // strcat(txt_m_file, ".txt");
  strcat(bin_m_file, ".bin");
  // fout = fopen(txt_m_file, "wb");
  fout = fopen(bin_m_file, "wb");
  SaveParam(MDL, vocab.size, fout);
  fclose(fout);
  return;
}

void ScheduleSaveModel() {
  int i, j, k;
  real save_interval;
  save_thread_start = (real*)malloc(s3eThreadNum * sizeof(real));
  save_thread_cnt = (real*)malloc(s3eThreadNum * sizeof(real));
  j = s3eIntModelSave / s3eThreadNum;
  k = s3eIntModelSave % s3eThreadNum;
  for (i = 0; i < s3eThreadNum; i++) save_thread_cnt[i] = j;
  for (i = 0; i < k; i++) save_thread_cnt[i]++;
  if (s3eIntModelSave != 0) {
    save_interval = 100.0 / s3eIntModelSave;
    for (i = 0; i < s3eThreadNum; i++)
      save_thread_start[i] = (i + 1) * save_interval;
    save_thread_interval = s3eThreadNum * save_interval;
  }
  return;
}

void* ThreadedTrain(void* arg) {
  int i;
  clock_t t1;
  struct Bookkeeping* bkkp =
      (struct Bookkeeping*)malloc(sizeof(struct Bookkeeping));
  struct Bookkeeping* bkkp2 =
      (struct Bookkeeping*)malloc(sizeof(struct Bookkeeping));
  struct Heap* heap = (struct Heap*)malloc(sizeof(struct Heap));
  struct Heap* heap2 = (struct Heap*)malloc(sizeof(struct Heap));
  struct Grad* gd = (struct Grad*)malloc(sizeof(struct Grad));
  struct Grad* bgd = (struct Grad*)malloc(sizeof(struct Grad));
  if (!bkkp || !bkkp2 || !heap || !heap2 || !gd || !bgd) {
    LOG(0, "error allocating grad, bkkps or heaps");
    exit(1);
  }
  InitBkkp(bkkp);
  InitBkkp(bkkp2);
  InitGrad(gd);
  InitGrad(bgd);
  int words[s3eMaxSentenceLength], words2[s3eMaxSentenceLength], wn, iter;
  real lr, int_accuracy, dc;
  long long int thread_id, seed, fbeg, fend, fpos, int_s_num, int_w_num,
      int_correct_num, int_wrong_num;
  thread_id = (long)arg;
  FILE* fin = fopen(text_file_path, "rb");
  if (!fin) {
    LOG(0, "Error!");
    exit(1);
  }
  fbeg = file_size * thread_id / s3eThreadNum;
  fend = file_size * (thread_id + 1) / s3eThreadNum;
  seed = thread_id;  // run random seed past cold start
  for (i = 0; i < 100; i++) RAND(seed);
  lr = s3eInitLr;
  dc = s3eDecay;
  for (iter = 0; iter < s3eMaxIterNum; iter++) {
    fseek(fin, fbeg, SEEK_SET);
    int_s_num = int_w_num = int_correct_num = int_wrong_num = 0;
    fpos = -1;
    while (1) {
      t_sent_cnt[thread_id] = g_sent_cnt++;
      wn = LoadSent(fin, words);
      if (wn >= 2)
        CKYTrainOneSentence(words, words2, wn, lr, dc, bkkp, bkkp2, heap, heap2,
                            gd, bgd, &int_correct_num, &int_wrong_num);
      int_s_num++;
      int_w_num += wn;
      if (int_s_num == 100) {
        prd_sent_num += int_s_num;
        prd_word_num += int_w_num;
        int_accuracy =
            (int_correct_num + 1e-6) / (int_correct_num + int_wrong_num + 1e-6);
        int_s_num = int_w_num = int_correct_num = int_wrong_num = 0;
        t1 = clock();
        fpos = ftell(fin);
        progress = 100.0 * prd_word_num / eff_corpus_w_cnt / s3eMaxIterNum;
        speed = prd_sent_num / ((t1 - t0) / (real)CLOCKS_PER_SEC);
        accuracy = 0.99 * accuracy + 0.01 * int_accuracy;
        if (s3eLrShrink) {
          lr = s3eInitLr * (1 - progress / 100.0);
          if (lr < 1e-2 * s3eInitLr) lr = 1e-2 * s3eInitLr;
        }
        if (s3eDynDecay) {
          dc = 0.5 + (s3eDecay - 0.5) * (1 - progress / 100.0);
        }
        if (save_thread_cnt[thread_id] > 0) {
          if (progress >= save_thread_start[thread_id]) {
            // add small value to fix numeric precision loss
            SaveModel((int)(save_thread_start[thread_id] /
                                ((real)(save_thread_interval / s3eThreadNum)) +
                            1e-6));
            save_thread_cnt[thread_id]--;
            save_thread_start[thread_id] += save_thread_interval;
            if (save_thread_start[thread_id] > 100)
              save_thread_start[thread_id] = 100;
          }
        }
        // LOG(2,
        //   "Sent/Word [%8.5lfM, %8.5lfM] "
        //   "Lr [%6e], Sent/Sec/Thread [%6.3lf], Progress [%5.2lf%%], "
        //   "Accuracy [%6.3f] \r",
        //   prd_sent_num / 1000000.0, prd_word_num / 1000000.0,
        //   lr, speed, progress, accuracy
        //   );
        LOG(2, "[%2lld@%3d:%5.2lf%%:%8.6lf]: %s A %5.3lf \r", thread_id, iter,
            progress, lr, ToStringParam(MDL), accuracy);
      }
      if (t_sent_cnt[thread_id] % 100000 == 0) {
        LOG(2, "%s[%2lld@%3d:%5.2lf%%:%8.6lf]: %s A %5.3lf %s \n",
            ANSI_COLOR_YELLOW, thread_id, iter, progress, lr,
            ToStringParam(MDL), accuracy, ANSI_COLOR_RESET);
      }
      if (fpos >= fend || feof(fin)) break;
    }
  }
  FreeBkkp(bkkp);
  FreeBkkp(bkkp2);
  free(heap);
  free(heap2);
  FreeGrad(gd);
  FreeGrad(bgd);
  pthread_exit(NULL);
  return 0;
}

void Train() {
  long i;  // 64 bit
  // Prepping
  if (s3eLoadVocab)
    LoadVocab();
  else {
    BuildVocab();   // file_size
    ReduceVocab();  // eff_corpus_w_cnt
    SaveVocab();
  }
  if (file_size == -1) {
    FILE* fin = fopen(text_file_path, "rb");
    fseek(fin, 0, SEEK_END);
    file_size = ftell(fin);
    fclose(fin);
  }
  LOG(1, "[Prep]: Initialize Negative Distribution\n");
  InitNegUnigram();
  LOG(1, "[Prep]: Initialize Squashing Table\n");
  InitSquashTable();
  LOG(1, "[Prep]: Initialize Model\n");
  InitModel();
  ScheduleSaveModel();
  LOG(1, "[Prep]: Initialize Threads\n");
  pthread_t* pt = (pthread_t*)malloc(s3eThreadNum * sizeof(pthread_t));
  t0 = clock();
  prd_word_num = 0;
  prd_sent_num = 0;
  for (i = 0; i < s3eThreadNum; i++)
    pthread_create(&pt[i], NULL, ThreadedTrain, (void*)i);
  for (i = 0; i < s3eThreadNum; i++) pthread_join(pt[i], NULL);
  LOG(2, "\n");
  LOG(1, "[Save]: Training finished. Save model to %s(.bin, .txt)\n",
      model_file_path);
  SaveModel(0);
  return;
}

///////////////////////////
// Command Line Interface //
///////////////////////////
int GetOptPos(char* str, int argc, char** argv) {
  int i;
  for (i = 1; i < argc; i++)
    if (strcmp(str, argv[i]) == 0) return i;
  return -1;
}

void Cmd(int argc, char** argv) {
  if (argc == 2 && strcmp("-help", argv[1]) == 0) {
    LOG(0, "[S3E]: Semantic-Syntactic Sentence Embedding \n\n");
    LOG(0, "Options \n");
    LOG(0, "\t-smn          <int>\n");
    LOG(0, "\t\tSemantic embedding dimension; default is 100\n");
    LOG(0, "\t-syn          <int>\n");
    LOG(0, "\t\tSynatic embedding dimension; default is 10\n");
    LOG(0, "\t-vocab-size   <int>\n");
    LOG(0, "\t\tVocabulary size for training; set to -1 if no reduction");
    LOG(0, "\t\tdefault is 10000");
    LOG(0, "\t-vocab-cap    <int>\n");
    LOG(0, "\t\tTraining data unique words upperbound; default is 1M\n");
    LOG(0, "\t-bestk        <int>\n");
    LOG(0, "\t\tCKY besk-k; default is 5\n");
    LOG(0, "\t-debug        <int>\n");
    LOG(0, "\t\tDebug mode to control information to print; default is 2\n");
    LOG(0, "\t\t0: suppress printing; 1: brief printing; 2: full printing\n");
    LOG(0, "\t-word-norm    <int>\n");
    LOG(0, "\t\tSimple tokenization, normalization is provided for\n");
    LOG(0, "\t\tunprocessed text. To disable it, set to 0; default is 1\n");
    LOG(0, "\t-threads      <int>\n");
    LOG(0, "\t\tNumber of threads for training; default is 16\n");
    LOG(0, "\t-iters        <int>\n");
    LOG(0, "\t\tNumber of iterations for training; default is 100\n");
    LOG(0, "\t-load-vocab   <int>\n");
    LOG(0, "\t\tLoad (instead of build) vocabulary from file; default is 0\n");
    LOG(0, "\t\tTo enable, set to 1\n");
    LOG(0, "\t-sample-rate  <real>\n");
    LOG(0, "\t\tSample rate to construct negative examples; default is 0.2\n");
    LOG(0, "\t-init-lr      <real>\n");
    LOG(0, "\t\tInitial learning rate; default is 0.1\n");
    LOG(0, "\t-train        <file>\n");
    LOG(0, "\t\ttraining file path;\n");
    LOG(0, "\t-model        <file>\n");
    LOG(0, "\t\tfile path to save model; default is \"s3e.model\"\n");
    LOG(0, "\t-vocab        <file>\n");
    LOG(0, "\t\tfile path to save vocabulary; default is \"s3e.vocab\"\n");
    LOG(0, "\t-score-decay  <real>\n");
    LOG(0, "\t\tdecaying score for lower levels; default is 0.5\n");
    LOG(0, "\t-dynamic-score-decay  <int>\n");
    LOG(0, "\t\tdynamic decaying score for lower levels; default is 0\n");
    LOG(0, "\t-adagrad      <int>\n");
    LOG(0, "\t\tUse Adagrad instead of gradient descent; default is 0\n");
    LOG(0, "\t-shrink-lr    <int>\n");
    LOG(0, "\t\tShrink learning rate by iterations; default is 1\n");
    LOG(0, "\t-int-save     <int>\n");
    LOG(0, "\t\tNumber to save while training; default is 0\n");
    exit(0);
  }
  int i;
  if (argc > 2) LOG(0, "User-defined options:\n");
  if ((i = GetOptPos("-vocab-cap", argc, argv)) > 0) {
    s3eVocabCapacity = atoi(argv[i + 1]);
    LOG(0, "Vocabulary cap             : %d\n", s3eVocabCapacity);
  }
  if ((i = GetOptPos("-vocab-size", argc, argv)) > 0) {
    s3eVocabSize = atoi(argv[i + 1]);
    LOG(0, "vocabulary size            : %d\n", s3eVocabSize);
  }
  if ((i = GetOptPos("-smn", argc, argv)) > 0) {
    s3eN = atoi(argv[i + 1]);
    LOG(0, "Semantic space dimension   : %d\n", s3eN);
  }
  if ((i = GetOptPos("-syn", argc, argv)) > 0) {
    s3eM = atoi(argv[i + 1]);
    LOG(0, "Syntactic space dimension  : %d\n", s3eM);
  }
  if ((i = GetOptPos("-bestk", argc, argv)) > 0) {
    s3eBestK = atoi(argv[i + 1]);
    LOG(0, "CKY best K                 : %d\n", s3eBestK);
  }
  if ((i = GetOptPos("-debug", argc, argv)) > 0) {
    s3eDbgMode = atoi(argv[i + 1]);
    LOG(0, "Debug mode                 : %d\n", s3eDbgMode);
  }
  if ((i = GetOptPos("-word-norm", argc, argv)) > 0) {
    s3eNormWord = atoi(argv[i + 1]);
    LOG(0, "Word normalization mode    : %d\n", s3eNormWord);
  }
  if ((i = GetOptPos("-threads", argc, argv)) > 0) {
    s3eThreadNum = atoi(argv[i + 1]);
    LOG(0, "Thread number              : %d\n", s3eThreadNum);
  }
  if ((i = GetOptPos("-iters", argc, argv)) > 0) {
    s3eMaxIterNum = atoi(argv[i + 1]);
    LOG(0, "Max iteration number       : %d\n", s3eMaxIterNum);
  }
  if ((i = GetOptPos("-load-vocab", argc, argv)) > 0) {
    s3eLoadVocab = atoi(argv[i + 1]);
    LOG(0, "Vocabulary load mode       : %d\n", s3eLoadVocab);
  }
  if ((i = GetOptPos("-sample-rate", argc, argv)) > 0) {
    s3eSampleRate = atof(argv[i + 1]);
    LOG(0, "Negative sampling rate     : %lf\n", s3eSampleRate);
  }
  if ((i = GetOptPos("-init-lr", argc, argv)) > 0) {
    s3eInitLr = atof(argv[i + 1]);
    LOG(0, "Initial learning rate      : %lf\n", s3eInitLr);
  }
  if ((i = GetOptPos("-train", argc, argv)) > 0) {
    strcpy(text_file_path, argv[i + 1]);
    LOG(0, "Training file path         : %s\n", text_file_path);
  }
  if ((i = GetOptPos("-model", argc, argv)) > 0) {
    strcpy(model_file_path, argv[i + 1]);
    LOG(0, "Model file path to save    : %s\n", model_file_path);
  }
  if ((i = GetOptPos("-vocab", argc, argv)) > 0) {
    strcpy(vocab_file_path, argv[i + 1]);
    LOG(0, "Vocabulary file path       : %s\n", vocab_file_path);
  }
  if ((i = GetOptPos("-score-decay", argc, argv)) > 0) {
    s3eDecay = atof(argv[i + 1]);
    LOG(0, "Scoring decay rate         : %lf\n", s3eDecay);
  }
  if ((i = GetOptPos("-dynamic-score-decay", argc, argv)) > 0) {
    s3eDynDecay = atoi(argv[i + 1]);
    LOG(0, "Scoring dynamic decay      : %d\n", s3eDynDecay);
  }
  if ((i = GetOptPos("-adagrad", argc, argv)) > 0) {
    s3eAdaGrad = atoi(argv[i + 1]);
    LOG(0, "Use Adagrad                : %d\n", s3eAdaGrad);
  }
  if ((i = GetOptPos("-shrink-lr", argc, argv)) > 0) {
    s3eLrShrink = atoi(argv[i + 1]);
    LOG(0, "Shrink learning rate       : %d\n", s3eLrShrink);
  }
  if ((i = GetOptPos("-int-save", argc, argv)) > 0) {
    s3eIntModelSave = atoi(argv[i + 1]);
    LOG(0, "Intermediate model save    : %d\n", s3eIntModelSave);
  }
  return;
}

/////////////////////
// Running Example //
/////////////////////
#ifndef NO_MAIN_BLOCK
int main(int argc, char** argv) {
  Cmd(argc, argv);
  Train();
  return 0;
}
#endif
