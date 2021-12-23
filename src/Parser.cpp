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
        node.Start = offset;
        node.End = offset;

        int tokenCount = tokens.size() - offset;
        if (tokenCount <= 0) throw std::exception("Missing expression");

        if (tokens[offset].Value == ";") return node;

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
                node.End = property.End;
            }
            else if (tokenCount >= 2 && tokens[offset + 1].Value == "(") // Function call
            {
                auto parsedFunc = ParseList(tokens, offset + 2, ")", ",");

                // Left is the current identifier
                ParseNode callNode = {
                    NodeType::CallExpression,
                    ParseData::CallExpression::Create(
                        identifierNode,
                        parsedFunc.Elements
                    ),
                    offset, parsedFunc.LastToken
                };

                if (IsBasicOperator(tokens[parsedFunc.LastToken + 1].Value)) // Check for following operators
                {
                    ParseNode right = ParseBasic(tokens, parsedFunc.LastToken + 2);

                    // Left is the function call
                    node.Type = NodeType::BinaryExpression;
                    node.Data = ParseData::BinaryExpression::Create(
                        tokens[parsedFunc.LastToken + 1].Value,
                        callNode,
                        right
                    );
                    
                    // Account for the closing bracket if necessary
                    // TODO: better bracket operator handling
                    node.End = tokens[offset + 1].Value == "[" ? right.End + 1 : right.End;
                }
                else if (tokens[parsedFunc.LastToken + 1].Value == ".") // Check for following member expressions
                {
                    ParseNode prop = ParseBasic(tokens, parsedFunc.LastToken + 2);

                    // Obj is the function call
                    node.Type = NodeType::MemberExpression;
                    node.Data = ParseData::MemberExpression::Create(
                        callNode,
                        prop
                    );
                    node.End = prop.End;
                }
                else // Otherwise it is just the function call
                    node = callNode;
            }
            else if (tokenCount >= 2 && IsBasicOperator(tokens[offset + 1].Value)) // Basic expression
            {
                ParseNode right = ParseBasic(tokens, offset + 2);

                // Left is the current identifier
                ParseNode expressionNode = {
                    NodeType::BinaryExpression,
                    ParseData::BinaryExpression::Create(
                        tokens[offset + 1].Value,
                        identifierNode,
                        right
                    ),
                    offset, right.End
                };
                
                if (tokens[offset + 1].Value == "[") // If we are using the bracket operator, check for anything following
                {
                    right.End += 1; // account for closing bracket
                    if (tokens[right.End + 1].Value == ".") // Member expression
                    {
                        ParseNode prop = ParseBasic(tokens, right.End + 2);

                        // Obj is the expression
                        node.Type = NodeType::MemberExpression;
                        node.Data = ParseData::MemberExpression::Create(
                            expressionNode,
                            prop
                        );
                        node.End = prop.End;
                    }
                    else if (IsBasicOperator(tokens[right.End + 1].Value)) // Operators
                    {
                        ParseNode newRight = ParseBasic(tokens, right.End + 2);

                        // Left is the expression
                        node.Type = NodeType::BinaryExpression;
                        node.Data = ParseData::BinaryExpression::Create(
                            tokens[right.End + 1].Value,
                            expressionNode,
                            newRight
                        );
                        node.End = newRight.End;
                    }
                    else
                    {
                        node = expressionNode;
                        node.End += 1; // Account for closing bracket
                    }
                }
                else
                    node = expressionNode; // Otherwise the node is just the expression
            }
            else if (tokenCount >= 2 && IsUpdateOperator(tokens[offset + 1].Value)) // Update operators
            {
                // Target is the current identifier
                node.Type = NodeType::UpdateExpression;
                node.Data = ParseData::UpdateExpression::Create(
                    tokens[offset + 1].Value,
                    false,
                    identifierNode
                );
                node.End = offset + 1;
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
                if (tokens[offset + 1].Value == "[") throw std::exception("Unexpected [ following literal");

                ParseNode right = ParseBasic(tokens, offset + 2);

                // Left is the current literal
                node.Type = NodeType::BinaryExpression;
                node.Data = ParseData::BinaryExpression::Create(
                    tokens[offset + 1].Value,
                    literalNode,
                    right
                );

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
                auto args = ParseList(tokens, offset + 2, ")", ",");

                ParseNode castNode = {
                    NodeType::CastExpression,
                    ParseData::CastExpression::Create(
                        tokens[offset].Value,
                        args.Elements
                    ),
                    offset, args.LastToken
                };

                // Check for following operations
                if (tokens[args.LastToken + 1].Value == ".") // Member expression
                {
                    ParseNode prop = ParseBasic(tokens, args.LastToken + 2);

                    // Obj is the cast
                    node.Type = NodeType::MemberExpression;
                    node.Data = ParseData::MemberExpression::Create(
                        castNode,
                        prop
                    );
                    node.End = prop.End;
                }
                else if (IsBasicOperator(tokens[args.LastToken + 1].Value)) // Operators
                {
                    ParseNode newRight = ParseBasic(tokens, args.LastToken + 2);

                    // Left is the cast
                    node.Type = NodeType::BinaryExpression;
                    node.Data = ParseData::BinaryExpression::Create(
                        tokens[args.LastToken + 1].Value,
                        castNode,
                        newRight
                    );
                    node.End = newRight.End;
                }
                else
                    node = castNode;
            }
            else // Invalid
                throw std::exception("Unexpected type token");
        }
        else if (IsUpdateOperator(tokens[offset].Value)) // Prefix update operators
        {
            ParseNode target = ParseBasic(tokens, offset + 1);

            node.Type = NodeType::UpdateExpression;
            node.Data = ParseData::UpdateExpression::Create(
                tokens[offset].Value,
                true,
                target
            );
            node.End = offset + 1;
        }
        else if (tokens[offset].Value == "{") // Initializer list
        {
            auto parsedList = ParseList(tokens, offset + 1, "}", ",");

            node.Type = NodeType::ListExpression;
            node.Data = ParseData::ListExpression::Create(
                parsedList.Elements
            );
            node.End = parsedList.LastToken;
        }
        else if (tokens[offset].Value == "(") // Parenthesis
        {
            auto parsedExpression = ParseBasic(tokens, offset + 1);
            parsedExpression.End++; // account for closing parenthesis

            ParseNode parenNode = {
                NodeType::ParenExpression,
                ParseData::ParenExpression::Create(
                    parsedExpression
                ),
                offset, parsedExpression.End
            };

            if (tokens[parsedExpression.End + 1].Value == ".") // Member expression
            {
                ParseNode prop = ParseBasic(tokens, parsedExpression.End + 2);

                // Obj is the expression
                node.Type = NodeType::MemberExpression;
                node.Data = ParseData::MemberExpression::Create(
                    parenNode,
                    prop
                );
                node.End = prop.End;
            }
            else if (IsBasicOperator(tokens[parsedExpression.End + 1].Value)) // Operators
            {
                ParseNode newRight = ParseBasic(tokens, parsedExpression.End + 2);

                // Left is the expression
                node.Type = NodeType::BinaryExpression;
                node.Data = ParseData::BinaryExpression::Create(
                    tokens[parsedExpression.End + 1].Value,
                    parenNode,
                    newRight
                );
                node.End = newRight.End;
            }
            else
                node = parenNode;
        }
        else if (tokens[offset].Value == "-" || tokens[offset].Value == "!") // Negation
        {
            ParseNode target = ParseBasic(tokens, offset + 1);

            node.Type = NodeType::UpdateExpression;
            node.Data = ParseData::UpdateExpression::Create(
                tokens[offset].Value,
                true,
                target
            );
            node.End = target.End;
        }
        else
            throw std::exception("Invalid syntax");

        return node;
    }

    Parser::ParseListReturn Parser::ParseList(const std::vector<Token>& tokens, u32 offset, const std::string& endChar, const std::string& delim)
    {
        ParseListReturn returnVal;

        // Find the location of the endChar which indicates the end of the list
        auto endLoc = std::find_if(tokens.begin() + offset, tokens.end(), [endChar](const Token& val){ return val.Value == endChar; });
        if (endLoc == tokens.end()) throw std::exception(("Expecting " + endChar).c_str());
        size_t statementEnd = endLoc - tokens.begin();

        size_t i = offset;
        for (i; i < statementEnd; i += 2)
        {
            if (tokens[i].Value == endChar) break;

            // Parse the expression
            ParseNode parsed = ParseBasic(tokens, i);
            returnVal.Elements.push_back(parsed);

            // Search for an end char inside the parsed statement. If found, then we need to update our statement end so that it is no longer
            // pointing to the end char inside the parsed statement
            bool foundChar = false;
            for (size_t j = i; j <= parsed.End; j++)
            {
                if (tokens[j].Value == endChar)
                {
                    foundChar = true;
                    break;
                }
            }
            if (foundChar)
            {
                auto endLoc = std::find_if(tokens.begin() + parsed.End + 1, tokens.end(), [endChar](const Token& val){ return val.Value == endChar; });
                if (endLoc == tokens.end()) throw std::exception(("Expecting " + endChar).c_str());
                statementEnd = endLoc - tokens.begin();
            }

            i = parsed.End;
        }

        if (i == offset)
            returnVal.LastToken = i;
        else
            returnVal.LastToken = i - 1;

        return returnVal;
    }

    ParseNode Parser::ParseForLoop(const std::vector<Token>& tokens, u32 offset)
    {
        // Find the location of the ) token which indicates the end of the loop header
        auto endLoc = std::find_if(tokens.begin() + offset, tokens.end(), [](const Token& val){ return val.Value == ")"; });
        if (endLoc == tokens.end()) throw std::exception("Expecting )");
        size_t statementEnd = endLoc - tokens.begin();

        ParseNode Init = ParseStatement(tokens, offset);
        offset = Init.End + 1;

        ParseNode Test = ParseBasic(tokens, offset);
        offset = Test.End + 2; 

        ParseNode Update = ParseStatement(tokens, offset);
        offset = Update.End + 1; // Account for semicolon & parenthesis

        if (tokens[offset + 1].Value != "{") throw std::exception("Expected { after for loop");

        ParseNode Body = ParseBlock(tokens, offset + 1);

        ParseNode forNode = {
            NodeType::ForStatement,
            ParseData::ForStatement::Create(
                Init,
                Test,
                Update,
                Body
            ),
            offset, Body.End
        };

        return forNode;
    }

    Parser::ParseFunctionDeclarationReturn Parser::ParseFunctionDeclaration(const std::vector<Token>& tokens, u32 offset)
    {
        ParseFunctionDeclarationReturn returnVal;

        // Find the location of the ) token which indicates the end of the call signature
        auto endLoc = std::find_if(tokens.begin() + offset, tokens.end(), [](const Token& val){ return val.Value == ")"; });
        if (endLoc == tokens.end()) throw std::exception("Expecting )");
        size_t statementEnd = endLoc - tokens.begin();

        size_t i = offset;
        for (i; i < statementEnd; i += 3)
        {
            if (tokens[i].Value == ")") break;

            // Parse the expression
            if (i + 1 >= tokens.size()) throw std::exception("Unexpected end of function declaration");
            if (tokens[i].Type != TokenType::Type) throw std::exception("Expected a parameter type");
            if (tokens[i + 1].Type != TokenType::Identifier) throw std::exception("Expected an identifier parameter name");
            returnVal.Params.push_back({
                tokens[i].Value,
                tokens[i + 1].Value
            });
        }

        if (i == offset)
            returnVal.LastToken = i;
        else
            returnVal.LastToken = i - 1;

        return returnVal;
    }

    ParseNode Parser::ParseVariableDeclaration(const std::vector<Token>& tokens, u32 offset, bool isConst)
    {
        ParseNode node;
        node.Type = NodeType::VariableDeclaration;
        node.Start = offset;
        node.End = offset;

        int arrayCount = 0;
        u32 endingOffset = 2;

        if (tokens[offset + 2].Value == "[") // Array variable
        {
            try
            { arrayCount = stoi(tokens[offset + 3].Value); }
            catch (std::exception e)
            { throw std::exception("Expected literal in variable array declaration"); }

            if (tokens[offset + 4].Value != "]")
                throw std::exception("Expected ] after variable array declaration");

            endingOffset = 5;
        }

        if (tokens[offset + endingOffset].Value == "=")
        {
            ParseNode parsedInit = ParseBasic(tokens, offset + endingOffset + 1); // Parse the variable initializer
            node.Data = ParseData::VariableDeclaration::Create(
                isConst,
                tokens[offset].Value,
                tokens[offset + 1].Value,
                arrayCount,
                parsedInit
            );
            node.End = parsedInit.End + 1; // Account for semicolon

            if (tokens[parsedInit.End + 1].Value != ";") throw std::exception("Missing ; after variable initialization");
            
            // Return right away
            return node;
        }
        else if (tokens[offset + endingOffset].Value == ";")
        {
            node.Data = ParseData::VariableDeclaration::Create(
                isConst,
                tokens[offset].Value,
                tokens[offset + 1].Value,
                arrayCount,
                ParseNode() // No initializer
            );
            node.End = offset + endingOffset;

            // Return right away
            return node;
        }
        else
            throw std::exception("Unexpected token following variable declaration");

        return node;
    }

    ParseNode Parser::ParseStatement(const std::vector<Token>& tokens, u32 offset)
    {
        ParseNode node; // Once our node type is set, we have determined what type of statement this is, and an error is thrown if it is set again
        node.Start = offset;
        node.End = offset;

        if (static_cast<int>(tokens.size()) - offset <= 0) return node;

        // Find the next semicolon because it will likely be the scope of this node
        auto semiLoc = std::find_if(tokens.begin() + offset, tokens.end(), [](const Token& val){ return val.Value == ";"; });
        size_t statementEnd = semiLoc - tokens.begin();

        bool isConst = false;
        bool isUniform = false;
        ParseNode leftNode;
        std::string assignmentOperator;
        for (size_t i = offset; i < statementEnd; i++)
        {
            auto& token = tokens[i];

            if (token.Type == TokenType::Keyword) // Check for special start of statement keywords
            {
                if (node.Type != NodeType::None) throw std::exception("Unexpected keyword");
                    
                if (token.Value == "const") isConst = true;
                if (token.Value == "uniform") isUniform = true;
                else if (token.Value == "for")
                {
                    ParseNode parsedLoop = ParseForLoop(tokens, i + 2);

                    // Return right away
                    return parsedLoop;
                }
                else if (token.Value == "if")
                {
                    ParseNode condition = ParseBasic(tokens, i + 2);
                    
                    if (tokens[condition.End + 2].Value != "{") throw std::exception("Expected { after if statement");

                    ParseNode body = ParseBlock(tokens, condition.End + 2);

                    node.Type = NodeType::IfStatement;
                    node.Data = ParseData::IfStatement::Create(
                        condition,
                        body
                    );
                    node.End = body.End;

                    return node;
                }
                else if (token.Value == "while")
                {
                    
                }
                else if (token.Value == "return")
                {
                    ParseNode parsedValue = ParseBasic(tokens, i + 1);

                    node.Type = NodeType::ReturnStatement;
                    node.Data = ParseData::ReturnStatement::Create(
                        parsedValue
                    );
                    node.End = parsedValue.End + 1; // account for semicolon

                    // Return right away
                    return node;
                }
                else if (token.Value == "struct")
                {
                    if (tokens[i + 2].Value != "{") throw std::exception("Expected { after struct declaration");

                    ParseNode body = ParseBlock(tokens, i + 2);

                    node.Type = NodeType::StructDeclaration;
                    node.Data = ParseData::StructDeclaration::Create(
                        tokens[i + 1].Value,
                        body
                    );
                    node.End = body.End + 1; // account for semicolon

                    // Return right away
                    return node;
                }
            }
            else if (token.Type == TokenType::Type) // Some sort of declaration
            {
                if (i + 2 < statementEnd && tokens[i + 2].Value == "(") // Function declaration
                {
                    if (node.Type != NodeType::None) throw std::exception("Unexpected function declaration");
                    node.Type = NodeType::FunctionDeclaration;

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

                    node = ParseVariableDeclaration(tokens, i, isConst);

                    return node;
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
                    node.End = rightNode.End + 1; // account for semicolon

                    // Return right away
                    return node;
                }
                else
                {
                    leftNode = ParseBasic(tokens, i);
                    i = leftNode.End;
                }
            }
        }

        // If nothing else happens then we just return the left node and account for semicolon
        leftNode.End++;
        return leftNode;
    }

    ParseNode Parser::ParseBlock(const std::vector<Token>& tokens, u32 offset)
    {
        std::vector<ParseNode> nodes;

        std::vector<Token>::const_iterator scopeLoc;
        u32 blockStart = offset;
        bool scoped = false;
        if (tokens[offset].Value == "{") // If our first character is a brace, then we know our scope is limited to the closing brace
        {
            auto braceLoc = std::find_if(tokens.begin() + offset, tokens.end(), [](const Token& val){ return val.Value == "}"; });
            if (braceLoc == tokens.end()) throw std::exception("Missing }");
            offset++;
            scoped = true;
        }

        ParseNode statement = ParseStatement(tokens, offset);
        while (statement.Type != NodeType::None)
        {
            offset = statement.End + 1;
            nodes.push_back(statement);

            if (offset < tokens.size() && tokens[offset].Value == "}") break;
            statement = ParseStatement(tokens, offset);
        }

        ParseNode blockNode = {
            NodeType::BlockStatement,
            ParseData::BlockStatement::Create(
                scoped,
                nodes
            ),
            blockStart, nodes.empty() ? offset + 1 : nodes.back().End + 1
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
            value == "==" ||
            value == "<=" ||
            value == ">=" ||
            value == "|"  ||
            value == "||" ||
            value == "<"  ||
            value == "<<" ||
            value == ">"  ||
            value == ">>" ||
            value == "["
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

    bool Parser::IsUpdateOperator(const std::string& value)
    {
        return (
            value == "++"  ||
            value == "--"
        );
    }
}
