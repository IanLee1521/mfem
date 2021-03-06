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

#include "../fem/fem.hpp"
#include <algorithm>
#if defined(_MSC_VER) && (_MSC_VER < 1800)
#include <float.h>
#define copysign _copysign
#endif

namespace mfem
{

using namespace std;

const int KnotVector::MaxOrder = 10;

KnotVector::KnotVector(std::istream &input)
{
   input >> Order >> NumOfControlPoints;
   knot.Load(input, NumOfControlPoints + Order + 1);
   GetElements();
}

KnotVector::KnotVector(int Order_, int NCP)
{
   Order = Order_;
   NumOfControlPoints = NCP;
   knot.SetSize(NumOfControlPoints + Order + 1);

   knot = -1.;
}

KnotVector &KnotVector::operator=(const KnotVector &kv)
{
   Order = kv.Order;
   NumOfControlPoints = kv.NumOfControlPoints;
   NumOfElements = kv.NumOfElements;
   knot = kv.knot;
   // alternatively, re-compute NumOfElements
   // GetElements();

   return *this;
}

KnotVector *KnotVector::DegreeElevate(int t) const
{
   if (t < 0)
      mfem_error("KnotVector::DegreeElevate :\n"
                 " Parent KnotVector order higher than child");

   int nOrder = Order + t;
   KnotVector *newkv = new KnotVector(nOrder, GetNCP() + t);

   for (int i = 0; i <= nOrder; i++)
   {
      (*newkv)[i] = knot(0);
   }
   for (int i = nOrder + 1; i < newkv->GetNCP(); i++)
   {
      (*newkv)[i] = knot(i - t);
   }
   for (int i = 0; i <= nOrder; i++)
   {
      (*newkv)[newkv->GetNCP() + i] = knot(knot.Size()-1);
   }

   newkv->GetElements();

   return newkv;
}

void KnotVector::UniformRefinement(Vector &newknots) const
{
   newknots.SetSize(NumOfElements);
   int j = 0;
   for (int i = 0; i < knot.Size()-1; i++)
   {
      if (knot(i) != knot(i+1))
      {
         newknots(j) = 0.5*(knot(i) + knot(i+1));
         j++;
      }
   }
}

void KnotVector::GetElements()
{
   NumOfElements = 0;
   for (int i = Order; i < NumOfControlPoints; i++)
   {
      if (knot(i) != knot(i+1))
      {
         NumOfElements++;
      }
   }
}

void KnotVector::Flip()
{
   double apb = knot(0) + knot(knot.Size()-1);

   int ns = (NumOfControlPoints - Order)/2;
   for (int i = 1; i <= ns; i++)
   {
      double tmp = apb - knot(Order + i);
      knot(Order + i) = apb - knot(NumOfControlPoints - i);
      knot(NumOfControlPoints - i) = tmp;
   }
}

void KnotVector::Print(std::ostream &out) const
{
   out << Order << ' ' << NumOfControlPoints << ' ';
   knot.Print(out, knot.Size());
}

// Routine from "The NURBS book" - 2nd ed - Piegl and Tiller
void KnotVector::CalcShape(Vector &shape, int i, double xi)
{
   int    p = Order;
   int    ip = (i >= 0) ? (i + p) : (-1 - i + p);
   double u = getKnotLocation((i >= 0) ? xi : 1. - xi, ip), saved, tmp;
   double left[MaxOrder+1], right[MaxOrder+1];

#ifdef MFEM_DEBUG
   if (p > MaxOrder)
      mfem_error("KnotVector::CalcShape : Order > MaxOrder!");
#endif

   shape(0) = 1.;
   for (int j = 1; j <= p; ++j)
   {
      left[j]  = u - knot(ip+1-j);
      right[j] = knot(ip+j) - u;
      saved    = 0.;
      for (int r = 0; r < j; ++r)
      {
         tmp      = shape(r)/(right[r+1] + left[j-r]);
         shape(r) = saved + right[r+1]*tmp;
         saved    = left[j-r]*tmp;
      }
      shape(j) = saved;
   }
}

// Routine from "The NURBS book" - 2nd ed - Piegl and Tiller
void KnotVector::CalcDShape(Vector &grad, int i, double xi)
{
   int    p = Order, rk, pk;
   int    ip = (i >= 0) ? (i + p) : (-1 - i + p);
   double u = getKnotLocation((i >= 0) ? xi : 1. - xi, ip), temp, saved, d;
   double ndu[MaxOrder+1][MaxOrder+1], left[MaxOrder+1], right[MaxOrder+1];

#ifdef MFEM_DEBUG
   if (p > MaxOrder)
      mfem_error("KnotVector::CalcDShape : Order > MaxOrder!");
#endif

   ndu[0][0] = 1.0;
   for (int j = 1; j <= p; j++)
   {
      left[j] = u - knot(ip-j+1);
      right[j] = knot(ip+j) - u;
      saved = 0.0;
      for (int r = 0; r < j; r++)
      {
         ndu[j][r] = right[r+1] + left[j-r];
         temp = ndu[r][j-1]/ndu[j][r];
         ndu[r][j] = saved + right[r+1]*temp;
         saved = left[j-r]*temp;
      }
      ndu[j][j] = saved;
   }

   for (int r = 0; r <= p; ++r)
   {
      d = 0.0;
      rk = r-1;
      pk = p-1;
      if (r >= 1)
      {
         d = ndu[rk][pk]/ndu[p][rk];
      }
      if (r <= pk)
      {
         d -= ndu[r][pk]/ndu[p][r];
      }
      grad(r) = d;
   }

   if (i >= 0)
      grad *= p*(knot(ip+1) - knot(ip));
   else
      grad *= p*(knot(ip) - knot(ip+1));
}

int KnotVector::findKnotSpan(double u) const
{
   int low, mid, high;

   if (u == knot(NumOfControlPoints+Order))
   {
      mid = NumOfControlPoints;
   }
   else
   {
      low = Order;
      high = NumOfControlPoints + 1;
      mid = (low + high)/2;
      while ( (u < knot(mid-1)) || (u > knot(mid)) )
      {
         if (u < knot(mid-1))
         {
            high = mid;
         }
         else
         {
            low = mid;
         }
         mid = (low + high)/2;
      }
   }
   return mid;
}

void KnotVector::Difference(const KnotVector &kv, Vector &diff) const
{
   if (Order != kv.GetOrder())
   {
      mfem_error("KnotVector::Difference :\n"
                 " Can not compare knot vectors with different orders!");
   }

   int s = kv.Size() - Size();
   if (s < 0)
   {
      kv.Difference(*this, diff);
      return;
   }

   diff.SetSize(s);

   s = 0;
   int i = 0;
   for (int j = 0; j < kv.Size(); j++)
   {
      if (knot(i) == kv[j])
      {
         i++;
      }
      else
      {
         diff(s) = kv[j];
         s++;
      }
   }
}


void NURBSPatch::init(int dim_)
{
   Dim = dim_;
   sd = nd = -1;

   if (kv.Size() == 2)
   {
      ni = kv[0]->GetNCP();
      nj = kv[1]->GetNCP();
      nk = -1;

      data = new double[ni*nj*Dim];

#ifdef MFEM_DEBUG
      for (int i = 0; i < ni*nj*Dim; i++)
         data[i] = -999.99;
#endif
   }
   else if (kv.Size() == 3)
   {
      ni = kv[0]->GetNCP();
      nj = kv[1]->GetNCP();
      nk = kv[2]->GetNCP();

      data = new double[ni*nj*nk*Dim];

#ifdef MFEM_DEBUG
      for (int i = 0; i < ni*nj*nk*Dim; i++)
         data[i] = -999.99;
#endif
   }
   else
   {
      mfem_error("NURBSPatch::init : Wrond dimension of knotvectors!");
   }
}

NURBSPatch::NURBSPatch(std::istream &input)
{
   int pdim, dim, size = 1;
   string ident;

   input >> ws >> ident >> pdim; // knotvectors
   kv.SetSize(pdim);
   for (int i = 0; i < pdim; i++)
   {
      kv[i] = new KnotVector(input);
      size *= kv[i]->GetNCP();
   }

   input >> ws >> ident >> dim; // dimension
   init(dim + 1);

   input >> ws >> ident; // controlpoints (homogeneous coordinates)
   if (ident == "controlpoints" || ident == "controlpoints_homogeneous")
   {
      for (int j = 0, i = 0; i < size; i++)
         for (int d = 0; d <= dim; d++, j++)
            input >> data[j];
   }
   else // "controlpoints_cartesian" (Cartesian coordinates with weight)
   {
      for (int j = 0, i = 0; i < size; i++)
      {
         for (int d = 0; d <= dim; d++)
            input >> data[j+d];
         for (int d = 0; d < dim; d++)
            data[j+d] *= data[j+dim];
         j += (dim+1);
      }
   }
}

NURBSPatch::NURBSPatch(KnotVector *kv0, KnotVector *kv1, int dim_)
{
   kv.SetSize(2);
   kv[0] = new KnotVector(*kv0);
   kv[1] = new KnotVector(*kv1);
   init(dim_);
}

NURBSPatch::NURBSPatch(KnotVector *kv0, KnotVector *kv1, KnotVector *kv2,
                       int dim_)
{
   kv.SetSize(3);
   kv[0] = new KnotVector(*kv0);
   kv[1] = new KnotVector(*kv1);
   kv[2] = new KnotVector(*kv2);
   init(dim_);
}

NURBSPatch::NURBSPatch(Array<KnotVector *> &kv_,  int dim_)
{
   kv.SetSize(kv_.Size());
   for (int i = 0; i < kv.Size(); i++)
      kv[i] = new KnotVector(*kv_[i]);
   init(dim_);
}

NURBSPatch::NURBSPatch(NURBSPatch *parent, int dir, int Order, int NCP)
{
   kv.SetSize(parent->kv.Size());
   for (int i = 0; i < kv.Size(); i++)
      if (i != dir)
         kv[i] = new KnotVector(*parent->kv[i]);
      else
         kv[i] = new KnotVector(Order, NCP);
   init(parent->Dim);
}

void NURBSPatch::swap(NURBSPatch *np)
{
   if (data != NULL)
      delete [] data;

   for (int i = 0; i < kv.Size(); i++)
   {
      if (kv[i]) delete kv[i];
   }

   data = np->data;
   np->kv.Copy(kv);

   ni  = np->ni;
   nj  = np->nj;
   nk  = np->nk;
   Dim = np->Dim;

   np->data = NULL;
   np->kv.SetSize(0);

   delete np;
}

NURBSPatch::~NURBSPatch()
{
   if (data != NULL)
      delete [] data;

   for (int i = 0; i < kv.Size(); i++)
   {
      if (kv[i]) delete kv[i];
   }
}

void NURBSPatch::Print(std::ostream &out)
{
   int size = 1;

   out << "knotvectors\n" << kv.Size() << '\n';
   for (int i = 0; i < kv.Size(); i++)
   {
      kv[i]->Print(out);
      size *= kv[i]->GetNCP();
   }

   out << "\ndimension\n" << Dim - 1
       << "\n\ncontrolpoints\n";
   for (int j = 0, i = 0; i < size; i++)
   {
      out << data[j++];
      for (int d = 1; d < Dim; d++)
         out << ' ' << data[j++];
      out << '\n';
   }
}

int NURBSPatch::SetLoopDirection(int dir)
{
   if (nk == -1)
   {
      if (dir == 0)
      {
         sd = Dim;
         nd = ni;

         return nj*Dim;
      }
      else if (dir == 1)
      {
         sd = ni*Dim;
         nd = nj;

         return ni*Dim;
      }
      else
      {
         cerr << "NURBSPatch::SetLoopDirection :\n"
            " Direction error in 2D patch, dir = " << dir << '\n';
         mfem_error();
      }
   }
   else
   {
      if (dir == 0)
      {
         sd = Dim;
         nd = ni;

         return nj*nk*Dim;
      }
      else if (dir == 1)
      {
         sd = ni*Dim;
         nd = nj;

         return ni*nk*Dim;
      }
      else if (dir == 2)
      {
         sd = ni*nj*Dim;
         nd = nk;

         return ni*nj*Dim;
      }
      else
      {
         cerr << "NURBSPatch::SetLoopDirection :\n"
            " Direction error in 3D patch, dir = " << dir << '\n';
         mfem_error();
      }
   }

   return -1;
}

void NURBSPatch::UniformRefinement()
{
   Vector newknots;
   for (int dir = 0; dir < kv.Size(); dir++)
   {
      kv[dir]->UniformRefinement(newknots);
      KnotInsert(dir, newknots);
   }
}

void NURBSPatch::KnotInsert(Array<KnotVector *> &newkv)
{
   for (int dir = 0; dir < kv.Size(); dir++)
   {
      KnotInsert(dir, *newkv[dir]);
   }
}

void NURBSPatch::KnotInsert(int dir, const KnotVector &newkv)
{
   if (dir >= kv.Size() || dir < 0)
      mfem_error("NURBSPatch::KnotInsert : Incorrect direction!");

   int t = newkv.GetOrder() - kv[dir]->GetOrder();

   if (t > 0)
   {
      DegreeElevate(dir, t);
   }
   else if (t < 0)
   {
      mfem_error("NURBSPatch::KnotInsert : Incorrect order!");
   }

   Vector diff;
   GetKV(dir)->Difference(newkv, diff);
   if (diff.Size() > 0)
   {
      KnotInsert(dir, diff);
   }
}

// Routine from "The NURBS book" - 2nd ed - Piegl and Tiller
void NURBSPatch::KnotInsert(int dir, const Vector &knot)
{
   if (dir >= kv.Size() || dir < 0)
      mfem_error("NURBSPatch::KnotInsert : Incorrect direction!");

   NURBSPatch &oldp  = *this;
   KnotVector &oldkv = *kv[dir];

   NURBSPatch *newpatch = new NURBSPatch(this, dir, oldkv.GetOrder(),
                                         oldkv.GetNCP() + knot.Size());
   NURBSPatch &newp  = *newpatch;
   KnotVector &newkv = *newp.GetKV(dir);

   int size = oldp.SetLoopDirection(dir);
   if (size != newp.SetLoopDirection(dir))
      mfem_error("NURBSPatch::KnotInsert : Size mismatch!");

   int rr = knot.Size() - 1;
   int a  = oldkv.findKnotSpan(knot(0))  - 1;
   int b  = oldkv.findKnotSpan(knot(rr)) - 1;
   int pl = oldkv.GetOrder();
   int ml = oldkv.GetNCP();

   for (int j = 0; j <= a; j++)
   {
      newkv[j] = oldkv[j];
   }
   for (int j = b+pl; j <= ml+pl; j++)
   {
      newkv[j+rr+1] = oldkv[j];
   }
   for (int k = 0; k <= (a-pl); k++)
   {
      for (int ll = 0; ll < size; ll++)
         newp(k,ll) = oldp(k,ll);
   }
   for (int k = (b-1); k < ml; k++)
   {
      for (int ll = 0; ll < size; ll++)
         newp(k+rr+1,ll) = oldp(k,ll);
   }

   int i = b+pl-1;
   int k = b+pl+rr;

   for (int j = rr; j >= 0; j--)
   {
      while ( (knot(j) <= oldkv[i]) && (i > a) )
      {
         newkv[k] = oldkv[i];
         for (int ll = 0; ll < size; ll++)
            newp(k-pl-1,ll) = oldp(i-pl-1,ll);

         k--;
         i--;
      }

      for (int ll = 0; ll < size; ll++)
         newp(k-pl-1,ll) = newp(k-pl,ll);

      for (int l = 1; l <= pl; l++)
      {
         int ind = k-pl+l;
         double alfa = newkv[k+l] - knot(j);
         if (fabs(alfa) == 0.0)
         {
            for (int ll = 0; ll < size; ll++)
               newp(ind-1,ll) = newp(ind,ll);
         }
         else
         {
            alfa = alfa/(newkv[k+l] - oldkv[i-pl+l]);
            for (int ll = 0; ll < size; ll++)
               newp(ind-1,ll) = alfa*newp(ind-1,ll) + (1.0-alfa)*newp(ind,ll);
         }
      }

      newkv[k] = knot(j);
      k--;
   }

   newkv.GetElements();

   swap(newpatch);
}

void NURBSPatch::DegreeElevate(int t)
{
   for (int dir = 0; dir < kv.Size(); dir++)
   {
      DegreeElevate(dir, t);
   }
}

// Routine from "The NURBS book" - 2nd ed - Piegl and Tiller
void NURBSPatch::DegreeElevate(int dir, int t)
{
   if (dir >= kv.Size() || dir < 0)
      mfem_error("NURBSPatch::DegreeElevate : Incorrect direction!");

   int i, j, k, kj, mpi, mul, mh, kind, cind, first, last;
   int r, a, b, oldr, save, s, tr, lbz, rbz, l;
   double inv, ua, ub, numer, alf, den, bet, gam;

   NURBSPatch &oldp  = *this;
   KnotVector &oldkv = *kv[dir];

   NURBSPatch *newpatch = new NURBSPatch(this, dir, oldkv.GetOrder() + t,
                                         oldkv.GetNCP() + oldkv.GetNE()*t);
   NURBSPatch &newp  = *newpatch;
   KnotVector &newkv = *newp.GetKV(dir);

   int size = oldp.SetLoopDirection(dir);
   if (size != newp.SetLoopDirection(dir))
      mfem_error("NURBSPatch::DegreeElevate : Size mismatch!");

   int p = oldkv.GetOrder();
   int n = oldkv.GetNCP()-1;

   DenseMatrix bezalfs (p+t+1, p+1);
   DenseMatrix bpts    (p+1,   size);
   DenseMatrix ebpts   (p+t+1, size);
   DenseMatrix nextbpts(p-1,   size);
   Vector      alphas  (p-1);

   int m = n + p + 1;
   int ph = p + t;
   int ph2 = ph/2;

   {
      Array2D<int> binom(ph+1, ph+1);
      for (i = 0; i <= ph; i++)
      {
         binom(i,0) = binom(i,i) = 1;
         for (j = 1; j < i; j++)
            binom(i,j) = binom(i-1,j) + binom(i-1,j-1);
      }

      bezalfs(0,0)  = 1.0;
      bezalfs(ph,p) = 1.0;

      for (i = 1; i <= ph2; i++)
      {
         inv = 1.0/binom(ph,i);
         mpi = min(p,i);
         for (j = max(0,i-t); j <= mpi; j++)
         {
            bezalfs(i,j) = inv*binom(p,j)*binom(t,i-j);
         }
      }
   }

   for (i = ph2+1; i < ph; i++)
   {
      mpi = min(p,i);
      for (j = max(0,i-t); j <= mpi; j++)
      {
         bezalfs(i,j) = bezalfs(ph-i,p-j);
      }
   }

   mh = ph;
   kind = ph + 1;
   r = -1;
   a = p;
   b = p + 1;
   cind = 1;
   ua = oldkv[0];
   for (l = 0; l < size; l++)
      newp(0,l) = oldp(0,l);
   for (i = 0; i <= ph; i++)
      newkv[i] = ua;

   for (i = 0; i <= p; i++)
   {
      for (l = 0; l < size; l++)
         bpts(i,l) = oldp(i,l);
   }

   while (b < m)
   {
      i = b;
      while (b < m && oldkv[b] == oldkv[b+1])  b++;

      mul = b-i+1;

      mh = mh + mul + t;
      ub = oldkv[b];
      oldr = r;
      r = p-mul;
      if (oldr > 0) lbz = (oldr+2)/2;
      else          lbz = 1;

      if (r > 0) rbz = ph-(r+1)/2;
      else       rbz = ph;

      if (r > 0)
      {
         numer = ub - ua;
         for (k = p ; k > mul; k--)
            alphas[k-mul-1] = numer/(oldkv[a+k]-ua);

         for (j = 1; j <= r; j++)
         {
            save = r-j;
            s = mul+j;
            for (k = p; k >= s; k--)
            {
               for (l = 0; l < size; l++)
                  bpts(k,l) = (alphas[k-s]*bpts(k,l) +
                               (1.0-alphas[k-s])*bpts(k-1,l));
            }
            for (l = 0; l < size; l++)
               nextbpts(save,l) = bpts(p,l);
         }
      }

      for (i = lbz; i <= ph; i++)
      {
         for (l = 0; l < size; l++)
            ebpts(i,l) = 0.0;
         mpi = min(p,i);
         for (j = max(0,i-t); j <= mpi; j++)
         {
            for (l = 0; l < size; l++)
               ebpts(i,l) += bezalfs(i,j)*bpts(j,l);
         }
      }

      if (oldr > 1)
      {
         first = kind-2;
         last = kind;
         den = ub-ua;
         bet = (ub-newkv[kind-1])/den;

         for (tr = 1; tr < oldr; tr++)
         {
            i = first;
            j = last;
            kj = j-kind+1;
            while (j-i > tr)
            {
               if (i < cind)
               {
                  alf = (ub-newkv[i])/(ua-newkv[i]);
                  for (l = 0; l < size; l++)
                     newp(i,l) = alf*newp(i,l)-(1.0-alf)*newp(i-1,l);
               }
               if (j >= lbz)
               {
                  if ((j-tr) <= (kind-ph+oldr))
                  {
                     gam = (ub-newkv[j-tr])/den;
                     for (l = 0; l < size; l++)
                        ebpts(kj,l) = gam*ebpts(kj,l) + (1.0-gam)*ebpts(kj+1,l);
                  }
                  else
                  {
                     for (l = 0; l < size; l++)
                        ebpts(kj,l) = bet*ebpts(kj,l) + (1.0-bet)*ebpts(kj+1,l);
                  }
               }
               i = i+1;
               j = j-1;
               kj = kj-1;
            }
            first--;
            last++;
         }
      }

      if (a != p)
      {
         for (i = 0; i < (ph-oldr); i++)
         {
            newkv[kind] = ua;
            kind = kind+1;
         }
      }
      for (j = lbz; j <= rbz; j++)
      {
         for (l = 0; l < size; l++)
            newp(cind,l) =  ebpts(j,l);
         cind = cind +1;
      }

      if (b < m)
      {
         for (j = 0; j <r; j++)
            for (l = 0; l < size; l++)
               bpts(j,l) = nextbpts(j,l);

         for (j = r; j <= p; j++)
            for (l = 0; l < size; l++)
               bpts(j,l) = oldp(b-p+j,l);

         a = b;
         b = b+1;
         ua = ub;
      }
      else
      {
         for (i = 0; i <= ph; i++)
            newkv[kind+i] = ub;
      }
   }
   newkv.GetElements();

   swap(newpatch);
}

void NURBSPatch::FlipDirection(int dir)
{
   int size = SetLoopDirection(dir);

   for (int id = 0; id < nd/2; id++)
      for (int i = 0; i < size; i++)
         Swap<double>((*this)(id,i), (*this)(nd-1-id,i));
   kv[dir]->Flip();
}

void NURBSPatch::SwapDirections(int dir1, int dir2)
{
   if (abs(dir1-dir2) == 2)
      mfem_error("NURBSPatch::SwapDirections :"
                 " directions 0 and 2 are not supported!");

   Array<KnotVector *> nkv;

   kv.Copy(nkv);
   Swap<KnotVector *>(nkv[dir1], nkv[dir2]);
   NURBSPatch *newpatch = new NURBSPatch(nkv, Dim);

   int size = SetLoopDirection(dir1);
   newpatch->SetLoopDirection(dir2);

   for (int id = 0; id < nd; id++)
      for (int i = 0; i < size; i++)
         (*newpatch)(id,i) = (*this)(id,i);

   swap(newpatch);
}

void NURBSPatch::Get3DRotationMatrix(double n[], double angle, double r,
                                     DenseMatrix &T)
{
   double c, s, c1;
   double l2 = n[0]*n[0] + n[1]*n[1] + n[2]*n[2];
   double l = sqrt(l2);

   if (fabs(angle) == M_PI_2)
   {
      s = r*copysign(1., angle);
      c = 0.;
      c1 = -1.;
   }
   else if (fabs(angle) == M_PI)
   {
      s = 0.;
      c = -r;
      c1 = c - 1.;
   }
   else
   {
      s = r*sin(angle);
      c = r*cos(angle);
      c1 = c - 1.;
   }

   T.SetSize(3);

   T(0,0) =  (n[0]*n[0] + (n[1]*n[1] + n[2]*n[2])*c)/l2;
   T(0,1) = -(n[0]*n[1]*c1)/l2 - (n[2]*s)/l;
   T(0,2) = -(n[0]*n[2]*c1)/l2 + (n[1]*s)/l;
   T(1,0) = -(n[0]*n[1]*c1)/l2 + (n[2]*s)/l;
   T(1,1) =  (n[1]*n[1] + (n[0]*n[0] + n[2]*n[2])*c)/l2;
   T(1,2) = -(n[1]*n[2]*c1)/l2 - (n[0]*s)/l;
   T(2,0) = -(n[0]*n[2]*c1)/l2 - (n[1]*s)/l;
   T(2,1) = -(n[1]*n[2]*c1)/l2 + (n[0]*s)/l;
   T(2,2) =  (n[2]*n[2] + (n[0]*n[0] + n[1]*n[1])*c)/l2;
}

void NURBSPatch::Rotate3D(double n[], double angle)
{
   if (Dim != 4)
      mfem_error("NURBSPatch::Rotate3D : not a NURBSPatch in 3D!");

   DenseMatrix T(3);
   Vector x(3), y(NULL, 3);

   Get3DRotationMatrix(n, angle, 1., T);

   int size = 1;
   for (int i = 0; i < kv.Size(); i++)
      size *= kv[i]->GetNCP();

   for (int i = 0; i < size; i++)
   {
      y.SetData(data + i*Dim);
      x = y;
      T.Mult(x, y);
   }
}

int NURBSPatch::MakeUniformDegree()
{
   int maxd = -1;

   for (int dir = 0; dir < kv.Size(); dir++)
      if (maxd < kv[dir]->GetOrder())
         maxd = kv[dir]->GetOrder();

   for (int dir = 0; dir < kv.Size(); dir++)
      if (maxd > kv[dir]->GetOrder())
         DegreeElevate(dir, maxd - kv[dir]->GetOrder());

   return maxd;
}

NURBSPatch *Interpolate(NURBSPatch &p1, NURBSPatch &p2)
{
   if (p1.kv.Size() != p2.kv.Size() || p1.Dim != p2.Dim)
      mfem_error("Interpolate(NURBSPatch &, NURBSPatch &)");

   int size = 1, dim = p1.Dim;
   Array<KnotVector *> kv(p1.kv.Size() + 1);

   for (int i = 0; i < p1.kv.Size(); i++)
   {
      if (p1.kv[i]->GetOrder() < p2.kv[i]->GetOrder())
      {
         p1.KnotInsert(i, *p2.kv[i]);
         p2.KnotInsert(i, *p1.kv[i]);
      }
      else
      {
         p2.KnotInsert(i, *p1.kv[i]);
         p1.KnotInsert(i, *p2.kv[i]);
      }
      kv[i] = p1.kv[i];
      size *= kv[i]->GetNCP();
   }

   kv.Last() = new KnotVector(1, 2);
   KnotVector &nkv = *kv.Last();
   nkv[0] = nkv[1] = 0.0;
   nkv[2] = nkv[3] = 1.0;
   nkv.GetElements();

   NURBSPatch *patch = new NURBSPatch(kv, dim);
   delete kv.Last();

   for (int i = 0; i < size; i++)
   {
      for (int d = 0; d < dim; d++)
      {
         patch->data[i*dim+d] = p1.data[i*dim+d];
         patch->data[(i+size)*dim+d] = p2.data[i*dim+d];
      }
   }

   return patch;
}

NURBSPatch *Revolve3D(NURBSPatch &patch, double n[], double ang, int times)
{
   if (patch.Dim != 4)
      mfem_error("Revolve3D(NURBSPatch &, double [], double)");

   int size = 1, ns;
   Array<KnotVector *> nkv(patch.kv.Size() + 1);

   for (int i = 0; i < patch.kv.Size(); i++)
   {
      nkv[i] = patch.kv[i];
      size *= nkv[i]->GetNCP();
   }
   ns = 2*times + 1;
   nkv.Last() = new KnotVector(2, ns);
   KnotVector &lkv = *nkv.Last();
   lkv[0] = lkv[1] = lkv[2] = 0.0;
   for (int i = 1; i < times; i++)
      lkv[2*i+1] = lkv[2*i+2] = i;
   lkv[ns] = lkv[ns+1] = lkv[ns+2] = times;
   lkv.GetElements();
   NURBSPatch *newpatch = new NURBSPatch(nkv, 4);
   delete nkv.Last();

   DenseMatrix T(3), T2(3);
   Vector u(NULL, 3), v(NULL, 3);

   NURBSPatch::Get3DRotationMatrix(n, ang, 1., T);
   double c = cos(ang/2);
   NURBSPatch::Get3DRotationMatrix(n, ang/2, 1./c, T2);
   T2 *= c;

   double *op = patch.data, *np;
   for (int i = 0; i < size; i++)
   {
      np = newpatch->data + 4*i;
      for (int j = 0; j < 4; j++)
         np[j] = op[j];
      for (int j = 0; j < times; j++)
      {
         u.SetData(np);
         v.SetData(np += 4*size);
         T2.Mult(u, v);
         v[3] = c*u[3];
         v.SetData(np += 4*size);
         T.Mult(u, v);
         v[3] = u[3];
      }
      op += 4;
   }

   return newpatch;
}


// from mesh.cpp
extern void skip_comment_lines(std::istream &, const char);

NURBSExtension::NURBSExtension(std::istream &input)
{
   // Read topology
   patchTopo = new Mesh;
   patchTopo->LoadPatchTopo(input, edge_to_knot);
   own_topo = 1;

   CheckPatches();
   // CheckBdrPatches();

   skip_comment_lines(input, '#');

   // Read knotvectors or patches
   string ident;
   input >> ws >> ident; // 'knotvectors' or 'patches'
   if (ident == "knotvectors")
   {
      input >> NumOfKnotVectors;
      knotVectors.SetSize(NumOfKnotVectors);
      for (int i = 0; i < NumOfKnotVectors; i++)
      {
         knotVectors[i] = new KnotVector(input);
         if (knotVectors[i]->GetOrder() != knotVectors[0]->GetOrder())
            mfem_error("NURBSExtension::NURBSExtension :\n"
                       " Variable orders are not supported!");
      }
      Order = knotVectors[0]->GetOrder();
   }
   else if (ident == "patches")
   {
      patches.SetSize(GetNP());
      for (int p = 0; p < patches.Size(); p++)
      {
         skip_comment_lines(input, '#');
         patches[p] = new NURBSPatch(input);
      }

      NumOfKnotVectors = 0;
      for (int i = 0; i < patchTopo->GetNEdges(); i++)
         if (NumOfKnotVectors < KnotInd(i))
            NumOfKnotVectors = KnotInd(i);
      NumOfKnotVectors++;
      knotVectors.SetSize(NumOfKnotVectors);
      knotVectors = NULL;

      Array<int> edges, oedge;
      for (int p = 0; p < patches.Size(); p++)
      {
         patchTopo->GetElementEdges(p, edges, oedge);
         if (Dimension() == 2)
         {
            if (knotVectors[KnotInd(edges[0])] == NULL)
               knotVectors[KnotInd(edges[0])] =
                  new KnotVector(*patches[p]->GetKV(0));
            if (knotVectors[KnotInd(edges[1])] == NULL)
               knotVectors[KnotInd(edges[1])] =
                  new KnotVector(*patches[p]->GetKV(1));
         }
         else
         {
            if (knotVectors[KnotInd(edges[0])] == NULL)
               knotVectors[KnotInd(edges[0])] =
                  new KnotVector(*patches[p]->GetKV(0));
            if (knotVectors[KnotInd(edges[3])] == NULL)
               knotVectors[KnotInd(edges[3])] =
                  new KnotVector(*patches[p]->GetKV(1));
            if (knotVectors[KnotInd(edges[8])] == NULL)
               knotVectors[KnotInd(edges[8])] =
                  new KnotVector(*patches[p]->GetKV(2));
         }
      }
      Order = knotVectors[0]->GetOrder();
   }

   GenerateOffsets();
   CountElements();
   CountBdrElements();
   // NumOfVertices, NumOfElements, NumOfBdrElements, NumOfDofs

   skip_comment_lines(input, '#');

   // Check for a list of mesh elements
   if (patches.Size() == 0)
      input >> ws >> ident;
   if (patches.Size() == 0 && ident == "mesh_elements")
   {
      input >> NumOfActiveElems;
      activeElem.SetSize(GetGNE());
      activeElem = false;
      int glob_elem;
      for (int i = 0; i < NumOfActiveElems; i++)
      {
         input >> glob_elem;
         activeElem[glob_elem] = true;
      }

      skip_comment_lines(input, '#');
      input >> ws >> ident;
   }
   else
   {
      NumOfActiveElems = NumOfElements;
      activeElem.SetSize(NumOfElements);
      activeElem = true;
   }

   GenerateActiveVertices();
   GenerateElementDofTable();
   GenerateActiveBdrElems();
   GenerateBdrElementDofTable();

   if (patches.Size() == 0)
   {
      // weights
      if (ident == "weights")
      {
         weights.Load(input, GetNDof());
      }
      else // e.g. ident = "unitweights" or "autoweights"
      {
         weights.SetSize(GetNDof());
         weights = 1.0;
      }
   }
}

NURBSExtension::NURBSExtension(NURBSExtension *parent, int Order_)
{
   Order = Order_;

   patchTopo = parent->patchTopo;
   own_topo = 0;

   parent->edge_to_knot.Copy(edge_to_knot);

   NumOfKnotVectors = parent->GetNKV();
   knotVectors.SetSize(NumOfKnotVectors);
   for (int i = 0; i < NumOfKnotVectors; i++)
   {
      knotVectors[i] =
         parent->GetKnotVector(i)->DegreeElevate(Order-parent->GetOrder());
   }

   // copy some data from parent
   NumOfElements    = parent->NumOfElements;
   NumOfBdrElements = parent->NumOfBdrElements;

   GenerateOffsets(); // dof offsets will be different from parent

   NumOfActiveVertices = parent->NumOfActiveVertices;
   NumOfActiveElems    = parent->NumOfActiveElems;
   NumOfActiveBdrElems = parent->NumOfActiveBdrElems;
   parent->activeVert.Copy(activeVert);
   parent->activeElem.Copy(activeElem);
   parent->activeBdrElem.Copy(activeBdrElem);

   GenerateElementDofTable();
   GenerateBdrElementDofTable();

   weights.SetSize(GetNDof());
   weights = 1.0;
}

NURBSExtension::NURBSExtension(Mesh *mesh_array[], int num_pieces)
{
   NURBSExtension *parent = mesh_array[0]->NURBSext;

   if (!parent->own_topo)
      mfem_error("NURBSExtension::NURBSExtension :\n"
                 "  parent does not own the patch topology!");
   patchTopo = parent->patchTopo;
   own_topo = 1;
   parent->own_topo = 0;

   parent->edge_to_knot.Copy(edge_to_knot);

   Order = parent->GetOrder();

   NumOfKnotVectors = parent->GetNKV();
   knotVectors.SetSize(NumOfKnotVectors);
   for (int i = 0; i < NumOfKnotVectors; i++)
   {
      knotVectors[i] = new KnotVector(*parent->GetKnotVector(i));
   }

   GenerateOffsets();
   CountElements();
   CountBdrElements();

   // assuming the meshes define a partitioning of all the elements
   NumOfActiveElems = NumOfElements;
   activeElem.SetSize(NumOfElements);
   activeElem = true;

   GenerateActiveVertices();
   GenerateElementDofTable();
   GenerateActiveBdrElems();
   GenerateBdrElementDofTable();

   weights.SetSize(GetNDof());
   MergeWeights(mesh_array, num_pieces);
}

NURBSExtension::~NURBSExtension()
{
   delete bel_dof;
   delete el_dof;

   for (int i = 0; i < knotVectors.Size(); i++)
      delete knotVectors[i];

   for (int i = 0; i < patches.Size(); i++)
      delete patches[i];

   if (own_topo)
      delete patchTopo;
}

void NURBSExtension::Print(std::ostream &out) const
{
   patchTopo->PrintTopo(out, edge_to_knot);
   out << "\nknotvectors\n" << NumOfKnotVectors << '\n';
   for (int i = 0; i < NumOfKnotVectors; i++)
   {
      knotVectors[i]->Print(out);
   }

   if (NumOfActiveElems < NumOfElements)
   {
      out << "\nmesh_elements\n" << NumOfActiveElems << '\n';
      for (int i = 0; i < NumOfElements; i++)
         if (activeElem[i])
            out << i << '\n';
   }

   out << "\nweights\n";
   weights.Print(out, 1);
}

void NURBSExtension::PrintCharacteristics(std::ostream &out)
{
   out <<
      "NURBS Mesh entity sizes:\n"
      "Dimension           = " << Dimension() << "\n"
      "Order               = " << GetOrder() << "\n"
      "NumOfKnotVectors    = " << GetNKV() << "\n"
      "NumOfPatches        = " << GetNP() << "\n"
      "NumOfBdrPatches     = " << GetNBP() << "\n"
      "NumOfVertices       = " << GetGNV() << "\n"
      "NumOfElements       = " << GetGNE() << "\n"
      "NumOfBdrElements    = " << GetGNBE() << "\n"
      "NumOfDofs           = " << GetNTotalDof() << "\n"
      "NumOfActiveVertices = " << GetNV() << "\n"
      "NumOfActiveElems    = " << GetNE() << "\n"
      "NumOfActiveBdrElems = " << GetNBE() << "\n"
      "NumOfActiveDofs     = " << GetNDof() << '\n';
   for (int i = 0; i < NumOfKnotVectors; i++)
   {
      out << ' ' << i + 1 << ") ";
      knotVectors[i]->Print(out);
   }
   cout << endl;
}

void NURBSExtension::GenerateActiveVertices()
{
   int vert[8], nv, g_el, nx, ny, nz, dim = Dimension();

   NURBSPatchMap p2g(this);
   KnotVector *kv[3];

   g_el = 0;
   activeVert.SetSize(GetGNV());
   activeVert = -1;
   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchVertexMap(p, kv);

      nx = p2g.nx();
      ny = p2g.ny();
      nz = (dim == 3) ? p2g.nz() : 1;

      for (int k = 0; k < nz; k++)
      {
         for (int j = 0; j < ny; j++)
         {
            for (int i = 0; i < nx; i++)
            {
               if (activeElem[g_el])
               {
                  if (dim == 2)
                  {
                     vert[0] = p2g(i,  j  );
                     vert[1] = p2g(i+1,j  );
                     vert[2] = p2g(i+1,j+1);
                     vert[3] = p2g(i,  j+1);
                     nv = 4;
                  }
                  else
                  {
                     vert[0] = p2g(i,  j,  k);
                     vert[1] = p2g(i+1,j,  k);
                     vert[2] = p2g(i+1,j+1,k);
                     vert[3] = p2g(i,  j+1,k);

                     vert[4] = p2g(i,  j,  k+1);
                     vert[5] = p2g(i+1,j,  k+1);
                     vert[6] = p2g(i+1,j+1,k+1);
                     vert[7] = p2g(i,  j+1,k+1);
                     nv = 8;
                  }

                  for (int v = 0; v < nv; v++)
                     activeVert[vert[v]] = 1;
               }
               g_el++;
            }
         }
      }
   }

