#ifndef UR_GRAPH
#define UR_GRAPH

#include "Util/CLinfo.h"
#include "Util/Options.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "LLVMPasses/MachineFunctionCollector.h" // 
#include "LLVMPasses/StaticAddressProvider.h"    // mi -> addr
// #include "LLVMPasses/DispatchMemory.h" // cacheconfig

#include "Memory/Classification.h" // CL_MISS/UNKONWN/HIT
#include "PathAnalysis/LoopBoundInfo.h"

#include "Util/Util.h"
#include "llvm/Support/FileSystem.h" // ur-cfg
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iostream>
/*
  MI + ，UR-CFGMI
*/

class CtxMI {
public:
  const llvm::MachineInstr *MI;
  std::vector<const llvm::MachineInstr *> CallSites;
  CtxMI() { MI = nullptr; }

  bool operator==(const CtxMI &other) const {
    return MI == other.MI && CallSites == other.CallSites;
  }
  bool operator!=(const CtxMI &other) const { return !(*this == other); }
  bool operator<(const CtxMI &other) const { // map
    if (MI != other.MI) {
      return MI < other.MI;
    }
    return CallSites < other.CallSites;
  }
  friend std::ostream &operator<<(std::ostream &os, const CtxMI &tmpCM) {
    os << "[" << std::endl;
    unsigned tmpAddr =
        TimingAnalysisPass::StaticAddrProvider->getAddr(tmpCM.MI);
    os << "MI's Addr:";
    TimingAnalysisPass::printHex(os, tmpAddr);
    os << "_" << TimingAnalysisPass::getMachineInstrIdentifier(tmpCM.MI);
    os << std::endl;
    os << "With Call Context:" << std::endl;
    for (auto &tmpCS : tmpCM.CallSites) {
      os << "  " << TimingAnalysisPass::getMachineInstrIdentifier(tmpCS)
         << std::endl;
    }
    os << "]" << std::endl;
    return os;
  }
  std::vector<CtxMI> getSucc() {
    std::vector<CtxMI> retSucc;

    // ，
    assert((!MI->isTransient()) && "We should not see transient instr here");

    // call  return()
    auto &cg = TimingAnalysisPass::CallGraph::getGraph();
    if (MI->isCall() && !cg.callsExternal(MI)) {
      for (const auto *callee : cg.getPotentialCallees(MI)) {
        const MachineBasicBlock *calleeBeginBB = &*callee->begin();
        const llvm::MachineInstr *calleeBeginMI = &(calleeBeginBB->front());
        // assert((!calleeBeginMI->isTransient()) &&
        //        "First Instr of func shouldn't be transient");
        if (calleeBeginMI->isTransient()) {
          auto tmp_pair = transientHelper(calleeBeginBB, calleeBeginMI);
          calleeBeginMI = tmp_pair.second;
        }
        CtxMI succCM; // ，
        succCM.MI = calleeBeginMI;
        assert(CallSites == this->CallSites && "Just Want to make sure");
        succCM.CallSites = CallSites;
        succCM.CallSites.push_back(MI);
        retSucc.push_back(succCM);
      }
      return retSucc;
    }
    if (MI->isReturn()) {
      if (CallSites.size() > 0) { // main
        const llvm::MachineInstr *callsite = CallSites.back();
        const auto *callsiteMBB = callsite->getParent(); // call
        // call(callBasic Block)
        auto it = std::find_if(callsiteMBB->begin(), callsiteMBB->end(),
                               [callsite](const MachineInstr &Instr) {
                                 return &Instr == callsite;
                               });
        if (it != callsiteMBB->end() &&
            std::next(it) !=
                callsiteMBB->end()) { // MBB
          const llvm::MachineInstr *targetMI = &*(std::next(it));
          // 。transientBB
          if (targetMI->isTransient()) {
            auto tmp_pair = transientHelper(callsiteMBB, targetMI);
            targetMI = tmp_pair.second;
          }
          CtxMI succCM; // 
          succCM.MI = targetMI;
          succCM.CallSites = CallSites;
          succCM.CallSites.pop_back();
          retSucc.push_back(succCM);
        } else {
          assert(0 && "This will not happend");
        }
      }
      return retSucc;
    }

    const llvm::MachineBasicBlock *MBB = MI->getParent();
    // ，MBBMI，MBBMIMBBMI
    if (MI == &(MBB->back())) {
      for (auto succit = MBB->succ_begin(); succit != MBB->succ_end();
           ++succit) {
        const llvm::MachineBasicBlock *targetMBB = *succit;
        const llvm::MachineInstr *targetMI = &(targetMBB->front());
        if (targetMI->isTransient()) {
          auto tmp_pair = transientHelper(targetMBB, targetMI);
          targetMI = tmp_pair.second;
        }
        CtxMI succCM;
        succCM.MI = targetMI;
        succCM.CallSites = CallSites;
        retSucc.push_back(succCM);
      }
    } else {
      const llvm::MachineInstr *tmpMI = MI;
      auto it = std::find_if(
          MBB->begin(), MBB->end(),
          [tmpMI](const MachineInstr &Instr) { return &Instr == tmpMI; });
      if (it != MBB->end() && std::next(it) != MBB->end()) {
        const llvm::MachineInstr *targetMI = &*(std::next(it));
        // 
        // transientBB
        if (targetMI->isTransient()) {
          auto tmp_pair = transientHelper(MBB, targetMI);
          targetMI = tmp_pair.second;
        }
        CtxMI succCM;
        succCM.MI = targetMI;
        succCM.CallSites = CallSites;
        retSucc.push_back(succCM);
      }
    }
    return retSucc;
  }

