//------------------------------------------------------------------------------
// AST matching sample. Demonstrates:
//------------------------------------------------------------------------------
#include <string>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Lex/Preprocessor.h"
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
#include <cmath>
using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;
unordered_set<string> calleeset;
unordered_map<string, unordered_set<string> > callingmap;

static llvm::cl::OptionCategory MatcherSampleCategory("My Example");

//get the source code of the specific parts of AST
template <typename T>
static std::string getText(const SourceManager &SourceManager, const T &Node) {
  SourceLocation StartSpellingLocation =
      SourceManager.getSpellingLoc(Node.getLocStart());
  SourceLocation EndSpellingLocation =
      SourceManager.getSpellingLoc(Node.getLocEnd());
  if (!StartSpellingLocation.isValid() || !EndSpellingLocation.isValid()) {
    return std::string();
  }
  bool Invalid = true;
  const char *Text =
      SourceManager.getCharacterData(StartSpellingLocation, &Invalid);
  if (Invalid) {
    return std::string();
  }
  std::pair<FileID, unsigned> Start =
      SourceManager.getDecomposedLoc(StartSpellingLocation);
  std::pair<FileID, unsigned> End =
      SourceManager.getDecomposedLoc(Lexer::getLocForEndOfToken(
          EndSpellingLocation, 0, SourceManager, LangOptions()));
  if (Start.first != End.first) {
    // Start and end are in different files.
    return std::string();
  }
  if (End.second < Start.second) {
    // Shuffling text with macros may cause this.
    return std::string();
  }
  return std::string(Text, End.second - Start.second);
}

static std::string trim(const std::string& str,
                 const std::string& whitespace = " \t")
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}




class ErrorHandler : public MatchFinder::MatchCallback {
public:
  ErrorHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}
// get all the callee and caller and store these values to calleeset and callingmap
// calleeset is a complete set for all the callee and callingmap is a map from caller to a set of callee

  virtual void run(const MatchFinder::MatchResult &Result) {
    
    if(const auto *classDecl = Result.Nodes.getNodeAs<CXXRecordDecl>("class")){
      if(classDecl->isThisDeclarationADefinition()){
        const auto& SM = *Result.SourceManager;
        const auto& Loc = classDecl->getLocation();
        //const string classstmt = getText(*Result.SourceManager, *classDecl);
        string classname;
        if(classDecl->isClass()){
          classname = classDecl->getQualifiedNameAsString();
          llvm::outs() << SM.getFilename(Loc)
              << ','
              << SM.getSpellingLineNumber(Loc)
              << ','
              << "class"
              << ','
              << classname 
              <<'\n';
        }
      }
    }
    if(const auto *variableDecl = Result.Nodes.getNodeAs<VarDecl>("var")){
      
        const auto& SM = *Result.SourceManager;
        const auto& Loc = variableDecl->getLocation();
        //const auto& Loc = variableDecl->getInit()->getExprLoc ();
        //const string classstmt = getText(*Result.SourceManager, *classDecl);
        LangOptions lo;
        PrintingPolicy pp(lo);
        pp.SuppressTagKeyword = true;
        string type;
        if(SM.isInMainFile(Loc)&& variableDecl->getType().getTypePtr()->isClassType()){
          type =  variableDecl->getType().getAsString(pp);
        
          llvm::outs() << SM.getFilename(Loc)
              << ','
              << SM.getSpellingLineNumber(Loc)
              << ','
              << "type"
              << ','
              << type 
              <<'\n';
        }
        if(SM.isInMainFile(Loc)&& variableDecl->getType().getTypePtr()->isPointerType()){
            if(variableDecl->getType().getTypePtr()->getPointeeType().getTypePtr()->isClassType()){
              type =  variableDecl->getType().getAsString(pp);
            
              llvm::outs() << SM.getFilename(Loc)
                  << ','
                  << SM.getSpellingLineNumber(Loc)
                  << ','
                  << "type"
                  << ','
                  << type 
                  <<'\n';
            }
        }
        if(SM.isInMainFile(Loc)&& variableDecl->getType().getTypePtr()->isReferenceType()){
            if(variableDecl->getType().getTypePtr()->getPointeeCXXRecordDecl()){
              type =  variableDecl->getType().getAsString(pp);
            
              llvm::outs() << SM.getFilename(Loc)
                  << ','
                  << SM.getSpellingLineNumber(Loc)
                  << ','
                  << "type"
                  << ','
                  << type 
                  <<'\n';
            }
        }
    }    
       

  }

private:
  Rewriter &Rewrite;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers a couple of matchers and runs them on
