#ifndef OurMETHOD
#define OurMETHOD

#include "Memory/Classification.h"
#include "Options.h"
#include "Util/CLinfo.h"
#include "Util/PersistenceScope.h"
#include "Util/Statistics.h"
#include "Util/UrGraph.h"
#include "Util/Zhangmethod.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <fstream>
#include <future>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <ostream>
#include <queue>
#include <set>
#include <stdio.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
// extern const TimingAnalysisPass::dom::cache::Classification CL1_PS;
// extern const TimingAnalysisPass::dom::cache::Classification CL2_PS;
struct BlockInfo {
  unsigned address;
  int exe_cnt; // 
  int age;     // ，
  int cs_size; // ，
  int cap;     // cache
  TimingAnalysisPass::dom::cache::Classification cl;
  BlockInfo() : cs_size(INT_MAX) {}
  BlockInfo(const BlockInfo &other)
      : address(other.address), exe_cnt(other.exe_cnt), age(other.age),
        cs_size(other.cs_size), cap(other.cap), cl(other.cl) {}
  BlockInfo(unsigned addr, int cnt, int a,
            TimingAnalysisPass::dom::cache::Classification c,
            int sz = INT_MAX) {
    this->address = addr;
    this->exe_cnt = cnt;
    this->age = a;
    this->cs_size = sz; // 
    this->cl = c;
    if (this->cs_size != INT_MAX) {
      this->cap = std::max(int(L2assoc - cs_size + 1), 0);
    } else {
      this->cap = -1; // cap-1
    }
  }

  static unsigned getCachelineAddress(unsigned addr) {
    return addr & ~(L2linesize - 1);
  }

  bool operator<(
      const BlockInfo &a) const { // ，set<BlockInfo>  alg3
    if (address != a.address)
      return address < a.address;
    else if (cap != a.cap)
      return cap < a.cap;
    else if (age != a.age)
      return age < a.age;
    return exe_cnt < a.exe_cnt;
  }
  bool operator==(const BlockInfo &a) const { // ，set<BlockInfo>
    return this->address == a.address && this->age == a.age &&
           this->exe_cnt == a.exe_cnt && this->cs_size == a.cs_size &&
           this->cl == a.cl;
  }
};

class OurM {
  CEOPinfo Ceopinfos;
  CL_info Cl_infor;
  typedef std::vector<BlockInfo> UR;
  typedef std::vector<UR> Ceop;
  typedef std::vector<Ceop> Ceops;
  std::map<std::string, Ceops> f2ceopsforCR;
  // 
  std::map<
      std::string,
      std::map<CtxMI, std::vector<std::pair<const llvm::MachineLoop *, bool>>>>
      ctxmi2ps_loop_stack;
  std::map<std::string,
           std::map<TimingAnalysisPass::AbstractAddress,
                    std::vector<std::pair<const llvm::MachineLoop *, bool>>>>
      ctxdata2ps_loop_stack;

public:
  OurM(CEOPinfo &ceopinfos, CL_info &cl_infor)
      : Ceopinfos(ceopinfos), Cl_infor(cl_infor) {
    for (auto &tmp_core : Ceopinfos.CEOPs) {
      unsigned core_num = tmp_core.first;
      // FIXME, core_num is nor used now
      for (auto &tmp_task : tmp_core.second) {
        std::string f_name = tmp_task.first;
        if (f2ceopsforCR.count(f_name) > 0) {
          break;
        }
        Ceops ceops_res;
        for (auto &tmp_ceop : tmp_task.second) {
          Ceop ceop_res;
          ctxmi2ps_loop_stack = Ceopinfos.ctxmi2ps_loop_stack;
          ctxdata2ps_loop_stack = Ceopinfos.ctxdata2ps_loop_stack;
          for (auto &tmp_ur : tmp_ceop.URs) {
            UR ur_res;
            for (auto &tmp_pair : tmp_ur.mi2xclass) {
              // , 
              // 
              insert_Triple(ur_res, tmp_pair.first, tmp_pair.second, f_name);
              std::vector<AccessInfo> datalist;
              auto it =
                  Ceopinfos.entry2ctxmi2datainfo[f_name].find(tmp_pair.first);
              if (it != Ceopinfos.entry2ctxmi2datainfo[f_name].end()) {
                datalist = it->second;
              }
              for (AccessInfo &datainfo : datalist) {

                if (datainfo.data_addr.isPrecise()) {
                  CtxData CtxD(tmp_pair.first, datainfo.data_addr);
                  insert_Triple(ur_res, CtxD, datainfo, f_name);
                } else if (datainfo.data_addr.isArray()) {
                  auto l = datainfo.data_addr.getAsInterval();
                  for (int temp = l.lower(); temp < l.upper();
                       temp += Dlinesize) {
                    // AccessInfo dat = datainfo;
                    // dat.data_addr =
                    //     TimingAnalysisPass::AbstractAddress((unsigned)temp);
                    // CtxData CtxD(tmp_pair.first, dat.data_addr);
                    // insert_Triple(ur_res, CtxD, datainfo, f_name);

                    BlockInfo bi_res(temp, datainfo.x, datainfo.age,
                                     datainfo.classification);
                    ur_res.emplace_back(bi_res);
                  }
                }
              }
              // TODO data access is not handle
              // 
            }

            ceop_res.push_back(ur_res);
          }
          ceops_res.push_back(ceop_res);
        }
        f2ceopsforCR[f_name] = ceops_res;
      }
    }
    print_rwinfo();
  }

