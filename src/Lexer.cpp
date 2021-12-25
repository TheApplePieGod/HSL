#include "pch.h"
#include "Lexer.h"

namespace HSL
{
    std::vector<Token> Lexer::Lexify(const std::string& src)
    {
        std::vector<Token> tokens;
        std::string buffer = "";
        for (size_t i = 0; i < src.size(); i++)
        {
            char c = src[i];

            bool isPunctuation = IsPunctuation(c);

            // If there is a dot folling a number, then it is not actually a punctuation character
            if (!buffer.empty() && c == '.' && IsLiteral(buffer))
                isPunctuation = false;

            bool isLineComment = c == '/' && i != src.size() - 1 && src[i + 1] == '/';
            bool isBlockComment = c == '/' && i != src.size() - 1 && src[i + 1] == '*';
            bool isPreprocessor = tokens.size() >= 2 && tokens[tokens.size() - 2].Value == "#";
            bool statementEnd = IsWhitespace(c) || isPunctuation || isLineComment || isBlockComment || isPreprocessor;
            if (!statementEnd) buffer += c;

            // End of a statement
            if (!buffer.empty() && (statementEnd || i == src.size() - 1))
            {
                // Assume identifier if it matches no predefined rules
                TokenType type = TokenType::Identifier;
                
                if (IsLiteral(buffer))
                    type = TokenType::Literal;
                else if (IsType(buffer))
                    type = TokenType::Type;
                else if (IsKeyword(buffer))
                    type = TokenType::Keyword;

                tokens.push_back({
                    type, buffer
                });

                buffer.clear();
            }

            // Line comment
            if (isLineComment)
            {
                i = src.find("\n", i); // Advance the lexer position to the next line
                if (i == std::numeric_limits<size_t>::max()) // If there are no more lines then we are done
                    break;
                continue;
            }

            // Block comment
            if (isBlockComment)
            {
                i = src.find("*/", i); // Advance the lexer position to the first occurance of '*/'
                if (i == std::numeric_limits<size_t>::max()) // If there are end statements then we are done
                    break;
                i++; // second character
                continue;
            }

            // Preprocessor expression
            if (isPreprocessor)
            {
                size_t offset = src.find("\n", i); // Advance the lexer position to the next line

                // Parse everything on that line as one token
                std::string expression;
                if (offset == std::numeric_limits<size_t>::max())
                    expression = src.substr(i);
                else
                    expression = src.substr(i, offset - i);
                tokens.push_back({
                    TokenType::Literal, expression
                });

                i = offset;
                if (i == std::numeric_limits<size_t>::max()) // If there are no more lines then we are done
                    break;
                continue;
            }

            if (isPunctuation)
            {
                if (AppendsToPunctuation(c) &&
                    !tokens.empty() &&
                    tokens.back().Type == TokenType::Punctuation &&
                    AppendsToPunctuation(tokens.back().Value[0])
                ) // Contiguous punctuation get appended as one token
                    tokens.back().Value += c;
                else  // Otherwise add a new token entry
                {
                    tokens.push_back({
                        TokenType::Punctuation, std::string(1, c)
                    });
                }
            }
        }

        return tokens;
    }

    bool Lexer::IsWhitespace(char c)
    {
        return (
            c == ' ' ||
            c == '\n' ||
            c == '\t'
        );
    }

    bool Lexer::IsPunctuation(char c)
    {
        switch (c)
        {
            default: return false;
            case '+':
            case '-':
            case '/':
            case '*':
            case '~':
            case '<':
            case '>':
            case '=':
            case '|':
            case '&':
            case ',':
            case '.':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case ':':
            case ';':
            case '#':
            case '%':
            case '^':
                return true;
        };
    }

    bool Lexer::AppendsToPunctuation(char c)
    {
        switch (c)
        {
            default: return false;
            case '+':
            case '-':
            case '/':
            case '*':
            case '<':
            case '>':
            case '=':
            case '|':
            case '&':
            case '.':
            case '%':
            case '^':
                return true;
        };
    }

    bool Lexer::IsLiteral(const std::string& token)
    {
        bool isNumber = std::regex_match(token, std::regex("-?[0-9]+([\\.][0-9]+)?", std::regex::extended));
        return (
            isNumber        ||
            token == "true" ||
            token == "false"
        );
    }

    bool Lexer::IsType(const std::string& token)
    {
        return (
            (token._Starts_with("vec") && std::isdigit(token[3]))  ||
            (token._Starts_with("bvec") && std::isdigit(token[3])) ||
            (token._Starts_with("ivec") && std::isdigit(token[3])) ||
            (token._Starts_with("uvec") && std::isdigit(token[3])) ||
            (token._Starts_with("dvec") && std::isdigit(token[3])) ||
            (token._Starts_with("mat") && std::isdigit(token[3]) ) || // TODO: add more matrix types
            token == "bool"       || 
            token == "int"        ||
            token == "uint"       ||
            token == "float"      ||
            token == "double"     ||
            token == "void"       ||
            token == "tex2d"      ||
            token == "texCube"    ||
            token == "subpassTex" || 
            token == "buffer"
        );
    }

    bool Lexer::IsKeyword(const std::string& token)
    {
        return (
            token == "const"   ||
            token == "for"     ||
            token == "if"      ||
            token == "else"    ||
            token == "while"   ||
            token == "struct"  ||
            token == "uniform" ||
            token == "return"  ||
            token == "in"      ||
            token == "out"     ||
            token == "flat"
        );
    }
}
