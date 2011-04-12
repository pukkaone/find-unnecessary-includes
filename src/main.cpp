// $Id$
#include "clang/Basic/Version.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/Support/CommandLine.h"
#include "UnusedHeaderFinder.h"
#include "version.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

using namespace clang;
using namespace llvm;

namespace {

const std::string PROGRAM_NAME("find-unnecessary-includes");

void
showHelp ()
{
  std::cout <<
      "USAGE: " << PROGRAM_NAME << " [options] <inputs>\n"
      "\n"
      "OPTIONS:\n"
      "  --help                  show help\n"
      "  --version               show version\n"
      "  -D<macro>[=def]         define preprocessor macro\n"
      "  -I<dir>                 add include directory\n"
      "\n"
      "Many clang options are also supported.  "
      "See the clang manual for more options.\n";
}

bool
handleFrontEndOptions (FrontendOptions& opt)
{
  if (opt.ShowVersion)
  {
    std::cout << PROGRAM_NAME << ' ' << FUI_VERSION
        << "\nbased on " << getClangFullVersion() << std::endl;
    return false;
  }

  if (opt.ShowHelp)
  {
    showHelp();
    return false;
  }

  if (opt.Inputs.empty() || opt.Inputs.at(0).second == "-")
  {
    showHelp();
    return false;
  }

  return true;
}

class UnnecessaryIncludeFinderAction: public ASTFrontendAction
{
public:
  ASTConsumer* CreateASTConsumer(
      CompilerInstance &compilerInstance, StringRef inputFile)
  {
    UnusedHeaderFinder* pFinder = new UnusedHeaderFinder(
        compilerInstance.getSourceManager());
    compilerInstance.getPreprocessor().addPPCallbacks(
        pFinder->createPreprocessorCallbacks());
    return pFinder;
  }
};

}//namespace

int
main (int argc, char* argv[])
{
  CompilerInstance compilerInstance;

  // Create diagnostics so errors while processing command line arguments can
  // be reported.
  compilerInstance.createDiagnostics(argc, argv);

  CompilerInvocation::CreateFromArgs(
      compilerInstance.getInvocation(),
      argv + 1,
      argv + argc,
      compilerInstance.getDiagnostics());

  if (!handleFrontEndOptions(compilerInstance.getFrontendOpts())) {
    return EXIT_FAILURE;
  }

  compilerInstance.getInvocation().setLangDefaults(IK_CXX);

  UnnecessaryIncludeFinderAction action;
  compilerInstance.ExecuteAction(action);

  return EXIT_SUCCESS;
}