   NumOfActiveVertices = 0;
   for (int i = 0; i < GetGNV(); i++)
      if (activeVert[i] == 1)
         activeVert[i] = NumOfActiveVertices++;
}

void NURBSExtension::GenerateActiveBdrElems()
{
   int dim = Dimension();
   Array<KnotVector *> kv(dim);

   activeBdrElem.SetSize(GetGNBE());
   if (GetGNE() == GetNE())
   {
      activeBdrElem = true;
      NumOfActiveBdrElems = GetGNBE();
      return;
   }
   activeBdrElem = false;
   NumOfActiveBdrElems = 0;
   // the mesh will generate the actual boundary including boundary
   // elements that are not on boundary patches. we use this for
   // visialization of processor boundaries

   // TODO: generate actual boundary?
}


void NURBSExtension::MergeWeights(Mesh *mesh_array[], int num_pieces)
{
   Array<int> lelem_elem;

   for (int i = 0; i < num_pieces; i++)
   {
      NURBSExtension *lext = mesh_array[i]->NURBSext;

      lext->GetElementLocalToGlobal(lelem_elem);

      for (int lel = 0; lel < lext->GetNE(); lel++)
      {
         int gel = lelem_elem[lel];

         int nd = el_dof->RowSize(gel);
         int *gdofs = el_dof->GetRow(gel);
         int *ldofs = lext->el_dof->GetRow(lel);
         for (int j = 0; j < nd; j++)
            weights(gdofs[j]) = lext->weights(ldofs[j]);
      }
   }
}

