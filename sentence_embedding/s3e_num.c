/////////////////////////////////////////////////
// Algebra/Numeric and Composition Computation //
/////////////////////////////////////////////////
void BilinearEval(real *t,                                  // model
                  int p, int m, int n,                      // dim
                  real *x, real *y,                         // input
                  real *z) {                                // output
  int i, j, k = 0;
  int a, b = 0;
  for(i = 0; i < p; i++) {
    z[i] = 0;
    a = i * (m + 1) * (n + 1);
    for(j = 0; j < m; j++) {
      b = a + j * (n + 1);
      for(k = 0; k < n; k++) z[i] += x[j] * y[k] * t[b + k];
    }
    for(j = 0; j < m; j++) z[i] += x[j] * t[a + j * (n + 1) + n];
    b = a + m * (n + 1);
    for(k = 0; k < n; k++) z[i] += y[k] * t[b + k];
    z[i] += t[b + n];
  }
  return;
}

void BilinearAccGrad(real *t,                               // model
                     int p, int m, int n,                   // dim: m, n -> p
                     real *x, real *y,                      // input
                     real *gz,                              // grad output
                     real *gt,                              // rv: grad model
                     real *gx, real *gy) {                  // rv: grad input
  int i, j, k = 0;
  int a, b =0;
  for(i = 0; i < p; i++) {
    a = i * (m + 1) * (n + 1);
    for(j = 0; j < m; j++) {
      b = a + j * (n + 1);
      for(k = 0; k < n; k++) {
        gt[b + k] += gz[i] * x[j] * y[k];
        gx[j] += gz[i] * y[k] * t[b + k];
        gy[k] += gz[i] * x[j] * t[b + k];
      }
    }
    for(j = 0; j < m; j++) {
      gt[a + j * (n + 1) + n] += gz[i] * x[j];
      gx[j] += gz[i] * t[a + j * (n + 1) + n];
    }
    b = a + m * (n + 1);
    for(k = 0; k < n; k++) {
      gt[b + k] += gz[i] * y[k];
      gy[k] += gz[i] * t[b + k];
    }
    gt[b + n] += gz[i];
  }
  return;
}

void SemanticEval(real *s,                                        // model
                  int m, int n,                                   // dim
                  real *v1, real *v2, real *w1, real *w2,         // input
                  real *rw) {                                     // output
  int i, j, k, p1, p2, p3;
  real cv, c1, c2, cv12;
  p1 = 0;
  p2 = m * m  * (n + 1);
  p3 = 2 * m * m  * (n + 1);
  for(k = 0; k < n; k++) rw[k] = s[p3++];
  for(i = 0; i < m; i++)
    for(j = 0; j < m; j++) {
      cv = v1[i] * v2[j];
      for(k = 0, c1 = s[p1++]; k < n; k++) c1 += w1[k] * s[p1++];
      for(k = 0, c2 = s[p2++]; k < n; k++) c2 += w2[k] * s[p2++];
      cv12 = cv * c1 * c2;
      for(k = 0; k < n; k++) rw[k] += cv12 * s[p3++];
    }
  return;
}

void SemanticAccGrad(
        real *s,                                            // model
        int m, int n,                                       // dim
        real *v1, real *v2, real *w1, real *w2,             // input
        real *grw,                                          // grad output
        real *gs,                                           // rv: grad model
        real *gv1, real *gv2, real *gw1, real *gw2) {       // rv: grad input
  int i, j, k, p1, p2, p3, q1, q2, q3;
  real cv, c1, c2, c3, cv12, c123, cv23, cv13;
  p1 = 0;
  p2 = m * m  * (n + 1);
  p3 = 2 * m * m  * (n + 1);
  q1 = p1;
  q2 = p2;
  q3 = p3 + n;
  for(k = 0; k < n; k++) gs[p3++] = grw[k];
  for(i = 0; i < m; i++)
    for(j = 0; j < m; j++) {
      cv = v1[i] * v2[j];
      for(k = 0, c1 = s[p1++]; k < n; k++) c1 += w1[k] * s[p1++];
      for(k = 0, c2 = s[p2++]; k < n; k++) c2 += w2[k] * s[p2++];
      for(k = 0, c3 = 0; k < n; k++) c3 += grw[k] * s[p3++];
      cv12 = cv * c1 * c2;
      cv23 = cv * c2 * c3;
      cv13 = cv * c1 * c3;
      c123 = c1 * c2 * c3;
      gv1[i] += v2[j] * c123;
      gv2[j] += v1[i] * c123;
      for(k = 0, gs[q1++] += cv23; k < n; k++) {
        gw1[k] += cv23 * s[q1];
        gs[q1++] += cv23 * w1[k];
      }
      for(k = 0, gs[q2++] += cv13; k < n; k++) {
        gw2[k] += cv13 * s[q2];
        gs[q2++] += cv13 * w2[k];
      }
      for(k = 0; k < n; k++) gs[q3++] += cv12 * grw[k];
    }
  return;
}

