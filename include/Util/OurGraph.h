#include "Util/UrGraph.h"

#ifndef OUR_GRAPH
#define OUR_GRAPH
/// @brief UrGraphOurMethod
struct CEOPinfo {
  CEOPinfo() {}
  CEOPinfo(std::map<unsigned, std::map<std::string, std::vector<CEOP>>> &ceops)
      : CEOPs(ceops) {}
  std::map<unsigned, std::map<std::string, std::vector<CEOP>>> CEOPs;
  // Must Data Access
  /// @brief  helper：ctxmidata(AbstractAddress)
  /// Ourmethod
  std::map<std::string, // exe_cnt
           std::map<CtxMI, std::vector<AccessInfo>>>
      entry2ctxmi2datainfo;

  /// @brief  helper：ctxmidata(AbstractAddress)
  /// Ourmethod
  /// FIXME 
  std::map<std::string,
           std::map<CtxMI, std::vector<TimingAnalysisPass::AbstractAddress>>>
      entry2ctxmi2data_absaddr;

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
};
class OurGraph : public UrGraph {
public:
  OurGraph(std::vector<std::vector<std::string>> &setc, CL_info &cl_infor,
           std::map<std::string, unsigned> &func2corenum1);

  CEOPinfo Ceopinfo;

  // PS Instr Access()
  std::map<std::string, std::map<CtxMI, PSAccessInfo>> ctxmi2ps_ai;
  // PS Data Access()
  std::map<std::string, std::map<CtxData, PSAccessInfo>> ctxdata2ps_ai;

private:
  // ctxmi_miaiCEOPs
  void write_miai_ceops();
  void getDataExeCntMust();
  // ===== Persistence analysis =====
  // 
  std::map<const llvm::MachineLoop *, TimingAnalysisPass::PersistenceScope>
      loop2ps_scope;
  /// helper: PS Scope？(AbsAddr) get loop
  /// stack InstrData
  std::map<const llvm::MachineLoop *,
           std::map<TimingAnalysisPass::AbstractAddress, bool>>
      loop2addr_isps;
  std::map<std::string,
           std::map<TimingAnalysisPass::dom::cache::Classification, 
           std::pair<unsigned, unsigned>>>
      instr_cl_cnt; // for output
  std::map<std::string,
           std::map<TimingAnalysisPass::dom::cache::Classification, 
           std::pair<unsigned, unsigned>>>
      data_cl_cnt; // for output

  /// CEOPloop_stack
  void build_loop_stack();
  /// @brief run()ctxmi2ps_loop_stack，CtxMIloop stack
  /// @param CM
  /// @return
  std::vector<std::pair<const llvm::MachineLoop *, bool>>
  getGlobalLoop(CtxMI CM, const CtxMI topCM);

  /// @brief run()ctxdata2ps_loop_stack，
  /// CtxMIloop stack
  /// @param CM
  /// @return
  std::vector<std::pair<const llvm::MachineLoop *, bool>>
  getGlobalLoopData(CtxData CD);

  /// @brief PS，
  unsigned getExeCntPSI(CtxMI CM);
  /// @brief PS，
  unsigned getExeCntPSD(CtxData CD);
  // ===== end Persistence analysis =====
  /// ，UR-CFGACLSummary
  void print_info(
  CEOPinfo &Ceopinfo);
  void print_our_cfg(unsigned core, const std::string &function);
  /// @brief ContextCtxMICallsites
  /// @param tmp_acl 
  /// @return 
  std::vector<const llvm::MachineInstr *> 
    ctx_match_helper(const AddrCL &tmp_acl, bool isInstr);
  /// @brief Ctxentry point
  /// @param tmp_acl 
  /// @param func2corenum1 
  /// @return ，
  std::tuple<std::string, int> get_entry_helper
    (const AddrCL &tmp_acl, std::map<std::string, unsigned> &func2corenum1);
  /// @brief loop_stackAddrPSList
  void check_loop_stack(CL_info &cl_infor);
};

#endif