  void print_rwinfo(){
    std::map<std::string, std::map<TimingAnalysisPass::dom::cache::Classification,
                                std::pair<unsigned, unsigned>>>
      cl_cnt; // for output(first，second)
    for(auto &tmp_func:f2ceopsforCR){
      std::string func_name = tmp_func.first;
      for(auto &tmp_ceop:tmp_func.second){
        for(auto &tmp_ur:tmp_ceop){
          for(auto &tmp_blk:tmp_ur){
            TimingAnalysisPass::dom::cache::Classification tmp_cl = tmp_blk.cl;
            unsigned tmp_x = tmp_blk.exe_cnt;
            cl_cnt[func_name][tmp_cl].first += tmp_x;
          }
        }
      }
    }
    std::ofstream Myfile;
    Myfile.open("RWInfo.txt", std::ios_base::app);
    for (const auto &clmap : cl_cnt) {
      for (const auto &cl_pair : clmap.second) {
        Myfile << clmap.first << " instr&data " << cl_pair.first << " "
              << cl_pair.second.first << "\n";
      }
    }
  }

  void insert_Triple(UR &ur_res, const CtxData &CtD, AccessInfo &CMI_CL,
                     std::string fname) {
    unsigned addr = CtD.data_addr.getAsInterval().lower();
    bool isps = isPS(CtD, fname);
    // 
    // 1. CL_HIT  2. CL2_MISS+
    if (CMI_CL.classification == TimingAnalysisPass::dom::cache::CL_HIT ||
        (CMI_CL.classification == TimingAnalysisPass::dom::cache::CL2_MISS &&
         (!isps || ur_res.size() <= 1))) {
      BlockInfo bi_res(addr, CMI_CL.x, CMI_CL.age, CMI_CL.classification);
      ur_res.push_back(bi_res);
    } else if (isps && ur_res.size() > 1) {
      // 3. CL_2HIT 1
      // 5. CL2_MISS 12
      std::vector<BlockInfo> list = getPStriple(CtD, fname);
      // assert(!list.empty() && "！");
      int sum = 0;
      for (auto &block : list) {
        sum += block.exe_cnt;
      }
      // x-sum >1
      assert(CMI_CL.x - sum >= 1 && "ERR");
      BlockInfo bi_res(addr, CMI_CL.x - sum, CMI_CL.age, CMI_CL.classification);
      ur_res.push_back(bi_res);
      ur_res.insert(ur_res.end(), list.begin(), list.end());

    } // 4. CL_2HIT 1
    else if (CMI_CL.classification == TimingAnalysisPass::dom::cache::CL2_HIT &&
             (!isps || ur_res.size() <= 1)) {
      // (2，CS)
      // TODO get |CS|
      if (CMI_CL.x > 1) {
        BlockInfo bi_res1(addr, CMI_CL.x, CMI_CL.age,
                          TimingAnalysisPass::dom::cache::CL2_PS,
                          CMI_CL.age + 1);
        ur_res.push_back(bi_res1);
      } else {
        BlockInfo bi_res(addr, 1, CMI_CL.age, CMI_CL.classification);
        ur_res.push_back(bi_res);
      }
    } else {
      if (CMI_CL.classification == TimingAnalysisPass::dom::cache::CL_BOT) {
        BlockInfo bi_res(addr, CMI_CL.x, CMI_CL.age,
                         TimingAnalysisPass::dom::cache::CL_HIT);
        ur_res.push_back(bi_res);

      } else {
        assert(0 && "ERR");
      }
    }
    assert(!ur_res.empty() && "ERR");
  }