void NURBSExtension::MergeGridFunctions(
   GridFunction *gf_array[], int num_pieces, GridFunction &merged)
{
   FiniteElementSpace *gfes = merged.FESpace();
   Array<int> lelem_elem, dofs;
   Vector lvec;

   for (int i = 0; i < num_pieces; i++)
   {
      FiniteElementSpace *lfes = gf_array[i]->FESpace();
      NURBSExtension *lext = lfes->GetMesh()->NURBSext;

      lext->GetElementLocalToGlobal(lelem_elem);

      for (int lel = 0; lel < lext->GetNE(); lel++)
      {
         lfes->GetElementVDofs(lel, dofs);
         gf_array[i]->GetSubVector(dofs, lvec);

         gfes->GetElementVDofs(lelem_elem[lel], dofs);
         merged.SetSubVector(dofs, lvec);
      }
   }
}

void NURBSExtension::CheckPatches()
{
   Array<int> edges;
   Array<int> oedge;

   for (int p = 0; p < GetNP(); p++)
   {
      patchTopo->GetElementEdges(p, edges, oedge);

      for (int i = 0; i < edges.Size(); i++)
      {
         edges[i] = edge_to_knot[edges[i]];
         if (oedge[i] < 0)
            edges[i] = -1 - edges[i];
      }

      if ((Dimension() == 2 &&
           (edges[0] != -1 - edges[2] || edges[1] != -1 - edges[3])) ||

          (Dimension() == 3 &&
           (edges[0] != edges[2] || edges[0] != edges[4] ||
            edges[0] != edges[6] || edges[1] != edges[3] ||
            edges[1] != edges[5] || edges[1] != edges[7] ||
            edges[8] != edges[9] || edges[8] != edges[10] ||
            edges[8] != edges[11])))
      {
         cerr << "NURBSExtension::CheckPatch (patch = " << p
              << ")\n  Inconsistent edge-to-knot mapping!\n";
         mfem_error();
      }

      if ((Dimension() == 2 &&
           (edges[0] < 0 || edges[1] < 0)) ||

          (Dimension() == 3 &&
           (edges[0] < 0 || edges[3] < 0 || edges[8] < 0)))
      {
         cerr << "NURBSExtension::CheckPatch (patch = " << p
              << ") : Bad orientation!\n";
         mfem_error();
      }
   }
}

