/*
 * Author: Jaebaek Seo <jaebaek@kaist.ac.kr>
 */
#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/LiveVariables.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/MC/MCContext.h"

#define DEBUG_TYPE "x86-sgx-aslr"
#define DEBUG_X86SGXASLR 0

using namespace llvm;

#include <vector>
using namespace std;

/*
 * Jaebaek: An assumption for efficient instrumentation:
 * The base address of an enclave is aligned with upperPowerOf2(size_of_enclave)
 * (e.g., 32MB aligned for 30MB enclave).
 * Based on this assumption, the following instrumentation is allowed:

    Before ..
       sub  0x120, %rsp
    After ..
       sub  0x120, %esp
       sub  %r15d, %esp
       lea  (%rsp, %r15, 1), %rsp

    which is same with ..
       sub  0x120, %rsp
       sub  %r15, %rsp
       mov  %esp, %esp
       lea  (%rsp, %r15, 1), %rsp

 * This is because there is no case s.t. R15 < 4GB-boundary <= RSP
 * i.e., always 4GB-boundary <= R15 <= RSP <= 4GB-boundary

       sub  0x120, %esp --> if under-flow occurs, rsp - 0x120 < r15
       sub  %r15d, %esp --> under-flow does not occur if r15 <= rsp
       lea  (%rsp, %r15, 1), %rsp
 */

namespace {
  struct X86SGXASLR : public MachineFunctionPass {
    public:
      static char ID;
      X86SGXASLR() : MachineFunctionPass(ID) {}

      bool runOnMachineFunction(MachineFunction &MF) override;
    private:
      bool applyASLR(MachineFunction &MF, MachineFunction::iterator MFI);
      bool applySoftDEPandSFI(MachineBasicBlock &MBB);

