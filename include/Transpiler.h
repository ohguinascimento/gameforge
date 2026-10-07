#pragma once

#include "AST.h"
#include <set>
#include <unordered_set>
#include <string>
#include <sstream>

namespace GameLang {

enum class TranspileTarget {
    OpenGL33, // Acelerado por GPU: OpenGL 3.3 Core Profile + Shaders GLSL + Instanciamento
    Console   // Terminal clássico: ANSI Escape Sequences + Double Buffering
};

class Transpiler {
public:
    Transpiler() = default;

    std::string transpileToCpp(const Program& program, TranspileTarget target = TranspileTarget::OpenGL33);

private:
    void emitIndent(std::ostringstream& ss, int indent);
    void transpileExpression(const Expr& expr, std::ostringstream& ss);
    void transpileStatement(const Stmt& stmt, std::ostringstream& ss, int indent);
    void transpileBlock(const BlockStmt& block, std::ostringstream& ss, int indent);

    std::string transpileConsole(const Program& program);
    std::string transpileOpenGL(const Program& program);
};

} // namespace GameLang