void NURBSExtension::CheckBdrPatches()
{
   Array<int> edges;
   Array<int> oedge;

   for (int p = 0; p < GetNBP(); p++)
   {
      patchTopo->GetBdrElementEdges(p, edges, oedge);

      for (int i = 0; i < edges.Size(); i++)
      {
         edges[i] = edge_to_knot[edges[i]];
         if (oedge[i] < 0)
            edges[i] = -1 - edges[i];
      }

      if ((Dimension() == 2 && (edges[0] < 0)) ||
          (Dimension() == 3 && (edges[0] < 0 || edges[1] < 0)))
      {
         cerr << "NURBSExtension::CheckBdrPatch (boundary patch = "
              << p << ") : Bad orientation!\n";
         mfem_error();
      }
   }
}

void NURBSExtension::GetPatchKnotVectors(int p, Array<KnotVector *> &kv)
{
   Array<int> edges;
   Array<int> orient;

   kv.SetSize(Dimension());
   patchTopo->GetElementEdges(p, edges, orient);

   if (Dimension() == 2)
   {
      kv[0] = KnotVec(edges[0]);
      kv[1] = KnotVec(edges[1]);
   }
   else
   {
      kv[0] = KnotVec(edges[0]);
      kv[1] = KnotVec(edges[3]);
      kv[2] = KnotVec(edges[8]);
   }
}

