#include "pch.h"
#include "Parser.h"

namespace HSL
{
    ParseNode Parser::Parse(const std::vector<Token>& tokens)
    {
        return ParseBlock(tokens, 0);
    }

    ParseNode Parser::ParseBasic(const std::vector<Token>& tokens, u32 offset)
    {
        ParseNode node;

        int tokenCount = tokens.size() - offset;
        if (tokenCount <= 0) throw std::exception("Missing expression");

        if (tokens[offset].Type == TokenType::Identifier)
        {
            ParseNode identifierNode = {
                NodeType::Identifier,
                ParseData::Identifier::Create(tokens[offset].Value),
                offset, offset
            };

            if (tokenCount >= 2 && tokens[offset + 1].Value == ".") // Member expression
            {
                ParseNode property = ParseBasic(tokens, offset + 2);

                // Object is the current identifier
                node.Type = NodeType::MemberExpression;
                node.Data = ParseData::MemberExpression::Create(
                    identifierNode,
                    property
                );
                node.Start = offset;
                node.End = property.End;
            }
            else if (tokenCount >= 2 && tokens[offset + 1].Value == "(") // Function call
            {
                auto parsedFunc = ParseList(tokens, offset + 2, ")");

                // Left is the current identifier
                node.Type = NodeType::CallExpression;
                node.Data = ParseData::CallExpression::Create(
                    identifierNode,
                    parsedFunc.Elements
                );
                node.Start = offset;
                node.End = parsedFunc.LastToken;
            }
            else if (tokenCount >= 2 && IsBasicOperator(tokens[offset + 1].Value)) // Basic expression
            {
                ParseNode right = ParseBasic(tokens, offset + 2);

                // Left is the current identifier
                node.Type = NodeType::BinaryExpression;
                node.Data = ParseData::BinaryExpression::Create(
                    tokens[offset + 1].Value,
                    identifierNode,
                    right
                );
                node.Start = offset;
                node.End = right.End;
            }
            else // Just the identifier
                node = identifierNode;
        }
        else if (tokens[offset].Type == TokenType::Literal)
        {
            ParseNode literalNode = {
                NodeType::Literal,
                ParseData::Literal::Create(tokens[offset].Value),
                offset, offset
            };

            if (tokenCount >= 2 && IsBasicOperator(tokens[offset + 1].Value)) // Basic expression
            {
                ParseNode right = ParseBasic(tokens, offset + 2);

                // Left is the current literal
                node.Type = NodeType::BinaryExpression;
                node.Data = ParseData::BinaryExpression::Create(
                    tokens[offset + 1].Value,
                    literalNode,
                    right
                );
                node.Start = offset;
                node.End = right.End;
            }
            else // Just the literal
                node = literalNode;
        }
        else if (tokens[offset].Type == TokenType::Type)
        {
            ParseNode typeNode = {
                NodeType::Literal,
                ParseData::Literal::Create(tokens[offset].Value),
                offset, offset
            };

            if (tokenCount >= 2 && tokens[offset + 1].Value == "(") // Type cast
            {
                ParseNode right = ParseBasic(tokens, offset + 2);

                node.Type = NodeType::CastExpression;
                node.Data = ParseData::CastExpression::Create(
                    tokens[offset].Value,
                    right
                );
                node.Start = offset;
                node.End = right.End;
            }
            else // Invalid
                throw std::exception("Unexpected type token");
        }
        else if (tokens[offset].Value == "{") // Initializer list
        {
            auto parsedList = ParseList(tokens, offset + 1, "}");

            // Left is the current identifier
            node.Type = NodeType::ListExpression;
            node.Data = ParseData::ListExpression::Create(
                parsedList.Elements
            );
            node.Start = offset;
            node.End = parsedList.LastToken;
        }
        else
            throw std::exception("Invalid syntax");

        return node;
    }

    Parser::ParseListReturn Parser::ParseList(const std::vector<Token>& tokens, u32 offset, const std::string& endChar)
    {
        ParseListReturn returnVal;

        // Find the location of the ) token which indicates the end of the call signature
        auto endLoc = std::find_if(tokens.begin() + offset, tokens.end(), [endChar](const Token& val){ return val.Value == endChar; });
        if (endLoc == tokens.end()) throw std::exception(("Expecting " + endChar).c_str());
        size_t statementEnd = endLoc - tokens.begin();

        size_t i = offset;
        for (i; i < statementEnd; i++)
        {
            // Parse the expression
            returnVal.Elements.push_back(ParseBasic(tokens, i));

            // Find the next , token which indicates the next argument
            auto commaLoc = std::find_if(tokens.begin() + i, tokens.end(), [](const Token& val){ return val.Value == ","; });

            // If no commas are found inside the block then we are done
            if (commaLoc == tokens.end() || commaLoc - tokens.begin() > statementEnd)
                break;

            i = commaLoc - tokens.begin();
        }

        returnVal.LastToken = i + 1;

        return returnVal;
    }

