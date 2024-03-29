//===-- lldb-python.h --------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_lldb_python_h_
#define LLDB_lldb_python_h_

// Python.h needs to be included before any system headers in order to avoid redefinition of macros

#ifdef LLDB_DISABLE_PYTHON

// Python is disabled in this build

#else

#if defined(__linux__)
// features.h will define _POSIX_C_SOURCE if _GNU_SOURCE is defined.  This value
// may be different from the value that Python defines it to be which results
// in a warning.  Undefine _POSIX_C_SOURCE before including Python.h  The same
// holds for _XOPEN_SOURCE.
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#endif

#include <Python.h>

#endif // LLDB_DISABLE_PYTHON

#endif  // LLDB_lldb_python_h_