void NURBSExtension::GetBdrPatchKnotVectors(int p, Array<KnotVector *> &kv)
{
   Array<int> edges;
   Array<int> orient;

   kv.SetSize(Dimension() - 1);
   patchTopo->GetBdrElementEdges(p, edges, orient);

   if (Dimension() == 2)
   {
      kv[0] = KnotVec(edges[0]);
   }
   else
   {
      kv[0] = KnotVec(edges[0]);
      kv[1] = KnotVec(edges[1]);
   }
}

void NURBSExtension::GenerateOffsets()
{
   int nv = patchTopo->GetNV();
   int ne = patchTopo->GetNEdges();
   int nf = patchTopo->GetNFaces();
   int np = patchTopo->GetNE();
   int meshCounter, spaceCounter, dim = Dimension();

   Array<int> edges;
   Array<int> orient;

   v_meshOffsets.SetSize(nv);
   e_meshOffsets.SetSize(ne);
   f_meshOffsets.SetSize(nf);
   p_meshOffsets.SetSize(np);

   v_spaceOffsets.SetSize(nv);
   e_spaceOffsets.SetSize(ne);
   f_spaceOffsets.SetSize(nf);
   p_spaceOffsets.SetSize(np);

   // Get vertex offsets
   for (meshCounter = 0; meshCounter < nv; meshCounter++)
   {
      v_meshOffsets[meshCounter]  = meshCounter;
      v_spaceOffsets[meshCounter] = meshCounter;
   }
   spaceCounter = meshCounter;

   // Get edge offsets
   for (int e = 0; e < ne; e++)
   {
      e_meshOffsets[e]  = meshCounter;
      e_spaceOffsets[e] = spaceCounter;
      meshCounter  += KnotVec(e)->GetNE() - 1;
      spaceCounter += KnotVec(e)->GetNCP() - 2;
   }

   // Get face offsets
   for (int f = 0; f < nf; f++)
   {
      f_meshOffsets[f]  = meshCounter;
      f_spaceOffsets[f] = spaceCounter;

      patchTopo->GetFaceEdges(f, edges, orient);

      meshCounter +=
         (KnotVec(edges[0])->GetNE() - 1) *
         (KnotVec(edges[1])->GetNE() - 1);
      spaceCounter +=
         (KnotVec(edges[0])->GetNCP() - 2) *
         (KnotVec(edges[1])->GetNCP() - 2);
   }

   // Get patch offsets
   for (int p = 0; p < np; p++)
   {
      p_meshOffsets[p]  = meshCounter;
      p_spaceOffsets[p] = spaceCounter;

      patchTopo->GetElementEdges(p, edges, orient);

      if (dim == 2)
      {
         meshCounter +=
            (KnotVec(edges[0])->GetNE() - 1) *
            (KnotVec(edges[1])->GetNE() - 1);
         spaceCounter +=
            (KnotVec(edges[0])->GetNCP() - 2) *
            (KnotVec(edges[1])->GetNCP() - 2);
      }
      else
      {
         meshCounter +=
            (KnotVec(edges[0])->GetNE() - 1) *
            (KnotVec(edges[3])->GetNE() - 1) *
            (KnotVec(edges[8])->GetNE() - 1);
         spaceCounter +=
            (KnotVec(edges[0])->GetNCP() - 2) *
            (KnotVec(edges[3])->GetNCP() - 2) *
            (KnotVec(edges[8])->GetNCP() - 2);
      }
   }
   NumOfVertices = meshCounter;
   NumOfDofs     = spaceCounter;
}

void NURBSExtension::CountElements()
{
   int dim = Dimension();
   Array<KnotVector *> kv(dim);

   NumOfElements = 0;
   for (int p = 0; p < GetNP(); p++)
   {
      GetPatchKnotVectors(p, kv);

      int ne = kv[0]->GetNE();
      for (int d = 1; d < dim; d++)
         ne *= kv[d]->GetNE();

      NumOfElements += ne;
   }
}

void NURBSExtension::CountBdrElements()
{
   int dim = Dimension() - 1;
   Array<KnotVector *> kv(dim);

   NumOfBdrElements = 0;
   for (int p = 0; p < GetNBP(); p++)
   {
      GetBdrPatchKnotVectors(p, kv);

      int ne = kv[0]->GetNE();
      for (int d = 1; d < dim; d++)
         ne *= kv[d]->GetNE();

      NumOfBdrElements += ne;
   }
}

void NURBSExtension::GetElementTopo(Array<Element *> &elements)
{
   elements.SetSize(GetNE());

   if (Dimension() == 2)
   {
      Get2DElementTopo(elements);
   }
   else
   {
      Get3DElementTopo(elements);
   }
}

void NURBSExtension::Get2DElementTopo(Array<Element *> &elements)
{
   int el = 0;
   int eg = 0;
   int ind[4];
   NURBSPatchMap p2g(this);
   KnotVector *kv[2];

   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchVertexMap(p, kv);
      int nx = p2g.nx();
      int ny = p2g.ny();

      int patch_attr = patchTopo->GetAttribute(p);

      for (int j = 0; j < ny; j++)
      {
         for (int i = 0; i < nx; i++)
         {
            if (activeElem[eg])
            {
               ind[0] = activeVert[p2g(i,  j  )];
               ind[1] = activeVert[p2g(i+1,j  )];
               ind[2] = activeVert[p2g(i+1,j+1)];
               ind[3] = activeVert[p2g(i,  j+1)];

               elements[el] = new Quadrilateral(ind, patch_attr);
               el++;
            }
            eg++;
         }
      }
   }
}

void NURBSExtension::Get3DElementTopo(Array<Element *> &elements)
{
   int el = 0;
   int eg = 0;
   int ind[8];
   NURBSPatchMap p2g(this);
   KnotVector *kv[3];

   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchVertexMap(p, kv);
      int nx = p2g.nx();
      int ny = p2g.ny();
      int nz = p2g.nz();

      int patch_attr = patchTopo->GetAttribute(p);

      for (int k = 0; k < nz; k++)
      {
         for (int j = 0; j < ny; j++)
         {
            for (int i = 0; i < nx; i++)
            {
               if (activeElem[eg])
               {
                  ind[0] = activeVert[p2g(i,  j,  k)];
                  ind[1] = activeVert[p2g(i+1,j,  k)];
                  ind[2] = activeVert[p2g(i+1,j+1,k)];
                  ind[3] = activeVert[p2g(i,  j+1,k)];

                  ind[4] = activeVert[p2g(i,  j,  k+1)];
                  ind[5] = activeVert[p2g(i+1,j,  k+1)];
                  ind[6] = activeVert[p2g(i+1,j+1,k+1)];
                  ind[7] = activeVert[p2g(i,  j+1,k+1)];

                  elements[el] = new Hexahedron(ind, patch_attr);
                  el++;
               }
               eg++;
            }
         }
      }
   }
}

