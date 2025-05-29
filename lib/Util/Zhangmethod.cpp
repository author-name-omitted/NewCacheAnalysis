#include "Util/Zhangmethod.h"
#include "Memory/Classification.h"
#include "Util/UrGraph.h"
#include "llvm/Analysis/LoopInfo.h"
#include <vector>

// use_ps flag
void Zhangmethod::run(bool use_ps, bool use_data) {
  // use_ps，ps_Ipreprocess
  // if (use_ps) {
  //   ps_Ipreprocess();
  // }
  // if (use_ps && use_data) {
  //   // PS
  //   ps_Dpreprocess();
  // }
  ps_preprocess();

  if (ZWDebug) {
    print_mem_info();
  }
  // 
  outs() << " -> WCET Inter Analysis start\n";
  for (unsigned local = 0; local < CoreNums; ++local) {
    for (std::string &localFunc : coreinfo[local]) {
      // task
      unsigned wceetOfTSum = 0;
      for (unsigned inter = 0; inter < CoreNums; ++inter) { // interference
        if (local == inter)
          continue;
        for (std::string &interFunc :
             getInitConflictFunction(local, localFunc)) {
          // Init，
          // ，getInitConflictFunction
          // task
          TimingAnalysisPass::Statistics &Stats =
              TimingAnalysisPass::Statistics::getInstance();
          Stats.startMeasurement(std::to_string(local) + "_" + localFunc + "_" +
                                 std::to_string(inter) + "_" + interFunc +
                                 "_inter");
          unsigned wceetOf2T = 0; // TaskWCEET
          int inter1=inter;
          if(localFunc==interFunc){
            inter1=local;
          }
          for (const CEOP &localP : Ceopinfos.CEOPs[local][localFunc]) {
            for (const CEOP &interP : Ceopinfos.CEOPs[inter1][interFunc]) {
              // Path, dp，、
              unsigned localPLen = localP.URs.size();
              unsigned interPLen = interP.URs.size();
              unsigned ArvVal[localPLen][interPLen] = {0};

              ArvVal[0][0] = getFValue(localFunc, localP, 0, interFunc, interP,
                                       0, use_ps, use_data);
              for (unsigned i = 1; i < localPLen; i++) {
                ArvVal[i][0] =
                    getFValue(localFunc, localP, i, interFunc, interP, 0,
                              use_ps, use_data) +
                    ArvVal[i - 1][0]; // paper，
              }
              for (unsigned i = 1; i < interPLen; i++) {
                ArvVal[0][i] = getFValue(localFunc, localP, 0, interFunc,
                                         interP, i, use_ps, use_data) +
                               ArvVal[0][i - 1];
              }
              for (unsigned i = 1; i < localPLen; i++) {
                for (unsigned j = 1; j < interPLen; j++) {
                  ArvVal[i][j] = std::max(ArvVal[i - 1][j], ArvVal[i][j - 1]) +
                                 getFValue(localFunc, localP, i, interFunc,
                                           interP, j, use_ps, use_data);
                }
              }
              wceetOf2T =
                  std::max(wceetOf2T, ArvVal[localPLen - 1][interPLen - 1]);
              std::ofstream myfile;
              std::string fileName = "ZW_Output.txt";
              myfile.open(fileName, std::ios_base::app);
              for (int i = 0; i < localPLen; i++) {
                for (int j = 0; j < interPLen; j++) {
                  myfile << ArvVal[i][j] << " ";
                }
                myfile << std::endl; // 
              }
              myfile.close();
            }
          }
          Stats.stopMeasurement(std::to_string(local) + "_" + localFunc + "_" +
                                std::to_string(inter) + "_" + interFunc +
                                "_inter");
          wceetOf2T *=
              Latency; // BG Mem from Command Line, 
          wceetOfTSum += wceetOf2T; // 
          if (currWcetInter[localFunc].count(interFunc)==0){
            currWcetInter[localFunc][interFunc] = wceetOfTSum;
          }
        }
      }
      outs() << "Core-" << local << " Func:" << localFunc << " 's WCEET is "
             << wceetOfTSum << "\n";
    }
  }
  // WCET_{sum} = WCET_{intra} + WCEET
  // std::ofstream myfile;
  // std::string fileName = "ZW_Output.txt";
  // myfile.open(fileName, std::ios_base::app);
  std::ofstream myfile1;
  std::string fileName1 = "Result.txt";
  myfile1.open(fileName1, std::ios_base::app);
  for (unsigned local = 0; local < CoreNums; ++local) {
    for (std::string &localFunc : coreinfo[local]) {
      for (unsigned inter = 0; inter < CoreNums; ++inter) {
        if (local == inter)
          continue;
        for (std::string &interFunc : coreinfo[inter]) {
          unsigned wcet_intra = currWcetIntra[local][localFunc];
          unsigned wceet = currWcetInter[localFunc][interFunc];
          // myfile << "Core-" << local << " F-" << localFunc
          //        << " intra:" << wcet_intra << " wceet:" << wceet <<
          //        std::endl;
          myfile1 << localFunc << " inter " << interFunc << " " << wceet
                  << std::endl;
        }
      }
    }
  }
  // myfile.close();
  myfile1.close();
}

