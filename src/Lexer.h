#pragma once

namespace HSL
{
    enum class TokenType
    {
        None = 0,
        Identifier,
        Punctuation,
        Literal,
        Type,
        Keyword
    };

    struct Token
    {
        TokenType Type;
        std::string Value;
    };

    class Lexer
    {
    public:
        static std::vector<Token> Lexify(const std::string& src);

    private:
        static bool IsWhitespace(char c);
        static bool IsPunctuation(char c);
        static bool AppendsToPunctuation(char c);
        static bool IsLiteral(const std::string& token);
        static bool IsType(const std::string& token);
        static bool IsKeyword(const std::string& token);
    };
};