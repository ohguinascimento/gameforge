#pragma once

#include "Common.h"
#include "AST.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace GameLang {

struct SemanticDiagnostic {
    enum class Severity { Error, Warning };
    Severity severity;
    std::string message;
    int line = 0;
    int column = 0;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer() = default;

    bool analyze(const Program& program);

    const std::vector<SemanticDiagnostic>& getDiagnostics() const { return diagnostics; }
    bool hasErrors() const { return errorCount > 0; }
    size_t getErrorCount() const { return errorCount; }
    size_t getWarningCount() const { return warningCount; }

    void printReport(std::ostream& os) const;

private:
    std::vector<SemanticDiagnostic> diagnostics;
    size_t errorCount = 0;
    size_t warningCount = 0;

    std::unordered_set<std::string> declaredEntities;
    std::unordered_map<std::string, std::unordered_set<std::string>> entityFields;
    std::unordered_set<std::string> declaredGlobals;
    std::unordered_map<std::string, size_t> declaredFunctions; // name -> param count
    std::unordered_set<std::string> collisionPairs;

    void addError(const std::string& msg, int line = 0, int col = 0);
    void addWarning(const std::string& msg, int line = 0, int col = 0);

    void collectDeclarations(const Program& program);
    void validateEntities(const Program& program);
    void validateCollisionHandlers(const Program& program);
    void validateFunctions(const Program& program);
    void validateBlock(const BlockStmt* block, const std::unordered_set<std::string>& localScope);
    void validateStatement(const Stmt* stmt, std::unordered_set<std::string>& localScope);
    void validateExpression(const Expr* expr, const std::unordered_set<std::string>& localScope);

    bool isBuiltinFunction(const std::string& name) const;
};

} // namespace GameLang