  void insert_Triple(UR &ur_res, const CtxMI &CMI, AccessInfo &CMI_CL,
                     std::string fname) {
    unsigned addr = TimingAnalysisPass::StaticAddrProvider->getAddr(CMI.MI);
    bool isps = isPS(CMI, fname);

    // 
    // 1. CL_HIT  2. CL2_MISS+
    if (CMI_CL.classification == TimingAnalysisPass::dom::cache::CL_HIT ||
        (CMI_CL.classification == TimingAnalysisPass::dom::cache::CL2_MISS &&
         (!isps || ur_res.size() <= 1))) {
      BlockInfo bi_res(addr, CMI_CL.x, CMI_CL.age, CMI_CL.classification);
      ur_res.push_back(bi_res);
    } else if (isps && ur_res.size() > 1) {
      // 3. CL_2HIT 1
      // 5. CL2_MISS 12

      std::vector<BlockInfo> list = getPStriple(CMI, fname);
      // assert(!list.empty() && "！");
      // loopbound1
      int sum = 0;
      for (auto &block : list) {
        sum += block.exe_cnt;
      }
      // x-sum >1
      // assert(CMI_CL.x - sum >= 1 && "ERR");
      if (CMI_CL.x - sum >= 1) {
        BlockInfo bi_res(addr, CMI_CL.x - sum, CMI_CL.age,
                         CMI_CL.classification);
        ur_res.push_back(bi_res);
        ur_res.insert(ur_res.end(), list.begin(), list.end());
      } else {
        BlockInfo bi_res(addr, CMI_CL.x, CMI_CL.age, CMI_CL.classification);
        ur_res.push_back(bi_res);
      }

    } // 4. CL_2HIT 1
    else if (CMI_CL.classification == TimingAnalysisPass::dom::cache::CL2_HIT &&
             (!isps || ur_res.size() <= 1)) {
      // (2，CS)
      // TODO get |CS|
      if (CMI_CL.x > 1) {
        BlockInfo bi_res1(addr, CMI_CL.x, CMI_CL.age,
                          TimingAnalysisPass::dom::cache::CL2_PS,
                          CMI_CL.age + 1);
        ur_res.push_back(bi_res1);
      } else {
        BlockInfo bi_res(addr, 1, CMI_CL.age, CMI_CL.classification);
        ur_res.push_back(bi_res);
      }
    } else {
      if (CMI_CL.classification == TimingAnalysisPass::dom::cache::CL_BOT) {
        BlockInfo bi_res(addr, CMI_CL.x, CMI_CL.age,
                         TimingAnalysisPass::dom::cache::CL_HIT);
        ur_res.push_back(bi_res);

      } else {
        assert(0 && "ERR");
      }
    }
    assert(!ur_res.empty() && "ERR");
  }

  // onlyfor 2 core
  int run(std::string afn, std::string cfn) {
    int res = 0;
    unsigned tmp_cnt = 0; // for output
    for (Ceop &ceop : f2ceopsforCR[afn]) {
      CRs crlist(ceop);
      tmp_cnt++;
      std::string tmp_filename =
          "CRs_" + afn + "_path" + std::to_string(tmp_cnt);
      crlist.print_CRs(tmp_filename);
      if (crlist.crs.empty()) {
        std::cout << afn << ": No memory references hit at L2 ..." << std::endl;
        continue;
      }
      for (Ceop &ceop__ : f2ceopsforCR[cfn]) {
        int t = crlist.DpMaxOutNumber(ceop__);
        std::ofstream myfile;
        myfile.open("ourM_OUTPUT.txt", std::ios_base::app);
        myfile << afn << ": " << t << std::endl;
        myfile.close();
        res = res > t ? res : t;
      }
    }
    return res;
  }

  // onlyfor 2 core
  int runP(std::string afn, std::string cfn) {
    std::mutex file_mutex;
    std::mutex res_mutex;
    std::atomic<int> res(0); // 
    std::vector<std::future<void>> outer_futures;

    unsigned tmp_cnt = 0;
    auto &ceop_list_afn = f2ceopsforCR[afn];

    for (size_t i = 0; i < ceop_list_afn.size(); ++i) {
      Ceop &ceop = ceop_list_afn[i];

      //  ceop  async
      outer_futures.emplace_back(std::async(std::launch::async, [&, i]() {
        CRs crlist(ceop);

        std::string tmp_filename =
            "CRs_" + afn + "_path" + std::to_string(i + 1);
        crlist.print_CRs(tmp_filename);

        if (crlist.crs.empty()) {
          std::lock_guard<std::mutex> lock(file_mutex);
          std::cout << afn << ": No memory references hit at L2 ..."
                    << std::endl;
          return;
        }

        std::vector<std::future<void>> inner_futures;
        for (Ceop &ceop__ : f2ceopsforCR[cfn]) {
          inner_futures.emplace_back(
              std::async(std::launch::async, [&crlist, &afn, &ceop__, &res,
                                              this, &file_mutex]() {
                int t = crlist.DpMaxOutNumber(ceop__);

                {
                  std::lock_guard<std::mutex> lock(file_mutex);
                  std::ofstream myfile("ourM_OUTPUT.txt", std::ios_base::app);
                  myfile << afn << ": " << t << std::endl;
                }

                // 
                int current = res.load();
                while (t > current && !res.compare_exchange_weak(current, t)) {
                  // retry
                }
              }));
        }

        for (auto &f : inner_futures)
          f.get();
      }));
    }

    for (auto &f : outer_futures)
      f.get();

    return res.load();
  }

