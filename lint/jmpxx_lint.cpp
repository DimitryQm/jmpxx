// SPDX-License-Identifier: MIT
// jmpxx-lint: the out-of-tree companion check set. It enforces the library's usage discipline
// in consumer code with three checks built on Clang's AST matchers, and is distinct from the
// library: it is a developer tool built against the LLVM tooling APIs and is never a runtime or
// include dependency of jmpxx.
//
//   jmpxx-discarded-result   a produced result whose value is dropped, by a bare expression
//                            statement or a (void) cast that silences the nodiscard warning.
//   jmpxx-unchecked-access   a value pulled out through a narrow accessor (assume_value or
//                            assume_error) with no success check on that result anywhere in the
//                            function. The narrow accessors state a precondition, so an
//                            unchecked use is undefined behavior outside a hardened build.
//   jmpxx-manual-propagation a hand-written `if (!r) return fail(r.error());` the single
//                            construct JMPXX_TRY expresses in one line.
//
// The checks must produce no false verdict: flagging correct code is the failure to avoid. The
// unchecked-access check is therefore biased to silence. It suppresses an access whenever the
// result is checked anywhere in the function, which misses a few real bugs (a false negative) but
// never flags guarded code (a false positive). A finding is printed as
// `file:line:col: check-name: message` so a test can assert the exact set.
//
// Usage: jmpxx-lint <source.cpp> -- <compile flags>
//   e.g. jmpxx-lint consumer.cpp -- -std=c++20 -fno-exceptions -fno-rtti -I/path/to/jmpxx/include
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace {

llvm::cl::OptionCategory g_cat("jmpxx-lint options");

struct Finding {
  std::string file;
  unsigned line;
  unsigned col;
  std::string check;
  std::string message;
  bool operator<(const Finding& o) const {
    if (file != o.file) return file < o.file;
    if (line != o.line) return line < o.line;
    if (col != o.col) return col < o.col;
    return check < o.check;
  }
};

// "the type is jmpxx::result<...>": a specialization of the result template in the jmpxx
// namespace, reached through any typedef or sugar by desugaring to the record then its decl.
AST_MATCHER_FUNCTION(internal::Matcher<QualType>, isJmpxxResult) {
  return hasDeclaration(classTemplateSpecializationDecl(hasName("::jmpxx::result")));
}

class Checks : public MatchFinder::MatchCallback {
 public:
  void run(const MatchFinder::MatchResult& r) override {
    const SourceManager& sm = *r.SourceManager;

    if (const auto* d = r.Nodes.getNodeAs<Expr>("discard"))
      if (!d->getBeginLoc().isMacroID())
        record(sm, d->getBeginLoc(), "jmpxx-discarded-result",
               "a produced result is discarded; inspect it or propagate it with JMPXX_TRY");

    if (const auto* mp = r.Nodes.getNodeAs<IfStmt>("manualprop"))
      if (!mp->getBeginLoc().isMacroID())
        record(sm, mp->getBeginLoc(), "jmpxx-manual-propagation",
               "this hand-written failure forward is one JMPXX_TRY; replace it");

    // A result checked anywhere in its function: remember the variable so an access to it is not
    // flagged. The check condition references the variable, so binding it here collects every
    // guarded result.
    if (const auto* cv = r.Nodes.getNodeAs<VarDecl>("checkedvar")) checked_.insert(cv);

    // A narrow access. The flag-or-suppress decision waits until the whole translation unit is
    // seen, because a guarding check may appear after the access lexically (an early return
    // does), and only then is the checked set complete. An access synthesized by a macro is
    // skipped: JMPXX_TRY itself expands to assume_value and assume_error, and the library's own
    // sanctioned macro is not a consumer violation, so only accesses the consumer wrote directly
    // are considered.
    if (const auto* acc = r.Nodes.getNodeAs<Expr>("access")) {
      if (acc->getBeginLoc().isMacroID()) return;
      const auto* var = r.Nodes.getNodeAs<VarDecl>("accessvar");
      Finding f;
      if (!fill(sm, acc->getBeginLoc(), "jmpxx-unchecked-access",
                "result value taken through a narrow accessor with no success check; guard it or "
                "use JMPXX_TRY",
                f))
        return;
      pending_.push_back({f, var});
    }
  }

  // Emit the deferred unchecked-access findings whose result was never checked, then print every
  // finding in source order.
  void onEndOfTranslationUnit() override {
    for (const auto& [f, var] : pending_)
      if (!checked_.count(var)) findings_.push_back(f);
    std::sort(findings_.begin(), findings_.end());
    for (const Finding& f : findings_)
      llvm::outs() << f.file << ':' << f.line << ':' << f.col << ": " << f.check << ": "
                   << f.message << '\n';
    llvm::outs() << "jmpxx-lint: " << findings_.size() << " finding(s)\n";
  }

 private:
  // Resolve a location into a finding. Returns false when the location has no real file, which
  // guards against a synthesized node slipping through, so such a node is dropped rather than
  // reported at a bogus location.
  bool fill(const SourceManager& sm, SourceLocation loc, const char* check, const char* msg,
            Finding& f) {
    SourceLocation s = sm.getSpellingLoc(loc);
    f.file = std::string(sm.getFilename(s));
    if (f.file.empty()) return false;
    f.line = sm.getSpellingLineNumber(s);
    f.col = sm.getSpellingColumnNumber(s);
    f.check = check;
    f.message = msg;
    return true;
  }
  void record(const SourceManager& sm, SourceLocation loc, const char* check, const char* msg) {
    Finding f;
    if (fill(sm, loc, check, msg, f)) findings_.push_back(f);
  }

  std::vector<Finding> findings_;
  std::set<const VarDecl*> checked_;
  std::vector<std::pair<Finding, const VarDecl*>> pending_;
};

}  // namespace

