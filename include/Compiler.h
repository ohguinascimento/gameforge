#pragma once

#include "AST.h"
#include "Bytecode.h"

namespace GameLang {

class Compiler {
public:
    Compiler();

    std::unique_ptr<CompiledGame> compile(const Program& program);
    const std::vector<std::string>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }

private:
    struct Local {
        std::string name;
        int depth = 0;
    };

    struct ScopeContext {
        std::vector<Local> locals;
        int scopeDepth = 0;
    };

    void compileProgram(const Program& program, CompiledGame& game);
    void compileEntity(const EntityDecl& entity, CompiledGame& game);
    void compileFunction(const FunctionDecl& fn, CompiledGame& game);
    void compileCollisionHandler(const CollisionHandlerDecl& ch, CompiledGame& game);

    void compileBlock(const BlockStmt& block, Chunk& chunk, ScopeContext& scope);
    void compileStatement(const Stmt& stmt, Chunk& chunk, ScopeContext& scope);
    void compileVarDecl(const VarDeclStmt& stmt, Chunk& chunk, ScopeContext& scope);
    void compileIf(const IfStmt& stmt, Chunk& chunk, ScopeContext& scope);
    void compileWhile(const WhileStmt& stmt, Chunk& chunk, ScopeContext& scope);
    void compileReturn(const ReturnStmt& stmt, Chunk& chunk, ScopeContext& scope);
    void compileDestroy(const DestroyStmt& stmt, Chunk& chunk, ScopeContext& scope);
    void compileExpression(const Expr& expr, Chunk& chunk, ScopeContext& scope);

    void compileLiteral(const LiteralExpr& expr, Chunk& chunk);
    void compileVariable(const VariableExpr& expr, Chunk& chunk, ScopeContext& scope);
    void compileBinary(const BinaryExpr& expr, Chunk& chunk, ScopeContext& scope);
    void compileUnary(const UnaryExpr& expr, Chunk& chunk, ScopeContext& scope);
    void compileAssign(const AssignExpr& expr, Chunk& chunk, ScopeContext& scope);
    void compileMemberAccess(const MemberAccessExpr& expr, Chunk& chunk, ScopeContext& scope);
    void compileMemberAssign(const MemberAssignExpr& expr, Chunk& chunk, ScopeContext& scope);
    void compileCall(const CallExpr& expr, Chunk& chunk, ScopeContext& scope);
    void compileSpawn(const SpawnExpr& expr, Chunk& chunk);

    // Helpers
    void emitByte(Chunk& chunk, uint8_t byte, int line = 0);
    void emitBytes(Chunk& chunk, uint8_t b1, uint8_t b2, int line = 0);
    void emitOp(Chunk& chunk, OpCode op, int line = 0);
    void emitConstant(Chunk& chunk, Value value, int line = 0);
    size_t emitJump(Chunk& chunk, OpCode op, int line = 0);
    void patchJump(Chunk& chunk, size_t offset);

    int resolveLocal(const ScopeContext& scope, const std::string& name);
    void beginScope(ScopeContext& scope);
    void endScope(ScopeContext& scope, Chunk& chunk);
    void addLocal(ScopeContext& scope, const std::string& name);

    void error(const std::string& message);

    std::vector<std::string> errors;
    std::unordered_map<std::string, bool> declaredGlobals;
};

} // namespace GameLang
