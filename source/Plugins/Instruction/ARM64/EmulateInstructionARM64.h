//===-- EmulateInstructionARM64.h ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef EmulateInstructionARM64_h_
#define EmulateInstructionARM64_h_

#include "lldb/Core/EmulateInstruction.h"
#include "lldb/Core/Error.h"
#include "lldb/Interpreter/OptionValue.h"
#include "Plugins/Process/Utility/ARMDefines.h"

class EmulateInstructionARM64 : public lldb_private::EmulateInstruction
{
public: 
    static void
    Initialize ();
    
    static void
    Terminate ();

    static lldb_private::ConstString
    GetPluginNameStatic ();
    
    static const char *
    GetPluginDescriptionStatic ();
    
    static lldb_private::EmulateInstruction *
    CreateInstance (const lldb_private::ArchSpec &arch, 
                    lldb_private::InstructionType inst_type);
    
    static bool
    SupportsEmulatingInstructionsOfTypeStatic (lldb_private::InstructionType inst_type)
    {
        switch (inst_type)
        {
            case lldb_private::eInstructionTypeAny:
            case lldb_private::eInstructionTypePrologueEpilogue:
                return true;

            case lldb_private::eInstructionTypePCModifying:
            case lldb_private::eInstructionTypeAll:
                return false;
        }
        return false;
    }

    virtual lldb_private::ConstString
    GetPluginName();

    virtual lldb_private::ConstString
    GetShortPluginName()
    {
        return GetPluginNameStatic();
    }

    virtual uint32_t
    GetPluginVersion()
    {
        return 1;
    }

    bool
    SetTargetTriple (const lldb_private::ArchSpec &arch);
    
    EmulateInstructionARM64 (const lldb_private::ArchSpec &arch) :
        EmulateInstruction (arch),
        m_opcode_pstate (),
        m_emulated_pstate (),
        m_ignore_conditions (false)
    {
    }

    virtual bool
    SupportsEmulatingInstructionsOfType (lldb_private::InstructionType inst_type)
    {
        return SupportsEmulatingInstructionsOfTypeStatic (inst_type);
    }

    virtual bool 
    ReadInstruction ();
    
    virtual bool
    EvaluateInstruction (uint32_t evaluate_options);
    
    virtual bool
    TestEmulation (lldb_private::Stream *out_stream, 
                   lldb_private::ArchSpec &arch, 
                   lldb_private::OptionValueDictionary *test_data)
    {
        return false;
    }

    virtual bool
    GetRegisterInfo (uint32_t reg_kind, 
                     uint32_t reg_num, 
                     lldb_private::RegisterInfo &reg_info);

    virtual bool
    CreateFunctionEntryUnwind (lldb_private::UnwindPlan &unwind_plan);

    
    typedef enum 
    {
        AddrMode_OFF, 
        AddrMode_PRE, 
        AddrMode_POST
    } AddrMode;
    
    typedef enum 
    {
        BranchType_CALL, 
        BranchType_ERET, 
        BranchType_DRET, 
        BranchType_RET, 
        BranchType_JMP
    } BranchType;
    
    typedef enum
    {
        CountOp_CLZ, 
        CountOp_CLS, 
        CountOp_CNT
    }  CountOp;
    
    typedef enum
    {
        RevOp_RBIT, 
        RevOp_REV16, 
        RevOp_REV32, 
        RevOp_REV64
    } RevOp;
    
    typedef enum 
    {
        BitwiseOp_NOT, 
        BitwiseOp_RBIT
    } BitwiseOp;
    

    typedef enum
    {
        EL0 = 0,
        EL1 = 1, 
        EL2 = 2,
        EL3 = 3
    } ExceptionLevel;
    
    typedef enum 
    {
        ExtendType_SXTB, 
        ExtendType_SXTH, 
        ExtendType_SXTW, 
        ExtendType_SXTX,
        ExtendType_UXTB, 
        ExtendType_UXTH, 
        ExtendType_UXTW, 
        ExtendType_UXTX
    } ExtendType;
    
    typedef enum 
    {
        ExtractType_LEFT, 
        ExtractType_RIGHT
    } ExtractType;
    
    typedef enum 
    {
        LogicalOp_AND, 
        LogicalOp_EOR, 
        LogicalOp_ORR
    } LogicalOp;
    
    typedef enum 
    {
        MemOp_LOAD, 
        MemOp_STORE, 
        MemOp_PREFETCH, 
        MemOp_NOP
    } MemOp;
    
    typedef enum 
    {
        MoveWideOp_N, 
        MoveWideOp_Z, 
        MoveWideOp_K
    } MoveWideOp;
    
    typedef enum {
        ShiftType_LSL, 
        ShiftType_LSR, 
        ShiftType_ASR, 
        ShiftType_ROR
    } ShiftType;

    typedef enum 
    {
        SP0 = 0,
        SPx = 1
    } StackPointerSelection;
    
    typedef enum 
    {
        Unpredictable_WBOVERLAP, 
        Unpredictable_LDPOVERLAP
    } Unpredictable;
    
    typedef enum
    {
        Constraint_NONE, 
        Constraint_UNKNOWN, 
        Constraint_SUPPRESSWB, 
        Constraint_NOP
    } ConstraintType;
    
    typedef enum
    {
        AccType_NORMAL, 
        AccType_UNPRIV, 
        AccType_STREAM,
        AccType_ALIGNED, 
        AccType_ORDERED
    } AccType;

    typedef struct
    {
        uint32_t
            N:1,
            V:1,
            C:1,
            Z:1,   // condition code flags – can also be accessed as PSTATE.[N,Z,C,V]
            Q:1,   // AArch32 only – CSPR.Q bit
            IT:8,  // AArch32 only – CPSR.IT bits
            J:1,   // AArch32 only – CSPR.J bit
            T:1,   // AArch32 only – CPSR.T bit
            SS:1,  // Single step process state bit
            IL:1,  // Illegal state bit
            D:1,
            A:1,
            I:1,
            F:1,   // Interrupt masks – can also be accessed as PSTATE.[D,A,I,F]
            E:1,   // AArch32 only – CSPR.E bit
            M:5,   // AArch32 only – mode encodings
            RW:1,  // Current register width – 0 is AArch64, 1 is AArch32
            EL:2,  // Current exception level (see ExceptionLevel enum)
            SP:1;  // AArch64 only - Stack Pointer selection (see StackPointerSelection enum)
    } ProcState;

protected:

    typedef struct
    {
        uint32_t mask;
        uint32_t value;
        uint32_t vfp_variants;
        bool (EmulateInstructionARM64::*callback) (const uint32_t opcode);
        const char *name;
    }  Opcode;
    
    static Opcode*
    GetOpcodeForInstruction (const uint32_t opcode);

    bool
    Emulate_addsub_imm (const uint32_t opcode);
    
//    bool
//    Emulate_STP_Q_ldstpair_off (const uint32_t opcode);
//    
//    bool
//    Emulate_STP_S_ldstpair_off (const uint32_t opcode);
//    
//    bool
//    Emulate_STP_32_ldstpair_off (const uint32_t opcode);
//    
//    bool
//    Emulate_STP_D_ldstpair_off (const uint32_t opcode);
//    
    bool
    Emulate_ldstpair_off (const uint32_t opcode);

    bool
    Emulate_ldstpair_pre (const uint32_t opcode);
    
    bool
    Emulate_ldstpair (const uint32_t opcode, AddrMode a_mode);

    ProcState m_opcode_pstate;
    ProcState m_emulated_pstate; // This can get updated by the opcode.
    bool m_ignore_conditions;
};

#endif  // EmulateInstructionARM64_h_
