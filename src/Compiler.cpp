#include "pch.h"
#include "Compiler.h"

namespace HSL
{
    std::string Compiler::Compile(const ParseNode& rootNode)
    {
        auto result = ParseNodeData(rootNode);
        m_TabContext = 0;
        return result;
    }

    std::string Compiler::ParseNodeData(const ParseNode& node)
    {
        switch (node.Type)
        {
            default: { throw std::exception("Cannot compile specified nodetype"); }
            case NodeType::BlockStatement: { return ParseBlock(node); }
            case NodeType::Literal: { return ParseLiteral(node); }
            case NodeType::Identifier: { return ParseIdentifier(node); }
            case NodeType::BinaryExpression: { return ParseBinaryExpression(node); }
            case NodeType::MemberExpression: { return ParseMemberExpression(node); }
            case NodeType::ParenExpression: { return ParseParenExpression(node); }
            case NodeType::AssignmentExpression: { return ParseAssignmentExpression(node); }
            case NodeType::UpdateExpression: { return ParseUpdateExpression(node); }
            case NodeType::CallExpression: { return ParseCallExpression(node); }
            case NodeType::CastExpression: { return ParseCastExpression(node); }
            case NodeType::ListExpression: { return ParseListExpression(node); }
            case NodeType::VariableDeclaration: { return ParseVariableDeclaration(node); }
            case NodeType::FunctionDeclaration: { return ParseFunctionDeclaration(node); }
            case NodeType::ForStatement: { return ParseForStatement(node); }
            case NodeType::IfStatement: { return ParseIfStatement(node); }
            case NodeType::WhileStatement: { return ParseWhileStatement(node); }
            case NodeType::ReturnStatement: { return ParseReturnStatement(node); }
            case NodeType::StructDeclaration: { return ParseStructDeclaration(node); }
            case NodeType::None: { return ""; }
        }

        return "";
    }

    std::string Compiler::ParseBlock(const ParseNode& node)
    {
        auto data = (HSL::ParseData::BlockStatement*)node.Data.get();

        std::string initialTabString = std::string(m_TabContext * m_TabSize, ' '); // Each tab is four spaces
        std::string finalString = initialTabString;

        if (data->Scoped)
        {
            m_TabContext++;
            finalString += "{\n";
        }

        std::string bodyTabString = data->Scoped ? initialTabString + std::string(m_TabSize, ' ') : initialTabString;
        for (auto node : data->Body)
        {
            finalString += bodyTabString;
            finalString += ParseNodeData(node);
            finalString += ";\n";
        }

        if (data->Scoped)
        {
            finalString += initialTabString;
            finalString += "}";
            m_TabContext--;
        }

        return finalString;
    }

    std::string Compiler::ParseLiteral(const ParseNode& node)
    {
        auto data = (HSL::ParseData::Literal*)node.Data.get();

        return data->Value;
    }

    std::string Compiler::ParseIdentifier(const ParseNode& node)
    {
        auto data = (HSL::ParseData::Identifier*)node.Data.get();

        return data->Name;
    }

