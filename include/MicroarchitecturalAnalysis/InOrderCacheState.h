////////////////////////////////////////////////////////////////////////////////
//
//   LLVMTA - Timing Analyser performing worst-case execution time analysis
//     using the LLVM backend intermediate representation
//
// Copyright (C) 2013-2022  Saarland University
// Copyright (C) 2014-2015 Claus Faymonville
//
// This file is distributed under the Saarland University Software Release
// License. See LICENSE.TXT for details.
//
// THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY EXPRESSED OR IMPLIED
// WARRANTIES, INCLUDING BUT NOT LIMITED TO, THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE SAARLAND UNIVERSITY, THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING BUT NOT LIMITED TO PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED OR OTHER LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, TORT OR OTHERWISE, ARISING IN ANY WAY FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH A DAMAGE.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef INORDERCACHESTATE_H
#define INORDERCACHESTATE_H

#include "MicroarchitecturalAnalysis/MicroArchitecturalState.h"
#include "PreprocessingAnalysis/AddressInformation.h"
#include "Util/Util.h"

#include "Memory/AbstractCache.h"

#include "ARM.h"
#include "llvm/Support/Debug.h"

#include <iostream>
#include <set>

#include <boost/optional/optional.hpp>

namespace TimingAnalysisPass {

/**
 * Class implementing the cycle-based semantics of a simple processor with
 * a five-stage pipeline.
 * The resulting analysis is basically a counter-based microarchitectural
 * analysis.
 */
template <dom::cache::AbstractCache *(*makeCache)(bool), bool dataCache>
class InOrderCacheState
    : public MicroArchitecturalState<
          InOrderCacheState<makeCache, dataCache>,
          std::tuple<InstrContextMapping &, AddressInformation &>> {
public:
  // define SuperClass for convenience
  typedef MicroArchitecturalState<
      InOrderCacheState<makeCache, dataCache>,
      std::tuple<InstrContextMapping &, AddressInformation &>>
      SuperClass;

  typedef typename SuperClass::StateSet StateSet;
  void getACL() const {};
  void getACL(Context ctx) const {};
  template <typename T>
  using MapFromStates = typename SuperClass::template MapFromStates<T>;

  /**
   * Constructor. Creates a cache microarchitectural state for program location
   * pl.
   */
  explicit InOrderCacheState(ProgramLocation &pl)
      : SuperClass(pl), cachestate(makeCache(false)), missedCache(),
        finishedInstruction(boost::none) {}

  /**
   * Copy Constructor
   */
  explicit InOrderCacheState(const InOrderCacheState &cs)
      : SuperClass(cs), cachestate(cs.cachestate->clone()),
        missedCache(cs.missedCache),
        finishedInstruction(cs.finishedInstruction) {}

  /**
   * Virtual destructor
   */
  virtual ~InOrderCacheState() { delete cachestate; }

  /**
   * Container used to make the local metrics of this class
   * available to the world outside.
   */
  struct LocalMetrics : SuperClass::LocalMetrics {
    // shorthand for the base class
    typedef typename SuperClass::LocalMetrics LocalMetricsBase;
    /// Is this metrics for instruction or data cache
    static const bool dCache = dataCache;
    /// Contains the accessed intervals that might have missed the cache
    std::list<AbstractAddress> missedCache;

    /**
     * Creates the local metrics based on the
     * outer class' content.
     */
    LocalMetrics(const InOrderCacheState &outerClassInstance)
        : LocalMetricsBase(outerClassInstance),
          missedCache(outerClassInstance.missedCache) {}

    /**
     * Checks for equality between local metrics.
     */
    bool operator==(const LocalMetrics &anotherInstance) {
      return LocalMetricsBase::operator==(anotherInstance) &&
             missedCache == anotherInstance.missedCache;
    }
  };

  /**
   * Resets the local metrics to their initial values.
   */
  void resetLocalMetrics() {
    SuperClass::resetLocalMetrics();
    missedCache.clear(); // Clear the list of potential misses
  }

  /**
   * See superclass first.
   * Produces all possible successor cache states by executing the current
   * instruction. Important note: A cycle on a current state takes all cache
   * accesses into account that are (possibly speculatively) done until the next
   * instruction is known to definitely execute. Thus speculative accesses are
   * done with the last non-sepculative access.
   */
  virtual StateSet
  cycle(std::tuple<InstrContextMapping &, AddressInformation &> &dep) const;

  /**
   * See superclass first.
   * Checks whether a ProgramLocation left the pipeline after the last cycle.
   */
  virtual bool isFinal(ExecutionElement &pl) {
    bool finalFlag = false;
    if (StaticAddrProvider->goesExternal(pl.first)) {
      finalFlag =
          finishedInstruction && pl.first == finishedInstruction.get().first;
    } else {
      finalFlag = finishedInstruction &&
                  pl.first == finishedInstruction.get().first &&
                  pl.second == finishedInstruction.get().second;
    }
    if (finalFlag) {
      finishedInstruction = boost::none;
    }
    return finalFlag;
  }

  /// \see superclass
  bool operator==(const InOrderCacheState &cs) const {
    if (finishedInstruction != cs.finishedInstruction ||
        (finishedInstruction &&
         finishedInstruction.get() != cs.finishedInstruction.get())) {
      return false;
    }
    return SuperClass::operator==(cs) &&
           cachestate->lessequal(*cs.cachestate) &&
           cs.cachestate->lessequal(*cachestate) &&
           cs.missedCache == missedCache;
  }

  /// \see superclass
  virtual size_t hashcode() const {
    size_t val = SuperClass::hashcode();
    if (finishedInstruction) {
      hash_combine(val, finishedInstruction.get());
    }
    // TODO cannot hash caches yet
    for (AbstractAddress addr : missedCache) {
      AddressInterval itv = addr.getAsInterval();
      hash_combine(val, itv.upper());
      hash_combine(val, itv.lower());
    }
    return val;
  }

  /// \see superclass
  virtual bool isJoinable(const InOrderCacheState &cs) const {
    if (finishedInstruction != cs.finishedInstruction ||
        (finishedInstruction &&
         finishedInstruction.get() != cs.finishedInstruction.get())) {
      return false;
    }
    return SuperClass::isJoinable(cs)
           // && Caches are always joinable
           && missedCache == cs.missedCache;
  }

  /// \see superclass
  virtual void join(const InOrderCacheState &cs) {
    assert(isJoinable(cs) && "Cannot join non-joinable states.");
    SuperClass::join(cs);
    this->cachestate->join(*cs.cachestate);
    // missedCache must be equal anyway
  }

  // Output operation
  template <dom::cache::AbstractCache *(*makeCachef)(bool), bool dataCachef>
  friend std::ostream &
  operator<<(std::ostream &stream,
             const InOrderCacheState<makeCachef, dataCachef> &cs);

private:
  /**
   * The cache in this current microarchitectural cache state.
   */
  dom::cache::AbstractCache *cachestate;

  /**
   * Accesses to the listed address intervals might have been misses in the
   * current basic block.
   */
  std::list<AbstractAddress> missedCache;

  boost::optional<ExecutionElement> finishedInstruction;

  /**
   * Speculates up to two addresses after the current address.
   * Possible states are joined and returned.
   */
  StateSet speculate(unsigned currAddress);

  /**
   * Takes a state and accesses the cache once
   * Returns the resulting states.
   * This function performs speculative loads. Do not use it for
   * speculative stores if you want to add them.
   */
  void accessSpeculation(unsigned address);

  /**
   * Executes a data memory access on the state.
   */
  void performDMemAccess(AbstractAddress addr, AccessType load_store);
};

template <dom::cache::AbstractCache *(*makeCache)(bool), bool dataCache>
void InOrderCacheState<makeCache, dataCache>::performDMemAccess(
    AbstractAddress addrItv, AccessType load_store) {
  dom::cache::Classification cl = cachestate->classify(addrItv);
  dom::cache::WritePolicy policy = cachestate->getWritePolicy();

  assert(!(policy.WriteBack && !policy.WriteAllocate) &&
         "Writeback caches without write-allocate not supported");

  if (load_store == AccessType::STORE && !policy.WriteAllocate) {
    if (cl == dom::cache::CL_HIT)
      cachestate->update(addrItv, load_store);
    else if (cl == dom::cache::CL_UNKNOWN) {
      /* We don't know whether the cache should be updated or not. Do
       * a split and join the results to get a safe
       * overapproximation */
      InOrderCacheState notUpdated(*this);
      cachestate->update(addrItv, load_store);
      this->join(notUpdated);
    }
  } else {
    cachestate->update(addrItv, load_store);
    if (cl == dom::cache::CL_MISS || cl == dom::cache::CL_UNKNOWN)
      missedCache.push_back(addrItv);
  }

  /* A miss on a writeback-cache might trigger a writeback */
  if (policy.WriteBack) {
    if (cl == dom::cache::CL_MISS || cl == dom::cache::CL_UNKNOWN)
      missedCache.push_back(AbstractAddress::getUnknownAddress());
  }
}

template <dom::cache::AbstractCache *(*makeCache)(bool), bool dataCache>
typename InOrderCacheState<makeCache, dataCache>::StateSet
InOrderCacheState<makeCache, dataCache>::cycle(
    std::tuple<InstrContextMapping &, AddressInformation &> &dep) const {
  // Copy this state for the successor
  InOrderCacheState succ(*this);
  ++succ.time;

  auto fetchedElement = succ.pc.fetchNextInstruction();
  assert((StaticAddrProvider->goesExternal(fetchedElement.first) ||
          StaticAddrProvider->hasMachineInstrByAddr(fetchedElement.first)) &&
         "Non-program instruction");
  succ.finishedInstruction = fetchedElement;
  auto currInstrAddr = fetchedElement.first;

  const MachineInstr *currInstr;
  if (!StaticAddrProvider->goesExternal(currInstrAddr)) {
    currInstr = StaticAddrProvider->getMachineInstrByAddr(currInstrAddr);
  }

  // Cache update on succ with help of fetchedElement
  auto &addrInfo = std::get<1>(dep);

  if (dataCache) {
    // Data cache updates
    if (!StaticAddrProvider->goesExternal(currInstrAddr) &&
        (currInstr->mayLoad() || currInstr->mayStore())) {
      auto currCtx = fetchedElement.second;
      auto numOfAcc = addrInfo.getNumOfDataAccesses(currInstr);
      // TODO revisit trhis assertion with prefetching in mind.
      // assert(!(currInstr->mayStore() && currInstr->mayLoad()));
      for (unsigned i = 0; i < numOfAcc; ++i) {
        auto addrItv = addrInfo.getDataAccessAddress(currInstr, &currCtx, i);
        AccessType type =
            currInstr->mayLoad() ? AccessType::LOAD : AccessType::STORE;
        succ.performDMemAccess(addrItv, type);
      }
    }
  } else {
    // Instruction cache updates
    AbstractAddress addr(currInstrAddr);
    // Do classification
    auto cl = succ.cachestate->classify(addr);
    if (cl == dom::cache::CL_MISS || cl == dom::cache::CL_UNKNOWN) {
      succ.missedCache.push_back(addr);
    }
    succ.cachestate->update(addr, AccessType::LOAD);
  }

  // result set for this cycle method
  StateSet tmpres;

  // Handle additional accesses that are based on speculation
  if (!StaticAddrProvider->goesExternal(currInstrAddr) &&
      (currInstr->isBranch() || currInstr->isCall() || currInstr->isReturn())) {
    auto alternativeSucc =
        succ.handleBranching(fetchedElement, std::get<0>(dep));
    // Speculation in case of an instruction cache
    if (!dataCache) {
      /**
       * Speculation
       *
       * Speculated instruction accesses is defined as accesses which are
       * discarded later. Correctly speculated accesses are not handled here, as
       * they are processed in later cycles already.
       *
       * The number of speculations after a branch is guaranteed to be in the
       * range of 1-2 instructions. Therefore, we speculate twice and join all
       * resulting states after the first and second speculation.
       *
       * In case of a return or call, speculate in all states.
       * In case of a branch, speculate only in the "TAKEN" case, "NOT TAKEN" is
       * contained in alternativeSucc.
       */

      if (currInstr->isBranch()) {
        // for all branch instructions, account for misspeculation with succ
        // state
        auto altSpecS = succ.speculate(currInstrAddr);
        tmpres.insert(altSpecS.begin(), altSpecS.end());
      }
    }
    // add alternative successors when analysis data cache
    tmpres.insert(alternativeSucc.begin(), alternativeSucc.end());
  }

  tmpres.insert(succ);

  if (!needPersistenceScopes()) {
    DEBUG_WITH_TYPE(
        "driverSED", for (auto &succ
                          : tmpres) { std::cerr << succ; });
    return tmpres;
  } else {
    StateSet res;
    for (const auto &stateref : tmpres) {
      InOrderCacheState state(stateref);
      if (StaticAddrProvider->hasMachineInstrByAddr(state.pc.getPc().first)) {
        auto nextInstr =
            StaticAddrProvider->getMachineInstrByAddr(state.pc.getPc().first);
        auto edge =
            std::make_pair(currInstr->getParent(), nextInstr->getParent());
        // Do persistence scope entering
        if (edge.first != edge.second) {
          DEBUG_WITH_TYPE("persistence",
                          dbgs()
                              << "We see edge (BB" << edge.first->getNumber()
                              << ", BB" << edge.second->getNumber() << ")\n");
          if (PersistenceScopeInfo::getInfo().entersScope(edge)) {
            for (auto scope :
                 PersistenceScopeInfo::getInfo().getEnteringScopes(edge)) {
              DEBUG_WITH_TYPE("persistence",
                              dbgs() << "We are going to enter a scope: "
                                     << scope.getId() << "\n");
              state.cachestate->enterScope(scope);
            }
          }
          // Do persistence scope leaving
          if (PersistenceScopeInfo::getInfo().leavesScope(edge)) {
            for (auto scope :
                 PersistenceScopeInfo::getInfo().getLeavingScopes(edge)) {
              DEBUG_WITH_TYPE("persistence",
                              dbgs() << "We are going to leave a scope: "
                                     << scope.getId() << "\n");
              state.cachestate->leaveScope(scope);
            }
          }
        }
      }
      res.insert(state);
    }

    DEBUG_WITH_TYPE("driverSED", for (auto &succ : res) { std::cerr << succ; });
    return res;
  }
}

template <dom::cache::AbstractCache *(*makeCachef)(bool), bool dataCachef>
std::ostream &operator<<(std::ostream &stream,
                         const InOrderCacheState<makeCachef, dataCachef> &cs) {
  auto &base =
      (const MicroArchitecturalState<
          InOrderCacheState<makeCachef, dataCachef>,
          typename InOrderCacheState<makeCachef, dataCachef>::StateDep> &)cs;
  stream << base << "\n";
  stream << *cs.cachestate << "\n";

  stream << "{";
  bool emitComma = false;
  for (auto &itv : cs.missedCache) {
    if (emitComma)
      stream << ", ";
    stream << itv;
    emitComma = true;
  }
  stream << "}\n";
  if (cs.finishedInstruction) {
    stream << cs.finishedInstruction.get();
  }
  return stream;
}

template <dom::cache::AbstractCache *(*makeCache)(bool), bool dataCache>
typename InOrderCacheState<makeCache, dataCache>::StateSet
InOrderCacheState<makeCache, dataCache>::speculate(unsigned currAddress) {
  StateSet res;

  unsigned addrSpec1 = currAddress + 4;
  unsigned addrSpec2 = currAddress + 8;

  accessSpeculation(addrSpec1);

  InOrderCacheState spec2State(*this);
  spec2State.accessSpeculation(addrSpec2);

  if (isJoinable(spec2State)) {
    join(spec2State);
  } else {
    res.insert(spec2State);
  }
  return res;
}

template <dom::cache::AbstractCache *(*makeCache)(bool), bool dataCache>
void InOrderCacheState<makeCache, dataCache>::accessSpeculation(
    unsigned address) {
  // Instruction cache updates
  AbstractAddress addr(address);
  // Do classification
  auto cl = cachestate->classify(addr);
  if (cl == dom::cache::CL_MISS || cl == dom::cache::CL_UNKNOWN) {
    missedCache.push_back(addr);
  }
  /* We only speculate on instructions, never on data. */
  cachestate->update(addr, AccessType::LOAD);
}

} // namespace TimingAnalysisPass

#endif
