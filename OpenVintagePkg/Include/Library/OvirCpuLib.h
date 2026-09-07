/** @file
  OpenVintage CPU Intermediate Representation (OVIR-CPU) Library.
  Phase 4 Architecture-Agnostic CPU Translation Framework.

  Provides data structures, instruction representations, virtual register
  mapping, basic blocks, control flow graphs, and validation logic.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_CPU_LIB_H_
#define OVIR_CPU_LIB_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvMemoryLib.h>
#include <Library/OvLoggerLib.h>

//
// Supported Processor Architectures
//
typedef enum {
  OvirCpuArchUnknown = 0,
  OvirCpuArchArm64   = 1,
  OvirCpuArchX64     = 2,
  OvirCpuArchRiscV64 = 3
} OVIR_CPU_ARCH;

//
// Register Classes
//
typedef enum {
  OvirRegClassInt = 0,    // General Purpose Integer Register (32/64-bit)
  OvirRegClassFloat,      // Scalar Floating Point (32/64-bit)
  OvirRegClassVector,     // SIMD / Vector Register (128/256-bit)
  OvirRegClassFlag,       // Condition / Status Flags
  OvirRegClassSpecial     // Stack Pointer, Program Counter, Link Register, etc.
} OVIR_REG_CLASS;

//
// Virtual Register Identifiers
//
#define OVIR_VREG_INVALID     0xFFFF
#define OVIR_VREG_SP          0x1000
#define OVIR_VREG_PC          0x1001
#define OVIR_VREG_LR          0x1002
#define OVIR_VREG_FLAGS       0x1003
#define OVIR_VREG_FP          0x1004

typedef struct {
  UINT16          Index;      // Virtual register index (0..255 or special)
  OVIR_REG_CLASS  Class;      // Register type
  UINT8           BitWidth;   // 8, 16, 32, 64, 128, 256
} OVIR_VREG;

//
// Condition Codes (Control Flow)
//
typedef enum {
  OvirCondAlways = 0,   // Unconditional
  OvirCondEq,           // Equal (Z=1)
  OvirCondNe,           // Not Equal (Z=0)
  OvirCondCs,           // Carry Set / Unsigned Higher or Same (C=1)
  OvirCondCc,           // Carry Clear / Unsigned Lower (C=0)
  OvirCondMi,           // Negative / Minus (N=1)
  OvirCondPl,           // Positive or Zero (N=0)
  OvirCondVs,           // Overflow Set (V=1)
  OvirCondVc,           // Overflow Clear (V=0)
  OvirCondHi,           // Unsigned Higher (C=1 and Z=0)
  OvirCondLs,           // Unsigned Lower or Same (C=0 or Z=1)
  OvirCondGe,           // Signed Greater or Equal (N=V)
  OvirCondLt,           // Signed Less Than (N!=V)
  OvirCondGt,           // Signed Greater Than (Z=0 and N=V)
  OvirCondLe            // Signed Less or Equal (Z=1 or N!=V)
} OVIR_COND;

//
// Memory Addressing Modes: [Base + (Index * Scale) + Displacement]
//
typedef struct {
  OVIR_VREG       BaseReg;        // Base address register
  OVIR_VREG       IndexReg;       // Optional index register (OVIR_VREG_INVALID if unused)
  UINT8           Scale;          // 1, 2, 4, 8
  INT64           Displacement;   // Signed byte displacement
  UINT8           AccessSize;     // 1, 2, 4, 8, 16 bytes
} OVIR_MEM_OPERAND;

//
// Immediate Value
//
typedef struct {
  union {
    INT64         IntVal;
    UINT64        UintVal;
    UINT64        FloatBits;
  } Value;
  UINT8           BitWidth;       // 8, 16, 32, 64
  BOOLEAN         IsSigned;
} OVIR_IMM_OPERAND;

//
// Operand Kind
//
typedef enum {
  OvirOpKindNone = 0,
  OvirOpKindReg,
  OvirOpKindImm,
  OvirOpKindMem,
  OvirOpKindLabel
} OVIR_OPERAND_KIND;

//
// General IR Operand
//
typedef struct {
  OVIR_OPERAND_KIND Kind;
  union {
    OVIR_VREG         Reg;
    OVIR_IMM_OPERAND  Imm;
    OVIR_MEM_OPERAND  Mem;
    UINT32            LabelBlockId;
  } As;
} OVIR_CPU_OPERAND;

//
// Intermediate Representation Opcodes
//
typedef enum {
  OvirOpNop = 0,

  // Data Movement
  OvirOpMov,
  OvirOpLoad,
  OvirOpStore,
  OvirOpLea,

  // Arithmetic
  OvirOpAdd,
  OvirOpSub,
  OvirOpMul,
  OvirOpDiv,
  OvirOpNeg,
  OvirOpAbs,
  OvirOpAdc,
  OvirOpSbb,

  // Logical & Bitwise
  OvirOpAnd,
  OvirOpOr,
  OvirOpXor,
  OvirOpNot,
  OvirOpShl,
  OvirOpShr,
  OvirOpSar,
  OvirOpRor,

  // Control Flow & Comparison
  OvirOpCmp,
  OvirOpTest,
  OvirOpJmp,          // Unconditional branch
  OvirOpJcc,          // Conditional branch
  OvirOpCall,         // Function / Procedure call
  OvirOpRet,          // Subroutine return

  // Floating Point Operations
  OvirOpFadd,
  OvirOpFsub,
  OvirOpFmul,
  OvirOpFdiv,
  OvirOpFneg,
  OvirOpFcmp,
  OvirOpFmov,

  // SIMD / Vector Operations
  OvirOpVecLoad,
  OvirOpVecStore,
  OvirOpVecAdd,
  OvirOpVecSub,
  OvirOpVecMul,
  OvirOpVecDot,

  // System & Synchronization
  OvirOpSyscall,
  OvirOpTrap,
  OvirOpFence
} OVIR_OPCODE;

//
// Instruction Representation
//
#define OVIR_MAX_INSTRUCTION_OPERANDS 4

typedef struct {
  OVIR_OPCODE       Opcode;
  UINT8             BitWidth;       // 8, 16, 32, 64, 128
  OVIR_COND         Condition;      // Condition code (OvirCondAlways if unconditional)
  BOOLEAN           SetFlags;       // Does this instruction update CPU condition flags
  UINT8             OperandCount;   // Number of operands (0..4)
  OVIR_CPU_OPERAND  Operands[OVIR_MAX_INSTRUCTION_OPERANDS];
  UINT64            GuestPc;        // Original guest instruction address
  UINT8             GuestLength;    // Length of guest instruction in bytes
  UINT32            Flags;          // Additional metadata flags
} OVIR_CPU_INSTRUCTION;

//
// Basic Block & Control Flow Graph
//
#define OVIR_MAX_BLOCK_INSTRUCTIONS   64
#define OVIR_MAX_BLOCK_SUCCESSORS     2

typedef enum {
  OvirTerminatorFallthrough = 0,
  OvirTerminatorBranch,
  OvirTerminatorCondBranch,
  OvirTerminatorCall,
  OvirTerminatorReturn,
  OvirTerminatorTrap
} OVIR_TERMINATOR_TYPE;

typedef struct {
  UINT32                BlockId;
  UINT64                StartGuestPc;
  UINT64                EndGuestPc;
  UINT32                InstructionCount;
  OVIR_CPU_INSTRUCTION  Instructions[OVIR_MAX_BLOCK_INSTRUCTIONS];
  UINT32                SuccessorCount;
  UINT32                Successors[OVIR_MAX_BLOCK_SUCCESSORS];
  OVIR_TERMINATOR_TYPE  TerminatorType;
  BOOLEAN               IsTerminated;
} OVIR_CPU_BASIC_BLOCK;

//
// Program / Function IR
//
#define OVIR_MAX_PROGRAM_BLOCKS 32

typedef struct {
  UINT32                ProgramId;
  OVIR_CPU_ARCH         SourceArch;
  OVIR_CPU_ARCH         TargetArch;
  UINT32                BlockCount;
  OVIR_CPU_BASIC_BLOCK  Blocks[OVIR_MAX_PROGRAM_BLOCKS];
  UINT32                TotalInstructionCount;
  BOOLEAN               IsValidated;
} OVIR_CPU_PROGRAM;

//
// Core IR Construction & Validation APIs
//

/**
  Initialize an empty basic block.

  @param[out] Block         Basic block structure to initialize.
  @param[in]  BlockId       Unique identifier for the block.
  @param[in]  StartGuestPc  Base guest virtual address.
**/
VOID
EFIAPI
OvirCpuInitBlock (
  OUT OVIR_CPU_BASIC_BLOCK  *Block,
  IN  UINT32                BlockId,
  IN  UINT64                StartGuestPc
  );

