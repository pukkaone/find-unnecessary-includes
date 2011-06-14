// $Id$
#ifndef UNUSEDHEADERFINDER_H
#define UNUSEDHEADERFINDER_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Token.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include <ostream>
#include <set>
#include <string>
#include <vector>

class SourceFile;

/**
 * #include directive appearing in the source code.
 */
class IncludeDirective: public llvm::RefCountedBase<IncludeDirective>
{
  // location of #include directive in source code
  std::string directiveLocation_;

  // header file name as it appears in the source without surrounding delimiters
  std::string fileName_;

  // true if #include directive specified file name between angle brackets
  bool angled_;

public:
  typedef llvm::IntrusiveRefCntPtr<IncludeDirective> Ptr;

  /** header file included by #include directive */
  llvm::IntrusiveRefCntPtr<SourceFile> pHeader_;

  IncludeDirective(
      const std::string& hashLoc,
      llvm::StringRef fileName,
      bool angled):
    directiveLocation_(hashLoc),
    fileName_(fileName.str()),
    angled_(angled)
  { }

  bool angled () const
  { return angled_; }

  /**
   * Outputs file name with quotes as it appears in the source code.
   */
  void printFileName(std::ostream& out);

  /**
   * Outputs beginning of warning message.
   */
  void printWarningPrefix(std::ostream& out);
};

class IncludeDirectiveVisitor
{
public:
  /**
   * Return true if traversal should continue.
   */
  virtual bool visit(IncludeDirective::Ptr pIncludeDirective) = 0;
};

typedef std::set<std::string> UsedHeaders;

/**
 * Main source file or header file.
 */
class SourceFile: public llvm::RefCountedBase<SourceFile>
{
  std::string name_;

public:
  typedef llvm::IntrusiveRefCntPtr<SourceFile> Ptr;

  /** #include directives appearing in the source file */
  typedef std::vector<IncludeDirective::Ptr> IncludeDirectives;
  IncludeDirectives includeDirectives_;

  /** set of header files used by this source file */
  UsedHeaders usedHeaders_;

  SourceFile (const std::string& name):
    name_(name)
  { }

  const std::string& name () const
  { return name_; }

  void traverse(IncludeDirectiveVisitor& visitor);

  /**
   * Checks if any of the headers included by this source file are used.
   */
  bool haveNestedUsedHeader(const UsedHeaders& usedHeaders);

  /**
   * Reports the headers included by this source file that are used.
   */
  void reportNestedUsedHeaders(
      const UsedHeaders& usedHeaders, clang::SourceManager& sourceManager);

  /**
   * Reports unnecessary #include directives in this source file.
   *
   * @return true if an unnecessary #include directive was found
   */
  bool reportUnnecessaryIncludes(
      const UsedHeaders& allUsedHeaders, clang::SourceManager& sourceManager);
};

class UnnecessaryIncludeFinderAction;

/**
 * Finds unnecessary #include directives in translation units.
 *
 * Traverse the AST generated by clang.  For each symbol used by the main
 * source file, mark the header file which defines the symbol as used.  For
 * each #include directive in the main source file, if the header has not been
 * marked as used, report the #include directive as unnecessary.
 */