void NURBSExtension::GetBdrElementTopo(Array<Element *> &boundary)
{
   boundary.SetSize(GetNBE());

   if (Dimension() == 2)
   {
      Get2DBdrElementTopo(boundary);
   }
   else
   {
      Get3DBdrElementTopo(boundary);
   }
}

void NURBSExtension::Get2DBdrElementTopo(Array<Element *> &boundary)
{
   int g_be, l_be;
   int ind[2], okv[1];
   NURBSPatchMap p2g(this);
   KnotVector *kv[1];

   g_be = l_be = 0;
   for (int b = 0; b < GetNBP(); b++)
   {
      p2g.SetBdrPatchVertexMap(b, kv, okv);
      int nx = p2g.nx();

      int bdr_patch_attr = patchTopo->GetBdrAttribute(b);

      for (int i = 0; i < nx; i++)
      {
         if (activeBdrElem[g_be])
         {
            int _i = (okv[0] >= 0) ? i : (nx - 1 - i);
            ind[0] = activeVert[p2g[_i  ]];
            ind[1] = activeVert[p2g[_i+1]];

            boundary[l_be] = new Segment(ind, bdr_patch_attr);
            l_be++;
         }
         g_be++;
      }
   }
}

void NURBSExtension::Get3DBdrElementTopo(Array<Element *> &boundary)
{
   int g_be, l_be;
   int ind[4], okv[2];
   NURBSPatchMap p2g(this);
   KnotVector *kv[2];

   g_be = l_be = 0;
   for (int b = 0; b < GetNBP(); b++)
   {
      p2g.SetBdrPatchVertexMap(b, kv, okv);
      int nx = p2g.nx();
      int ny = p2g.ny();

      int bdr_patch_attr = patchTopo->GetBdrAttribute(b);

      for (int j = 0; j < ny; j++)
      {
         int _j = (okv[1] >= 0) ? j : (ny - 1 - j);
         for (int i = 0; i < nx; i++)
         {
            if (activeBdrElem[g_be])
            {
               int _i = (okv[0] >= 0) ? i : (nx - 1 - i);
               ind[0] = activeVert[p2g(_i,  _j  )];
               ind[1] = activeVert[p2g(_i+1,_j  )];
               ind[2] = activeVert[p2g(_i+1,_j+1)];
               ind[3] = activeVert[p2g(_i,  _j+1)];

               boundary[l_be] = new Quadrilateral(ind, bdr_patch_attr);
               l_be++;
            }
            g_be++;
         }
      }
   }
}

void NURBSExtension::GenerateElementDofTable()
{
   activeDof.SetSize(GetNTotalDof());
   activeDof = 0;

   if (Dimension() == 2)
   {
      Generate2DElementDofTable();
   }
   else
   {
      Generate3DElementDofTable();
   }

   NumOfActiveDofs = 0;
   for (int d = 0; d < GetNTotalDof(); d++)
      if (activeDof[d])
      {
         NumOfActiveDofs++;
         activeDof[d] = NumOfActiveDofs;
      }

   int *dof = el_dof->GetJ();
   int ndof = el_dof->Size_of_connections();
   for (int i = 0; i < ndof; i++)
   {
      dof[i] = activeDof[dof[i]] - 1;
   }
}

void NURBSExtension::Generate2DElementDofTable()
{
   int el = 0;
   int eg = 0;
   KnotVector *kv[2];
   NURBSPatchMap p2g(this);

   el_dof = new Table(NumOfActiveElems, (Order + 1)*(Order + 1));
   el_to_patch.SetSize(NumOfActiveElems);
   el_to_IJK.SetSize(NumOfActiveElems, 2);

   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchDofMap(p, kv);

      // Load dofs
      for (int j = 0; j < kv[1]->GetNKS(); j++)
      {
         if (kv[1]->isElement(j))
         {
            for (int i = 0; i < kv[0]->GetNKS(); i++)
            {
               if (kv[0]->isElement(i))
               {
                  if (activeElem[eg])
                  {
                     int *dofs = el_dof->GetRow(el);
                     int idx = 0;
                     for (int jj = 0; jj <= Order; jj++)
                     {
                        for (int ii = 0; ii <= Order; ii++)
                        {
                           dofs[idx] = p2g(i+ii,j+jj);
                           activeDof[dofs[idx]] = 1;
                           idx++;
                        }
                     }
                     el_to_patch[el] = p;
                     el_to_IJK(el,0) = i;
                     el_to_IJK(el,1) = j;

                     el++;
                  }
                  eg++;
               }
            }
         }
      }
   }
}

void NURBSExtension::Generate3DElementDofTable()
{
   int el = 0;
   int eg = 0;
   int idx;
   KnotVector *kv[3];
   NURBSPatchMap p2g(this);

   el_dof = new Table(NumOfActiveElems, (Order + 1)*(Order + 1)*(Order + 1));
   el_to_patch.SetSize(NumOfActiveElems);
   el_to_IJK.SetSize(NumOfActiveElems, 3);

   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchDofMap(p, kv);

      // Load dofs
      for (int k = 0; k < kv[2]->GetNKS(); k++)
      {
         if (kv[2]->isElement(k))
         {
            for (int j = 0; j < kv[1]->GetNKS(); j++)
            {
               if (kv[1]->isElement(j))
               {
                  for (int i = 0; i < kv[0]->GetNKS(); i++)
                  {
                     if (kv[0]->isElement(i))
                     {
                        if (activeElem[eg])
                        {
                           idx = 0;
                           int *dofs = el_dof->GetRow(el);
                           for (int kk = 0; kk <= Order; kk++)
                           {
                              for (int jj = 0; jj <= Order; jj++)
                              {
                                 for (int ii = 0; ii <= Order; ii++)
                                 {
                                    dofs[idx] = p2g(i+ii,j+jj,k+kk);
                                    activeDof[dofs[idx]] = 1;
                                    idx++;
                                 }
                              }
                           }

                           el_to_patch[el] = p;
                           el_to_IJK(el,0) = i;
                           el_to_IJK(el,1) = j;
                           el_to_IJK(el,2) = k;

                           el++;
                        }
                        eg++;
                     }
                  }
               }
            }
         }
      }
   }
}

void NURBSExtension::GenerateBdrElementDofTable()
{
   if (Dimension() == 2)
   {
      Generate2DBdrElementDofTable();
   }
   else
   {
      Generate3DBdrElementDofTable();
   }

   int *dof = bel_dof->GetJ();
   int ndof = bel_dof->Size_of_connections();
   for (int i = 0; i < ndof; i++)
   {
      dof[i] = activeDof[dof[i]] - 1;
   }
}

void NURBSExtension::Generate2DBdrElementDofTable()
{
   int gbe = 0;
   int lbe = 0, okv[1];
   KnotVector *kv[1];
   NURBSPatchMap p2g(this);

   bel_dof = new Table(NumOfActiveBdrElems, Order + 1);
   bel_to_patch.SetSize(NumOfActiveBdrElems);
   bel_to_IJK.SetSize(NumOfActiveBdrElems, 1);

   for (int b = 0; b < GetNBP(); b++)
   {
      p2g.SetBdrPatchDofMap(b, kv, okv);
      int nx = p2g.nx(); // NCP-1

      // Load dofs
      int nks0 = kv[0]->GetNKS();
      for (int i = 0; i < nks0; i++)
      {
         if (kv[0]->isElement(i))
         {
            if (activeBdrElem[gbe])
            {
               int *dofs = bel_dof->GetRow(lbe);
               int idx = 0;
               for (int ii = 0; ii <= Order; ii++)
               {
                  dofs[idx] = p2g[(okv[0] >= 0) ? (i+ii) : (nx-i-ii)];
                  idx++;
               }
               bel_to_patch[lbe] = b;
               bel_to_IJK(lbe,0) = (okv[0] >= 0) ? i : (-1-i);
               lbe++;
            }
            gbe++;
         }
      }
   }
}

void NURBSExtension::Generate3DBdrElementDofTable()
{
   int gbe = 0;
   int lbe = 0, okv[2];
   KnotVector *kv[2];
   NURBSPatchMap p2g(this);

   bel_dof = new Table(NumOfActiveBdrElems, (Order + 1)*(Order + 1));
   bel_to_patch.SetSize(NumOfActiveBdrElems);
   bel_to_IJK.SetSize(NumOfActiveBdrElems, 2);

   for (int b = 0; b < GetNBP(); b++)
   {
      p2g.SetBdrPatchDofMap(b, kv, okv);
      int nx = p2g.nx(); // NCP0-1
      int ny = p2g.ny(); // NCP1-1

      // Load dofs
      int nks0 = kv[0]->GetNKS();
      int nks1 = kv[1]->GetNKS();
      for (int j = 0; j < nks1; j++)
      {
         if (kv[1]->isElement(j))
         {
            for (int i = 0; i < nks0; i++)
            {
               if (kv[0]->isElement(i))
               {
                  if (activeBdrElem[gbe])
                  {
                     int *dofs = bel_dof->GetRow(lbe);
                     int idx = 0;
                     for (int jj = 0; jj <= Order; jj++)
                     {
                        int _jj = (okv[1] >= 0) ? (j+jj) : (ny-j-jj);
                        for (int ii = 0; ii <= Order; ii++)
                        {
                           int _ii = (okv[0] >= 0) ? (i+ii) : (nx-i-ii);
                           dofs[idx] = p2g(_ii,_jj);
                           idx++;
                        }
                     }
                     bel_to_patch[lbe] = b;
                     bel_to_IJK(lbe,0) = (okv[0] >= 0) ? i : (-1-i);
                     bel_to_IJK(lbe,1) = (okv[1] >= 0) ? j : (-1-j);
                     lbe++;
                  }
                  gbe++;
               }
            }
         }
      }
   }
}

void NURBSExtension::GetVertexLocalToGlobal(Array<int> &lvert_vert)
{
   lvert_vert.SetSize(GetNV());
   for (int gv = 0; gv < GetGNV(); gv++)
      if (activeVert[gv] >= 0)
         lvert_vert[activeVert[gv]] = gv;
}

void NURBSExtension::GetElementLocalToGlobal(Array<int> &lelem_elem)
{
   lelem_elem.SetSize(GetNE());
   for (int le = 0, ge = 0; ge < GetGNE(); ge++)
      if (activeElem[ge])
         lelem_elem[le++] = ge;
}