  void analis(std::vector<std::vector<std::string>> &coreinfo) {
    int res = -1;
    if (coreinfo.size() == 2) {
      std::ofstream myfile;
      myfile.open("Result.txt", std::ios_base::app);
      TimingAnalysisPass::Statistics &Stats =
          TimingAnalysisPass::Statistics::getInstance();
      Stats.startMeasurement("0_" + coreinfo[0][0] + "_1_" + coreinfo[1][0] +
                             "_inter");
      res = run(coreinfo[0][0], coreinfo[1][0]);
      Stats.stopMeasurement("0_" + coreinfo[0][0] + "_1_" + coreinfo[1][0] +
                            "_inter");
      std::cout << coreinfo[0][0] << " WCEET: " << res * Latency << std::endl;
      myfile << coreinfo[0][0] << " inter " << coreinfo[1][0] << " "
             << res * Latency << std::endl;
      Stats.startMeasurement("1_" + coreinfo[1][0] + "_0_" + coreinfo[0][0] +
                             "_inter");
      res = run(coreinfo[1][0], coreinfo[0][0]);
      Stats.stopMeasurement("1_" + coreinfo[1][0] + "_0_" + coreinfo[0][0] +
                            "_inter");
      std::cout << coreinfo[1][0] << " WCEET: " << res * Latency << std::endl;
      myfile << coreinfo[1][0] << " inter " << coreinfo[0][0] << " "
             << res * Latency << std::endl;
    } else {
      std::cerr << " > 2 ";
    }
  }

  static std::pair<unsigned, unsigned> getTagAndIndex(unsigned addr) {
    unsigned blockNumber = addr / L2linesize;
    return std::make_pair(blockNumber / NN_SET, blockNumber % NN_SET);
  }

  // static unsigned getIndex(unsigned addr) {
  //   int blockOffset = log2(L2linesize), setOffset = log2(NN_SET);
  //   if ((1 << blockOffset) < L2linesize)
  //     blockOffset++;
  //   if ((1 << setOffset) < NN_SET)
  //     setOffset++;

  //   return (addr >> blockOffset) & ((1 << setOffset) - 1);
  // }
  // static unsigned getTag(unsigned addr) {
  //   int blockOffset = log2(L2linesize), setOffset = log2(NN_SET);
  //   if ((1 << blockOffset) < L2linesize)
  //     blockOffset++;
  //   if ((1 << setOffset) < NN_SET)
  //     setOffset++;

  //   return addr >> blockOffset >> setOffset;
  // }

  void print_ceopsforCR(const std::string &filename = "ceopsforCR.txt") {
    std::ofstream outfile(filename);
    if (!outfile) {
      std::cerr << "Error: Could not open file " << filename
                << " for writing.\n";
      return;
    }

    // （）
    const std::string COLOR_HEADER = "";
    const std::string COLOR_KEY = "";
    const std::string COLOR_RESET = "";

    // 
    outfile << std::left;

    for (const auto &[funcName, ceops] : f2ceopsforCR) {
      outfile << COLOR_HEADER << "Function: " << COLOR_KEY << funcName
              << COLOR_RESET << "\n"
              << std::string(50, '-') << "\n";

      for (size_t ceopIdx = 0; ceopIdx < ceops.size(); ++ceopIdx) {
        outfile << "  CEOP #" << ceopIdx + 1 << ":\n";

        for (size_t urIdx = 0; urIdx < ceops[ceopIdx].size(); ++urIdx) {
          outfile << "    UR #" << urIdx + 1 << ":\n";

          for (const BlockInfo &block : ceops[ceopIdx][urIdx]) {
            outfile << "      - Address: 0x" << std::hex << block.address
                    << std::dec << "\n";
            outfile << "        ExecCnt: " << block.exe_cnt << "\n";
            outfile << "        Age:     " << block.age << "\n";
            outfile << "        CS_Size: " << block.cs_size << "\n";
            outfile << "        Cap:     " << block.cap << "\n";
            outfile << "        Class:   " << block.cl << "\n";
            outfile << std::string(30, '.') << "\n";
          }
        }
        outfile << std::string(40, '=') << "\n";
      }
      outfile << "\n";
    }

    outfile.close();
  }

  struct CR {
    std::vector<BlockInfo> PersistentBlock;
    std::vector<BlockInfo> NotPersistBlock;

    // State，UR
    int OutNumber_NotPersistBlock(int &State, std::vector<BlockInfo> SameBlock,
                                  std::set<unsigned> &s, UR ur) {
      std::map<unsigned, std::set<unsigned>> setInfo;
      for (auto block : ur) { // UR
        if (block.cl == TimingAnalysisPass::dom::cache::CL_HIT ||
            block.cl == TimingAnalysisPass::dom::cache::CL_PS) {
          continue;
        }
        unsigned tag, index, addr = block.address;
        std::tie(tag, index) = getTagAndIndex(addr);
        setInfo[index].insert(tag);
      }

      int ans = 0;
      for (int i = 0; i < NotPersistBlock.size(); ++i) { // 
        if (s.count(NotPersistBlock[i].address))
          continue;

        BlockInfo block = NotPersistBlock[i];
        int age = block.age;
        unsigned tag, index, addr = block.address;
        std::tie(tag, index) = getTagAndIndex(addr);

        if (age + setInfo[index].size() - setInfo[index].count(tag) > L2assoc) {
          for (int j = 0; j < SameBlock.size(); ++j)
            if (SameBlock[j].address == addr) {
              // j1
              if (!((State >> j) & 1)) {
                State |= (1 << j);
                ++ans;
              }
              break;
            }
        }
      }
      return ans;
    }