class UnnecessaryIncludeFinder:
    public clang::PPCallbacks,
    public clang::ASTConsumer,
    public clang::RecursiveASTVisitor<UnnecessaryIncludeFinder>
{
  UnnecessaryIncludeFinderAction& action_;
  clang::SourceManager& sourceManager_;

  // map file to last #include directive that includes it 
  typedef llvm::DenseMap<const clang::FileEntry*, IncludeDirective::Ptr>
      FileToIncludeDirectiveMap;
  FileToIncludeDirectiveMap fileToIncludeDirectiveMap_;

  // map file to source
  typedef llvm::DenseMap<const clang::FileEntry*, SourceFile::Ptr>
      FileToSourceMap;
  FileToSourceMap fileToSourceMap_;

  // stack of included source files.  The first element pushed will be the main
  // source file.
  std::vector<SourceFile::Ptr> includeStack_;

  // current main source file being analyzed
  SourceFile::Ptr pMainSource_;

  bool isFromMainFile (clang::SourceLocation sourceLocation)
  { return sourceManager_.isFromMainFile(sourceLocation); }

  SourceFile::Ptr getSource(const clang::FileEntry* pFile);

  SourceFile::Ptr enterHeader(const clang::FileEntry* pFile);

  void markUsed(
      clang::SourceLocation declarationLocation,
      clang::SourceLocation usageLocation);

public:
  UnnecessaryIncludeFinder (
      UnnecessaryIncludeFinderAction& action,
      clang::SourceManager& sourceManager):
    action_(action),
    sourceManager_(sourceManager)
  { }

  /**
   * Creates object to receive notifications of preprocessor events.
   * We need to create a new object because the preprocessor will take
   * ownership of it and invoke the delete operator on it.
   */
  clang::PPCallbacks* createPreprocessorCallbacks();

#ifdef CLANG_POST_R130246
  virtual void InclusionDirective(
      clang::SourceLocation hashLoc,
      const clang::Token& includeToken,
      llvm::StringRef fileName,
      bool isAngled,
      const clang::FileEntry* pFile,
      clang::SourceLocation endLoc,
      llvm::StringRef SearchPath,
      llvm::StringRef RelativePath);
#else
  virtual void InclusionDirective(
      clang::SourceLocation hashLoc,
      const clang::Token& includeToken,
      llvm::StringRef fileName,
      bool isAngled,
      const clang::FileEntry* pFile,
      clang::SourceLocation endLoc,
      const llvm::SmallVectorImpl<char>& rawPath);
#endif

  virtual void FileChanged(
      clang::SourceLocation newLocation,
      clang::PPCallbacks::FileChangeReason reason,
      clang::SrcMgr::CharacteristicKind fileType);

  virtual void FileSkipped(
      const clang::FileEntry& file,
      const clang::Token& fileNameToken,
      clang::SrcMgr::CharacteristicKind fileType);

  virtual void MacroExpands(
      const clang::Token& nameToken, const clang::MacroInfo* pMacro);

  virtual void HandleTranslationUnit(clang::ASTContext& astContext);

  // Called when a typedef is used.
  bool VisitTypedefTypeLoc(clang::TypedefTypeLoc typeLoc);

  // Called when a enum, struct or class is used.
  bool VisitTagTypeLoc(clang::TagTypeLoc typeLoc);

  // Called when a class template is used.
  bool VisitTemplateSpecializationTypeLoc(
      clang::TemplateSpecializationTypeLoc typeLoc);
  
  // Called when a variable, function, or enum constant is used.
  bool VisitDeclRefExpr(clang::DeclRefExpr* pExpr);
  
  // Called when a class, struct, or union member is used.
  bool VisitMemberExpr(clang::MemberExpr* pExpr);

  // Called when a member function is called.
  bool VisitCXXMemberCallExpr(clang::CXXMemberCallExpr* pExpr);
};

class UnnecessaryIncludeFinderAction: public clang::ASTFrontendAction
{
  friend class UnnecessaryIncludeFinder;

  // all main source files that have been analyzed
  typedef std::vector<SourceFile::Ptr> SourceFiles;
  SourceFiles mainSources_;

  // union of header files used by all main source files
  UsedHeaders allUsedHeaders_;

public:
  virtual clang::ASTConsumer* CreateASTConsumer(
      clang::CompilerInstance& compiler, llvm::StringRef inputFile);

  /**
   * Reports unnecessary #include directives.
   *
   * @return true if any unnecessary #include directives were found
   */
  bool reportUnnecessaryIncludes(clang::SourceManager& sourceManager);
};

#endif
