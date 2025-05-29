#include "Util/UrGraph.h"
#include "Util/GlobalVars.h"
#include <cassert>

UrGraph::UrGraph(std::vector<std::vector<std::string>> &setc) {
  this->coreinfo = setc; // in base
  // URCEOP
  for (unsigned i = 0; i < coreinfo.size(); ++i) {
    outs() << " -> UR Analysis for core: " << i;

    for (std::string &functionName : coreinfo[i]) {
      outs() << " entry point: " << functionName << '\n';
      URCalculation(i, functionName);
      outs() << "Core-" << i << " Func: " << functionName << " have "
             << CEOPs[i][functionName].size() << " CEOP(s)" << '\n';
    }
  }
  // handsome_ceop_instr();
}

void UrGraph::handsome_ceop_instr() {
  std::ofstream myfile;
  std::string fileName = "Zhao_Output.txt";
  myfile.open(fileName, std::ios_base::app);
  for (auto &tmp_core : CEOPs) {
    unsigned core_num = tmp_core.first;
    for (auto &tmp_task : tmp_core.second) {
      std::string enrty_name = tmp_task.first;
      unsigned num_ceop = 0;
      unsigned max = 0;
      for (CEOP &tmp_ceop : tmp_task.second) {
        num_ceop++;
        unsigned sum_of_instr = 0;
        for (UnorderedRegion &tmp_ur : tmp_ceop.URs) {
          for (auto &tmp_pair : tmp_ur.mi2xclass) {
            CtxMI tmp_cm = tmp_pair.first;
            sum_of_instr += ctxmi_miai[core_num][enrty_name][tmp_cm].x;
          }
        }
        myfile << enrty_name << "_" << "_CEOP_" << num_ceop << " have "
               << sum_of_instr << " instructions" << std::endl;
        max = max > sum_of_instr ? max : sum_of_instr;
      }
      myfile << enrty_name << "_" << "MAX_CEOP_: " << max << std::endl;
    }
  }
  myfile.close();
}

void UrGraph::write_mi_cfgs(const std::string &function) {
  for (auto &tmp_pair : mi_cfg) {
    mi_cfgs[function][tmp_pair.first] = tmp_pair.second;
  }
}

void UrGraph::URCalculation(unsigned core, const std::string &function) {
  // analysisDriver.h
  auto MF =
      TimingAnalysisPass::machineFunctionCollector->getFunctionByName(function);
  const MachineBasicBlock *analysisStart = &*(MF->begin());
  const llvm::MachineInstr *firstMI = &(analysisStart->front());
  // 
  dfncnt = stack_pt = ur_id = 0;
  // task；map，
  dfn.clear();
  low.clear();
  ur_stack.clear();
  in_stack.clear();
  mi_ur.clear();
  ur_size.clear();
  ur_graph.clear();
  ur_mi.clear();
  tmpCEOPs.clear();
  mi_cfg.clear();

  CtxMI firstCM;
  firstCM.MI = firstMI;
  tarjan_it(firstCM); // module1: CFGUR
  collectUrInfo(); // module2: URur
  collectCEOPInfo(firstCM, core,
                  function); // module4: dfs，CEOP
  CEOPs[core][function] = tmpCEOPs;
  getExeCntMust(); // 
  write_mi_cfgs(function);
  if (ZWDebug) {
    print_mi_cfg(core, function); // debug
  }

  if (ZWDebug) {
    // std::ofstream myfile;
    // std::string fileName = "ZW_Output.txt";
    // myfile.open(fileName, std::ios_base::app);
    // myfile << "EntryPoint: " << function << " with " << tmpCEOPs.size()
    //        << "CEOPs" << std::endl;
    // for (int i = 0; i < tmpCEOPs.size(); i++) {
    //   CEOP tmp_ceop = tmpCEOPs[i];
    //   myfile << "  CEOP-" << i << " have " << tmp_ceop.URs.size() << " UR(s)"
    //          << std::endl;
    // }
  }
}

