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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NO_MAIN_BLOCK
#include "s3e.c"

char test[100];

void LoadModel() {
  char bin_m_file[s3eMaxStringLength];
  strcpy(bin_m_file, model_file_path); strcat(bin_m_file, ".bin");
  FILE *fin = fopen(bin_m_file, "rb");
  // SCR_SIZE = s3eH * (s3eM + 1) * (s3eN + 1);
  MDL = LoadParam(fin);
  fclose(fin);
  return;
}

real *sim;
int CmpSim(const void *x, const void *y) {
   real sx = sim[*((int*)x)], sy = sim[*((int*)y)];
   if(sx > sy) return -1;
   else if(sx == sy) return 0;
   else return 1;
}

void WordSimilarity() {
  int i, j, k, v;
  real syn_sim;
  char word[s3eMaxStringLength];
  sim = (real *) malloc(vocab.size * sizeof(real));
  int *idx = (int *)malloc(vocab.size * sizeof(int));
  real w1[s3eMaxDimension], w2[s3eMaxDimension], nm1, nm2;
  real v1[s3eMaxDimension], v2[s3eMaxDimension];
  InitSquashTable();
  while(1){
    printf("enter a word: ");
    scanf("%s", word);
    for(i = 0; i < vocab.size; i++) if(strcmp(word, vocab.id2wd[i]) == 0) break;
    if(i == vocab.size) {
      printf("%s is not in vocabulary, choose another word or exit\n", word);
      continue;
    }
    for(j = 0; j < vocab.size; j++) idx[j] = j;
    for(k = 0; k < s3eN; k++) w1[k] = TANH(MDL->smnlut[i * s3eN + k]);
    nm1 = sqrt(Dot(w1, w1, s3eN)) + 1e-12;
    for(j = 0; j < vocab.size; j++)
      if(j == i) sim[i] = -1e100;
      else {
        for(k = 0; k < s3eN; k++) w2[k] = TANH(MDL->smnlut[j * s3eN + k]);
        nm2 = sqrt(Dot(w2, w2, s3eN)) + 1e-12;
        sim[j] = Dot(w1, w2, s3eN) / nm1 / nm2;
      }
    qsort(idx, vocab.size, sizeof(int), CmpSim);
    for(j = 0; j < 40; j++) {
      v = idx[j];
      SoftMaxEval(s3eM, MDL->synlut + i * s3eM, v1);
      SoftMaxEval(s3eM, MDL->synlut + v * s3eM, v2);
      syn_sim = Dot(v1, v2, s3eM) / sqrt(Dot(v1, v1, s3eM)) /
                sqrt(Dot(v2, v2, s3eM));
      printf("[%5d]%40s : %10e %10e\n", v, vocab.id2wd[v], sim[v], syn_sim);
    }
    printf("\n");
  }
  return;
}

