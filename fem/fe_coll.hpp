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

#ifndef MFEM_FE_COLLECTION
#define MFEM_FE_COLLECTION

#include "../config/config.hpp"
#include "geom.hpp"
#include "fe.hpp"

namespace mfem
{

/** Collection of finite elements from the same family in multiple dimensions.
    This class is used to match the degrees of freedom of a FiniteElementSpace
    between elements, and to provide the finite element restriction from an
    element to its boundary. */
class FiniteElementCollection
{
public:
   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const = 0;

   virtual int DofForGeometry(int GeomType) const = 0;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const = 0;

   virtual const char * Name() const { return "Undefined"; }

   int HasFaceDofs(int GeomType) const;

   virtual const FiniteElement *TraceFiniteElementForGeometry(
      int GeomType) const
   {
      return FiniteElementForGeometry(GeomType);
   }

   virtual ~FiniteElementCollection() { }

   static FiniteElementCollection *New(const char *name);
};

/// Arbitrary order H1-conforming (continuous) finite elements.
class H1_FECollection : public FiniteElementCollection
{
private:
   char h1_name[32];
   FiniteElement *H1_Elements[Geometry::NumGeom];
   int H1_dof[Geometry::NumGeom];
   int *SegDofOrd[2], *TriDofOrd[6], *QuadDofOrd[8];

public:
   explicit H1_FECollection(const int p, const int dim = 3, const int type = 0);

   virtual const FiniteElement *FiniteElementForGeometry(int GeomType) const
   { return H1_Elements[GeomType]; }
   virtual int DofForGeometry(int GeomType) const
   { return H1_dof[GeomType]; }
   virtual int *DofOrderForOrientation(int GeomType, int Or) const;
   virtual const char *Name() const { return h1_name; }

   virtual ~H1_FECollection();
};

/** Arbitrary order H1-conforming (continuous) finite elements with positive
    basis functions. */
class H1Pos_FECollection : public H1_FECollection
{
public:
   explicit H1Pos_FECollection(const int p, const int dim = 3)
      : H1_FECollection(p, dim, 1) { }
};

/// Arbitrary order "L2-conforming" discontinuous finite elements.
class L2_FECollection : public FiniteElementCollection
{
private:
   char d_name[32];
   FiniteElement *L2_Elements[Geometry::NumGeom];
   FiniteElement *Tr_Elements[Geometry::NumGeom];
   int *SegDofOrd[2]; // for rotating segment dofs in 1D
   int *TriDofOrd[6]; // for rotating triangle dofs in 2D

public:
   L2_FECollection(const int p, const int dim, const int type = 0);

   virtual const FiniteElement *FiniteElementForGeometry(int GeomType) const
   { return L2_Elements[GeomType]; }
   virtual int DofForGeometry(int GeomType) const
   {
      if (L2_Elements[GeomType])
         return L2_Elements[GeomType]->GetDof();
      return 0;
   }
   virtual int *DofOrderForOrientation(int GeomType, int Or) const;
   virtual const char *Name() const { return d_name; }

   virtual const FiniteElement *TraceFiniteElementForGeometry(
      int GeomType) const
   {
      return Tr_Elements[GeomType];
   }

   virtual ~L2_FECollection();
};

// Declare an alternative name for L2_FECollection = DG_FECollection
typedef L2_FECollection DG_FECollection;

/// Arbitrary order H(div)-conforming Raviart-Thomas finite elements.
class RT_FECollection : public FiniteElementCollection
{
protected:
   char rt_name[32];
   FiniteElement *RT_Elements[Geometry::NumGeom];
   int RT_dof[Geometry::NumGeom];
   int *SegDofOrd[2], *TriDofOrd[6], *QuadDofOrd[8];

   // Initialize only the face elements
   void InitFaces(const int p, const int dim, const int map_type);

   // Constructor used by the constructor of RT_Trace_FECollection
   RT_FECollection(const int p, const int dim, const int map_type)
   { InitFaces(p, dim, map_type); }

public:
   RT_FECollection(const int p, const int dim);

   virtual const FiniteElement *FiniteElementForGeometry(int GeomType) const
   { return RT_Elements[GeomType]; }
   virtual int DofForGeometry(int GeomType) const
   { return RT_dof[GeomType]; }
   virtual int *DofOrderForOrientation(int GeomType, int Or) const;
   virtual const char *Name() const { return rt_name; }

