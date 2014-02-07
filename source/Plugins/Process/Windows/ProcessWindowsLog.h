//===-- ProcessWindowsLog.h -----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_ProcessWindowsLog_h_
#define liblldb_ProcessWindowsLog_h_

// C Includes
// C++ Includes
// Other libraries and framework includes

// Project includes
#include "lldb/Core/Log.h"

#define WINDOWS_LOG_VERBOSE                  (1u << 0)
#define WINDOWS_LOG_PROCESS                  (1u << 1)
#define WINDOWS_LOG_THREAD                   (1u << 2)
#define WINDOWS_LOG_PACKETS                  (1u << 3)
#define WINDOWS_LOG_MEMORY                   (1u << 4)    // Log memory reads/writes calls
#define WINDOWS_LOG_MEMORY_DATA_SHORT        (1u << 5)    // Log short memory reads/writes bytes
#define WINDOWS_LOG_MEMORY_DATA_LONG         (1u << 6)    // Log all memory reads/writes bytes
#define WINDOWS_LOG_BREAKPOINTS              (1u << 7)
#define WINDOWS_LOG_WATCHPOINTS              (1u << 8)
#define WINDOWS_LOG_STEP                     (1u << 9)
#define WINDOWS_LOG_COMM                     (1u << 10)
#define WINDOWS_LOG_ASYNC                    (1u << 11)
#define WINDOWS_LOG_ALL                      (UINT32_MAX)
#define WINDOWS_LOG_DEFAULT                  WINDOWS_LOG_PACKETS

class ProcessWindowsLog
{
public:
    static lldb_private::Log *
    GetLogIfAllCategoriesSet(uint32_t mask = 0);
    
    static lldb_private::Log *
    GetLogIfAnyCategoryIsSet (uint32_t mask);

    static void
    DisableLog (const char **categories, lldb_private::Stream *feedback_strm);

    static lldb_private::Log *
    EnableLog (lldb::StreamSP &log_stream_sp, uint32_t log_options, const char **categories, lldb_private::Stream *feedback_strm);

    static void
    ListLogCategories (lldb_private::Stream *strm);

    static void
    LogIf (uint32_t mask, const char *format, ...);
};

#endif  // liblldb_ProcessWindowsLog_h_