    Parser::ParseFunctionDeclarationReturn Parser::ParseFunctionDeclaration(const std::vector<Token>& tokens, u32 offset)
    {
        ParseFunctionDeclarationReturn returnVal;

        // Find the location of the ) token which indicates the end of the call signature
        auto endLoc = std::find_if(tokens.begin() + offset, tokens.end(), [](const Token& val){ return val.Value == ")"; });
        if (endLoc == tokens.end()) throw std::exception("Expecting )");
        size_t statementEnd = endLoc - tokens.begin();

        size_t i = offset;
        for (i; i < statementEnd; i++)
        {
            // Parse the expression
            if (i + 1 >= tokens.size()) throw std::exception("Unexpected end of function declaration");
            if (tokens[i].Type != TokenType::Type) throw std::exception("Expected a parameter type");
            if (tokens[i + 1].Type != TokenType::Identifier) throw std::exception("Expected an identifier parameter name");
            returnVal.Params.push_back({
                tokens[i].Value,
                tokens[i + 1].Value
            });

            // Find the next , token which indicates the next argument
            auto commaLoc = std::find_if(tokens.begin() + i, tokens.end(), [](const Token& val){ return val.Value == ","; });

            // If no commas are found inside the block then we are done
            if (commaLoc == tokens.end() || commaLoc - tokens.begin() > statementEnd)
                break;

            i = commaLoc - tokens.begin();
        }

        returnVal.LastToken = i + 2;

        return returnVal;
    }

