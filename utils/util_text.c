#ifndef UTIL_TEXT
#define UTIL_TEXT

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util_misc.c"

// Macros Constants
//  HASH_SLOTS        : at most this number of elements without crash
//  TEXT_INIT_VCB_CAP : vocabulary initial size cap
//  TEXT_MAX_WORD_LEN : maximal word length
//  TEXT_MAX_SENT_WCT : maximal number of words in one sentence
// Values
//  TEXT_CORPUS_WORD_CNT  : corpus word count
//  TEXT_CORPUS_FILE_SIZE : corpus file size

////////////////
// Vocabulary //
////////////////

#define HASH_SLOTS 0xFFFFF  // around 1M slots by default

// BKDR Hash for string
unsigned long GetStrHash(char* str) {
  unsigned long h = 0;
  char ch;
  while ((ch = *(str++))) h = (((h << 7) + (h << 1) + (h) + ch) & HASH_SLOTS);
  return h;
}

// vocabulary, a hash map
typedef struct Vocabulary {
  char** id2wd;
  int* id2cnt;
  int* id2next;
  int* hash2head;
  int size;
  // initial settings
  int VCB_CAP;
} Vocabulary;

void VocabInitStorage(Vocabulary* vcb, int cap) {
  vcb->id2wd = (char**)malloc(cap * sizeof(char*));
  vcb->id2cnt = (int*)malloc(cap * sizeof(int));
  vcb->id2next = (int*)malloc(cap * sizeof(int));
  vcb->hash2head = (int*)malloc(HASH_SLOTS * sizeof(int));
  if (!vcb->id2wd || !vcb->id2cnt || !vcb->id2next || !vcb->hash2head) {
    LOG(0, "[VocabInitStorage]: allocating error\n");
    exit(1);
  }
  vcb->size = 0;
  vcb->VCB_CAP = cap;
  memset(vcb->hash2head, 0xFF, HASH_SLOTS * sizeof(int));
}

void VocabClearStorage(Vocabulary* vcb) {
  int i;
  for (i = 0; i < vcb->size; i++) free(vcb->id2wd[i]);
  free(vcb->id2wd);
  free(vcb->id2cnt);
  free(vcb->id2next);
  free(vcb->hash2head);
  return;
}

Vocabulary* VocabCreate(int cap) {
  Vocabulary* vcb = (Vocabulary*)malloc(sizeof(Vocabulary));
  VocabInitStorage(vcb, cap);
  return vcb;
}

void VocabFree(Vocabulary* vcb) {
  VocabClearStorage(vcb);
  free(vcb);
  return;
}

int VocabAdd(Vocabulary* vcb, char* str, int cnt) {
  int h = GetStrHash(str);
  int id = vcb->hash2head[h];
  while (id != -1 && strcmp(vcb->id2wd[id], str) != 0) id = vcb->id2next[id];
  if (id == -1) {
    id = vcb->size++;
    vcb->id2wd[id] = sclone(str);
    vcb->id2cnt[id] = 0;
    vcb->id2next[id] = vcb->hash2head[h];
    vcb->hash2head[h] = id;
    if (vcb->size + 2 > vcb->VCB_CAP) {
      vcb->VCB_CAP <<= 1;
      vcb->id2wd = (char**)realloc(vcb->id2wd, vcb->VCB_CAP * sizeof(char*));
      vcb->id2cnt = (int*)realloc(vcb->id2cnt, vcb->VCB_CAP * sizeof(int));
      vcb->id2next = (int*)realloc(vcb->id2next, vcb->VCB_CAP * sizeof(int));
    }
  }
  vcb->id2cnt[id] += cnt;
  return id;
}

int* value_for_compare;
int CmpVal(const void* x, const void* y) {
  return value_for_compare[*((int*)y)] - value_for_compare[*((int*)x)];
}

void VocabReduce(Vocabulary* vcb, int cap) {  // if cap = -1,  no limit
  int i;
  int* keys = (int*)malloc(vcb->size * sizeof(int));
  for (i = 0; i < vcb->size; i++) keys[i] = i;
  value_for_compare = vcb->id2cnt;
  qsort(keys, vcb->size, sizeof(int), CmpVal);
  int size = vcb->size;
  if (cap > 0 && size > cap) size = cap;
  char** words = (char**)malloc(size * sizeof(char*));
  int* counts = (int*)malloc(size * sizeof(int));
  for (i = 0; i < size; i++) {
    words[i] = sclone(vcb->id2wd[keys[i]]);
    counts[i] = vcb->id2cnt[keys[i]];
  }
  VocabClearStorage(vcb);
  VocabInitStorage(vcb, (int)(size * 1.1));
  for (i = 0; i < size; i++) VocabAdd(vcb, words[i], counts[i]);
  for (i = 0; i < size; i++) free(words[i]);
  free(keys);
  free(words);
  free(counts);
  return;
}

int VocabGetId(Vocabulary* vcb, char* str) {
  int h = GetStrHash(str);
  int id = vcb->hash2head[h];
  while (id != -1 && strcmp(vcb->id2wd[id], str) != 0) id = vcb->id2next[id];
  return id;  // UNK = -1 or in vocab
}

