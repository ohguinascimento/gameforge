#include "Parser.h"

namespace GameLang {

Parser::Parser(std::vector<Token> tokens)
    : tokens(std::move(tokens)) {}

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::previous() const {
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EndOfFile;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType t : types) {
        if (check(t)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error(peek(), message);
    return peek();
}

void Parser::error(const Token& token, const std::string& message) {
    std::ostringstream ss;
    ss << "[Syntax Error] Line " << token.location.line << ":" << token.location.column
       << " near '" << token.text << "': " << message;
    errors.push_back(ss.str());
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == TokenType::Semicolon) return;
        switch (peek().type) {
            case TokenType::Game:
            case TokenType::Entity:
            case TokenType::Var:
            case TokenType::Func:
            case TokenType::Init:
            case TokenType::Update:
            case TokenType::Render:
            case TokenType::On:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Return:
                return;
            default:
                break;
        }
        advance();
    }
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto prog = std::make_unique<Program>();

    while (!isAtEnd()) {
        try {
            if (match(TokenType::Game)) {
                parseGameConfig(*prog);
            } else if (match(TokenType::Entity)) {
                auto ent = parseEntityDecl();
                if (ent) prog->entities.push_back(std::move(ent));
            } else if (match(TokenType::Var)) {
                auto g = parseVarDecl();
                if (g) prog->globals.push_back(std::move(g));
            } else if (match(TokenType::Func)) {
                auto fn = parseFunctionDecl();
                if (fn) prog->functions.push_back(std::move(fn));
            } else if (match(TokenType::Init)) {
                prog->initBlock = parseBlockStmt();
            } else if (match(TokenType::Update)) {
                prog->updateBlock = parseBlockStmt();
            } else if (match(TokenType::Render)) {
                prog->renderBlock = parseBlockStmt();
            } else if (match(TokenType::On)) {
                if (match(TokenType::Collision)) {
                    auto col = parseCollisionHandler();
                    if (col) prog->collisionHandlers.push_back(std::move(col));
                } else {
                    error(peek(), "Expected 'collision' after 'on'");
                    synchronize();
                }
            } else {
                error(peek(), "Unexpected token at top level: '" + peek().text + "'");
                synchronize();
            }
        } catch (...) {
            synchronize();
        }
    }

    return prog;
}

void Parser::parseGameConfig(Program& prog) {
    if (check(TokenType::String)) {
        prog.config.title = advance().text;
    }
    consume(TokenType::LeftBrace, "Expected '{' for game configuration");
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        Token key = consume(TokenType::Identifier, "Expected config setting name (width, height, fps)");
        consume(TokenType::Colon, "Expected ':' after setting name");
        Token val = consume(TokenType::Number, "Expected number value for setting");

        if (key.text == "width") prog.config.width = static_cast<int>(val.numberValue);
        else if (key.text == "height") prog.config.height = static_cast<int>(val.numberValue);
        else if (key.text == "fps") prog.config.fps = static_cast<int>(val.numberValue);

        if (check(TokenType::Comma)) advance();
    }
    consume(TokenType::RightBrace, "Expected '}' after game configuration");
}

std::unique_ptr<EntityDecl> Parser::parseEntityDecl() {
    Token name = consume(TokenType::Identifier, "Expected entity name");
    auto entity = std::make_unique<EntityDecl>();
    entity->name = name.text;

    consume(TokenType::LeftBrace, "Expected '{' before entity body");
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        consume(TokenType::Var, "Expected 'var' to declare entity field");
        Token fieldName = consume(TokenType::Identifier, "Expected field name");
        std::unique_ptr<Expr> defaultVal = nullptr;
        if (match(TokenType::Equal)) {
            defaultVal = parseExpression();
        }
        consume(TokenType::Semicolon, "Expected ';' after field declaration");
        entity->fields.push_back(FieldDecl{fieldName.text, std::move(defaultVal)});
    }
    consume(TokenType::RightBrace, "Expected '}' after entity body");
    return entity;
}

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    Token name = consume(TokenType::Identifier, "Expected function name");
    consume(TokenType::LeftParen, "Expected '(' after function name");

    std::vector<std::string> params;
    if (!check(TokenType::RightParen)) {
        do {
            Token param = consume(TokenType::Identifier, "Expected parameter name");
            params.push_back(param.text);
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RightParen, "Expected ')' after parameters");

    auto body = parseBlockStmt();
    auto fn = std::make_unique<FunctionDecl>();
    fn->name = name.text;
    fn->params = std::move(params);
    fn->body = std::move(body);
    return fn;
}