/**
  Append an instruction to a basic block.

  @param[in,out] Block        Target basic block.
  @param[in]     Instruction  Instruction to append.

  @retval EFI_SUCCESS         Instruction appended.
  @retval EFI_BUFFER_TOO_SMALL Block has reached max capacity.
**/
EFI_STATUS
EFIAPI
OvirCpuAppendInstruction (
  IN OUT OVIR_CPU_BASIC_BLOCK        *Block,
  IN     CONST OVIR_CPU_INSTRUCTION  *Instruction
  );

/**
  Initialize an empty IR program.

  @param[out] Program     Program structure to initialize.
  @param[in]  ProgramId   Unique identifier for the program.
  @param[in]  SourceArch  Source processor architecture.
  @param[in]  TargetArch  Target execution architecture.
**/
VOID
EFIAPI
OvirCpuInitProgram (
  OUT OVIR_CPU_PROGRAM  *Program,
  IN  UINT32            ProgramId,
  IN  OVIR_CPU_ARCH     SourceArch,
  IN  OVIR_CPU_ARCH     TargetArch
  );

/**
  Append a basic block to an IR program.

  @param[in,out] Program    Target program.
  @param[in]     Block      Block to copy into program.

  @retval EFI_SUCCESS       Block appended.
  @retval EFI_BUFFER_TOO_SMALL Program block limit reached.
**/
EFI_STATUS
EFIAPI
OvirCpuAppendBlock (
  IN OUT OVIR_CPU_PROGRAM            *Program,
  IN     CONST OVIR_CPU_BASIC_BLOCK  *Block
  );

