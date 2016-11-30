/////////////////////////////////
// Vocabulary and Text Process //
/////////////////////////////////
// BKDR Hash for string
int GetStrHash(char *str) {
  unsigned long long h = 0;
  char ch;
  while((ch = *(str++))) h = (((h<<7) + (h<<1) + (h) + ch) & s3eVocabHashSize);
  return h;
}

// vocabulary, a hash map
struct Vocabulary {
  char **id2wd;
  int *id2cnt;
  int *id2next;
  int *hash2head;
  int size;
} vocab;

void InitVocab() {
  vocab.size = 0;
  vocab.id2wd = (char**) malloc(s3eVocabCapacity * sizeof(char*));
  vocab.id2cnt = (int*) malloc(s3eVocabCapacity * sizeof(int));
  vocab.id2next = (int*) malloc(s3eVocabCapacity * sizeof(int));
  vocab.hash2head = (int*) malloc(s3eVocabHashSize * sizeof(int));
  if(!vocab.id2wd || !vocab.id2cnt  || !vocab.id2next || !vocab.hash2head) {
    LOG(0, "allocating error @ InitVocab");
    exit(1);
  }
  memset(vocab.hash2head, 0xFF, s3eVocabHashSize * sizeof(int));
  return;
}

void FreeVocab() {
  int i;
  for(i = 0; i < vocab.size; i++) free(vocab.id2wd[i]);
  free(vocab.id2wd);
  free(vocab.id2cnt);
  free(vocab.id2next);
  free(vocab.hash2head);
  vocab.size = 0;
  return;
}

void ResetVocab() {
  int i;
  for(i = 0; i < vocab.size; i++) free(vocab.id2wd[i]);
  memset(vocab.hash2head, 0xFF, s3eVocabHashSize * sizeof(int));
  vocab.size = 0;
  return;
}

void AddWordToVocab(char *str, int cnt) {
  int h = GetStrHash(str);
  int id = vocab.hash2head[h];
  while(id != -1 && strcmp(vocab.id2wd[id], str) != 0) id = vocab.id2next[id];
  if(id == -1) {
    id = vocab.size++;
    STR_CLONE(vocab.id2wd[id], str);
    vocab.id2cnt[id] = 0;
    vocab.id2next[id] = vocab.hash2head[h];
    vocab.hash2head[h] = id;
    if(vocab.size + 2 > s3eVocabCapacity) {
      s3eVocabCapacity <<= 1;
      vocab.id2wd = (char**) realloc(vocab.id2wd,
                                    s3eVocabCapacity * sizeof(char*));
      vocab.id2cnt = (int*) realloc(vocab.id2cnt,
                                    s3eVocabCapacity * sizeof(int));
      vocab.id2next = (int*) realloc(vocab.id2next,
                                    s3eVocabCapacity * sizeof(int));
    }
  }
  vocab.id2cnt[id] += cnt;
  return;
}

int GetWordId(char *str) {
  int h = GetStrHash(str);
  int id = vocab.hash2head[h];
  while(id != -1 && strcmp(vocab.id2wd[id], str) != 0) id = vocab.id2next[id];
  return id; //UNK = -1 or in vocab
}

int CmpWordCnt(const void *x, const void *y) {
  return vocab.id2cnt[*((int*)y)] - vocab.id2cnt[*((int*)x)];
}

void ReduceVocab() {
  int i, size;
  LOG(1, "[reducing vocabulary]: sorting size %d\n", vocab.size);
  int *sort_ids = (int*) malloc(vocab.size * sizeof(int));
  for(i = 0; i < vocab.size; i++) sort_ids[i] = i;
  qsort(sort_ids, vocab.size, sizeof(int), CmpWordCnt);
  char **words = (char**) malloc(vocab.size * sizeof(char*));
  int *counts = (int*) malloc(vocab.size * sizeof(int));
  eff_corpus_w_cnt = 0;
  for(i = 0; i < vocab.size; i++) {
    STR_CLONE(words[i], vocab.id2wd[sort_ids[i]]);
    counts[i] = vocab.id2cnt[sort_ids[i]];
    eff_corpus_w_cnt += counts[i];
  }
  if(s3eVocabSize == -1) s3eVocabSize = vocab.size;
  size = vocab.size;
  if(size > s3eVocabSize) size = s3eVocabSize;
  ResetVocab();
  LOG(1, "[reducing vocabulary]: reconstructing size %d\n", size);
  for(i = 0; i < size; i++) AddWordToVocab(words[i], counts[i]);
  for(i = 0; i < size; i++) free(words[i]);
  free(sort_ids); free(words); free(counts);
  return;
}

/**
 * Read a word and filter trailing whitespace
 * @return  flag : 0 hit by space or tab
 *               	 1 hit by newline
 *               	 2 hit by end-of-file
 */
 int ReadWord(FILE *fin, char *str) {
  int i = 0, flag = -1;
  char ch;
  while(1) {
    ch = fgetc(fin);
    if(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == EOF) break;
    if(i < s3eMaxStringLength - 1) str[i++] = ch;
  }
  str[i] = '\0';
  while(1) {
    if(ch == ' ' || ch == '\t') flag = 0;
    else if(ch == '\n' || ch == '\r') {
      flag = 1;
      if(fin == stdin) break;
    }
    else if(ch == EOF) { flag = 2; break; }
    else { ungetc(ch, fin); break; }
    ch = fgetc(fin);
  }
  return flag;
}

