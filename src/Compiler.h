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

        std::string Compile(const ParseNode& rootNode, std::string includeBase);

        inline void SetCompileTarget(CompileTarget target) { m_Target = target; }

    public:
        static std::string CompileFromFile(const std::string& path, CompileTarget target);

    private:
        struct Scope
        {
            std::vector<std::string> Variables;
            std::vector<std::string> Functions;
            std::vector<std::string> Structs;
            std::vector<std::string> Buffers;

            inline bool ContainsVariable(const std::string& identifier)
            {
                return std::find(Variables.begin(), Variables.end(), identifier) != Variables.end();
            }
            inline bool ContainsFunction(const std::string& identifier)
            {
                return std::find(Functions.begin(), Functions.end(), identifier) != Functions.end();
            }
            inline bool ContainsStruct(const std::string& identifier)
            {
                return std::find(Structs.begin(), Structs.end(), identifier) != Structs.end();
            }
            inline bool ContainsBuffer(const std::string& identifier)
            {
                return std::find(Buffers.begin(), Buffers.end(), identifier) != Buffers.end();
            }
        };
        struct CompileState
        {
            u32 TabContext = 0;
            u32 BufferCount = 0;
            u32 InVariableContext = 0;
            u32 OutVariableContext = 0;
            std::filesystem::path IncludeBase;
            std::vector<Scope> ScopeStack;
        };

    private:
        std::string Compile(const ParseNode& rootNode, std::string includeBase, std::shared_ptr<CompileState> inheritedState);
        static std::string CompileFromFile(const std::string& path, CompileTarget target, std::shared_ptr<CompileState> inheritedState);

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
        std::string ParsePreprocessorExpression(const ParseNode& node);
        std::string ParseVariableDeclaration(const ParseNode& node);
        std::string ParseFunctionDeclaration(const ParseNode& node);
        std::string ParseForStatement(const ParseNode& node);
        std::string ParseIfStatement(const ParseNode& node);
        std::string ParseElseStatement(const ParseNode& node);
        std::string ParseElseIfStatement(const ParseNode& node);
        std::string ParseWhileStatement(const ParseNode& node);
        std::string ParseReturnStatement(const ParseNode& node);
        std::string ParseStructDeclaration(const ParseNode& node);

        std::string GeneratePreDefinitions();
        std::string ParseType(const std::string& type, bool isUniform);

        inline bool IsVariableDefined(const std::string& identifier)
        {
            for (auto& scope : m_CompileState->ScopeStack)
                if (scope.ContainsVariable(identifier)) return true;
            return false;
        }
        inline bool IsFunctionDefined(const std::string& identifier)
        {
            for (auto& scope : m_CompileState->ScopeStack)
                if (scope.ContainsFunction(identifier)) return true;
            return false;
        }
        inline bool IsStructDefined(const std::string& identifier)
        {
            for (auto& scope : m_CompileState->ScopeStack)
                if (scope.ContainsStruct(identifier)) return true;
            return false;
        }
        inline bool IsBufferDefined(const std::string& identifier)
        {   
            // Buffers can only be defined on the outermost scope
            return m_CompileState->ScopeStack.front().ContainsBuffer(identifier);
        }

        bool IsSpecialType(const std::string& type);

    private:
        const u32 m_TabSize = 4;

        CompileTarget m_Target;
        std::shared_ptr<CompileState> m_CompileState;
    };
};