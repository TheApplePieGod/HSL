#pragma once

#include "Lexer.h"

#define PARSE_DATA_CREATE(dataStruct) \
    template<typename ... Args> \
    inline static std::shared_ptr<void> Create(Args&& ... args) \
    { return std::make_shared<dataStruct>(std::forward<Args>(args)...); }

namespace HSL
{
    enum class NodeType
    {
        None = 0,
        BlockStatement,
        Literal,
        Identifier,
        BinaryExpression,
        MemberExpression,
        ParenExpression,
        AssignmentExpression,
        UpdateExpression,
        CallExpression,
        CastExpression,
        ListExpression,
        VariableDeclaration,
        FunctionDeclaration,
        StructDeclaration,
        ForStatement,
        IfStatement,
        WhileStatement,
        ReturnStatement
    };

    struct ParseNode
    {
        NodeType Type = NodeType::None;
        std::shared_ptr<void> Data;

        // Token offsets
        size_t Start;
        size_t End;
    };

    namespace ParseData
    {
        struct BlockStatement
        {
            BlockStatement(bool scoped, const std::vector<ParseNode>& body) : Scoped(scoped), Body(body) {}

            bool Scoped;
            std::vector<ParseNode> Body;

            PARSE_DATA_CREATE(BlockStatement);
        };

        struct Literal
        {
            Literal(const std::string& value) : Value(value) {}

            std::string Value;

            PARSE_DATA_CREATE(Literal);
        };

        struct Identifier
        {
            Identifier(const std::string& name) : Name(name) {}

            std::string Name;

            PARSE_DATA_CREATE(Identifier);
        };
        
        struct BinaryExpression
        {
            BinaryExpression(const std::string& op, const ParseNode& left, const ParseNode& right) : Operator(op), Left(left), Right(right) {}

            std::string Operator;
            ParseNode Left;
            ParseNode Right;
            
            PARSE_DATA_CREATE(BinaryExpression);
        };

        struct MemberExpression
        {
            MemberExpression(const ParseNode& obj, const ParseNode& prop) : Object(obj), Property(prop) {}

            ParseNode Object;
            ParseNode Property;

            PARSE_DATA_CREATE(MemberExpression);
        };

        struct ParenExpression
        {
            ParenExpression(const ParseNode& inside) : Inside(inside) {}

            ParseNode Inside;

            PARSE_DATA_CREATE(ParenExpression);
        };

        struct AssignmentExpression
        {
            AssignmentExpression(const std::string& op, const ParseNode& left, const ParseNode& right) : Operator(op), Left(left), Right(right) {}

            std::string Operator;
            ParseNode Left;
            ParseNode Right;

            PARSE_DATA_CREATE(AssignmentExpression);
        };

        struct UpdateExpression
        {
            UpdateExpression(const std::string& op, bool prefix, const ParseNode& target) : Operator(op), Prefix(prefix), Target(target) {}

            std::string Operator;
            bool Prefix;
            ParseNode Target;

            PARSE_DATA_CREATE(UpdateExpression);
        };

        struct CallExpression
        {
            CallExpression(const ParseNode& left, const std::vector<ParseNode>& args) : Left(left), Args(args) {}

            ParseNode Left;
            std::vector<ParseNode> Args;

            PARSE_DATA_CREATE(CallExpression);
        };

        struct CastExpression
        {
            CastExpression(const std::string& type, const std::vector<ParseNode>& args) : Type(type), Args(args) {}

            std::string Type;
            std::vector<ParseNode> Args;

            PARSE_DATA_CREATE(CastExpression);
        };

        struct ListExpression
        {
            ListExpression(const std::vector<ParseNode>& elems) : Elements(elems) {}

            std::vector<ParseNode> Elements;

            PARSE_DATA_CREATE(ListExpression);
        };

        struct VariableDeclaration
        {
            VariableDeclaration(bool isConst, const std::string& type, const std::string& name, int arrayCount, const ParseNode& init)
                : Const(isConst), Type(type), Name(name), ArrayCount(arrayCount), Init(init) {}

            bool Const;
            bool Uniform;
            std::string Type;
            std::string Name;
            int ArrayCount;
            ParseNode Init;

            PARSE_DATA_CREATE(VariableDeclaration);
        };

        struct FunctionParam
        {
            std::string Type;
            std::string Name;
        };

        struct FunctionDeclaration
        {
            FunctionDeclaration(bool isConst, const std::string& returnType, const std::vector<FunctionParam>& params, const std::string& name, const ParseNode& body)
                : Const(isConst), ReturnType(returnType), Params(params), Name(name), Body(body)
            {}

            bool Const;
            std::string ReturnType;
            std::vector<FunctionParam> Params;
            std::string Name;
            ParseNode Body;

            PARSE_DATA_CREATE(FunctionDeclaration);
        };

        struct StructDeclaration
        {
            StructDeclaration(const std::string& name, const ParseNode& body) : Name(name), Body(body) {}

            std::string Name;
            ParseNode Body;

            PARSE_DATA_CREATE(StructDeclaration);
        };

        struct ForStatement
        {
            ForStatement(const ParseNode& init, const ParseNode& test, const ParseNode& update, const ParseNode& body)
                : Init(init), Test(test), Update(update), Body(body)
            {}

            ParseNode Init;
            ParseNode Test;
            ParseNode Update;
            ParseNode Body;

            PARSE_DATA_CREATE(ForStatement);
        };

        struct IfStatement
        {
            IfStatement(const ParseNode& condition, const ParseNode& body) : Condition(condition), Body(body) {}

            ParseNode Condition;
            ParseNode Body;

            PARSE_DATA_CREATE(IfStatement);
        };

        struct WhileStatement
        {
            WhileStatement(const ParseNode& condition, const ParseNode& body) : Condition(condition), Body(body) {}

            ParseNode Condition;
            ParseNode Body;

            PARSE_DATA_CREATE(WhileStatement);
        };

        struct ReturnStatement
        {
            ReturnStatement(const ParseNode& value) : Value(value) {}

            ParseNode Value;

            PARSE_DATA_CREATE(ReturnStatement);
        };
    }

    class Parser
    {
    public:
        static ParseNode Parse(const std::vector<Token>& tokens);

    private:
        struct ParseListReturn
        {
            std::vector<ParseNode> Elements;
            size_t LastToken;
        };
        struct ParseFunctionDeclarationReturn
        {
            std::vector<ParseData::FunctionParam> Params;
            size_t LastToken;
        };

    private:
        static ParseNode ParseBlock(const std::vector<Token>& tokens, u32 offset);
        static ParseNode ParseStatement(const std::vector<Token>& tokens, u32 offset);
        static ParseNode ParseBasic(const std::vector<Token>& tokens, u32 offset);
        static ParseListReturn ParseList(const std::vector<Token>& tokens, u32 offset, const std::string& endChar, const std::string& delim);
        static ParseFunctionDeclarationReturn ParseFunctionDeclaration(const std::vector<Token>& tokens, u32 offset);
        static ParseNode ParseForLoop(const std::vector<Token>& tokens, u32 offset);
        static ParseNode ParseVariableDeclaration(const std::vector<Token>& tokens, u32 offset, bool isConst);

        static bool IsBasicOperator(const std::string& value);
        static bool IsUpdateOperator(const std::string& value);
        static bool IsAssignmentOperator(const std::string& value);

        inline static bool CheckToken(const Token& token, TokenType type, const std::string& check)
        {
            return token.Type == type && token.Value == check;
        }
    };
};