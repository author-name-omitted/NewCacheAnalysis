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

#include "Memory/Classification.h"

namespace TimingAnalysisPass {

namespace dom {
namespace cache {

const char *ClassificationNames[16] = {
    "Bot",    "Hit", "Miss", "L1unknow", "L2Hit",     "5F",  "6F",   "7F",
    "L2Miss", "9F",  "10F",  "11F",      "L2unknown", "13F", "L1PS", "L2PS"};

const Classification CL_BOT((unsigned char)0);
const Classification CL_HIT((unsigned char)1);
const Classification CL_MISS((unsigned char)2); // L2
const Classification CL_UNKNOWN((unsigned char)3);
const Classification CL2_HIT((unsigned char)4);
const Classification CL2_MISS((unsigned char)8);
const Classification CL2_UNKNOWN((unsigned char)12);
const Classification CL_PS((unsigned char)14);
const Classification CL2_PS((unsigned char)15);

} // namespace cache
} // namespace dom

} // namespace TimingAnalysisPass
