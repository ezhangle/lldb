//===-- ProcessWindows.cpp ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-python.h"
#include "lldb/Host/Config.h"

// C Includes
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

// C++ Includes
#include <algorithm>
#include <map>

// Other libraries and framework includes

#include "lldb/Breakpoint/Watchpoint.h"
#include "lldb/Interpreter/Args.h"
#include "lldb/Core/ArchSpec.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/ConnectionFileDescriptor.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/ModuleSpec.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/State.h"
#include "lldb/Core/StreamFile.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Core/Timer.h"
#include "lldb/Core/Value.h"
#include "lldb/Host/FileSpec.h"
#include "lldb/Host/Symbols.h"
#include "lldb/Host/TimeValue.h"
#include "lldb/Interpreter/CommandInterpreter.h"
#include "lldb/Interpreter/CommandObject.h"
#include "lldb/Interpreter/CommandObjectMultiword.h"
#include "lldb/Interpreter/CommandReturnObject.h"
#ifndef LLDB_DISABLE_PYTHON
#include "lldb/Interpreter/PythonDataObjects.h"
#endif
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Target/DynamicLoader.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/TargetList.h"
#include "lldb/Target/ThreadPlanCallFunction.h"
#include "lldb/Utility/PseudoTerminal.h"

// Project includes
#include "lldb/Host/Host.h"
#include "ProcessWindows.h"
#include "ProcessWindowsLog.h"

#include <process.h>

using namespace lldb;
using namespace lldb_private;

lldb_private::ConstString
ProcessWindows::GetPluginNameStatic()
{
    static ConstString g_name("windows");
    return g_name;
}

const char *
ProcessWindows::GetPluginDescriptionStatic()
{
    return "Process plugin for Windows";
}

void
ProcessWindows::Terminate()
{
    PluginManager::UnregisterPlugin (ProcessWindows::CreateInstance);
}


lldb::ProcessSP
ProcessWindows::CreateInstance (Target &target, Listener &listener, const FileSpec *crash_file_path)
{
    assert(crash_file_path == nullptr);

    lldb::ProcessSP process_sp;
    process_sp.reset (new ProcessWindows (target, listener));
    return process_sp;
}

bool
ProcessWindows::CanDebug (Target &target, bool plugin_specified_by_name)
{
    if (plugin_specified_by_name)
        return true;

    // For now we are just making sure the file exists for a given module
    Module *exe_module = target.GetExecutableModulePointer();
    if (exe_module)
    {
        ObjectFile *exe_objfile = exe_module->GetObjectFile();
        // We can't debug core files...
        switch (exe_objfile->GetType())
        {
            case ObjectFile::eTypeInvalid:
            case ObjectFile::eTypeCoreFile:
            case ObjectFile::eTypeDebugInfo:
            case ObjectFile::eTypeObjectFile:
            case ObjectFile::eTypeSharedLibrary:
            case ObjectFile::eTypeStubLibrary:
                return false;
            case ObjectFile::eTypeExecutable:
            case ObjectFile::eTypeDynamicLinker:
            case ObjectFile::eTypeUnknown:
                break;
        }
        return exe_module->GetFileSpec().Exists();
    }
    // However, if there is no executable module, we return true since we might be preparing to attach.
    return true;
}

//----------------------------------------------------------------------
// ProcessWindows constructor
//----------------------------------------------------------------------
ProcessWindows::ProcessWindows(Target& target, Listener &listener) :
    Process (target, listener)
{
    printf("CREATING A WINDOWS PROCESS PLUGIN\n");
}

//----------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------
ProcessWindows::~ProcessWindows()
{
}

//----------------------------------------------------------------------
// PluginInterface
//----------------------------------------------------------------------
ConstString
ProcessWindows::GetPluginName()
{
    return GetPluginNameStatic();
}

uint32_t
ProcessWindows::GetPluginVersion()
{
    return 1;
}

Error
ProcessWindows::WillLaunch (Module* module)
{
    return WillLaunchOrAttach ();
}

Error
ProcessWindows::WillAttachToProcessWithID (lldb::pid_t pid)
{
    return WillLaunchOrAttach ();
}

Error
ProcessWindows::WillAttachToProcessWithName (const char *process_name, bool wait_for_launch)
{
    return WillLaunchOrAttach ();
}

Error
ProcessWindows::WillLaunchOrAttach ()
{
    Error error;
    assert(false && "not implemented");
    return error;
}