void UrGraph::ceopDfs(unsigned u, unsigned &cur_core, std::string &cur_func) {
  UnorderedRegion curUR{};
  std::vector<CtxMI> curMIs = ur_mi[u];
  for (int i = 0; i < curMIs.size(); i++) {
    AccessInfo obj = ctxmi_miai[cur_core][cur_func][curMIs[i]];
    // if(curMIs[i]->isTransient()) continue; // 
    curUR.mi2xclass.insert(std::make_pair(curMIs[i], obj));
  }
  assert(curUR.mi2xclass.size() != 0 && "UR must have at least 1 instr");
  tmpPath.push_back(curUR);

  std::vector<unsigned> vs = ur_graph[u];
  if (vs.size() == 0) { // 
    CEOP curCeop{};
    curCeop.URs = tmpPath;
    tmpCEOPs.push_back(curCeop); // recorded
    tmpPath.pop_back();
    return;
  }

  for (int i = 0; i < vs.size(); i++) {
    unsigned v = vs[i];
    ceopDfs(v, cur_core, cur_func);
  }
  tmpPath.pop_back();
  return;
}

void UrGraph::ceopDfs_it(unsigned start_u, unsigned &cur_core, std::string &cur_func) {
    std::stack<std::pair<unsigned, bool>> stack;  //  <u, >
    stack.push({start_u, false});

    while (!stack.empty()) {
        auto [u, expanded] = stack.top();
        stack.pop();

        if (!expanded) {
            // ：
            UnorderedRegion curUR{};
            std::vector<CtxMI> curMIs = ur_mi[u];
            
            // UR
            for (int i = 0; i < curMIs.size(); i++) {
                AccessInfo obj = ctxmi_miai[cur_core][cur_func][curMIs[i]];
                // if (curMIs[i]->isTransient()) continue; // 
                curUR.mi2xclass.insert({curMIs[i], obj});
            }
            assert(curUR.mi2xclass.size() != 0 && "UR must have at least 1 instr");
            tmpPath.push_back(curUR);

            // （）
            std::vector<unsigned> vs = ur_graph[u];
            if (vs.empty()) {
                CEOP curCeop{};
                curCeop.URs = tmpPath;
                tmpCEOPs.push_back(curCeop);
                tmpPath.pop_back();  // 
                continue;
            }

            // （）
            stack.push({u, true});

            // ，
            for (auto it = vs.rbegin(); it != vs.rend(); ++it) {
                stack.push({*it, false});
            }
        } else {
            // ：
            tmpPath.pop_back();
        }
    }
}

void UrGraph::collectCEOPInfo(CtxMI firstCM, unsigned core,
                              std::string function) {
  unsigned s = mi_ur[firstCM];
  ceopDfs_it(s, core, function);
}

void UrGraph::collectUrInfo() { //  ur_ctxmi
  for (auto m_u : mi_ur) {
    CtxMI tmp_cm = m_u.first;
    unsigned ur_id_num = m_u.second;
    auto it = ur_graph.find(ur_id_num);
    if (it == ur_graph.end()) { // UR
      std::vector<unsigned> ur_out_edge_vec;
      ur_graph[ur_id_num] = ur_out_edge_vec;
      std::vector<CtxMI> ur_mi_vec;
      ur_mi[ur_id_num] = ur_mi_vec;
    }
    // urmi
    ur_mi[ur_id_num].push_back(tmp_cm);
    // urur
    std::vector<CtxMI> succ_cms = tmp_cm.getSucc();
    for (auto succ_cm : succ_cms) {
      unsigned target_ur_id_num = mi_ur[succ_cm];
      if (ur_id_num != target_ur_id_num) {
        ur_graph[ur_id_num].push_back(target_ur_id_num);
      }
    }
  }
}

