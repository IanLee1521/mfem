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

#ifndef MFEM_ERROR
#define MFEM_ERROR

#include "../config/config.hpp"
#include <iomanip>
#include <sstream>

namespace mfem
{

void mfem_error(const char *msg = NULL);

void mfem_warning(const char *msg = NULL);

}

#ifndef _MFEM_FUNC_NAME
#ifndef _WIN32
// This is nice because it shows the class and method name
#define _MFEM_FUNC_NAME __PRETTY_FUNCTION__
// This one is C99 standard.
//#define _MFEM_FUNC_NAME __func__
#else
// for Visual Studio C++
#define _MFEM_FUNC_NAME __FUNCSIG__
#endif
#endif

// Common error message and abort macro
#define _MFEM_MESSAGE(msg, warn)                                        \
   {                                                                    \
      std::ostringstream s;                                             \
      s << std::setprecision(16);                                       \
      s << std::setiosflags(std::ios_base::scientific);                 \
      s << msg << '\n';                                                 \
      s << " ... at line " << __LINE__;                                 \
      s << " in " << _MFEM_FUNC_NAME << " of file " << __FILE__ << "."; \
      s << std::ends;                                                   \
      if (!(warn))                                                      \
         mfem::mfem_error(s.str().c_str());                             \
      else                                                              \
         mfem::mfem_warning(s.str().c_str());                           \
   }

// Outputs lots of useful information and aborts.
// For all of these functions, "msg" is pushed to an ostream, so you can
// write useful (if complicated) error messages instead of writing
// out to the screen first, then calling abort.  For example:
// MFEM_ABORT( "Unknown geometry type: " << type );
#define MFEM_ABORT(msg) _MFEM_MESSAGE("MFEM abort: " << msg, 0)

// Does a check, and then outputs lots of useful information if the test fails
#define MFEM_VERIFY(x, msg)                             \
   if (!(x))                                            \
   {                                                    \
      _MFEM_MESSAGE("Verification failed: ("            \
                    << #x << ") is false: " << msg, 0); \
   }

// Use this if the only place your variable is used is in ASSERTs
// For example, this code snippet:
//   int err = MPI_Reduce(ldata, maxdata, 5, MPI_INT, MPI_MAX, 0, MyComm);
//   MFEM_CONTRACT_VAR(err);
//   MFEM_ASSERT( err == 0, "MPI_Reduce gave an error with length "
//                       << ldata );
#define MFEM_CONTRACT_VAR(x) if (0 && &x == &x){}

// Now set up some optional checks, but only if the right flags are on
#ifdef MFEM_DEBUG

#define MFEM_ASSERT(x, msg)                             \
   if (!(x))                                            \
   {                                                    \
      _MFEM_MESSAGE("Assertion failed: ("               \
                    << #x << ") is false: " << msg, 0); \
   }

#else

// Get rid of all this code, since we're not checking.
#define MFEM_ASSERT(x, msg)

#endif

// Generate a warning message - always generated, regardless of MFEM_DEBUG.
#define MFEM_WARNING(msg) _MFEM_MESSAGE("MFEM Warning: " << msg, 1)

#endif