//----------------------------------------------------------------------
// Process Control
//----------------------------------------------------------------------
Error
ProcessWindows::DoLaunch (Module *exe_module, ProcessLaunchInfo &launch_info)
{
    Error error;

    printf("LAUNCHING A WINDOWS PROCESS\n");

    SetPrivateState(eStateLaunching);

    uint32_t launch_flags = launch_info.GetFlags().Get();
    const char *stdin_path = NULL;
    const char *stdout_path = NULL;
    const char *stderr_path = NULL;
    const char *working_dir = launch_info.GetWorkingDirectory();

    const ProcessLaunchInfo::FileAction *file_action;
    file_action = launch_info.GetFileActionForFD (STDIN_FILENO);
    if (file_action)
    {
        if (file_action->GetAction () == ProcessLaunchInfo::FileAction::eFileActionOpen)
            stdin_path = file_action->GetPath();
    }
    file_action = launch_info.GetFileActionForFD (STDOUT_FILENO);
    if (file_action)
    {
        if (file_action->GetAction () == ProcessLaunchInfo::FileAction::eFileActionOpen)
            stdout_path = file_action->GetPath();
    }
    file_action = launch_info.GetFileActionForFD (STDERR_FILENO);
    if (file_action)
    {
        if (file_action->GetAction () == ProcessLaunchInfo::FileAction::eFileActionOpen)
            stderr_path = file_action->GetPath();
    }

    char **argv = launch_info.GetArguments().GetArgumentVector();
    char **envp = launch_info.GetEnvironmentEntries().GetArgumentVector();

    Log *log (ProcessWindowsLog::GetLogIfAllCategoriesSet (WINDOWS_LOG_PROCESS));

    // Propagate the environment if one is not supplied.
    if (envp == NULL || envp[0] == NULL)
        envp = const_cast<char **>(environ);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(
        NULL,           // No module name (use command line)
        "c:\\Windows\\System32\\ipconfig.exe",      // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        DEBUG_ONLY_THIS_PROCESS,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi)            // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("CreateProcess failed (%d)\n", GetLastError());
        exit(1);
    }

    SetID(pi.dwProcessId);
    m_module = exe_module;

    return error;
}

void
ProcessWindows::DidLaunchOrAttach ()
{
}

void
ProcessWindows::DidLaunch ()
{
    DidLaunchOrAttach ();
}

Error
ProcessWindows::DoAttachToProcessWithID (lldb::pid_t attach_pid)
{
    ProcessAttachInfo attach_info;
    return DoAttachToProcessWithID(attach_pid, attach_info);
}

Error
ProcessWindows::DoAttachToProcessWithID (lldb::pid_t attach_pid, const ProcessAttachInfo &attach_info)
{
    Error error;
    assert(false && "not implemented");
    return error;
}

void
ProcessWindows::DidAttach ()
{
    DidLaunchOrAttach ();
}

Error
ProcessWindows::WillResume ()
{
    return Error();
}

Error
ProcessWindows::DoResume ()
{
    Error error;
    return error;
}

bool
ProcessWindows::UpdateThreadList (ThreadList &old_thread_list, ThreadList &new_thread_list)
{
    assert(false && "not implemented");
    return true;
}

void
ProcessWindows::RefreshStateAfterStop ()
{
}

Error
ProcessWindows::DoHalt (bool &caused_stop)
{
    Error error;
    return error;
}

Error
ProcessWindows::DoDetach(bool keep_stopped)
{
    Error error;
    assert(false && "not implemented");
    return error;
}

Error
ProcessWindows::DoDestroy ()
{
    Error error;
    assert(false && "not implemented");
    return error;
}


//------------------------------------------------------------------
// Process Queries
//------------------------------------------------------------------

bool
ProcessWindows::IsAlive ()
{
    assert(false && "not implemented");
    return false;
}

addr_t
ProcessWindows::GetImageInfoAddress()
{
    assert(false && "not implemented");
    return LLDB_INVALID_ADDRESS;
}

//------------------------------------------------------------------
// Process Memory
//------------------------------------------------------------------
size_t
ProcessWindows::DoReadMemory (addr_t addr, void *buf, size_t size, Error &error)
{
    assert(m_inferior_handle);

    Log *log(ProcessWindowsLog::GetLogIfAllCategoriesSet(WINDOWS_LOG_ALL));
    if (log && log->GetMask().Test(WINDOWS_LOG_MEMORY))
        log->Printf("ProcessWindows::%s(%" PRIu64 ", %p, %p, %zd, _)",
                    __FUNCTION__, pid, (void *)addr, buf, size);

    SIZE_T bytes_read = 0;
    if (ReadProcessMemory(m_inferior_handle, (void *)addr, buffer, size,
                          &bytes_read) != TRUE)
        error.SetErrorToGetLastError();

    return bytesRead;
}