   virtual ~RT_FECollection();
};

/** Arbitrary order "H^{-1/2}-conforming" face finite elements defined on the
    interface between mesh elements (faces); these are the normal trace FEs of
    the H(div)-conforming FEs. */
class RT_Trace_FECollection : public RT_FECollection
{
public:
   RT_Trace_FECollection(const int p, const int dim,
                         const int map_type = FiniteElement::INTEGRAL);
};

/// Arbitrary order H(curl)-conforming Nedelec finite elements.
class ND_FECollection : public FiniteElementCollection
{
private:
   char nd_name[32];
   FiniteElement *ND_Elements[Geometry::NumGeom];
   int ND_dof[Geometry::NumGeom];
   int *SegDofOrd[2], *TriDofOrd[6], *QuadDofOrd[8];

public:
   ND_FECollection(const int p, const int dim);

   virtual const FiniteElement *FiniteElementForGeometry(int GeomType) const
   { return ND_Elements[GeomType]; }
   virtual int DofForGeometry(int GeomType) const
   { return ND_dof[GeomType]; }
   virtual int *DofOrderForOrientation(int GeomType, int Or) const;
   virtual const char *Name() const { return nd_name; }

   virtual ~ND_FECollection();
};

/// Arbitrary order non-uniform rational B-splines (NURBS) finite elements.
class NURBSFECollection : public FiniteElementCollection
{
private:
   NURBS1DFiniteElement *SegmentFE;
   NURBS2DFiniteElement *QuadrilateralFE;
   NURBS3DFiniteElement *ParallelepipedFE;

   char name[16];

   void Allocate(int Order);
   void Deallocate();

public:
   explicit NURBSFECollection(int Order) { Allocate(Order); }

   int GetOrder() const { return SegmentFE->GetOrder(); }

   /// Change the order of the collection
   void UpdateOrder(int Order) { Deallocate(); Allocate(Order); }

   void Reset() const
   {
      SegmentFE->Reset();
      QuadrilateralFE->Reset();
      ParallelepipedFE->Reset();
   }

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int *DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char *Name() const { return name; }

   virtual ~NURBSFECollection() { Deallocate(); }
};


/// Piecewise-(bi)linear continuous finite elements.
class LinearFECollection : public FiniteElementCollection
{
private:
   const PointFiniteElement PointFE;
   const Linear1DFiniteElement SegmentFE;
   const Linear2DFiniteElement TriangleFE;
   const BiLinear2DFiniteElement QuadrilateralFE;
   const Linear3DFiniteElement TetrahedronFE;
   const TriLinear3DFiniteElement ParallelepipedFE;
public:
   LinearFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "Linear"; };
};

/// Piecewise-(bi)quadratic continuous finite elements.
class QuadraticFECollection : public FiniteElementCollection
{
private:
   const PointFiniteElement PointFE;
   const Quad1DFiniteElement SegmentFE;
   const Quad2DFiniteElement TriangleFE;
   const BiQuad2DFiniteElement QuadrilateralFE;
   const Quadratic3DFiniteElement TetrahedronFE;
   const LagrangeHexFiniteElement ParallelepipedFE;

public:
   QuadraticFECollection() : ParallelepipedFE(2) { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "Quadratic"; };
};

/// Version of QuadraticFECollection with positive basis functions.
class QuadraticPosFECollection : public FiniteElementCollection
{
private:
   const QuadPos1DFiniteElement   SegmentFE;
   const BiQuadPos2DFiniteElement QuadrilateralFE;

public:
   QuadraticPosFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "QuadraticPos"; };
};

/// Piecewise-(bi)cubic continuous finite elements.
class CubicFECollection : public FiniteElementCollection
{
private:
   const PointFiniteElement PointFE;
   const Cubic1DFiniteElement SegmentFE;
   const Cubic2DFiniteElement TriangleFE;
   const BiCubic2DFiniteElement QuadrilateralFE;
   const Cubic3DFiniteElement TetrahedronFE;
   const LagrangeHexFiniteElement ParallelepipedFE;

public:
   CubicFECollection() : ParallelepipedFE(3) { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "Cubic"; };
};

/// Crouzeix-Raviart nonconforming elements in 2D.
class CrouzeixRaviartFECollection : public FiniteElementCollection
{
private:
   const P0SegmentFiniteElement SegmentFE;
   const CrouzeixRaviartFiniteElement TriangleFE;
   const CrouzeixRaviartQuadFiniteElement QuadrilateralFE;
public:
   CrouzeixRaviartFECollection() : SegmentFE(1) { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "CrouzeixRaviart"; };
};

/// Piecewise-linear nonconforming finite elements in 3D.
class LinearNonConf3DFECollection : public FiniteElementCollection
{
private:
   const P0TriangleFiniteElement TriangleFE;
   const P1TetNonConfFiniteElement TetrahedronFE;
   const P0QuadFiniteElement QuadrilateralFE;
   const RotTriLinearHexFiniteElement ParallelepipedFE;

public:
   LinearNonConf3DFECollection () { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "LinearNonConf3D"; };
};