void DecompEval(real *d,                                  // model
                int m, int n,                             // dim
                real *v1, real *v2, real *w,              // input
                real *arw1, real *arw2) {                 // output: autoencoded
  int i, j, k, p0, p1, p2;
  real c0, cv, cv0;
  p0 = 0;
  p1 = m * m * (n + 1);
  p2 = p1 + (m * m + 1) * n;
  for(k = 0; k < n; k++) arw1[k] = d[p1++];
  for(k = 0; k < n; k++) arw2[k] = d[p2++];
  for(i = 0; i < m; i++)
    for(j = 0; j < m; j++) {
      cv = v1[i] * v2[j];
      for(k = 0, c0 = d[p0++]; k < n; k++) c0 += w[k] * d[p0++];
      cv0 = cv * c0;
      for(k = 0; k < n; k++) arw1[k] += cv0 * d[p1++];
      for(k = 0; k < n; k++) arw2[k] += cv0 * d[p2++];
    }
  return;
}

void DecompAccGrad(real *d,                               // model
                   int m, int n,                          // dim
                   real *v1, real *v2, real *w,           // input
                   real *garw1, real *garw2,              // grad output
                   real *gd,                              // rv: grad model
                   real *gv1, real *gv2, real *gw) {      // rv: grad input
  int i, j, k, p0, p1, p2, q0;
  real c0, c1, c2, cv, cv0, cv1p2;
  p0 = 0;
  p1 = m * m * (n + 1);
  p2 = p1 + (m * m + 1) * n;
  q0 = p0;
  for(k = 0; k < n; k++) gd[p1++] += garw1[k];
  for(k = 0; k < n; k++) gd[p2++] += garw2[k];
  for(i = 0; i < m; i++)
    for(j = 0; j < m; j++) {
      cv = v1[i] * v2[j];
      for(k = 0, c0 = d[p0++]; k < n; k++) c0 += w[k] * d[p0++];
      cv0 = cv * c0;
      for(k = 0, c1 = 0, c2 = 0; k < n; k++) {
        c1 += garw1[k] * d[p1];
        c2 += garw2[k] * d[p2];
        gd[p1++] += cv0 * garw1[k];
        gd[p2++] += cv0 * garw2[k];
      }
      gv1[i] += v2[j] * c0 * (c1 + c2);
      gv2[j] += v1[i] * c0 * (c1 + c2);
      cv1p2 = cv * (c1 + c2);
      for(k = 0, gd[q0++] += cv1p2; k < n; k++) {
        gw[k] += d[q0] * cv1p2;
        gd[q0++] += w[k] * cv1p2;
      }
    }
  return;
}

void SoftMaxEval(int l,                                     // dim
                 real *vi,                                  // input
                 real *vo) {                                // output
  int i;
  real b = MAX(vi, l);
  for(i = 0; i < l; i++) vo[i] = EXP(vi[i] - b);
  real s = SUM(vo, l);
  for(i = 0; i < l; i++) vo[i] /= s;
  return;
}

void SoftMaxAccGrad(int l,                                  // dim
                    real *vo,                               // output
                    real *gvo,                              // grad output
                    real *gvi) {                            // rv: grad input
  int i,j;
  real c;
  for(i = 0; i < l; i++) {
    c = gvo[i] * vo[i];
    for(j = 0; j < l; j++) {
      gvi[j] += c * ((i == j ? 1.0 : 0.0) - vo[j]);
    }
  }
  return;
}