    // access L2
    void cleanUR(std::vector<UR> &urs2c) {
      for (auto &ur : urs2c) {
        UR ur1;
        for (auto &block : ur) {
          if (block.cl == TimingAnalysisPass::dom::cache::CL_HIT ||
              block.cl == TimingAnalysisPass::dom::cache::CL_PS) {
            continue;
          }
          ur1.emplace_back(block);
        }
        ur = ur1;
      }
    }

    std::vector<std::priority_queue<int>> getpriorityQ(std::vector<UR> &urs2c,
                                                       unsigned index) {
      std::vector<std::priority_queue<int>> res;
      for (auto &ur : urs2c) {
        std::priority_queue<int> pq;
        for (auto &block : ur) {
          int index2, tag2;
          std::tie(tag2, index2) = getTagAndIndex(block.address);
          if (index2 == index) {
            // temp[tag2] += block.exe_cnt;
            pq.push(block.exe_cnt);
          }
        }
        res.push_back(pq);
      }

      return res;
    }

    std::pair<std::priority_queue<int>, int>
    getpriorityQ_add(std::vector<UR> &urs2c, unsigned index) {
      std::priority_queue<int> res;
      int res2 = 0;
      for (auto &ur : urs2c) {
        std::priority_queue<int> pq;
        std::map<int, int> temp1;
        for (auto &block : ur) {
          int index2, tag2;
          std::tie(tag2, index2) = getTagAndIndex(block.address);
          if (index2 == index) {
            temp1[tag2] += block.exe_cnt;
          }
        }
        for (auto &value : temp1) {
          pq.push(value.second);
        }
        std::priority_queue<int> temp;
        res2 += pq.size();
        while (!pq.empty()) {
          int x1 = pq.top();
          pq.pop();
          if (!res.empty()) {
            x1 += res.top();
            res.pop();
          }
          temp.push(x1);
        }
        while (!res.empty()) {
          temp.push(res.top());
          res.pop();
        }
        res = temp;
      }
      return std::make_pair(res, res2);
    }

    // UR
    std::vector<int> OutNumber_PersistentBlock_old(std::vector<UR> &urs2c) {
      cleanUR(urs2c);
      std::map<unsigned, std::vector<BlockInfo>> mp;
      for (auto block :
           PersistentBlock) { // CRaddrblockur
        unsigned addr = block.address;
        if (mp.count(addr))
          mp[addr].push_back(block);
        else {
          std::vector<BlockInfo> tmp;
          tmp.push_back(block);
          mp[addr] = tmp;
        }
      }
      std::vector<int> outNs;
      for (auto it : mp) {
        std::sort(it.second.begin(),
                  it.second.end()); // cap，
        int index, tag;
        std::tie(tag, index) = getTagAndIndex(it.second.front().address);
        std::vector<std::priority_queue<int>> UrCntList =
            getpriorityQ(urs2c, index);
        for (auto block : it.second) {
          int outN = 0;
          int Legacy = 0;
          for (auto &UrCnt : UrCntList) {
            while (Legacy > 0) {
              UrCnt.push(1);
              Legacy--;
            }
            while (block.exe_cnt > 0) {
              if (UrCnt.size() < block.cap) // URcnt
                break;
              std::vector<int> changeVal;
              for (int i = 0; i < block.cap; ++i)
                changeVal.push_back(UrCnt.top() - 1), UrCnt.pop();

              for (int v : changeVal) {
                if (v > 0) {
                  UrCnt.push(v);
                }
              }
              // if (changeVal.size())
              //   break; // ，URcnt>0cnt
              ++outN, --block.exe_cnt;
            }
            if (block.exe_cnt == 0) {
              break;
            }
            if (block.exe_cnt > 0 && UrCnt.size() < block.cap) {
              Legacy = UrCnt.size();
            }
          }
          outNs.push_back(outN);
        }
      }

      return outNs;
    }

    // UR
    std::vector<int>
    OutNumber_PersistentBlock_add_with_continue(std::vector<UR> &urs2c) {
      cleanUR(urs2c);
      std::map<unsigned, std::vector<std::pair<BlockInfo, int>>> mp;
      for (int i = 0; i < PersistentBlock.size();
           i++) { // CRaddrblockur
        auto block = PersistentBlock[i];
        unsigned addr = block.address;
        if (mp.count(addr))
          mp[addr].push_back(std::make_pair(block, i));
        else {
          std::vector<std::pair<BlockInfo, int>> tmp;
          tmp.push_back(std::make_pair(block, i));
          mp[addr] = tmp;
        }
      }
      std::vector<int> outNs;
      outNs.resize(PersistentBlock.size());
      for (auto it : mp) {
        std::sort(it.second.begin(),
                  it.second.end()); // cap，
        int index, tag;
        std::tie(tag, index) = getTagAndIndex(it.second.front().first.address);
        std::priority_queue<int> addQueue;
        int temp;
        std::tie(addQueue, temp) = getpriorityQ_add(urs2c, index);
        for (auto block : it.second) {
          int outN = 0;
          while (block.first.exe_cnt > 0) {
            if (addQueue.size() < block.first.cap) // URcnt
              break;
            std::vector<int> changeVal;
            for (int i = 0; i < block.first.cap; ++i)
              changeVal.push_back(addQueue.top() - 1), addQueue.pop();

            for (int v : changeVal) {
              if (v > 0) {
                addQueue.push(v);
              }
            }
            ++outN, --block.first.exe_cnt;
          }
          int cap = block.first.cap;
          if (cap > 8) {
            cap = block.first.cap / 2;
          }
          if (block.first.exe_cnt >= temp / cap) {
            outN += temp / cap;
            temp = 0;
          } else {
            outN += block.first.exe_cnt;
            block.first.exe_cnt = 0;
            temp -= block.first.exe_cnt * cap;
          }
          outNs[block.second] = outN;
        }
      }

      return outNs;
    }