  std::vector<CtxMI> getSuccWithTransient() { // useless
    std::vector<CtxMI> retSucc;
    // call  return()
    auto &cg = TimingAnalysisPass::CallGraph::getGraph();
    if (MI->isCall() && !cg.callsExternal(MI)) {
      for (const auto *callee : cg.getPotentialCallees(MI)) {
        const MachineBasicBlock *calleeBeginBB = &*callee->begin();
        const llvm::MachineInstr *calleeBeginMI = &(calleeBeginBB->front());
        CtxMI succCM; // ，
        succCM.MI = calleeBeginMI;
        assert(CallSites == this->CallSites && "Just Want to make sure");
        succCM.CallSites = CallSites;
        succCM.CallSites.push_back(MI);
        retSucc.push_back(succCM);
      }
      return retSucc;
    }
    if (MI->isReturn()) {
      if (CallSites.size() > 0) { // main
        const llvm::MachineInstr *callsite = CallSites.back();
        const auto *callsiteMBB = callsite->getParent(); // call
        // call(callBasic Block)
        auto it = std::find_if(callsiteMBB->begin(), callsiteMBB->end(),
                               [callsite](const MachineInstr &Instr) {
                                 return &Instr == callsite;
                               });
        if (it != callsiteMBB->end() &&
            std::next(it) !=
                callsiteMBB->end()) { // MBB
          const llvm::MachineInstr *targetMI = &*(std::next(it));
          CtxMI succCM; // 
          succCM.MI = targetMI;
          succCM.CallSites = CallSites;
          succCM.CallSites.pop_back();
          retSucc.push_back(succCM);
        } else {
          assert(0 && "This will not happend");
        }
      }
      return retSucc;
    }

    const llvm::MachineBasicBlock *MBB = MI->getParent();
    // ，MBBMI，MBBMIMBBMI
    if (MI == &(MBB->back())) {
      for (auto succit = MBB->succ_begin(); succit != MBB->succ_end();
           ++succit) {
        const llvm::MachineBasicBlock *targetMBB = *succit;
        const llvm::MachineInstr *targetMI = &(targetMBB->front());
        CtxMI succCM;
        succCM.MI = targetMI;
        succCM.CallSites = CallSites;
        retSucc.push_back(succCM);
      }
    } else {
      const llvm::MachineInstr *tmpMI = MI;
      auto it = std::find_if(
          MBB->begin(), MBB->end(),
          [tmpMI](const MachineInstr &Instr) { return &Instr == tmpMI; });
      if (it != MBB->end() && std::next(it) != MBB->end()) {
        const llvm::MachineInstr *targetMI = &*(std::next(it));
        CtxMI succCM;
        succCM.MI = targetMI;
        succCM.CallSites = CallSites;
        retSucc.push_back(succCM);
      }
    }
    return retSucc;
  }

private:
  // transienttransient()
  // transient
  std::pair<const llvm::MachineBasicBlock *, const llvm::MachineInstr *>
  transientHelper(const llvm::MachineBasicBlock *MBB,
                  const llvm::MachineInstr *MI) {
    if (!MI->isTransient()) {
      return std::make_pair(MBB, MI);
    }
    assert((MI != &(MBB->back())) &&
           "I assume transient instr isn't last instr of bb, but this "
           "indicates I am wrong");
    auto it =
        std::find_if(MBB->begin(), MBB->end(),
                     [MI](const MachineInstr &Instr) { return &Instr == MI; });
    if (it != MBB->end() && std::next(it) != MBB->end()) {
      const llvm::MachineInstr *targetMI = &*(std::next(it));
      return transientHelper(MBB, targetMI);
    }
  }
};

class CtxData {
public:
  CtxMI ctx_mi;
  TimingAnalysisPass::AbstractAddress data_addr;
  CtxData() : data_addr(TimingAnalysisPass::AbstractAddress(unsigned(0))) {}
  CtxData(CtxMI ctx_i, TimingAnalysisPass::AbstractAddress daddr)
      : ctx_mi(ctx_i), data_addr(daddr) {}
  bool operator==(const CtxData &other) const {
    return ctx_mi == other.ctx_mi && data_addr == other.data_addr;
  }
  bool operator!=(const CtxData &other) const { return !(*this == other); }
  bool operator<(const CtxData &other) const { // map
    if (ctx_mi != other.ctx_mi) {
      return ctx_mi < other.ctx_mi;
    }
    return data_addr < other.data_addr;
  }
};