void UrGraph::print_mi_cfg(unsigned cur_core, const std::string &function) {
  const std::vector<std::string> colors = {
      "turquoise", "lightblue", "lightgreen", "lightyellow", "white"};
  std::unordered_map<std::string, std::string> func_color_map;
  int color_index = 0;
  std::error_code EC;
  raw_fd_ostream File("UrGraph_" + function + ".dot", EC,
                      sys::fs::OpenFlags::OF_Text);
  if (EC) {
    errs() << "Error opening file: " << EC.message() << "\n";
  }
  File << "digraph \"MachineCFG of " + function + "\" {\n";
  // CtxMIID
  std::map<CtxMI, unsigned> cm_id_map;
  unsigned cnt = 0;
  for (auto tmp_pair : mi_cfg) {
    CtxMI CM = tmp_pair.first;
    cm_id_map[CM] = cnt++;
  }
  for (auto tmp_pair : mi_cfg) {
    // 
    CtxMI CM = tmp_pair.first;
    const llvm::MachineInstr *MI = CM.MI;
    const std::string func_name =
        MI->getParent()->getParent()->getFunction().getName().str();
    if (func_color_map.find(func_name) == func_color_map.end()) {
      func_color_map[func_name] = colors[color_index % colors.size()];
      color_index++;
    }
    const std::string color = func_color_map[func_name];
    // 
    File << "  " << "Node" << cm_id_map[CM] << " [label=\"MI" << MI << "\\l  ";
    MI->print(File, false, false, true);
    File << "\\l  ";
    std::string tmp_flag = (MI->isTransient()) ? "True" : "False";
    File << "isTransient:" << tmp_flag << "\\l  ";
    // UrGraphCL
    File << "ExeCnt:" << ctxmi_miai[cur_core][function][CM].x << "\\l  ";
    // File << "ExeCnt:" << ctxmi_miai[cur_core][function][CM].x << " "
    //      << "CHMC:" << ctxmi_miai[cur_core][function][CM].classification
    //      << "\\l  ";
    unsigned tmp_addr = TimingAnalysisPass::StaticAddrProvider->getAddr(MI);
    unsigned tmp_line = tmp_addr / L2linesize;
    unsigned tmp_index = tmp_line % NN_SET;
    File << "MI's addr:";
    TimingAnalysisPass::printHex(File, tmp_addr);
    File << " cache line:" << tmp_line << " cache index:" << tmp_index
         << "\\l  ";
    File << "More Info of MI:"
         << TimingAnalysisPass::getMachineInstrIdentifier(MI) << "\\l";
    File << "in UR" << mi_ur[CM] << "\\l  ";
    File << "May Load?" << MI->mayLoad() << "\\l  ";
    File << "May Store?" << MI->mayStore() << "\\l  ";
    // UrGraphdata accessExeCntCL，OurGraph
    // File << "data access: [\n";
    // for (AccessInfo tmpai : entry2ctxmi2datainfo[function][CM]) {
    //   File << "[addr_0x";
    //   TimingAnalysisPass::printHex(File, tmpai.data_addr);
    //   // File << "_cl_" << tmpai.classification << "_age_" << tmpai.age
    //   //      << "_execnt_" << tmpai.x << "]\n";
    // }
    File << "]\\l  ";
    File << "\" fillcolor=\"" << color << "\" style=\"filled\"];\n";
    // 
    std::vector<CtxMI> tmp_CMs = tmp_pair.second;
    for (auto tmp_CM : tmp_CMs) {
      File << "  " << "Node" << cm_id_map[CM] << " -> " << "Node"
           << cm_id_map[tmp_CM] << ";\n";
    }
  }
  File << "}\n";
}

void UrGraph::tarjan(CtxMI CM) {
  low[CM] = dfn[CM] = ++dfncnt;
  ur_stack[++stack_pt] = CM;
  in_stack[CM] = 1;
  std::vector<CtxMI> SUCCs = CM.getSucc();

  // mi_cfg，debug
  if (mi_cfg.find(CM) == mi_cfg.end()) {
    mi_cfg[CM] = SUCCs;
  }

  for (auto SUCC : SUCCs) {
    if (dfn.find(SUCC) == dfn.end()) { // 
      tarjan(SUCC);
      low[CM] = std::min(low[CM], low[SUCC]); // ，somehow
    } else if (in_stack[SUCC]) {
      low[CM] = std::min(low[CM], dfn[SUCC]);
    }
  }
  if (dfn[CM] == low[CM]) { // ，eg1
    // ，pop
    ++ur_id;
    do {
      mi_ur[ur_stack[stack_pt]] = ur_id;
      ur_size[ur_id] += 1;
      in_stack[ur_stack[stack_pt]] = 0;
    } while (ur_stack[stack_pt--] != CM);
  }
}

