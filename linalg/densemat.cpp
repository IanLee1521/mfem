// Copyright (c) 2010, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-443211. All Rights
// reserved. See file COPYRIGHT for details.
//
// This file is part of the MFEM library. For more information and source code
// availability see http://mfem.googlecode.com.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License (as published by the Free
// Software Foundation) version 2.1 dated February 1999.


// Implementation of data types dense matrix, inverse dense matrix


#include "vector.hpp"
#include "matrix.hpp"
#include "densemat.hpp"
#include "../general/table.hpp"

#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <cstdlib>
#if defined(_MSC_VER) && (_MSC_VER < 1800)
#include <float.h>
#define copysign _copysign
#endif

namespace mfem
{

using namespace std;

DenseMatrix::DenseMatrix() : Matrix(0)
{
   data = NULL;
}

DenseMatrix::DenseMatrix(const DenseMatrix &m) : Matrix(m.height, m.width)
{
   int hw = height * width;
   data = new double[hw];
   for (int i = 0; i < hw; i++)
      data[i] = m.data[i];
}

DenseMatrix::DenseMatrix(int s) : Matrix(s)
{
   data = new double[s*s];

   for (int i = 0; i < s; i++)
      for (int j = 0; j < s; j++)
         (*this)(i,j) = 0;
}

DenseMatrix::DenseMatrix(int m, int n) : Matrix(m, n)
{
   data = new double[m*n];

   for (int i = 0; i < m; i++)
      for (int j = 0; j < n; j++)
         (*this)(i,j) = 0;
}

DenseMatrix::DenseMatrix(const DenseMatrix &mat, char ch)
   : Matrix(mat.width, mat.height)
{
   data = new double[height*width];

   for (int i = 0; i < height; i++)
      for (int j = 0; j < width; j++)
         (*this)(i,j) = mat(j,i);
}

void DenseMatrix::SetSize(int s)
{
   if (Height() == s && Width() == s)
      return;
   if (data != NULL)
      delete [] data;
   height = width = s;
   if (s > 0)
   {
      int ss = s*s;
      data = new double[ss];
      for (int i = 0; i < ss; i++)
         data[i] = 0.0;
   }
   else
      data = 0;
}

void DenseMatrix::SetSize(int h, int w)
{
   if (Height() == h && Width() == w)
      return;
   if (data != NULL)
      delete [] data;
   height = h;
   width = w;
   if (h > 0 && w > 0)
   {
      int hw = h*w;
      data = new double[hw];
      for (int i = 0; i < hw; i++)
         data[i] = 0.0;
   }
   else
      data = 0;
}

double &DenseMatrix::Elem(int i, int j)
{
   return (*this)(i,j);
}

const double &DenseMatrix::Elem(int i, int j) const
{
   return (*this)(i,j);
}

void DenseMatrix::Mult(const double *x, double *y) const
{
   double *d_col = data;
   double x_col = x[0];
   for (int row = 0; row < height; row++)
      y[row] = x_col*d_col[row];
   d_col += height;
   for (int col = 1; col < width; col++)
   {
      x_col = x[col];
      for (int row = 0; row < height; row++)
         y[row] += x_col*d_col[row];
      d_col += height;
   }
}

void DenseMatrix::Mult(const Vector &x, Vector &y) const
{
#ifdef MFEM_DEBUG
   if ( height != y.Size() || width != x.Size() )
      mfem_error("DenseMatrix::Mult");
#endif

   Mult((const double *)x, (double *)y);
}

double DenseMatrix::operator *(const DenseMatrix &m) const
{
#ifdef MFEM_DEBUG
   if (Height() != m.Height() || Width() != m.Width())
      mfem_error("DenseMatrix::operator *(...)");
#endif

   int hw = height * width;
   double a = 0.0;
   for (int i = 0; i < hw; i++)
      a += data[i] * m.data[i];

   return a;
}

void DenseMatrix::MultTranspose(const double *x, double *y) const
{
   double *d_col = data;
   for (int col = 0; col < width; col++)
   {
      double y_col = 0.0;
      for (int row = 0; row < height; row++)
         y_col += x[row]*d_col[row];
      y[col] = y_col;
      d_col += height;
   }
}

void DenseMatrix::MultTranspose(const Vector &x, Vector &y) const
{
#ifdef MFEM_DEBUG
   if ( height != x.Size() || width != y.Size() )
      mfem_error("DenseMatrix::MultTranspose");
#endif

   MultTranspose((const double *)x, (double *)y);
}

void DenseMatrix::AddMult(const Vector &x, Vector &y) const
{
#ifdef MFEM_DEBUG
   if ( height != y.Size() || width != x.Size() )
      mfem_error("DenseMatrix::AddMult");
#endif

   const double *xp = x;
   double *d_col = data, *yp = y;
   for (int col = 0; col < width; col++)
   {
      double x_col = xp[col];
      for (int row = 0; row < height; row++)
         yp[row] += x_col*d_col[row];
      d_col += height;
   }
}

double DenseMatrix::InnerProduct(const double *x, const double *y) const
{
   double prod = 0.0;

   for (int i = 0; i < height; i++)
   {
      double Axi = 0.0;
      for (int j = 0; j < width; j++)
         Axi += (*this)(i,j) * x[j];
      prod += y[i] * Axi;
   }

   return prod;
}

// LeftScaling this = diag(s) * this
void DenseMatrix::LeftScaling(const Vector & s)
{
   double * it_data = data;
   for (int j = 0; j < width; ++j)
      for (int i = 0; i < height; ++i)
         *(it_data++) *= s(i);
}

// InvLeftScaling this = diag(1./s) * this
void DenseMatrix::InvLeftScaling(const Vector & s)
{
   double * it_data = data;
   for (int j = 0; j < width; ++j)
      for (int i = 0; i < height; ++i)
         *(it_data++) /= s(i);
}

// RightScaling: this = this * diag(s);
void DenseMatrix::RightScaling(const Vector & s)
{
   double sj;
   double * it_data = data;
   for (int j = 0; j < width; ++j)
   {
      sj = s(j);
      for (int i = 0; i < height; ++i)
         *(it_data++) *= sj;
   }
}

// InvRightScaling: this = this * diag(1./s);
void DenseMatrix::InvRightScaling(const Vector & s)
{
   double sj;
   double * it_data = data;
   for (int j = 0; j < width; ++j)
   {
      sj = 1./s(j);
      for (int i = 0; i < height; ++i)
         *(it_data++) *= sj;
   }
}

// SymmetricScaling this = diag(sqrt(s)) * this * diag(sqrt(s))
void DenseMatrix::SymmetricScaling(const Vector & s)
{
   if (height != width || s.Size() != height)
      mfem_error("DenseMatrix::SymmetricScaling");

   double * ss = new double[width];
   double * it_s = s.GetData();
   double * it_ss = ss;
   for( double * end_s = it_s + width; it_s != end_s; ++it_s)
      *(it_ss++) = sqrt(*it_s);

   double * it_data = data;
   for (int j = 0; j < width; ++j)
      for (int i = 0; i < height; ++i)
         *(it_data++) *= ss[i]*ss[j];

   delete[] ss;
}

// InvSymmetricScaling this = diag(sqrt(1./s)) * this * diag(sqrt(1./s))
void DenseMatrix::InvSymmetricScaling(const Vector & s)
{
   if (height != width || s.Size() != width)
      mfem_error("DenseMatrix::SymmetricScaling");

   double * ss = new double[width];
   double * it_s = s.GetData();
   double * it_ss = ss;
   for( double * end_s = it_s + width; it_s != end_s; ++it_s)
      *(it_ss++) = 1./sqrt(*it_s);

   double * it_data = data;
   for (int j = 0; j < width; ++j)
      for (int i = 0; i < height; ++i)
         *(it_data++) *= ss[i]*ss[j];

   delete[] ss;
}

double DenseMatrix::Trace() const
{
#ifdef MFEM_DEBUG
   if (Width() != Height())
      mfem_error("DenseMatrix::Trace() : not a square matrix!");
#endif

   double t = 0.0;

   for (int i = 0; i < width; i++)
      t += (*this)(i, i);

   return t;
}

MatrixInverse *DenseMatrix::Inverse() const
{
   return new DenseMatrixInverse(*this);
}

double DenseMatrix::Det() const
{
#ifdef MFEM_DEBUG
   if (Height() != Width() || Height() < 1 || Height() > 3)
      mfem_error("DenseMatrix::Det");
#endif

   switch (Height())
   {
   case 1:
      return data[0];

   case 2:
      return data[0] * data[3] - data[1] * data[2];

   case 3:
   {
      const double *d = data;
      return
         d[0] * (d[4] * d[8] - d[5] * d[7]) +
         d[3] * (d[2] * d[7] - d[1] * d[8]) +
         d[6] * (d[1] * d[5] - d[2] * d[4]);
   }
   }
   return 0.0;
}

double DenseMatrix::Weight() const
{
   if (Height() == Width())
   {
      // return fabs(Det());
      return Det();
   }
   else if ((Height() == 2) && (Width() == 1))
   {
      return sqrt(data[0] * data[0] + data[1] * data[1]);
   }
   else if ((Height() == 3) && (Width() == 1))
   {
      return sqrt(data[0] * data[0] + data[1] * data[1] + data[2] * data[2]);
   }
   else if ((Height() == 3) && (Width() == 2))
   {
      const double *d = data;
      double E = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
      double G = d[3] * d[3] + d[4] * d[4] + d[5] * d[5];
      double F = d[0] * d[3] + d[1] * d[4] + d[2] * d[5];
      return sqrt(E * G - F * F);
   }
   mfem_error("DenseMatrix::Weight()");
   return 0.0;
}

void DenseMatrix::Add(const double c, const DenseMatrix &A)
{
   for (int i = 0; i < Height(); i++)
      for (int j = 0; j < Width(); j++)
         (*this)(i,j) += c * A(i,j);
}

DenseMatrix &DenseMatrix::operator=(double c)
{
   int s = Height()*Width();
   if (data != NULL)
      for (int i = 0; i < s; i++)
         data[i] = c;
   return *this;
}

DenseMatrix &DenseMatrix::operator=(const double *d)
{
   if (data != NULL)
   {
      int s = Height()*Width();
      for (int i = 0; i < s; i++)
         data[i] = d[i];
   }
   return *this;
}

DenseMatrix &DenseMatrix::operator=(const DenseMatrix &m)
{
   int i, hw;

   SetSize(m.height, m.width);

   hw = height * width;
   for (i = 0; i < hw; i++)
      data[i] = m.data[i];

   return *this;
}

DenseMatrix &DenseMatrix::operator+=(DenseMatrix &m)
{
   MFEM_ASSERT(Height() == m.Height() && Width() == m.Width(),
               "incompatible matrix sizes.");

   for (int i = 0; i < height; i++)
      for (int j = 0; j < width; j++)
         (*this)(i, j) += m(i, j);

   return *this;
}

DenseMatrix &DenseMatrix::operator-=(DenseMatrix &m)
{
   int i, j;

   for (i = 0; i < height; i++)
      for (j = 0; j < width; j++)
         (*this)(i, j) -= m(i, j);

   return *this;
}

DenseMatrix &DenseMatrix::operator*=(double c)
{
   int s = Height()*Width();
   if (data != NULL)
      for (int i = 0; i < s; i++)
         data[i] *= c;
   return *this;
}

void DenseMatrix::Neg()
{
   int i, hw = Height() * Width();

   for (i = 0; i < hw; i++)
      data[i] = -data[i];
}

#ifdef MFEM_USE_LAPACK
extern "C" void
dgetrf_(int *, int *, double *, int *, int *, int *);
extern "C" void
dgetrs_(char *, int *, int *, double *, int *, int *, double *, int *, int *);
extern "C" void
dgetri_(int *N, double *A, int *LDA, int *IPIV, double *WORK,
        int *LWORK, int *INFO);
#endif

void DenseMatrix::Invert()
{
#ifdef MFEM_DEBUG
   if (Height() <= 0 || Height() != Width())
      mfem_error("DenseMatrix::Invert()");
#endif

#ifdef MFEM_USE_LAPACK
   int   *ipiv = new int[width];
   int    lwork = -1;
   double qwork, *work;
   int    info;

   dgetrf_(&width, &width, data, &width, ipiv, &info);

   if (info)
      mfem_error("DenseMatrix::Invert() : Error in DGETRF");

   dgetri_(&width, data, &width, ipiv, &qwork, &lwork, &info);

   lwork = (int) qwork;
   work = new double[lwork];

   dgetri_(&width, data, &width, ipiv, work, &lwork, &info);

   if (info)
      mfem_error("DenseMatrix::Invert() : Error in DGETRI");

   delete [] work;
   delete [] ipiv;
#else
   int c, i, j, n = Width();
   double a, b;
   Array<int> piv(n);

   for (c = 0; c < n; c++)
   {
      a = fabs((*this)(c, c));
      i = c;
      for (j = c + 1; j < n; j++)
      {
         b = fabs((*this)(j, c));
         if (a < b)
         {
            a = b;
            i = j;
         }
      }
      if (a == 0.0)
         mfem_error("DenseMatrix::Invert() : singular matrix");
      piv[c] = i;
      for (j = 0; j < n; j++)
         Swap<double>((*this)(c, j), (*this)(i, j));

      a = (*this)(c, c) = 1.0 / (*this)(c, c);
      for (j = 0; j < c; j++)
         (*this)(c, j) *= a;
      for (j++; j < n; j++)
         (*this)(c, j) *= a;
      for (i = 0; i < c; i++)
      {
         (*this)(i, c) = a * (b = -(*this)(i, c));
         for (j = 0; j < c; j++)
            (*this)(i, j) += b * (*this)(c, j);
         for (j++; j < n; j++)
            (*this)(i, j) += b * (*this)(c, j);
      }
      for (i++; i < n; i++)
      {
         (*this)(i, c) = a * (b = -(*this)(i, c));
         for (j = 0; j < c; j++)
            (*this)(i, j) += b * (*this)(c, j);
         for (j++; j < n; j++)
            (*this)(i, j) += b * (*this)(c, j);
      }
   }

   for (c = n - 1; c >= 0; c--)
   {
      j = piv[c];
      for (i = 0; i < n; i++)
         Swap<double>((*this)(i, c), (*this)(i, j));
   }
#endif
}

void DenseMatrix::Norm2(double *v) const
{
   for (int j = 0; j < Width(); j++)
   {
      v[j] = 0.0;
      for (int i = 0; i < Height(); i++)
         v[j] += (*this)(i,j)*(*this)(i,j);
      v[j] = sqrt(v[j]);
   }
}

double DenseMatrix::MaxMaxNorm() const
{
   int hw = Height()*Width();
   const double *d = data;
   double norm = 0.0, abs_entry;

   for (int i = 0; i < hw; i++)
   {
      abs_entry = fabs(d[i]);
      if (norm < abs_entry)
         norm = abs_entry;
   }

   return norm;
}

double DenseMatrix::FNorm() const
{
   int i, hw = Height() * Width();
   double max_norm = 0.0, entry, fnorm2;

   for (i = 0; i < hw; i++)
   {
      entry = fabs(data[i]);
      if (entry > max_norm)
         max_norm = entry;
   }

   if (max_norm == 0.0)
      return 0.0;

   fnorm2 = 0.0;
   for (i = 0; i < hw; i++)
   {
      entry = data[i] / max_norm;
      fnorm2 += entry * entry;
   }

   return max_norm * sqrt(fnorm2);
}

#ifdef MFEM_USE_LAPACK
extern "C" void
dsyevr_(char *JOBZ, char *RANGE, char *UPLO, int *N, double *A, int *LDA,
        double *VL, double *VU, int *IL, int *IU, double *ABSTOL, int *M,
        double *W, double *Z, int *LDZ, int *ISUPPZ, double *WORK, int *LWORK,
        int *IWORK, int *LIWORK, int *INFO);
extern "C" void
dsyev_(char *JOBZ, char *UPLO, int *N, double *A, int *LDA, double *W,
       double *WORK, int *LWORK, int *INFO);
extern "C" void
dgesvd_(char *JOBU, char *JOBVT, int *M, int *N, double *A, int *LDA,
        double *S, double *U, int *LDU, double *VT, int *LDVT, double *WORK,
        int *LWORK, int *INFO);
#endif

void dsyevr_Eigensystem(DenseMatrix &a, Vector &ev, DenseMatrix *evect)
{

#ifdef MFEM_USE_LAPACK

   ev.SetSize(a.Width());

   char      JOBZ     = 'N';
   char      RANGE    = 'A';
   char      UPLO     = 'U';
   int       N        = a.Width();
   double   *A        = new double[N*N];
   int       LDA      = N;
   double    VL       = 0.0;
   double    VU       = 1.0;
   int       IL       = 0;
   int       IU       = 1;
   double    ABSTOL   = 0.0;
   int       M;
   double   *W        = ev.GetData();
   double   *Z        = NULL;
   int       LDZ      = 1;
   int      *ISUPPZ   = new int[2*N];
   int       LWORK    = -1; // query optimal (double) workspace size
   double    QWORK;
   double   *WORK     = NULL;
   int       LIWORK   = -1; // query optimal (int) workspace size
   int       QIWORK;
   int      *IWORK    = NULL;
   int       INFO;

   if (evect) // Compute eigenvectors too
   {
      evect->SetSize(N);

      JOBZ     = 'V';
      Z        = evect->Data();
      LDZ      = N;
   }

   int hw = a.Height() * a.Width();
   double *data = a.Data();

   for (int i = 0; i < hw; i++)
      A[i] = data[i];

   dsyevr_( &JOBZ, &RANGE, &UPLO, &N, A, &LDA, &VL, &VU, &IL, &IU,
            &ABSTOL, &M, W, Z, &LDZ, ISUPPZ, &QWORK, &LWORK,
            &QIWORK, &LIWORK, &INFO );

   LWORK  = (int) QWORK;
   LIWORK = QIWORK;

   WORK  = new double[LWORK];
   IWORK = new int[LIWORK];

   dsyevr_( &JOBZ, &RANGE, &UPLO, &N, A, &LDA, &VL, &VU, &IL, &IU,
            &ABSTOL, &M, W, Z, &LDZ, ISUPPZ, WORK, &LWORK,
            IWORK, &LIWORK, &INFO );

   if (INFO != 0)
   {
      cerr << "dsyevr_Eigensystem(...): DSYEVR error code: "
           << INFO << endl;
      mfem_error();
   }

#ifdef MFEM_DEBUG
   if (M < N)
   {
      cerr << "dsyevr_Eigensystem(...):\n"
           << " DSYEVR did not find all eigenvalues "
           << M << "/" << N << endl;
      mfem_error();
   }
   for (IL = 0; IL < N; IL++)
      if (!finite(W[IL]))
         mfem_error("dsyevr_Eigensystem(...): !finite value in W");
   for (IL = 0; IL < N*N; IL++)
      if (!finite(Z[IL]))
         mfem_error("dsyevr_Eigensystem(...): !finite value in Z");
   VU = 0.0;
   for (IL = 0; IL < N; IL++)
      for (IU = 0; IU <= IL; IU++)
      {
         VL = 0.0;
         for (M = 0; M < N; M++)
            VL += Z[M+IL*N] * Z[M+IU*N];
         if (IU < IL)
            VL = fabs(VL);
         else
            VL = fabs(VL-1.0);
         if (VL > VU)
            VU = VL;
         if (VU > 0.5)
         {
            cerr << "dsyevr_Eigensystem(...):"
                 << " Z^t Z - I deviation = " << VU
                 << "\n W[max] = " << W[N-1] << ", W[min] = "
                 << W[0] << ", N = " << N << endl;
            mfem_error();
         }
      }
   if (VU > 1e-9)
   {
      cerr << "dsyevr_Eigensystem(...):"
           << " Z^t Z - I deviation = " << VU
           << "\n W[max] = " << W[N-1] << ", W[min] = "
           << W[0] << ", N = " << N << endl;
   }
   if (VU > 1e-5)
      mfem_error("dsyevr_Eigensystem(...): ERROR: ...");
   VU = 0.0;
   for (IL = 0; IL < N; IL++)
      for (IU = 0; IU < N; IU++)
      {
         VL = 0.0;
         for (M = 0; M < N; M++)
            VL += Z[IL+M*N] * W[M] * Z[IU+M*N];
         VL = fabs(VL-data[IL+N*IU]);
         if (VL > VU)
            VU = VL;
      }
   if (VU > 1e-9)
   {
      cerr << "dsyevr_Eigensystem(...):"
           << " max matrix deviation = " << VU
           << "\n W[max] = " << W[N-1] << ", W[min] = "
           << W[0] << ", N = " << N << endl;
   }
   if (VU > 1e-5)
      mfem_error("dsyevr_Eigensystem(...): ERROR: ...");
#endif

   delete [] IWORK;
   delete [] WORK;
   delete [] ISUPPZ;
   delete [] A;

#endif
}

void dsyev_Eigensystem(DenseMatrix &a, Vector &ev, DenseMatrix *evect)
{

#ifdef MFEM_USE_LAPACK

   int   N      = a.Width();
   char  JOBZ   = 'N';
   char  UPLO   = 'U';
   int   LDA    = N;
   int   LWORK  = -1; /* query optimal workspace size */
   int   INFO;

   ev.SetSize(N);

   double *A    = NULL;
   double *W    = ev.GetData();
   double *WORK = NULL;
   double  QWORK;

   if (evect)
   {
      JOBZ = 'V';
      evect->SetSize(N);
      A = evect->Data();
   }
   else
   {
      A = new double[N*N];
   }

   int hw = a.Height() * a.Width();
   double *data = a.Data();
   for (int i = 0; i < hw; i++)
      A[i] = data[i];

   dsyev_(&JOBZ, &UPLO, &N, A, &LDA, W, &QWORK, &LWORK, &INFO);

   LWORK = (int) QWORK;
   WORK = new double[LWORK];

   dsyev_(&JOBZ, &UPLO, &N, A, &LDA, W, WORK, &LWORK, &INFO);

   if (INFO != 0)
   {
      cerr << "dsyev_Eigensystem: DSYEV error code: " << INFO << endl;
      mfem_error();
   }

   delete [] WORK;
   if (evect == NULL)  delete [] A;

#endif
}

void DenseMatrix::Eigensystem(Vector &ev, DenseMatrix *evect)
{
#ifdef MFEM_USE_LAPACK

   // dsyevr_Eigensystem(*this, ev, evect);

   dsyev_Eigensystem(*this, ev, evect);

#else

   mfem_error("DenseMatrix::Eigensystem");

#endif
}

void DenseMatrix::SingularValues(Vector &sv) const
{
#ifdef MFEM_USE_LAPACK
   DenseMatrix copy_of_this = *this;
   char        jobu         = 'N';
   char        jobvt        = 'N';
   int         m            = Height();
   int         n            = Width();
   double      *a           = copy_of_this.data;
   sv.SetSize(min(m, n));
   double      *s           = sv;
   double      *u           = NULL;
   double      *vt          = NULL;
   double      *work        = NULL;
   int         lwork        = -1;
   int         info;
   double      qwork;

   dgesvd_(&jobu, &jobvt, &m, &n, a, &m,
           s, u, &m, vt, &n, &qwork, &lwork, &info);

   lwork = (int) qwork;
   work = new double[lwork];

   dgesvd_(&jobu, &jobvt, &m, &n, a, &m,
           s, u, &m, vt, &n, work, &lwork, &info);

   delete [] work;
   if (info)
   {
      cerr << "DenseMatrix::SingularValues : info = " << info << endl;
      mfem_error();
   }
#else
   // compiling without lapack
   mfem_error("DenseMatrix::SingularValues");
#endif
}

int DenseMatrix::Rank(double tol) const
{
   int rank=0;
   Vector sv(min(Height(), Width()));
   SingularValues(sv);

   for (int i=0; i < sv.Size(); ++i)
      if (sv(i) >= tol)
         ++rank;

   return rank;
}

static const double sqrt_1_eps = sqrt(1./numeric_limits<double>::epsilon());

inline void Eigenvalues2S(const double &d12, double &d1, double &d2)
{
   if (d12 != 0.)
   {
      // "The Symmetric Eigenvalue Problem", B. N. Parlett, pp.189-190
      double t, zeta = (d2 - d1)/(2*d12); // inf/inf from overflows?
      if (fabs(zeta) < sqrt_1_eps)
         t = d12*copysign(1./(fabs(zeta) + sqrt(1. + zeta*zeta)), zeta);
      else
         t = d12*copysign(0.5/fabs(zeta), zeta);
      d1 -= t;
      d2 += t;
   }
}

inline void Eigensystem2S(const double &d12, double &d1, double &d2,
                          double &c, double &s)
{
   if (d12 == 0.)
   {
      c = 1.;
      s = 0.;
   }
   else
   {
      // "The Symmetric Eigenvalue Problem", B. N. Parlett, pp.189-190
      double t, zeta = (d2 - d1)/(2*d12);
      if (fabs(zeta) < sqrt_1_eps)
         t = copysign(1./(fabs(zeta) + sqrt(1. + zeta*zeta)), zeta);
      else
         t = copysign(0.5/fabs(zeta), zeta);
      // c = 1./sqrt(1. + t*t);
      c = sqrt(1./(1. + t*t));
      s = c*t;
      t *= d12;
      d1 -= t;
      d2 += t;
   }
}

inline void vec_normalize3_aux(
   const double &x1, const double &x2, const double &x3,
   double &n1, double &n2, double &n3)
{
   double m, t, r;

   m = fabs(x1);
   r = x2/m;
   t = 1. + r*r;
   r = x3/m;
   t = sqrt(1./(t + r*r));
   n1 = copysign(t, x1);
   t /= m;
   n2 = x2*t;
   n3 = x3*t;
}

inline void vec_normalize3(const double &x1, const double &x2, const double &x3,
                           double &n1, double &n2, double &n3)
{
   // should work ok when xk is the same as nk for some or all k

   if (fabs(x1) >= fabs(x2))
   {
      if (fabs(x1) >= fabs(x3))
      {
         if (x1 != 0.)
         {
            vec_normalize3_aux(x1, x2, x3, n1, n2, n3);
         }
         else
         {
            n1 = n2 = n3 = 0.;
         }
         return;
      }
   }
   else if (fabs(x2) >= fabs(x3))
   {
      vec_normalize3_aux(x2, x1, x3, n2, n1, n3);
      return;
   }
   vec_normalize3_aux(x3, x1, x2, n3, n1, n2);
}

inline bool KernelVector2G(
   const int &mode,
   double &d1, double &d12, double &d21, double &d2)
{
   // Find a vector (z1,z2) in the "near"-kernel of the matrix
   // |  d1  d12 |
   // | d21   d2 |
   // using QR factorization.
   // The vector (z1,z2) is returned in (d1,d2). Return 'true' if the matrix
   // is zero without setting (d1,d2).
   // Note: in the current implementation |z1| + |z2| = 1.

   // l1-norms of the columns
   double n1 = fabs(d1) + fabs(d21);
   double n2 = fabs(d2) + fabs(d12);

   bool swap_columns = (n2 > n1);
   double mu;

   if (!swap_columns)
   {
      if (n1 == 0.)
         return true;

      if (mode == 0) // eliminate the larger entry in the column
      {
         if (fabs(d1) > fabs(d21))
         {
            Swap(d1, d21);
            Swap(d12, d2);
         }
      }
      else // eliminate the smaller entry in the column
      {
         if (fabs(d1) < fabs(d21))
         {
            Swap(d1, d21);
            Swap(d12, d2);
         }
      }
   }
   else
   {
      // n2 > n1, swap columns 1 and 2
      if (mode == 0) // eliminate the larger entry in the column
      {
         if (fabs(d12) > fabs(d2))
         {
            Swap(d1, d2);
            Swap(d12, d21);
         }
         else
         {
            Swap(d1, d12);
            Swap(d21, d2);
         }
      }
      else // eliminate the smaller entry in the column
      {
         if (fabs(d12) < fabs(d2))
         {
            Swap(d1, d2);
            Swap(d12, d21);
         }
         else
         {
            Swap(d1, d12);
            Swap(d21, d2);
         }
      }
   }

   n1 = hypot(d1, d21);

   if (d21 != 0.)
   {
      // v = (n1, n2)^t,  |v| = 1
      // Q = I - 2 v v^t,  Q (d1, d21)^t = (mu, 0)^t
      mu = copysign(n1, d1);
      n1 = -d21*(d21/(d1 + mu)); // = d1 - mu
      d1 = mu;
      // normalize (n1,d21) to avoid overflow/underflow
      // normalize (n1,d21) by the max-norm to avoid the sqrt call
      if (fabs(n1) <= fabs(d21))
      {
         // (n1,n2) <-- (n1/d21,1)
         n1 = n1/d21;
         mu = (2./(1. + n1*n1))*(n1*d12 + d2);
         d2  = d2  - mu;
         d12 = d12 - mu*n1;
      }
      else
      {
         // (n1,n2) <-- (1,d21/n1)
         n2 = d21/n1;
         mu = (2./(1. + n2*n2))*(d12 + n2*d2);
         d2  = d2  - mu*n2;
         d12 = d12 - mu;
      }
   }

   // Solve:
   // | d1 d12 | | z1 | = | 0 |
   // |  0  d2 | | z2 |   | 0 |

   // choose (z1,z2) to minimize |d1*z1 + d12*z2| + |d2*z2|
   // under the condition |z1| + |z2| = 1, z2 >= 0 (for uniqueness)
   // set t = z1, z2 = 1 - |t|, -1 <= t <= 1
   // objective function is:
   // |d1*t + d12*(1 - |t|)| + |d2|*(1 - |t|) -- piecewise linear with
   // possible minima are -1,0,1,t1 where t1: d1*t1 + d12*(1 - |t1|) = 0
   // values: @t=+/-1 -> |d1|, @t=0 -> |n1| + |d2|, @t=t1 -> |d2|*(1 - |t1|)

   // evaluate z2 @t=t1
   mu = -d12/d1;
   // note: |mu| <= 1,       if using l2-norm for column pivoting
   //       |mu| <= sqrt(2), if using l1-norm
   n2 = 1./(1. + fabs(mu));
   // check if |d1|<=|d2|*z2
   if (fabs(d1) <= n2*fabs(d2))
   {
      d2 = 0.;
      d1 = 1.;
   }
   else
   {
      d2 = n2;
      // d1 = (n2 < 0.5) ? copysign(1. - n2, mu) : mu*n2;
      d1 = mu*n2;
   }

   if (swap_columns)
      Swap(d1, d2);

   return false;
}

inline int KernelVector3G_aux(
   const int &mode,
   double &d1, double &d2, double &d3, double &c12, double &c13, double &c23,
   double &c21, double &c31, double &c32)
{
   int kdim;
   double mu, n1, n2, n3, s1, s2, s3;

   s1 = hypot(c21, c31);
   n1 = hypot(d1, s1);

   if (s1 != 0.)
   {
      // v = (s1, s2, s3)^t,  |v| = 1
      // Q = I - 2 v v^t,  Q (d1, c12, c13)^t = (mu, 0, 0)^t
      mu = copysign(n1, d1);
      n1 = -s1*(s1/(d1 + mu)); // = d1 - mu
      d1 = mu;

      // normalize (n1,c21,c31) to avoid overflow/underflow
      // normalize (n1,c21,c31) by the max-norm to avoid the sqrt call
      if (fabs(n1) >= fabs(c21))
      {
         if (fabs(n1) >= fabs(c31))
         {
            // n1 is max, (s1,s2,s3) <-- (1,c21/n1,c31/n1)
            s2 = c21/n1;
            s3 = c31/n1;
            mu = 2./(1. + s2*s2 + s3*s3);
            n2  = mu*(c12 + s2*d2  + s3*c32);
            n3  = mu*(c13 + s2*c23 + s3*d3);
            c12 = c12 -    n2;
            d2  = d2  - s2*n2;
            c32 = c32 - s3*n2;
            c13 = c13 -    n3;
            c23 = c23 - s2*n3;
            d3  = d3  - s3*n3;
            goto done_column_1;
         }
      }
      else if (fabs(c21) >= fabs(c31))
      {
         // c21 is max, (s1,s2,s3) <-- (n1/c21,1,c31/c21)
         s1 = n1/c21;
         s3 = c31/c21;
         mu = 2./(1. + s1*s1 + s3*s3);
         n2  = mu*(s1*c12 + d2  + s3*c32);
         n3  = mu*(s1*c13 + c23 + s3*d3);
         c12 = c12 - s1*n2;
         d2  = d2  -    n2;
         c32 = c32 - s3*n2;
         c13 = c13 - s1*n3;
         c23 = c23 -    n3;
         d3  = d3  - s3*n3;
         goto done_column_1;
      }
      // c31 is max, (s1,s2,s3) <-- (n1/c31,c21/c31,1)
      s1 = n1/c31;
      s2 = c21/c31;
      mu = 2./(1. + s1*s1 + s2*s2);
      n2  = mu*(s1*c12 + s2*d2  + c32);
      n3  = mu*(s1*c13 + s2*c23 + d3);
      c12 = c12 - s1*n2;
      d2  = d2  - s2*n2;
      c32 = c32 -    n2;
      c13 = c13 - s1*n3;
      c23 = c23 - s2*n3;
      d3  = d3  -    n3;
   }

done_column_1:

   // Solve:
   // |  d2 c23 | | z2 | = | 0 |
   // | c32  d3 | | z3 |   | 0 |
   if (KernelVector2G(mode, d2, c23, c32, d3))
   {
      // Have two solutions:
      // two vectors in the kernel are P (-c12/d1, 1, 0)^t and
      // P (-c13/d1, 0, 1)^t where P is the permutation matrix swapping
      // entries 1 and col.

      // A vector orthogonal to both these vectors is P (1, c12/d1, c13/d1)^t
      d2 = c12/d1;
      d3 = c13/d1;
      d1 = 1.;
      kdim = 2;
   }
   else
   {
      // solve for z1:
      // note: |z1| <= a since |z2| + |z3| = 1, and
      // max{|c12|,|c13|} <= max{norm(col. 2),norm(col. 3)}
      //                  <= norm(col. 1) <= a |d1|
      // a = 1,       if using l2-norm for column pivoting
      // a = sqrt(3), if using l1-norm
      d1 = -(c12*d2 + c13*d3)/d1;
      kdim = 1;
   }

   vec_normalize3(d1, d2, d3, d1, d2, d3);

   return kdim;
}

inline int KernelVector3S(
   const int &mode,
   const double &d12, const double &d13, const double &d23,
   double &d1, double &d2, double &d3)
{
   // Find a unit vector (z1,z2,z3) in the "near"-kernel of the matrix
   // |  d1  d12  d13 |
   // | d12   d2  d23 |
   // | d13  d23   d3 |
   // using QR factorization.
   // The vector (z1,z2,z3) is returned in (d1,d2,d3).
   // Returns the dimension of the kernel, kdim, but never zero.
   // - if kdim == 3, then (d1,d2,d3) is not defined,
   // - if kdim == 2, then (d1,d2,d3) is a vector orthogonal to the kernel,
   // - otherwise kdim == 1 and (d1,d2,d3) is a vector in the "near"-kernel.

   double c12 = d12, c13 = d13, c23 = d23;
   double c21, c31, c32;
   int col, row;

   // l1-norms of the columns:
   c32 = fabs(d1) + fabs(c12) + fabs(c13);
   c31 = fabs(d2) + fabs(c12) + fabs(c23);
   c21 = fabs(d3) + fabs(c13) + fabs(c23);

   // column pivoting: choose the column with the largest norm
   if (c32 >= c21)
      col = (c32 >= c31) ? 1 : 2;
   else
      col = (c31 >= c21) ? 2 : 3;
   switch (col)
   {
   case 1:
      if (c32 == 0.) // zero matrix
         return 3;
      break;

   case 2:
      if (c31 == 0.) // zero matrix
         return 3;
      Swap(c13, c23);
      Swap(d1, d2);
      break;

   case 3:
      if (c21 == 0.) // zero matrix
         return 3;
      Swap(c12, c23);
      Swap(d1, d3);
   }

   // row pivoting depending on 'mode'
   if (mode == 0)
   {
      if (fabs(d1) <= fabs(c13))
         row = (fabs(d1) <= fabs(c12)) ? 1 : 2;
      else
         row = (fabs(c12) <= fabs(c13)) ? 2 : 3;
   }
   else
   {
      if (fabs(d1) >= fabs(c13))
         row = (fabs(d1) >= fabs(c12)) ? 1 : 2;
      else
         row = (fabs(c12) >= fabs(c13)) ? 2 : 3;
   }
   switch (row)
   {
   case 1:
      c21 = c12;
      c31 = c13;
      c32 = c23;
      break;

   case 2:
      c21 = d1;
      c31 = c13;
      c32 = c23;
      d1 = c12;
      c12 = d2;
      d2 = d1;
      c13 = c23;
      c23 = c31;
      break;

   case 3:
      c21 = c12;
      c31 = d1;
      c32 = c12;
      d1 = c13;
      c12 = c23;
      c13 = d3;
      d3 = d1;
   }

   row = KernelVector3G_aux(mode, d1, d2, d3, c12, c13, c23, c21, c31, c32);
   // row is kdim

   switch (col)
   {
   case 2:
      Swap(d1, d2);
      break;

   case 3:
      Swap(d1, d3);
   }

   return row;
}

inline int Reduce3S(
   const int &mode,
   double &d1, double &d2, double &d3, double &d12, double &d13, double &d23,
   double &z1, double &z2, double &z3, double &v1, double &v2, double &v3,
   double &g)
{
   // Given the matrix
   //     |  d1  d12  d13 |
   // A = | d12   d2  d23 |
   //     | d13  d23   d3 |
   // and a unit eigenvector z=(z1,z2,z3), transform the matrix A into the
   // matrix B = Q P A P Q that has the form
   //                 | b1   0   0 |
   // B = Q P A P Q = | 0   b2 b23 |
   //                 | 0  b23  b3 |
   // where P is the permitation matrix switching entries 1 and k, and
   // Q is the reflection matrix Q = I - g v v^t, defined by: set y = P z and
   // v = c(y - e_1); if y = e_1, then v = 0 and Q = I.
   // Note: Q y = e_1, Q e_1 = y ==> Q P A P Q e_1 = ... = lambda e_1.
   // The entries (b1,b2,b3,b23) are returned in (d1,d2,d3,d23), and the
   // return value of the function is k. The variable g = 2/(v1^2+v2^2+v3^3).

   int k;
   double s, w1, w2, w3;

   if (mode == 0)
   {
      // choose k such that z^t e_k = zk has the smallest absolute value, i.e.
      // the angle between z and e_k is closest to pi/2
      if (fabs(z1) <= fabs(z3))
         k = (fabs(z1) <= fabs(z2)) ? 1 : 2;
      else
         k = (fabs(z2) <= fabs(z3)) ? 2 : 3;
   }
   else
   {
      // choose k such that zk is the largest by abosolute value
      if (fabs(z1) >= fabs(z3))
         k = (fabs(z1) >= fabs(z2)) ? 1 : 2;
      else
         k = (fabs(z2) >= fabs(z3)) ? 2 : 3;
   }
   switch (k)
   {
   case 2:
      Swap(d13, d23);
      Swap(d1, d2);
      Swap(z1, z2);
      break;

   case 3:
      Swap(d12, d23);
      Swap(d1, d3);
      Swap(z1, z3);
   }

   s = hypot(z2, z3);

   if (s == 0.)
   {
      // s can not be zero, if zk is the smallest (mode == 0)
      v1 = v2 = v3 = 0.;
      g = 1.;
   }
   else
   {
      g = copysign(1., z1);
      v1 = -s*(s/(z1 + g)); // = z1 - g
      // normalize (v1,z2,z3) by its max-norm, avoiding the sqrt call
      g = fabs(v1);
      if (fabs(z2) > g) g = fabs(z2);
      if (fabs(z3) > g) g = fabs(z3);
      v1 = v1/g;
      v2 = z2/g;
      v3 = z3/g;
      g = 2./(v1*v1 + v2*v2 + v3*v3);

      // Compute Q A Q = A - v w^t - w v^t, where
      // w = u - (g/2)(v^t u) v, and u = g A v
      // set w = g A v
      w1 = g*( d1*v1 + d12*v2 + d13*v3);
      w2 = g*(d12*v1 +  d2*v2 + d23*v3);
      w3 = g*(d13*v1 + d23*v2 +  d3*v3);
      // w := w - (g/2)(v^t w) v
      s = (g/2)*(v1*w1 + v2*w2 + v3*w3);
      w1 -= s*v1;
      w2 -= s*v2;
      w3 -= s*v3;
      // dij -= vi*wj + wi*vj
      d1  -= 2*v1*w1;
      d2  -= 2*v2*w2;
      d23 -= v2*w3 + v3*w2;
      d3  -= 2*v3*w3;
#ifdef MFEM_DEBUG
      // compute the offdiagonal entries on the first row/column of B which
      // should be zero:
      s = d12 - v1*w2 - v2*w1;  // b12 = 0
      s = d13 - v1*w3 - v3*w1;  // b13 = 0
#endif
   }

   switch (k)
   {
   case 2:
      Swap(z1, z2);
      break;

   case 3:
      Swap(z1, z3);
   }

   return k;
}

inline void GetScalingFactor(const double &d_max, double &mult)
{
   int d_exp;
   if (d_max > 0.)
   {
      mult = frexp(d_max, &d_exp);
      if (d_exp == numeric_limits<double>::max_exponent)
         mult *= numeric_limits<double>::radix;
      mult = d_max/mult;
   }
   else
      mult = 1.;
   // mult = 2^d_exp is such that d_max/mult is in [0.5,1)
   // or in other words d_max is in the interval [0.5,1)*mult
}

double DenseMatrix::CalcSingularvalue(const int i) const
{
#ifdef MFEM_DEBUG
   if (Height() != Width() || Height() < 1 || Height() > 3)
      mfem_error("DenseMatrix::CalcSingularvalue");
#endif

   const int n = Height();
   const double *d = data;

   if (n == 1)
   {
      return d[0];
   }
   else if (n == 2)
   {
      double d0, d1, d2, d3;
      d0 = d[0];
      d1 = d[1];
      d2 = d[2];
      d3 = d[3];
      double mult;
      {
         double d_max = fabs(d0);
         if (d_max < fabs(d1)) d_max = fabs(d1);
         if (d_max < fabs(d2)) d_max = fabs(d2);
         if (d_max < fabs(d3)) d_max = fabs(d3);

         GetScalingFactor(d_max, mult);
      }
      d0 /= mult;
      d1 /= mult;
      d2 /= mult;
      d3 /= mult;
      // double b11 = d[0]*d[0] + d[1]*d[1];
      // double b12 = d[0]*d[2] + d[1]*d[3];
      // double b22 = d[2]*d[2] + d[3]*d[3];
      // t = 0.5*(a+b).(a-b) = 0.5*(|a|^2-|b|^2)
      // with a,b - the columns of (*this)
      // double t = 0.5*(b11 - b22);
      double t = 0.5*((d0+d2)*(d0-d2)+(d1-d3)*(d1+d3));
      // double s = sqrt(0.5*(b11 + b22) + sqrt(t*t + b12*b12));
      double s = d0*d2 + d1*d3;
      s = sqrt(0.5*(d0*d0 + d1*d1 + d2*d2 + d3*d3) + sqrt(t*t + s*s));
      if (s == 0.0)
         return 0.0;
      t = fabs(d0*d3 - d1*d2) / s;
      if (t > s)
      {
         if (i == 0)
            return t*mult;
         return s*mult;
      }
      if (i == 0)
         return s*mult;
      return t*mult;
   }
   else
   {
      double d0, d1, d2, d3, d4, d5, d6, d7, d8;
      d0 = d[0];  d3 = d[3];  d6 = d[6];
      d1 = d[1];  d4 = d[4];  d7 = d[7];
      d2 = d[2];  d5 = d[5];  d8 = d[8];
      double mult;
      {
         double d_max = fabs(d0);
         if (d_max < fabs(d1)) d_max = fabs(d1);
         if (d_max < fabs(d2)) d_max = fabs(d2);
         if (d_max < fabs(d3)) d_max = fabs(d3);
         if (d_max < fabs(d4)) d_max = fabs(d4);
         if (d_max < fabs(d5)) d_max = fabs(d5);
         if (d_max < fabs(d6)) d_max = fabs(d6);
         if (d_max < fabs(d7)) d_max = fabs(d7);
         if (d_max < fabs(d8)) d_max = fabs(d8);

         GetScalingFactor(d_max, mult);
      }

      d0 /= mult;  d1 /= mult;  d2 /= mult;
      d3 /= mult;  d4 /= mult;  d5 /= mult;
      d6 /= mult;  d7 /= mult;  d8 /= mult;

      double b11 = d0*d0 + d1*d1 + d2*d2;
      double b12 = d0*d3 + d1*d4 + d2*d5;
      double b13 = d0*d6 + d1*d7 + d2*d8;
      double b22 = d3*d3 + d4*d4 + d5*d5;
      double b23 = d3*d6 + d4*d7 + d5*d8;
      double b33 = d6*d6 + d7*d7 + d8*d8;

      // double a, b, c;
      // a = -(b11 + b22 + b33);
      // b = b11*(b22 + b33) + b22*b33 - b12*b12 - b13*b13 - b23*b23;
      // c = b11*(b23*b23 - b22*b33) + b12*(b12*b33 - 2*b13*b23) + b13*b13*b22;

      // double Q = (a * a - 3 * b) / 9;
      // double Q = (b12*b12 + b13*b13 + b23*b23 +
      //             ((b11 - b22)*(b11 - b22) +
      //              (b11 - b33)*(b11 - b33) +
      //              (b22 - b33)*(b22 - b33))/6)/3;
      // Q = (3*(b12^2 + b13^2 + b23^2) +
      //      ((b11 - b22)^2 + (b11 - b33)^2 + (b22 - b33)^2)/2)/9
      //   or
      // Q = (1/6)*|B-tr(B)/3|_F^2
      // Q >= 0 and
      // Q = 0  <==> B = scalar * I
      // double R = (2 * a * a * a - 9 * a * b + 27 * c) / 54;
      double aa = (b11 + b22 + b33)/3;  // aa = tr(B)/3
      double c1, c2, c3;
      // c1 = b11 - aa; // ((b11 - b22) + (b11 - b33))/3
      // c2 = b22 - aa; // ((b22 - b11) + (b22 - b33))/3
      // c3 = b33 - aa; // ((b33 - b11) + (b33 - b22))/3
      {
         double b11_b22 = ((d0-d3)*(d0+d3)+(d1-d4)*(d1+d4)+(d2-d5)*(d2+d5));
         double b22_b33 = ((d3-d6)*(d3+d6)+(d4-d7)*(d4+d7)+(d5-d8)*(d5+d8));
         double b33_b11 = ((d6-d0)*(d6+d0)+(d7-d1)*(d7+d1)+(d8-d2)*(d8+d2));
         c1 = (b11_b22 - b33_b11)/3;
         c2 = (b22_b33 - b11_b22)/3;
         c3 = (b33_b11 - b22_b33)/3;
      }
      double Q, R;
      Q = (2*(b12*b12 + b13*b13 + b23*b23) + c1*c1 + c2*c2 + c3*c3)/6;
      R = (c1*(b23*b23 - c2*c3)+ b12*(b12*c3 - 2*b13*b23) +b13*b13*c2)/2;
      // R = (-1/2)*det(B-(tr(B)/3)*I)
      // Note: 54*(det(S))^2 <= |S|_F^6, when S^t=S and tr(S)=0, S is 3x3
      // Therefore: R^2 <= Q^3

      if (Q <= 0.)
      {
         ;
      }

      // else if (fabs(R) >= sqrtQ3)
      // {
      //    double det = (d[0] * (d[4] * d[8] - d[5] * d[7]) +
      //                  d[3] * (d[2] * d[7] - d[1] * d[8]) +
      //                  d[6] * (d[1] * d[5] - d[2] * d[4]));
      //
      //    if (R > 0.)
      //    {
      //       if (i == 2)
      //          // aa -= 2*sqrtQ;
      //          return fabs(det)/(aa + sqrtQ);
      //       else
      //          aa += sqrtQ;
      //    }
      //    else
      //    {
      //       if (i != 0)
      //          aa -= sqrtQ;
      //          // aa = fabs(det)/sqrt(aa + 2*sqrtQ);
      //       else
      //          aa += 2*sqrtQ;
      //    }
      // }

      else
      {
         double sqrtQ = sqrt(Q);
         double sqrtQ3 = Q*sqrtQ;
         // double sqrtQ3 = sqrtQ*sqrtQ*sqrtQ;
         // double sqrtQ3 = pow(Q, 1.5);
         double r;

         if (fabs(R) >= sqrtQ3)
         {
            if (R < 0.)
            {
               R = -1.;
               r = 2*sqrtQ;
            }
            else
            {
               R = 1.;
               r = -2*sqrtQ;
            }
         }
         else
         {
            R = R/sqrtQ3;

            // if (fabs(R) <= 0.95)
            if (fabs(R) <= 0.9)
            {
               if (i == 2)
                  aa -= 2*sqrtQ*cos(acos(R)/3); // min
               else if (i == 0)
                  aa -= 2*sqrtQ*cos((acos(R) + 2.0*M_PI)/3); // max
               else
                  aa -= 2*sqrtQ*cos((acos(R) - 2.0*M_PI)/3); // mid
               goto have_aa;
            }

            if (R < 0.)
            {
               r = -2*sqrtQ*cos((acos(R) + 2.0*M_PI)/3); // max
               if (i == 0)
               {
                  aa += r;
                  goto have_aa;
               }
            }
            else
            {
               r = -2*sqrtQ*cos(acos(R)/3); // min
               if (i == 2)
               {
                  aa += r;
                  goto have_aa;
               }
            }
         }

         // (tr(B)/3 + r) is the root which is separated from the other
         // two roots which are close to each other when |R| is close to 1

         c1 -= r;
         c2 -= r;
         c3 -= r;
         // aa += r;

         // Type of Householder reflections: z --> mu ek, where k is the index
         // of the entry in z with:
         // mode == 0: smallest absolute value --> angle closest to pi/2
         //            (eliminate large entries)
         // mode == 1: largest absolute value --> angle farthest from pi/2
         //            (eliminate small entries)
         const int mode = 1;

         // Find a unit vector z = (z1,z2,z3) in the "near"-kernel of
         //  |  c1  b12  b13 |
         //  | b12   c2  b23 | = B - aa*I
         //  | b13  b23   c3 |
         // This vector is also an eigenvector for B corresponding to aa
         // The vector z overwrites (c1,c2,c3).
         switch (KernelVector3S(mode, b12, b13, b23, c1, c2, c3))
         {
         case 3:
            aa += r;
            goto have_aa;
         case 2:
            // ok, continue with the returned vector orthogonal to the kernel
         case 1:
            // ok, continue with the returned vector in the "near"-kernel
            ;
         }

         // Using the eigenvector c = (c1,c2,c3) to transform B into
         //                   | b11   0   0 |
         // B <-- Q P B P Q = |  0  b22 b23 |
         //                   |  0  b23 b33 |
         double v1, v2, v3, g;
         Reduce3S(mode, b11, b22, b33, b12, b13, b23,
                  c1, c2, c3, v1, v2, v3, g);
         // Q = I - g v v^t
         // P - permitation matrix switching rows and columns 1 and k

         // find the eigenvalues of
         //  | b22 b23 |
         //  | b23 b33 |
         Eigenvalues2S(b23, b22, b33);

         if (i == 2)
         {
            aa = std::min(std::min(b11, b22), b33);
         }
         else if (i == 1)
         {
            if (b11 <= b22)
            {
               aa = (b22 <= b33) ? b22 : std::max(b11, b33);
            }
            else
            {
               aa = (b11 <= b33) ? b11 : std::max(b33, b22);
            }
         }
         else
         {
            aa = std::max(std::max(b11, b22), b33);
         }
      }

   have_aa:

      return sqrt(fabs(aa))*mult; // take abs before we sort?
   }
}

void DenseMatrix::CalcEigenvalues(double *lambda, double *vec) const
{
#ifdef MFEM_DEBUG
   if (Height() != Width() || Height() < 2 || Height() > 3)
      mfem_error("DenseMatrix::CalcEigenvalues");
#endif

   const int n = Height();
   const double *d = data;

   if (n == 2)
   {
      double d0 = d[0];
      double d2 = d[2]; // use the upper triangular entry
      double d3 = d[3];

      double c, s;
      Eigensystem2S(d2, d0, d3, c, s);
      if (d0 <= d3)
      {
         lambda[0] = d0;
         lambda[1] = d3;
         vec[0] =  c;
         vec[1] = -s;
         vec[2] =  s;
         vec[3] =  c;
      }
      else
      {
         lambda[0] = d3;
         lambda[1] = d0;
         vec[0] =  s;
         vec[1] =  c;
         vec[2] =  c;
         vec[3] = -s;
      }
   }
   else
   {
      double d11 = d[0];
      double d12 = d[3]; // use the upper triangular entries
      double d22 = d[4];
      double d13 = d[6];
      double d23 = d[7];
      double d33 = d[8];

      double mult;
      {
         double d_max = fabs(d11);
         if (d_max < fabs(d22)) d_max = fabs(d22);
         if (d_max < fabs(d33)) d_max = fabs(d33);
         if (d_max < fabs(d12)) d_max = fabs(d12);
         if (d_max < fabs(d13)) d_max = fabs(d13);
         if (d_max < fabs(d23)) d_max = fabs(d23);

         GetScalingFactor(d_max, mult);
      }

      d11 /= mult;  d22 /= mult;  d33 /= mult;
      d12 /= mult;  d13 /= mult;  d23 /= mult;

      double aa = (d11 + d22 + d33)/3;  // aa = tr(A)/3
      double c1 = d11 - aa;
      double c2 = d22 - aa;
      double c3 = d33 - aa;

      double Q, R;

      Q = (2*(d12*d12 + d13*d13 + d23*d23) + c1*c1 + c2*c2 + c3*c3)/6;
      R = (c1*(d23*d23 - c2*c3)+ d12*(d12*c3 - 2*d13*d23) + d13*d13*c2)/2;

      if (Q <= 0.)
      {
         lambda[0] = lambda[1] = lambda[2] = aa;
         vec[0] = 1.; vec[3] = 0.; vec[6] = 0.;
         vec[1] = 0.; vec[4] = 1.; vec[7] = 0.;
         vec[2] = 0.; vec[5] = 0.; vec[8] = 1.;
      }
      else
      {
         double sqrtQ = sqrt(Q);
         double sqrtQ3 = Q*sqrtQ;
         // double sqrtQ3 = sqrtQ*sqrtQ*sqrtQ;
         // double sqrtQ3 = pow(Q, 1.5);
         double r;
         if (fabs(R) >= sqrtQ3)
         {
            if (R < 0.)
            {
               R = -1.;
               r = 2*sqrtQ;
            }
            else
            {
               R = 1.;
               r = -2*sqrtQ;
            }
         }
         else
         {
            R = R/sqrtQ3;

            if (R < 0.)
            {
               r = -2*sqrtQ*cos((acos(R) + 2.0*M_PI)/3); // max
            }
            else
            {
               r = -2*sqrtQ*cos(acos(R)/3); // min
            }
         }

         aa += r;
         c1 = d11 - aa;
         c2 = d22 - aa;
         c3 = d33 - aa;

         // Type of Householder reflections: z --> mu ek, where k is the index
         // of the entry in z with:
         // mode == 0: smallest absolute value --> angle closest to pi/2
         // mode == 1: largest absolute value --> angle farthest from pi/2
         // Observations:
         // mode == 0 produces better eigenvectors, less accurate eigenvalues?
         // mode == 1 produces better eigenvalues, less accurate eigenvectors?
         const int mode = 0;

         // Find a unit vector z = (z1,z2,z3) in the "near"-kernel of
         //  |  c1  d12  d13 |
         //  | d12   c2  d23 | = A - aa*I
         //  | d13  d23   c3 |
         // This vector is also an eigenvector for A corresponding to aa.
         // The vector z overwrites (c1,c2,c3).
         switch (KernelVector3S(mode, d12, d13, d23, c1, c2, c3))
         {
         case 3:
            // 'aa' is a triple eigenvalue
            lambda[0] = lambda[1] = lambda[2] = aa;
            vec[0] = 1.; vec[3] = 0.; vec[6] = 0.;
            vec[1] = 0.; vec[4] = 1.; vec[7] = 0.;
            vec[2] = 0.; vec[5] = 0.; vec[8] = 1.;
            goto done_3d;

         case 2:
            // ok, continue with the returned vector orthogonal to the kernel
         case 1:
            // ok, continue with the returned vector in the "near"-kernel
            ;
         }

         // Using the eigenvector c=(c1,c2,c3) transform A into
         //                   | d11   0   0 |
         // A <-- Q P A P Q = |  0  d22 d23 |
         //                   |  0  d23 d33 |
         double v1, v2, v3, g;
         int k = Reduce3S(mode, d11, d22, d33, d12, d13, d23,
                          c1, c2, c3, v1, v2, v3, g);
         // Q = I - 2 v v^t
         // P - permitation matrix switching entries 1 and k

         // find the eigenvalues and eigenvectors for
         // | d22 d23 |
         // | d23 d33 |
         double c, s;
         Eigensystem2S(d23, d22, d33, c, s);
         // d22 <-> P Q (0, c, -s), d33 <-> P Q (0, s, c)

         double *vec_1, *vec_2, *vec_3;
         if (d11 <= d22)
         {
            if (d22 <= d33)
            {
               lambda[0] = d11;  vec_1 = vec;
               lambda[1] = d22;  vec_2 = vec + 3;
               lambda[2] = d33;  vec_3 = vec + 6;
            }
            else if (d11 <= d33)
            {
               lambda[0] = d11;  vec_1 = vec;
               lambda[1] = d33;  vec_3 = vec + 3;
               lambda[2] = d22;  vec_2 = vec + 6;
            }
            else
            {
               lambda[0] = d33;  vec_3 = vec;
               lambda[1] = d11;  vec_1 = vec + 3;
               lambda[2] = d22;  vec_2 = vec + 6;
            }
         }
         else
         {
            if (d11 <= d33)
            {
               lambda[0] = d22;  vec_2 = vec;
               lambda[1] = d11;  vec_1 = vec + 3;
               lambda[2] = d33;  vec_3 = vec + 6;
            }
            else if (d22 <= d33)
            {
               lambda[0] = d22;  vec_2 = vec;
               lambda[1] = d33;  vec_3 = vec + 3;
               lambda[2] = d11;  vec_1 = vec + 6;
            }
            else
            {
               lambda[0] = d33;  vec_3 = vec;
               lambda[1] = d22;  vec_2 = vec + 3;
               lambda[2] = d11;  vec_1 = vec + 6;
            }
         }

         vec_1[0] = c1;
         vec_1[1] = c2;
         vec_1[2] = c3;
         d22 = g*(v2*c - v3*s);
         d33 = g*(v2*s + v3*c);
         vec_2[0] =    - v1*d22;  vec_3[0] =   - v1*d33;
         vec_2[1] =  c - v2*d22;  vec_3[1] = s - v2*d33;
         vec_2[2] = -s - v3*d22;  vec_3[2] = c - v3*d33;
         switch (k)
         {
         case 2:
            Swap(vec_2[0], vec_2[1]);
            Swap(vec_3[0], vec_3[1]);
            break;

         case 3:
            Swap(vec_2[0], vec_2[2]);
            Swap(vec_3[0], vec_3[2]);
         }
      }

   done_3d:
      lambda[0] *= mult;
      lambda[1] *= mult;
      lambda[2] *= mult;
   }
}

void DenseMatrix::GetColumn(int c, Vector &col)
{
   int n;
   double *cp, *vp;

   n = Height();
   col.SetSize(n);
   cp = data + c * n;
   vp = col.GetData();

   for (int i = 0; i < n; i++)
      vp[i] = cp[i];
}

void DenseMatrix::GetDiag(Vector &d)
{
   if (height != width)
      mfem_error("DenseMatrix::GetDiag\n");
   d.SetSize(height);

   for (int i = 0; i < height; ++i)
      d(i) = (*this)(i,i);
}

void DenseMatrix::Getl1Diag(Vector &l)
{
   if (height != width)
      mfem_error("DenseMatrix::Getl1Diag\n");
   l.SetSize(height);

   l = 0.0;

   for (int j = 0; j < width; ++j)
      for (int i = 0; i < height; ++i)
         l(i) += fabs((*this)(i,j));
}

void DenseMatrix::Diag(double c, int n)
{
   SetSize(n);

   int i, N = n*n;
   for (i = 0; i < N; i++)
      data[i] = 0.0;
   for (i = 0; i < n; i++)
      data[i*(n+1)] = c;
}

void DenseMatrix::Diag(double *diag, int n)
{
   SetSize(n);

   int i, N = n*n;
   for (i = 0; i < N; i++)
      data[i] = 0.0;
   for (i = 0; i < n; i++)
      data[i*(n+1)] = diag[i];
}

void DenseMatrix::Transpose()
{
   int i, j;
   double t;

   if (Width() == Height())
   {
      for (i = 0; i < Height(); i++)
         for (j = i+1; j < Width(); j++)
         {
            t = (*this)(i,j);
            (*this)(i,j) = (*this)(j,i);
            (*this)(j,i) = t;
         }
   }
   else
   {
      DenseMatrix T(*this,'t');
      (*this) = T;
   }
}

void DenseMatrix::Transpose(DenseMatrix &A)
{
   SetSize(A.Width(),A.Height());

   for (int i = 0; i < Height(); i++)
      for (int j = 0; j < Width(); j++)
         (*this)(i,j) = A(j,i);
}

void DenseMatrix::Symmetrize()
{
#ifdef MFEM_DEBUG
   if (Width() != Height())
      mfem_error("DenseMatrix::Symmetrize() : not a square matrix!");
#endif

   for (int i = 0; i < Height(); i++)
      for (int j = 0; j < i; j++)
      {
         double a = 0.5 * ((*this)(i,j) + (*this)(j,i));
         (*this)(j,i) = (*this)(i,j) = a;
      }
}

void DenseMatrix::Lump()
{
   for (int i = 0; i < Height(); i++)
   {
      double L = 0.0;
      for (int j = 0; j < Width(); j++)
      {
         L += (*this)(i, j);
         (*this)(i, j) = 0.0;
      }
      (*this)(i, i) = L;
   }
}

void DenseMatrix::GradToCurl(DenseMatrix &curl)
{
   int n = Height();

#ifdef MFEM_DEBUG
   if ((Width() != 2 || curl.Width() != 1 || 2*n != curl.Height()) &&
       (Width() != 3 || curl.Width() != 3 || 3*n != curl.Height()))
      mfem_error("DenseMatrix::GradToCurl(...)");
#endif

   if (Width() == 2)
   {
      for (int i = 0; i < n; i++)
      {
         // (x,y) is grad of Ui
         double x = (*this)(i,0);
         double y = (*this)(i,1);

         int j = i+n;

         // curl of (Ui,0)
         curl(i,0) = -y;

         // curl of (0,Ui)
         curl(j,0) =  x;
      }
   }
   else
   {
      for (int i = 0; i < n; i++)
      {
         // (x,y,z) is grad of Ui
         double x = (*this)(i,0);
         double y = (*this)(i,1);
         double z = (*this)(i,2);

         int j = i+n;
         int k = j+n;

         // curl of (Ui,0,0)
         curl(i,0) =  0.;
         curl(i,1) =  z;
         curl(i,2) = -y;

         // curl of (0,Ui,0)
         curl(j,0) = -z;
         curl(j,1) =  0.;
         curl(j,2) =  x;

         // curl of (0,0,Ui)
         curl(k,0) =  y;
         curl(k,1) = -x;
         curl(k,2) =  0.;
      }
   }
}

void DenseMatrix::GradToDiv(Vector &div)
{

#ifdef MFEM_DEBUG
   if (Width()*Height() != div.Size())
      mfem_error("DenseMatrix::GradToDiv(...)");
#endif

   // div(dof*j+i) <-- (*this)(i,j)

   int n = height * width;
   double *ddata = div.GetData();

   for (int i = 0; i < n; i++)
      ddata[i] = data[i];
}

void DenseMatrix::CopyRows(DenseMatrix &A, int row1, int row2)
{
   SetSize(row2 - row1 + 1, A.Width());

   for (int i = row1; i <= row2; i++)
      for (int j = 0; j < Width(); j++)
         (*this)(i-row1,j) = A(i,j);
}

void DenseMatrix::CopyCols(DenseMatrix &A, int col1, int col2)
{
   SetSize(A.Height(), col2 - col1 + 1);

   for (int i = 0; i < Height(); i++)
      for (int j = col1; j <= col2; j++)
         (*this)(i,j-col1) = A(i,j);
}

void DenseMatrix::CopyMN(DenseMatrix &A, int m, int n, int Aro, int Aco)
{
   int i, j;

   SetSize(m,n);

   for (j = 0; j < n; j++)
      for (i = 0; i < m; i++)
         (*this)(i,j) = A(Aro+i,Aco+j);
}

void DenseMatrix::CopyMN(DenseMatrix &A, int row_offset, int col_offset)
{
   int i, j;
   double *v = A.data;

   for (j = 0; j < A.Width(); j++)
      for (i = 0; i < A.Height(); i++)
         (*this)(row_offset+i,col_offset+j) = *(v++);
}

void DenseMatrix::CopyMNt(DenseMatrix &A, int row_offset, int col_offset)
{
   int i, j;
   double *v = A.data;

   for (i = 0; i < A.Width(); i++)
      for (j = 0; j < A.Height(); j++)
         (*this)(row_offset+i,col_offset+j) = *(v++);
}

void DenseMatrix::CopyMNDiag(double c, int n, int row_offset, int col_offset)
{
   int i, j;

   for (i = 0; i < n; i++)
      for (j = i+1; j < n; j++)
         (*this)(row_offset+i,col_offset+j) =
            (*this)(row_offset+j,col_offset+i) = 0.0;

   for (i = 0; i < n; i++)
      (*this)(row_offset+i,col_offset+i) = c;
}

void DenseMatrix::CopyMNDiag(double *diag, int n, int row_offset,
                             int col_offset)
{
   int i, j;

   for (i = 0; i < n; i++)
      for (j = i+1; j < n; j++)
         (*this)(row_offset+i,col_offset+j) =
            (*this)(row_offset+j,col_offset+i) = 0.0;

   for (i = 0; i < n; i++)
      (*this)(row_offset+i,col_offset+i) = diag[i];
}

void DenseMatrix::AddMatrix(DenseMatrix &A, int ro, int co)
{
   int h, ah, aw;
   double *p, *ap;

   h  = Height();
   ah = A.Height();
   aw = A.Width();

#ifdef MFEM_DEBUG
   if (co+aw > Width() || ro+ah > h)
      mfem_error("DenseMatrix::AddMatrix(...) 1");
#endif

   p  = data + ro + co * h;
   ap = A.data;

   for (int c = 0; c < aw; c++)
   {
      for (int r = 0; r < ah; r++)
         p[r] += ap[r];
      p  += h;
      ap += ah;
   }
}

void DenseMatrix::AddMatrix(double a, DenseMatrix &A, int ro, int co)
{
   int h, ah, aw;
   double *p, *ap;

   h  = Height();
   ah = A.Height();
   aw = A.Width();

#ifdef MFEM_DEBUG
   if (co+aw > Width() || ro+ah > h)
      mfem_error("DenseMatrix::AddMatrix(...) 2");
#endif

   p  = data + ro + co * h;
   ap = A.data;

   for (int c = 0; c < aw; c++)
   {
      for (int r = 0; r < ah; r++)
         p[r] += a * ap[r];
      p  += h;
      ap += ah;
   }
}

void DenseMatrix::AddToVector(int offset, Vector &v) const
{
   int i, n = height * width;
   double *vdata = v.GetData() + offset;

   for (i = 0; i < n; i++)
      vdata[i] += data[i];
}

void DenseMatrix::GetFromVector(int offset, const Vector &v)
{
   int i, n = height * width;
   const double *vdata = v.GetData() + offset;

   for (i = 0; i < n; i++)
      data[i] = vdata[i];
}

void DenseMatrix::AdjustDofDirection(Array<int> &dofs)
{
   int n = Height();

#ifdef MFEM_DEBUG
   if (dofs.Size() != n || Width() != n)
      mfem_error("DenseMatrix::AdjustDofDirection(...)");
#endif

   int *dof = dofs;
   for (int i = 0; i < n-1; i++)
   {
      int s = (dof[i] < 0) ? (-1) : (1);
      for (int j = i+1; j < n; j++)
      {
         int t = (dof[j] < 0) ? (-s) : (s);
         if (t < 0)
         {
            (*this)(i,j) = -(*this)(i,j);
            (*this)(j,i) = -(*this)(j,i);
         }
      }
   }
}

void DenseMatrix::SetRow(int row, double value)
{
   for (int j = 0; j < Width(); j++)
      (*this)(row, j) = value;
}

void DenseMatrix::SetCol(int col, double value)
{
   for (int i = 0; i < Height(); i++)
      (*this)(i, col) = value;
}

void DenseMatrix::Print(std::ostream &out, int width_) const
{
   // output flags = scientific + show sign
   out << setiosflags(ios::scientific | ios::showpos);
   for (int i = 0; i < height; i++)
   {
      out << "[row " << i << "]\n";
      for (int j = 0; j < width; j++)
      {
         out << (*this)(i,j);
         if (j+1 == width || (j+1) % width_ == 0)
            out << '\n';
         else
            out << ' ';
      }
   }
}

void DenseMatrix::PrintMatlab(std::ostream &out) const
{
   // output flags = scientific + show sign
   out << setiosflags(ios::scientific | ios::showpos);
   for (int i = 0; i < height; i++)
   {
      for (int j = 0; j < width; j++)
      {
         out << (*this)(i,j);
         out << ' ';
      }
      out << "\n";
   }
}

void DenseMatrix::PrintT(std::ostream &out, int width_) const
{
   // output flags = scientific + show sign
   out << setiosflags(ios::scientific | ios::showpos);
   for (int j = 0; j < width; j++)
   {
      out << "[col " << j << "]\n";
      for (int i = 0; i < height; i++)
      {
         out << (*this)(i,j);
         if (i+1 == height || (i+1) % width_ == 0)
            out << '\n';
         else
            out << ' ';
      }
   }
}

void DenseMatrix::TestInversion()
{
   DenseMatrix copy(*this), C(width);
   Invert();
   mfem::Mult(*this, copy, C);

   double i_max = 0.;
   for (int j = 0; j < width; j++)
      for (int i = 0; i < width; i++)
      {
         if (i == j)
            C(i,j) -= 1.;
         i_max = std::max(i_max, fabs(C(i, j)));
      }
   cout << "size = " << width << ", i_max = " << i_max
        << ", cond_F = " << FNorm()*copy.FNorm() << endl;
}

DenseMatrix::~DenseMatrix()
{
   if (data != NULL)
      delete [] data;
}



void Add(const DenseMatrix &A, const DenseMatrix &B,
         double alpha, DenseMatrix &C)
{
   for (int i = 0; i < C.Height(); i++)
      for (int j = 0; j < C.Width(); j++)
         C(i,j) = A(i,j) + alpha * B(i,j);
}


#ifdef MFEM_USE_LAPACK
extern "C" void
dgemm_(char *, char *, int *, int *, int *, double *, double *,
       int *, double *, int *, double *, double *, int *);
#endif

void Mult(const DenseMatrix &b, const DenseMatrix &c, DenseMatrix &a)
{
#ifdef MFEM_DEBUG
   if (a.height != b.height || a.width != c.width || b.width != c.height)
      mfem_error("Mult (product of DenseMatrices)");
#endif

#ifdef MFEM_USE_LAPACK
   static char transa = 'N', transb = 'N';
   static double alpha = 1.0, beta = 0.0;
   int m = b.Height(), n = c.Width(), k = b.Width();

   dgemm_(&transa, &transb, &m, &n, &k, &alpha, b.Data(), &m,
          c.Data(), &k, &beta, a.Data(), &m);
#else
   int ah = a.height;
   int as = a.width;
   int bs = b.width;
   double *ad = a.data;
   double *bd = b.data;
   double *cd = c.data;
   int i, j, k;
   double d, *bdd, *cdd;

   for (j = 0; j < as; j++, cd += bs)
   {
      for (i = 0; i < ah; i++, ad++)
      {
         d = 0.0;
         bdd = bd+i;
         cdd = cd;
         for (k = 0 ; k < bs; k++)
         {
            d += (*bdd) * (*cdd);
            cdd++;
            bdd += ah;
         }
         *ad = d;
      }
   }
#endif
}

void CalcAdjugate(const DenseMatrix &a, DenseMatrix &adja)
{
#ifdef MFEM_DEBUG
   if (a.Width() > a.Height() || a.Width() < 1 || a.Height() > 3)
      mfem_error("CalcAdjugate(...)");
   if (a.Width() != adja.Height() || a.Height() != adja.Width())
      mfem_error("CalcAdjugate(...)");
#endif

   if (a.Width() < a.Height())
   {
      const double *d = a.Data();
      double *ad = adja.Data();
      if (a.Width() == 1)
      {
         // N x 1, N = 2,3
         ad[0] = d[0];
         ad[1] = d[1];
         if (a.Height() == 3)
            ad[2] = d[2];
      }
      else
      {
         // 3 x 2
         double e, g, f;
         e = d[0]*d[0] + d[1]*d[1] + d[2]*d[2];
         g = d[3]*d[3] + d[4]*d[4] + d[5]*d[5];
         f = d[0]*d[3] + d[1]*d[4] + d[2]*d[5];

         ad[0] = d[0]*g - d[3]*f;
         ad[1] = d[3]*e - d[0]*f;
         ad[2] = d[1]*g - d[4]*f;
         ad[3] = d[4]*e - d[1]*f;
         ad[4] = d[2]*g - d[5]*f;
         ad[5] = d[5]*e - d[2]*f;
      }
      return;
   }

   if (a.Width() == 1)
   {
      adja(0,0) = 1.0;
   }
   else if (a.Width() == 2)
   {
      adja(0,0) =  a(1,1);
      adja(0,1) = -a(0,1);
      adja(1,0) = -a(1,0);
      adja(1,1) =  a(0,0);
   }
   else
   {
      adja(0,0) = a(1,1)*a(2,2)-a(1,2)*a(2,1);
      adja(0,1) = a(0,2)*a(2,1)-a(0,1)*a(2,2);
      adja(0,2) = a(0,1)*a(1,2)-a(0,2)*a(1,1);

      adja(1,0) = a(1,2)*a(2,0)-a(1,0)*a(2,2);
      adja(1,1) = a(0,0)*a(2,2)-a(0,2)*a(2,0);
      adja(1,2) = a(0,2)*a(1,0)-a(0,0)*a(1,2);

      adja(2,0) = a(1,0)*a(2,1)-a(1,1)*a(2,0);
      adja(2,1) = a(0,1)*a(2,0)-a(0,0)*a(2,1);
      adja(2,2) = a(0,0)*a(1,1)-a(0,1)*a(1,0);
   }
}

void CalcAdjugateTranspose(const DenseMatrix &a, DenseMatrix &adjat)
{
#ifdef MFEM_DEBUG
   if (a.Height() != a.Width() || adjat.Height() != adjat.Width() ||
       a.Width() != adjat.Width() || a.Width() < 1 || a.Width() > 3)
      mfem_error("CalcAdjugateTranspose(...)");
#endif
   if (a.Width() == 1)
   {
      adjat(0,0) = 1.0;
   }
   else if (a.Width() == 2)
   {
      adjat(0,0) =  a(1,1);
      adjat(1,0) = -a(0,1);
      adjat(0,1) = -a(1,0);
      adjat(1,1) =  a(0,0);
   }
   else
   {
      adjat(0,0) = a(1,1)*a(2,2)-a(1,2)*a(2,1);
      adjat(1,0) = a(0,2)*a(2,1)-a(0,1)*a(2,2);
      adjat(2,0) = a(0,1)*a(1,2)-a(0,2)*a(1,1);

      adjat(0,1) = a(1,2)*a(2,0)-a(1,0)*a(2,2);
      adjat(1,1) = a(0,0)*a(2,2)-a(0,2)*a(2,0);
      adjat(2,1) = a(0,2)*a(1,0)-a(0,0)*a(1,2);

      adjat(0,2) = a(1,0)*a(2,1)-a(1,1)*a(2,0);
      adjat(1,2) = a(0,1)*a(2,0)-a(0,0)*a(2,1);
      adjat(2,2) = a(0,0)*a(1,1)-a(0,1)*a(1,0);
   }
}

void CalcInverse(const DenseMatrix &a, DenseMatrix &inva)
{
#ifdef MFEM_DEBUG
   if (a.Width() > a.Height() || a.Width() < 1 || a.Height() > 3)
      mfem_error("CalcInverse(...)");
#endif

   double t;

   if (a.Width() < a.Height())
   {
      const double *d = a.Data();
      double *id = inva.Data();
      if (a.Height() == 2)
      {
         t = 1.0 / (d[0]*d[0] + d[1]*d[1]);
         id[0] = d[0] * t;
         id[1] = d[1] * t;
      }
      else
      {
         if (a.Width() == 1)
         {
            t = 1.0 / (d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
            id[0] = d[0] * t;
            id[1] = d[1] * t;
            id[2] = d[2] * t;
         }
         else
         {
            double e, g, f;
            e = d[0]*d[0] + d[1]*d[1] + d[2]*d[2];
            g = d[3]*d[3] + d[4]*d[4] + d[5]*d[5];
            f = d[0]*d[3] + d[1]*d[4] + d[2]*d[5];
            t = 1.0 / (e*g - f*f);
            e *= t; g *= t; f *= t;

            id[0] = d[0]*g - d[3]*f;
            id[1] = d[3]*e - d[0]*f;
            id[2] = d[1]*g - d[4]*f;
            id[3] = d[4]*e - d[1]*f;
            id[4] = d[2]*g - d[5]*f;
            id[5] = d[5]*e - d[2]*f;
         }
      }
      return;
   }

#ifdef MFEM_DEBUG
   t = a.Det();
   if (fabs(t) < 1.0e-14 * pow(a.FNorm()/a.Width(), a.Width()))
      cerr << "CalcInverse(...) : singular matrix!"
           << endl;
   t = 1. / t;
#else
   t = 1.0 / a.Det();
#endif

   switch (a.Height())
   {
   case 1:
      inva(0,0) = t;
      break;
   case 2:
      inva(0,0) = a(1,1) * t ;
      inva(0,1) = -a(0,1) * t ;
      inva(1,0) = -a(1,0) * t ;
      inva(1,1) = a(0,0) * t ;
      break;
   case 3:
      inva(0,0) = (a(1,1)*a(2,2)-a(1,2)*a(2,1))*t;
      inva(0,1) = (a(0,2)*a(2,1)-a(0,1)*a(2,2))*t;
      inva(0,2) = (a(0,1)*a(1,2)-a(0,2)*a(1,1))*t;

      inva(1,0) = (a(1,2)*a(2,0)-a(1,0)*a(2,2))*t;
      inva(1,1) = (a(0,0)*a(2,2)-a(0,2)*a(2,0))*t;
      inva(1,2) = (a(0,2)*a(1,0)-a(0,0)*a(1,2))*t;

      inva(2,0) = (a(1,0)*a(2,1)-a(1,1)*a(2,0))*t;
      inva(2,1) = (a(0,1)*a(2,0)-a(0,0)*a(2,1))*t;
      inva(2,2) = (a(0,0)*a(1,1)-a(0,1)*a(1,0))*t;
      break;
   }
}

void CalcInverseTranspose(const DenseMatrix &a, DenseMatrix &inva)
{
#ifdef MFEM_DEBUG
   if ( (a.Width() != a.Height()) || ( (a.Height()!= 1) && (a.Height()!= 2)
                                       && (a.Height()!= 3) ) )
      mfem_error("CalcInverseTranspose(...)");
#endif

   double t = 1. / a.Det() ;

   switch (a.Height())
   {
   case 1:
      inva(0,0) = 1.0 / a(0,0);
      break;
   case 2:
      inva(0,0) = a(1,1) * t ;
      inva(1,0) = -a(0,1) * t ;
      inva(0,1) = -a(1,0) * t ;
      inva(1,1) = a(0,0) * t ;
      break;
   case 3:
      inva(0,0) = (a(1,1)*a(2,2)-a(1,2)*a(2,1))*t;
      inva(1,0) = (a(0,2)*a(2,1)-a(0,1)*a(2,2))*t;
      inva(2,0) = (a(0,1)*a(1,2)-a(0,2)*a(1,1))*t;

      inva(0,1) = (a(1,2)*a(2,0)-a(1,0)*a(2,2))*t;
      inva(1,1) = (a(0,0)*a(2,2)-a(0,2)*a(2,0))*t;
      inva(2,1) = (a(0,2)*a(1,0)-a(0,0)*a(1,2))*t;

      inva(0,2) = (a(1,0)*a(2,1)-a(1,1)*a(2,0))*t;
      inva(1,2) = (a(0,1)*a(2,0)-a(0,0)*a(2,1))*t;
      inva(2,2) = (a(0,0)*a(1,1)-a(0,1)*a(1,0))*t;
      break;
   }
}

void CalcOrtho(const DenseMatrix &J, Vector &n)
{
#ifdef MFEM_DEBUG
   if (((J.Height() != 2 || J.Width() != 1) &&
        (J.Height() != 3 || J.Width() != 2)) || (J.Height() != n.Size()))
      mfem_error("CalcNormal(...)");
#endif

   const double *d = J.Data();
   if (J.Height() == 2)
   {
      n(0) =  d[1];
      n(1) = -d[0];
   }
   else
   {
      n(0) = d[1]*d[5] - d[2]*d[4];
      n(1) = d[2]*d[3] - d[0]*d[5];
      n(2) = d[0]*d[4] - d[1]*d[3];
   }
}

void MultAAt(const DenseMatrix &a, DenseMatrix &aat)
{
   for (int i = 0; i < a.Height(); i++)
      for (int j = 0; j <= i; j++)
      {
         double temp = 0.;
         for (int k = 0; k < a.Width(); k++)
            temp += a(i,k) * a(j,k);
         aat(j,i) = aat(i,j) = temp;
      }
}

void AddMultADAt(const DenseMatrix &A, const Vector &D, DenseMatrix &ADAt)
{
   for (int i = 0; i < A.Height(); i++)
   {
      for (int j = 0; j < i; j++)
      {
         double t = 0.;
         for (int k = 0; k < A.Width(); k++)
            t += D(k) * A(i, k) * A(j, k);
         ADAt(i, j) += t;
         ADAt(j, i) += t;
      }
   }

   // process diagonal
   for (int i = 0; i < A.Height(); i++)
   {
      double t = 0.;
      for (int k = 0; k < A.Width(); k++)
         t += D(k) * A(i, k) * A(i, k);
      ADAt(i, i) += t;
   }
}

void MultADAt(const DenseMatrix &A, const Vector &D, DenseMatrix &ADAt)
{
   for (int i = 0; i < A.Height(); i++)
   {
      for (int j = 0; j <= i; j++)
      {
         double t = 0.;
         for (int k = 0; k < A.Width(); k++)
            t += D(k) * A(i, k) * A(j, k);
         ADAt(j, i) = ADAt(i, j) = t;
      }
   }
}

void MultABt(const DenseMatrix &A, const DenseMatrix &B, DenseMatrix &ABt)
{
#ifdef MFEM_DEBUG
   if (A.Height() != ABt.Height() || B.Height() != ABt.Width() ||
       A.Width() != B.Width())
      mfem_error("MultABt(...)");
#endif

#ifdef MFEM_USE_LAPACK
   static char transa = 'N', transb = 'T';
   static double alpha = 1.0, beta = 0.0;
   int m = A.Height(), n = B.Height(), k = A.Width();

   dgemm_(&transa, &transb, &m, &n, &k, &alpha, A.Data(), &m,
          B.Data(), &n, &beta, ABt.Data(), &m);
#elif 1
   const int ah = A.Height();
   const int bh = B.Height();
   const int aw = A.Width();
   const double *ad = A.Data();
   const double *bd = B.Data();
   double *cd = ABt.Data();

   for (int i = 0, s = ah*bh; i < s; i++)
      cd[i] = 0.0;
   for (int k = 0; k < aw; k++)
   {
      double *cp = cd;
      for (int j = 0; j < bh; j++)
      {
         const double bjk = bd[j];
         for (int i = 0; i < ah; i++)
         {
            cp[i] += ad[i] * bjk;
         }
         cp += ah;
      }
      ad += ah;
      bd += bh;
   }
#elif 1
   const int ah = A.Height();
   const int bh = B.Height();
   const int aw = A.Width();
   const double *ad = A.Data();
   const double *bd = B.Data();
   double *cd = ABt.Data();

   for (int j = 0; j < bh; j++)
      for (int i = 0; i < ah; i++)
      {
         double d = 0.0;
         const double *ap = ad + i;
         const double *bp = bd + j;
         for (int k = 0; k < aw; k++)
         {
            d += (*ap) * (*bp);
            ap += ah;
            bp += bh;
         }
         *(cd++) = d;
      }
#else
   int i, j, k;
   double d;

   for (i = 0; i < A.Height(); i++)
      for (j = 0; j < B.Height(); j++)
      {
         d = 0.0;
         for (k = 0; k < A.Width(); k++)
            d += A(i, k) * B(j, k);
         ABt(i, j) = d;
      }
#endif
}

void AddMultABt(const DenseMatrix &A, const DenseMatrix &B, DenseMatrix &ABt)
{
#ifdef MFEM_DEBUG
   if (A.Height() != ABt.Height() || B.Height() != ABt.Width() ||
       A.Width() != B.Width())
      mfem_error("AddMultABt(...)");
#endif

#ifdef MFEM_USE_LAPACK
   static char transa = 'N', transb = 'T';
   static double alpha = 1.0, beta = 1.0;
   int m = A.Height(), n = B.Height(), k = A.Width();

   dgemm_(&transa, &transb, &m, &n, &k, &alpha, A.Data(), &m,
          B.Data(), &n, &beta, ABt.Data(), &m);
#elif 1
   const int ah = A.Height();
   const int bh = B.Height();
   const int aw = A.Width();
   const double *ad = A.Data();
   const double *bd = B.Data();
   double *cd = ABt.Data();

   for (int k = 0; k < aw; k++)
   {
      double *cp = cd;
      for (int j = 0; j < bh; j++)
      {
         const double bjk = bd[j];
         for (int i = 0; i < ah; i++)
         {
            cp[i] += ad[i] * bjk;
         }
         cp += ah;
      }
      ad += ah;
      bd += bh;
   }
#else
   int i, j, k;
   double d;

   for (i = 0; i < A.Height(); i++)
      for (j = 0; j < B.Height(); j++)
      {
         d = 0.0;
         for (k = 0; k < A.Width(); k++)
            d += A(i, k) * B(j, k);
         ABt(i, j) += d;
      }
#endif
}

void MultAtB(const DenseMatrix &A, const DenseMatrix &B, DenseMatrix &AtB)
{
#ifdef MFEM_DEBUG
   if (A.Width() != AtB.Height() || B.Width() != AtB.Width() ||
       A.Height() != B.Height())
      mfem_error("MultAtB(...)");
#endif

#ifdef MFEM_USE_LAPACK
   static char transa = 'T', transb = 'N';
   static double alpha = 1.0, beta = 0.0;
   int m = A.Width(), n = B.Width(), k = A.Height();

   dgemm_(&transa, &transb, &m, &n, &k, &alpha, A.Data(), &k,
          B.Data(), &k, &beta, AtB.Data(), &m);
#elif 1
   const int ah = A.Height();
   const int aw = A.Width();
   const int bw = B.Width();
   const double *ad = A.Data();
   const double *bd = B.Data();
   double *cd = AtB.Data();

   for (int j = 0; j < bw; j++)
   {
      const double *ap = ad;
      for (int i = 0; i < aw; i++)
      {
         double d = 0.0;
         for (int k = 0; k < ah; k++)
         {
            d += ap[k] * bd[k];
         }
         *(cd++) = d;
         ap += ah;
      }
      bd += ah;
   }
#else
   int i, j, k;
   double d;

   for (i = 0; i < A.Width(); i++)
      for (j = 0; j < B.Width(); j++)
      {
         d = 0.0;
         for (k = 0; k < A.Height(); k++)
            d += A(k, i) * B(k, j);
         AtB(i, j) = d;
      }
#endif
}

void AddMult_a_AAt(double a, const DenseMatrix &A, DenseMatrix &AAt)
{
   double d;

   for (int i = 0; i < A.Height(); i++)
   {
      for (int j = 0; j < i; j++)
      {
         d = 0.;
         for (int k = 0; k < A.Width(); k++)
            d += A(i,k) * A(j,k);
         AAt(i, j) += (d *= a);
         AAt(j, i) += d;
      }
      d = 0.;
      for (int k = 0; k < A.Width(); k++)
         d += A(i,k) * A(i,k);
      AAt(i, i) += a * d;
   }
}

void Mult_a_AAt(double a, const DenseMatrix &A, DenseMatrix &AAt)
{
   for (int i = 0; i < A.Height(); i++)
      for (int j = 0; j <= i; j++)
      {
         double d = 0.;
         for (int k = 0; k < A.Width(); k++)
            d += A(i,k) * A(j,k);
         AAt(i, j) = AAt(j, i) = a * d;
      }
}

void MultVVt(const Vector &v, DenseMatrix &vvt)
{
   for (int i = 0; i < v.Size(); i++)
      for (int j = 0; j <= i; j++)
      {
         vvt(i,j) = vvt(j,i) = v(i) * v(j);
      }
}

void MultVWt(const Vector &v, const Vector &w, DenseMatrix &VWt)
{
   int i, j;
   double vi;

#ifdef MFEM_DEBUG
   if (v.Size() != VWt.Height() || w.Size() != VWt.Width())
      mfem_error("MultVWt(...)");
#endif

   for (i = 0; i < v.Size(); i++)
   {
      vi = v(i);
      for (j = 0; j < w.Size(); j++)
         VWt(i, j) = vi * w(j);
   }
}

void AddMultVWt(const Vector &v, const Vector &w, DenseMatrix &VWt)
{
   int m = v.Size(), n = w.Size();

#ifdef MFEM_DEBUG
   if (VWt.Height() != m || VWt.Width() != n)
      mfem_error("AddMultVWt(...)");
#endif

   for (int i = 0; i < m; i++)
   {
      double vi = v(i);
      for (int j = 0; j < n; j++)
         VWt(i, j) += vi * w(j);
   }
}

void AddMult_a_VWt(const double a, const Vector &v, const Vector &w, DenseMatrix &VWt)
{
   int m = v.Size(), n = w.Size();

#ifdef MFEM_DEBUG
   if (VWt.Height() != m || VWt.Width() != n)
      mfem_error("AddMultVWt(...)");
#endif

   for (int i = 0; i < m; i++)
   {
      double avi = a * v(i);
      for (int j = 0; j < n; j++)
         VWt(i, j) += avi * w(j);
   }
}

void AddMult_a_VVt(const double a, const Vector &v, DenseMatrix &VVt)
{
   int n = v.Size();

#ifdef MFEM_DEBUG
   if (VVt.Height() != n || VVt.Width() != n)
      mfem_error("AddMult_a_VVt(...)");
#endif

   for (int i = 0; i < n; i++)
   {
      double avi = a * v(i);
      for (int j = 0; j < i; j++)
      {
         double avivj = avi * v(j);
         VVt(i, j) += avivj;
         VVt(j, i) += avivj;
      }
      VVt(i, i) += avi * v(i);
   }
}


DenseMatrixInverse::DenseMatrixInverse(const DenseMatrix &mat)
   : MatrixInverse(mat)
{
   MFEM_ASSERT(height == width, "not a square matrix");
   a = &mat;
   data = new double[width*width];
#ifdef MFEM_USE_LAPACK
   ipiv = new int[width];
#endif
   Factor();
}

DenseMatrixInverse::DenseMatrixInverse(const DenseMatrix *mat)
   : MatrixInverse(*mat)
{
   MFEM_ASSERT(height == width, "not a square matrix");
   a = mat;
   data = new double[width*width];
#ifdef MFEM_USE_LAPACK
   ipiv = new int[width];
#endif
}

void DenseMatrixInverse::Factor()
{
   const double *adata = a->data;

#ifdef MFEM_USE_LAPACK
   for (int i = 0; i < width*width; i++)
      data[i] = adata[i];

   int info;
   dgetrf_(&width, &width, data, &width, ipiv, &info);

   if (info)
      mfem_error("DenseMatrixInverse::Factor : Error in DGETRF");
#else
   // compiling without LAPACK
   int i, j, k;

   // perform LU factorization.
   for (i = 0; i < width; i++)
   {
#ifdef MFEM_DEBUG
      if (i > 0 && data[i-1+width*(i-1)] == 0.0)
         mfem_error("DenseMatrixInverse::Factor()");
#endif
      for (j = 0; j < i; j++)
      {
         data[i+width*j] = adata[i+width*j];
         for (k = 0; k < j; k++)
            data[i+width*j] -= data[i+width*k] * data[k+width*j];
         data[i+width*j] /= data[j+width*j];
      }

      for (j = i; j < width; j++)
      {
         data[i+width*j] = adata[i+width*j];
         for (k = 0; k < i; k++)
            data[i+width*j] -= data[i+width*k] * data[k+width*j];
      }
   }
#endif
}

void DenseMatrixInverse::Factor(const DenseMatrix &mat)
{
#ifdef MFEM_DEBUG
   if (mat.height != mat.width)
      mfem_error("DenseMatrixInverse::Factor #1");
   if (width != mat.width)
      mfem_error("DenseMatrixInverse::Factor #2");
#endif
   a = &mat;

   Factor();
}

void DenseMatrixInverse::SetOperator(const Operator &op)
{
   a = dynamic_cast<const DenseMatrix*>(&op);
   MFEM_VERIFY(a != NULL, "Operator is not a DenseMatrix!");
   MFEM_VERIFY(a->height == a->width, "DenseMatrix is not square!");
   if (width != a->width)
   {
      height = width = a->width;
      delete [] data;
      data = new double[width*width];
#ifdef MFEM_USE_LAPACK
      delete [] ipiv;
      ipiv = new int[width];
#endif
   }
   Factor();
}

void DenseMatrixInverse::Mult(const Vector &x, Vector &y) const
{
#ifdef MFEM_USE_LAPACK
   char trans = 'N';
   int  n     = width;
   int  nrhs  = 1;
   int  info;
   y = x;
   dgetrs_(&trans, &n, &nrhs, data, &n, ipiv, y.GetData(), &n, &info);

   if (info)
      mfem_error("DenseMatrixInverse::Mult #1");
#else
   // compiling without LAPACK
   int i, j;

   // y <- L^{-1} x
   for (i = 0; i < width; i++)
   {
      y(i) = x(i);
      for (j = 0; j < i; j++)
         y(i) -= data[i+width*j] * y(j);
   }

   // y <- U^{-1} y
   for (i = width-1; i >= 0; i--)
   {
      for (j = i+1; j < width; j++)
         y(i) -= data[i+width*j] * y(j);
#ifdef MFEM_DEBUG
      if ((data[i+width*i]) == 0.0)
         mfem_error("DenseMatrixInverse::Mult #2");
#endif
      y(i) /= data[i+width*i];
   }
#endif
}

DenseMatrixInverse::~DenseMatrixInverse()
{
   delete [] data;
#ifdef MFEM_USE_LAPACK
   delete [] ipiv;
#endif
}


DenseMatrixEigensystem::DenseMatrixEigensystem(DenseMatrix &m)
   : mat(m)
{
   n = mat.Width();
   EVal.SetSize(n);
   EVect.SetSize(n);
   ev.SetDataAndSize(NULL, n);

#ifdef MFEM_USE_LAPACK
   jobz = 'V';
   uplo = 'U';
   lwork = -1;
   double qwork;
   dsyev_(&jobz, &uplo, &n, EVect.Data(), &n, EVal.GetData(),
          &qwork, &lwork, &info);

   lwork = (int) qwork;
   work = new double[lwork];
#endif
}

void DenseMatrixEigensystem::Eval()
{
#ifdef MFEM_DEBUG
   if (mat.Width() != n)
      mfem_error("DenseMatrixEigensystem::Eval()");
#endif

#ifdef MFEM_USE_LAPACK
   EVect = mat;
   dsyev_(&jobz, &uplo, &n, EVect.Data(), &n, EVal.GetData(),
          work, &lwork, &info);

   if (info != 0)
   {
      cerr << "DenseMatrixEigensystem::Eval(): DSYEV error code: "
           << info << endl;
      mfem_error();
   }
#else
   mfem_error("DenseMatrixEigensystem::Eval(): Compiled without LAPACK");
#endif
}

DenseMatrixEigensystem::~DenseMatrixEigensystem()
{
#ifdef MFEM_USE_LAPACK
   delete [] work;
#endif
}


DenseMatrixSVD::DenseMatrixSVD(DenseMatrix &M)
{
   m = M.Height();
   n = M.Width();
   Init();
}

DenseMatrixSVD::DenseMatrixSVD(int h, int w)
{
   m = h;
   n = w;
   Init();
}

void DenseMatrixSVD::Init()
{
#ifdef MFEM_USE_LAPACK
   sv.SetSize(min(m, n));

   jobu  = 'N';
   jobvt = 'N';

   double qwork;
   lwork = -1;
   dgesvd_(&jobu, &jobvt, &m, &n, NULL, &m, sv.GetData(), NULL, &m,
           NULL, &n, &qwork, &lwork, &info);

   lwork = (int) qwork;
   work = new double[lwork];
#else
   mfem_error("DenseMatrixSVD::Init(): Compiled without LAPACK");
#endif
}

void DenseMatrixSVD::Eval(DenseMatrix &M)
{
#ifdef MFEM_DEBUG
   if (M.Height() != m || M.Width() != n)
      mfem_error("DenseMatrixSVD::Eval()");
#endif

#ifdef MFEM_USE_LAPACK
   dgesvd_(&jobu, &jobvt, &m, &n, M.Data(), &m, sv.GetData(), NULL, &m,
           NULL, &n, work, &lwork, &info);

   if (info)
   {
      cerr << "DenseMatrixSVD::Eval() : info = " << info << endl;
      mfem_error();
   }
#else
   mfem_error("DenseMatrixSVD::Eval(): Compiled without LAPACK");
#endif
}

DenseMatrixSVD::~DenseMatrixSVD()
{
#ifdef MFEM_USE_LAPACK
   delete [] work;
#endif
}


void DenseTensor::AddMult(const Table &elem_dof, const Vector &x, Vector &y)
   const
{
   int n = SizeI(), ne = SizeK();
   const int *I = elem_dof.GetI(), *J = elem_dof.GetJ(), *dofs;
   double *d_col = tdata, *yp = y, x_col;
   const double *xp = x;
   // the '4' here can be tuned for given platform and compiler
   if (n <= 4)
   {
      for (int i = 0; i < ne; i++)
      {
         dofs = J + I[i];
         for (int col = 0; col < n; col++)
         {
            x_col = xp[dofs[col]];
            for (int row = 0; row < n; row++)
               yp[dofs[row]] += x_col*d_col[row];
            d_col += n;
         }
      }
   }
   else
   {
      Vector ye(n);
      for (int i = 0; i < ne; i++)
      {
         dofs = J + I[i];
         x_col = xp[dofs[0]];
         for (int row = 0; row < n; row++)
            ye(row) = x_col*d_col[row];
         d_col += n;
         for (int col = 1; col < n; col++)
         {
            x_col = xp[dofs[col]];
            for (int row = 0; row < n; row++)
               ye(row) += x_col*d_col[row];
            d_col += n;
         }
         for (int row = 0; row < n; row++)
            yp[dofs[row]] += ye(row);
      }
   }
}

}