std::unique_ptr<CollisionHandlerDecl> Parser::parseCollisionHandler() {
    // on collision(EntityA, EntityB) as (a, b) { ... }
    consume(TokenType::LeftParen, "Expected '(' after collision");
    Token a = consume(TokenType::Identifier, "Expected first entity type");
    consume(TokenType::Comma, "Expected ',' between entity types");
    Token b = consume(TokenType::Identifier, "Expected second entity type");
    consume(TokenType::RightParen, "Expected ')' after entity types");

    consume(TokenType::As, "Expected 'as' after collision types");
    consume(TokenType::LeftParen, "Expected '(' for alias variables");
    Token varA = consume(TokenType::Identifier, "Expected first variable alias");
    consume(TokenType::Comma, "Expected ',' between variable aliases");
    Token varB = consume(TokenType::Identifier, "Expected second variable alias");
    consume(TokenType::RightParen, "Expected ')' after alias variables");

    auto body = parseBlockStmt();

    auto col = std::make_unique<CollisionHandlerDecl>();
    col->entityA = a.text;
    col->entityB = b.text;
    col->varA = varA.text;
    col->varB = varB.text;
    col->body = std::move(body);
    return col;
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    if (match(TokenType::Var)) return parseVarDecl();
    if (match(TokenType::If)) return parseIfStmt();
    if (match(TokenType::While)) return parseWhileStmt();
    if (match(TokenType::For)) return parseForStmt();
    if (match(TokenType::Return)) return parseReturnStmt();
    if (match(TokenType::Destroy)) return parseDestroyStmt();
    if (check(TokenType::LeftBrace)) return parseBlockStmt();
    return parseExprStmt();
}