      bool ApplyRewrites(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
      bool enforceRSPGreatOrEqualtoR15(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
      bool enforceDSTMemGreatOrEqualtoR15(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
      bool enforceRBPGreatOrEqualtoR15(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
      bool alignedControlNotToOCALL(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);

      const TargetInstrInfo *TII;
      const TargetRegisterInfo *TRI;
      bool isEnclaveMain;

      // later, it will be used to EFLAGS backup if needed for conditional instructions
      MachineBasicBlock::iterator addedBundleHeader;
      bool modifiedWithoutAddedBundle;
  };
  char X86SGXASLR::ID = 0;
}

#if DEBUG_X86SGXASLR
#include <cstdio>
#endif
bool X86SGXASLR::runOnMachineFunction(MachineFunction &Func)
{
  if (Func.hasInlineAsm())
    return false;
  TII = Func.getSubtarget().getInstrInfo();
  TRI = Func.getSubtarget().getRegisterInfo();
#if DEBUG_X86SGXASLR
  printf("X86SGXASLR %s\n", Func.getName().data());
#endif
  bool modified = false;
  isEnclaveMain = (Func.getName() == "enclave_main");
    /*
  if (Func.getName() == "enclave_main") {
    MachineBasicBlock &EntBB = *Func.begin();
    MachineBasicBlock::iterator FirstI = EntBB.begin();
    MachineInstr &MI = *FirstI;
    DebugLoc DL = MI.getDebugLoc();

//       pop    %r13 --> pop the return address
//       movabs $__enclave_exit, %r14
//       mov    %r13, (%r14)
//       movabs $__stack_backup, %r14
//       mov    %rsp, (%r14)
//       movabs $_stack, %rsp
//       // add    $0x7fff00, %rsp
//       ...
//      enclave_exit:
//       mov    (__stack_backup), %rsp
//       jmp    *(__enclave_exit)
    BuildMI(EntBB, FirstI, DL, TII->get(X86::POP64r)).addReg(X86::R13);
    BuildMI(EntBB, FirstI, DL, TII->get(X86::MOV64ri))
      .addReg(X86::R14).addSym(Func.getContext().getOrCreateSymbol("__enclave_exit"));
    BuildMI(EntBB, FirstI, DL, TII->get(X86::MOV64mr))
      .addReg(X86::R14).addImm(0).addReg(0).addImm(0).addReg(0)
      .addReg(X86::R13);
    BuildMI(EntBB, FirstI, DL, TII->get(X86::MOV64ri))
      .addReg(X86::R14).addSym(Func.getContext().getOrCreateSymbol("__stack_backup"));
    BuildMI(EntBB, FirstI, DL, TII->get(X86::MOV64mr))
      .addReg(X86::R14).addImm(0).addReg(0).addImm(0).addReg(0)
      .addReg(X86::RSP);
    BuildMI(EntBB, FirstI, DL, TII->get(X86::MOV64ri))
      .addReg(X86::RSP).addSym(Func.getContext().getOrCreateSymbol("_stack"));
//    BuildMI(EntBB, FirstI, DL, TII->get(X86::ADD64ri32), X86::RSP)
//      .addReg(X86::RSP).addImm(0x7fff00);

//    movabs $dep.bdr, %r15
//    movabs $ocall.bdr, %r14
    BuildMI(EntBB, FirstI, DL, TII->get(X86::MOV64ri))
      .addReg(X86::R15).addSym(Func.getContext().getOrCreateSymbol("dep.bdr"));
    BuildMI(EntBB, FirstI, DL, TII->get(X86::MOV64ri))
      .addReg(X86::R14).addSym(Func.getContext().getOrCreateSymbol("ocall.bdr"));

    isEnclaveMain = true;
  } else
    isEnclaveMain = false;
    */

  for (MachineFunction::iterator I = Func.begin(), E = Func.end(); I != E; ++I) {
    // modified |= applyASLR(Func, I);
    modified |= applySoftDEPandSFI(*I);
  }
  return modified;
}

/*
 * Jaebaek: I cannot find TAILJMPd while compiling musl-libc.
 */
bool X86SGXASLR::ApplyRewrites(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI) {
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();
  unsigned Opc = MI.getOpcode();

  // These direct jumps need their opcode rewritten
  // and variable operands removed.
  unsigned NewOpc = 0;
  switch (Opc) {
  // 32-bit direct calls are handled unmodified by the assemblers
  case X86::CALLpcrel32          : return true;
  case X86::TAILJMPd             : NewOpc = X86::JMP_4; break;
  }
  if (NewOpc) {
    assert(!"TAILJMPd in");
    BuildMI(MBB, MBBI, DL, TII->get(NewOpc))
      .addOperand(MI.getOperand(0));
    MI.eraseFromParent();
    return true;
  }
  return false;
}

static bool IsPushPop(MachineInstr &MI) {
  const unsigned Opcode = MI.getOpcode();
  switch (Opcode) {
    default:
      return false;
    case X86::PUSH64r:
    case X86::POP64r:
      return true;
  }
}

static bool IsFrameChange(MachineInstr &MI, const TargetRegisterInfo *TRI) {
  return MI.modifiesRegister(X86::EBP, TRI);
}

static bool IsStackChange(MachineInstr &MI, const TargetRegisterInfo *TRI) {
  return MI.modifiesRegister(X86::ESP, TRI);
}

static bool HasControlFlow(const MachineInstr &MI) {
  return MI.getDesc().isBranch() ||
    MI.getDesc().isCall() ||
    MI.getDesc().isReturn() ||
    MI.getDesc().isTerminator() ||
    MI.getDesc().isBarrier(); // Jaebaek: we do not support barrier SFI
}

static bool IsDirectBranch(const MachineInstr &MI) {
  return  MI.getDesc().isBranch() &&
    !MI.getDesc().isIndirectBranch();
}

//
// True if this MI restores RSP from RBP with a slight adjustment offset.
//
static bool MatchesSPAdj(const MachineInstr &MI) {
  assert (MI.getOpcode() == X86::LEA64r && "Call to MatchesSPAdj w/ non LEA");
  const MachineOperand &DestReg = MI.getOperand(0);
  const MachineOperand &BaseReg = MI.getOperand(1);
  const MachineOperand &Scale = MI.getOperand(2);
  const MachineOperand &IndexReg = MI.getOperand(3);
  const MachineOperand &Offset = MI.getOperand(4);
  return (DestReg.isReg() && DestReg.getReg() == X86::RSP &&
          BaseReg.isReg() && BaseReg.getReg() == X86::RBP &&
          Scale.getImm() == 1 &&
          IndexReg.isReg() && IndexReg.getReg() == 0 &&
          Offset.isImm());
}

/*
   MBBI
   sub  %r15d, %rXd
   lea  (%rX, %r15, 1), %rX

   make above instructions as a bundle
*/
static void resetRXBasedOnRZP(unsigned rX, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI, const TargetInstrInfo *TII, DebugLoc& DL) {
  unsigned rX32 = getX86SubSuperRegister(rX, 32, false);
  MachineFunction &MF = *MBB.getParent();

  MachineInstr *LEA = MF.CreateMachineInstr(TII->get(X86::LEA64r), DL);
  MBB.insertAfter(MBBI, LEA);
  MachineInstrBuilder(MF, LEA)
    .addReg(rX).addReg(rX)
    .addImm(1).addReg(X86::R15).addImm(0).addReg(0);
  MachineBasicBlock::iterator LEAI = LEA;

  BuildMI(MBB, LEAI, DL, TII->get(X86::SUB32rr), rX32)
    .addReg(rX32).addReg(X86::R15D);

  MIBundleBuilder(MBB, MBBI, ++LEAI);
  finalizeBundle(MBB, MBBI.getInstrIterator());
}

bool X86SGXASLR::enforceRSPGreatOrEqualtoR15(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI) {
  MachineInstr &MI = *MBBI;

  if (!IsStackChange(MI, TRI))
    return false;

  if (IsPushPop(MI))
    return false;

  if (MI.getDesc().isCall())
    return false;

  unsigned Opc = MI.getOpcode();
  DebugLoc DL = MI.getDebugLoc();
  unsigned DestReg = MI.getOperand(0).getReg();
  assert(DestReg == X86::ESP || DestReg == X86::RSP);

  switch (Opc) {
    /*
       before:
         sub  A, %rsp
       after:
         sub  %r15d, %esp
         sub  A, %esp
         lea  (%rsp, %r15, 1), %rsp
    */
    case X86::SUB64ri8 :
    case X86::SUB64ri32: {
      unsigned tmpOpc = (Opc == X86::SUB64ri8) ? X86::SUB32ri8 : X86::SUB32ri;
      MachineInstr *subMI
        = BuildMI(MBB, MBBI, DL, TII->get(X86::SUB32rr), X86::ESP)
        .addReg(X86::ESP).addReg(X86::R15D);
      MachineBasicBlock::iterator SUBI = subMI;

      BuildMI(MBB, MBBI, DL, TII->get(tmpOpc), X86::ESP)
        .addReg(X86::ESP).addImm(MI.getOperand(2).getImm());

      MachineInstr *tmpMI
        = BuildMI(MBB, MBBI, DL, TII->get(X86::LEA64r))
        .addReg(X86::RSP).addReg(X86::RSP)
        .addImm(1).addReg(X86::R15).addImm(0).addReg(0);
      MachineBasicBlock::iterator LEAI = tmpMI;

      MI.eraseFromParent();

      addedBundleHeader = SUBI;

      MIBundleBuilder(MBB, SUBI, ++LEAI);
      finalizeBundle(MBB, SUBI.getInstrIterator());
      return true;
    }
    case X86::SUB64rr: case X86::SUB32rr: case X86::SUB16rr:
    case X86::SUB8rr: case X86::SUB32ri: case X86::SUB32ri8:
    case X86::SUB16ri: case X86::SUB16ri8: case X86::SUB8ri:
      assert(!"another RSP SUB!!!");

    /*
       before:
         and  A, %rsp
       after:
         and  A, %rsp
         resetRXBasedOnRZP
    */
    case X86::AND64ri8:
    case X86::AND64ri32: {
      addedBundleHeader = MBBI;
      unsigned tmpOpc = (Opc == X86::AND64ri8) ? X86::AND32ri8 : X86::AND32ri;
      MI.setDesc(TII->get(tmpOpc));
      MI.getOperand(0).setReg(X86::ESP);
      MI.getOperand(1).setReg(X86::ESP);
      resetRXBasedOnRZP(X86::RSP, MBB, MBBI, TII, DL);
      return true;
    }
    case X86::AND64rr: case X86::AND32rr: case X86::AND16rr:
    case X86::AND8rr: case X86::AND32ri: case X86::AND32ri8:
    case X86::AND16ri: case X86::AND16ri8: case X86::AND8ri:
      assert(!"another RSP AND!!!");

    // do nothing for ADD
    case X86::ADD64ri8: case X86::ADD64ri32:
    case X86::ADD64rr: case X86::ADD32rr: case X86::ADD16rr:
    case X86::ADD8rr: case X86::ADD32ri: case X86::ADD32ri8:
    case X86::ADD16ri: case X86::ADD16ri8: case X86::ADD8ri:
      return false;

    default: break;
  }

  // Promote "MOV ESP, EBP" to a 64-bit move
  if (Opc == X86::MOV32rr && MI.getOperand(1).getReg() == X86::EBP) {
    MI.getOperand(0).setReg(X86::RSP);
    MI.getOperand(1).setReg(X86::RBP);
    MI.setDesc(TII->get(X86::MOV64rr));
    Opc = X86::MOV64rr;
  }

  // "MOV RBP, RSP" is already safe
  if (Opc == X86::MOV64rr && MI.getOperand(1).getReg() == X86::RBP) {
    modifiedWithoutAddedBundle = true;
    return true;
  }

  assert(Opc != X86::LEA32r && "Invalid opcode in 64-bit mode!");
  if (Opc == X86::LEA64_32r){
    unsigned BaseReg = MI.getOperand(1).getReg();
    if (BaseReg == X86::EBP) {
      // leal N(%ebp), %esp can be promoted to leaq N(%rbp), %rsp, which
      // converts to SPAJDi32 below.
      unsigned Scale   = MI.getOperand(2).getImm();
      unsigned IndexReg = MI.getOperand(3).getReg();
      assert(Scale == 1);
      assert(IndexReg == 0);
      MI.getOperand(0).setReg(X86::RSP);
      MI.getOperand(1).setReg(X86::RBP);
      MI.setDesc(TII->get(X86::LEA64r));
      Opc = X86::LEA64r;
    } else {
      /*
         before:
           lea  disp32(base, index, scale), %esp
         after:
           lea  disp32(base, index, scale), %esp
           resetRXBasedOnRZP
      */
      addedBundleHeader = MBBI;
      resetRXBasedOnRZP(X86::RSP, MBB, MBBI, TII, DL);
      return true;
    }
  }

  if (Opc == X86::LEA64r && MatchesSPAdj(MI)) {
    return false;
  }

  /*
     before:
       mov  %eax, %esp or mov  %rax, %rsp
     after:
       mov  %eax, %esp
       resetRXBasedOnRZP
  */
  if (Opc == X86::MOV64rr) {
    MI.getOperand(0).setReg(X86::ESP);
    MI.getOperand(1).setReg(getX86SubSuperRegister(
          MI.getOperand(1).getReg(), 32, false));
    MI.setDesc(TII->get(X86::MOV32rr));
    Opc = X86::MOV32rr;
  }
  if (Opc == X86::MOV32rr) {
    addedBundleHeader = MBBI;
    resetRXBasedOnRZP(X86::RSP, MBB, MBBI, TII, DL);
    return true;
  }

  if (Opc == X86::MOV32rm || Opc == X86::MOV64rm) {
    addedBundleHeader = MBBI;
    resetRXBasedOnRZP(X86::RSP, MBB, MBBI, TII, DL);
    return true;
  }

  llvm_unreachable("Unhandled Stack SFI");
}

static bool FindMemoryOperand(const MachineInstr &MI,
    SmallVectorImpl<unsigned>* indices) {
  int NumFound = 0;
  for (unsigned i = 0; i < MI.getNumOperands(); ) {
    if (isMem(MI, i)) {
      NumFound++;
      indices->push_back(i);
      i += X86::AddrNumOperands;
    } else {
      i++;
    }
  }

  // Intrinsics and other functions can have mayLoad and mayStore to reflect
  // the side effects of those functions.  This function is used to find
  // explicit memory references in the instruction, of which there are none.
  if (NumFound == 0)
    return false;
  return true;
}

//
// Sandboxes loads and stores (64-bit only)
//
bool X86SGXASLR::enforceDSTMemGreatOrEqualtoR15(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI) {
  MachineInstr &MI = *MBBI;

  if (!MI.mayStore()) // !MI.mayLoad() &&
    return false;

  if (IsPushPop(MI))
    return false;

  if (MI.getNumOperands() < 5 || MI.getOpcode() < X86::AAA)
    return false;

  DebugLoc DL = MI.getDebugLoc();

  SmallVector<unsigned, 2> MemOps;
  if (!FindMemoryOperand(MI, &MemOps))
    return false;

  // Jaebaek: as a prototype, only support one memory operand
  assert(MemOps.size() <= 1);

  for (unsigned MemOp : MemOps) {
    MachineOperand &BaseReg  = MI.getOperand(MemOp + 0);
    MachineOperand &Scale = MI.getOperand(MemOp + 1);
    MachineOperand &IndexReg  = MI.getOperand(MemOp + 2);
    MachineOperand &Disp = MI.getOperand(MemOp + 3);
    MachineOperand &SegmentReg = MI.getOperand(MemOp + 4);

    // If one of RIP, RBP and RSP is a base reg
    // and no index reg, it is safe
    // --> because the attacker cannot change the dest address
    if ((BaseReg.getReg() == X86::RIP
          || BaseReg.getReg() == X86::RBP
          || BaseReg.getReg() == X86::RSP)
        && IndexReg.getReg() == 0
        && SegmentReg.getReg() == 0)
      continue;

    unsigned rX;
    MachineBasicBlock::iterator head;
    /*
      before:
       mov  src, (base)
      after:
       sub  %r15d, %base-lower
       mov  src, (%r15, %base, 1)
     */
    if (0) {// (IndexReg.getReg() == 0) {
      rX = BaseReg.getReg();
      unsigned rX32 = getX86SubSuperRegister(rX, 32, false);
      MachineInstr *subMI
        = BuildMI(MBB, MBBI, DL, TII->get(X86::SUB32rr), rX32).addReg(rX32)
        .addReg(X86::R15D);
      head = subMI;
    } else {
      /*
        before:
         mov  src, disp(base, index, scale)
        after:
         lea  disp(base, index, scale), %r13
         sub  %r15d, %r13d
         mov  src, (%r15, %r13, 1)
       */
      rX = X86::R13;
      MachineInstr *leaMI
        = BuildMI(MBB, MBBI, DL, TII->get(X86::LEA64r))
        .addReg(X86::R13).addReg(BaseReg.getReg())
        .addOperand(Scale).addReg(IndexReg.getReg())
        .addOperand(Disp).addReg(SegmentReg.getReg());
      head = leaMI;

      BuildMI(MBB, MBBI, DL, TII->get(X86::SUB32rr), X86::R13D)
        .addReg(X86::R13D).addReg(X86::R15D);
    }

    MachineInstrBuilder tmp = BuildMI(MBB, MBBI, DL, MI.getDesc());
    for (unsigned i = 0;i < MI.getNumOperands();) {
      if (i != MemOp) {
        tmp.addOperand(MI.getOperand(i));
        ++i;
      } else {
        tmp.addReg(X86::R15).addImm(1).addReg(rX).addImm(0).addReg(0);
        i += 5;
      }
    }
    MachineBasicBlock::iterator I = *tmp;

    MI.eraseFromParent();

    addedBundleHeader = head;

    MIBundleBuilder(MBB, head, ++I);
    finalizeBundle(MBB, head.getInstrIterator());
    return true;
  }
  return false;
}

bool X86SGXASLR::enforceRBPGreatOrEqualtoR15(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI) {
  MachineInstr &MI = *MBBI;

  return false;
  if (!IsFrameChange(MI, TRI))
    return false;

  unsigned Opc = MI.getOpcode();
  DebugLoc DL = MI.getDebugLoc();

  // Handle moves to RBP
  if (Opc == X86::MOV64rr || Opc == X86::MOV64rm) {
    assert(MI.getOperand(0).getReg() == X86::RBP);
    unsigned SrcReg = MI.getOperand(1).getReg();

    // MOV RBP, RSP is already safe
    if (SrcReg == X86::RSP)
      return false;

    /*
      before:
       mov  %rX, %rbp
      after:
       mov  %rX, %rbp
       sub  %r15d, %ebp
       lea  (%rbp, %r15, 1), %rbp
       */
    addedBundleHeader = MBBI;
    resetRXBasedOnRZP(X86::RBP, MBB, MBBI, TII, DL);
    return true;
  }

  /*
      before:
       pop %rbp
      after:
       pop %rbp
       resetRXBasedOnRZP
  */
  if (Opc == X86::POP64r) {
    assert(MI.getOperand(0).getReg() == X86::RBP);
    addedBundleHeader = MBBI;
    resetRXBasedOnRZP(X86::RBP, MBB, MBBI, TII, DL);
    return true;
  }
  llvm_unreachable("Unhandled Frame SFI");

  return false;
}

bool X86SGXASLR::alignedControlNotToOCALL(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI) {
  MachineInstr &MI = *MBBI;

  if (!HasControlFlow(MI))
    return false;

  // Direct branches are OK
  if (IsDirectBranch(MI))
    return false;

  DebugLoc DL = MI.getDebugLoc();
  unsigned Opc = MI.getOpcode();

  // Align indirect jump/call instructions
  // + enforce Target Reg >= %R14 (i.e., OCALL boundary)
  switch (Opc) {
    case X86::JMP16r: case X86::JMP32r: case X86::CALL16r:
    case X86::CALL32r: case X86::TAILJMPr:
      llvm_unreachable("Jump target is not 64 bit");
    case X86::TAILJMPr64: case X86::JMP64r: case X86::CALL64r: {
      unsigned rX = MI.getOperand(0).getReg(); // target register
      unsigned rX32 = getX86SubSuperRegister(rX, 32, false);

      /*
         and  $-32, %rX32
         sub  %r14d, %rX32
         lea  (%rX, %r14, 1), %rX
         jmp  *%rX  --> MBBI
       */
      MachineInstr *andMI =
        BuildMI(MBB, MBBI, DL, TII->get(X86::AND32ri8), rX32).addReg(rX32).addImm(-32);
      MachineBasicBlock::iterator ANDI = andMI;
      BuildMI(MBB, MBBI, DL, TII->get(X86::SUB32rr), rX32).addReg(rX32).addReg(X86::R14D);
      BuildMI(MBB, MBBI, DL, TII->get(X86::LEA64r)).addReg(rX)
        .addReg(rX).addImm(1).addReg(X86::R14).addImm(0).addReg(0);

      MIBundleBuilder(MBB, ANDI, ++MBBI);
      finalizeBundle(MBB, ANDI.getInstrIterator());
      return true;
    }
    default: break;
  }

  // Align the return address
  // + enforce the return address >= %R14 (i.e., OCALL boundary)
  // XXX: do not handle complicated cases
  /*
    before:
       retq
    after:
       popq %r13
       and  $-32, %r13d
       sub  %r14d, %r13d
       lea  (%r13, %r14, 1), %r13
       jmpq *%r13
  if (Opc == X86::RETIL || Opc == X86::RETIQ || Opc == X86::RETL || Opc == X86::RETQ) {
    if (isEnclaveMain) {
      MI.eraseFromParent();
//      MachineFunction &MF = *MBB.getParent();
//      MachineInstr *jmpMI = BuildMI(MBB, MBBI, DL, TII->get(X86::JMP_4))
//        .addSym(MF.getContext().getOrCreateSymbol("enclave_exit"));
//      JMPI = jmpMI;
    } else {
      MachineInstr *popMI = BuildMI(MBB, MBBI, DL, TII->get(X86::POP64r)).addReg(X86::R13);
      MachineBasicBlock::iterator POPI = popMI;

      BuildMI(MBB, MBBI, DL, TII->get(X86::AND32ri8), X86::R13D).addReg(X86::R13D).addImm(-32);
      BuildMI(MBB, MBBI, DL, TII->get(X86::SUB32rr), X86::R13D).addReg(X86::R13D)
          .addReg(X86::R14D);
      BuildMI(MBB, MBBI, DL, TII->get(X86::LEA64r)).addReg(X86::R13)
        .addReg(X86::R13).addImm(1).addReg(X86::R14).addImm(0).addReg(0);
      MachineInstr *jmpMI = BuildMI(MBB, MBBI, DL, TII->get(X86::JMP64r)).addReg(X86::R13);
      MachineBasicBlock::iterator JMPI = jmpMI;

      MI.eraseFromParent();

      MIBundleBuilder(MBB, POPI, ++JMPI);
      finalizeBundle(MBB, POPI.getInstrIterator());
    }
    return true;
  }
   */
  // assert(!MI.isReturn() && "not handled RET");
  return false;
}

/*
 * Jaebaek: check if "test, cmp, sub"
 * TODO: This can cause bugs .. because of many false-positive
 */
static bool effectOnEFlags(const MachineInstr &MI) {
  if (MI.getDesc().isCompare()) return true;
  if (HasControlFlow(MI)) return false;
  unsigned Opc = MI.getOpcode();
  if (X86::MOV16ao16 <= Opc && Opc <= X86::MOVZX64rr8) return false;
  if (X86::LEA16r <= Opc && Opc <= X86::LEA64r) return false;
  if (X86::CMOVA16rm <= Opc && Opc <= X86::CMOV_V8I64) return false;
  if (X86::SETAEm <= Opc && Opc <= X86::SETSr) return false;
  if (Opc == X86::LOOPE || Opc == X86::LOOPNE) return false;
  /*
  if (X86::DEC16m <= Opc && Opc <= X86::DEC8r) return true;
  */
  return true;
}

static bool isCondInstr(const MachineInstr &MI) {
  if (MI.getDesc().isConditionalBranch()) return true;
  unsigned Opc = MI.getOpcode();
  if (X86::CMOVA16rm <= Opc && Opc <= X86::CMOV_V8I64) return true;
  if (X86::SETAEm <= Opc && Opc <= X86::SETSr) return true;
  if (Opc == X86::LOOPE || Opc == X86::LOOPNE) return true;
  return false;
}

/*
 * Jaebaek: The basis of this code is from pnacl-llvm project.
 */
bool X86SGXASLR::applySoftDEPandSFI(MachineBasicBlock &MBB) {
  bool Modified = false;
  /* BB-level aslr enforces loading each BB in an aligned address,
   * when .text section is aligned.
  if (MBB.hasAddressTaken()) {
    //FIXME: use a symbolic constant or get this value from some configuration
    MBB.setAlignment(5);
    Modified = true;
  }
  */
  unsigned i = 0;
  unsigned lastAddedBundleI = 0, lastEFlagsUpdateI = 0;
  for (MachineBasicBlock::iterator MBBI = MBB.begin(), NextMBBI = MBBI;
       MBBI != MBB.end(); MBBI = NextMBBI, ++i) {
    ++NextMBBI;

    modifiedWithoutAddedBundle = false;
    if (effectOnEFlags(*MBBI))
      lastEFlagsUpdateI = i;
    if (isCondInstr(*MBBI)) {
      /*
       * cmp const, %rX     <-- EFlags should be set here
       * ...
       * sub %r15, %rY      <-- EFlags changed!
       * mov %rW, disp(%r15, %rY, scale)
       * ...
       * jcc BasicBlock     <-- cond instr not working!
       */
      if (lastEFlagsUpdateI < lastAddedBundleI) {
        MachineBasicBlock::iterator MII = addedBundleHeader;
        --MII;
        DebugLoc DL = (*MII).getDebugLoc();
        BuildMI(MBB, MII, DL, TII->get(X86::PUSHF16));

        MII = addedBundleHeader;
        while (++MII != MBB.end() && (*MII).isInsideBundle());
        DL = (*MII).getDebugLoc();
        BuildMI(MBB, MII, DL, TII->get(X86::POPF16));

        lastAddedBundleI = 0; // <-- this enforce pushf / popf only once for sub
      }
    }

    if (ApplyRewrites(MBB, MBBI)                  ||
        // TODO: change call and ret as jmp
        // "X86::R14" should be used as Call Stack Pointer
        alignedControlNotToOCALL(MBB, MBBI)) {
      Modified = true;
    } else if (enforceRSPGreatOrEqualtoR15(MBB, MBBI)    ||
               enforceDSTMemGreatOrEqualtoR15(MBB, MBBI) ||
               enforceRBPGreatOrEqualtoR15(MBB, MBBI)) {
      if (!modifiedWithoutAddedBundle)
        lastAddedBundleI = i;
      Modified = true;
    }
  }
  return Modified;
}

/*
 * Jaebaek: applyASLR() makes all basic-blocks have UncondBr or Return or IndirectBr.
 * Interating only terminators:
 *  1. if there is UncondBr or Return or IndirectBr, do nothing.
 *  2. otherwise, if it is the last basic-block, add Return.
 *  3. otherwise, add UncondBr.
 */
bool X86SGXASLR::applyASLR(MachineFunction &MF, MachineFunction::iterator MFI)
{
  bool hasUncondBrOrRet = false;
  bool needUncondBr = false;
  bool modified = false;

  for (MachineBasicBlock::iterator I = MFI->getFirstTerminator(), E = MFI->end();
      I != E; ++I) {
    if (I->isUnconditionalBranch() || I->isReturn() || I->isIndirectBranch()) {
      hasUncondBrOrRet = true;
    }
    else if (I->isConditionalBranch() || !needUncondBr) {
      MachineBasicBlock::iterator J = I;
      do ++J;
      while(J != MFI->end()
          && J->isTerminator()
          && !J->isReturn()
          && !J->isUnconditionalBranch()
          && !J->isIndirectBranch());

      // fall through the next MBB --> add "JMP to the next MBB"
      if (J == MFI->end()) {
        // add Uncond Br at the end of the MBB
        needUncondBr = true;
      }
      // is there any such case? --> maybe not .. check out the MBB.terminators()
      else if (!J->isTerminator())
        assert(J->isTerminator());
    }
  }

  if (needUncondBr || !hasUncondBrOrRet) {
    MachineFunction::iterator nextMFI = MFI;
    ++nextMFI;

    // the end of the function
    if (nextMFI == MF.end())
      BuildMI(*MFI, MFI->end(),
          MFI->getLastNonDebugInstr()->getDebugLoc(), TII->get(X86::RETQ));
    else
      BuildMI(*MFI, MFI->end(),
          MFI->getLastNonDebugInstr()->getDebugLoc(), TII->get(X86::JMP_1))
        .addMBB(&*nextMFI);
    modified = true;
  }

#if DEBUG_X86SGXASLR
  // This is just for debugging
  printf("Terminators:");
  for (MachineBasicBlock::iterator I = MFI->getFirstTerminator(), E = MFI->end();
      I != E; ++I) {
    if (I->isUnconditionalBranch()) { printf(" Uncond"); }
    else if (I->isConditionalBranch()) { printf(" Cond"); }
    else if (I->isIndirectBranch()) printf(" IndirBr");
    else if (I->isReturn()) printf(" Return");
    else if (I->isCall()) printf(" Call");
    else printf(" %d", I->getOpcode());
  }
  printf("\n");
#endif

  return modified;
}

FunctionPass *llvm::createX86SGXASLR() { return new X86SGXASLR(); }
