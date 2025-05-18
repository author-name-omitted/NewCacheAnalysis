////////////////////////////////////////////////////////////////////////////////
//
//   LLVMTA - Timing Analyser performing worst-case execution time analysis
//     using the LLVM backend intermediate representation
//
// Copyright (C) 2013-2022  Saarland University
// Copyright (C) 2014-2015  Claus Faymonville
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

#ifndef OSEPARATECACHESMEMORYTOPOLOGY_H
#define OSEPARATECACHESMEMORYTOPOLOGY_H

#include "llvm/Support/Debug.h"

#include "Memory/AbstractCache.h"
#include "Memory/Classification.h"
#include "Memory/MemoryTopologyInterface.h"
#include "MicroarchitecturalAnalysis/InOrderPipelineState.h"
#include "Util/GlobalVars.h"
#include "Util/Options.h"

#include <climits>
#include <cstddef>
#include <list>

namespace TimingAnalysisPass {

using namespace dom::cache;

/**
 * Class representing a memory topology with separate caches for both
 * instruction and data cache, plus a background memory for cache misses. The
 * background memory has to be another memory topology.
 */
template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
class OSeparateCachesMemoryTopology
    : public MemoryTopologyInterface<OSeparateCachesMemoryTopology<
          makeInstrCache, makeDataCache, makeL2Cache, BgMem>> {
public:
  /**
   * Public Default Contructor
   */
  OSeparateCachesMemoryTopology();

  /**
   * Copy Constructor
   */
  OSeparateCachesMemoryTopology(const OSeparateCachesMemoryTopology &scmt);

  /**
   * Assignment operator
   */
  OSeparateCachesMemoryTopology &
  operator=(const OSeparateCachesMemoryTopology &scmt);

  /**
   * Destructor
   */
  ~OSeparateCachesMemoryTopology() {}

  /**
   * Container used to make the local metrics of this class
   * available to the world outside.
   */
  struct LocalMetrics {
    // the fields added to the local metrics used as base
    // The access to the address - if any - just missed the cache
    boost::optional<AbstractAddress> justMissedInstrCache;
    // The access to the address - if any - just missed the cache
    boost::optional<AbstractAddress> justMissedDataCache;
    boost::optional<AbstractAddress> justMissedL2Cache;
    // The persistence scope just entered
    boost::optional<std::set<PersistenceScope>> justEntered;
    // The access - if any - that updates the I-cache repl policy state
    boost::optional<AbstractAddress> justUpdatedInstrCache;
    // The access - if any - that updates the D-cache repl policy state
    boost::optional<AbstractAddress> justUpdatedDataCache;
    boost::optional<AbstractAddress> justDirtifiedLine;
    bool justWroteBackLine;
    // Number of misses to instruction cache
    unsigned instrMisses;
    // Number of misses to data cache
    unsigned dataMisses, l2Misses;
    // Number of stores to the bus
    unsigned storesToBus;
    // Reference to the instruction cache
    AbstractCache *instrCache;
    // Reference to the data cache
    AbstractCache *dataCache;
    AbstractCache *l2Cache;
    // Local Metrics of the internally wrapped memory topology
    typename BgMem::LocalMetrics backgroundMemory;

    /**
     * Creates the local metrics based on the
     * outer class' content.
     */
    LocalMetrics(const OSeparateCachesMemoryTopology &outerClassInstance)
        : justMissedInstrCache(
              outerClassInstance.instructionComponent.justMissedCache),
          justMissedDataCache(outerClassInstance.dataComponent.justMissedCache),
          justMissedL2Cache(outerClassInstance.L2Component.justMissedCache),
          justEntered(outerClassInstance.justEntered),
          justUpdatedInstrCache(
              outerClassInstance.instructionComponent.justUpdatedCache),
          justUpdatedDataCache(
              outerClassInstance.dataComponent.justUpdatedCache),
          justDirtifiedLine(outerClassInstance.dataComponent.justDirtifiedLine),
          justWroteBackLine(outerClassInstance.dataComponent.justWroteBackLine),
          instrMisses(outerClassInstance.instructionComponent.nmisses),
          dataMisses(outerClassInstance.dataComponent.nmisses),
          l2Misses(outerClassInstance.L2Component.nmisses),
          // dataComponent
          storesToBus(outerClassInstance.dataComponent.numStoreBusAccess),
          instrCache(outerClassInstance.instructionComponent.cache->clone()),
          dataCache(outerClassInstance.dataComponent.cache->clone()),
          l2Cache(outerClassInstance.L2Component.cache->clone()),
          backgroundMemory(outerClassInstance.memory) {}

    ~LocalMetrics() {
      delete instrCache;
      delete dataCache;
      delete l2Cache;
    }

    /**
     * Checks for equality between local metrics.
     */
    bool operator==(const LocalMetrics &anotherInstance) {
      return justMissedInstrCache == anotherInstance.justMissedInstrCache &&
             justMissedDataCache == anotherInstance.justMissedDataCache &&
             justMissedL2Cache == anotherInstance.justMissedL2Cache &&
             justEntered == anotherInstance.justEntered &&
             justUpdatedInstrCache == anotherInstance.justUpdatedInstrCache &&
             justUpdatedDataCache == anotherInstance.justUpdatedDataCache &&
             justDirtifiedLine == anotherInstance.justDirtifiedLine &&
             justWroteBackLine == anotherInstance.justWroteBackLine &&
             instrMisses == anotherInstance.instrMisses &&
             dataMisses == anotherInstance.dataMisses &&
             // l2
             l2Misses == anotherInstance.l2Misses &&
             storesToBus == anotherInstance.storesToBus &&
             instrCache->equals(*anotherInstance.instrCache) &&
             dataCache->equals(*anotherInstance.dataCache) &&
             l2Cache->equals(*anotherInstance.l2Cache) &&
             backgroundMemory == anotherInstance.backgroundMemory;
    }
  };

  /**
   * Resets the local metrics to their initial values.
   */
  void resetLocalMetrics() {
    instructionComponent.justMissedCache = boost::none;
    instructionComponent.justUpdatedCache = boost::none;
    justEntered = boost::none;
    instructionComponent.nmisses = 0;

    instructionComponent.numStoreBusAccess = 0;
    dataComponent.justMissedCache = boost::none;
    dataComponent.justUpdatedCache = boost::none;
    dataComponent.justDirtifiedLine = boost::none;
    dataComponent.justWroteBackLine = false;
    dataComponent.nmisses = 0;

    dataComponent.numStoreBusAccess = 0;

    L2Component.justMissedCache = boost::none;
    L2Component.justUpdatedCache = boost::none;
    L2Component.justDirtifiedLine = boost::none;
    L2Component.justWroteBackLine = false;
    L2Component.nmisses = 0;

    L2Component.numStoreBusAccess = 0;
    memory.resetLocalMetrics();
  }

  /**
   * Accesses the instruction with the given address.
   * This may override a previous access.
   * Returns an id.
   */
  virtual boost::optional<unsigned> accessInstr(unsigned addr,
                                                unsigned numWords);
  virtual boost::optional<unsigned>
  accessL2(unsigned addr, AccessType load_store, unsigned numWords);

  boost::optional<unsigned> accessInstrWithCtx(unsigned addr, unsigned numWords,
                                               Context ctx);

  /**
   * Accesses data at the given address.
   * Returns an id or none if the access cannot be processed.
   */
  virtual boost::optional<unsigned>
  accessData(AbstractAddress addr, AccessType load_store, unsigned numWords);

  /**
   * Cycles the topology once.
   * This includes cycling all memory modules inside.
   * The bool parameter says whether there are potential data misses pending
   * between the fetch and memory phase. A strictly in-order pipeline requires
   * stalling fetch misses. It is passed to the underlying topology.
   */
  virtual std::list<OSeparateCachesMemoryTopology>
  cycle(bool potentialDataMissesPending) const;

  /**
   * Ask the underlying memory topology to stall during this cycle.
   */
  virtual bool shouldPipelineStall() const;

  /**
   * Checks whether an instruction access finished.
   */
  virtual bool finishedInstrAccess(unsigned accessId);

  /**
   * Checks whether a data access finished.
   */
  virtual bool finishedDataAccess(unsigned accessId);

  /// See superclass
  virtual void enterScope(const PersistenceScope &scope);
  virtual void leaveScope(const PersistenceScope &scope);

  virtual bool hasUnfinishedAccesses() const;

  /**
   * Checks whether this topology is the same as the given one.
   */
  virtual bool operator==(const OSeparateCachesMemoryTopology &scmt) const;

  /**
   * Hashes the topology.
   */
  virtual size_t hashcode() const;

  /**
   * Is the memory topology waiting to be joined with similar topologies?
   */
  virtual bool isWaitingForJoin() const;

  virtual bool isJoinable(const OSeparateCachesMemoryTopology &scmt) const;

  virtual void join(const OSeparateCachesMemoryTopology &scmt);

  /**
   * Is the memory topology currently performing an instruction access?
   */
  virtual bool isBusyAccessingInstr() const;
  boost::optional<std::tuple<AbstractAddress, Classification, int>>
  getIaccAdress() const;
  boost::optional<std::tuple<AbstractAddress, Classification, int>>
  getDaccAdress() const;

  boost::optional<std::tuple<AbstractAddress, Classification, int, Context>>
  getIaccAddressWithCtx() const;

  /**
   * Is the memory topology currently performing a data access?
   */
  virtual bool isBusyAccessingData() const;

  /**
   * Fast-forwarding of the memory topology.
   */
  virtual std::list<OSeparateCachesMemoryTopology> fastForward() const;

  /**
   * Outputs the data content of this topology.
   */
  template <AbstractCache *(*makeInstrCachef)(bool),
            AbstractCache *(*makeDataCachef)(bool),
            AbstractCache *(*makeL2Cachef)(bool), class BgMemf>
  friend std::ostream &operator<<(
      std::ostream &stream,
      const OSeparateCachesMemoryTopology<makeInstrCachef, makeDataCachef,
                                            makeL2Cachef, BgMemf> &scmt);

private:
  typedef MemoryTopologyInterface<OSeparateCachesMemoryTopology> Super;

  typedef typename Super::Access Access;

  /**
   * Increments the current id.
   * Prevents '0' being used as current id, as this is used to represent
   * nothing.
   */
  void incrementCurrentId();

  /**
   * Trigger an access if currently no access is ongoing
   */
  std::list<OSeparateCachesMemoryTopology> startInstructionAccess();
  std::list<OSeparateCachesMemoryTopology> mystartInstructionAccess();
  std::list<OSeparateCachesMemoryTopology> startDataAccess();
  std::list<OSeparateCachesMemoryTopology> mystartDataAccess();

  /**
   * According to the given classification perform action in the memory topology
   */
  void processInstrCacheAccess(Classification cl);
  void processDataCacheAccess(Classification cl);

  /**
   * Check whether accesses to cache or background memory are finished
   */
  std::list<OSeparateCachesMemoryTopology> checkInstructionPart();
  std::list<OSeparateCachesMemoryTopology> checkDataPart();

  // Reference to the memory topology that is accessed whenever a cache miss
  // occurs. Has to be derived by MemoryTopology.
  BgMem memory;

  /**
   * Counter to provide a new identifier for each access.
   * Should start counting with 1, cf. currentIdAccessed.
   */
  unsigned currentId;

  /**
   * The different phases an access can be in (for the general case)
   */
  enum class AccessPhase {
    WaitForSTART, // We are waiting for the access to start
    WaitForLOAD, // We are waiting for a load from background memory topology to
                 // finish
    WaitForCACHE, // We are waiting for the cache to finish its update
    WaitForSTORE  // We are waiting for a store from background memory topology
                  // to finish
  };

  /**
   * One ongoing access
   */
  struct OngoingAccess {
    /// Stores the id currently being accessed by the topology.
    Access access;
    /// The access phase of the current access
    AccessPhase phase;
    /// The background memory could not take the access as it was busy, try
    /// again
    bool bgmemStall;
    /// Stores the id currently accessed in the background memory topology.
    unsigned bgMemAccessId;
    /// Stores the id currently accessed in the l2 topology.
    unsigned bgl2AccessId;
    /// Blocking counter to wait until a cache access is finished.
    unsigned l1timeBlocked, l2timeBlocked;
    /// Classification of this access by the cache
    Classification cl;
    int age1;
    int age2;

    // Constructor
    OngoingAccess(Access acc)
        : access(acc), phase(AccessPhase::WaitForSTART), bgmemStall(false),
          bgMemAccessId(0), bgl2AccessId(0), l1timeBlocked(0), l2timeBlocked(0),
          cl(CL_BOT), age1(-1), age2(-1) {}
  };

  struct MemoryComponent {
    /// Queue holding future accesses
    std::list<Access> waitingQueue;
    /// The on-going access
    boost::optional<OngoingAccess> ongoingAccess;
    /// Stores the id of the finished access
    unsigned finishedId;
    /// The abstract cache
    AbstractCache *cache;
    // Remember the access we just performed to update the cache
    boost::optional<AbstractAddress> justUpdatedCache;
    // Remember which address we have just missed in cache - if any
    boost::optional<AbstractAddress> justMissedCache;
    // true if the last access dirtified the line it accessed.
    boost::optional<AbstractAddress> justDirtifiedLine;
    // true if the last access wrote back a line
    bool justWroteBackLine;
    // Collect the numer of misses we encountered in this basic block
    unsigned nmisses;
    // Collect the number of stores that access the bus in this basic block
    unsigned numStoreBusAccess;

    // Constructor
    MemoryComponent(AbstractCache *cache)
        : waitingQueue(), ongoingAccess(boost::none), finishedId(0),
          cache(cache), justUpdatedCache(boost::none),
          justMissedCache(boost::none), justDirtifiedLine(boost::none),
          justWroteBackLine(false), numStoreBusAccess(0), nmisses(0) {}
    // Copy constructor
    MemoryComponent(const MemoryComponent &mc)
        : waitingQueue(mc.waitingQueue), ongoingAccess(mc.ongoingAccess),
          finishedId(mc.finishedId), cache(mc.cache->clone()),
          justUpdatedCache(mc.justUpdatedCache),
          justMissedCache(mc.justMissedCache),
          justDirtifiedLine(mc.justDirtifiedLine),
          justWroteBackLine(mc.justWroteBackLine), nmisses(mc.nmisses),
          numStoreBusAccess(mc.numStoreBusAccess) {}
    // Assignment operator
    MemoryComponent &operator=(const MemoryComponent &mc) {
      waitingQueue = mc.waitingQueue;
      ongoingAccess = mc.ongoingAccess;
      finishedId = mc.finishedId;
      delete cache;
      cache = mc.cache->clone();
      justUpdatedCache = mc.justUpdatedCache;
      justMissedCache = mc.justMissedCache;
      justDirtifiedLine = mc.justDirtifiedLine;
      justWroteBackLine = mc.justWroteBackLine;

      numStoreBusAccess = mc.numStoreBusAccess;
      nmisses = mc.nmisses;
      return *this;
    }
    // Destructor
    ~MemoryComponent() { delete cache; }
    // Equals operator
    bool equals(const MemoryComponent &mc, unsigned myId, unsigned mcId) const {
      return cache->lessequal(*mc.cache) && mc.cache->lessequal(*cache) &&
             // do not compare justMissed because if they differ, the caches
             // should differ as well
             justDirtifiedLine == mc.justDirtifiedLine &&
             justWroteBackLine == mc.justWroteBackLine &&

             numStoreBusAccess == mc.numStoreBusAccess &&
             nmisses == mc.nmisses && equalsWithoutCache(mc, myId, mcId);
    }
    bool equalsWithoutCache(const MemoryComponent &mc, unsigned myId,
                            unsigned mcId) const {
      return Super::areQueuesEqual(waitingQueue, mc.waitingQueue, myId, mcId) &&
             ((!ongoingAccess && !mc.ongoingAccess) ||
              (ongoingAccess && mc.ongoingAccess &&
               Super::areAccessElementsEqual(ongoingAccess.get().access,
                                             mc.ongoingAccess.get().access,
                                             myId, mcId) &&
               // do only a "soft" compare of the background topology ids, since
               // they are relative to an unknown value
               ((ongoingAccess.get().bgMemAccessId != 0 &&
                 mc.ongoingAccess.get().bgMemAccessId != 0) ||
                (ongoingAccess.get().bgMemAccessId == 0 &&
                 mc.ongoingAccess.get().bgMemAccessId == 0)) &&
               ((ongoingAccess.get().bgl2AccessId != 0 &&
                 mc.ongoingAccess.get().bgl2AccessId != 0) ||
                (ongoingAccess.get().bgl2AccessId == 0 &&
                 mc.ongoingAccess.get().bgl2AccessId == 0)) &&
               ongoingAccess.get().l1timeBlocked ==
                   mc.ongoingAccess.get()
                       .l1timeBlocked && // l2timeblocked
               ongoingAccess.get().l2timeBlocked ==
                   mc.ongoingAccess.get().l2timeBlocked &&
               ongoingAccess.get().phase == mc.ongoingAccess.get().phase &&
               ongoingAccess.get().bgmemStall ==
                   mc.ongoingAccess.get().bgmemStall &&
               ongoingAccess.get().cl == mc.ongoingAccess.get().cl)) &&
             Super::areAccessIdsEqual(finishedId, mc.finishedId, myId, mcId) &&
             // If we keep the accessed addresses, we need them precisely
             justUpdatedCache == mc.justUpdatedCache &&
             /* If these field differs we shouldn't join or
              * the dfs-bound gets weaker */
             justWroteBackLine == mc.justWroteBackLine &&
             justDirtifiedLine == mc.justDirtifiedLine;
    }
    void joinCache(const MemoryComponent &mc) {
      cache->join(*mc.cache);
      if (justMissedCache != mc.justMissedCache) {
        justMissedCache = boost::none;
      }
      assert(justUpdatedCache == mc.justUpdatedCache);
      assert(justDirtifiedLine == mc.justDirtifiedLine);
      assert(justWroteBackLine == mc.justWroteBackLine);
      nmisses = std::max(nmisses, mc.nmisses);
      numStoreBusAccess = std::max(numStoreBusAccess, mc.numStoreBusAccess);
    }
  };

  /**
   * Instruction part of the topology
   */
  MemoryComponent instructionComponent;
  /**
   * Data part of the topology
   */
  MemoryComponent dataComponent;

  // O: 2
  MemoryComponent L2Component;

  /**
   * The persistence scope just entered
   */
  boost::optional<std::set<PersistenceScope>> justEntered;
};

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::OSeparateCachesMemoryTopology()
    : memory(BgMem()), currentId(1),
      instructionComponent(makeInstrCache(false)),
      dataComponent(makeDataCache(false)), L2Component(makeL2Cache(false)),
      justEntered(boost::none) {}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::
    OSeparateCachesMemoryTopology(const OSeparateCachesMemoryTopology &scmt)
    : memory(scmt.memory), currentId(scmt.currentId),
      instructionComponent(scmt.instructionComponent),
      dataComponent(scmt.dataComponent), L2Component(scmt.L2Component),
      justEntered(scmt.justEntered) {}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem> &
OSeparateCachesMemoryTopology<
    makeInstrCache, makeDataCache, makeL2Cache,
    BgMem>::operator=(const OSeparateCachesMemoryTopology &scmt) {
  instructionComponent = scmt.instructionComponent;
  dataComponent = scmt.dataComponent;
  L2Component = scmt.L2Component;
  memory = scmt.memory;
  currentId = scmt.currentId;
  justEntered = scmt.justEntered;
  return *this;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
boost::optional<unsigned>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::accessInstr(unsigned addr,
                                                    unsigned numWords) {
  // override (delete) previous access
  if (instructionComponent.waitingQueue.size() > 0) {
    instructionComponent.waitingQueue.pop_front();
  }
  unsigned resId = currentId;
  instructionComponent.waitingQueue.push_back(
      Access(resId, AbstractAddress(addr), AccessType::LOAD, numWords));
  incrementCurrentId();
  return boost::optional<unsigned>(resId);
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
boost::optional<unsigned>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::accessInstrWithCtx(unsigned addr,
                                                           unsigned numWords,
                                                           Context ctx) {
  // override (delete) previous access
  if (instructionComponent.waitingQueue.size() > 0) {
    instructionComponent.waitingQueue.pop_front();
  }
  unsigned resId = currentId;
  instructionComponent.waitingQueue.push_back(
      Access(resId, AbstractAddress(addr), AccessType::LOAD, numWords, ctx));
  incrementCurrentId();
  return boost::optional<unsigned>(resId);
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
boost::optional<unsigned>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::accessL2(unsigned addr,
                                                 AccessType load_store,
                                                 unsigned numWords) {
  // override (delete) previous access
  // if (L2Component.waitingQueue.size() > 0) {
  //   L2Component.waitingQueue.pop_front();
  // }
  unsigned resId = currentId;
  // L2Component.waitingQueue.push_back(
  //     Access(resId, AbstractAddress(addr), load_store, numWords));
  return boost::optional<unsigned>(resId);
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
boost::optional<unsigned>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::accessData(AbstractAddress addr,
                                                   AccessType load_store,
                                                   unsigned numWords) {
  unsigned resId;
  if (dataComponent.waitingQueue.size() == 0) {
    resId = currentId;
    dataComponent.waitingQueue.push_back(
        Access(resId, addr, load_store, numWords));
    incrementCurrentId();
    return boost::optional<unsigned>(resId);
  } else {
    return boost::none;
  }
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
bool OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                     BgMem>::finishedInstrAccess(unsigned id) {
  bool res = instructionComponent.finishedId == id;
  if (res) {
    instructionComponent.finishedId = 0;
  }
  return res;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
bool OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                     BgMem>::finishedDataAccess(unsigned id) {
  bool res = dataComponent.finishedId == id;
  if (res) {
    dataComponent.finishedId = 0;
  }
  return res;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
void OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                     BgMem>::enterScope(const PersistenceScope
                                                            &scope) {
  instructionComponent.cache->enterScope(scope);
  dataComponent.cache->enterScope(scope);
  L2Component.cache->enterScope(scope);
  if (!justEntered) {
    justEntered = std::set<PersistenceScope>();
  }
  justEntered.get().insert(scope);
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
void OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                     BgMem>::leaveScope(const PersistenceScope
                                                            &scope) {
  instructionComponent.cache->leaveScope(scope);
  dataComponent.cache->leaveScope(scope);
  L2Component.cache->leaveScope(scope);
}
// L2component
template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
bool OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                     BgMem>::hasUnfinishedAccesses() const {
  bool result = instructionComponent.ongoingAccess ||
                !instructionComponent.waitingQueue.empty() ||
                dataComponent.ongoingAccess ||
                !dataComponent.waitingQueue.empty() ||
                memory.hasUnfinishedAccesses();
  return result;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
void OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                     BgMem>::incrementCurrentId() {
  currentId++;
  if (currentId == 0) { // prevent zero from begin used!
    currentId++;
  }
}

// cycle is called from the pipeline
template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
std::list<OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache,
                                          makeL2Cache, BgMem>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::cycle(bool potentialDataMissesPending)
    const {
  // Overall result list
  std::list<OSeparateCachesMemoryTopology> resultList;

  OSeparateCachesMemoryTopology succ(*this);
  // We delete our just-missed events in all our successors
  succ.instructionComponent.justMissedCache = boost::none;
  succ.dataComponent.justMissedCache = boost::none;
  succ.L2Component.justMissedCache = boost::none;
  // We delete our just-accessed events in all our successors
  succ.instructionComponent.justUpdatedCache = boost::none;
  succ.dataComponent.justUpdatedCache = boost::none;
  succ.dataComponent.justDirtifiedLine = boost::none;
  succ.dataComponent.justWroteBackLine = false;
  succ.L2Component.justUpdatedCache = boost::none;
  succ.L2Component.justDirtifiedLine = boost::none;
  succ.L2Component.justWroteBackLine = false;
  // We delete scope entering
  succ.justEntered = boost::none;

  // If starting instruction accesses is feasible, do it
  for (auto &startinstrTopology : succ.mystartInstructionAccess()) {
    for (auto &startdataTopology : startinstrTopology.mystartDataAccess()) {
      // Cycle background memory
      for (auto &memory :
           startdataTopology.memory.cycle(potentialDataMissesPending)) {
        OSeparateCachesMemoryTopology afterbgmemcycle(startdataTopology);
        afterbgmemcycle.memory = memory;

        // If we should have stalled the pipeline in this cycle, we should also
        // stall the topology
        if (afterbgmemcycle.shouldPipelineStall()) {
          resultList.push_back(afterbgmemcycle);
        } else {
          // Cycle Memory Topology
          for (auto &memAfterInstr : afterbgmemcycle.checkInstructionPart()) {
            for (auto &memAfterData : memAfterInstr.checkDataPart()) {
              resultList.push_back(memAfterData);
            }
          }
        }
      }
    }
  }

  return resultList;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
std::list<OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache,
                                          makeL2Cache, BgMem>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::startInstructionAccess() {
  std::list<OSeparateCachesMemoryTopology> resultList;

  // We do not have any ongoing instruction access, so start one if there is one
  // waiting
  if (!instructionComponent.ongoingAccess) {
    // nothing accessed yet
    if (instructionComponent.waitingQueue.size() > 0) {
      instructionComponent.ongoingAccess =
          OngoingAccess(instructionComponent.waitingQueue.front());
      instructionComponent.waitingQueue.pop_front();

      // check cache for hit/miss/unknown
      Classification res = instructionComponent.cache->classify(
          instructionComponent.ongoingAccess.get().access.addr);
      if (res == CL_UNKNOWN) {
        if (FollowLocalWorstType.isSet(LocalWorstCaseType::ICMISS)) {
          processInstrCacheAccess(CL_UNKNOWN);
        } else {
          // split here - this is hit state, need another state for miss
          OSeparateCachesMemoryTopology scmtMiss(*this);
          scmtMiss.processInstrCacheAccess(CL_MISS);
          resultList.push_back(scmtMiss);
          processInstrCacheAccess(CL_HIT);
        }
      } else {
        // If definite cache hit but in preemption mode, we consider also the
        // instruction cache miss case (if not always hit)
        if (res == CL_HIT && PreemptiveExecution &&
            InstrCacheReplPolType != CacheReplPolicyType::ALHIT) {
          OSeparateCachesMemoryTopology scmtMiss(*this);
          scmtMiss.processInstrCacheAccess(CL_MISS);
          resultList.push_back(scmtMiss);
        }
        // process with the given result
        processInstrCacheAccess(res);
      }
    }
  }
  resultList.push_back(*this);
  return resultList;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
std::list<OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache,
                                          makeL2Cache, BgMem>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::mystartInstructionAccess() {
  std::list<OSeparateCachesMemoryTopology> resultList;

  // We do not have any ongoing instruction access, so start one if there is one
  // waiting
  if (!instructionComponent.ongoingAccess) {
    // nothing accessed yet
    if (instructionComponent.waitingQueue.size() > 0) {
      instructionComponent.ongoingAccess =
          OngoingAccess(instructionComponent.waitingQueue.front());
      instructionComponent.waitingQueue.pop_front();

      if (MulCType == MultiCoreType::OUR || MulCType == MultiCoreType::ZhangW) {
        instructionComponent.ongoingAccess.get().age1 =
            instructionComponent.cache->getAge(
                instructionComponent.ongoingAccess.get().access.addr);
        instructionComponent.ongoingAccess.get().age2 =
            L2Component.cache->getAge(
                instructionComponent.ongoingAccess.get().access.addr);
      }

      // check cache for hit/l2hit/l2miss/l2unknown
      Classification L1 = instructionComponent.cache->classify(
          instructionComponent.ongoingAccess.get().access.addr);
      Classification L2 = L2Component.cache->classify(
          instructionComponent.ongoingAccess.get().access.addr);
      if (L1 == CL_HIT || (L1 == CL_UNKNOWN && ::isBCET)) {
        this->processInstrCacheAccess(CL_HIT);
      } else {
        if (L2 == CL_HIT || (L2 == CL_UNKNOWN && ::isBCET)) {
          if (L1 == CL_UNKNOWN && TimingAnomalyAnalysis) {
            // ,split
            OSeparateCachesMemoryTopology l1hit(*this);
            l1hit.processInstrCacheAccess(CL_HIT);
            resultList.push_back(l1hit);
          }
          this->processInstrCacheAccess(CL2_HIT);
        } else {
          if (L2 == CL_UNKNOWN && TimingAnomalyAnalysis) {
            if (L1 == CL_UNKNOWN) {
              // 
              OSeparateCachesMemoryTopology l2hit(*this);
              l2hit.processInstrCacheAccess(CL_HIT);
              resultList.push_back(l2hit);
            }
            OSeparateCachesMemoryTopology l3hit(*this);
            l3hit.processInstrCacheAccess(CL2_HIT);
            resultList.push_back(l3hit);
          }
          this->processInstrCacheAccess(CL2_MISS);
        }
      }
    }
  }
  resultList.push_back(*this);
  return resultList;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
std::list<OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache,
                                          makeL2Cache, BgMem>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::startDataAccess() {
  std::list<OSeparateCachesMemoryTopology> resultList;

  // We do not have any ongoing data access, so start one if there is one
  // waiting
  if (!dataComponent.ongoingAccess) {
    if (dataComponent.waitingQueue.size() > 0) {
      dataComponent.ongoingAccess =
          OngoingAccess(dataComponent.waitingQueue.front());
      dataComponent.waitingQueue.pop_front();

      // check cache for hit/miss/unknown
      Classification res = dataComponent.cache->classify(
          dataComponent.ongoingAccess.get().access.addr);
      if (res == CL_UNKNOWN) {
        if (FollowLocalWorstType.isSet(LocalWorstCaseType::DCMISS)) {
          processDataCacheAccess(CL_UNKNOWN);
        } else {
          // split here - this is hit state, need another state for miss
          OSeparateCachesMemoryTopology scmtMiss(*this);
          scmtMiss.processDataCacheAccess(CL_MISS);
          resultList.push_back(scmtMiss);
          processDataCacheAccess(CL_HIT);
        }
      } else {
        // If definite cache hit but in preemption mode, we consider also the
        // data cache miss case (if not always hit)
        if (res == CL_HIT && PreemptiveExecution &&
            DataCacheReplPolType != CacheReplPolicyType::ALHIT) {
          OSeparateCachesMemoryTopology scmtMiss(*this);
          scmtMiss.processDataCacheAccess(CL_MISS);
          resultList.push_back(scmtMiss);
        }
        processDataCacheAccess(res);
      }
    }
  }
  resultList.push_back(*this);
  return resultList;
}
template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
std::list<OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache,
                                          makeL2Cache, BgMem>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::mystartDataAccess() {
  std::list<OSeparateCachesMemoryTopology> resultList;

  // We do not have any ongoing data access, so start one if there is one
  // waiting
  if (!dataComponent.ongoingAccess) {
    if (dataComponent.waitingQueue.size() > 0) {
      dataComponent.ongoingAccess =
          OngoingAccess(dataComponent.waitingQueue.front());
      dataComponent.waitingQueue.pop_front();
      if (MulCType == MultiCoreType::OUR || MulCType == MultiCoreType::ZhangW) {
        // get the age
        dataComponent.ongoingAccess.get().age1 = dataComponent.cache->getAge(
            dataComponent.ongoingAccess.get().access.addr);
        dataComponent.ongoingAccess.get().age2 = L2Component.cache->getAge(
            dataComponent.ongoingAccess.get().access.addr);
      }

      // check cache for hit/miss/unknown
      Classification L1 = dataComponent.cache->classify(
          dataComponent.ongoingAccess.get().access.addr);
      Classification L2 = L2Component.cache->classify(
          dataComponent.ongoingAccess.get().access.addr);
      if (L1 == CL_HIT || (L1 == CL_UNKNOWN && ::isBCET)) {
        this->processDataCacheAccess(CL_HIT);
      } else {
        if (L2 == CL_HIT || (L2 == CL_UNKNOWN && ::isBCET)) {
          // 
          if (L1 == CL_UNKNOWN && TimingAnomalyAnalysis) {
            OSeparateCachesMemoryTopology l1hit(*this);
            l1hit.processDataCacheAccess(CL_HIT);
            resultList.push_back(l1hit);
          }
          this->processDataCacheAccess(CL2_HIT);
        } else {
          // 
          if (L2 == CL_UNKNOWN && TimingAnomalyAnalysis) {
            if (L1 == CL_UNKNOWN) {
              OSeparateCachesMemoryTopology l2hit(*this);
              l2hit.processDataCacheAccess(CL_HIT);
              resultList.push_back(l2hit);
            }
            OSeparateCachesMemoryTopology l3hit(*this);
            l3hit.processDataCacheAccess(CL2_HIT);
            resultList.push_back(l3hit);
          }
          this->processDataCacheAccess(CL2_MISS);
        }
      }
    }
  }
  resultList.push_back(*this);
  return resultList;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
std::list<OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache,
                                          makeL2Cache, BgMem>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::checkInstructionPart() {
  std::list<OSeparateCachesMemoryTopology> resultList;

  if (instructionComponent.ongoingAccess) // We are accessing something
  {
    auto &ongoingAcc = instructionComponent.ongoingAccess.get();
    // check whether something was accessed in the background memory
    if (ongoingAcc.bgMemAccessId != 0) {
      if (memory.finishedInstrAccess(ongoingAcc.bgMemAccessId)) {
        // Not waiting for memory any more
        ongoingAcc.bgl2AccessId = ongoingAcc.bgMemAccessId;

        ongoingAcc.bgMemAccessId = 0;
        // Blocked for cache update
        ongoingAcc.l2timeBlocked = L2Component.cache->getHitLatency();
      }
    } else if (ongoingAcc.bgl2AccessId != 0) {
      if (ongoingAcc.l2timeBlocked > 0) {
        ongoingAcc.l2timeBlocked--;
      }

      if (ongoingAcc.l2timeBlocked == 0) {
        ongoingAcc.bgl2AccessId = 0;
        // Blocked for cache update
        ongoingAcc.l1timeBlocked = instructionComponent.cache->getHitLatency();
        L2Component.cache->update(ongoingAcc.access.addr,
                                  ongoingAcc.access.load_store, false,
                                  ongoingAcc.cl);
      }
    } else {
      if (ongoingAcc.l1timeBlocked > 0) {
        ongoingAcc.l1timeBlocked--;
      }

      if (ongoingAcc.l1timeBlocked == 0) {
        // if (0 || MulCType == MultiCoreType::OUR ||
        //     MulCType == MultiCoreType::ZhangW ||
        //     MulCType == MultiCoreType::LiangY) {
        //   Classification L1 = instructionComponent.cache->classify(
        //       instructionComponent.ongoingAccess.get().access.addr);
        //   Classification L2 = L2Component.cache->classify(
        //       instructionComponent.ongoingAccess.get().access.addr);
        //   if (L1 == CL_UNKNOWN && L2 == CL_UNKNOWN) {
        //     L2Component.cache->update(ongoingAcc.access.addr,
        //                               ongoingAcc.access.load_store, false,
        //                               ongoingAcc.cl);
        //   }
        // }

        instructionComponent.finishedId = ongoingAcc.access.id;
        if (needAccessedInstructionAddresses()) {
          instructionComponent.justUpdatedCache = ongoingAcc.access.addr;
        }
        instructionComponent.cache->update(ongoingAcc.access.addr,
                                           ongoingAcc.access.load_store, false,
                                           ongoingAcc.cl);

        instructionComponent.ongoingAccess = boost::none;
      }
    }
  }
  resultList.push_back(*this);
  return resultList;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
std::list<OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache,
                                          makeL2Cache, BgMem>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::checkDataPart() {
  std::list<OSeparateCachesMemoryTopology> resultList;

  if (dataComponent.ongoingAccess) // We are accessing something
  {
    auto &ongoingAcc = dataComponent.ongoingAccess.get();
    // Do we need to stall as background memory is busy, try again now
    if (ongoingAcc.bgmemStall) {
      assert(UnblockStores && "Cannot be stalled on background memory when "
                              "blocking Stores are used");
      assert((ongoingAcc.phase == AccessPhase::WaitForLOAD ||
              ongoingAcc.phase == AccessPhase::WaitForSTORE) &&
             "Stall => waiting for load or store");
      auto accTy = (ongoingAcc.phase == AccessPhase::WaitForLOAD)
                       ? AccessType::LOAD
                       : AccessType::STORE;
      // Loads are done in cacheline chunks
      auto nwords = (accTy == AccessType::LOAD) ? Dlinesize / 4
                                                : ongoingAcc.access.numWords;
      auto res = memory.accessData(ongoingAcc.access.addr, accTy, nwords);
      if (res) {
        ongoingAcc.bgMemAccessId = res.get();
        ongoingAcc.bgmemStall = false;
      }
    } else {
      switch (ongoingAcc.phase) {
      case AccessPhase::WaitForLOAD: {
        assert(ongoingAcc.bgMemAccessId != 0 &&
               "Waiting for load but not accessing anything");
        if (memory.finishedDataAccess(
                ongoingAcc
                    .bgMemAccessId)) { // We waited for load and are finished
          ongoingAcc.bgl2AccessId = ongoingAcc.bgMemAccessId;
          ongoingAcc.bgMemAccessId = 0;
          ongoingAcc.l2timeBlocked = L2Component.cache->getHitLatency();
          ongoingAcc.phase = AccessPhase::WaitForCACHE;
        }
        break;
      }
      case AccessPhase::WaitForCACHE: {
        // something was accessed in cache (either a hit, or an update after a
        // miss) and we wait for it to process
        assert(ongoingAcc.bgMemAccessId == 0 &&
               "Waiting for cache but accessing in main memory");

        /* decrement remaining blocking time.
         * The if guards against underflow */
        Access acc = ongoingAcc.access;
        AbstractAddress addr = ongoingAcc.access.addr;
        AccessType accType = ongoingAcc.access.load_store;
        bool isWBCache = dataComponent.cache->getWritePolicy().WriteBack;
        if (ongoingAcc.bgl2AccessId != 0) {
          if (ongoingAcc.bgl2AccessId == -1) {
            ongoingAcc.l2timeBlocked = L2Component.cache->getHitLatency();
          }
          if (ongoingAcc.l2timeBlocked > 0) {
            ongoingAcc.l2timeBlocked--;
          }
          if (ongoingAcc.l2timeBlocked == 0) {

            bool mightMiss =
                ongoingAcc.cl == CL2_MISS || ongoingAcc.cl == CL2_UNKNOWN;
            boost::optional<AbstractAddress> WBVictimItv(
                AbstractAddress::getUnknownAddress());
            bool wantReport =
                isWBCache && (mightMiss || accType == AccessType::STORE);

            Classification L2 = L2Component.cache->classify(
                dataComponent.ongoingAccess.get().access.addr);
            UpdateReport *report;
            if (L2 == CL_MISS && ongoingAcc.cl == CL2_HIT) {
              report = L2Component.cache->update(ongoingAcc.access.addr,
                                                 ongoingAcc.access.load_store,
                                                 wantReport, CL2_MISS);

            } else {
              report = L2Component.cache->update(ongoingAcc.access.addr,
                                                 ongoingAcc.access.load_store,
                                                 wantReport, ongoingAcc.cl);
            }

            if (isWBCache) {
              /* If the report is a WriteBackReport improve our
               * writeback/dirtifying store information */
              auto wbreport = dynamic_cast<WritebackReport *>(report);
              if (wbreport && StaticallyRefuteWritebacks) {
                WBVictimItv = wbreport->potentialWritebacks();
              }
              if (accType == AccessType::STORE) {
                dataComponent.justDirtifiedLine = acc.addr;
                if (WBBound == WBBoundType::DIRTIFYING_STORE && wbreport &&
                    !wbreport->dirtifyingStore) {
                  dataComponent.justDirtifiedLine = boost::none;
                }
              }
              if (mightMiss) {
                AnalysisResults::getInstance().incrementResult("staticMisses");
                if (!WBVictimItv) {
                  AnalysisResults::getInstance().incrementResult(
                      "staticallyRefutedWritebacks");
                }
              }
            }

            if (accType == AccessType::STORE && !isWBCache) {
              // Write-through policy and we have a store
              // We want to launch a store to the address we were accessing
              // originally
              auto res =
                  memory.accessData(acc.addr, AccessType::STORE, acc.numWords);
              if (res) {
                ongoingAcc.bgMemAccessId = res.get();
              } else {
                ongoingAcc.bgmemStall = true;
              }
              ongoingAcc.phase = AccessPhase::WaitForSTORE;
              ++dataComponent.numStoreBusAccess;
            } else if (isWBCache && mightMiss && WBVictimItv) {
              OSeparateCachesMemoryTopology scmtNoWriteback(*this);
              scmtNoWriteback.dataComponent.finishedId =
                  scmtNoWriteback.dataComponent.ongoingAccess.get().access.id;
              scmtNoWriteback.dataComponent.ongoingAccess = boost::none;
              resultList.push_back(scmtNoWriteback);

              // We should do a write-back
              auto res = memory.accessData(WBVictimItv.get(), AccessType::STORE,
                                           Dlinesize / 4);
              assert(res && "Background Memory Topology rejected data access!");
              ongoingAcc.bgMemAccessId = res.get();
              ongoingAcc.phase = AccessPhase::WaitForSTORE;

              ++dataComponent.numStoreBusAccess;
              dataComponent.justWroteBackLine = true;
            } else {
              if (ongoingAcc.bgl2AccessId != -1) {
                ongoingAcc.l1timeBlocked =
                    instructionComponent.cache->getHitLatency();
              } else {
                dataComponent.finishedId =
                    dataComponent.ongoingAccess.get().access.id;
                dataComponent.ongoingAccess = boost::none;
              }
              ongoingAcc.bgl2AccessId = 0;
            }
            if (isWBCache && dataComponent.justDirtifiedLine) {
              /* split the states to allow restricting the
               * dfs by persistence constraints */
              OSeparateCachesMemoryTopology noDFSsplit(*this);
              noDFSsplit.dataComponent.justDirtifiedLine = boost::none;
              resultList.push_back(noDFSsplit);
            }
            delete report;
          }

        } else {
          if (ongoingAcc.l1timeBlocked > 0) {
            ongoingAcc.l1timeBlocked--;
          }

          /* check if we are ready to progress */
          if (ongoingAcc.l1timeBlocked > 0) {
            break;
          }

          bool mightMiss =
              ongoingAcc.cl == CL2_MISS || ongoingAcc.cl == CL2_UNKNOWN;
          if (needAccessedDataAddresses()) {
            dataComponent.justUpdatedCache = addr;
          }

          /* In the absence of any further information assume any access
           * causes a writeback and any store is dirtifying */
          boost::optional<AbstractAddress> WBVictimItv(
              AbstractAddress::getUnknownAddress());

          /* we are interested in the report for dirtifying stores and
           * writebacks
           */

          // if (0|| MulCType == MultiCoreType::OUR ||
          //     MulCType == MultiCoreType::ZhangW ||
          //     MulCType == MultiCoreType::LiangY) {
          //   Classification L1 = dataComponent.cache->classify(
          //       dataComponent.ongoingAccess.get().access.addr);
          //   Classification L2 = L2Component.cache->classify(
          //       dataComponent.ongoingAccess.get().access.addr);
          //   if (L1 == CL_UNKNOWN && L2 == CL_UNKNOWN) {
          //     L2Component.cache->update(addr, accType, false, ongoingAcc.cl);
          //   }
          // }

          bool wantReport =
              isWBCache && (mightMiss || accType == AccessType::STORE);
          UpdateReport *report = dataComponent.cache->update(
              addr, accType, wantReport, ongoingAcc.cl);

          if (isWBCache) {
            /* If the report is a WriteBackReport improve our
             * writeback/dirtifying store information */
            auto wbreport = dynamic_cast<WritebackReport *>(report);
            if (wbreport && StaticallyRefuteWritebacks) {
              WBVictimItv = wbreport->potentialWritebacks();
            }
            if (accType == AccessType::STORE) {
              dataComponent.justDirtifiedLine = acc.addr;
              if (WBBound == WBBoundType::DIRTIFYING_STORE && wbreport &&
                  !wbreport->dirtifyingStore) {
                dataComponent.justDirtifiedLine = boost::none;
              }
            }

            if (mightMiss) {
              AnalysisResults::getInstance().incrementResult("staticMisses");
              if (!WBVictimItv) {
                AnalysisResults::getInstance().incrementResult(
                    "staticallyRefutedWritebacks");
              }
            }
          }

          if (accType == AccessType::STORE && !isWBCache) {
            // Write-through policy and we have a store
            // We want to launch a store to the address we were accessing
            // originally
            auto res =
                memory.accessData(acc.addr, AccessType::STORE, acc.numWords);
            if (res) {
              ongoingAcc.bgMemAccessId = res.get();
            } else {
              ongoingAcc.bgmemStall = true;
            }
            ongoingAcc.phase = AccessPhase::WaitForSTORE;
            ++dataComponent.numStoreBusAccess;
          }
          /* We split if we cannot prove there is no writeback
             TODO should we try to prove that there *is* a
             writeback to save a split? It might also help when we add
             storebuffers, since it might draw writeback budget away from more
             critical points */
          else if (isWBCache && mightMiss && WBVictimItv) {
            // TODOL2
            OSeparateCachesMemoryTopology scmtNoWriteback(*this);
            scmtNoWriteback.dataComponent.finishedId =
                scmtNoWriteback.dataComponent.ongoingAccess.get().access.id;
            scmtNoWriteback.dataComponent.ongoingAccess = boost::none;
            resultList.push_back(scmtNoWriteback);

            // // We should do a write-back to L2
            // ongoingAcc.bgl2AccessId = -1;
            dataComponent.justDirtifiedLine = boost::none;

            auto res = memory.accessData(WBVictimItv.get(), AccessType::STORE,
                                         Dlinesize / 4);
            assert(res && "Background Memory Topology rejected data access!");
            ongoingAcc.bgMemAccessId = res.get();
            ongoingAcc.phase = AccessPhase::WaitForSTORE;

            ++dataComponent.numStoreBusAccess;
            dataComponent.justWroteBackLine = true;
          } else { // We are finished, nothing left to do
            dataComponent.finishedId =
                dataComponent.ongoingAccess.get().access.id;
            dataComponent.ongoingAccess = boost::none;
          }

          if (isWBCache && dataComponent.justDirtifiedLine) {
            /* split the states to allow restricting the
             * dfs by persistence constraints */
            OSeparateCachesMemoryTopology noDFSsplit(*this);
            noDFSsplit.dataComponent.justDirtifiedLine = boost::none;
            resultList.push_back(noDFSsplit);
          }
          delete report;
        }
        break;
      }
      case AccessPhase::WaitForSTORE: {
        assert(ongoingAcc.bgMemAccessId != 0 &&
               "Waiting for store but not accessing anything");
        if (UnblockStores) { // If option for unblocking stores is set, finish
                             // access directly
          dataComponent.finishedId = ongoingAcc.access.id;
          dataComponent.ongoingAccess = boost::none;
        } else {
          // We wait for the store to finish before signaling finish to the
          // outside
          if (memory.finishedDataAccess(ongoingAcc.bgMemAccessId)) {
            dataComponent.finishedId = ongoingAcc.access.id;
            dataComponent.ongoingAccess = boost::none;
          }
        }
        break;
      }
      default:
        assert(0 && "We got an unexpected access phase during memory cycling");
      }
    }
  }

  resultList.push_back(*this);
  return resultList;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
void OSeparateCachesMemoryTopology<
    makeInstrCache, makeDataCache, makeL2Cache,
    BgMem>::processInstrCacheAccess(Classification cl) {
  // Remember Classification
  auto &ongoingAcc = instructionComponent.ongoingAccess.get();
  ongoingAcc.cl = cl;
  // // Assert part
  // Classification actualcl =
  //     instructionComponent.cache->classify(ongoingAcc.access.addr);
  // Classification copycl(actualcl);
  // copycl.join(cl);
  // assert((actualcl == copycl || PreemptiveExecution) &&
  //        "Incompatible classifications detected");

  if (cl == CL_HIT) {
    ongoingAcc.l1timeBlocked = instructionComponent.cache->getHitLatency();
  } else if (cl == CL2_HIT) {
    ++instructionComponent.nmisses;
    ongoingAcc.l2timeBlocked = L2Component.cache->getHitLatency();
    Access acc = ongoingAcc.access;
    if ((InstrCachePersType != PersistenceType::NONE || PreemptiveExecution)) {
      // We just missed the instr cache (only needed for persistence analysis
      // and integrated preemptive execution mode)
      assert(acc.addr.isPrecise());
      instructionComponent.justMissedCache =
          AbstractAddress(instructionComponent.cache->alignToCacheline(
              acc.addr.getAsInterval().lower()));
    }
    ongoingAcc.bgl2AccessId = currentId;
  } else {
    ++instructionComponent.nmisses;
    ++L2Component.nmisses;
    Access acc = ongoingAcc.access;
    // O:we add l2Cache persistence analysis
    if (cl == CL2_MISS &&
        (L2CachePersType != PersistenceType::NONE || PreemptiveExecution)) {
      assert(acc.addr.isPrecise());
      L2Component.justMissedCache =
          AbstractAddress(L2Component.cache->alignToCacheline(
              acc.addr.getAsInterval().lower()));
    }
    if (cl == CL2_MISS &&
        (InstrCachePersType != PersistenceType::NONE || PreemptiveExecution)) {
      assert(acc.addr.isPrecise());
      instructionComponent.justMissedCache =
          AbstractAddress(instructionComponent.cache->alignToCacheline(
              acc.addr.getAsInterval().lower()));
    }
    boost::optional<unsigned> res =
        memory.accessInstr(acc.addr.getAsInterval().lower(), Ilinesize / 4);
    if (res) {
      ongoingAcc.bgMemAccessId = *res;
      // memory.setcurrentid(currentId);
    } else {
      assert(false &&
             "Background memory topology rejected instruction access.");
    }
  }
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
void OSeparateCachesMemoryTopology<
    makeInstrCache, makeDataCache, makeL2Cache,
    BgMem>::processDataCacheAccess(Classification cl) {
  auto &ongoingAcc = dataComponent.ongoingAccess.get();
  ongoingAcc.cl = cl;

  if (cl == CL_HIT) { // We hit the cache (in case we load and in case we store)
    ongoingAcc.l1timeBlocked = dataComponent.cache->getHitLatency();
    ongoingAcc.phase = AccessPhase::WaitForCACHE;
  } else if (cl == CL2_HIT) {
    ongoingAcc.l2timeBlocked = L2Component.cache->getHitLatency();
    Access acc = dataComponent.ongoingAccess.get().access;
    ++dataComponent.nmisses;
    ongoingAcc.phase = AccessPhase::WaitForCACHE;
    if (DataCachePersType != PersistenceType::NONE || PreemptiveExecution) {
      // We just missed the data cache (only needed for persistence analysis
      // and integrated preemptive execution mode)
      if (dataComponent.cache->alignToCacheline(
              acc.addr.getAsInterval().lower()) ==
          dataComponent.cache->alignToCacheline(
              acc.addr.getAsInterval().upper())) {
        unsigned addr = dataComponent.cache->alignToCacheline(
            acc.addr.getAsInterval().lower());
        dataComponent.justMissedCache =
            AbstractAddress(dataComponent.cache->alignToCacheline(
                acc.addr.getAsInterval().lower()));
      } else if (acc.addr.isArray()) {
        /* TODO we should have a way to tell addr about alignment, such that
         * getAsInterval() returns correctly aligned bounds. For now just
         * don't call getAsInterval on the justMissedCache member */
        dataComponent.justMissedCache = acc.addr;
      }
    }
    ongoingAcc.bgl2AccessId = currentId;

  } else {
    Access acc = dataComponent.ongoingAccess.get().access;
    // What shall we do?
    if (acc.load_store == AccessType::STORE &&
        !dataComponent.cache->getWritePolicy().WriteAllocate) {
      // We store (missed) write-non-allocate
      assert(!dataComponent.cache->getWritePolicy().WriteBack &&
             "Only write-through can be non-write-allocate");
      auto res = memory.accessData(acc.addr, AccessType::STORE, acc.numWords);
      if (res) {
        ongoingAcc.bgMemAccessId = res.get();
      } else {
        assert(false && "l2 topology rejected instruction access.");
      }
      ongoingAcc.phase = AccessPhase::WaitForSTORE;
      ++dataComponent.numStoreBusAccess;
    } else {
      ++dataComponent.nmisses;
      ++L2Component.nmisses;
      if (cl == CL2_MISS &&
          (L2CachePersType != PersistenceType::NONE || PreemptiveExecution)) {
        if (L2Component.cache->alignToCacheline(
                acc.addr.getAsInterval().lower()) ==
            L2Component.cache->alignToCacheline(
                acc.addr.getAsInterval().upper())) {
          L2Component.justMissedCache =
              AbstractAddress(L2Component.cache->alignToCacheline(
                  acc.addr.getAsInterval().lower()));
        } else if (acc.addr.isArray()) {
          L2Component.justMissedCache = acc.addr;
        }
      }
      if (cl == CL2_MISS &&
          (DataCachePersType != PersistenceType::NONE || PreemptiveExecution)) {
        if (dataComponent.cache->alignToCacheline(
                acc.addr.getAsInterval().lower()) ==
            dataComponent.cache->alignToCacheline(
                acc.addr.getAsInterval().upper())) {
          dataComponent.justMissedCache =
              AbstractAddress(dataComponent.cache->alignToCacheline(
                  acc.addr.getAsInterval().lower()));
        } else if (acc.addr.isArray()) {
          dataComponent.justMissedCache = acc.addr;
        }
      }

      auto res =
          memory.accessData(acc.addr, AccessType::LOAD,
                            Dlinesize / 4); // first load the necessary data
      if (res) {
        ongoingAcc.bgMemAccessId = res.get();
        // memory.setcurrentid(currentId);
      } else {
        ongoingAcc.bgmemStall = true;
      }
      ongoingAcc.phase = AccessPhase::WaitForLOAD; // We wait for the load
    }
  }
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
bool OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                     BgMem>::shouldPipelineStall() const {
  return memory.shouldPipelineStall();
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
bool OSeparateCachesMemoryTopology<
    makeInstrCache, makeDataCache, makeL2Cache,
    BgMem>::operator==(const OSeparateCachesMemoryTopology &scmt) const {
  bool result =
      memory == scmt.memory &&
      instructionComponent.equals(scmt.instructionComponent, currentId,
                                  scmt.currentId) &&
      dataComponent.equals(scmt.dataComponent, currentId, scmt.currentId) &&
      L2Component.equals(scmt.L2Component, currentId, scmt.currentId) &&
      justEntered == scmt.justEntered;
  return result;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
size_t OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache,
                                       makeL2Cache, BgMem>::hashcode() const {
  size_t res = 0;
  /*hash_combine(res, this->instructionComponent.cache);
  hash_combine(res, this->dataComponent.cache);*/ // TODO fix hashing error first
  hash_combine_hashcode(res, this->memory);
  // TODO hash queues here
  if (instructionComponent.ongoingAccess) {
    Super::hash_access(res, instructionComponent.ongoingAccess.get().access,
                       this->currentId);
    hash_combine(res, instructionComponent.ongoingAccess.get().l1timeBlocked);
    hash_combine(res, instructionComponent.ongoingAccess.get().l2timeBlocked);
  }
  if (dataComponent.ongoingAccess) {
    Super::hash_access(res, dataComponent.ongoingAccess.get().access,
                       this->currentId);
    hash_combine(res, dataComponent.ongoingAccess.get().l1timeBlocked);
    hash_combine(res, dataComponent.ongoingAccess.get().l2timeBlocked);
  }
  if (instructionComponent.finishedId != 0)
    hash_combine(res, instructionComponent.finishedId - this->currentId);
  if (dataComponent.finishedId != 0)
    hash_combine(res, dataComponent.finishedId - this->currentId);
  return res;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
bool OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                     BgMem>::isWaitingForJoin() const {
  return memory.isWaitingForJoin();
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
bool OSeparateCachesMemoryTopology<
    makeInstrCache, makeDataCache, makeL2Cache,
    BgMem>::isJoinable(const OSeparateCachesMemoryTopology &scmt) const {
  return memory.isJoinable(scmt.memory) &&
         // caches are assumed to be joinable, so are justMissed
         instructionComponent.equalsWithoutCache(scmt.instructionComponent,
                                                 currentId, scmt.currentId) &&
         dataComponent.equalsWithoutCache(scmt.dataComponent, currentId,
                                          scmt.currentId) &&
         L2Component.equalsWithoutCache(scmt.L2Component, currentId,
                                        scmt.currentId) &&
         justEntered == scmt.justEntered;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
void OSeparateCachesMemoryTopology<
    makeInstrCache, makeDataCache, makeL2Cache,
    BgMem>::join(const OSeparateCachesMemoryTopology &scmt) {
  assert(isJoinable(scmt) && "Cannot join non-joinable states.");

  // Join sub-components
  memory.join(scmt.memory);
  instructionComponent.joinCache(scmt.instructionComponent);
  dataComponent.joinCache(scmt.dataComponent);
  L2Component.joinCache(scmt.L2Component);

  // Everything id related is just kept, as it is equal (relative to the
  // absolute id) anyway. Not that this makes the join non-symmetric w.r.t.
  // absolute id's but to relative id's.
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
bool OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                     BgMem>::isBusyAccessingInstr() const {
  return !instructionComponent.waitingQueue.empty() ||
         instructionComponent.ongoingAccess;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
bool OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                     BgMem>::isBusyAccessingData() const {
  return !dataComponent.waitingQueue.empty() || dataComponent.ongoingAccess;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
std::list<OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache,
                                          makeL2Cache, BgMem>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::fastForward() const {
  std::list<OSeparateCachesMemoryTopology> res;

  bool instrCompIdle = !isBusyAccessingInstr();
  bool dataCompIdle = !isBusyAccessingData();

  bool instrCompAllowsFastForward =
      (instrCompIdle ||
       (instructionComponent.ongoingAccess &&
        instructionComponent.ongoingAccess.get().bgMemAccessId != 0)) &&
      instructionComponent.justMissedCache ==
          boost::none // Do not forget any events
      && L2Component.justMissedCache == boost::none &&
      instructionComponent.justUpdatedCache == boost::none;

  bool dataCompAllowsFastForward =
      (dataCompIdle ||
       (dataComponent.ongoingAccess &&
        (dataComponent.ongoingAccess->phase == AccessPhase::WaitForLOAD ||
         ((!UnblockStores || dataComponent.ongoingAccess->bgmemStall) &&
          dataComponent.ongoingAccess->phase == AccessPhase::WaitForSTORE)))) &&
      dataComponent.justMissedCache == boost::none &&
      L2Component.justMissedCache == boost::none // Do not forget any events
      && dataComponent.justUpdatedCache == boost::none;

  if (instrCompAllowsFastForward && dataCompAllowsFastForward &&
      justEntered == boost::none) {
    // fast-forward the inner memory, then return
    for (auto &forwardedInnerMem : memory.fastForward()) {
      auto copy = res.emplace(res.end(), *this);
      copy->memory = forwardedInnerMem;
    }
    return res;
  }

  // return an unchanged copy
  res.emplace(res.end(), *this);
  return res;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
std::ostream &
operator<<(std::ostream &stream,
           const OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache,
                                                 makeL2Cache, BgMem> &scmt) {
  if (scmt.justEntered) {
    stream << "Just entered Scopes ";
    bool comma = false;
    for (auto &scope : scmt.justEntered.get()) {
      if (comma) {
        stream << ", ";
      }
      stream << scope.getId();
      comma = true;
    }
    stream << "\n";
  }

  if (scmt.instructionComponent.waitingQueue.size() > 0) {
    stream << " Instruction element in cache queue: ";
    MemoryTopologyInterface<OSeparateCachesMemoryTopology<
        makeInstrCache, makeDataCache, makeL2Cache, BgMem>>::
        outputAccess(stream, scmt.instructionComponent.waitingQueue.front(),
                     true, scmt.currentId);
  } else {
    stream << " Instruction cache queue empty.\n";
  }

  stream << "Instruction Cache:\n " << *scmt.instructionComponent.cache << "\n";
  stream << "Misses up to now: " << scmt.instructionComponent.nmisses << "\n";
  if (scmt.instructionComponent.justUpdatedCache) {
    stream << "Just updated: "
           << scmt.instructionComponent.justUpdatedCache.get() << "\n";
  }

  stream << "Currently accessed in Instruction Cache: ";
  if (scmt.instructionComponent.ongoingAccess) {
    MemoryTopologyInterface<OSeparateCachesMemoryTopology<
        makeInstrCache, makeDataCache, makeL2Cache, BgMem>>::
        outputAccess(stream,
                     scmt.instructionComponent.ongoingAccess.get().access, true,
                     scmt.currentId);
    stream << "Access Phase: "
           << (int)scmt.instructionComponent.ongoingAccess.get().phase << "\n";
    stream << "BgMem Stall: "
           << scmt.instructionComponent.ongoingAccess.get().bgmemStall << "\n";
    stream << "Cache TimeBlocked: "
           << scmt.instructionComponent.ongoingAccess.get().l1timeBlocked
           << "\n";
    stream << "Classification: "
           << scmt.instructionComponent.ongoingAccess.get().cl << "\n";
  } else {
    stream << "Nothing accessed.\n";
  }

  stream << "Finished Instruction Access in SCMT, relative ID: ";
  if (scmt.instructionComponent.finishedId != 0) {
    stream << (int)(scmt.instructionComponent.finishedId - scmt.currentId)
           << "\n";
  } else {
    stream << "Nothing finished.\n";
  }

  if (scmt.dataComponent.waitingQueue.size() > 0) {
    stream << "Data element in cache queue: ";
    MemoryTopologyInterface<OSeparateCachesMemoryTopology<
        makeInstrCache, makeDataCache, makeL2Cache,
        BgMem>>::outputAccess(stream, scmt.dataComponent.waitingQueue.front(),
                              false, scmt.currentId);
  } else {
    stream << "Data cache queue empty.\n";
  }

  // 
  stream << "Data Cache:\n " << *scmt.dataComponent.cache;
  stream << "Misses up to now: " << scmt.dataComponent.nmisses << "\n";
  stream << "Stores to bus up to now: " << scmt.dataComponent.numStoreBusAccess
         << "\n";
  if (scmt.dataComponent.justUpdatedCache) {
    stream << "Just updated: " << scmt.dataComponent.justUpdatedCache.get()
           << "\n";
  }
  if (scmt.dataComponent.justDirtifiedLine) {
    stream << "Just dirtified cacheline\n";
  }
  if (scmt.dataComponent.justWroteBackLine) {
    stream << "Just wrote back cacheline\n";
  }

  stream << "Currently accessed in Data Cache: ";
  if (scmt.dataComponent.ongoingAccess) {
    MemoryTopologyInterface<OSeparateCachesMemoryTopology<
        makeInstrCache, makeDataCache, makeL2Cache, BgMem>>::
        outputAccess(stream, scmt.dataComponent.ongoingAccess.get().access,
                     false, scmt.currentId);
    stream << "Access Phase: "
           << (int)scmt.dataComponent.ongoingAccess.get().phase << "\n";
    stream << "BgMem Stall: "
           << scmt.dataComponent.ongoingAccess.get().bgmemStall << "\n";
    stream << "Cache TimeBlocked: "
           << scmt.dataComponent.ongoingAccess.get().l1timeBlocked << "\n";
    stream << "Classification: " << scmt.dataComponent.ongoingAccess.get().cl
           << "\n";
  } else {
    stream << "Nothing accessed.\n";
  }
  stream << "L2 Cache:\n " << *scmt.L2Component.cache << "\n";
  stream << "Misses up to now: " << scmt.L2Component.nmisses << "\n";

  stream << "Finished DataAccess in SCMT, relative ID: ";
  if (scmt.dataComponent.finishedId != 0) {
    stream << (int)(scmt.dataComponent.finishedId - scmt.currentId) << "\n";
  } else {
    stream << "Nothing finished\n";
  }

  stream << "Background Topology: \n" << scmt.memory;
  return stream;
}
template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
boost::optional<std::tuple<AbstractAddress, Classification, int>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::getIaccAdress() const {
  if (instructionComponent.ongoingAccess) {
    AbstractAddress addr = instructionComponent.ongoingAccess.get().access.addr;
    Classification CL = instructionComponent.ongoingAccess.get().cl;
    int age = -1;
    if (CL == CL2_MISS) {
      age = INT_MAX;
    } else if (CL == CL2_HIT) {
      age = instructionComponent.ongoingAccess.get().age2;
    } else if (CL == CL_HIT) {
      age = instructionComponent.ongoingAccess.get().age1;
    }

    return std::make_tuple(addr, CL, age);
  }
  return boost::none;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
boost::optional<std::tuple<AbstractAddress, Classification, int, Context>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::getIaccAddressWithCtx() const {
  if (instructionComponent.ongoingAccess) {
    AbstractAddress addr = instructionComponent.ongoingAccess.get().access.addr;
    Classification CL = instructionComponent.ongoingAccess.get().cl;
    int age = -1;
    if (CL == CL2_MISS) {
      age = INT_MAX;
    } else if (CL == CL2_HIT) {
      age = instructionComponent.ongoingAccess.get().age2;
    } else if (CL == CL_HIT) {
      age = instructionComponent.ongoingAccess.get().age1;
    }
    Context ctx = instructionComponent.ongoingAccess.get().access.ctx;

    return std::make_tuple(addr, CL, age, ctx);
  }
  return boost::none;
}

template <AbstractCache *(*makeInstrCache)(bool),
          AbstractCache *(*makeDataCache)(bool),
          AbstractCache *(*makeL2Cache)(bool), class BgMem>
boost::optional<std::tuple<AbstractAddress, Classification, int>>
OSeparateCachesMemoryTopology<makeInstrCache, makeDataCache, makeL2Cache,
                                BgMem>::getDaccAdress() const {
  if (dataComponent.ongoingAccess) {
    AbstractAddress addr = dataComponent.ongoingAccess.get().access.addr;
    Classification CL = dataComponent.ongoingAccess.get().cl;
    int age = -1;
    if (CL == CL2_MISS) {
      age = INT_MAX;
    } else if (CL == CL2_HIT) {
      age = dataComponent.ongoingAccess.get().age2;
    } else {
      age = dataComponent.ongoingAccess.get().age1;
    }
    return std::make_tuple(addr, CL, age);
  }
  return boost::none;
}

} // namespace TimingAnalysisPass
#endif
