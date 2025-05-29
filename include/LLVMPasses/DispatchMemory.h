////////////////////////////////////////////////////////////////////////////////
//
//   LLVMTA - Timing Analyser performing worst-case execution time analysis
//     using the LLVM backend intermediate representation
//
// Copyright (C) 2013-2022  Saarland University
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

#ifndef DISPATCHMEMORY_H
#define DISPATCHMEMORY_H

#include "Memory/AbstractCache.h"
#include "Memory/AbstractCyclingMemory.h"

#include "Util/IntervalCounter.h"

namespace TimingAnalysisPass {

extern dom::cache::CacheTraits icacheConf;

extern dom::cache::CacheTraits dcacheConf;

extern dom::cache::CacheTraits l2cacheConf;

class CacheFactory {
public:
  static dom::cache::AbstractCache *
  makeOptionsInstrCache(bool assumeEmptyCache);
  static dom::cache::AbstractCache *makeOptionsDataCache(bool assumeEmptyCache);
  static dom::cache::AbstractCache *makeOptionsL2Cache(bool assumeEmptyCache);
  /// Ignore the compositional flags set
  static dom::cache::AbstractCache *
  makeOptionsInstrCacheIgnComp(bool assumeEmptyCache);
  static dom::cache::AbstractCache *
  makeOptionsDataCacheIgnComp(bool assumeEmptyCache);
};

void configureCyclingMemories();

AbstractCyclingMemory *makeOptionsBackgroundMem();

AbstractCyclingMemory *makeOptionsPrivInstrMem();

unsigned getUbAccesses(const AbstractCyclingMemory::LocalMetrics *pBaseMetrics);

unsigned
getLbBlockingCycles(const AbstractCyclingMemory::LocalMetrics *pBaseMetrics);

unsigned
getUbAccessCycles(const AbstractCyclingMemory::LocalMetrics *pBaseMetrics);

typedef IntervalCounter<true /*lower bound needed*/,
                        true /*upper bound needed*/, true /*perform join*/>
    ForwardedCycles;

ForwardedCycles getFastForwardedAccessCycles(
    const AbstractCyclingMemory::LocalMetrics *pBaseMetrics);

ForwardedCycles getFastForwardedBlocking(
    const AbstractCyclingMemory::LocalMetrics *pBaseMetrics);

unsigned
getLbConcAccesses(const AbstractCyclingMemory::LocalMetrics *pBaseMetrics);

} // namespace TimingAnalysisPass

#endif