    // UR
    std::vector<int>
    OutNumber_PersistentBlock_add_with_(std::vector<UR> &urs2c) {
      cleanUR(urs2c);
      std::vector<int> outNs;
      for (auto &block : PersistentBlock) {
        int index, tag;
        std::tie(tag, index) = getTagAndIndex(block.address);
        std::priority_queue<int> addQueue;
        int temp;
        std::tie(addQueue, temp) = getpriorityQ_add(urs2c, index);
        int outN = 0;
        while (block.exe_cnt > 0) {
          if (addQueue.size() < block.cap) // URcnt
            break;
          std::vector<int> changeVal;
          for (int i = 0; i < block.cap; ++i)
            changeVal.push_back(addQueue.top() - 1), addQueue.pop();

          for (int v : changeVal) {
            if (v > 0) {
              addQueue.push(v);
            }
          }
          ++outN, --block.exe_cnt;
        }
        if (block.exe_cnt >= temp) {
          outN += temp;
          temp = 0;
        } else {
          outN += block.exe_cnt;
          temp -= block.exe_cnt;
        }
        outNs.push_back(outN);
      }
      return outNs;
    }

    // Cr
    // //////////////////////////////////////////////////////
    void updataCrUr(std::vector<int> PersistOut, UR ur, UR &NextUr) {
      int mxOut = 0;
      for (int i = 0; i < PersistentBlock.size(); ++i) {
        int num = PersistOut[i];
        mxOut = std::max(
            mxOut,
            num); // cr（urblock）
        PersistentBlock[i].exe_cnt -= num;
      }
      for (auto block : ur) {
        if (block.exe_cnt >
            mxOut) // ururaddressblock
          block.exe_cnt = 1, NextUr.push_back(block);
      }
    }
    int updateCR(std::vector<int> PersistOut, UR ur) {
      int res = 0;
      for (int i = 0; i < PersistentBlock.size(); ++i) {
        int num = PersistOut[i];
        assert((num <= 0 || num > PersistentBlock[i].exe_cnt) &&
               "");
        PersistentBlock[i].exe_cnt -= num;
        bool temp = false;
        for (auto block : ur) {
          if (block.exe_cnt > num) {
            temp = true;
            break;
          }
        }
        res += temp;
      }
      return res;
    }
  };

  struct CRs {
    std::vector<CR> crs;

    // CRs
    CRs(Ceop ceop) {
      // UR（）
      std::vector<std::vector<BlockInfo>> leftNodesInURs, rightNodesInURs;
      // UR（ cnt cap）
      std::vector<std::vector<BlockInfo>> PersistBlockInURs;
      leftNodesInURs.resize(ceop.size()), rightNodesInURs.resize(ceop.size());
      PersistBlockInURs.resize(ceop.size());

      std::map<unsigned, int> LastURforBlock; // Block addr  UR 

      for (int i = 0; i < ceop.size(); ++i) {
        std::vector<unsigned> addlist;
        for (BlockInfo &block : ceop[i]) {
          unsigned addr = block.address;
          unsigned clineaddr = block.address / L2linesize;
          addlist.emplace_back(clineaddr);
          if (block.cl != TimingAnalysisPass::dom::cache::CL2_HIT &&
              block.cl != TimingAnalysisPass::dom::cache::CL2_PS) {
            continue;
          }
          int cnt = block.exe_cnt, age = block.age, cap = block.cap;

          // （cap <= 0）
          // ///////////////////////////////////
          if (block.cl == TimingAnalysisPass::dom::cache::CL2_PS) // cnt > 1
            PersistBlockInURs[i].push_back(block);
          // 
          else if (LastURforBlock.count(clineaddr)) {
            int leftUR = LastURforBlock[clineaddr] + 1, rightUR = i - 1;
            // UR
            if (ceop[LastURforBlock[clineaddr]].size() > 1) {
              leftUR = LastURforBlock[clineaddr];
            }
            if (ceop[i].size() > 1) {
              rightUR = i;
            }
            leftNodesInURs[leftUR].push_back(block);
            rightNodesInURs[rightUR].push_back(block);
          } else if (ceop[i].size() > 1) {
            leftNodesInURs[i].push_back(block);
            rightNodesInURs[i].push_back(block);
          }
        }
        for (unsigned addr : addlist) {
          LastURforBlock[addr] = i;
        }
      }

      // URCR
      std::set<BlockInfo> NotPersistBlocks;
      for (int i = 0; i < ceop.size(); ++i) {
        // URCR
        if (!leftNodesInURs[i].empty() || !rightNodesInURs[i].empty() ||
            !PersistBlockInURs[i].empty()) {
          // block id
          for (auto &p : rightNodesInURs[i])
            NotPersistBlocks.erase(p);
          for (auto &p : leftNodesInURs[i])
            NotPersistBlocks.insert(p);

          CR cr;
          cr.NotPersistBlock.assign(NotPersistBlocks.begin(),
                                    NotPersistBlocks.end());
          cr.PersistentBlock.assign(PersistBlockInURs[i].begin(),
                                    PersistBlockInURs[i].end());
          if (!cr.PersistentBlock.empty() || !cr.NotPersistBlock.empty()) {
            crs.push_back(cr); // cr
          }
        }
      }
    }

