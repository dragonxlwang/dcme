#ifndef CLASSIFY_HELPER
#define CLASSIFY_HELPER

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../classify/constants.c"
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"

long int HelperLocateNextInstance(char* file_path, long int pos) {
  char ln[LUP];
  FILE* fin = fopen(file_path, "rb");
  fseek(fin, pos, SEEK_SET);
  long int pp[4] = {0};
  int isvcode[4] = {0}, isvname[4] = {0}, isyear[4] = {0}, isabs[4] = {0};
  int i;
  while (1) {
    for (i = 3; i > 0; i--) pp[i] = pp[i - 1];
    for (i = 3; i > 0; i--) isvcode[i] = isvcode[i - 1];
    for (i = 3; i > 0; i--) isvname[i] = isvname[i - 1];
    for (i = 3; i > 0; i--) isyear[i] = isyear[i - 1];
    for (i = 3; i > 0; i--) isabs[i] = isabs[i - 1];
    pp[0] = ftell(fin);
    isvcode[0] = 0;
    isvname[0] = 0;
    isyear[0] = 0;
    isabs[0] = 0;
    if (!fgets(ln, LUP, fin)) break;
    if (strlen(ln) == 5) {
      isyear[0] = 1;
      for (i = 0; i < 4; i++)
        if (!cisdigit(ln[i])) isyear[0] = 0;
    }
    if (strlen(ln) < 100) isvcode[0] = 1;
    if (strlen(ln) < 200) isvname[0] = 1;
    if (strlen(ln) > 40) isabs[0] = 1;
    if (isvcode[3] && isvname[2] && isyear[1] && isabs[0]) break;
  }
  fclose(fin);
  return pp[3];
}

Vocabulary* HelperBuildVocab(char* file_path, int if_lower,
                             int if_rm_trail_punc, int cap) {
  // cap = -1 if no limit
  char str[WUP], ln[LUP];
  int flag = 0, pos = 0;
  Vocabulary* vcb = VocabCreate(TEXT_INIT_VCB_CAP);
  TEXT_CORPUS_FILE_SIZE = FileSize(file_path);
  TEXT_CORPUS_WORD_CNT = 0;
  FILE* fin = fopen(file_path, "rb");
  if (!fin) {
    LOG(0, "[error]: cannot find file %s\n", file_path);
    exit(1);
  }
  while (1) {
    if (!fgets(ln, LUP, fin)) break;  // venue code
    if (!fgets(ln, LUP, fin)) break;  // venue name
    if (!fgets(ln, LUP, fin)) break;  // year
    if (!fgets(ln, LUP, fin)) break;  // abs
    pos = 0;
    while (1) {
      flag = TextReadWordFromStr(ln, &pos, str);
      TextNormWord(str, if_lower, if_rm_trail_punc);
      if (str[0] != '\0') {  // filter 0-length string
        VocabAdd(vcb, str, 1);
        TEXT_CORPUS_WORD_CNT++;
        if ((TEXT_CORPUS_WORD_CNT & 0xFFFFF) == 0xFFFFF)
          LOG(2,
              "\33[2K\r[HelperBuildVocab]: reading %ld (*2^20 | M) word, "
              "vocabulary size %d, "
              "complete %.2lf%%",
              TEXT_CORPUS_WORD_CNT >> 20, vcb->size,
              100.0 * ftell(fin) / ((double)TEXT_CORPUS_FILE_SIZE));
      }
      if (flag == 2) break;
    }
  }
  LOGCLR(1);
  LOG(1, "[HelperBuildVocab]: reading %ld (*2^20 | M) word\n",
      TEXT_CORPUS_WORD_CNT >> 20);
  LOG(1, "[HelperBuildVocab]: file size %ld (*2^20 | M)\n",
      TEXT_CORPUS_FILE_SIZE >> 20);
  LOG(1, "[HelperBuildVocab]: vocabulary size %d\n", vcb->size);
  fclose(fin);
  VocabReduce(vcb, cap);
  return vcb;
}

int HelperReadInstance(FILE* fin, Vocabulary* vcb, Dictionary* classes,
                       int* fsv, int* fn_ptr, int* label_ptr, int if_lower,
                       int if_rm_trail_punc) {
  char ln[LUP];
  if (!fgets(ln, LUP, fin)) return 0;  // venue code
  sstrip(ln);
  *label_ptr = DictLocate(classes, ln);
  /* if (DictLocate(classes, ln) < 0) { */
  /*   printf("missing %s\n", ln); */
  /*   exit(1); */
  /* } */
  if (!fgets(ln, LUP, fin)) return 0;  // venue name
  if (!fgets(ln, LUP, fin)) return 0;  // year
  if (!fgets(ln, LUP, fin)) return 0;  // abs
  *fn_ptr = TextReadStr(ln, vcb, fsv, if_lower, if_rm_trail_punc);
  return 1;
}

Dictionary* HelperBuildClasses(char* file_path) {
  Dictionary* classes = DictCreate(0xFFF);
  char ln[LUP];
  FILE* fin = fopen(file_path, "rb");
  long int cnt = 0;
  if (!fin) {
    LOG(0, "[error]: cannot find file %s\n", file_path);
    exit(1);
  }
  while (1) {
    if (!fgets(ln, LUP, fin)) break;  // venue code
    sstrip(ln);
    DictInsert(classes, ln, DictGet(classes, ln, 0) + 1);
    if (!fgets(ln, LUP, fin)) break;  // venue name
    if (!fgets(ln, LUP, fin)) break;  // year
    if (!fgets(ln, LUP, fin)) break;  // abs
    cnt++;
  }
  LOG(1, "[HelperBuildClasses]: reading %ld instances\n", cnt);
  LOG(1, "[HelperBuildClasses]: classes number %d\n", classes->size);
  fclose(fin);
  return classes;
}

#endif /* ifndef CLASSIFY_HELPER */
