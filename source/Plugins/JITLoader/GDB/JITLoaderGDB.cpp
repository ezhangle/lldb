//===-- JITLoaderGDB.cpp ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// C Includes

#include "lldb/Breakpoint/Breakpoint.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/ModuleSpec.h"
#include "lldb/Core/Section.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/SectionLoadList.h"
#include "lldb/Target/Target.h"
#include "lldb/Symbol/SymbolVendor.h"

#include "JITLoaderGDB.h"

#include "../../SymbolFile/DWARF/SymbolFileDWARF.h"

using namespace lldb;
using namespace lldb_private;

JITLoaderGDB::JITLoaderGDB (lldb_private::Process *process) :
    JITLoader(process),
    m_jit_objects(),
    m_jit_break_id(LLDB_INVALID_BREAK_ID)
{}

JITLoaderGDB::~JITLoaderGDB ()
{
    Clear(true);
}

void JITLoaderGDB::DidAttach() 
{
    SetJITBreakpoint();
}

void JITLoaderGDB::DidLaunch() 
{
    SetJITBreakpoint();
}

void
JITLoaderGDB::Clear (bool clear_process)
{
    if (m_process->IsAlive() && LLDB_BREAK_ID_IS_VALID(m_jit_break_id))
        m_process->GetTarget().RemoveBreakpointByID (m_jit_break_id);

    m_jit_break_id = LLDB_INVALID_BREAK_ID;

    if (clear_process)
        m_process = NULL;
}

//------------------------------------------------------------------
// Debug Interface Structures
//------------------------------------------------------------------
typedef enum
{
    JIT_NOACTION = 0,
    JIT_REGISTER_FN,
    JIT_UNREGISTER_FN
} jit_actions_t;

struct jit_code_entry
{
    struct jit_code_entry *next_entry;
    struct jit_code_entry *prev_entry;
    const char *symfile_addr;
    uint64_t symfile_size;
};
 
struct jit_descriptor
{
    uint32_t version;
    uint32_t action_flag; // Values are jit_action_t
    struct jit_code_entry *relevant_entry;
    struct jit_code_entry *first_entry;
};
     

//------------------------------------------------------------------
// Setup the JIT Breakpoint
//------------------------------------------------------------------
void
JITLoaderGDB::SetJITBreakpoint()
{
    Log *log(GetLogIfAnyCategoriesSet(LIBLLDB_LOG_JIT_LOADER));
    if (log)
        log->Printf("Setting up JIT breakpoint");

    Target &target = m_process->GetTarget();
    Breakpoint *bp = target.CreateBreakpoint(
        NULL,NULL,                          // All modules
        "__jit_debug_register_code",        // Search for the jit notification function
        eFunctionNameTypeFull,              // That's the full function name
        eLazyBoolCalculate,                 
        false,                              // Internal Breakpoint
        false).get();                       // Do not Request a hardware breakpoing

    bp->SetCallback(JITDebugBreakpointHit, this, true);

    m_jit_break_id = bp->GetID();
}

typedef std::map<lldb::addr_t, const lldb::ModuleSP> JITObjectMap;

