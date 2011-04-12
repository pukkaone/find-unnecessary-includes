// $Id$
#ifndef UNUSEDHEADERFINDER_H
#define UNUSEDHEADERFINDER_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Token.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include <map>
#include <set>
#include <string>
#include <vector>

/** map header file to a flag indicating if the header is used */
typedef std::map<const clang::FileEntry*, bool> UsedHeaderMap;

struct IncludeDirective: public llvm::RefCountedBase<IncludeDirective>
{
  typedef llvm::IntrusiveRefCntPtr<IncludeDirective> Ptr;

  /** location of #include directive in source code */
  clang::SourceLocation sourceLocation_;

  /**
   * header file name as it appears in the source without surrounding delimiters
   */
  llvm::StringRef fileName_;

  /** true if angle brackets surrounded the file name in the source */
  bool angled_;

  /** actual file included by #include directive */
  const clang::FileEntry* fileEntry_;

  /** header files that this header file includes directly or indirectly */
  typedef std::set<const clang::FileEntry*> NestedHeaders;
  NestedHeaders nestedHeaders_;

  IncludeDirective(
      clang::SourceLocation hashLoc,
      llvm::StringRef fileName,
      bool angled,
      const clang::FileEntry* fileEntry):
    sourceLocation_(hashLoc),
    fileName_(fileName),
    angled_(angled),
    fileEntry_(fileEntry)
  { }

  /**
   * Checks if any of the header files this header includes is used.
   */
  bool nestedHeaderUsed(const UsedHeaderMap& usedHeaders);
};

/**
 * When the main source file uses a symbol, mark the header file which declares
 * the symbol as used.
 */
class UnusedHeaderFinder:
    public clang::PPCallbacks,
    public clang::ASTConsumer,
    public clang::RecursiveASTVisitor<UnusedHeaderFinder>
{
  clang::SourceManager& sourceManager_;

  // map header to flag indicating the header is used
  UsedHeaderMap usedHeaders_;

  // map header to #include directive that includes it 
  typedef std::map<const clang::FileEntry*, IncludeDirective::Ptr>
      HeaderIncludeDirectiveMap;
  HeaderIncludeDirectiveMap headerIncludeDirectiveMap_;

  // include nesting depth (0 = currently in main file)
  int includeDepth_;

  // #include directives in main file
  typedef std::vector<IncludeDirective::Ptr> IncludeDirectives;
  IncludeDirectives includeDirectives_;

  bool isFromMainFile (clang::SourceLocation sourceLocation)
  { return sourceManager_.isFromMainFile(sourceLocation); }

  const clang::FileEntry* getFileEntry (clang::SourceLocation sourceLocation)
  {
    return sourceManager_.getFileEntryForID(
        sourceManager_.getFileID(sourceLocation));
  }

  void markUsed(
      clang::SourceLocation declarationLocation,
      clang::SourceLocation usageLocation);

  std::string format(clang::SourceLocation sourceLocation);

public:
  UnusedHeaderFinder (clang::SourceManager& sourceManager):
    sourceManager_(sourceManager)
  { }

  /**
   * Creates object to receive notifications of preprocessor events.
   * We need to create a new object because the preprocessor will take
   * ownership of it and invoke the delete operator on it.
   */
  clang::PPCallbacks* createPreprocessorCallbacks();

  virtual void InclusionDirective(
      clang::SourceLocation hashLoc,
      const clang::Token& includeToken,
      llvm::StringRef fileName,
      bool isAngled,
      const clang::FileEntry* pFile,
      clang::SourceLocation endLoc,
      const llvm::SmallVectorImpl<char>& rawPath);

  virtual void FileChanged(
      clang::SourceLocation newLocation,
      clang::PPCallbacks::FileChangeReason reason,
      clang::SrcMgr::CharacteristicKind fileType);

  virtual void MacroExpands(
      const clang::Token& nameToken, const clang::MacroInfo* pMacro);

  virtual void HandleTranslationUnit(clang::ASTContext& astContext);

  // Called when a typedef is used.
  bool VisitTypedefTypeLoc(clang::TypedefTypeLoc typeLoc);

  // Called when a enum, struct or class is used.
  bool VisitTagTypeLoc(clang::TagTypeLoc typeLoc);

  // Called class template is used.
  bool VisitTemplateSpecializationTypeLoc(
      clang::TemplateSpecializationTypeLoc typeLoc);
  
  // Called when a variable, function, or enum constant is used.
  bool VisitDeclRefExpr(clang::DeclRefExpr* pExpr);
};

#endif