void UrGraph::tarjan_iterative(CtxMI start) {
  // Stack to simulate recursion
  std::stack<std::pair<CtxMI, int>> call_stack;
  std::map<CtxMI, int> succ_index;
  
  call_stack.push({start, 0});
  
  while (!call_stack.empty()) {
    CtxMI CM = call_stack.top().first;
    int& idx = call_stack.top().second;
    std::vector<CtxMI> SUCCs;
    
    if (idx == 0) {
      // First visit to this node - initialize
      low[CM] = dfn[CM] = ++dfncnt;
      ur_stack[++stack_pt] = CM;
      in_stack[CM] = 1;
      SUCCs = CM.getSucc();
      
      // mi_cfg，debug
      if (mi_cfg.find(CM) == mi_cfg.end()) {
        mi_cfg[CM] = SUCCs;
      }
      
      succ_index[CM] = 0;
    } else {
      // Returning from a recursive call
      SUCCs = mi_cfg[CM];
      
      // Update low value after child's visit
      if (idx > 0) {
        CtxMI SUCC = SUCCs[idx - 1];
        low[CM] = std::min(low[CM], low[SUCC]);
      }
    }
    
    // Process next successor
    while (succ_index[CM] < SUCCs.size()) {
      CtxMI SUCC = SUCCs[succ_index[CM]];
      succ_index[CM]++;
      
      if (dfn.find(SUCC) == dfn.end()) {
        // Push current state back with updated index
        idx = succ_index[CM];
        // Push successor to process it
        call_stack.push({SUCC, 0});
        break;
      } else if (in_stack[SUCC]) {
        low[CM] = std::min(low[CM], dfn[SUCC]);
      }
    }
    
    // If we've processed all successors or have none
    if (succ_index[CM] >= SUCCs.size()) {
      // Check if this is the root of an SCC
      if (dfn[CM] == low[CM]) {
        ++ur_id;
        do {
          mi_ur[ur_stack[stack_pt]] = ur_id;
          ur_size[ur_id] += 1;
          in_stack[ur_stack[stack_pt]] = 0;
        } while (ur_stack[stack_pt--] != CM);
      }
      
      call_stack.pop();
    }
  }
}

void UrGraph::tarjan_it(CtxMI start) {
    std::stack<std::pair<CtxMI, bool>> stack; // <, >
    std::stack<CtxMI> call_stack;            // 
    std::map<CtxMI, std::vector<CtxMI>::iterator> iterators;

    call_stack.push(start);
    stack.push({start, false});

    while (!stack.empty()) {
        auto [CM, expanded] = stack.top();
        stack.pop();

        if (!expanded) { // 
            low[CM] = dfn[CM] = ++dfncnt;
            ur_stack[++stack_pt] = CM;
            in_stack[CM] = 1;

            // mi_cfg
            if (mi_cfg.find(CM) == mi_cfg.end()) {
                std::vector<CtxMI> SUCCs = CM.getSucc();
                mi_cfg[CM] = SUCCs;
            }

            auto& succs = mi_cfg[CM];
            iterators[CM] = succs.begin();
            
            // 
            stack.push({CM, true});
            
            // 
            for (auto it = succs.rbegin(); it != succs.rend(); ++it) {
                CtxMI SUCC = *it;
                if (dfn.find(SUCC) == dfn.end()) {
                    call_stack.push(CM); // 
                    stack.push({SUCC, false});
                }
            }
        } else { // 
            auto& it = iterators[CM];
            auto& succs = mi_cfg[CM];

            for (; it != succs.end(); ++it) {
                CtxMI SUCC = *it;
                if (dfn.find(SUCC) == dfn.end()) { 
                    // 
                    low[CM] = std::min(low[CM], low[SUCC]);
                } else if (in_stack[SUCC]) {
                    low[CM] = std::min(low[CM], dfn[SUCC]);
                }
            }

            // 
            if (dfn[CM] == low[CM]) {
                ++ur_id;
                CtxMI top;
                do {
                    top = ur_stack[stack_pt--];
                    mi_ur[top] = ur_id;
                    ur_size[ur_id] += 1;
                    in_stack[top] = 0;
                } while (top != CM);
            }

            // low
            if (!call_stack.empty()) {
                CtxMI parent = call_stack.top();
                call_stack.pop();
                low[parent] = std::min(low[parent], low[CM]);
            }
        }
    }
}

