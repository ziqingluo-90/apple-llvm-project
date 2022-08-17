//===- DependencyScanningTool.cpp - clang-scan-deps service ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Tooling/DependencyScanning/DependencyScanningTool.h"
#include "clang/Frontend/Utils.h"

using namespace clang;
using namespace tooling;
using namespace dependencies;

static std::vector<std::string>
makeTUCommandLineWithoutPaths(ArrayRef<std::string> OriginalCommandLine) {
  std::vector<std::string> Args = OriginalCommandLine;

  Args.push_back("-fno-implicit-modules");
  Args.push_back("-fno-implicit-module-maps");

  // These arguments are unused in explicit compiles.
  llvm::erase_if(Args, [](StringRef Arg) {
    if (Arg.consume_front("-fmodules-")) {
      return Arg.startswith("cache-path=") ||
             Arg.startswith("prune-interval=") ||
             Arg.startswith("prune-after=") ||
             Arg == "validate-once-per-build-session";
    }
    return Arg.startswith("-fbuild-session-file=");
  });

  return Args;
}

DependencyScanningTool::DependencyScanningTool(
    DependencyScanningService &Service,
    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS)
    : Worker(Service, std::move(FS)) {}

llvm::Expected<std::string> DependencyScanningTool::getDependencyFile(
    const std::vector<std::string> &CommandLine, StringRef CWD,
    llvm::Optional<StringRef> ModuleName) {
  /// Prints out all of the gathered dependencies into a string.
  class MakeDependencyPrinterConsumer : public DependencyConsumer {
  public:
    void handleSimpleDriverJob(std::string Executable, std::vector<std::string> Args) override {}
    void handleInvocation(CompilerInvocation CI) override {}

    void
    handleDependencyOutputOpts(const DependencyOutputOptions &Opts) override {
      this->Opts = std::make_unique<DependencyOutputOptions>(Opts);
    }

    void handleFileDependency(StringRef File) override {
      Dependencies.push_back(std::string(File));
    }

    void handlePrebuiltModuleDependency(PrebuiltModuleDep PMD) override {
      // Same as `handleModuleDependency`.
    }

    void handleModuleDependency(ModuleDeps MD) override {
      // These are ignored for the make format as it can't support the full
      // set of deps, and handleFileDependency handles enough for implicitly
      // built modules to work.
    }

    void handleContextHash(std::string Hash) override {}

    std::string lookupModuleOutput(const ModuleID &ID,
                                   ModuleOutputKind Kind) override {
      llvm::report_fatal_error("unexpected call to lookupModuleOutput");
    }

    void printDependencies(std::string &S) {
      assert(Opts && "Handled dependency output options.");

      class DependencyPrinter : public DependencyFileGenerator {
      public:
        DependencyPrinter(DependencyOutputOptions &Opts,
                          ArrayRef<std::string> Dependencies)
            : DependencyFileGenerator(Opts) {
          for (const auto &Dep : Dependencies)
            addDependency(Dep);
        }

        void printDependencies(std::string &S) {
          llvm::raw_string_ostream OS(S);
          outputDependencyFile(OS);
        }
      };

      DependencyPrinter Generator(*Opts, Dependencies);
      Generator.printDependencies(S);
    }

  private:
    std::unique_ptr<DependencyOutputOptions> Opts;
    std::vector<std::string> Dependencies;
  };

  MakeDependencyPrinterConsumer Consumer;
  auto Result =
      Worker.computeDependencies(CWD, CommandLine, Consumer, ModuleName);
  if (Result)
    return std::move(Result);
  std::string Output;
  Consumer.printDependencies(Output);
  return Output;
}

llvm::Expected<FullDependenciesResult>
DependencyScanningTool::getFullDependencies(
    const std::vector<std::string> &CommandLine, StringRef CWD,
    const llvm::StringSet<> &AlreadySeen,
    LookupModuleOutputCallback LookupModuleOutput,
    llvm::Optional<StringRef> ModuleName) {
  FullDependencyConsumer Consumer(AlreadySeen, LookupModuleOutput);
  llvm::Error Result =
      Worker.computeDependencies(CWD, CommandLine, Consumer, ModuleName);
  if (Result)
    return std::move(Result);
  return Consumer.takeFullDependencies();
}