void Zhangmethod::print_mem_info() {
  std::map<std::string, std::map<TimingAnalysisPass::dom::cache::Classification,
                                 std::pair<unsigned, unsigned>>>
      instr_cl_cnt; // for output(second)
  std::map<std::string, std::map<TimingAnalysisPass::dom::cache::Classification,
                                 std::pair<unsigned, unsigned>>>
      data_cl_cnt; // for output
  for (auto &tmp_core : Ceopinfos.CEOPs) {
    unsigned core_num = tmp_core.first;
    for (auto &tmp_task : tmp_core.second) {
      std::string f_name = tmp_task.first;
      for (auto &tmp_ceop : tmp_task.second) {
        for (auto &tmp_ur : tmp_ceop.URs) {
          for (auto &tmp_pair : tmp_ur.mi2xclass) {
            const CtxMI &tmp_cm = tmp_pair.first;
            instr_cl_cnt[f_name][tmp_pair.second.classification].first +=
                tmp_pair.second.x;
            instr_cl_cnt[f_name][tmp_pair.second.classification].second +=
                1;
            for (auto &tmp_ai :
                 Ceopinfos.entry2ctxmi2datainfo[f_name][tmp_cm]) {
              data_cl_cnt[f_name][tmp_ai.classification].first += tmp_ai.x;
              data_cl_cnt[f_name][tmp_ai.classification].second += 1;
            }
          }
        }
      }
    }
  }
  // fixme,LATENCY=1，BOTL1HIT
  for (const auto &clmap : instr_cl_cnt) {
    instr_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_HIT].first +=
      instr_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_BOT].first;
    instr_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_BOT].first = 0;
        instr_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_HIT].second +=
      instr_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_BOT].second;
    instr_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_BOT].second = 0;
  }
  for (const auto &clmap : data_cl_cnt) {
    data_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_HIT].first +=
      data_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_BOT].first;
    data_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_BOT].first = 0;
    data_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_HIT].second +=
      data_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_BOT].second;
    data_cl_cnt[clmap.first][TimingAnalysisPass::dom::cache::CL_BOT].second = 0;
  }

  std::ofstream Myfile;
  Myfile.open("RWInfo.txt", std::ios_base::app);
  for (const auto &clmap : instr_cl_cnt) {
    for (const auto &cl_pair : clmap.second) {
      Myfile << clmap.first << " instr " << cl_pair.first << " "
             << cl_pair.second.first << "\n";
    }
  }
  for (const auto &clmap : data_cl_cnt) {
    for (const auto &cl_pair : clmap.second) {
      Myfile << clmap.first << " data " << cl_pair.first << " "
             << cl_pair.second.first << "\n";
    }
  }
  Myfile.close();

  Myfile.open("RWInfo_u.txt", std::ios_base::app);
  for (const auto &clmap : instr_cl_cnt) {
    for (const auto &cl_pair : clmap.second) {
      Myfile << clmap.first << " instr " << cl_pair.first << " "
             << cl_pair.second.second << "\n";
    }
  }
  for (const auto &clmap : data_cl_cnt) {
    for (const auto &cl_pair : clmap.second) {
      Myfile << clmap.first << " data " << cl_pair.first << " "
             << cl_pair.second.second << "\n";
    }
  }
  Myfile.close();
}