    void print_CRs(const std::string &filename = "CRs.txt") {
      std::ofstream outfile(filename);
      if (!outfile) {
        std::cerr << "Error: Could not open file " << filename
                  << " for writing.\n";
        return;
      }

      // 
      outfile << std::left;

      // 
      outfile << "CR Blocks Analysis Report\n";
      outfile << std::string(50, '=') << "\n\n";

      for (size_t crIdx = 0; crIdx < crs.size(); ++crIdx) {
        outfile << "CR #" << crIdx + 1 << ":\n";
        outfile << std::string(40, '-') << "\n";

        // 
        outfile << "  Persistent Blocks (" << crs[crIdx].PersistentBlock.size()
                << "):\n";
        if (!crs[crIdx].PersistentBlock.empty()) {
          outfile
              << "  +------------+---------+-----+---------+-----+-------+\n";
          outfile
              << "  | Address    | ExecCnt | Age | CS_Size | Cap | Class |\n";
          outfile
              << "  +------------+---------+-----+---------+-----+-------+\n";

          for (const BlockInfo &block : crs[crIdx].PersistentBlock) {
            outfile << "  | 0x" << std::hex << std::setw(8) << std::setfill(' ')
                    << block.address << std::dec << " | " << std::setw(7)
                    << block.exe_cnt << " | " << std::setw(3) << block.age
                    << " | " << std::setw(7) << block.cs_size << " | "
                    << std::setw(3) << block.cap << " | " << std::setw(5)
                    << block.cl << " |\n";
          }
          outfile
              << "  +------------+---------+-----+---------+-----+-------+\n";
        } else {
          outfile << "  (No persistent blocks)\n";
        }

        // 
        outfile << "\n  Non-Persistent Blocks ("
                << crs[crIdx].NotPersistBlock.size() << "):\n";
        if (!crs[crIdx].NotPersistBlock.empty()) {
          outfile
              << "  "
                 "+------------+---------+-----+------------------+-------+\n";
          outfile << "  | Address    | ExecCnt | Age | (Not Persistent) | "
                     "Class |\n";
          outfile
              << "  "
                 "+------------+---------+-----+------------------+-------+\n";

          for (const BlockInfo &block : crs[crIdx].NotPersistBlock) {
            outfile << "  | 0x" << std::hex << std::setw(8) << std::setfill(' ')
                    << block.address << std::dec << " | " << std::setw(7)
                    << block.exe_cnt << " | " << std::setw(3) << block.age
                    << " | " << std::setw(16) << "N/A" << " | " << std::setw(5)
                    << block.cl << " |\n";
          }
          outfile
              << "  +------------+---------+-----+----------------+-------+\n";
        } else {
          outfile << "  (No non-persistent blocks)\n";
        }

        outfile << "\n" << std::string(50, '=') << "\n\n";
      }

      // 
      outfile << "\nSUMMARY:\n";
      outfile << "Total CRs analyzed: " << crs.size() << "\n";
      size_t totalPersistent = 0, totalNonPersistent = 0;
      for (const CR &cr : crs) {
        totalPersistent += cr.PersistentBlock.size();
        totalNonPersistent += cr.NotPersistBlock.size();
      }
      outfile << "- Total Persistent Blocks: " << totalPersistent << "\n";
      outfile << "- Total Non-Persistent Blocks: " << totalNonPersistent
              << "\n";

      outfile.close();
    }