llvm::Expected<FullDependenciesResult>
DependencyScanningTool::getFullDependenciesLegacyDriverCommand(
    const std::vector<std::string> &CommandLine, StringRef CWD,
    const llvm::StringSet<> &AlreadySeen,
    LookupModuleOutputCallback LookupModuleOutput,
    llvm::Optional<StringRef> ModuleName) {
  FullDependencyConsumer Consumer(AlreadySeen, LookupModuleOutput);
  llvm::Error Result =
      Worker.computeDependencies(CWD, CommandLine, Consumer, ModuleName);
  if (Result)
    return std::move(Result);
  return Consumer.getFullDependenciesLegacyDriverCommand(CommandLine);
}

Command::~Command() = default;
CC1Command::CC1Command(CompilerInvocation CI) : Command(CK_CC1), BuildInvocation(std::move(CI)) {}
std::string CC1Command::getExecutable() {
  // FIXME: get the real value
  return "clang";
}
std::vector<std::string> CC1Command::getArguments() {
  return serializeCompilerInvocation(BuildInvocation);
}

void FullDependencyConsumer::handleInvocation(CompilerInvocation CI) {
  clearImplicitModuleBuildOptions(CI);
  Commands.push_back(std::make_unique<CC1Command>(std::move(CI)));
}

FullDependenciesResult FullDependencyConsumer::takeFullDependencies() {
  FullDependenciesResult FDR;
  FullDependencies &FD = FDR.FullDeps;

  FD.ID.ContextHash = std::move(ContextHash);
  FD.FileDeps = std::move(Dependencies);
  FD.PrebuiltModuleDeps = std::move(PrebuiltModuleDeps);
  FD.Commands = std::move(Commands);

  for (auto &&M : ClangModuleDeps) {
    auto &MD = M.second;
    if (MD.ImportedByMainFile)
      FD.ClangModuleDeps.push_back(MD.ID);
    // TODO: Avoid handleModuleDependency even being called for modules
    //   we've already seen.
    if (AlreadySeen.count(M.first))
      continue;
    FDR.DiscoveredModules.push_back(std::move(MD));
  }

  for (auto &Command : FD.Commands) {
    // FIXME: should this only apply to certain commands?
    if (auto *CC1 = dyn_cast<CC1Command>(Command.get())) {
      auto &FrontendOpts = CC1->BuildInvocation.getFrontendOpts();
      for (const auto &PMD : FD.PrebuiltModuleDeps)
        FrontendOpts.ModuleFiles.push_back(PMD.PCMFile);
      for (const auto &ID : FD.ClangModuleDeps)
        FrontendOpts.ModuleFiles.push_back(LookupModuleOutput(ID, ModuleOutputKind::ModuleFile));
    }
  }
  

  return FDR;
}

FullDependenciesResult FullDependencyConsumer::getFullDependenciesLegacyDriverCommand(
    const std::vector<std::string> &OriginalCommandLine) const {
  FullDependencies FD;

  FD.DriverCommandLine = makeTUCommandLineWithoutPaths(
      ArrayRef<std::string>(OriginalCommandLine).slice(1));

  FD.ID.ContextHash = std::move(ContextHash);

  FD.FileDeps.assign(Dependencies.begin(), Dependencies.end());

  for (const PrebuiltModuleDep &PMD : PrebuiltModuleDeps)
    FD.DriverCommandLine.push_back("-fmodule-file=" + PMD.PCMFile);

  for (auto &&M : ClangModuleDeps) {
    auto &MD = M.second;
    if (MD.ImportedByMainFile) {
      FD.ClangModuleDeps.push_back(MD.ID);
      FD.DriverCommandLine.push_back(
          "-fmodule-file=" +
          LookupModuleOutput(MD.ID, ModuleOutputKind::ModuleFile));
    }
  }

  FD.PrebuiltModuleDeps = std::move(PrebuiltModuleDeps);

  FullDependenciesResult FDR;

  for (auto &&M : ClangModuleDeps) {
    // TODO: Avoid handleModuleDependency even being called for modules
    //   we've already seen.
    if (AlreadySeen.count(M.first))
      continue;
    FDR.DiscoveredModules.push_back(std::move(M.second));
  }

  FDR.FullDeps = std::move(FD);
  return FDR;
}
