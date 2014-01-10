//===-- CodeLoader.cpp ------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-private.h"
#include "lldb/Target/CodeLoader.h"
#include "lldb/Target/Process.h"

using namespace lldb;
using namespace lldb_private;

//----------------------------------------------------------------------
// CodeLoader constructor
//----------------------------------------------------------------------
CodeLoader::CodeLoader(Process *process) :
    m_process (process)
{
}

//----------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------
CodeLoader::~CodeLoader()
{
}

//----------------------------------------------------------------------
// Accessosors to the global setting as to whether to stop at image
// (shared library) loading/unloading.
//----------------------------------------------------------------------
bool
CodeLoader::GetStopWhenImagesChange () const
{
    return m_process->GetStopOnSharedLibraryEvents();
}

void
CodeLoader::SetStopWhenImagesChange (bool stop)
{
    m_process->SetStopOnSharedLibraryEvents (stop);
}