void SentSimilarity() {
  int i, j, k;
  int MaxSentNum = 1000000;
  FILE *fin = fopen(text_file_path, "rb");
  if(!fin) { LOG(0, "Error!"); exit(1); }
  struct Bookkeeping *bkkp =
                      (struct Bookkeeping *) malloc(sizeof(struct Bookkeeping));
  struct Heap *heap = (struct Heap *) malloc(sizeof(struct Heap));
  InitBkkp(bkkp);
  int **word_ids, *word_num;
  AllocAlignMem("word_ids", (void **)&word_ids,
                (long long) MaxSentNum * sizeof(int*));
  AllocAlignMem("word_num", (void **)&word_num,
                (long long) MaxSentNum * sizeof(int));
  for(i = 0; i < MaxSentNum; i++)
    AllocAlignMem("word_ids[i]", (void **)(word_ids + i),
                  (long long) s3eMaxSentenceLength * sizeof(int));
  real *w_vecs, *v_vecs, *scores;
  AllocAlignMem("w_vecs", (void **)&w_vecs,
                  (long long) MaxSentNum * s3eN * sizeof(real));
  AllocAlignMem("v_vecs", (void **)&v_vecs,
                  (long long) MaxSentNum * s3eM * sizeof(real));
  AllocAlignMem("scores", (void **)&scores,
                  (long long) MaxSentNum * sizeof(real));
  int sent_num;
  i = 0;
  while(!feof(fin)) {
    word_num[i] = LoadSent(fin, word_ids[i]);
    if(i == MaxSentNum) break;
    if(i % 1000 == 0) printf("%d sentences read\r", i);
    i++;
  }
  sent_num = i;
  printf("%d sentences read\n", sent_num);
  for(i = 0; i < sent_num; i++) {
    printf("%d sentences decoded \r", i);
    CKYDecodeEvalOneSentence(word_ids[i], word_num[i], s3eDecay, bkkp, heap);
    j = bkkp->sentence_bkkp_entry;
    scores[i] = bkkp->sentence_score;
    memcpy(w_vecs + i * s3eN, bkkp->w + j * s3eN, s3eN * sizeof(real));
    memcpy(v_vecs + i * s3eM, bkkp->v + j * s3eM, s3eM * sizeof(real));
  }
  printf("%d sentences decoded\n", sent_num);
  int sid;
  int *idx = (int *)malloc(sent_num * sizeof(int));
  sim = (real *) malloc(sent_num * sizeof(real));
  real sim2;
  while(1) {
    printf("enter an sentence index: ");
    scanf("%d", &sid);
    for(k = 0; k < word_num[sid]; k++)
      printf("%s ", vocab.id2wd[word_ids[sid][k]]);
    printf("\n");
    for(k = 0; k < 80; k++) printf("-");
    printf("\n");
    for(i = 0; i < sent_num; i++){
      if(i == sid) sim[i] = 1e-100;
      else sim[i] = Cos(w_vecs + i * s3eN, w_vecs + sid * s3eN, s3eN);
    }
    for(i = 0; i < sent_num; i++) idx[i] = i;
    qsort(idx, sent_num, sizeof(int), CmpSim);
    for(i = 0; i < 40; i++) {
      j = idx[i];
      sim2 = Cos(v_vecs + j * s3eM, v_vecs + sid * s3eM, s3eM);
      printf("[%8d] %10.3e,%10.3e : ", j, sim[j], sim2);
      for(k = 0; k < word_num[j]; k++)
        printf("%s ", vocab.id2wd[word_ids[j][k]]);
      printf("\n");
    }
    printf("\n");
  }
  return;
}

void PhraseWord() {
  int max_sent_len = 100, wn = 0, i, j, k;
  int *wids = (int*) malloc(max_sent_len * sizeof(int));
  real *v, *w, w2[s3eMaxDimension], v2[s3eMaxDimension], syn_sim;
  sim = (real *) malloc(vocab.size * sizeof(real));
  int *idx = (int *)malloc(vocab.size * sizeof(int));
  struct Bookkeeping *bkkp =
                      (struct Bookkeeping *) malloc(sizeof(struct Bookkeeping));
  struct Heap *heap = (struct Heap *) malloc(sizeof(struct Heap));
  InitBkkp(bkkp);
  InitSquashTable();
  while(1) {
    printf("enter a phrase: ");
    wn = LoadSent(stdin, wids);
    CKYDecodeEvalOneSentence(wids, wn, s3eDecay, bkkp, heap);
    i = bkkp->sentence_bkkp_entry;
    w = bkkp->w + i * s3eN;
    v = bkkp->v + i * s3eM;
    for(j = 0; j < vocab.size; j++) idx[j] = j;
    for(j = 0; j < vocab.size; j++) {
      TanhEval(s3eN, MDL->smnlut + j * s3eN, w2);
      sim[j] = Cos(w, w2, s3eN);
    }
    qsort(idx, vocab.size, sizeof(int), CmpSim);
    for(j = 0; j < 40; j++) {
      k = idx[j];
      SoftMaxEval(s3eM, MDL->synlut + k * s3eM, v2);
      syn_sim = Cos(v, v2, s3eM);
      printf("[%5d]%40s : %10e %10e\n", k, vocab.id2wd[k], sim[k], syn_sim);
    }
    printf("\n");
  }
  return;
}

int main(int argc, char **argv) {
  int i;
  Cmd(argc, argv);
  if((i = GetOptPos("-test", argc, argv)) > 0) {
    strcpy(test, argv[i + 1]);
    LOG(0, "test                       : %s\n", test);
  }
  InitSquashTable();
  LoadVocab();
  LoadModel();
  if(strcmp(test, "word_similarity") == 0) WordSimilarity();
  else if(strcmp(test, "sentence_similarity") == 0) SentSimilarity();
  else if(strcmp(test, "phrase_word") == 0) PhraseWord();
  else {
    printf("Test not specified");
  }
  return 0;
}