void TanhEval(int l, real *vi, real *vo) {
  int i;
  for(i = 0; i < l; i++)  vo[i] = TANH(vi[i]);
}

void TanhAccGrad(int l, real *vo, real *gvo, real *gvi) {
  int i;
  for(i = 0; i < l; i++) gvi[i] += (1 - SQ(vo[i])) * gvo[i];
  return;
}

////////////////////////////////////
// Abstract Computation Interface //
////////////////////////////////////
real CompositionEval(real *v1, real *v2, real *w1, real *w2,    // input
                     real *v, real *w, real *aw1, real *aw2) {  // output
  real rv[s3eMaxDimension], rw[s3eMaxDimension],
       arw1[s3eMaxDimension], arw2[s3eMaxDimension], s;
  // composition
  BilinearEval(MDL->syn, s3eM, s3eM, s3eM, v1, v2, rv);         // rv
  SoftMaxEval(s3eM, rv, v);                                     // v
  SemanticEval(MDL->smn, s3eM, s3eN, v1, v2, w1, w2, rw);       // rw
  TanhEval(s3eN, rw, w);                                        // w
  // decomposition
  DecompEval(MDL->dsmn, s3eM, s3eN, v1, v2, w, arw1, arw2);     // arw12
  TanhEval(s3eN, arw1, aw1);                                    // aw12
  TanhEval(s3eN, arw2, aw2);
  s = - 0.5 * (Dist(aw1, w1, s3eN) + Dist(aw2, w2, s3eN));      // s
  return s;
}

void CompositionAccGrad(
    real *v1, real *v2, real *w1, real *w2,                   // input
    real *v, real *w, real *aw1, real *aw2, real s,           // output
    real *gv, real *gw, real gs,                              // grad output
    struct Param *gm,
    real *gv1, real *gv2, real *gw1, real *gw2) {             // rv:g in
  int i;
  real grv[s3eMaxDimension], grw[s3eMaxDimension],
       garw1[s3eMaxDimension], garw2[s3eMaxDimension],
       gaw1[s3eMaxDimension], gaw2[s3eMaxDimension];
  memset(grv, 0, s3eM * sizeof(real));
  memset(grw, 0, s3eN * sizeof(real));
  memset(garw1, 0, s3eN * sizeof(real));
  memset(garw2, 0, s3eN * sizeof(real));

  for(i = 0; i < s3eN; i++) {                             // gs -> gw12, gaw12
    gaw1[i] = gs * (w1[i] - aw1[i]);
    gaw2[i] = gs * (w2[i] - aw2[i]);
    gw1[i] -= gaw1[i];
    gw2[i] -= gaw2[i];
  }
  TanhAccGrad(s3eN, aw1, gaw1, garw1);                    // gaw12 -> garw12
  TanhAccGrad(s3eN, aw2, gaw2, garw2);
  DecompAccGrad(MDL->dsmn, s3eM, s3eN, v1, v2, w,         // garw12 -> gv12, gw
                garw1, garw2, gm->dsmn, gv1, gv2, gw);
  SoftMaxAccGrad(s3eM, v, gv, grv);                       // gv  -> grv
  BilinearAccGrad(MDL->syn, s3eM, s3eM, s3eM, v1, v2,     // grv -> gv12
                  grv, gm->syn, gv1, gv2);
  TanhAccGrad(s3eN, w, gw, grw);                          // gw -> grw
  SemanticAccGrad(MDL->smn, s3eM, s3eN, v1, v2, w1, w2,   // grw -> gw12, gv12
                  grw, gm->smn, gv1, gv2, gw1, gw2);
  return;
}

void LookupTableEval(int idx,                       // input
                     real *v, real *w) {            // output
  SoftMaxEval(s3eM, MDL->synlut + idx * s3eM, v);                     //rv->v
  TanhEval(s3eN, MDL->smnlut + idx * s3eN, w);                        //rw->w
  return;
}

void LookupTableAccGrad(int idx,                          // intput
                        real *v, real *w,                 // output
                        real *gv, real *gw,               // grad output
                        real *grv, real *grw) {           // rv: g model
  SoftMaxAccGrad(s3eM, v, gv, grv);                               // gv -> grv
  TanhAccGrad(s3eN, w, gw, grw);                                  // gw -> grw
  return;
}