/**
 * Normalize a word
 * @return  : if if_rm_trail_punc set 1, return 1 if str ends with .,:;?!
 */
int NormWord(char *str, int if_lower, int if_rm_trail_punc) {
  int i = 0, j, flag = 0;
  char ch;
  if(if_lower) while(str[i] != '\0') {
    if(str[i] >= 'A' && str[i] <= 'Z') str[i] = str[i] - 'A' + 'a';
    i++;
  }
  i--;
  if(if_rm_trail_punc) {
    while(i >= 0) {
      ch = str[i];
      if(ch == '.' || ch == ',' || ch == ':' ||
         ch == ';' || ch == '?' || ch == '!' ) {
        str[i] = '\0'; // removing clause-terminating punctuation
        flag = 1;
      }
      else if(ch == '`' || ch == '"' || ch == '\'' || ch == ')' || ch == ']') {
        str[i] = '\0'; // removing proceeding enclosing punctuation
      }
      else break;
      i--;
    }
    i = 0;
    while(1) {  // removing preceding enclosing punctuation
      ch = str[i];
      if(ch == '`' || ch == '"' || ch == '\'' || ch == '(' || ch == '[') i++;
      else break;
    }
    if(i != 0) for(j = i; str[j - 1] !='\0'; j++) str[j - i] = str[j];
  }
  return flag;
}

void BuildVocab() {
  char str[s3eMaxStringLength];
  int flag = 0;
  long long wcnt = 0;
  InitVocab();
  FILE *fin = fopen(text_file_path, "rb");
  if(!fin) {
     fprintf(stderr, "[error]: cannot find file %s\n", text_file_path);
     exit(1);
  }
  while(1) {
    flag = ReadWord(fin, str);
    if(s3eNormWord) NormWord(str, 1, 1);
    if(str[0] != '\0') { // filter 0-length string
      AddWordToVocab(str, 1);
      wcnt++;
      if(wcnt % 100000 == 0)
        LOG(2, "[building vocabulary]: reading %lldK word\r", wcnt / 1000);
    }
    if(flag == 2) break;
  }
  LOG(1, "[building vocabulary]: reading %lldK word\n", wcnt / 1000);
  file_size = ftell(fin);
  LOG(1, "[building vocabulary]: file size %lld\n", file_size);
  fclose(fin);
  return;
}

int LoadSent(FILE *fin, int* word_ids) {
  char str[s3eMaxStringLength];
  int flag1, flag2 = 0, id;
  int word_num = 0;
  while(1) {
    flag1 = ReadWord(fin, str);
    if(s3eNormWord) flag2 = NormWord(str, 1, 1);
    if(str[0] == '\0') id = -1; // empty string
    else id = GetWordId(str);   // non-empty string
    if(id != -1 && word_num < s3eMaxSentenceLength) word_ids[word_num++] = id;
    if(flag1 > 0 || flag2 == 1) break;
  }
  return word_num;
}

void SaveVocab() {
  int i;
  FILE *fout = fopen(vocab_file_path, "wb");
  fprintf(fout, "%lld %lld\n", file_size, eff_corpus_w_cnt);
  for(i = 0; i < vocab.size; i++)
    fprintf(fout, "%s %d\n", vocab.id2wd[i], vocab.id2cnt[i]);
  fclose(fout);
  LOG(1, "[saving vocabulary]: %s\n", vocab_file_path);
  return;
}

void LoadVocab() {
  int cnt, flag;
  FILE *fin = fopen(vocab_file_path, "rb");
  char s_word[s3eMaxStringLength], s_cnt[s3eMaxHeapSize];
  if(!fin) {
    fprintf(stderr, "[error]: cannot find file %s\n", vocab_file_path);
    exit(1);
  }
  fscanf(fin, "%lld %lld\n", &file_size, &eff_corpus_w_cnt);
  InitVocab();
  while(1) {
    ReadWord(fin, s_word);
    flag = ReadWord(fin, s_cnt);
    cnt = atoi(s_cnt);
    AddWordToVocab(s_word, cnt);
    if(flag == 2) break;
  }
  fclose(fin);
  LOG(1, "[loading vocabulary]: %s\n"
         "[loading vocabulary]: size %d\n", vocab_file_path, vocab.size);
  return;
}

long long int neg_unigram_size = 1e8;
void InitNegUnigram() {
  long long int i, j, k, total_word_cnt = 0, cnt = 0;
  real cdf, power = 0.75;
  for(i = 0; i < vocab.size; i++) total_word_cnt += pow(vocab.id2cnt[i], power);
  neg_unigram = (int*) malloc(1e8 * sizeof(int));
  for(i = 0, j = 0; i < vocab.size; i++) {
    cnt += pow(vocab.id2cnt[i], power);
    cdf = (real) cnt / total_word_cnt;
    cdf = (cdf > 1.0) ? 1.0 : cdf;
    k = neg_unigram_size * cdf;
    while(j < k) neg_unigram[j++] = i;
  }
  return;
}


int SampleNegUnigram(unsigned long long int s) {
  long long int r = RAND(s) * neg_unigram_size;
  return neg_unigram[r];
}
