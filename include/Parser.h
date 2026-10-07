#pragma once

#include "Token.h"
#include "AST.h"

namespace GameLang {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::unique_ptr<Program> parseProgram();
    const std::vector<std::string>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }

private:
    // Helper checks
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);
    void error(const Token& token, const std::string& message);
    void synchronize();

    // Top level
    void parseGameConfig(Program& prog);
    std::unique_ptr<EntityDecl> parseEntityDecl();
    std::unique_ptr<FunctionDecl> parseFunctionDecl();
    std::unique_ptr<CollisionHandlerDecl> parseCollisionHandler();

    // Statements
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<VarDeclStmt> parseVarDecl();
    std::unique_ptr<IfStmt> parseIfStmt();
    std::unique_ptr<WhileStmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<BlockStmt> parseBlockStmt();
    std::unique_ptr<ReturnStmt> parseReturnStmt();
    std::unique_ptr<DestroyStmt> parseDestroyStmt();
    std::unique_ptr<ExprStmt> parseExprStmt();

    // Expressions
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();

    std::vector<Token> tokens;
    size_t current = 0;
    std::vector<std::string> errors;
};

} // namespace GameLang
