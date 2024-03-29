//===-- OptionGroupVariable.cpp -----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-python.h"

#include "lldb/Interpreter/OptionGroupVariable.h"

// C Includes
// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/Core/Error.h"
#include "lldb/DataFormatters/DataVisualization.h"
#include "lldb/Interpreter/CommandInterpreter.h"
#include "lldb/Target/Target.h"
#include "lldb/Utility/Utils.h"

using namespace lldb;
using namespace lldb_private;

// if you add any options here, remember to update the counters in OptionGroupVariable::GetNumDefinitions()
static OptionDefinition
g_option_table[] =
{
    { LLDB_OPT_SET_1 | LLDB_OPT_SET_2, false, "no-args",         'a', OptionParser::eNoArgument,       nullptr, 0, eArgTypeNone, "Omit function arguments."},
    { LLDB_OPT_SET_1 | LLDB_OPT_SET_2, false, "no-locals",       'l', OptionParser::eNoArgument,       nullptr, 0, eArgTypeNone, "Omit local variables."},
    { LLDB_OPT_SET_1 | LLDB_OPT_SET_2, false, "show-globals",    'g', OptionParser::eNoArgument,       nullptr, 0, eArgTypeNone, "Show the current frame source file global and static variables."},
    { LLDB_OPT_SET_1 | LLDB_OPT_SET_2, false, "show-declaration",'c', OptionParser::eNoArgument,       nullptr, 0, eArgTypeNone, "Show variable declaration information (source file and line where the variable was declared)."},
    { LLDB_OPT_SET_1 | LLDB_OPT_SET_2, false, "regex",           'r', OptionParser::eNoArgument,       nullptr, 0, eArgTypeRegularExpression, "The <variable-name> argument for name lookups are regular expressions."},
    { LLDB_OPT_SET_1 | LLDB_OPT_SET_2, false, "scope",           's', OptionParser::eNoArgument,       nullptr, 0, eArgTypeNone, "Show variable scope (argument, local, global, static)."},
    { LLDB_OPT_SET_1,                  false, "summary",         'y', OptionParser::eRequiredArgument, nullptr, 0, eArgTypeName, "Specify the summary that the variable output should use."},
    { LLDB_OPT_SET_2,                  false, "summary-string",  'z', OptionParser::eRequiredArgument, nullptr, 0, eArgTypeName, "Specify a summary string to use to format the variable output."},
};

static Error
ValidateNamedSummary (const char* str, void*)
{
    if (!str || !str[0])
        return Error("must specify a valid named summary");
    TypeSummaryImplSP summary_sp;
    if (DataVisualization::NamedSummaryFormats::GetSummaryFormat(ConstString(str), summary_sp) == false)
        return Error("must specify a valid named summary");
    return Error();
}

static Error
ValidateSummaryString (const char* str, void*)
{
    if (!str || !str[0])
        return Error("must specify a non-empty summary string");
    return Error();
}

OptionGroupVariable::OptionGroupVariable (bool show_frame_options) :
    OptionGroup(),
    include_frame_options (show_frame_options),
    summary(ValidateNamedSummary),
    summary_string(ValidateSummaryString)
{
}

OptionGroupVariable::~OptionGroupVariable ()
{
}

Error
OptionGroupVariable::SetOptionValue (CommandInterpreter &interpreter,
                                     uint32_t option_idx, 
                                     const char *option_arg)
{
    Error error;
    if (!include_frame_options)
        option_idx += 3;
    const int short_option = g_option_table[option_idx].short_option;
    switch (short_option)
    {
        case 'r':   use_regex    = true;  break;
        case 'a':   show_args    = false; break;
        case 'l':   show_locals  = false; break;
        case 'g':   show_globals = true;  break;
        case 'c':   show_decl    = true;  break;
        case 's':
            show_scope = true;
            break;
        case 'y':
            error = summary.SetCurrentValue(option_arg);
            break;
        case 'z':
            error = summary_string.SetCurrentValue(option_arg);
            break;
        default:
            error.SetErrorStringWithFormat("unrecognized short option '%c'", short_option);
            break;
    }
    
    return error;
}

void
OptionGroupVariable::OptionParsingStarting (CommandInterpreter &interpreter)
{
    show_args     = true;   // Frame option only
    show_locals   = true;   // Frame option only
    show_globals  = false;  // Frame option only
    show_decl     = false;
    use_regex     = false;
    show_scope    = false;
    summary.Clear();
    summary_string.Clear();
}

#define NUM_FRAME_OPTS 3

const OptionDefinition*
OptionGroupVariable::GetDefinitions ()
{
    // Show the "--no-args", "--no-locals" and "--show-globals" 
    // options if we are showing frame specific options
    if (include_frame_options)
        return g_option_table;

    // Skip the "--no-args", "--no-locals" and "--show-globals" 
    // options if we are not showing frame specific options (globals only)
    return &g_option_table[NUM_FRAME_OPTS];
}

uint32_t
OptionGroupVariable::GetNumDefinitions ()
{
    // Count the "--no-args", "--no-locals" and "--show-globals" 
    // options if we are showing frame specific options.
    if (include_frame_options)
        return llvm::array_lengthof(g_option_table);
    else
        return llvm::array_lengthof(g_option_table) - NUM_FRAME_OPTS;
}


