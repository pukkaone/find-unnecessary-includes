// $Id$
#include "clang/Basic/Version.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/Support/ManagedStatic.h"
#include "UnusedHeaderFinder.h"
#include "version.h"
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace clang;
using namespace llvm;

namespace {

const std::string PROGRAM_NAME("find-unnecessary-includes");
const std::string EXIT_FAILURE_OPT("-c");
int exitStatus = EXIT_SUCCESS;

typedef std::vector<char*> Arguments;

void
handleCommandLineOptions (int argc, char* argv[], Arguments& filteredArgs)
{
  for (int i = 1; i < argc; ++i) {
    if (EXIT_FAILURE_OPT.compare(argv[i]) == 0) {
      exitStatus = EXIT_FAILURE;
      continue;
    }

    filteredArgs.push_back(argv[i]);
  }
}

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
      "  -c                      exit with status 1\n"
      "  -fms-extensions         enable Microsoft extensions\n"
      "\n"
      "Many clang options are also supported.  "
      "See the clang manual for more options.\n";
}

bool
handleFrontEndOptions (FrontendOptions& opt)
{
  if (opt.ShowVersion) {
    std::cout << PROGRAM_NAME << ' ' << FUI_VERSION
        << "\nbased on " << getClangFullVersion() << std::endl;
    return false;
  }

  if (opt.ShowHelp) {
    showHelp();
    return false;
  }

  if (opt.Inputs.empty() || opt.Inputs.at(0).second == "-") {
    showHelp();
    return false;
  }

  return true;
}

class UnnecessaryIncludeFinderAction: public ASTFrontendAction
{
public:
  bool foundUnnecessary_;

  UnnecessaryIncludeFinderAction ():
      foundUnnecessary_(false)
  { }

  virtual ASTConsumer* CreateASTConsumer (
      CompilerInstance& compiler, StringRef inputFile)
  {
    UnusedHeaderFinder* pFinder = new UnusedHeaderFinder(
        compiler.getSourceManager(), foundUnnecessary_);
    compiler.getPreprocessor().addPPCallbacks(
        pFinder->createPreprocessorCallbacks());
    return pFinder;
  }
};

}//namespace

int
main (int argc, char* argv[])
{
  // Process our own command line options and remove them from the arguments
  // before letting the compiler invocation process the arguments.
  Arguments arguments;
  handleCommandLineOptions(argc, argv, arguments);

  CompilerInstance compiler;

  // Create diagnostics so errors while processing command line arguments can
  // be reported.
  compiler.createDiagnostics(argc, argv);

  CompilerInvocation::CreateFromArgs(
      compiler.getInvocation(),
      &arguments[0],
      &arguments[0] + arguments.size(),
      compiler.getDiagnostics());

  if (!handleFrontEndOptions(compiler.getFrontendOpts())) {
    return EXIT_FAILURE;
  }

  compiler.getInvocation().setLangDefaults(IK_CXX);

  if (compiler.getHeaderSearchOpts().UseBuiltinIncludes
   && compiler.getHeaderSearchOpts().ResourceDir.empty())
  {
    compiler.getHeaderSearchOpts().ResourceDir =
        CompilerInvocation::GetResourcesPath(
            argv[0], reinterpret_cast<void*>(showHelp));
  }

  if (compiler.getLangOpts().Microsoft) {
    // Kludge to allow clang to parse Microsoft headers.
    // Visual C++ does name resolution at template instantiation, but clang does
    // name resolution at template definition.  A Microsoft header defines a
    // template referencing _invalid_parameter_noinfo but is not declared at
    // that point. It is declared later in the <xutility> header file.
    compiler.getPreprocessorOpts().addMacroDef(
        "_invalid_parameter_noinfo=__noop");
  }

  UnnecessaryIncludeFinderAction action;
  compiler.ExecuteAction(action);

  llvm_shutdown();
  return action.foundUnnecessary_ ? EXIT_FAILURE : exitStatus;
}
