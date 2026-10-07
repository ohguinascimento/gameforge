#pragma once

#include "AST.h"
#include <set>
#include <unordered_set>

namespace GameLang {

class Transpiler {
public:
    Transpiler() = default;

    std::string transpileToCpp(const Program& program);

private:
    void emitIndent(std::ostringstream& ss, int indent);
    void transpileExpression(const Expr& expr, std::ostringstream& ss);
    void transpileStatement(const Stmt& stmt, std::ostringstream& ss, int indent);
    void transpileBlock(const BlockStmt& block, std::ostringstream& ss, int indent);
};

} // namespace GameLang
