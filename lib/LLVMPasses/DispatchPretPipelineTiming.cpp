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

#include "LLVMPasses/DispatchPretPipeline.h"

#include "LLVMPasses/DispatchMemory.h"
#include "LLVMPasses/DispatchMuArchAnalysis.h"
#include "Memory/OSeparateCachesMemoryTopology.h"
#include "Memory/SingleMemoryTopology.h"
#include "MicroarchitecturalAnalysis/PretPipelineState.h"

namespace TimingAnalysisPass {

boost::optional<BoundItv>
dispatchPretTimingAnalysis(AddressInformation &addressInfo) {
  std::tuple<AddressInformation &> addrInfoTuple(addressInfo);

  configureCyclingMemories();

  switch (MemTopType) {
  case MemoryTopologyType::SINGLEMEM: {
    assert(InstrCachePersType == PersistenceType::NONE &&
           DataCachePersType == PersistenceType::NONE &&
           "Cannot use Persistence analyses here");
    typedef SingleMemoryTopology<makeOptionsBackgroundMem> MemTop;
    return dispatchTimingAnalysisJoin<PretPipelineState<MemTop>>(addrInfoTuple);
  }
  case MemoryTopologyType::SEPARATECACHES: {
    typedef SingleMemoryTopology<makeOptionsBackgroundMem> BgMem;
    typedef OSeparateCachesMemoryTopology<
        CacheFactory::makeOptionsInstrCache, CacheFactory::makeOptionsDataCache,
        CacheFactory::makeOptionsL2Cache, BgMem>
        MemTop;
    return dispatchTimingAnalysisJoin<PretPipelineState<MemTop>>(addrInfoTuple);
  }
  default:
    errs() << "No known memory topology chosen.\n";
    return boost::none;
  }
}

} // namespace TimingAnalysisPass