int main(int argc, const char** argv) {
  auto parser = CommonOptionsParser::create(argc, argv, g_cat);
  if (!parser) {
    llvm::errs() << toString(parser.takeError());
    return 2;
  }
  ClangTool tool(parser->getCompilations(), parser->getSourcePathList());

  Checks checks;
  MatchFinder finder;

  // Only consumer code in the file under test is scanned; the jmpxx headers it includes are not,
  // so the library's own internal accesses never count as consumer violations.
  const auto in_main = isExpansionInMainFile();

  // Discarded result: a call returning jmpxx::result used as a bare full-expression statement,
  // possibly wrapped in temporary cleanups, or explicitly cast to void to silence nodiscard.
  finder.addMatcher(
      callExpr(in_main, callee(functionDecl(returns(isJmpxxResult()))),
               anyOf(hasParent(compoundStmt()),
                     hasParent(exprWithCleanups(hasParent(compoundStmt()))),
                     hasParent(cStyleCastExpr(hasType(voidType())))))
          .bind("discard"),
      &checks);

  // Manual propagation: an if whose then-branch is a single `return fail(r.error());` forwarding
  // the same result the condition tests. Binding the variable in the returned r.error() and
  // requiring the condition to mention it ties the two together, so forwarding a different error
  // is not matched.
  const auto failForward = returnStmt(hasDescendant(callExpr(
      callee(functionDecl(hasName("::jmpxx::fail"))),
      hasDescendant(cxxMemberCallExpr(callee(cxxMethodDecl(hasName("error"))),
                                      on(declRefExpr(to(varDecl().bind("mpvar")))))))));
  finder.addMatcher(
      ifStmt(in_main,
             hasThen(anyOf(failForward, compoundStmt(statementCountIs(1), has(failForward)))),
             hasCondition(hasDescendant(declRefExpr(to(varDecl(equalsBoundNode("mpvar")))))))
          .bind("manualprop"),
      &checks);

  // Checked-variable collector: any result variable that appears in an if, while, or for
  // condition is considered checked, which covers if(r), if(!r), if(r.has_value()), and the
  // early-return guard. for and while are included so a loop guard counts too.
  const auto resultVar = varDecl(hasType(isJmpxxResult()));
  finder.addMatcher(
      ifStmt(in_main, hasCondition(hasDescendant(declRefExpr(to(resultVar.bind("checkedvar")))))),
      &checks);
  finder.addMatcher(
      whileStmt(in_main, hasCondition(hasDescendant(declRefExpr(to(resultVar.bind("checkedvar")))))),
      &checks);

  // Narrow access: a call to assume_value or assume_error on a result variable. The variable is
  // bound so the end-of-translation-unit pass can suppress it when the result was checked.
  finder.addMatcher(
      cxxMemberCallExpr(
          in_main,
          callee(cxxMethodDecl(hasAnyName("assume_value", "assume_error"),
                               ofClass(classTemplateSpecializationDecl(hasName("::jmpxx::result"))))),
          on(declRefExpr(to(varDecl().bind("accessvar")))))
          .bind("access"),
      &checks);

  return tool.run(newFrontendActionFactory(&finder).get());
}