void Zhangmethod::ps_Ipreprocess() {
  // ：PSCeopinfos.CEOPs
  // ，Ceopinfoloopstack
  //  (，x-1，，loop)
  // ，Cl_infor.AddrPSList，L2_PS
  for (auto &tmp_core : Ceopinfos.CEOPs) {
    unsigned core_num = tmp_core.first;
    for (auto &tmp_task : tmp_core.second) {
      std::string f_name = tmp_task.first;
      for (auto &tmp_ceop : tmp_task.second) {
        for (auto &tmp_ur : tmp_ceop.URs) {
          for (auto &tmp_pair : tmp_ur.mi2xclass) {
            const CtxMI &tmp_cm = tmp_pair.first;
            if (Ceopinfos.ctxmi2ps_loop_stack[f_name][tmp_cm].size() == 0) {
              continue;
            }
            // const llvm::MachineLoop *tmp_loop =
            //     Ceopinfos.ctxmi2ps_loop_stack[f_name][tmp_cm][0].first;
            if (tmp_pair.second.classification ==
                TimingAnalysisPass::dom::cache::CL2_MISS) {
              int lb = getPStriple(tmp_cm, f_name);
              if (lb > 0 && tmp_pair.second.x > 1) {
                tmp_pair.second.classification =
                    TimingAnalysisPass::dom::cache::CL2_PS;
                // 
                tmp_pair.second.x = lb;
              }
            }
          }
        }
      }
    }
  }
}
void Zhangmethod::ps_preprocess() {
  // ：PSCeopinfos.CEOPs
  // ，Ceopinfoloopstack
  //  (，x-1，，loop)
  // ，Cl_infor.AddrPSList，L2_PS
  for (auto &tmp_core : Ceopinfos.CEOPs) {
    unsigned core_num = tmp_core.first;
    for (auto &tmp_task : tmp_core.second) {
      std::string f_name = tmp_task.first;
      for (auto &tmp_ceop : tmp_task.second) {
        // 
        ctxmi2ps_loop_stack = Ceopinfos.ctxmi2ps_loop_stack;
        ctxdata2ps_loop_stack = Ceopinfos.ctxdata2ps_loop_stack;
        for (auto &tmp_ur : tmp_ceop.URs) {
          for (auto &tmp_pair : tmp_ur.mi2xclass) {
            const CtxMI &tmp_cm = tmp_pair.first;
            if (ctxmi2ps_loop_stack[f_name][tmp_cm].size() != 0) {
              if (tmp_pair.second.classification ==
                  TimingAnalysisPass::dom::cache::CL2_MISS) {
                int lb = getPStriple(tmp_cm, f_name);
                if (lb > 0 && tmp_pair.second.x > 1) {
                  tmp_pair.second.classification =
                      TimingAnalysisPass::dom::cache::CL2_PS;
                  // 
                  tmp_pair.second.x = lb;
                }
              }
            }
            if (Ceopinfos.entry2ctxmi2datainfo[f_name].find(tmp_cm) !=
                Ceopinfos.entry2ctxmi2datainfo[f_name].end()) {
              for (auto &tmp_ai :
                   Ceopinfos.entry2ctxmi2datainfo[f_name][tmp_cm]) {
                if (tmp_ai.data_addr.isPrecise()) { // precise
                  CtxData tmp_cd(tmp_cm, tmp_ai.data_addr);
                  if (ctxdata2ps_loop_stack[f_name][tmp_ai.data_addr].size() ==
                      0) {
                    continue;
                  }
                  if (tmp_ai.classification ==
                      TimingAnalysisPass::dom::cache::CL2_MISS) {
                    int lb = getPStriple(tmp_cd, f_name);
                    if (lb > 0 && tmp_ai.x > 1) {
                      tmp_ai.classification =
                          TimingAnalysisPass::dom::cache::CL2_PS;
                      // 
                      tmp_ai.x = lb;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void Zhangmethod::ps_Dpreprocess() {
  for (auto &tmp_entry : Ceopinfos.entry2ctxmi2datainfo) {
    std::string f_name = tmp_entry.first;
    for (auto &tmp_cm_pair : tmp_entry.second) {
      CtxMI tmp_cm = tmp_cm_pair.first;
      for (auto &tmp_ai : tmp_cm_pair.second) {
        if (tmp_ai.data_addr.isPrecise()) { // precise
          CtxData tmp_cd(tmp_cm, tmp_ai.data_addr);
          if (Ceopinfos.ctxdata2ps_loop_stack[f_name][tmp_ai.data_addr]
                  .size() == 0) {
            continue;
          }
          // const llvm::MachineLoop *tmp_loop =
          //     Ceopinfos.ctxdata2ps_loop_stack[f_name][tmp_ai.data_addr][0]
          //         .first;
          if (tmp_ai.classification ==
              TimingAnalysisPass::dom::cache::CL2_MISS) {
            int lb = getPStriple(tmp_cd, f_name);
            if (lb > 0 && tmp_ai.x > 1) {
              tmp_ai.classification = TimingAnalysisPass::dom::cache::CL2_PS;
              // 
              tmp_ai.x = lb;
            }
          }
        }
      }
    }
  }
}

unsigned Zhangmethod::get_ps_execnt(std::string f_name, CtxMI tmp_cm) {
  bool reach_not_ps = false;
  unsigned ps_x = 1;
  unsigned not_ps_x = 1;
  auto &st = Ceopinfos.ctxmi2ps_loop_stack[f_name][tmp_cm];
  for (int i = st.size() - 1; i >= 0; i--) {
    auto loop = st[i];
    if (loop.second) {
      int b = TimingAnalysisPass::LoopBoundInfo->GgetUpperLoopBound(loop.first);
    }
  }
  return (ps_x - 1) * not_ps_x;
}
unsigned Zhangmethod::get_ps_execnt(std::string f_name, CtxData tmp_cm) {
  bool reach_not_ps = false;
  unsigned ps_x = 1;
  unsigned not_ps_x = 1;
  for (auto &loop : Ceopinfos.ctxdata2ps_loop_stack[f_name][tmp_cm.data_addr]) {
    if (!loop.second) {
      reach_not_ps = true;
      loop.second = false; // 
    }
    int b = TimingAnalysisPass::LoopBoundInfo->GgetUpperLoopBound(loop.first);
    assert(b > 0);
    if (!reach_not_ps) {
      ps_x *= (unsigned)b;
    } else {
      not_ps_x *= (unsigned)b;
    }
  }
  return (ps_x - 1) * not_ps_x;
}

int Zhangmethod::is_l2ps(CtxMI tmp_cm, std::string f_name) {
  unsigned addr = TimingAnalysisPass::StaticAddrProvider->getAddr(tmp_cm.MI);
  std::vector<int> loopb;
  int l2ps = -1;
  for (auto &scop : Cl_infor.fun2scope[f_name]) {
    for (const AddrPS &ps : Cl_infor.AddrPSList[scop]) {
      if (ps.address.getAsInterval().lower() == addr) {
        loopb.emplace_back(
            TimingAnalysisPass::LoopBoundInfo->GgetUpperLoopBound(scop.loop));
        if (ps.LEVEL == 2) {
          l2ps = loopb.size() - 1;
        }
      }
    }
  }
  int res = 1;
  int b = 1;
  for (int j = 0; j < l2ps; j++) {
    res *= loopb[j];
  }
  if (l2ps > 0) {
    b = loopb[l2ps];
  }
  return res * (b - 1);
}

// Is this
// correct?？AddrPSoperator<，
int Zhangmethod::is_l2ps(CtxData tmp_cd, std::string f_name) {
  std::vector<int> loopb;
  int l2ps = -1;
  unsigned addr = tmp_cd.data_addr.getAsInterval().lower();
  std::vector<TimingAnalysisPass::PersistenceScope> remo;
  AddrPS copy;
  for (auto &scop : Cl_infor.fun2scope[f_name]) {
    for (const AddrPS &ps : Cl_infor.AddrPSList[scop]) {
      if (ps.address == tmp_cd.data_addr) {
        copy = ps;
        loopb.emplace_back(
            TimingAnalysisPass::LoopBoundInfo->GgetUpperLoopBound(scop.loop));
        if (ps.LEVEL == 2) {
          l2ps = loopb.size() - 1;
        }
      }
    }
  }
  int res = 1;
  int b = 1;
  for (int j = 0; j < l2ps; j++) {
    res *= loopb[j];
  }
  if (l2ps > 0) {
    b = loopb[l2ps];
  }
  for (auto &scop : remo) {
    Cl_infor.AddrPSList[scop].erase(copy); // 
  }
  return res * (b - 1);
}

std::vector<std::string>
Zhangmethod::getInitConflictFunction(unsigned core,
                                     const std::string &function) {
  // 
  std::vector<std::string> list;
  for (int i = 0; i < coreinfo.size(); i++) {
    if (i == core) {
      continue;
    }
    for (int j = 0; j < coreinfo[i].size(); j++) {
      list.emplace_back(coreinfo[i][j]);
    }
  }
  return list;
}
// use_psPS，use_dataData
// psf：L2_Hitx，L2_PSx-1；L1_PS(
// L1_Hit) Dataprecise
unsigned Zhangmethod::getFValue(std::string localFunc, const CEOP &localPath,
                                unsigned localUR, std::string interFunc,
                                const CEOP &interPath, unsigned interUR,
                                bool use_ps, bool use_data) {
  UnorderedRegion local_ur = localPath.URs[localUR];
  UnorderedRegion inter_ur = interPath.URs[interUR];
  std::map<unsigned, unsigned> index2ExeTimes;
  std::map<unsigned, bool> indexIsDisturbed;
  unsigned ret_val = 0;

  // for debug
  std::ofstream myfile;
  // std::string fileName = "ZW_F_addr.txt";
  // if (ZWDebug) {
  //   myfile.open(fileName, std::ios_base::app);
  //   myfile << "Func:" << localFunc << " LocalUR:" << localUR
  //          << " Func:" << interFunc << " InterUR:" << interUR << "\n";
  // }
  // 
  // 
  for (const auto &local_pair : local_ur.mi2xclass) {
    const llvm::MachineInstr *local_mi = local_pair.first.MI;
    unsigned tmp_exe_times = local_pair.second.x;
    if (local_pair.second.classification ==
        TimingAnalysisPass::dom::cache::CL2_HIT) {
      index2ExeTimes[mi2cacheIndex(local_mi)] += tmp_exe_times;
      if (ZWDebug) {
        myfile << "  " << "LocalMI(L2_HIT)" << local_mi << " Iaddr:";
        TimingAnalysisPass::printHex(
            myfile, TimingAnalysisPass::StaticAddrProvider->getAddr(local_mi));
        myfile << " Cindex" << mi2cacheIndex(local_mi)
               << " EX : " << tmp_exe_times << "\n";
      }
    } else if (use_ps && local_pair.second.classification ==
                             TimingAnalysisPass::dom::cache::CL2_PS) {
      index2ExeTimes[mi2cacheIndex(local_mi)] += tmp_exe_times;
      // }
      if (ZWDebug) {
        myfile << "  " << "LocalMI(L2_PS)" << local_mi << " Iaddr:";
        TimingAnalysisPass::printHex(
            myfile, TimingAnalysisPass::StaticAddrProvider->getAddr(local_mi));
        myfile << " Cindex" << mi2cacheIndex(local_mi)
               << " EX : " << tmp_exe_times << "\n";
      }
    }
  }

  // 
  if (use_data) {
    // 
    for (const auto &local_pair : local_ur.mi2xclass) {
      // const llvm::MachineInstr *local_mi = local_pair.first.MI;
      // TODO (entry, CtxMI) -> datalist，MI
      CtxMI MI = local_pair.first;
      // AccessInfo CLx = local_pair.second;
      std::vector<AccessInfo> datalist;
      auto it = Ceopinfos.entry2ctxmi2datainfo[localFunc].find(MI);
      if (it != Ceopinfos.entry2ctxmi2datainfo[localFunc].end()) {
        datalist = it->second;
      }
      for (AccessInfo &datainfo : datalist) {
        // TODO 
        if (datainfo.data_addr.isPrecise()) {
          if (datainfo.classification ==
              TimingAnalysisPass::dom::cache::CL2_HIT) {
            index2ExeTimes[getcacheIndex(
                datainfo.data_addr.getAsInterval().lower())] += datainfo.x;
            // 
            if (ZWDebug) {
              myfile << "  " << "LocalMI(L2_HIt)" << MI.MI << " Daddr:";
              TimingAnalysisPass::printHex(
                  myfile, datainfo.data_addr.getAsInterval().lower());
              myfile << " Cindex"
                     << getcacheIndex(
                            datainfo.data_addr.getAsInterval().lower())
                     << " EX : " << datainfo.x << "\n";
            }
          } else if (use_ps && datainfo.classification ==
                                   TimingAnalysisPass::dom::cache::CL2_PS) {
            // if (datainfo.x > 1) {
            index2ExeTimes[getcacheIndex(
                datainfo.data_addr.getAsInterval().lower())] += datainfo.x;
            // }
            // 
            if (ZWDebug) {
              myfile << "  " << "LocalMI(L2_PS)" << MI.MI << " Daddr:";
              TimingAnalysisPass::printHex(
                  myfile, datainfo.data_addr.getAsInterval().lower());
              myfile << " Cindex"
                     << getcacheIndex(
                            datainfo.data_addr.getAsInterval().lower())
                     << " EX : " << datainfo.x << "\n";
            }
          }
        }
      }
    }
  }
  // 
  for (const auto &inter_pair : inter_ur.mi2xclass) {
    const llvm::MachineInstr *inter_mi = inter_pair.first.MI;
    if (inter_pair.second.classification !=
            TimingAnalysisPass::dom::cache::CL_HIT ||
        inter_pair.second.classification !=
            TimingAnalysisPass::dom::cache::CL_BOT) {
      indexIsDisturbed[mi2cacheIndex(inter_mi)] = true;
      if (ZWDebug) {
        myfile << "  " << "InterMI" << inter_mi << " Iaddr:";
        TimingAnalysisPass::printHex(
            myfile, TimingAnalysisPass::StaticAddrProvider->getAddr(inter_mi));
        myfile << " Cindex" << mi2cacheIndex(inter_mi) << "\n";
      }
    }
    // const llvm::MachineInstr *inter_mi = inter_pair.first.MI;
    // TODO (entry, CtxMI) -> datalist，MI
    std::vector<AccessInfo> datalist;
    auto it = Ceopinfos.entry2ctxmi2datainfo[interFunc].find(inter_pair.first);
    if (it != Ceopinfos.entry2ctxmi2datainfo[interFunc].end()) {
      datalist = it->second;
    }
    for (AccessInfo &datainfo : datalist) {
      // TODO 
      if (datainfo.data_addr.isPrecise()) {
        if (datainfo.classification != TimingAnalysisPass::dom::cache::CL_HIT ||
            datainfo.classification !=
                TimingAnalysisPass::dom::cache::CL_BOT) {
          indexIsDisturbed[getcacheIndex(
              datainfo.data_addr.getAsInterval().lower())] = true;
        }
      }else if (datainfo.data_addr.isArray()) {
        auto l = datainfo.data_addr.getAsInterval();
        if (datainfo.classification != TimingAnalysisPass::dom::cache::CL_HIT ||
                      datainfo.classification !=
                          TimingAnalysisPass::dom::cache::CL_BOT){
          for (int temp = l.lower(); temp < l.upper(); temp += Dlinesize) {
            {
              indexIsDisturbed[getcacheIndex(temp)] = true;
            }
          }
        }
      }
    }
  }
  for (auto &pair : index2ExeTimes) {
    if (indexIsDisturbed[pair.first]) {
      ret_val += pair.second;
    }
  }
  if (ZWDebug) {
    myfile << "  Contention for " << ret_val << " times\n";
  }
  return ret_val;
}