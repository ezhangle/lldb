//===-- CodeLoader.h --------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_CodeLoader_h_
#define liblldb_CodeLoader_h_

// Project includes
#include "lldb/lldb-private.h"
#include "lldb/Core/PluginInterface.h"

namespace lldb_private {

//----------------------------------------------------------------------
/// @class DynamicLoader DynamicLoader.h "lldb/Target/DynamicLoader.h"
/// @brief A common super class for JIT and Shared Library loadres
///
//----------------------------------------------------------------------
class CodeLoader :
    public PluginInterface
{
public:

    //------------------------------------------------------------------
    /// Construct with a process.
    //------------------------------------------------------------------
    CodeLoader (Process *process);


    virtual 
    ~CodeLoader ();


    //------------------------------------------------------------------
    /// Called after attaching a process.
    ///
    /// Allow DynamicLoader plug-ins to execute some code after
    /// attaching to a process.
    //------------------------------------------------------------------
    virtual void
    DidAttach () = 0;

    //------------------------------------------------------------------
    /// Called after launching a process.
    ///
    /// Allow DynamicLoader plug-ins to execute some code after
    /// the process has stopped for the first time on launch.
    //------------------------------------------------------------------
    virtual void
    DidLaunch () = 0;

    //------------------------------------------------------------------
    /// Get whether the process should stop when images change.
    ///
    /// When images (executables and shared libraries) get loaded or
    /// unloaded, often debug sessions will want to try and resolve or
    /// unresolve breakpoints that are set in these images. Any
    /// breakpoints set by DynamicLoader plug-in instances should
    /// return this value to ensure consistent debug session behaviour.
    ///
    /// @return
    ///     Returns \b true if the process should stop when images
    ///     change, \b false if the process should resume.
    //------------------------------------------------------------------
    bool
    GetStopWhenImagesChange () const;

    //------------------------------------------------------------------
    /// Set whether the process should stop when images change.
    ///
    /// When images (executables and shared libraries) get loaded or
    /// unloaded, often debug sessions will want to try and resolve or
    /// unresolve breakpoints that are set in these images. The default
    /// is set so that the process stops when images change, but this
    /// can be overridden using this function callback.
    ///
    /// @param[in] stop
    ///     Boolean value that indicates whether the process should stop
    ///     when images change.
    //------------------------------------------------------------------
    void
    SetStopWhenImagesChange (bool stop);

protected:
    //------------------------------------------------------------------
    // Member variables.
    //------------------------------------------------------------------
    Process* m_process; ///< The process that this dynamic loader plug-in is tracking.
};

} // namespace lldb_private

#endif   // liblldb_CodeLoader_h_