bool
JITLoaderGDB::JITDebugBreakpointHit(void *baton,
                                              StoppointCallbackContext *context,
                                              user_id_t break_id,
                                              user_id_t break_loc_id)
{
    Log *log(GetLogIfAnyCategoriesSet(LIBLLDB_LOG_JIT_LOADER));
    if (log)
        log->Printf("JIT Breakpoint hit!");
    JITLoaderGDB* instance = static_cast<JITLoaderGDB*>(baton);
    Process *process = instance->m_process;

    SymbolContextList target_symbols;
    Target &target = process->GetTarget();
    ModuleList &images = target.GetImages();

    if (!images.FindSymbolsWithNameAndType(ConstString("__jit_debug_descriptor"), eSymbolTypeData, target_symbols))
    {
        if (log)
            log->Printf("Could not find __jit_debug_descriptor");
        return false;
    }

    SymbolContext sym_ctx;
    target_symbols.GetContextAtIndex(0, sym_ctx);

    const Address *jit_descriptor_addr = &sym_ctx.symbol->GetAddress();

    if (!jit_descriptor_addr || !jit_descriptor_addr->IsValid())
    {
        if(log)
            log->Printf("__jit_debug_descriptor address is not valid");
        return false;
    }

    const addr_t jit_addr = jit_descriptor_addr->GetLoadAddress(&target);

    if (jit_addr == LLDB_INVALID_ADDRESS) {
        log->Printf("Could not get the address for __jit_debug_descriptor in the target process!");
        return false;
    }

    jit_descriptor jit_desc;
    const size_t jit_desc_size = sizeof(jit_desc);
    Error error;
    size_t bytes_read = process->DoReadMemory(jit_addr, &jit_desc, jit_desc_size, error);
    if (bytes_read != jit_desc_size || !error.Success()) {
        if (log)
            log->Printf("Failed to read the JIT descirptor");
        return false;
    }

    jit_code_entry jit_entry;
    const addr_t &jit_relevant_entry = (addr_t)jit_desc.relevant_entry;

    const size_t jit_entry_size = sizeof(jit_entry);
    bytes_read = process->DoReadMemory(jit_relevant_entry, &jit_entry, jit_entry_size, error);
    if (bytes_read != jit_entry_size || !error.Success()) {
        if (log)
        log->Printf("Failed to read the JIT entry!");
        return false;
    }

    const jit_actions_t &jit_action = (jit_actions_t)jit_desc.action_flag;
    const addr_t &symbolfile_addr = (addr_t)jit_entry.symfile_addr;
    const size_t &symbolfile_size = (size_t)jit_entry.symfile_size;
    JITObjectMap &jit_objects = instance->m_jit_objects;
    ModuleSP module_sp;

    if (jit_action == JIT_REGISTER_FN)
    {
        if (log)
            log->Printf("Registering Function!");
        module_sp = process->ReadModuleFromMemory(FileSpec("in_memory_object", false), symbolfile_addr, symbolfile_size);
        if (module_sp)
        {
            bool changed;
            jit_objects.insert(std::pair<lldb::addr_t,const lldb::ModuleSP>(symbolfile_addr,module_sp));
            module_sp->SetLoadAddress(target, 0, changed);
            images.AppendIfNeeded(module_sp);
            SymbolFileDWARF *s = static_cast<SymbolFileDWARF*>(module_sp->GetSymbolVendor()->GetSymbolFile());
            //s->Index();
            //s->DumpIndexes();
        } else {
            if (log)
                log->Printf("Failed to load Module!");
        }
    }
    else if (jit_action == JIT_UNREGISTER_FN)
    {
        if (log)
            log->Printf("Unregistering Function!");
        JITObjectMap::iterator it = jit_objects.find(symbolfile_addr);
        if (it != jit_objects.end())
        {
            module_sp = it->second;
            ObjectFile *image_object_file = module_sp->GetObjectFile();
            if (image_object_file)
            {
                const SectionList *section_list = image_object_file->GetSectionList ();
                if (section_list)
                {
                    const uint32_t num_sections = section_list->GetSize();
                    for (uint32_t i = 0; i<num_sections; ++i)
                    {
                        SectionSP section_sp(section_list->GetSectionAtIndex(i));
                        if (section_sp)
                        {
                            target.GetSectionLoadList().SetSectionUnloaded (section_sp);
                        }
                    }
                }
            }
            images.Remove(module_sp);
            jit_objects.erase(it);
        }
    }
    else if (jit_action == JIT_NOACTION)
    {
        // Nothing to do
    }
    else
    {
        assert(false && "Unknown jit action");
    }

    return false; // Continue Running.
}

//------------------------------------------------------------------
// PluginInterface protocol
//------------------------------------------------------------------
lldb_private::ConstString
JITLoaderGDB::GetPluginNameStatic()
{
    static ConstString g_name("gdbjit");
    return g_name;
}

JITLoader *
JITLoaderGDB::CreateInstance(Process *process, bool force)
{
    return new JITLoaderGDB(process);
}

const char *
JITLoaderGDB::GetPluginDescriptionStatic()
{
    return "JIT loader plug-in that watches for JIT events using the GDB interface.";
}

lldb_private::ConstString
JITLoaderGDB::GetPluginName()
{
    return GetPluginNameStatic();
}

uint32_t
JITLoaderGDB::GetPluginVersion()
{
    return 1;
}

void
JITLoaderGDB::Initialize()
{
    PluginManager::RegisterPlugin (GetPluginNameStatic(),
                                   GetPluginDescriptionStatic(),
                                   CreateInstance);
}

void
JITLoaderGDB::Terminate()
{
    PluginManager::UnregisterPlugin (CreateInstance);
}


