// $Id$
#include "UnusedHeaderFinder.h"
#include "clang/AST/Decl.h"
#include "clang/Basic/FileManager.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <utility>

using namespace clang;
using namespace llvm;

void
SourceFile::traverse (SourceVisitor& visitor)
{
  if (!visitor.visit(this)) {
    return;
  }

  for (IncludeDirectives::iterator ppInclude = includeDirectives_.begin();
      ppInclude != includeDirectives_.end();
      ++ppInclude)
  {
    IncludeDirective::Ptr pIncludeDirective(*ppInclude);
    pIncludeDirective->pHeader_->traverse(visitor);
  }
}

class UsedSourceFinder: public SourceVisitor
{
  const SourceFile::UsedHeaders& usedHeaders_;
  SourceFile::UsedHeaders visitedHeaders_;

public:
  bool found_;

  UsedSourceFinder (const SourceFile::UsedHeaders& usedHeaders):
    usedHeaders_(usedHeaders),
    found_(false)
  { }

  virtual bool visit (SourceFile* pSource)
  {
    FileID fileID = pSource->fileID_;
    if (visitedHeaders_.count(fileID)) {
      return false;
    }
    visitedHeaders_.insert(fileID);

    if (usedHeaders_.count(fileID)) {
      found_ = true;
      return false;
    }
    return true;
  }
};

bool
SourceFile::haveNestedUsedHeader (SourceFile::Ptr pParentSource)
{
  UsedSourceFinder finder(pParentSource->usedHeaders_);
  traverse(finder);
  return finder.found_;
}

class UsedSourceReporter: public SourceVisitor
{
  const SourceFile::UsedHeaders& usedHeaders_;
  SourceManager& sourceManager_;
  SourceFile::UsedHeaders visitedHeaders_;

public:
  UsedSourceReporter (
      const SourceFile::UsedHeaders& usedHeaders, SourceManager& sourceManager):
    usedHeaders_(usedHeaders),
    sourceManager_(sourceManager)
  { }

  virtual bool visit (SourceFile* pSource)
  {
    FileID fileID = pSource->fileID_;
    if (visitedHeaders_.count(fileID)) {
      return false;
    }
    visitedHeaders_.insert(fileID);

    if (usedHeaders_.count(fileID)) {
      const FileEntry* pFile = sourceManager_.getFileEntryForID(fileID);
      std::cout << std::endl << pFile->getName();
    }
    return true;
  }
};

void
SourceFile::reportNestedUsedHeaders (
    SourceFile::Ptr pParentSource, SourceManager& sourceManager)
{
  UsedSourceReporter reporter(pParentSource->usedHeaders_, sourceManager);
  traverse(reporter);
}

std::string
SourceFile::format (SourceLocation sourceLocation, SourceManager& sourceManager)
{
  std::string result;
  raw_string_ostream rso(result);
  sourceLocation.print(rso, sourceManager);
  rso.flush();
  return result;
}

bool
SourceFile::reportUnnecessaryIncludes (SourceManager& sourceManager)
{
  bool foundUnnecessary = false;

  for (IncludeDirectives::iterator ppInclude = includeDirectives_.begin();
      ppInclude != includeDirectives_.end();
      ++ppInclude)
  {
    IncludeDirective::Ptr pIncludeDirective(*ppInclude);

    SourceFile* pHeader = pIncludeDirective->pHeader_;
    if (usedHeaders_.count(pHeader->fileID_) == false) {
      SourceFile::Ptr pSource(this);
      bool haveNestedUsedHeader = pHeader->haveNestedUsedHeader(pSource);
      if (haveNestedUsedHeader && pIncludeDirective->angled_) {
        // This header is unused but one of headers it includes is used.
        // Don't complain if the #include directive surrounded the file name
        // with angle brackets.
        continue;
      }

      foundUnnecessary = true;
      std::cout << format(pIncludeDirective->directiveLocation_, sourceManager)
          << ": warning: #include "
          << (pIncludeDirective->angled_ ? '<' : '"')
          << pIncludeDirective->fileName_.str()
          << (pIncludeDirective->angled_ ? '>' : '"')
          << ' ';
      if (haveNestedUsedHeader) {
        std::cout << "is optional "
            "but it includes these used headers:";
        pHeader->reportNestedUsedHeaders(pSource, sourceManager);
      } else {
        std::cout << "is unnecessary";
      }

      std::cout << std::endl;
    }
  }

  return foundUnnecessary;
}

