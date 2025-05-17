#ifndef ZHANGWEI_M
#define ZHANGWEI_M

#include "Util/CLinfo.h"
#include "Util/Options.h"
#include "Util/OurGraph.h"
#include "Util/UrGraph.h"
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

#include "Util/Statistics.h"
#include "Util/Util.h"
#include "llvm/Support/FileSystem.h" // ur-cfg
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iostream>

class Zhangmethod {
public:
  Zhangmethod() {}
  Zhangmethod(OurGraph urgg, CL_info &cl_infor) {
    this->coreinfo = urgg.coreinfo;
    // this->CEOPs = urgg.Ceopinfo.CEOPs;
    this->Cl_infor = cl_infor;
    this->Ceopinfos = urgg.Ceopinfo; // Write，
  }
  // CoreNum -> vector of function
  std::vector<std::vector<std::string>> coreinfo;
  // Must Instr Access
  // std::map<unsigned, std::map<std::string, std::vector<CEOP>>> CEOPs;
  std::map<std::string, std::map<std::string, unsigned>> currWcetInter;
  std::map<unsigned, std::map<std::string, unsigned>> currWcetIntra;
  // data，
  CEOPinfo Ceopinfos;
  // PS info
  CL_info Cl_infor;

  /// helper: ，()
  std::vector<std::string> getInitConflictFunction(unsigned core,
                                                   const std::string &function);
  /// UR，WCEET
  void run(bool use_ps, bool use_data);

private:
  // 01,0,main; FIXME;
  // TODOcache
  unsigned getFValue(std::string localFunc, const CEOP &localPath,
                     unsigned localUR, std::string interFunc,
                     const CEOP &interPath, unsigned interUR, bool use_ps,
                     bool use_data);

  // helper function
  unsigned mi2cacheIndex(const llvm::MachineInstr *mi) {
    unsigned tmp_addr = TimingAnalysisPass::StaticAddrProvider->getAddr(mi);
    return (tmp_addr / L2linesize) % NN_SET;
    // line_size64byte，6offset；1024set，10index
  }
  unsigned getcacheIndex(unsigned addr) { return (addr / L2linesize) % NN_SET; }
  /// ps Ceopinfos.CEOPs 
  /// 
  void ps_Ipreprocess();
  /// @brief ps Ceopinfos.entry2ctxmi2datainfo 
  /// 
  void ps_Dpreprocess();
  void ps_preprocess();
  /// PS
  unsigned get_ps_execnt(std::string f_name, CtxMI tmp_cm);
  unsigned get_ps_execnt(std::string f_name, CtxData tmp_cm);
  /// @brief
  void print_mem_info();
  /// @brief 
  /// @param tmp_cm
  /// @return
  int is_l2ps(CtxMI tmp_cm, std::string f_name);
  int is_l2ps(CtxData tmp_cd, std::string f_name);
  /// ctxmiloop，loopvector
  std::map<
      std::string,
      std::map<CtxMI, std::vector<std::pair<const llvm::MachineLoop *, bool>>>>
      ctxmi2ps_loop_stack;

  /// ctxdataloop
  std::map<std::string,
           std::map<TimingAnalysisPass::AbstractAddress,
                    std::vector<std::pair<const llvm::MachineLoop *, bool>>>>
      ctxdata2ps_loop_stack;

  unsigned getCachelineAddress(unsigned addr) {
    return addr & ~(L2linesize - 1);
  }
  // PS triple
  int getPStriple(const CtxData &CtD, std::string fname) {
    int res = 0;
    // handling PS access
    // st
    auto it = ctxdata2ps_loop_stack[fname].find(CtD.data_addr);
    std::vector<std::pair<const llvm::MachineLoop *, bool>> &st = it->second;
    unsigned addr = CtD.data_addr.getAsInterval().lower();
    // 
    int x = 1; // 
    int b = 1;
    for (int i = st.size() - 1; i >= 0; --i) {
      std::pair<const llvm::MachineLoop *, bool> loop = st[i];
      x *= b;
      b = TimingAnalysisPass::LoopBoundInfo->GgetUpperLoopBound(loop.first);
      // 
      if (loop.second) {
        int CS = INT_MAX;
        st[i].second = false;
        TimingAnalysisPass::dom::cache::Classification cl;
        for (auto &scop : Cl_infor.AddrPSList) {
          if (scop.first.loop == loop.first) {
            for (const AddrPS &ps : scop.second) {
              if (ps.address.getAsInterval().lower() ==
                  getCachelineAddress(addr)) {
                if (ps.LEVEL == 1) {
                  cl = TimingAnalysisPass::dom::cache::CL_PS;
                } else if (ps.LEVEL == 2) {
                  cl = TimingAnalysisPass::dom::cache::CL2_PS;
                }
                CS = ps.CS_size;
                break;
              }
            }
            break;
          }
        }

        assert(CS != INT_MAX && "ERR");
        if (cl == TimingAnalysisPass::dom::cache::CL2_PS) {
          assert(b != -1);
          if (b > 1) { // 1
            res += x * (b - 1);
          }
        }
      }
    }
    return res;
  }

  int getPStriple(const CtxMI &MI, std::string fname) {
    int res = 0;
    auto it = ctxmi2ps_loop_stack[fname].find(MI);
    std::vector<std::pair<const llvm::MachineLoop *, bool>> &st = it->second;
    unsigned addr = TimingAnalysisPass::StaticAddrProvider->getAddr(MI.MI);
    // 
    int x = 1; // 
    int b = 1;
    for (int i = st.size() - 1; i >= 0; --i) {
      std::pair<const llvm::MachineLoop *, bool> loop = st[i];
      x *= b;
      int b = TimingAnalysisPass::LoopBoundInfo->GgetUpperLoopBound(loop.first);
      // 
      if (loop.second) {
        st[i].second = false; // 
        int CS = INT_MAX;
        TimingAnalysisPass::dom::cache::Classification cl;
        for (auto &scop : Cl_infor.AddrPSList) {
          if (scop.first.loop == loop.first) {
            for (const AddrPS &ps : scop.second) {
              if (ps.address.getAsInterval().lower() ==
                  getCachelineAddress(addr)) {
                if (ps.LEVEL == 1) {
                  cl = TimingAnalysisPass::dom::cache::CL_PS;
                } else if (ps.LEVEL == 2) {
                  cl = TimingAnalysisPass::dom::cache::CL2_PS;
                }
                CS = ps.CS_size; // 1
                break;
              }
            }
            break;
          }
        }

        assert(CS != INT_MAX && "ERR");
        if (cl == TimingAnalysisPass::dom::cache::CL2_PS) {
          assert(b != -1);
          if (b > 1) { // 1
            res += x * (b - 1);
          }
        }
      }
    }
    return res;
  }
};

#endif