char* VocabGetWord(Vocabulary* vcb, int id) { return vcb->id2wd[id]; }
int VocabGetCount(Vocabulary* vcb, int id) { return vcb->id2cnt[id]; }

//////////
// Text //
//////////

#define TEXT_INIT_VCB_CAP 0x200000   // around 2M slots by default
#define TEXT_MAX_WORD_LEN 100        // one word has max 100 char
#define TEXT_MAX_SENT_WCT 200        // one sentence has max 200 word
long int TEXT_CORPUS_WORD_CNT = 0;   // corpus wourd count
long int TEXT_CORPUS_FILE_SIZE = 0;  // corpus file size

int TextReadWord(FILE* fin, char* str) {
  // return flag: 0=hit by space or tab; 1=newline; 2=end-of-file
  // 1: read non-whitespaces until hit character otherwise
  // 2: skip all whitespaces until hit end-of-file or non-whitespace character
  //    for stdin, stop skipping also when hit newline
  // return flag:
  //    if EOF ever hit: return 2
  //    otherwise if newline ever hit: return 1
  //    otherwise return 0 (hit space or tab)
  int i = 0, flag = -1;
  char ch;
  while (1) {
    ch = fgetc(fin);
    if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == EOF) break;
    if (i < TEXT_MAX_WORD_LEN - 1) str[i++] = ch;
  }
  str[i] = '\0';
  while (1) {
    if (ch == ' ' || ch == '\t') {
      if (flag != 1) flag = 0;
    } else if (ch == '\n' || ch == '\r') {
      flag = 1;
      if (fin == stdin) break;
    } else if (ch == EOF) {
      flag = 2;
      break;
    } else {
      ungetc(ch, fin);
      break;
    }
    ch = fgetc(fin);
  }
  return flag;
}

int TextReadWordFromStr(char* txt, int* posptr, char* str) {
  // return flag: 0=hit by space or tab; 1=newline; 2=end-of-str
  // posptr is set by this function and should not be changed outside
  int i = 0, flag = -1, len = strlen(txt);
  char ch;
  while (1) {
    ch = txt[(*posptr)++];
    if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || (*posptr) == len)
      break;
    if (i < TEXT_MAX_WORD_LEN - 1) str[i++] = ch;
  }
  str[i] = '\0';
  while (1) {
    if ((*posptr) == len) {
      flag = 2;
      break;
    } else if (ch == ' ' || ch == '\t') {
      if (flag != 1) flag = 0;
    } else if (ch == '\n' || ch == '\r') {
      flag = 1;
    } else {
      (*posptr)--;
      break;
    }
    ch = txt[(*posptr)++];
  }
  return flag;
}

int TextNormWord(char* str, int if_lower, int if_rm_trail_punc) {
  // return: when if_rm_trail_punc set 1, return 1 if str ends with .,:;?!
  int i = 0, j, flag = 0;
  char ch;
  if (if_lower)
    while (str[i] != '\0') {
      if (str[i] >= 'A' && str[i] <= 'Z') str[i] = str[i] - 'A' + 'a';
      i++;
    }
  i--;
  if (if_rm_trail_punc) {
    while (i >= 0) {
      ch = str[i];
      if (ch == '.' || ch == ',' || ch == ':' || ch == ';' || ch == '?' ||
          ch == '!') {
        str[i] = '\0';  // removing clause-terminating punctuation
        flag = 1;
      } else if (ch == '`' || ch == '"' || ch == '\'' || ch == ')' ||
                 ch == ']') {
        str[i] = '\0';  // removing proceeding enclosing punctuation
      } else
        break;
      i--;
    }
    i = 0;
    while (1) {  // removing preceding enclosing punctuation
      ch = str[i];
      if (ch == '`' || ch == '"' || ch == '\'' || ch == '(' || ch == '[')
        i++;
      else
        break;
    }
    if (i != 0)
      for (j = i; str[j - 1] != '\0'; j++) str[j - i] = str[j];
  }
  return flag;
}

Vocabulary* TextBuildVocab(char* text_file_path, int if_lower,
                           int if_rm_trail_punc, int cap) {
  // cap = -1 if no limit
  char str[TEXT_MAX_WORD_LEN];
  int flag = 0;
  Vocabulary* vcb = VocabCreate(TEXT_INIT_VCB_CAP);
  FILE* fin = fopen(text_file_path, "rb");
  fseek(fin, 0, SEEK_END);
  long int fsz = ftell(fin);
  fseek(fin, 0, SEEK_SET);
  TEXT_CORPUS_WORD_CNT = 0;
  if (!fin) {
    LOG(0, "[error]: cannot find file %s\n", text_file_path);
    exit(1);
  }
  while (1) {
    flag = TextReadWord(fin, str);
    TextNormWord(str, if_lower, if_rm_trail_punc);
    if (str[0] != '\0') {  // filter 0-length string
      VocabAdd(vcb, str, 1);
      TEXT_CORPUS_WORD_CNT++;
      if ((TEXT_CORPUS_WORD_CNT & 0xFFFFF) == 0xFFFFF)
        LOG(2,
            "\33[2K\r[TextBuildVocab]: reading %ld (*2^20 | M) word, "
            "vocabulary size %d, "
            "complete %.2lf%%",
            TEXT_CORPUS_WORD_CNT >> 20, vcb->size,
            100.0 * ftell(fin) / ((double)fsz));
    }
    if (flag == 2) break;
  }
  LOGCLR(1);
  LOG(1, "[TextBuildVocab]: reading %ld (*2^20 | M) word\n",
      TEXT_CORPUS_WORD_CNT >> 20);
  TEXT_CORPUS_FILE_SIZE = ftell(fin);
  LOG(1, "[TextBuildVocab]: file size %ld (*2^20 | M)\n",
      TEXT_CORPUS_FILE_SIZE >> 20);
  LOG(1, "[TextBuildVocab]: vocabulary size %d\n", vcb->size);
  fclose(fin);
  VocabReduce(vcb, cap);
  return vcb;
}

