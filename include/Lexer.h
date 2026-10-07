#pragma once

#include "Token.h"

namespace GameLang {

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> tokenize();
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;
    bool match(char expected);

    void skipWhitespaceAndComments();
    Token scanToken();
    Token makeToken(TokenType type);
    Token makeErrorToken(const std::string& message);
    Token scanIdentifier();
    Token scanNumber();
    Token scanString();

    TokenType checkKeyword(size_t start, size_t length, const std::string& rest, TokenType type) const;
    TokenType identifierType(const std::string& text) const;

    std::string source;
    size_t start = 0;
    size_t current = 0;
    int line = 1;
    int column = 1;
    int startColumn = 1;

    std::vector<std::string> errors;
};

} // namespace GameLang
