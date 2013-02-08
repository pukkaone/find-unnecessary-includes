// $Id$
#include "clang/Basic/Version.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/ManagedStatic.h"
#include "UnnecessaryIncludeFinder.h"
#include "version.h"
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace clang;
using namespace llvm;

namespace {

const std::string PROGRAM_NAME("find-unnecessary-includes");

typedef std::vector<char*> Arguments;

void
handleCommandLineOptions (int argc, char* argv[], Arguments& filteredArgs)
{
  for (int i = 1; i < argc; ++i) {
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
      "  -include <file>         include file before main source\n"
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

  if (opt.Inputs.empty() || opt.Inputs.at(0).getFile() == "-") {
    showHelp();
    return false;
  }

  return true;
}

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
  compiler.createDiagnostics();

  CompilerInvocation::CreateFromArgs(
      compiler.getInvocation(),
      &arguments[0],
      &arguments[0] + arguments.size(),
      compiler.getDiagnostics());

  if (!handleFrontEndOptions(compiler.getFrontendOpts())) {
    return EXIT_FAILURE;
  }

  if (compiler.getHeaderSearchOpts().UseBuiltinIncludes
   && compiler.getHeaderSearchOpts().ResourceDir.empty())
  {
    compiler.getHeaderSearchOpts().ResourceDir =
        CompilerInvocation::GetResourcesPath(
            argv[0], reinterpret_cast<void*>(showHelp));
  }

  if (compiler.getLangOpts().MicrosoftMode) {
    // TODO: Kludge to allow clang to parse Microsoft headers.
    // Visual C++ does name resolution at template instantiation, but clang does
    // name resolution at template definition.  A Microsoft header defines a
    // template referencing _invalid_parameter_noinfo but is not declared at
    // that point. It is declared later in the <xutility> header file.
  }

  UnnecessaryIncludeFinderAction action;
  compiler.ExecuteAction(action);
  bool foundUnnecessary = action.reportUnnecessaryIncludes(
      compiler.getSourceManager());

  llvm_shutdown();
  return foundUnnecessary ? EXIT_FAILURE : EXIT_SUCCESS;
}