void UrGraph::getExeCntMust() {
  for (auto &tmp_core : CEOPs) {
    unsigned core_num = tmp_core.first;
    for (auto &tmp_task : tmp_core.second) {
      std::string enrty_name = tmp_task.first;
      for (CEOP &tmp_ceop : tmp_task.second) {
        for (UnorderedRegion &tmp_ur : tmp_ceop.URs) {
          for (auto &tmp_pair : tmp_ur.mi2xclass) {
            const CtxMI &tmp_cm = tmp_pair.first;
            unsigned a = getGlobalUpBd(enrty_name, tmp_cm);
            unsigned b = ctxmi_miai[core_num][enrty_name][tmp_cm].x;
            tmp_pair.second.x = a > b ? a : b;
            ctxmi_miai[core_num][enrty_name][tmp_cm].x = tmp_pair.second.x;
          }
        }
      }
    }
  }
}

unsigned UrGraph::getGlobalUpBd(std::string entry, CtxMI CM) {
  const llvm::MachineInstr *MI = CM.MI;
  const llvm::MachineBasicBlock *MBB = MI->getParent();
  const llvm::MachineFunction *MF = MBB->getParent();
  unsigned x_local = 1;
  // local execute times
  for (const MachineLoop *loop :
       TimingAnalysisPass::LoopBoundInfo->getAllLoops()) {
    if (MF == loop->getHeader()->getParent() &&
        loop->contains(
            MBB)) { // loop，、
      x_local *= bd_helper1(entry, MBB, loop);
      if (loop->getParentLoop() != nullptr) {
        x_local *= bd_helper2(entry, loop->getParentLoop());
      }
      break;
    }
  }
  if (CM.CallSites.size() != 0) { // 
    const llvm::MachineInstr *callsite = CM.CallSites.back();
    std::vector<const llvm::MachineInstr *> tmpCS = CM.CallSites;
    tmpCS.pop_back();
    CtxMI tmpCM;
    tmpCM.MI = callsite;
    tmpCM.CallSites = tmpCS;
    x_local *= getGlobalUpBd(entry, tmpCM);
  }
  return x_local;
}

unsigned UrGraph::bd_helper1(std::string entry,
                             const llvm::MachineBasicBlock *MBB,
                             const llvm::MachineLoop *Loop) {
  unsigned x_local = 1;
  int b = TimingAnalysisPass::LoopBoundInfo->GgetUpperLoopBound(Loop);
  assert(b != -1 && "we have a loop but no LoopBound?");
  x_local *= b;
  for (auto *Subloop : Loop->getSubLoops()) {
    if (Subloop->getParentLoop() == Loop // ，
        && Subloop->contains(MBB)) {
      x_local *= bd_helper1(entry, MBB, Subloop);
      break; // 
    }
  }
  return x_local;
}

unsigned UrGraph::bd_helper2(std::string entry, const llvm::MachineLoop *Loop) {
  unsigned scalar = 1;
  int b = TimingAnalysisPass::LoopBoundInfo->GgetUpperLoopBound(Loop);
  assert(b != -1 && "we have a loop but no LoopBound?");
  scalar *= b;
  if (Loop->getParentLoop() == nullptr) { // 
    return scalar;
  } else {
    scalar *= bd_helper2(entry, Loop->getParentLoop());
  }
  return scalar;
}