/** First order Raviart-Thomas finite elements in 2D. This class is kept only
    for backward compatibility, consider using RT_FECollection instead. */
class RT0_2DFECollection : public FiniteElementCollection
{
private:
   const P0SegmentFiniteElement SegmentFE; // normal component on edge
   const RT0TriangleFiniteElement TriangleFE;
   const RT0QuadFiniteElement QuadrilateralFE;
public:
   RT0_2DFECollection() : SegmentFE(0) { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "RT0_2D"; };
};

/** Second order Raviart-Thomas finite elements in 2D. This class is kept only
    for backward compatibility, consider using RT_FECollection instead. */
class RT1_2DFECollection : public FiniteElementCollection
{
private:
   const P1SegmentFiniteElement SegmentFE; // normal component on edge
   const RT1TriangleFiniteElement TriangleFE;
   const RT1QuadFiniteElement QuadrilateralFE;
public:
   RT1_2DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "RT1_2D"; };
};

/** Third order Raviart-Thomas finite elements in 2D. This class is kept only
    for backward compatibility, consider using RT_FECollection instead. */
class RT2_2DFECollection : public FiniteElementCollection
{
private:
   const P2SegmentFiniteElement SegmentFE; // normal component on edge
   const RT2TriangleFiniteElement TriangleFE;
   const RT2QuadFiniteElement QuadrilateralFE;
public:
   RT2_2DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "RT2_2D"; };
};

/** Piecewise-constant discontinuous finite elements in 2D. This class is kept
    only for backward compatibility, consider using L2_FECollection instead. */
class Const2DFECollection : public FiniteElementCollection
{
private:
   const P0TriangleFiniteElement TriangleFE;
   const P0QuadFiniteElement QuadrilateralFE;
public:
   Const2DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "Const2D"; };
};

/** Piecewise-linear discontinuous finite elements in 2D. This class is kept
    only for backward compatibility, consider using L2_FECollection instead. */
class LinearDiscont2DFECollection : public FiniteElementCollection
{
private:
   const Linear2DFiniteElement TriangleFE;
   const BiLinear2DFiniteElement QuadrilateralFE;

public:
   LinearDiscont2DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "LinearDiscont2D"; };
};

/// Version of LinearDiscont2DFECollection with dofs in the Gaussian points.
class GaussLinearDiscont2DFECollection : public FiniteElementCollection
{
private:
   // const CrouzeixRaviartFiniteElement TriangleFE;
   const GaussLinear2DFiniteElement TriangleFE;
   const GaussBiLinear2DFiniteElement QuadrilateralFE;

public:
   GaussLinearDiscont2DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "GaussLinearDiscont2D"; };
};

/// Linear (P1) finite elements on quadrilaterals.
class P1OnQuadFECollection : public FiniteElementCollection
{
private:
   const P1OnQuadFiniteElement QuadrilateralFE;
public:
   P1OnQuadFECollection() { };
   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;
   virtual int DofForGeometry(int GeomType) const;
   virtual int * DofOrderForOrientation(int GeomType, int Or) const;
   virtual const char * Name() const { return "P1OnQuad"; };
};

/** Piecewise-quadratic discontinuous finite elements in 2D. This class is kept
    only for backward compatibility, consider using L2_FECollection instead. */
class QuadraticDiscont2DFECollection : public FiniteElementCollection
{
private:
   const Quad2DFiniteElement TriangleFE;
   const BiQuad2DFiniteElement QuadrilateralFE;

public:
   QuadraticDiscont2DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "QuadraticDiscont2D"; };
};

/// Version of QuadraticDiscont2DFECollection with positive basis functions.
class QuadraticPosDiscont2DFECollection : public FiniteElementCollection
{
private:
   const BiQuadPos2DFiniteElement QuadrilateralFE;

public:
   QuadraticPosDiscont2DFECollection() { };
   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;
   virtual int DofForGeometry(int GeomType) const;
   virtual int * DofOrderForOrientation(int GeomType, int Or) const
   { return NULL; };
   virtual const char * Name() const { return "QuadraticPosDiscont2D"; };
};

/// Version of QuadraticDiscont2DFECollection with dofs in the Gaussian points.
class GaussQuadraticDiscont2DFECollection : public FiniteElementCollection
{
private:
   // const Quad2DFiniteElement TriangleFE;
   const GaussQuad2DFiniteElement TriangleFE;
   const GaussBiQuad2DFiniteElement QuadrilateralFE;

public:
   GaussQuadraticDiscont2DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "GaussQuadraticDiscont2D"; };
};

