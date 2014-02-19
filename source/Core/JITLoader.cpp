//===-- JITLoader.cpp -------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-private.h"
#include "lldb/Target/JITLoader.h"
#include "lldb/Target/Process.h"
#include "lldb/Core/PluginManager.h"

using namespace lldb;
using namespace lldb_private;

void
JITLoader::LoadPlugins (Process *process, std::vector<std::unique_ptr<JITLoader>> &list)
{
    JITLoaderCreateInstance create_callback = NULL;
    for (uint32_t idx = 0; (create_callback = PluginManager::GetJITLoaderCreateCallbackAtIndex(idx)) != NULL; ++idx)
    {
        std::unique_ptr<JITLoader> instance_ap(create_callback(process, false));
        if (instance_ap)
            list.push_back(std::move(instance_ap));
    }
}

JITLoader::JITLoader(Process *process) :
    m_process (process)
{
}

JITLoader::~JITLoader()
{
}