void NURBSExtension::LoadFE(int i, const FiniteElement *FE)
{
   const NURBSFiniteElement *NURBSFE =
      dynamic_cast<const NURBSFiniteElement *>(FE);

   if (NURBSFE->GetElement() != i)
   {
      Array<int> dofs;
      NURBSFE->SetIJK(el_to_IJK.GetRow(i));
      if (el_to_patch[i] != NURBSFE->GetPatch())
      {
         GetPatchKnotVectors(el_to_patch[i], NURBSFE->KnotVectors());
         NURBSFE->SetPatch(el_to_patch[i]);
      }
      el_dof->GetRow(i, dofs);
      weights.GetSubVector(dofs, NURBSFE->Weights());
      NURBSFE->SetElement(i);
   }
}

void NURBSExtension::LoadBE(int i, const FiniteElement *BE)
{
   const NURBSFiniteElement *NURBSFE =
      dynamic_cast<const NURBSFiniteElement *>(BE);

   if (NURBSFE->GetElement() != i)
   {
      Array<int> dofs;
      NURBSFE->SetIJK(bel_to_IJK.GetRow(i));
      if (bel_to_patch[i] != NURBSFE->GetPatch())
      {
         GetBdrPatchKnotVectors(bel_to_patch[i], NURBSFE->KnotVectors());
         NURBSFE->SetPatch(bel_to_patch[i]);
      }
      bel_dof->GetRow(i, dofs);
      weights.GetSubVector(dofs, NURBSFE->Weights());
      NURBSFE->SetElement(i);
   }
}

void NURBSExtension::ConvertToPatches(const Vector &Nodes)
{
   delete el_dof;
   delete bel_dof;

   if (patches.Size() == 0)
      GetPatchNets(Nodes);
}

void NURBSExtension::SetCoordsFromPatches(Vector &Nodes)
{
   if (patches.Size() == 0) return;

   SetSolutionVector(Nodes);
   patches.SetSize(0);
}

void NURBSExtension::SetKnotsFromPatches()
{
   if (patches.Size() == 0)
      mfem_error("NURBSExtension::SetKnotsFromPatches :"
                 " No patches available!");

   Array<KnotVector *> kv;

   for (int p = 0; p < patches.Size(); p++)
   {
      GetPatchKnotVectors(p, kv);

      for (int i = 0; i < kv.Size(); i++)
         *kv[i] = *patches[p]->GetKV(i);
   }

   Order = knotVectors[0]->GetOrder();
   for (int i = 0; i < NumOfKnotVectors; i++)
   {
      if (Order != knotVectors[i]->GetOrder())
         mfem_error("NURBSExtension::Reset :\n"
                    " Variable orders are not supported!");
   }

   GenerateOffsets();
   CountElements();
   CountBdrElements();

   // all elements must be active
   NumOfActiveElems = NumOfElements;
   activeElem.SetSize(NumOfElements);
   activeElem = true;

   GenerateActiveVertices();
   GenerateElementDofTable();
   GenerateActiveBdrElems();
   GenerateBdrElementDofTable();
}

void NURBSExtension::DegreeElevate(int t)
{
   for (int p = 0; p < patches.Size(); p++)
   {
      patches[p]->DegreeElevate(t);
   }
}

void NURBSExtension::UniformRefinement()
{
   for (int p = 0; p < patches.Size(); p++)
   {
      patches[p]->UniformRefinement();
   }
}

void NURBSExtension::KnotInsert(Array<KnotVector *> &kv)
{
   Array<int> edges;
   Array<int> orient;

   Array<KnotVector *> pkv(Dimension());

   for (int p = 0; p < patches.Size(); p++)
   {
      patchTopo->GetElementEdges(p, edges, orient);

      if (Dimension()==2)
      {
         pkv[0] = kv[KnotInd(edges[0])];
         pkv[1] = kv[KnotInd(edges[1])];
      }
      else
      {
         pkv[0] = kv[KnotInd(edges[0])];
         pkv[1] = kv[KnotInd(edges[3])];
         pkv[2] = kv[KnotInd(edges[8])];
      }

      patches[p]->KnotInsert(pkv);
   }
}

void NURBSExtension::GetPatchNets(const Vector &coords)
{
   if (Dimension() == 2)
   {
      Get2DPatchNets(coords);
   }
   else
   {
      Get3DPatchNets(coords);
   }
}

void NURBSExtension::Get2DPatchNets(const Vector &coords)
{
   Array<KnotVector *> kv(2);
   NURBSPatchMap p2g(this);

   patches.SetSize(GetNP());
   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchDofMap(p, kv);
      patches[p] = new NURBSPatch(kv, 2+1);
      NURBSPatch &Patch = *patches[p];

      for (int j = 0; j < kv[1]->GetNCP(); j++)
      {
         for (int i = 0; i < kv[0]->GetNCP(); i++)
         {
            int l = p2g(i,j);
            for (int d = 0; d < 2; d++)
            {
               Patch(i,j,d) = coords(l*2 + d)*weights(l);
            }
            Patch(i,j,2) = weights(l);
         }
      }
   }
}

void NURBSExtension::Get3DPatchNets(const Vector &coords)
{
   Array<KnotVector *> kv(3);
   NURBSPatchMap p2g(this);

   patches.SetSize(GetNP());
   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchDofMap(p, kv);
      patches[p] = new NURBSPatch(kv, 3+1);
      NURBSPatch &Patch = *patches[p];

      for (int k = 0; k < kv[2]->GetNCP(); k++)
      {
         for (int j = 0; j < kv[1]->GetNCP(); j++)
         {
            for (int i = 0; i < kv[0]->GetNCP(); i++)
            {
               int l = p2g(i,j,k);
               for (int d = 0; d < 3; d++)
               {
                  Patch(i,j,k,d) = coords(l*3 + d)*weights(l);
               }
               Patch(i,j,k,3) = weights(l);
            }
         }
      }
   }
}

void NURBSExtension::SetSolutionVector(Vector &coords)
{
   if (Dimension() == 2)
   {
      Set2DSolutionVector(coords);
   }
   else
   {
      Set3DSolutionVector(coords);
   }
}

void NURBSExtension::Set2DSolutionVector(Vector &coords)
{
   Array<KnotVector *> kv(2);
   NURBSPatchMap p2g(this);

   weights.SetSize(GetNDof());
   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchDofMap(p, kv);
      NURBSPatch &Patch = *patches[p];

      for (int j = 0; j < kv[1]->GetNCP(); j++)
      {
         for (int i = 0; i < kv[0]->GetNCP(); i++)
         {
            int l = p2g(i,j);
            for (int d = 0; d < 2; d++)
            {
               coords(l*2 + d) = Patch(i,j,d)/Patch(i,j,2);
            }
            weights(l) = Patch(i,j,2);
         }
      }
      delete patches[p];
   }
}

void NURBSExtension::Set3DSolutionVector(Vector &coords)
{
   Array<KnotVector *> kv(3);
   NURBSPatchMap p2g(this);

   weights.SetSize(GetNDof());
   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchDofMap(p, kv);
      NURBSPatch &Patch = *patches[p];

      for (int k = 0; k < kv[2]->GetNCP(); k++)
      {
         for (int j = 0; j < kv[1]->GetNCP(); j++)
         {
            for (int i = 0; i < kv[0]->GetNCP(); i++)
            {
               int l = p2g(i,j,k);
               for (int d = 0; d < 3; d++)
               {
                  coords(l*3 + d) = Patch(i,j,k,d)/Patch(i,j,k,3);
               }
               weights(l) = Patch(i,j,k,3);
            }
         }
      }
      delete patches[p];
   }
}


#ifdef MFEM_USE_MPI
ParNURBSExtension::ParNURBSExtension(MPI_Comm comm, NURBSExtension *parent,
                                     int *part, const Array<bool> &active_bel)
   : gtopo(comm)
{
   if (parent->NumOfActiveElems < parent->NumOfElements)
      // SetActive (BuildGroups?) and the way the weights are copied
      // do not support this case
      mfem_error("ParNURBSExtension::ParNURBSExtension :\n"
                 " all elements in the parent must be active!");

   patchTopo = parent->patchTopo;
   // steal ownership of patchTopo from the 'parent' NURBS extension
   if (!parent->own_topo)
      mfem_error("ParNURBSExtension::ParNURBSExtension :\n"
                 "  parent does not own the patch topology!");
   own_topo = 1;
   parent->own_topo = 0;

   parent->edge_to_knot.Copy(edge_to_knot);

   Order = parent->GetOrder();

   NumOfKnotVectors = parent->GetNKV();
   knotVectors.SetSize(NumOfKnotVectors);
   for (int i = 0; i < NumOfKnotVectors; i++)
   {
      knotVectors[i] = new KnotVector(*parent->GetKnotVector(i));
   }

   GenerateOffsets();
   CountElements();
   CountBdrElements();

   // copy 'part' to 'partitioning'
   partitioning = new int[GetGNE()];
   for (int i = 0; i < GetGNE(); i++)
      partitioning[i] = part[i];
   SetActive(partitioning, active_bel);

   GenerateActiveVertices();
   GenerateElementDofTable();
   // GenerateActiveBdrElems(); // done by SetActive for now
   GenerateBdrElementDofTable();

   Table *serial_elem_dof = parent->GetElementDofTable();
   BuildGroups(partitioning, *serial_elem_dof);

   weights.SetSize(GetNDof());
   // copy weights from parent
   for (int gel = 0, lel = 0; gel < GetGNE(); gel++)
   {
      if (activeElem[gel])
      {
         int  ndofs = el_dof->RowSize(lel);
         int *ldofs = el_dof->GetRow(lel);
         int *gdofs = serial_elem_dof->GetRow(gel);
         for (int i = 0; i < ndofs; i++)
            weights(ldofs[i]) = parent->weights(gdofs[i]);
         lel++;
      }
   }
}