namespace {

class PreprocessorCallbacks: public clang::PPCallbacks
{
  UnusedHeaderFinder& delegate_;

public:
  PreprocessorCallbacks (UnusedHeaderFinder& delegate):
    delegate_(delegate)
  { }

  virtual void InclusionDirective (
      SourceLocation hashLoc,
      const clang::Token& includeToken,
      StringRef fileName,
      bool isAngled,
      const FileEntry* pFile,
      SourceLocation endLoc,
      const SmallVectorImpl<char>& rawPath)
  {
    delegate_.InclusionDirective(
        hashLoc, includeToken, fileName, isAngled, pFile, endLoc, rawPath);
  }

  virtual void FileChanged (
      SourceLocation newLocation,
      PPCallbacks::FileChangeReason reason,
      SrcMgr::CharacteristicKind fileType)
  {
    delegate_.FileChanged(newLocation, reason, fileType);
  }

  virtual void FileSkipped (
      const FileEntry& file,
      const Token& fileNameToken,
      SrcMgr::CharacteristicKind fileType)
  {
    delegate_.FileSkipped(file, fileNameToken, fileType);
  }

  virtual void MacroExpands (const Token& nameToken, const MacroInfo* pMacro)
  {
    delegate_.MacroExpands(nameToken, pMacro);
  }
};

}//namespace

PPCallbacks*
UnusedHeaderFinder::createPreprocessorCallbacks ()
{
  return new PreprocessorCallbacks(*this);
}

void
UnusedHeaderFinder::markUsed (
    SourceLocation declarationLocation, SourceLocation usageLocation)
{
  // Is the symbol declared in an included file and is it being used in the
  // main file?
  if (usageLocation.isInvalid() || !isFromMainFile(usageLocation)) {
    return;
  }

  if (declarationLocation.isInvalid()) {
    return;
  }

  FileID declarationFileID = sourceManager_.getFileID(declarationLocation);
  if (declarationFileID != sourceManager_.getMainFileID()) {
    pMainSource_->usedHeaders_.insert(declarationFileID);
  }
}

void
UnusedHeaderFinder::InclusionDirective (
    SourceLocation hashLoc,
    const Token& includeToken,
    StringRef fileName,
    bool isAngled,
    const FileEntry* pFile,
    SourceLocation endLoc,
    const SmallVectorImpl<char>& rawPath)
{
  // Remember #include directive that included the file.
  IncludeDirective::Ptr pIncludeDirective(
      new IncludeDirective(hashLoc, fileName, isAngled));

  fileToIncludeDirectiveMap_.erase(pFile);
  fileToIncludeDirectiveMap_.insert(std::make_pair(pFile, pIncludeDirective));
}

SourceFile::Ptr
UnusedHeaderFinder::getSource (const FileEntry* pFile, FileID fileID)
{
  FileToSourceMap::iterator pPair = fileToSourceMap_.find(pFile);
  if (pPair != fileToSourceMap_.end()) {
    return pPair->second;
  }

  SourceFile::Ptr pSource(new SourceFile(fileID));
  fileToSourceMap_.insert(std::make_pair(pFile, pSource));
  return pSource;
}

SourceFile::Ptr
UnusedHeaderFinder::enterHeader (const FileEntry* pFile, FileID fileID)
{
  SourceFile::Ptr pHeader = getSource(pFile, fileID);

  // Find the #include directive that included this header.
  FileToIncludeDirectiveMap::iterator pPair =
      fileToIncludeDirectiveMap_.find(pFile);
  IncludeDirective::Ptr pIncludeDirective(pPair->second);

  // The #include directive did not have the header during construction.
  // Set it now.
  pIncludeDirective->pHeader_ = pHeader.getPtr();

  // Remember the parent file included this file.
  SourceFile::Ptr pParentSource = includeStack_.top();
  pParentSource->includeDirectives_.push_back(pIncludeDirective);

  return pHeader;
}

