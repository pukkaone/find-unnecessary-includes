// $Id$
#include "TranslationUnitReader.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendOptions.h"
#include "clang/Frontend/Utils.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Parse/Parser.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Ownership.h"
#include "clang/Sema/Sema.h"

using namespace clang;

TranslationUnitReader::TranslationUnitReader ():
  pTextDiagnosticPrinter_(new TextDiagnosticPrinter(llvm::errs(), diagnosticOptions_)),
  diagnostic_(pDiagIDs_, pTextDiagnosticPrinter_),
  fileManager_(fileSystemOptions_),
  sourceManager_(diagnostic_, fileManager_)
{
  CompilerInvocation::setLangDefaults(langOptions_, IK_CXX);
}

std::string
TranslationUnitReader::read (const std::string& mainFileName)
{
  const FileEntry* pFile = fileManager_.getFile(mainFileName);
  if (pFile == 0) {
    return mainFileName + " not found";
  }

  sourceManager_.createMainFileID(pFile);

  HeaderSearch headerSearch(fileManager_);

  TargetOptions targetOptions;
  targetOptions.Triple = LLVM_HOSTTRIPLE;

  TargetInfo* pTargetInfo = TargetInfo::CreateTargetInfo(
      diagnostic_, targetOptions);

  ApplyHeaderSearchOptions(
          headerSearch,
          headerSearchOptions_,
          langOptions_,
          pTargetInfo->getTriple());

  Preprocessor preprocessor(
          diagnostic_,
          langOptions_,
          *pTargetInfo,
          sourceManager_,
          headerSearch);
  preprocessor.addPPCallbacks(pPreprocessorCallbacks_);

  FrontendOptions frontendOptions;
  InitializePreprocessor(
          preprocessor,
          preprocessorOptions_,
          headerSearchOptions_,
          frontendOptions);

  IdentifierTable identifierTable(langOptions_);
  SelectorTable selectorTable;

  Builtin::Context builtinContext(*pTargetInfo);
  ASTContext astContext(
      langOptions_,
      sourceManager_,
      *pTargetInfo,
      identifierTable,
      selectorTable,
      builtinContext,
      0 /* size_reserve*/);

  Sema sema(preprocessor, astContext, *pAstConsumer_);
  sema.Initialize();

  pTextDiagnosticPrinter_->BeginSourceFile(langOptions_, &preprocessor);
  ParseAST(preprocessor, pAstConsumer_, astContext); 
  pTextDiagnosticPrinter_->EndSourceFile();
  preprocessor.EndSourceFile();

  return "";
}
