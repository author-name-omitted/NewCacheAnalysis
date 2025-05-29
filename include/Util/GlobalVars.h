#ifndef __G10bAL_vARs__
#define __G10bAL_vARs__

#include "CLinfo.h"
#include "Lianginfo.h"
#include "PreprocessingAnalysis/AddressInformation.h"
#include "Zhangmethod.h"
#include "muticoreinfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include <string>
#include <vector>

extern Multicoreinfo mcif;
// extern std::vector<std::string> conflicFunctions;
extern bool isBCET;
extern int IMISS;
extern int DMISS;
extern int STBUS;
extern int L2MISS;
extern int BOUND;

extern CL_info cl_info;
extern Liangy_info ly_info;
extern std::map<std::string, TimingAnalysisPass::LoopBoundInfoPass*>
    ALLLoopBoundInfo;
// extern Zhangmethod ZW_mth;
//////////////////////////////////////////////////////////////////
// 
// extern std::set<const MachineBasicBlock *> mylist;

// extern TimingAnalysisPass::AddressInformation *glAddrInfo;

// unsigned getbound(const MachineBasicBlock *MBB,
//                   const TimingAnalysisPass::Context &ctx);

// void celectaddr(const MachineBasicBlock *MBB,
//                 const TimingAnalysisPass::Context &ctx);
/* ， */

#endif