size_t
ProcessWindows::DoWriteMemory (addr_t addr, const void *buf, size_t size, Error &error)
{
    assert(m_inferior_handle);

    Log *log(ProcessWindowsLog::GetLogIfAllCategoriesSet(WINDOWS_LOG_ALL));
    if (log && log->GetMask().Test(WINDOWS_LOG_MEMORY))
        log->Printf("ProcessWindows::%s(%" PRIu64 ", %p, %p, %zd, _)",
                    __FUNCTION__, pid, (void *)addr, buf, size);

    SIZE_T bytes_written = 0;
    unsigned char byte = (unsigned char)data;
    if (WriteProcessMemory(m_inferior_handle, (void *)addr, (const void *)&byte,
                           size, &bytes_written) != TRUE)
    {
        error.SetErrorToGetLastError();
        return bytes_written;
    }

    if (FlushInstructionCache(m_inferior_handle, (void *)addr, size) != TRUE)
        error.SetErrorToGetLastError();

    return bytes_written;
}

lldb::addr_t
ProcessWindows::DoAllocateMemory (size_t size, uint32_t permissions, Error &error)
{
    addr_t allocated_addr = LLDB_INVALID_ADDRESS;
    assert(false && "not implemented");
    return allocated_addr;
}

Error
ProcessWindows::GetMemoryRegionInfo (addr_t load_addr,
                                       MemoryRegionInfo &region_info)
{
    Error error;
    assert(false && "not implemented");
    return error;
}

Error
ProcessWindows::GetWatchpointSupportInfo (uint32_t &num)
{
    Error error;
    assert(false && "not implemented");
    return error;
}

Error
ProcessWindows::GetWatchpointSupportInfo (uint32_t &num, bool& after)
{
    Error error;
    assert(false && "not implemented");
    return error;
}

Error
ProcessWindows::DoDeallocateMemory (lldb::addr_t addr)
{
    Error error;
    assert(false && "not implemented");
    return error;
}

//------------------------------------------------------------------
// Process STDIO
//------------------------------------------------------------------
size_t
ProcessWindows::PutSTDIN (const char *src, size_t src_len, Error &error)
{
    if (m_stdio_communication.IsConnected())
    {
        ConnectionStatus status;
        m_stdio_communication.Write(src, src_len, status, NULL);
    }
    return 0;
}

Error
ProcessWindows::EnableBreakpointSite (BreakpointSite *bp_site)
{
    return EnableSoftwareBreakpoint(bp_site);
}

Error
ProcessWindows::DisableBreakpointSite (BreakpointSite *bp_site)
{
    return DisableSoftwareBreakpoint(bp_site);
}

Error
ProcessWindows::EnableWatchpoint (Watchpoint *wp, bool notify)
{
    Error error;
    assert(false && "not implemented");
    return error;
}

Error
ProcessWindows::DisableWatchpoint (Watchpoint *wp, bool notify)
{
    Error error;
    assert(false && "not implemented");
    return error;
}

void
ProcessWindows::Clear()
{
    m_thread_list_real.Clear();
    m_thread_list.Clear();
}

Error
ProcessWindows::DoSignal (int signo)
{
    Error error;
    assert(false && "not implemented");
    return error;
}

void
ProcessWindows::Initialize()
{
    static bool g_initialized = false;

    if (g_initialized == false)
    {
        g_initialized = true;
        PluginManager::RegisterPlugin (GetPluginNameStatic(),
                                       GetPluginDescriptionStatic(),
                                       CreateInstance);

        Log::Callbacks log_callbacks = {
            ProcessWindowsLog::DisableLog,
            ProcessWindowsLog::EnableLog,
            ProcessWindowsLog::ListLogCategories
        };

        Log::RegisterLogChannel (ProcessWindows::GetPluginNameStatic(), log_callbacks);
    }
}

bool
ProcessWindows::StartNoticingNewThreads()
{
    assert(false && "not implemented");
    return false;
}

bool
ProcessWindows::StopNoticingNewThreads()
{
    assert(false && "not implemented");
    return false;
}

lldb_private::DynamicLoader *
ProcessWindows::GetDynamicLoader ()
{
    if (m_dyld_ap.get() == NULL)
        m_dyld_ap.reset (DynamicLoader::FindPlugin(this, NULL));
    return m_dyld_ap.get();
}