ParNURBSExtension::ParNURBSExtension(NURBSExtension *parent,
                                     ParNURBSExtension *par_parent)
   : gtopo(par_parent->gtopo.GetComm())
{
   // steal all data from parent
   Order = parent->Order;

   patchTopo = parent->patchTopo;
   own_topo = parent->own_topo;
   parent->own_topo = 0;

   Swap(edge_to_knot, parent->edge_to_knot);

   NumOfKnotVectors = parent->NumOfKnotVectors;
   Swap(knotVectors, parent->knotVectors);

   NumOfVertices    = parent->NumOfVertices;
   NumOfElements    = parent->NumOfElements;
   NumOfBdrElements = parent->NumOfBdrElements;
   NumOfDofs        = parent->NumOfDofs;

   Swap(v_meshOffsets, parent->v_meshOffsets);
   Swap(e_meshOffsets, parent->e_meshOffsets);
   Swap(f_meshOffsets, parent->f_meshOffsets);
   Swap(p_meshOffsets, parent->p_meshOffsets);

   Swap(v_spaceOffsets, parent->v_spaceOffsets);
   Swap(e_spaceOffsets, parent->e_spaceOffsets);
   Swap(f_spaceOffsets, parent->f_spaceOffsets);
   Swap(p_spaceOffsets, parent->p_spaceOffsets);

   NumOfActiveVertices = parent->NumOfActiveVertices;
   NumOfActiveElems    = parent->NumOfActiveElems;
   NumOfActiveBdrElems = parent->NumOfActiveBdrElems;
   NumOfActiveDofs     = parent->NumOfActiveDofs;

   Swap(activeVert, parent->activeVert);
   Swap(activeElem, parent->activeElem);
   Swap(activeBdrElem, parent->activeBdrElem);
   Swap(activeDof, parent->activeDof);

   el_dof  = parent->el_dof;
   bel_dof = parent->bel_dof;
   parent->el_dof = parent->bel_dof = NULL;

   Swap(el_to_patch, parent->el_to_patch);
   Swap(bel_to_patch, parent->bel_to_patch);
   Swap(el_to_IJK, parent->el_to_IJK);
   Swap(bel_to_IJK, parent->bel_to_IJK);

   swap(&weights, &parent->weights);

   delete parent;

   partitioning = NULL;

#ifdef MFEM_DEBUG
   if (par_parent->partitioning == NULL)
      mfem_error("ParNURBSExtension::ParNURBSExtension :\n"
                 " parent ParNURBSExtension has no partitioning!");
#endif

   Table *serial_elem_dof = GetGlobalElementDofTable();
   BuildGroups(par_parent->partitioning, *serial_elem_dof);
   delete serial_elem_dof;
}

Table *ParNURBSExtension::GetGlobalElementDofTable()
{
   if (Dimension() == 2)
      return Get2DGlobalElementDofTable();
   else
      return Get3DGlobalElementDofTable();
}

Table *ParNURBSExtension::Get2DGlobalElementDofTable()
{
   int el = 0;
   KnotVector *kv[2];
   NURBSPatchMap p2g(this);

   Table *gel_dof = new Table(GetGNE(), (Order + 1)*(Order + 1));

   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchDofMap(p, kv);

      // Load dofs
      for (int j = 0; j < kv[1]->GetNKS(); j++)
      {
         if (kv[1]->isElement(j))
         {
            for (int i = 0; i < kv[0]->GetNKS(); i++)
            {
               if (kv[0]->isElement(i))
               {
                  int *dofs = gel_dof->GetRow(el);
                  int idx = 0;
                  for (int jj = 0; jj <= Order; jj++)
                  {
                     for (int ii = 0; ii <= Order; ii++)
                     {
                        dofs[idx] = p2g(i+ii,j+jj);
                        idx++;
                     }
                  }
                  el++;
               }
            }
         }
      }
   }
   return gel_dof;
}

Table *ParNURBSExtension::Get3DGlobalElementDofTable()
{
   int el = 0;
   KnotVector *kv[3];
   NURBSPatchMap p2g(this);

   Table *gel_dof = new Table(GetGNE(), (Order + 1)*(Order + 1)*(Order + 1));

   for (int p = 0; p < GetNP(); p++)
   {
      p2g.SetPatchDofMap(p, kv);

      // Load dofs
      for (int k = 0; k < kv[2]->GetNKS(); k++)
      {
         if (kv[2]->isElement(k))
         {
            for (int j = 0; j < kv[1]->GetNKS(); j++)
            {
               if (kv[1]->isElement(j))
               {
                  for (int i = 0; i < kv[0]->GetNKS(); i++)
                  {
                     if (kv[0]->isElement(i))
                     {
                        int *dofs = gel_dof->GetRow(el);
                        int idx = 0;
                        for (int kk = 0; kk <= Order; kk++)
                        {
                           for (int jj = 0; jj <= Order; jj++)
                           {
                              for (int ii = 0; ii <= Order; ii++)
                              {
                                 dofs[idx] = p2g(i+ii,j+jj,k+kk);
                                 idx++;
                              }
                           }
                        }
                        el++;
                     }
                  }
               }
            }
         }
      }
   }
   return gel_dof;
}

void ParNURBSExtension::SetActive(int *_partitioning,
                                  const Array<bool> &active_bel)
{
   activeElem.SetSize(GetGNE());
   activeElem = false;
   NumOfActiveElems = 0;
   int MyRank = gtopo.MyRank();
   for (int i = 0; i < GetGNE(); i++)
      if (_partitioning[i] == MyRank)
      {
         activeElem[i] = true;
         NumOfActiveElems++;
      }

   active_bel.Copy(activeBdrElem);
   NumOfActiveBdrElems = 0;
   for (int i = 0; i < GetGNBE(); i++)
      if (activeBdrElem[i])
         NumOfActiveBdrElems++;
}

void ParNURBSExtension::BuildGroups(int *_partitioning, const Table &elem_dof)
{
   Table dof_proc;

   ListOfIntegerSets  groups;
   IntegerSet         group;

   Transpose(elem_dof, dof_proc); // dof_proc is dof_elem
   // convert elements to processors
   for (int i = 0; i < dof_proc.Size_of_connections(); i++)
      dof_proc.GetJ()[i] = _partitioning[dof_proc.GetJ()[i]];

   // the first group is the local one
   int MyRank = gtopo.MyRank();
   group.Recreate(1, &MyRank);
   groups.Insert(group);

   int dof = 0;
   ldof_group.SetSize(GetNDof());
   for (int d = 0; d < GetNTotalDof(); d++)
      if (activeDof[d])
      {
         group.Recreate(dof_proc.RowSize(d), dof_proc.GetRow(d));
         ldof_group[dof] = groups.Insert(group);

         dof++;
      }

   gtopo.Create(groups, 1822);
}
#endif


void NURBSPatchMap::GetPatchKnotVectors(int p, KnotVector *kv[])
{
   Ext->patchTopo->GetElementVertices(p, verts);
   Ext->patchTopo->GetElementEdges(p, edges, oedge);
   if (Ext->Dimension() == 2)
   {
      kv[0] = Ext->KnotVec(edges[0]);
      kv[1] = Ext->KnotVec(edges[1]);
   }
   else
   {
      Ext->patchTopo->GetElementFaces(p, faces, oface);

      kv[0] = Ext->KnotVec(edges[0]);
      kv[1] = Ext->KnotVec(edges[3]);
      kv[2] = Ext->KnotVec(edges[8]);
   }
   opatch = 0;
}

void NURBSPatchMap::GetBdrPatchKnotVectors(int p, KnotVector *kv[], int *okv)
{
   Ext->patchTopo->GetBdrElementVertices(p, verts);
   Ext->patchTopo->GetBdrElementEdges(p, edges, oedge);
   kv[0] = Ext->KnotVec(edges[0], oedge[0], &okv[0]);
   if (Ext->Dimension() == 3)
   {
      faces.SetSize(1);
      Ext->patchTopo->GetBdrElementFace(p, &faces[0], &opatch);
      kv[1] = Ext->KnotVec(edges[1], oedge[1], &okv[1]);
   }
   else
      opatch = oedge[0];
}

void NURBSPatchMap::SetPatchVertexMap(int p, KnotVector *kv[])
{
   GetPatchKnotVectors(p, kv);

   I = kv[0]->GetNE() - 1;
   J = kv[1]->GetNE() - 1;

   for (int i = 0; i < verts.Size(); i++)
      verts[i] = Ext->v_meshOffsets[verts[i]];

   for (int i = 0; i < edges.Size(); i++)
      edges[i] = Ext->e_meshOffsets[edges[i]];

   if (Ext->Dimension() == 3)
   {
      K = kv[2]->GetNE() - 1;

      for (int i = 0; i < faces.Size(); i++)
         faces[i] = Ext->f_meshOffsets[faces[i]];
   }

   pOffset = Ext->p_meshOffsets[p];
}

void NURBSPatchMap::SetPatchDofMap(int p, KnotVector *kv[])
{
   GetPatchKnotVectors(p, kv);

   I = kv[0]->GetNCP() - 2;
   J = kv[1]->GetNCP() - 2;

   for (int i = 0; i < verts.Size(); i++)
      verts[i] = Ext->v_spaceOffsets[verts[i]];

   for (int i = 0; i < edges.Size(); i++)
      edges[i] = Ext->e_spaceOffsets[edges[i]];

   if (Ext->Dimension() == 3)
   {
      K = kv[2]->GetNCP() - 2;

      for (int i = 0; i < faces.Size(); i++)
         faces[i] = Ext->f_spaceOffsets[faces[i]];
   }

   pOffset = Ext->p_spaceOffsets[p];
}

void NURBSPatchMap::SetBdrPatchVertexMap(int p, KnotVector *kv[], int *okv)
{
   GetBdrPatchKnotVectors(p, kv, okv);

   I = kv[0]->GetNE() - 1;

   for (int i = 0; i < verts.Size(); i++)
      verts[i] = Ext->v_meshOffsets[verts[i]];

   if (Ext->Dimension() == 2)
   {
      pOffset = Ext->e_meshOffsets[edges[0]];
   }
   else
   {
      J = kv[1]->GetNE() - 1;

      for (int i = 0; i < edges.Size(); i++)
         edges[i] = Ext->e_meshOffsets[edges[i]];

      pOffset = Ext->f_meshOffsets[faces[0]];
   }
}

void NURBSPatchMap::SetBdrPatchDofMap(int p, KnotVector *kv[],  int *okv)
{
   GetBdrPatchKnotVectors(p, kv, okv);

   I = kv[0]->GetNCP() - 2;

   for (int i = 0; i < verts.Size(); i++)
      verts[i] = Ext->v_spaceOffsets[verts[i]];

   if (Ext->Dimension() == 2)
   {
      pOffset = Ext->e_spaceOffsets[edges[0]];
   }
   else
   {
      J = kv[1]->GetNCP() - 2;

      for (int i = 0; i < edges.Size(); i++)
         edges[i] = Ext->e_spaceOffsets[edges[i]];

      pOffset = Ext->f_spaceOffsets[faces[0]];
   }
}

}
