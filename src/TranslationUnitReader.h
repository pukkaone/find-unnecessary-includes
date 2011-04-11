// $Id$
#ifndef TRANSLATIONUNITREADER_H
#define TRANSLATIONUNITREADER_H

#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/FileSystemOptions.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/DiagnosticOptions.h"
#include "clang/Frontend/HeaderSearchOptions.h"
#include "clang/Frontend/PreprocessorOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/PPCallbacks.h"
#include <string>

/**
 * Facade providing a simple API for preprocessing and parsing a translation
 * unit.
 */
class TranslationUnitReader
{
  clang::DiagnosticOptions diagnosticOptions_;
  clang::TextDiagnosticPrinter* pTextDiagnosticPrinter_;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> pDiagIDs_;
  clang::Diagnostic diagnostic_;
  clang::FileSystemOptions fileSystemOptions_;
  clang::FileManager fileManager_;
  clang::SourceManager sourceManager_;
  clang::LangOptions langOptions_;
  clang::HeaderSearchOptions headerSearchOptions_;
  clang::PreprocessorOptions preprocessorOptions_;
  clang::PPCallbacks* pPreprocessorCallbacks_;
  clang::ASTConsumer* pAstConsumer_;

public:
  TranslationUnitReader();

  clang::SourceManager&
  TranslationUnitReader::sourceManager ()
  { return sourceManager_; }

  TranslationUnitReader& msExtensions(bool msExtensions)
  {
    langOptions_.Microsoft = msExtensions;
    return *this;
  }

  TranslationUnitReader& defineMacro(const std::string& macro)
  {
    preprocessorOptions_.addMacroDef(macro);
    return *this;
  }

  TranslationUnitReader& includeDirectory(const std::string& directory)
  {
    headerSearchOptions_.AddPath(
        directory, clang::frontend::Angled, true, false, false);
    return *this;
  }

  TranslationUnitReader& preprocessorCallbacks (
      clang::PPCallbacks* pPreprocessorCallbacks)
  {
    pPreprocessorCallbacks_ = pPreprocessorCallbacks;
    return *this;
  }

  TranslationUnitReader& astConsumer (clang::ASTConsumer* pAstConsumer)
  {
    pAstConsumer_ = pAstConsumer;
    return *this;
  }

  /**
   * Preprocess and parse the translation unit.
   *
   * @return error message (empty string if no error occurred)
   */
  std::string read(const std::string& mainFileName);
};

#endif