    int DpMaxOutNumber(Ceop Urs) {
      std::vector<BlockInfo> SameBlock, lastSameBlock;
      // std::map<int, std::vector<int>> Dp, lastDp;
      // Dp[0] = std::vector<int>(Urs.size(), 0); // ，
      std::map<int, std::map<int, int>> Dp, lastDp;
      // Dp[0] = std::vector<int>(Urs.size(), 0); // ，
      for (int k = 0; k < Urs.size(); k++) {
        Dp[0][k] = 0;
      }
      for (int i = 0; i < crs.size(); ++i) {
        CR cr = crs[i];
        swap(lastSameBlock, SameBlock); // DP
        SameBlock.clear();
        // if (i + 1 < crs.size()) { // CRSameBlock
        // CR cr2 = crs[i + 1];
        for (auto block : cr.NotPersistBlock)
          // for (auto block2 : cr2.NotPersistBlock)
          //   if (block.address ==
          //       block2
          //           .address) {
          //           //////////////////////////////////////////////////////
          //                       /// addr?
          SameBlock.push_back(block);
        // break;
        // }
        // }

        swap(Dp, lastDp); // Dp
        Dp.clear();
        //  Dp.resize(1 << SameBlock.size(), std::vector<int>(Urs.size(), 0))
        for (auto &Sate : lastDp) {
          int j = Sate.first;
          // jblock id
          std::set<unsigned> s;
          for (int id = 0; id < lastSameBlock.size(); ++id)
            if (j & (1 << id))
              s.insert(lastSameBlock[id].address); // Dp addr
          // j'
          int j3 = 0;
          for (int id = 0; id < SameBlock.size(); ++id)
            if (s.count(SameBlock[id].address))
              j3 |= (1 << id); // jblock addr

          // for (int k = 0; k < Urs.size(); ++k) {

          for (auto it = Sate.second.begin(); it != Sate.second.end(); it++) {
            int k = it->first;
            // if (k > 0 && lastDp[j][k] == lastDp[j][k - 1])
            //   continue;
            UR urcopy = UR();
            CR CRcopy = cr;
            std::set<unsigned> scopy = s;
            int totalOut1 = 0, j2 = j3;
            for (int k2 = k; k2 < Urs.size(); ++k2) {
              urcopy.insert(urcopy.end(), Urs[k2].begin(), Urs[k2].end());
              totalOut1 += CRcopy.OutNumber_NotPersistBlock(j2, SameBlock,
                                                            scopy, urcopy);
              std::vector<UR> urs2c;
              urs2c.insert(urs2c.end(), Urs.begin() + k, Urs.begin() + k2 + 1);
              std::vector<int> PersistOut =
                  CRcopy.OutNumber_PersistentBlock_add_with_continue(urs2c);
              int totalOut = 0;
              for (auto val : PersistOut) {
                assert(val >= 0 && "0");
                totalOut += val;
              }
              // // UR,todo....
              // if (j + 1 < Urs.size())
              //   CRcopy.updateCR(PersistOut, Urs[k2]);
              // dp
              if (Dp[j2].count(k2) == 0) {
                Dp[j2][k2] = 0;
                // Dp[j2] = std::vector<int>(Urs.size(), 0); // ，
              }
              Dp[j2][k2] =
                  std::max(Dp[j2][k2], lastDp[j][k] + totalOut1 + totalOut);
            }
          }
        }
      }
      // 
      int maxVal = 0;
      for (auto &states : Dp)
        for (auto &val : states.second)
          maxVal = std::max(maxVal, val.second);
      return maxVal;
    }
  };

private:
  /*get the outmost loop in the same function, so that we can identify an ps
   * scope*/
  /// ctxmiloop，loopvector

  bool isPS(const CtxMI &MI, std::string fname) {
    auto it = Ceopinfos.ctxmi2ps_loop_stack[fname].find(MI);
    std::vector<std::pair<const llvm::MachineLoop *, bool>> st = it->second;
    for (auto &scop : st) {
      if (scop.second) {
        return true;
      }
    }
    return false;
  }

  bool isPS(const CtxData &CtD, std::string fname) {
    auto it = Ceopinfos.ctxdata2ps_loop_stack[fname].find(CtD.data_addr);
    std::vector<std::pair<const llvm::MachineLoop *, bool>> &st = it->second;
    for (auto &scop : st) {
      if (scop.second) {
        return true;
      }
    }
    return false;
  }

  // PS triple
  std::vector<BlockInfo> getPStriple(const CtxMI &MI, std::string fname) {
    std::vector<BlockInfo> res;
    // handling PS access
    // st

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
        st[i].second = false;
        int CS = INT_MAX;
        TimingAnalysisPass::dom::cache::Classification cl;
        for (auto &scop : Cl_infor.AddrPSList) {
          if (scop.first.loop == loop.first) {
            for (const AddrPS &ps : scop.second) {
              if (ps.address.getAsInterval().lower() ==
                  BlockInfo::getCachelineAddress(addr)) {
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
        if (CS != INT_MAX) {
          assert(b != -1);
          if (b > 1) { // 1
            res.emplace_back(BlockInfo(addr, x * (b - 1), -1, cl, CS));
          }
        }
      }
    }
    return res;
  }

  // PS triple
  std::vector<BlockInfo> getPStriple(const CtxData &CtD, std::string fname) {
    std::vector<BlockInfo> res;
    // handling PS access
    // st
    // bug
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
                  BlockInfo::getCachelineAddress(addr)) {
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
        if (CS != INT_MAX) {
          assert(b != -1);
          if (b > 1) { // 1
            res.emplace_back(BlockInfo(addr, x * (b - 1), -1, cl, CS));
          }
        }
      }
    }
    return res;
  }
};

#endif