    ParseNode Parser::ParseStatement(const std::vector<Token>& tokens, u32 offset)
    {
        ParseNode node; // Once our node type is set, we have determined what type of statement this is, and an error is thrown if it is set again
        node.Start = offset;
        node.End = offset;

        if (static_cast<int>(tokens.size()) - offset <= 0) return node;

        // Find the next semicolon / close brace because one of them will be the scope of this node
        auto braceLoc = std::find_if(tokens.begin() + offset, tokens.end(), [](const Token& val){ return val.Value == "}"; });
        auto semiLoc = std::find_if(tokens.begin() + offset, tokens.end(), [](const Token& val){ return val.Value == ";"; });
        size_t statementEnd = semiLoc - tokens.begin() + offset;

        bool isConst = false;
        ParseNode leftNode;
        std::string assignmentOperator;
        for (size_t i = offset; i < statementEnd; i++)
        {
            auto& token = tokens[i];

            if (token.Type == TokenType::Keyword) // Check for special start of statement keywords
            {
                if (token.Value == "const") isConst = true;
            }
            else if (token.Type == TokenType::Type) // Some sort of declaration
            {
                if (i + 2 < statementEnd && tokens[i + 2].Value == "(") // Function declaration
                {
                    if (node.Type != NodeType::None) throw std::exception("Unexpected function declaration");
                    node.Type = NodeType::FunctionDeclaration;

                    // Update the statement end to reflect the end of the block instead of a semicolon
                    if (braceLoc == tokens.end()) throw std::exception("Expected a }");
                    statementEnd = braceLoc - tokens.begin() + offset;

                    // Parse the function declaration
                    auto parsedDeclaration = ParseFunctionDeclaration(tokens, i + 3);

                    // Make sure the following character is the start of a block
                    if (parsedDeclaration.LastToken + 1 >= tokens.size() || tokens[parsedDeclaration.LastToken + 1].Value != "{")
                        throw std::exception("Expected a { after function declaration");

                    // Parse the function body
                    ParseNode body = ParseBlock(tokens, parsedDeclaration.LastToken + 1);

                    node.Data = ParseData::FunctionDeclaration::Create(
                        isConst,
                        token.Value,
                        parsedDeclaration.Params,
                        tokens[i + 1].Value,
                        body
                    );
                    node.End = body.End;

                    // Return right away
                    return node;
                }
                else if (i + 2 <= statementEnd) // Variable declaration
                {
                    if (node.Type != NodeType::None) throw std::exception("Unexpected variable declaration");
                    node.Type = NodeType::VariableDeclaration;

                    int arrayCount = 0;
                    u32 endingOffset = 2;

                    if (tokens[i + 2].Value == "[") // Array variable
                    {
                        try
                        { arrayCount = stoi(tokens[i + 3].Value); }
                        catch (std::exception e)
                        { throw std::exception("Expected literal in variable array declaration"); }

                        if (tokens[i + 4].Value != "]")
                            throw std::exception("Expected ] after variable array declaration");

                        endingOffset = 5;
                    }

                    if (tokens[i + endingOffset].Value == "=")
                    {
                        ParseNode parsedInit = ParseBasic(tokens, i + endingOffset + 1); // Parse the variable initializer
                        node.Data = ParseData::VariableDeclaration::Create(
                            isConst,
                            token.Value,
                            tokens[i + 1].Value,
                            arrayCount,
                            parsedInit
                        );
                        node.End = parsedInit.End + 1;

                        // TODO: paren resolving
                        if (tokens[parsedInit.End + 1].Value != ";") throw std::exception("Missing ; after variable initialization");
                        
                        // Return right away
                        return node;
                    }
                    else if (tokens[i + endingOffset].Value == ";")
                    {
                        node.Data = ParseData::VariableDeclaration::Create(
                            isConst,
                            token.Value,
                            tokens[i + 1].Value,
                            arrayCount,
                            ParseNode() // No initializer
                        );
                        node.End = i + endingOffset;

                        // Return right away
                        return node;
                    }
                    else
                        throw std::exception("Unexpected token following variable declaration");
                }
                else
                    throw std::exception("Unexpected type token");
            }
            else if (IsAssignmentOperator(token.Value)) // Check for assignment statements
            {
                if (node.Type != NodeType::None) throw std::exception("Unexpected assignment operator");
                node.Type = NodeType::AssignmentExpression;

                assignmentOperator = token.Value;
            }
            else
            {
                if (leftNode.Type != NodeType::None)
                {
                    if (node.Type != NodeType::AssignmentExpression) throw std::exception("Unexpected expression");

                    // Parse right node
                    ParseNode rightNode = ParseBasic(tokens, i);
                    
                    node.Data = ParseData::AssignmentExpression::Create(
                        assignmentOperator,
                        leftNode,
                        rightNode
                    );
                    node.End = rightNode.End;

                    // Return right away
                    return node;
                }
                else
                {
                    leftNode = ParseBasic(tokens, i);
                    i = leftNode.End + 1;
                }
            }
        }

        return node;
    }

    ParseNode Parser::ParseBlock(const std::vector<Token>& tokens, u32 offset)
    {
        std::vector<ParseNode> nodes;

        std::vector<Token>::const_iterator scopeLoc;
        size_t blockEnd = 0;
        u32 blockStart = offset;
        if (tokens[offset].Value == "{") // If our first character is a brace, then we know our scope is limited to the closing brace
        {
            auto braceLoc = std::find_if(tokens.begin() + offset, tokens.end(), [](const Token& val){ return val.Value == "}"; });
            if (braceLoc == tokens.end()) throw std::exception("Missing }");
            blockEnd = braceLoc - tokens.begin() + offset;
            offset++;
        }
        else
            blockEnd = tokens.size();

        ParseNode statement = ParseStatement(tokens, offset);
        while (statement.Type != NodeType::None)
        {
            offset = statement.End + 1;
            nodes.push_back(statement);
            statement = ParseStatement(tokens, offset);
        }

        ParseNode blockNode = {
            NodeType::BlockStatement,
            ParseData::BlockStatement::Create(
                nodes
            ),
            blockStart, nodes.empty() ? offset : nodes.back().End
        };

        return blockNode;
    }

    bool Parser::IsBasicOperator(const std::string& value)
    {
        return (
            value == "+"  ||
            value == "-"  ||
            value == "*"  ||
            value == "/"  ||
            value == "%"  ||
            value == "^"  ||
            value == "&"  ||
            value == "&&" ||
            value == "|"  ||
            value == "||" ||
            value == "<"  ||
            value == "<<" ||
            value == ">"  ||
            value == ">>"
        );
    }

    bool Parser::IsAssignmentOperator(const std::string& value)
    {
        return (
            value == "="  ||
            value == "+=" ||
            value == "-=" ||
            value == "/=" ||
            value == "*=" ||
            value == "&=" ||
            value == "|=" ||
            value == "%=" ||
            value == "^="
        );
    }
}