void TextSaveVocab(char* vcb_fp, Vocabulary* vcb) {
  int i;
  FILE* fout = fopen(vcb_fp, "wb");
  fprintf(fout, "%ld %ld\n", TEXT_CORPUS_FILE_SIZE, TEXT_CORPUS_WORD_CNT);
  for (i = 0; i < vcb->size; i++)
    fprintf(fout, "%s %d\n", vcb->id2wd[i], vcb->id2cnt[i]);
  fclose(fout);
  LOG(1, "[VOCABULARY]: Save to %s\n", vcb_fp);
  return;
}

Vocabulary* TextLoadVocab(char* vcb_fp, int cap, int high_freq_cutoff) {
  // cap: -1=no limit; high_freq_cutoff: first h_f_c number of words removed;
  int cnt, flag, id = 0;
  FILE* fin = fopen(vcb_fp, "rb");
  char s_word[TEXT_MAX_WORD_LEN], s_cnt[TEXT_MAX_WORD_LEN];
  if (!fin) {
    LOG(0, "[error]: cannot find file %s\n", vcb_fp);
    exit(1);
  }
  fscanf(fin, "%ld %ld\n", &TEXT_CORPUS_FILE_SIZE, &TEXT_CORPUS_WORD_CNT);
  Vocabulary* vcb =
      VocabCreate((cap > 0) ? (int)(cap * 1.1) : TEXT_INIT_VCB_CAP);
  while (1) {
    TextReadWord(fin, s_word);
    flag = TextReadWord(fin, s_cnt);
    cnt = atoi(s_cnt);
    if (id++ >= high_freq_cutoff) VocabAdd(vcb, s_word, cnt);
    if (flag == 2) break;
    if (cap > 0 && vcb->size >= cap) break;
  }
  fclose(fin);
  LOG(1,
      "[VOCABULARY]: Load from %s\n"
      "              size %d\n",
      vcb_fp, vcb->size);
  return vcb;
}

int TextReadSent(FILE* fin, Vocabulary* vcb, int* word_ids, int if_lower,
                 int if_rm_trail_punc, int till_line_end) {
  // if till_line_end = 1, each line is one sentence
  // else, line is delimited by punctuation (newline alone won't stop reading)
  // EOF info is not exposed to caller function
  char str[TEXT_MAX_WORD_LEN];
  int flag1 = 0, flag2 = 0, id;
  int word_num = 0;
  while (flag1 != 2) {
    flag1 = TextReadWord(fin, str);
    flag2 = TextNormWord(str, if_lower, if_rm_trail_punc);
    if (str[0] == '\0')
      id = -1;  // empty string
    else
      id = VocabGetId(vcb, str);  // non-empty string
    if (id != -1) word_ids[word_num++] = id;
    if (word_num == TEXT_MAX_SENT_WCT) break;
    if ((till_line_end && flag1 > 0) || (!till_line_end && flag2 == 1)) break;
  }
  return word_num;
}

int TextReadStr(char* str, Vocabulary* vcb, int* word_ids, int if_lower,
                int if_rm_trail_punc) {
  int pos = 0, flag = 0, word_num = 0, word_id;
  char word[TEXT_MAX_WORD_LEN];
  while (flag != 2) {
    flag = TextReadWordFromStr(str, &pos, word);
    TextNormWord(word, if_lower, if_rm_trail_punc);
    word_id = VocabGetId(vcb, word);
    if (word_id != -1) word_ids[word_num++] = word_id;
  }
  return word_num;
}

void TextPrintSent(Vocabulary* vcb, int* word_ids, int word_num) {
  int i;
  for (i = 0; i < word_num; i++)
    printf("%s(%d) ", VocabGetWord(vcb, word_ids[i]), word_ids[i]);
  printf("\n");
  return;
}

void TextPrintSentWord(Vocabulary* vcb, int* word_ids, int word_num) {
  int i;
  for (i = 0; i < word_num; i++) printf("%s ", VocabGetWord(vcb, word_ids[i]));
  printf("\n");
  return;
}

#endif /* end of include guard: UTIL_TEXT */