// the AST.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : HandleError(R),Rewrite(R) {
    static DeclarationMatcher variableDecl = varDecl().bind("var");
    static DeclarationMatcher classDecl = cxxRecordDecl().bind("class");
    
    Matcher.addMatcher(variableDecl, &HandleError); //match the caller and callee
    Matcher.addMatcher(classDecl, &HandleError); //match the caller and callee
  }
  SourceLocation getRealStartLoc(SourceLocation loc){   

    if( loc.isMacroID() ) {
        // Get the start/end expansion locations
        std::pair< SourceLocation, SourceLocation > expansionRange = 
                 this->Rewrite.getSourceMgr().getImmediateExpansionRange( loc );

        // We're just interested in the start location
        loc = expansionRange.first;
    }
    return loc;
  }
   SourceLocation getRealEndLoc(SourceLocation loc){   

    if( loc.isMacroID() ) {
        // Get the start/end expansion locations
        std::pair< SourceLocation, SourceLocation > expansionRange = 
                 this->Rewrite.getSourceMgr().getImmediateExpansionRange( loc );

        // We're just interested in the start location
        loc = expansionRange.second;
    }
    return loc;
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    // Run the matchers when we have the whole TU parsed.
    Matcher.matchAST(Context);   

  }

private:
  ErrorHandler HandleError;
  MatchFinder Matcher;
  Rewriter &Rewrite;
};

class CallbacksProxy : public PPCallbacks
{
public:
    CallbacksProxy(clang::PPCallbacks &master)
    : master(master)
    {
    }
    void InclusionDirective(clang::SourceLocation hashLoc,
                                   const clang::Token &includeTok,
                                   clang::StringRef fileName,
                                   bool isAngled,
                                   clang::CharSourceRange filenameRange,
                                   const clang::FileEntry *file,
                                   clang::StringRef searchPath,
                                   clang::StringRef relativePath,
                                   const clang::Module *imported)
    {
        master.InclusionDirective(hashLoc,
                                  includeTok,
                                  fileName,
                                  isAngled,
                                  filenameRange,
                                  file,
                                  searchPath,
                                  relativePath,
                                  imported);
    }
private:
   clang::PPCallbacks &master;
};

class IncludeFinder : private clang::PPCallbacks
{
public:
    IncludeFinder(const clang::CompilerInstance &compiler)
    : compiler(compiler)
    {
        const clang::FileID mainFile = compiler.getSourceManager().getMainFileID();
        name = compiler.getSourceManager().getFileEntryForID(mainFile)->getName();
    }

public:
    std::unique_ptr<CallbacksProxy> createPreprocessorCallbacks(){
      std::unique_ptr<CallbacksProxy> find_includes_callback(new CallbacksProxy(*this));
      return find_includes_callback;
    }

    void diagnoseAndReport(){
      //llvm::outs()<<"output"<<'\n';
      typedef Includes::iterator It;
      int i = 0;
      for (It it = includes.begin(); it != includes.end(); ++it) {
        llvm::outs() << filenames[i]
              << ','
              << it->first
              << ','
              << "include"
              << ','
              << it->second 
              <<'\n';
        i++;
      }
    }

    virtual void InclusionDirective(clang::SourceLocation hashLoc,
                                  const clang::Token &includeTok,
                                  clang::StringRef fileName,
                                  bool isAngled,
                                  clang::CharSourceRange filenameRange,
                                  const clang::FileEntry *file,
                                  clang::StringRef searchPath,
                                  clang::StringRef relativePath,
                                  const clang::Module *imported)
    {
        clang::SourceManager &sm = compiler.getSourceManager();
//        llvm::outs()<<"debug include"<<'\n';
        //if (sm.isInMainFile(hashLoc)) {
            filenames.push_back(sm.getFilename(hashLoc));
            const unsigned int lineNum = sm.getSpellingLineNumber(hashLoc);
            includes.push_back(std::make_pair(lineNum, fileName));
        //}
    }


private:
    const clang::CompilerInstance &compiler;
    std::string name;
    vector<string> filenames;
    typedef std::pair<int, std::string> IncludeInfo;
    typedef std::vector<IncludeInfo> Includes;
    Includes includes;
};




// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  virtual void ExecuteAction(){
    IncludeFinder includeFinder(getCompilerInstance());
    getCompilerInstance().getPreprocessor().addPPCallbacks(
        includeFinder.createPreprocessorCallbacks()
    );

    clang::ASTFrontendAction::ExecuteAction();

    includeFinder.diagnoseAndReport();
}

  void EndSourceFileAction() override {


  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                StringRef file) override {
   TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, MatcherSampleCategory);
  RefactoringTool Tool(op.getCompilations(), op.getSourcePathList());

  return Tool.runAndSave(newFrontendActionFactory<MyFrontendAction>().get());
}
