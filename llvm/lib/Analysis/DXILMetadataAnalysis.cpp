//=- DXILMetadataAnalysis.cpp - Representation of Module metadata -*- C++ -*=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/DXILMetadataAnalysis.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"

#define DEBUG_TYPE "dxil-metadata-analysis"

using namespace llvm;
using namespace dxil;

void ModuleMetadataInfo::dump(raw_ostream &OS) const {
  OS << "Shader Model Version : " << ShaderModelVersion.getAsString() << "\n";
  OS << "DXIL Version : " << DXILVersion.getAsString() << "\n";
  OS << "Shader Stage : " << Triple::getEnvironmentTypeName(ShaderStage)
     << "\n";
  OS << "Validator Version : " << ValidatorVersion.getAsString() << "\n";
}

void ModuleMetadataInfo::init(Module &M) {
  Triple TT(Triple(M.getTargetTriple()));
  DXILVersion = TT.getDXILVersion();
  ShaderModelVersion = TT.getOSVersion();
  ShaderStage = TT.getEnvironment();
  NamedMDNode *Entry = M.getOrInsertNamedMetadata("dx.valver");
  if (Entry->getNumOperands() == 0) {
    ValidatorVersion = VersionTuple(1, 0);
  } else {
    auto *ValVerMD = cast<MDNode>(Entry->getOperand(0));
    auto *MajorMD = mdconst::extract<ConstantInt>(ValVerMD->getOperand(0));
    auto *MinorMD = mdconst::extract<ConstantInt>(ValVerMD->getOperand(1));
    ValidatorVersion =
        VersionTuple(MajorMD->getZExtValue(), MinorMD->getZExtValue());
  }
}

//===----------------------------------------------------------------------===//
// DXILMetadataAnalysis and DXILMetadataAnalysisPrinterPass

// Provide an explicit template instantiation for the static ID.
AnalysisKey DXILMetadataAnalysis::Key;

llvm::dxil::ModuleMetadataInfo
DXILMetadataAnalysis::run(Module &M, ModuleAnalysisManager &AM) {
  Data.init(M);
  return Data;
}

PreservedAnalyses
DXILMetadataAnalysisPrinterPass::run(Module &M, ModuleAnalysisManager &AM) {
  llvm::dxil::ModuleMetadataInfo &Data = AM.getResult<DXILMetadataAnalysis>(M);

  Data.dump(OS);
  return PreservedAnalyses::all();
}

//===----------------------------------------------------------------------===//
// DXILMetadataAnalysisLegacyPass

DXILMetadataAnalysisLegacyPass::DXILMetadataAnalysisLegacyPass()
    : ModulePass(ID) {
  initializeDXILMetadataAnalysisLegacyPassPass(
      *PassRegistry::getPassRegistry());
}

DXILMetadataAnalysisLegacyPass::~DXILMetadataAnalysisLegacyPass() = default;

void DXILMetadataAnalysisLegacyPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

bool DXILMetadataAnalysisLegacyPass::runOnModule(Module &M) {
  Data.init(M);
  AnalysisDone = true;
  return false;
}

void DXILMetadataAnalysisLegacyPass::print(raw_ostream &OS,
                                           const Module *) const {
  if (!AnalysisDone) {
    OS << "No module metadata info has been built!\n";
    return;
  }
  Data.dump();
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
LLVM_DUMP_METHOD
void DXILMetadataAnalysisLegacyPass::dump() const { print(dbgs(), nullptr); }
#endif

INITIALIZE_PASS(DXILMetadataAnalysisLegacyPass, "dxil-metadata-analysis",
                "DXIL Module Metadata analysis", false, true)
char DXILMetadataAnalysisLegacyPass::ID = 0;

namespace {

//===----------------------------------------------------------------------===//
// Pass to print - for verification - of analysis information collected in
// DXILMetadataAnalysisLegacyPass

class DXILMetadataAnalysisLegacyPrinter : public ModulePass {
public:
  static char ID;

  DXILMetadataAnalysisLegacyPrinter();

  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

} // namespace

DXILMetadataAnalysisLegacyPrinter::DXILMetadataAnalysisLegacyPrinter()
    : ModulePass(ID) {
  initializeDXILMetadataAnalysisLegacyPrinterPass(
      *PassRegistry::getPassRegistry());
}

void DXILMetadataAnalysisLegacyPrinter::getAnalysisUsage(
    AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<DXILMetadataAnalysisLegacyPass>();
}

bool DXILMetadataAnalysisLegacyPrinter::runOnModule(Module &M) {
  auto &MMI = getAnalysis<DXILMetadataAnalysisLegacyPass>();
  MMI.print(errs(), nullptr);
  return false;
}

INITIALIZE_PASS_BEGIN(DXILMetadataAnalysisLegacyPrinter,
                      "dxil-metadata-analysis-print",
                      "Print DXIL Module Metadata Analysis", false, false)
INITIALIZE_PASS_DEPENDENCY(DXILMetadataAnalysisLegacyPass)
INITIALIZE_PASS_END(DXILMetadataAnalysisLegacyPrinter,
                    "dxil-metadata-analysis-print",
                    "Print DXIL Module Metadata Analysis", false, false)

char DXILMetadataAnalysisLegacyPrinter::ID = 0;
