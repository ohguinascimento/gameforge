#pragma once

#include "Common.h"
#include "Token.h"

namespace GameLang {

// Forward declarations
struct ASTVisitor;

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual void dump(std::ostream& os, int indent = 0) const = 0;
};

// ================= Expressions =================

struct Expr : public ASTNode {};

struct LiteralExpr : public Expr {
    enum class Kind { Number, String, Boolean, Null } kind;
    double numberVal = 0.0;
    std::string stringVal;
    bool boolVal = false;

    LiteralExpr(double val) : kind(Kind::Number), numberVal(val) {}
    LiteralExpr(std::string val) : kind(Kind::String), stringVal(std::move(val)) {}
    LiteralExpr(bool val) : kind(Kind::Boolean), boolVal(val) {}
    LiteralExpr() : kind(Kind::Null) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct VariableExpr : public Expr {
    std::string name;
    explicit VariableExpr(std::string name) : name(std::move(name)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct BinaryExpr : public Expr {
    std::unique_ptr<Expr> left;
    TokenType op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, TokenType op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct UnaryExpr : public Expr {
    TokenType op;
    std::unique_ptr<Expr> right;

    UnaryExpr(TokenType op, std::unique_ptr<Expr> right)
        : op(op), right(std::move(right)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct AssignExpr : public Expr {
    std::string name;
    TokenType op; // =, +=, -=
    std::unique_ptr<Expr> value;

    AssignExpr(std::string name, TokenType op, std::unique_ptr<Expr> value)
        : name(std::move(name)), op(op), value(std::move(value)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct MemberAccessExpr : public Expr {
    std::unique_ptr<Expr> object;
    std::string member;

    MemberAccessExpr(std::unique_ptr<Expr> object, std::string member)
        : object(std::move(object)), member(std::move(member)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct MemberAssignExpr : public Expr {
    std::unique_ptr<Expr> object;
    std::string member;
    TokenType op; // =, +=, -=
    std::unique_ptr<Expr> value;

    MemberAssignExpr(std::unique_ptr<Expr> object, std::string member, TokenType op, std::unique_ptr<Expr> value)
        : object(std::move(object)), member(std::move(member)), op(op), value(std::move(value)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct CallExpr : public Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(std::string callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::move(callee)), arguments(std::move(args)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct SpawnExpr : public Expr {
    std::string entityType;

    explicit SpawnExpr(std::string entityType)
        : entityType(std::move(entityType)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

// ================= Statements =================

struct Stmt : public ASTNode {};

struct ExprStmt : public Stmt {
    std::unique_ptr<Expr> expr;
    explicit ExprStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct VarDeclStmt : public Stmt {
    std::string name;
    std::unique_ptr<Expr> initializer;

    VarDeclStmt(std::string name, std::unique_ptr<Expr> init)
        : name(std::move(name)), initializer(std::move(init)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct BlockStmt : public Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    void dump(std::ostream& os, int indent = 0) const override;
};

struct IfStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;

    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenB, std::unique_ptr<Stmt> elseB = nullptr)
        : condition(std::move(cond)), thenBranch(std::move(thenB)), elseBranch(std::move(elseB)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct WhileStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body)
        : condition(std::move(cond)), body(std::move(body)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct ReturnStmt : public Stmt {
    std::unique_ptr<Expr> value;
    explicit ReturnStmt(std::unique_ptr<Expr> val = nullptr) : value(std::move(val)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

struct DestroyStmt : public Stmt {
    std::unique_ptr<Expr> target;
    explicit DestroyStmt(std::unique_ptr<Expr> target) : target(std::move(target)) {}

    void dump(std::ostream& os, int indent = 0) const override;
};

// ================= Top-level Declarations =================

struct FieldDecl {
    std::string name;
    std::unique_ptr<Expr> defaultValue;
};

struct EntityDecl : public ASTNode {
    std::string name;
    std::vector<FieldDecl> fields;

    void dump(std::ostream& os, int indent = 0) const override;
};

struct FunctionDecl : public ASTNode {
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<BlockStmt> body;

    void dump(std::ostream& os, int indent = 0) const override;
};

struct CollisionHandlerDecl : public ASTNode {
    std::string entityA;
    std::string entityB;
    std::string varA;
    std::string varB;
    std::unique_ptr<BlockStmt> body;

    void dump(std::ostream& os, int indent = 0) const override;
};

struct Program : public ASTNode {
    GameConfig config;
    std::vector<std::unique_ptr<EntityDecl>> entities;
    std::vector<std::unique_ptr<VarDeclStmt>> globals;
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    std::unique_ptr<BlockStmt> initBlock;
    std::unique_ptr<BlockStmt> updateBlock;
    std::unique_ptr<BlockStmt> renderBlock;
    std::vector<std::unique_ptr<CollisionHandlerDecl>> collisionHandlers;

    void dump(std::ostream& os, int indent = 0) const override;
};

} // namespace GameLang