/**
  Validate structural and semantic correctness of an IR instruction.

  @param[in] Instruction    Instruction to validate.

  @retval EFI_SUCCESS       Instruction is valid.
  @retval EFI_INVALID_PARAMETER Instruction has invalid opcode/operands.
**/
EFI_STATUS
EFIAPI
OvirCpuValidateInstruction (
  IN CONST OVIR_CPU_INSTRUCTION  *Instruction
  );

/**
  Validate an entire basic block.

  @param[in] Block          Basic block to validate.

  @retval EFI_SUCCESS       All instructions and control flow are valid.
  @retval EFI_INVALID_PARAMETER Validation failure.
**/
EFI_STATUS
EFIAPI
OvirCpuValidateBlock (
  IN CONST OVIR_CPU_BASIC_BLOCK  *Block
  );

/**
  Validate an entire IR program.

  @param[in,out] Program    Program to validate.

  @retval EFI_SUCCESS       Program is valid.
  @retval EFI_INVALID_PARAMETER Validation failure.
**/
EFI_STATUS
EFIAPI
OvirCpuValidateProgram (
  IN OUT OVIR_CPU_PROGRAM  *Program
  );

/**
  Convert opcode enum to human-readable string.

  @param[in] Opcode   Opcode enum.

  @return String representation.
**/
CONST CHAR16 *
EFIAPI
OvirCpuOpcodeToString (
  IN OVIR_OPCODE  Opcode
  );

/**
  Convert architecture enum to string.

  @param[in] Arch     Architecture enum.

  @return String representation.
**/
CONST CHAR16 *
EFIAPI
OvirCpuArchToString (
  IN OVIR_CPU_ARCH  Arch
  );

#endif // OVIR_CPU_LIB_H_
