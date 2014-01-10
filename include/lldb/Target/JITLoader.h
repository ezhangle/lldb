//===-- JITLoader.h ---------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_JITLoader_h_
#define liblldb_JITLoader_h_

#include <vector>

#include "lldb/Target/CodeLoader.h"

namespace lldb_private {

//----------------------------------------------------------------------
/// @class JITLoader JITLoader.h "lldb/Target/JITLoader.h"
/// @brief A plug-in interface definition class for JIT loaders.
///
/// Plugins of this kind listen for code generated at runtime in the 
/// target. They are very similar to dynamic loader, with the difference
/// that they do not have information about the the target's dyld and
/// that there may be multiple JITLoader plugins per process, while 
/// there is at most one DynamicLoader. 
//----------------------------------------------------------------------
class JITLoader :
    public CodeLoader
{
public:
    //------------------------------------------------------------------
    /// Find a JIT loader plugin for a given process.
    ///
    /// Scans the installed DynamicLoader plug-ins and tries to find
    /// all applicable instances for the current process.
    ///
    /// @param[in] process
    ///     The process for which to try and locate a JIT loader
    ///     plug-in instance.
    ///
    //------------------------------------------------------------------
    static void 
    LoadPlugins (Process *process, std::vector<std::unique_ptr<JITLoader>> &list);

    //------------------------------------------------------------------
    /// Construct with a process.
    //------------------------------------------------------------------
    JITLoader (Process *process);


    virtual 
    ~JITLoader ();

};

} // namespace lldb_private

#endif // liblldb_JITLoader_h_