    std::string Compiler::ParseBinaryExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::BinaryExpression*)node.Data.get();

        std::string left = ParseNodeData(data->Left);
        std::string right = ParseNodeData(data->Right);

        if (data->Operator == "[")
            return left + "[" + right + "]";
        return left + " " + data->Operator + " " + right;
    }

    std::string Compiler::ParseMemberExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::MemberExpression*)node.Data.get();

        std::string obj = ParseNodeData(data->Object);
        std::string prop = ParseNodeData(data->Property);

        return obj + "." + prop;
    }

    std::string Compiler::ParseParenExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::ParenExpression*)node.Data.get();

        std::string inside = ParseNodeData(data->Inside);

        return "(" + inside + ")";
    }

    std::string Compiler::ParseAssignmentExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::AssignmentExpression*)node.Data.get();

        std::string left = ParseNodeData(data->Left);
        std::string right = ParseNodeData(data->Right);

        return left + " " + data->Operator + " " + right;
    }

    std::string Compiler::ParseUpdateExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::UpdateExpression*)node.Data.get();

        std::string target = ParseNodeData(data->Target);

        if (data->Prefix)
            return data->Operator + target;
        return target + data->Operator;
    }

    std::string Compiler::ParseCallExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::CallExpression*)node.Data.get();

        std::string signature = ParseNodeData(data->Left);
        signature += "(";

        for (size_t i = 0; i < data->Args.size(); i++)
        {
            signature += ParseNodeData(data->Args[i]);
            if (i != data->Args.size() - 1)
                signature += ", ";
        }

        signature += ')';

        return signature;
    }

    std::string Compiler::ParseCastExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::CastExpression*)node.Data.get();

        std::string signature = ParseType(data->Type, false, false);
        signature += '(';

        for (size_t i = 0; i < data->Args.size(); i++)
        {
            signature += ParseNodeData(data->Args[i]);
            if (i != data->Args.size() - 1)
                signature += ", ";
        }

        signature += ')';

        return signature;
    }

    std::string Compiler::ParseListExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::ListExpression*)node.Data.get();

        std::string elems = "{";
        for (size_t i = 0; i < data->Elements.size(); i++)
        {
            elems += ParseNodeData(data->Elements[i]);
            if (i != data->Elements.size() - 1)
                elems += ", ";
        }

        elems +=  '}';

        return elems;
    }

    std::string Compiler::ParseVariableDeclaration(const ParseNode& node)
    {
        auto data = (HSL::ParseData::VariableDeclaration*)node.Data.get();

        std::string declaration = data->Const ? "const " : "";
        declaration += ParseType(data->Type, data->Const, data->Uniform);
        declaration += " ";
        declaration += data->Name;

        if (data->ArrayCount > 0)
        {
            declaration += "[";
            declaration += std::to_string(data->ArrayCount);
            declaration += "]";
        }
        
        if (data->Init.Type != NodeType::None)
        {
            std::string init = ParseNodeData(data->Init);
            declaration += " = ";
            declaration += init;
        }

        return declaration;
    }

    std::string Compiler::ParseFunctionDeclaration(const ParseNode& node)
    {
        auto data = (HSL::ParseData::FunctionDeclaration*)node.Data.get();

        std::string declaration = data->Const ? "const " : "";
        declaration += data->ReturnType;
        declaration += " ";
        declaration += data->Name;
        declaration += '(';

        for (size_t i = 0; i < data->Params.size(); i++)
        {
            declaration += data->Params[i].Type;
            declaration += " ";
            declaration += data->Params[i].Name;
            if (i != data->Params.size() - 1)
                declaration += ", ";
        }
        
        declaration += ")\n";

        std::string body = ParseNodeData(data->Body);
        declaration += body;

        return declaration;
    }

    std::string Compiler::ParseForStatement(const ParseNode& node)
    {
        auto data = (HSL::ParseData::ForStatement*)node.Data.get();

        std::string signature = "for (";
        signature += ParseNodeData(data->Init);
        signature += "; ";
        signature += ParseNodeData(data->Test);
        signature += "; ";
        signature += ParseNodeData(data->Update);
        signature += ")\n";

        signature += ParseNodeData(data->Body);

        return signature;
    }

    std::string Compiler::ParseIfStatement(const ParseNode& node)
    {
        auto data = (HSL::ParseData::IfStatement*)node.Data.get();

        std::string condition = ParseNodeData(data->Condition);
        std::string body = ParseNodeData(data->Body);

        return "if (" + condition + ")\n" + body;
    }

    std::string Compiler::ParseWhileStatement(const ParseNode& node)
    {
        auto data = (HSL::ParseData::WhileStatement*)node.Data.get();

        return "";
    }

    std::string Compiler::ParseReturnStatement(const ParseNode& node)
    {
        auto data = (HSL::ParseData::ReturnStatement*)node.Data.get();

        std::string value = ParseNodeData(data->Value);

        return "return " + value;
    }

    std::string Compiler::ParseStructDeclaration(const ParseNode& node)
    {
        auto data = (HSL::ParseData::StructDeclaration*)node.Data.get();

        std::string body = ParseNodeData(data->Body);

        return "struct " + data->Name + "\n" + body;
    }

    std::string Compiler::ParseType(const std::string& type, bool isConst, bool isUniform)
    {
        if (type._Starts_with("vec"))
        {
            // Numerical component of the vec
            std::string componentString = "";
            if (std::isdigit(type.back()))
                componentString += type.back();

            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL: return "vec" + componentString;
                case CompileTarget::VulkanGLSL: return "vec" + componentString;
                case CompileTarget::HLSL: return "float" + componentString;
                //case CompileTarget::Metal:
            }
        }
        else if (type._Starts_with("bvec"))
        {
            // Numerical component of the bvec
            std::string componentString = "";
            if (std::isdigit(type.back()))
                componentString += type.back();

            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL: return "bvec" + componentString;
                case CompileTarget::VulkanGLSL: return "bvec" + componentString;
                case CompileTarget::HLSL: return "bool" + componentString;
                //case CompileTarget::Metal:
            }
        }
        else if (type._Starts_with("ivec"))
        {
            // Numerical component of the vec
            std::string componentString = "";
            if (std::isdigit(type.back()))
                componentString += type.back();

            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL: return "ivec" + componentString;
                case CompileTarget::VulkanGLSL: return "ivec" + componentString;
                case CompileTarget::HLSL: return "int" + componentString;
                //case CompileTarget::Metal:
            }
        }
        else if (type._Starts_with("uvec"))
        {
            // Numerical component of the vec
            std::string componentString = "";
            if (std::isdigit(type.back()))
                componentString += type.back();

            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL: return "uvec" + componentString;
                case CompileTarget::VulkanGLSL: return "uvec" + componentString;
                case CompileTarget::HLSL: return "uint" + componentString;
                //case CompileTarget::Metal:
            }
        }
        else if (type._Starts_with("dvec"))
        {
            // Numerical component of the vec
            std::string componentString = "";
            if (std::isdigit(type.back()))
                componentString += type.back();

            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL: return "dvec" + componentString;
                case CompileTarget::VulkanGLSL: return "dvec" + componentString;
                case CompileTarget::HLSL: return "double" + componentString;
                //case CompileTarget::Metal:
            }
        }
        else if (type._Starts_with("mat"))
        {
            // Numerical component of the mat
            if (!std::isdigit(type.back())) throw std::exception("Invalid matrix type");
            
            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL: return std::string("mat") + type.back();
                case CompileTarget::VulkanGLSL: return std::string("mat") + type.back();
                case CompileTarget::HLSL: return std::string("float") + type.back() + "x" + type.back();
                //case CompileTarget::Metal:
            }
        }
        else if (type == "tex2d")
        {
            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL: return "uniform sampler2D";
                case CompileTarget::VulkanGLSL: return "uniform sampler2D";
                case CompileTarget::HLSL: return "Texture2D";
                //case CompileTarget::Metal:
            }
        }
        else if (type == "texCube")
        {
            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL: return "uniform samplerCube";
                case CompileTarget::VulkanGLSL: return "uniform samplerCube";
                //case CompileTarget::HLSL: return "Texture2D";
                //case CompileTarget::Metal:
            }
        }
        else if (type == "buffer")
        {
            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL:
                case CompileTarget::VulkanGLSL:
                {
                    std::string finalString = isConst ? "readonly " : "";

                    finalString += isUniform ? "uniform" : "buffer";

                    return finalString;
                } break;
                case CompileTarget::HLSL:
                {
                    return isUniform ? "ConstantBuffer" : "StructuredBuffer";
                } break;
                //case CompileTarget::Metal:
            }
        }
        else if (type == "bool")
            return "bool";
        else if (type == "int")
            return "int";
        else if (type == "uint")
            return "uint";
        else if (type == "float")
            return "float";
        else if (type == "double")
            return "double";
        else if (type == "void")
            return "void";

        throw std::exception("Unsupported type");

        return "";
    }
}