void
UnusedHeaderFinder::FileChanged (
    SourceLocation newLocation,
    PPCallbacks::FileChangeReason reason,
    SrcMgr::CharacteristicKind fileType)
{
  if (reason == PPCallbacks::EnterFile) {
    FileID newFileID = sourceManager_.getFileID(newLocation);
    const FileEntry* pFile = sourceManager_.getFileEntryForID(newFileID);
    if (pFile != 0) {
      if (newFileID == sourceManager_.getMainFileID()) {
        // Entering main source file for the first time.
        pMainSource_ = getSource(pFile, newFileID);
        includeStack_.push(pMainSource_);
      } else {
        // Push new header onto include stack.
        SourceFile::Ptr pHeader(enterHeader(pFile, newFileID));
        includeStack_.push(pHeader);
      }
    } else {
      // Entering built-in source.  There's no real file.  Push a dummy source
      // file onto the include stack so there's something to pop when exiting.
      SourceFile::Ptr pHeader(getSource(0, newFileID));
      includeStack_.push(pHeader);
    }
  } else if (reason == PPCallbacks::ExitFile) {
    // Pop include stack.
    includeStack_.pop();
  }
}

void
UnusedHeaderFinder::FileSkipped (
      const FileEntry& file,
      const Token& fileNameToken,
      SrcMgr::CharacteristicKind fileType)
{
  FileID dummyFileID;
  enterHeader(&file, dummyFileID);
}

void
UnusedHeaderFinder::MacroExpands (
    const Token& nameToken, const MacroInfo* pMacro)
{
  // Ignore expansion of builtin macros like __LINE__.
  if (pMacro->isBuiltinMacro() == false) {
    markUsed(pMacro->getDefinitionLoc(), nameToken.getLocation());
  }
}

void
UnusedHeaderFinder::HandleTranslationUnit (ASTContext& astContext)
{
  TraverseDecl(astContext.getTranslationUnitDecl());

  foundUnnecessary_ = pMainSource_->reportUnnecessaryIncludes(sourceManager_);
}

bool
UnusedHeaderFinder::VisitTypedefTypeLoc (TypedefTypeLoc typeLoc)
{
  markUsed(typeLoc.getTypedefDecl()->getLocation(), typeLoc.getBeginLoc());
  return true;
}

bool
UnusedHeaderFinder::VisitTagTypeLoc(TagTypeLoc typeLoc)
{
  markUsed(typeLoc.getDecl()->getLocation(), typeLoc.getBeginLoc());
  return true;
}

bool
UnusedHeaderFinder::VisitTemplateSpecializationTypeLoc (
    TemplateSpecializationTypeLoc typeLoc)
{
  CXXRecordDecl* pCXXRecordDecl = typeLoc.getTypePtr()->getAsCXXRecordDecl();
  if (pCXXRecordDecl) {
    markUsed(pCXXRecordDecl->getLocation(), typeLoc.getTemplateNameLoc());
  }
  return true;
}

bool
UnusedHeaderFinder::VisitDeclRefExpr (DeclRefExpr* pExpr)
{
  markUsed(pExpr->getDecl()->getLocation(), pExpr->getLocation());
  return true;
}

bool
UnusedHeaderFinder::VisitMemberExpr (MemberExpr* pExpr)
{
  markUsed(
      pExpr->getMemberDecl()->getLocation(),
      pExpr->getMemberLoc());
  return true;
}

bool
UnusedHeaderFinder::VisitCXXMemberCallExpr (CXXMemberCallExpr* pExpr)
{
  markUsed(
      pExpr->getMethodDecl()->getLocation(),
      pExpr->getExprLoc());
  return true;
}
