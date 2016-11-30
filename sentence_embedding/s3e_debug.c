///////////////
// CKY Debug //
///////////////

void CKYPrintSent(int *word_ids, int word_num) {
  int i;
  for(i = 0; i < word_num; i++)
    printf("%s%s%s:%d ", ANSI_COLOR_YELLOW, vocab.id2wd[word_ids[i]],
            ANSI_COLOR_RESET, word_ids[i]);
  printf("\n");
}

void CKYPrintTree(struct Bookkeeping *bkkp, int word_num, int c, int prefix,
                  int level) {
  int i, cl, cr, beg, end;
  char pstr[100] = {0};
  cl = bkkp->l[c];
  cr = bkkp->r[c];
  for(i = level - 1; i >= 0; i--) {
    if((!(prefix & (1 << i))) && (i == 0)) sprintf(pstr + strlen(pstr), "\\ ");
    else if(prefix & (1 << i)) sprintf(pstr + strlen(pstr), "| ");
    else sprintf(pstr + strlen(pstr), "  ");
  }
  printf("%s", pstr);
  if(cl == -1) {
    printf("\b--word %s%s%s:%d, %s%lf%s\n",
          ANSI_COLOR_RED, vocab.id2wd[cr], ANSI_COLOR_RESET,
          cr,
          ANSI_COLOR_YELLOW, bkkp->s[c], ANSI_COLOR_RESET);

    printf("%s", pstr);
    if(pstr[strlen(pstr) - 2] == '\\') printf("\b\b  ");
    PrintArr("\b     v:", bkkp->v + c * s3eM, s3eM);


    printf("%s", pstr);
    if(pstr[strlen(pstr) - 2] == '\\') printf("\b\b  ");
    PrintArr("\b     w:", bkkp->w + c * s3eN, s3eN);
  }
  else {
    ResolveSpanId((int) (c / s3eBestK), word_num, &beg, &end);

    printf("\b--(%d %d) ", beg, end);
    printf("%lf, %s%lf%s\n",
           bkkp->s[c] - s3eDecay * (bkkp->s[cl] + bkkp->s[cr]),
           ANSI_COLOR_YELLOW, bkkp->s[c], ANSI_COLOR_RESET);

    printf("%s", pstr);
    if(pstr[strlen(pstr) - 2] == '\\') printf("\b\b  ");
    printf("| ");
    PrintArr("\b  v:", bkkp->v + c * s3eM, s3eM);

    printf("%s", pstr);
    if(pstr[strlen(pstr) - 2] == '\\') printf("\b\b  ");
    printf("| ");
    PrintArr("\b  w:", bkkp->w + c * s3eN, s3eN);

    // real rw[s3eMaxDimension], ww[s3eMaxDimension];
    // SemanticEval(SMN1, SMN2, SMN3, SMNB1, SMNB2, s3eM, s3eN,
    //              bkkp->v + cl * s3eM, bkkp->v + cr * s3eM,
    //              bkkp->w + cl * s3eN, bkkp->w + cr * s3eN,
    //              rw);
    // printf("%s", pstr);
    // if(pstr[strlen(pstr) - 2] == '\\') printf("\b\b  ");
    // printf("| ");
    // PrintArr("\b rw:", rw, s3eN);
    //
    // printf("%s", pstr);
    // if(pstr[strlen(pstr) - 2] == '\\') printf("\b\b  ");
    // printf("| ");
    // for(i = 0; i < s3eN; i++) ww[i] = TANH(rw[i]);
    // PrintArr("\b ww:", ww, s3eN);

    CKYPrintTree(bkkp, word_num, cl, prefix * 2 + 1, level + 1);
    CKYPrintTree(bkkp, word_num, cr, prefix * 2 + 0, level + 1);
  }
  return;
}

void CKYDebug(int *word_ids, int word_num, struct Bookkeeping *bkkp) {
  int i;
  printf("\ng_sent_cnt = %d\n", g_sent_cnt);
  CKYPrintSent(word_ids, word_num);
  CKYPrintTree(bkkp, word_num, bkkp->sentence_bkkp_entry, 0, 0);
  printf("\n");
  char* syn_name = malloc(10);
  printf("SYN\n");
  for(i = 0; i < s3eM; i++) {
    sprintf(syn_name, "syn_%d", i);
    PrintMatrix(syn_name, MDL->syn + i * (s3eM + 1) * (s3eM + 1),
                  s3eM + 1, s3eM + 1);
    printf("\n");
  }
  printf("\n");
  printf("smn=%lf\n", Norm(MDL->smn, SMN_SIZE));
  printf("\n");
  printf("\n");
  return;
}