std::unique_ptr<VarDeclStmt> Parser::parseVarDecl() {
    Token name = consume(TokenType::Identifier, "Expected variable name");
    std::unique_ptr<Expr> init = nullptr;
    if (match(TokenType::Equal)) {
        init = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after variable declaration");
    return std::make_unique<VarDeclStmt>(name.text, std::move(init));
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    consume(TokenType::LeftParen, "Expected '(' after 'if'");
    auto cond = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after if condition");

    auto thenBranch = parseStatement();
    std::unique_ptr<Stmt> elseBranch = nullptr;
    if (match(TokenType::Else)) {
        elseBranch = parseStatement();
    }
    return std::make_unique<IfStmt>(std::move(cond), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
    consume(TokenType::LeftParen, "Expected '(' after 'while'");
    auto cond = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after while condition");
    auto body = parseStatement();
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

std::unique_ptr<Stmt> Parser::parseForStmt() {
    consume(TokenType::LeftParen, "Expected '(' after 'for'");
    std::unique_ptr<Stmt> initializer = nullptr;
    if (match(TokenType::Semicolon)) {
        initializer = nullptr;
    } else if (match(TokenType::Var)) {
        initializer = parseVarDecl();
    } else {
        initializer = parseExprStmt();
    }

    std::unique_ptr<Expr> condition = nullptr;
    if (!check(TokenType::Semicolon)) {
        condition = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after for loop condition");

    std::unique_ptr<Expr> increment = nullptr;
    if (!check(TokenType::RightParen)) {
        increment = parseExpression();
    }
    consume(TokenType::RightParen, "Expected ')' after for clauses");

    auto body = parseStatement();

    if (increment) {
        auto block = std::make_unique<BlockStmt>();
        block->statements.push_back(std::move(body));
        block->statements.push_back(std::make_unique<ExprStmt>(std::move(increment)));
        body = std::move(block);
    }

    if (!condition) {
        condition = std::make_unique<LiteralExpr>(true);
    }
    auto whileLoop = std::make_unique<WhileStmt>(std::move(condition), std::move(body));

    if (initializer) {
        auto block = std::make_unique<BlockStmt>();
        block->statements.push_back(std::move(initializer));
        block->statements.push_back(std::move(whileLoop));
        return block;
    }
    return whileLoop;
}

std::unique_ptr<BlockStmt> Parser::parseBlockStmt() {
    consume(TokenType::LeftBrace, "Expected '{'");
    auto block = std::make_unique<BlockStmt>();
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        block->statements.push_back(parseStatement());
    }
    consume(TokenType::RightBrace, "Expected '}'");
    return block;
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    std::unique_ptr<Expr> val = nullptr;
    if (!check(TokenType::Semicolon)) {
        val = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after return");
    return std::make_unique<ReturnStmt>(std::move(val));
}

std::unique_ptr<DestroyStmt> Parser::parseDestroyStmt() {
    auto target = parseExpression();
    consume(TokenType::Semicolon, "Expected ';' after destroy");
    return std::make_unique<DestroyStmt>(std::move(target));
}

std::unique_ptr<ExprStmt> Parser::parseExprStmt() {
    auto expr = parseExpression();
    consume(TokenType::Semicolon, "Expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr));
}

// Expressions
std::unique_ptr<Expr> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment() {
    auto expr = parseLogicalOr();

    if (match(TokenType::Equal) || match(TokenType::PlusEqual) || match(TokenType::MinusEqual)) {
        TokenType op = previous().type;
        auto value = parseAssignment();

        // Check if expr was VariableExpr
        if (auto varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_unique<AssignExpr>(varExpr->name, op, std::move(value));
        }
        // Check if expr was MemberAccessExpr
        if (auto memExpr = dynamic_cast<MemberAccessExpr*>(expr.get())) {
            return std::make_unique<MemberAssignExpr>(std::move(memExpr->object), memExpr->member, op, std::move(value));
        }

        error(previous(), "Invalid assignment target");
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    while (match(TokenType::PipePipe)) {
        TokenType op = previous().type;
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    while (match(TokenType::AmpAmp)) {
        TokenType op = previous().type;
        auto right = parseEquality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto expr = parseComparison();
    while (match(TokenType::EqualEqual) || match(TokenType::BangEqual)) {
        TokenType op = previous().type;
        auto right = parseComparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseComparison() {
    auto expr = parseTerm();
    while (match(TokenType::Greater) || match(TokenType::GreaterEqual) ||
           match(TokenType::Less) || match(TokenType::LessEqual)) {
        TokenType op = previous().type;
        auto right = parseTerm();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseTerm() {
    auto expr = parseFactor();
    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        TokenType op = previous().type;
        auto right = parseFactor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseFactor() {
    auto expr = parseUnary();
    while (match(TokenType::Star) || match(TokenType::Slash) || match(TokenType::Percent)) {
        TokenType op = previous().type;
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(TokenType::Bang) || match(TokenType::Minus)) {
        TokenType op = previous().type;
        auto right = parseUnary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }
    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (true) {
        if (match(TokenType::Dot)) {
            Token member = consume(TokenType::Identifier, "Expected member name after '.'");
            expr = std::make_unique<MemberAccessExpr>(std::move(expr), member.text);
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (match(TokenType::True)) return std::make_unique<LiteralExpr>(true);
    if (match(TokenType::False)) return std::make_unique<LiteralExpr>(false);
    if (match(TokenType::Null)) return std::make_unique<LiteralExpr>();

    if (match(TokenType::Number)) {
        return std::make_unique<LiteralExpr>(previous().numberValue);
    }
    if (match(TokenType::String)) {
        return std::make_unique<LiteralExpr>(previous().text);
    }

    if (match(TokenType::Spawn)) {
        Token entityType = consume(TokenType::Identifier, "Expected entity name after 'spawn'");
        return std::make_unique<SpawnExpr>(entityType.text);
    }

    if (match(TokenType::Identifier)) {
        std::string name = previous().text;
        // Check if it's a function call
        if (match(TokenType::LeftParen)) {
            std::vector<std::unique_ptr<Expr>> args;
            if (!check(TokenType::RightParen)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RightParen, "Expected ')' after function arguments");
            return std::make_unique<CallExpr>(name, std::move(args));
        }
        return std::make_unique<VariableExpr>(name);
    }

    if (match(TokenType::LeftParen)) {
        auto expr = parseExpression();
        consume(TokenType::RightParen, "Expected ')' after grouped expression");
        return expr;
    }

    error(peek(), "Expected expression, found '" + peek().text + "'");
    advance();
    return std::make_unique<LiteralExpr>();
}

} // namespace GameLang
