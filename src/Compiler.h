#pragma once

#include "Parser.h"

namespace HSL
{
    enum class CompileTarget
    {
        None = 0,
        OpenGLSL, VulkanGLSL, HLSL, Metal
    };

    class Compiler
    {
    public:
        Compiler(CompileTarget target)
            : m_Target(target)
        {}

        std::string Compile(const ParseNode& rootNode);

        inline void SetCompileTarget(CompileTarget target) { m_Target = target; }

    private:
        std::string ParseNodeData(const ParseNode& node);
        std::string ParseBlock(const ParseNode& node);
        std::string ParseLiteral(const ParseNode& node);
        std::string ParseIdentifier(const ParseNode& node);
        std::string ParseBinaryExpression(const ParseNode& node);
        std::string ParseMemberExpression(const ParseNode& node);
        std::string ParseParenExpression(const ParseNode& node);
        std::string ParseAssignmentExpression(const ParseNode& node);
        std::string ParseUpdateExpression(const ParseNode& node);
        std::string ParseCallExpression(const ParseNode& node);
        std::string ParseCastExpression(const ParseNode& node);
        std::string ParseListExpression(const ParseNode& node);
        std::string ParseVariableDeclaration(const ParseNode& node);
        std::string ParseFunctionDeclaration(const ParseNode& node);
        std::string ParseForStatement(const ParseNode& node);
        std::string ParseIfStatement(const ParseNode& node);
        std::string ParseWhileStatement(const ParseNode& node);
        std::string ParseReturnStatement(const ParseNode& node);
        std::string ParseStructDeclaration(const ParseNode& node);

        std::string ParseType(const std::string& type, bool isConst, bool isUniform);

    private:
        const u32 m_TabSize = 4;

        CompileTarget m_Target;
        u32 m_TabContext = 0;
    };
};