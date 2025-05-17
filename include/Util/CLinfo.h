#ifndef ZHANG_WEI
#define ZHANG_WEI

#include "Memory/Classification.h"
#include "PartitionUtil/Context.h"
#include "Util/AbstractAddress.h"
#include "Util/PersistenceScope.h"
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <vector>

// const char *ClassificationNames[13] = {
//     "Bot", "Hit",    "Miss", "L1unknow", "L2Hit", "F",        "F",
//     "F",   "L2Miss", "F",    "F",        "F",     "L2unknown"};
class AddrCL {
public:
  AddrCL(TimingAnalysisPass::AbstractAddress addr,
         TimingAnalysisPass::Context ctxa,
         TimingAnalysisPass::dom::cache::Classification cl, int a,
         unsigned aI = 0) {
    this->address = addr;
    this->ctx = ctxa;
    this->CL = cl;
    this->age = a;
    this->MIAddr = aI;
  }
  AddrCL(AddrCL const &o) {
    this->address = o.address;
    this->ctx = o.ctx;
    this->CL = o.CL;
    this->age = o.age;
    this->MIAddr = o.MIAddr;
  }

  TimingAnalysisPass::AbstractAddress address =
      TimingAnalysisPass::AbstractAddress((unsigned)0);
  int age;
  TimingAnalysisPass::dom::cache::Classification CL;
  TimingAnalysisPass::Context ctx;
  unsigned MIAddr;

  // join
  bool operator==(const AddrCL &other) const {
    return this->address == other.address && this->ctx == other.ctx &&
           this->MIAddr == other.MIAddr;
  }
  // 
  bool operator<(const AddrCL &other) const {
    if (this->address < other.address) {
      return true;
    }
    // 
    if (this->ctx < other.ctx) {
      return true;
    }
    if (this->MIAddr < other.MIAddr) {
      return true;
    }
    return false;
  }
  // , 1610，
  std::ostream &print(std::ostream &stream) const {
    stream << "Address: " << std::hex << this->address << " : [" << this->CL
           << "  | age : " << std::dec << this->age << "\nContex: " << ctx
           << "|" << std::hex << this->MIAddr << std::dec << "]\n";
    return stream;
  }
  bool join(AddrCL &other) {
    if (*this == other) {
      if (this->CL < other.CL) {
        this->CL = other.CL;
        this->age = other.age;
      }
    } else {
      std::cerr << "Addresses and contexts that do not match cannot be merged";
      return false;
    }
  }
};

class AddrPS {
public:
  AddrPS(TimingAnalysisPass::AbstractAddress addr, int a, int l)
      : address(addr), CS_size(a), LEVEL(l) {}
  AddrPS(){};
  TimingAnalysisPass::AbstractAddress address =
      TimingAnalysisPass::AbstractAddress((unsigned)0);
  int CS_size; // |CS|
  int LEVEL;   // cache level

  bool operator==(const AddrPS &other) const {
    return this->address == other.address && this->LEVEL == other.LEVEL;
  }

  bool operator<(const AddrPS &other) const {
    if (this->address < other.address) {
      return true;
    }
    if (this->LEVEL < other.LEVEL) {
      return true;
    }
    // if (this->CS_size < other.CS_size) {
    //   return true;
    // }
    return false;
  }
  bool join(AddrPS &other) {
    if (*this == other) {
      if (this->CS_size < other.CS_size)
        this->CS_size = other.CS_size;
    } else {
      std::cerr << "Addresses and contexts that do not match cannot be merged";
      return false;
    }
  }

  // bool join(AddrPS other) {
  //   if (*this == other) {
  //     this->CS_size =
  //         this->CS_size > other.CS_size ? this->CS_size : other.CS_size;
  //   } else {
  //     std::cerr << "Address that do not match cannot be merged";
  //     return false;
  //   }
  // }
  // 
};
inline std::ostream &operator<<(std::ostream &stream, const AddrPS &ads) {
  stream << "[ Address : " << std::hex << ads.address << std::dec << " | L"
         << ads.LEVEL << " | " << ads.CS_size << "]\n";
  return stream;
}

class CL_info {
public:
  std::vector<AddrCL> AddrCList;
  std::map<std::string, std::set<TimingAnalysisPass::PersistenceScope>>
      fun2scope;
  std::map<TimingAnalysisPass::PersistenceScope, std::set<AddrPS>> AddrPSList;
  void CL_clean() {
    std::map<TimingAnalysisPass::PersistenceScope, std::set<AddrPS>>
        AddrPSList_clean;
    // 
    for (auto &entry : AddrCList) {
      if (entry.CL == TimingAnalysisPass::dom::cache::CL_BOT) {
        entry.CL = TimingAnalysisPass::dom::cache::CL_HIT;
      } else if ((entry.CL != TimingAnalysisPass::dom::cache::CL2_MISS) &&
                 entry.age == INT_MAX) {
        entry.CL = TimingAnalysisPass::dom::cache::CL2_MISS;
      }
    }
    for (const auto &entry : AddrPSList) {
      for (auto &a : entry.second) {
        if (a.LEVEL == 2) {
          bool t = true;
          for (auto &b : entry.second) {
            if (a.address == b.address && b.LEVEL == 1) {
              t = false;
              break;
            }
          }
          if (t) {
            AddrPSList_clean[entry.first].insert(a);
          }
        } else {
          AddrPSList_clean[entry.first].insert(a);
        }
      }
    }
    std::ofstream myfile;
    myfile.open("Clist_clean.txt", std::ios_base::trunc);
    for (const auto &entry : AddrCList) {
      entry.print(myfile);
    }
    for (const auto &entry : AddrPSList_clean) {
      myfile << "Scop:" << entry.first << "\n";
      for (auto &a : entry.second) {
        myfile << a;
      }
    }
    myfile.close();
    this->AddrPSList = AddrPSList_clean;
  }

  void insert_PS(const TimingAnalysisPass::PersistenceScope &s, AddrPS &aps) {
    auto &addr_set = AddrPSList[s]; //  map
    auto it = addr_set.find(aps);   // 

    if (it != addr_set.end()) {
      AddrPS old_aps = *it; // 
      addr_set.erase(it);   // 
      aps.join(old_aps);    // 
    }

    addr_set.insert(aps); // 
  }

  void insert_CL(AddrCL &acl) {
    int i = 0;
    for (; i < AddrCList.size(); i++) {
      if (AddrCList[i] == acl) {
        break;
      }
    }
    if (i != AddrCList.size()) {
      //  acl
      AddrCList[i].join(acl);
    } else {
      AddrCList.emplace_back(acl); // 
    }
  }

  std::ostream &print(std::ostream &stream) const {
    int i = 0;
    for (const auto &entry : AddrCList) {
      stream << "(" << i++ << ")";
      entry.print(stream);
    }
    for (const auto &entry : AddrPSList) {
      stream << "Scop:" << entry.first << "\n";
      for (auto &a : entry.second) {
        stream << a;
      }
    }
  }
};

#endif