class AccessInfo {
public:
  unsigned x; // 
  TimingAnalysisPass::dom::cache::Classification classification;
  int age;
  TimingAnalysisPass::AbstractAddress data_addr; // only for data access
  AccessInfo()
      : x(1), classification(TimingAnalysisPass::dom::cache::CL_BOT),
        age(INT_MAX),
        data_addr(TimingAnalysisPass::AbstractAddress((unsigned)0)) {}
  friend std::ostream &operator<<(std::ostream &os, const AccessInfo &AI) {
    os << "[addr_0x" << std::hex << AI.data_addr << "_cl_" << AI.classification
       << "_age_" << AI.age << "_execnt_" << AI.x << "]\n";
    return os;
  }
};

struct PSAccessInfo {
  unsigned exe_cnt;
  int cs_size;
  unsigned address;
  PSAccessInfo() : exe_cnt(0), cs_size(INT_MAX), address(0) {}
};

///  ZW paper f
class UnorderedRegion {
public:
  std::map<CtxMI, AccessInfo> mi2xclass;
};

class CEOP {
public:
  std::vector<UnorderedRegion> URs; // UR()
  // ，UR
};

/// UR-CFG，CEOPs，MI，AccessInfo
class UrGraph {
public:
  UrGraph(std::vector<std::vector<std::string>> &setc);

  std::vector<std::vector<std::string>> coreinfo;
  // ceop
  void handsome_ceop_instr();

protected:
  // ===  ===
  // core, function, ctxmi -> xclass
  std::map<unsigned, std::map<std::string, std::map<CtxMI, AccessInfo>>>
      ctxmi_miai;
  std::map<unsigned, std::map<std::string, std::vector<CEOP>>> &getCEOP() {
    return CEOPs;
  }
private:
  // Must Instr Access
  std::map<unsigned, std::map<std::string,
                              std::vector<CEOP>>>
      CEOPs; // taskCEOP(set，)
  /// ctxmi_miai
  void getExeCntMust();
  /*
      ，ok，loop，BBloop
    loop；，Basic
    Block
      ：CM，token，，callsite
    。。
      local，loop，BB。
  */
  unsigned getGlobalUpBd(std::string entry, CtxMI CM);
  unsigned bd_helper1(std::string entry, const llvm::MachineBasicBlock *MBB,
                      const llvm::MachineLoop *Loop);
  unsigned bd_helper2(std::string entry, const llvm::MachineLoop *Loop);
  // === tarjan ===
  // MachineLoop，MBB，；
  // LoopBoundInfoPass
  // std::map<std::string, MachineLoopInfo> f2MLI;
  // ，
  // module1: taskUR，oi-wiki tarjan
  std::map<CtxMI, unsigned> dfn;
  std::map<CtxMI, unsigned> low;
  unsigned dfncnt;
  std::map<unsigned, CtxMI> ur_stack;
  std::map<CtxMI, unsigned> in_stack;
  unsigned stack_pt;

protected:
  std::map<CtxMI, unsigned> mi_ur; // MIur_id
private:
  std::map<unsigned, unsigned> ur_size; // MI
  unsigned ur_id; // ，
  // module2: ，URURMI
  std::map<unsigned, std::vector<unsigned>> ur_graph; // 
  std::map<unsigned, std::vector<CtxMI>> ur_mi; // urMI

  // module3： module3module
  std::map<unsigned,
           std::map<std::string,
                    std::map<const llvm::MachineInstr *,
                             TimingAnalysisPass::dom::cache::Classification>>>
      mi_class; // aborted
  // module: CEOP
  std::vector<UnorderedRegion> tmpPath; // URDFSPATH
  std::vector<CEOP> tmpCEOPs;           // task
  std::map<CtxMI, std::vector<CtxMI>> mi_cfg;

protected:
  // MI-CFG，debug
  std::map<std::string, std::map<CtxMI, std::vector<CtxMI>>> mi_cfgs;

private:
  /// URCEOP
  void URCalculation(unsigned core, const std::string &function);
  // helper, CEOPtmpCEOPs
  void ceopDfs(unsigned u, unsigned &cur_core, std::string &cur_func);
  void ceopDfs_it(unsigned u, unsigned &cur_core, std::string &cur_func);
  // CEOPUR，URAccessInfo
  void collectCEOPInfo(CtxMI firstCM, unsigned core, std::string function);
  // UR -> (、 MI);UR
  void collectUrInfo();
  void write_mi_cfgs(const std::string &function);
  void print_mi_cfg(unsigned core, const std::string &function);

  // MI-CFG
  void tarjan(CtxMI CM);
  void tarjan_iterative(CtxMI CM);
  void tarjan_it(CtxMI CM);
};

#endif