/** Piecewise-cubic discontinuous finite elements in 2D. This class is kept
    only for backward compatibility, consider using L2_FECollection instead. */
class CubicDiscont2DFECollection : public FiniteElementCollection
{
private:
   const Cubic2DFiniteElement TriangleFE;
   const BiCubic2DFiniteElement QuadrilateralFE;

public:
   CubicDiscont2DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "CubicDiscont2D"; };
};

/** Piecewise-constant discontinuous finite elements in 3D. This class is kept
    only for backward compatibility, consider using L2_FECollection instead. */
class Const3DFECollection : public FiniteElementCollection
{
private:
   const P0TetFiniteElement TetrahedronFE;
   const P0HexFiniteElement ParallelepipedFE;

public:
   Const3DFECollection () { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "Const3D"; };
};

/** Piecewise-linear discontinuous finite elements in 3D. This class is kept
    only for backward compatibility, consider using L2_FECollection instead. */
class LinearDiscont3DFECollection : public FiniteElementCollection
{
private:
   const Linear3DFiniteElement TetrahedronFE;
   const TriLinear3DFiniteElement ParallelepipedFE;

public:
   LinearDiscont3DFECollection () { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "LinearDiscont3D"; };
};

/** Piecewise-quadratic discontinuous finite elements in 3D. This class is kept
    only for backward compatibility, consider using L2_FECollection instead. */
class QuadraticDiscont3DFECollection : public FiniteElementCollection
{
private:
   const Quadratic3DFiniteElement TetrahedronFE;
   const LagrangeHexFiniteElement ParallelepipedFE;

public:
   QuadraticDiscont3DFECollection () : ParallelepipedFE(2) { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "QuadraticDiscont3D"; };
};

/// Finite element collection on a macro-element.
class RefinedLinearFECollection : public FiniteElementCollection
{
private:
   const PointFiniteElement PointFE;
   const RefinedLinear1DFiniteElement SegmentFE;
   const RefinedLinear2DFiniteElement TriangleFE;
   const RefinedBiLinear2DFiniteElement QuadrilateralFE;
   const RefinedLinear3DFiniteElement TetrahedronFE;
   const RefinedTriLinear3DFiniteElement ParallelepipedFE;

public:
   RefinedLinearFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "RefinedLinear"; };
};

/** Lowest order Nedelec finite elements in 3D. This class is kept only for
    backward compatibility, consider using the new ND_FECollection instead. */
class ND1_3DFECollection : public FiniteElementCollection
{
private:
   const Nedelec1HexFiniteElement HexahedronFE;
   const Nedelec1TetFiniteElement TetrahedronFE;

public:
   ND1_3DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "ND1_3D"; };
};

/** First order Raviart-Thomas finite elements in 3D. This class is kept only
    for backward compatibility, consider using RT_FECollection instead. */
class RT0_3DFECollection : public FiniteElementCollection
{
private:
   const P0TriangleFiniteElement TriangleFE;
   const P0QuadFiniteElement QuadrilateralFE;
   const RT0HexFiniteElement HexahedronFE;
   const RT0TetFiniteElement TetrahedronFE;
public:
   RT0_3DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "RT0_3D"; };
};

/** Second order Raviart-Thomas finite elements in 3D. This class is kept only
    for backward compatibility, consider using RT_FECollection instead. */
class RT1_3DFECollection : public FiniteElementCollection
{
private:
   const Linear2DFiniteElement TriangleFE;
   const BiLinear2DFiniteElement QuadrilateralFE;
   const RT1HexFiniteElement HexahedronFE;
public:
   RT1_3DFECollection() { };

   virtual const FiniteElement *
   FiniteElementForGeometry(int GeomType) const;

   virtual int DofForGeometry(int GeomType) const;

   virtual int * DofOrderForOrientation(int GeomType, int Or) const;

   virtual const char * Name() const { return "RT1_3D"; };
};

/// Discontinuous collection defined locally by a given finite element.
class Local_FECollection : public FiniteElementCollection
{
private:
   char d_name[32];
   int GeomType;
   FiniteElement *Local_Element;

public:
   Local_FECollection(const char *fe_name);

   virtual const FiniteElement *FiniteElementForGeometry(int _GeomType) const
   { return (GeomType == _GeomType) ? Local_Element : NULL; }
   virtual int DofForGeometry(int _GeomType) const
   { return (GeomType == _GeomType) ? Local_Element->GetDof() : 0; }
   virtual int *DofOrderForOrientation(int GeomType, int Or) const
   { return NULL; }
   virtual const char *Name() const { return d_name; }

   virtual ~Local_FECollection() { delete Local_Element; }
};

}

#endif
