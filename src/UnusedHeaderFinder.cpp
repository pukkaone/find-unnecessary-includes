// $Id$
#include "UnusedHeaderFinder.h"
#include "clang/AST/Decl.h"
#include "clang/Basic/FileManager.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <utility>

using namespace clang;
using namespace llvm;

bool
IncludeDirective::nestedHeaderUsed (const UsedHeaderMap& usedHeaders)
{
  for (NestedHeaders::iterator pHeader = nestedHeaders_.begin();
      pHeader != nestedHeaders_.end();
      ++pHeader)
  {
    UsedHeaderMap::const_iterator pPair = usedHeaders.find(*pHeader);
    if (pPair != usedHeaders.end() && pPair->second) {
      return true;
    }
  }

  return false;
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
  if (!isFromMainFile(declarationLocation) && isFromMainFile(usageLocation)) {
    const FileEntry* pFile = getFileEntry(declarationLocation);
    usedHeaders_[pFile] = true;
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
  // Is the #include directive in the main file?
  if (isFromMainFile(hashLoc)) {
    // Remember #include directive that included the header.
    IncludeDirective::Ptr pIncludeDirective(
        new IncludeDirective(hashLoc, fileName, isAngled, pFile));
    headerIncludeDirectiveMap_.insert(std::make_pair(
        pFile, pIncludeDirective));
  }
}

void
UnusedHeaderFinder::FileChanged (
    SourceLocation newLocation,
    PPCallbacks::FileChangeReason reason,
    SrcMgr::CharacteristicKind fileType)
{
  if (isFromMainFile(newLocation)) {
      includeDepth_ = 0;
  } else {
    if (reason == PPCallbacks::EnterFile) {
      ++includeDepth_;

      const FileEntry* pFile = getFileEntry(newLocation);
      if (pFile != 0) {
        // Mark the header as unused if we haven't seen it before.
        if (usedHeaders_.count(pFile) == 0) {
          usedHeaders_.insert(std::make_pair(pFile, false));
        }

        if (includeDepth_ == 1) {
          // This is the header directly included by the main file.
          includeDirectives_.push_back(headerIncludeDirectiveMap_[pFile]);
        } else {
          // Remember all headers transitively included from #include directive
          // in the main file.
          includeDirectives_.back()->nestedHeaders_.insert(pFile);
        }
      }
    } else {
      --includeDepth_;
    }
  }
}

void
UnusedHeaderFinder::MacroExpands (
    const Token& nameToken, const MacroInfo* pMacro)
{
  markUsed(pMacro->getDefinitionLoc(), nameToken.getLocation());
}


std::string
UnusedHeaderFinder::format (SourceLocation sourceLocation)
{
  std::string result;
  raw_string_ostream rso(result);
  sourceLocation.print(rso, sourceManager_);
  rso.flush();
  return result;
}

void
UnusedHeaderFinder::HandleTranslationUnit (ASTContext& astContext)
{
  TraverseDecl(astContext.getTranslationUnitDecl());

  for (IncludeDirectives::iterator ppIncludeDirective = includeDirectives_.begin();
      ppIncludeDirective != includeDirectives_.end();
      ++ppIncludeDirective)
  {
    IncludeDirective::Ptr pIncludeDirective(*ppIncludeDirective);

    if (usedHeaders_[pIncludeDirective->fileEntry_] == false)
    {
      bool nestedHeaderUsed =
          pIncludeDirective->nestedHeaderUsed(usedHeaders_);
      if (nestedHeaderUsed && pIncludeDirective->angled_)
      {
        // This header is unused but one of headers it includes is used.
        // Don't complain if the #include directive surrounded the file name
        // with angle brackets.
        continue;
      }

      std::cout << format(pIncludeDirective->sourceLocation_)
          << ": warning: #include "
          << (pIncludeDirective->angled_ ? '<' : '"')
          << pIncludeDirective->fileName_.str()
          << (pIncludeDirective->angled_ ? '>' : '"')
          << ' ';
      if (nestedHeaderUsed) {
        std::cout << "is optional but it includes one or more headers which are used:";
        
        const IncludeDirective::NestedHeaders& nestedHeaders =
            pIncludeDirective->nestedHeaders_;
        for (IncludeDirective::NestedHeaders::const_iterator ppFile = nestedHeaders.begin();
            ppFile != nestedHeaders.end();
            ++ppFile)
        {
          const FileEntry* pFile = *ppFile;
          std::cout << std::endl << pFile->getName();
        }
      } else {
        std::cout << "is unnecessary";
      }

      std::cout << std::endl;
    }
  }
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
  markUsed(
    typeLoc.getTypePtr()->getAsCXXRecordDecl()->getLocation(),
    typeLoc.getTemplateNameLoc());
  return true;
}

bool
UnusedHeaderFinder::VisitDeclRefExpr (DeclRefExpr* pExpr)
{
  markUsed(pExpr->getDecl()->getLocation(), pExpr->getLocation());
  return true;
}
