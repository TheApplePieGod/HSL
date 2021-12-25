#include "pch.h"
#include "Compiler.h"

namespace HSL
{
    std::string Compiler::CompileFromFile(const std::string& path, CompileTarget target)
    {
        return CompileFromFile(path, target, nullptr);
    }

    std::string Compiler::CompileFromFile(const std::string& path, CompileTarget target, std::shared_ptr<CompileState> inheritedState)
    {
        std::ifstream f(path);
        std::stringstream buffer;
        buffer << f.rdbuf();

        auto tokens = HSL::Lexer::Lexify(buffer.str());
        auto parsed = HSL::Parser::Parse(tokens);

        std::string includeBase = std::filesystem::path(path).parent_path().generic_u8string();

        HSL::Compiler comp(target);
        return comp.Compile(parsed, includeBase, inheritedState);
    }

    std::string Compiler::Compile(const ParseNode& rootNode, std::string includeBase)
    {
        return Compile(rootNode, includeBase, nullptr);
    }

    std::string Compiler::Compile(const ParseNode& rootNode, std::string includeBase, std::shared_ptr<CompileState> inheritedState)
    {
        if (inheritedState)
            m_CompileState = inheritedState;
        else
            m_CompileState = std::make_shared<CompileState>();
        m_CompileState->IncludeBase = includeBase;
        auto result = ParseNodeData(rootNode);
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
            case NodeType::PreprocessorExpression: { return ParsePreprocessorExpression(node); }
            case NodeType::VariableDeclaration: { return ParseVariableDeclaration(node); }
            case NodeType::FunctionDeclaration: { return ParseFunctionDeclaration(node); }
            case NodeType::ForStatement: { return ParseForStatement(node); }
            case NodeType::IfStatement: { return ParseIfStatement(node); }
            case NodeType::ElseStatement: { return ParseElseStatement(node); }
            case NodeType::ElseIfStatement: { return ParseElseIfStatement(node); }
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

        std::string initialTabString = std::string(m_CompileState->TabContext * m_TabSize, ' '); // Each tab is four spaces
        std::string finalString = initialTabString;

        if (data->Scoped)
        {
            m_CompileState->TabContext++;
            finalString += "{\n";
        }

        bool scopePushed = false;
        if (data->Scoped || m_CompileState->ScopeStack.empty())
        {
            scopePushed = true;
            m_CompileState->ScopeStack.emplace_back(); // Only push it if the outer scope does not exist yet or we are in a block
        }

        // If we are parsing the global scope, add any required predefined variables/functions
        if (scopePushed && m_CompileState->ScopeStack.size() == 1)
            finalString += GeneratePreDefinitions();

        std::string bodyTabString = data->Scoped ? initialTabString + std::string(m_TabSize, ' ') : initialTabString;
        for (auto node : data->Body)
        {
            finalString += bodyTabString;
            finalString += ParseNodeData(node);
            if (node.Type != NodeType::PreprocessorExpression) // Do not add semis after # blocks
                finalString += ';';
            finalString += '\n';
        }

        if (data->Scoped)
        {
            finalString += initialTabString;
            finalString += "}";
            m_CompileState->TabContext--;
        }

        if (scopePushed)
            m_CompileState->ScopeStack.pop_back();

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

        switch (m_Target)
        {
            default: throw std::exception("Unsupported target");
            case CompileTarget::OpenGLSL:
            {
                if (data->Name == "hl_OutPosition") return "gl_Position";
                if (data->Name == "hl_PixelPosition") return "gl_FragCoord";
                if (data->Name == "hl_VertexId") return "gl_VertexID";
                if (data->Name == "hl_InstanceIndex") return "(gl_BaseInstance + gl_InstanceID)";
            } break;
            case CompileTarget::VulkanGLSL:
            {
                if (data->Name == "hl_OutPosition") return "gl_Position";
                if (data->Name == "hl_PixelPosition") return "gl_FragCoord";
                if (data->Name == "hl_VertexId") return "gl_VertexIndex";
                if (data->Name == "hl_InstanceIndex") return "gl_InstanceIndex";
            } break;
        }

        return data->Name;
    }

    std::string Compiler::ParseBinaryExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::BinaryExpression*)node.Data.get();

        std::string left = ParseNodeData(data->Left);
        std::string right = ParseNodeData(data->Right);

        // Check if the left is a buffer identifier. If yes, then we append .data to the end of it because that is how it is defined in glsl
        // TODO this probably isn't the case for hlsl
        bool isBuffer = data->Left.Type == NodeType::Identifier && IsBufferDefined(left); 
        if (isBuffer) left += ".data";

        if (data->Operator == "[")
            return left + "[" + right + "]";
        if (!isBuffer)
            return left + " " + data->Operator + " " + right;
        return left + "[0] " + data->Operator + " " + right; // If the buffer isn't accessed by a specific index, then we assume element 0
    }

    std::string Compiler::ParseMemberExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::MemberExpression*)node.Data.get();

        std::string obj = ParseNodeData(data->Object);
        std::string prop = ParseNodeData(data->Property);

        // Check if the object is a buffer identifier. If yes, then we append .data[0] to the end of it because that is how it is defined in glsl
        // and we assume the first element if no [] is provided
        // TODO this probably isn't the case for hlsl
        bool isBuffer = data->Object.Type == NodeType::Identifier && IsBufferDefined(obj); 
        if (isBuffer) obj += ".data[0]";

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

        std::string signature;
        std::string funcName = ParseNodeData(data->Left);

        // Universal checks
        if (funcName == "subpassRead")
        {
            if (data->Args.size() != 2) throw std::exception("Expected 2 arguments in 'subpassRead' call");
            
            //if (data->Args[0].Type != NodeType::Identifier) throw std::exception("Expected identifier for template argument 0 of buffer declaration");
            //if (data->Args[1].Type != NodeType::Literal) throw std::exception("Expected literal for template argument 1 of buffer declaration");
        }

        
        switch (m_Target)
        {
            default: throw std::exception("Unsupported target");
            case CompileTarget::OpenGLSL:
            {
                if (funcName == "subpassRead")
                {
                    signature += "texture(";
                    signature += ParseNodeData(data->Args[0]);
                    signature += ", ";
                    signature += ParseNodeData(data->Args[1]);
                    signature += ")";
                }
                else
                {
                    signature += funcName;
                    signature += "(";

                    for (size_t i = 0; i < data->Args.size(); i++)
                    {
                        signature += ParseNodeData(data->Args[i]);
                        if (i != data->Args.size() - 1)
                            signature += ", ";
                    }
                }
            } break;
            case CompileTarget::VulkanGLSL:
            {
                if (funcName == "subpassRead")
                {
                    // We don't care about the texture coordinate here
                    signature += "subpassLoad(";
                    signature += ParseNodeData(data->Args[0]);
                }
                else
                {
                    signature += funcName;
                    signature += "(";

                    for (size_t i = 0; i < data->Args.size(); i++)
                    {
                        signature += ParseNodeData(data->Args[i]);
                        if (i != data->Args.size() - 1)
                            signature += ", ";
                    }
                }
            } break;
        }

        signature += ')';

        return signature;
    }

    std::string Compiler::ParseCastExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::CastExpression*)node.Data.get();

        std::string signature = ParseType(data->Type, false);
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

    std::string Compiler::ParsePreprocessorExpression(const ParseNode& node)
    {
        auto data = (HSL::ParseData::PreprocessorExpression*)node.Data.get();

        if (data->Expression == "include")
        {
            // Purge any whitespace or unneeded chars from the path
            std::string path;
            for (char c : data->Body)
                if (c != '<' && c != '>' && c != '"' && c != '\'' && c != ' ' && c != '\t')
                    path += c;

            std::string finalPath = m_CompileState->IncludeBase.append(path).generic_u8string();

            std::string compileResult = "\n// BEGIN INCLUDE (";
            compileResult += path;
            compileResult += ")\n// #########################\n";

            // Compile the other file separately and return the string
            // Include paths are relative
            auto originialIncludeBase = m_CompileState->IncludeBase;
            compileResult += CompileFromFile(finalPath, m_Target, m_CompileState);
            m_CompileState->IncludeBase = originialIncludeBase;

            compileResult += "// #########################\n";

            return compileResult;
        }
        else
            return "#" + data->Expression + " " + data->Body;
    }

    std::string Compiler::ParseVariableDeclaration(const ParseNode& node)
    {
        auto data = (HSL::ParseData::VariableDeclaration*)node.Data.get();

        if (m_CompileState->ScopeStack.back().ContainsVariable(data->Name))
            throw std::exception("Variable already defined in this scope");

        m_CompileState->ScopeStack.back().Variables.emplace_back(data->Name);

        // Universal checks
        if (data->Type == "buffer")
        {
            if (m_CompileState->ScopeStack.size() != 1) throw std::exception("Buffers can only be defined in the outermost scope");

            if (data->TemplateArgs.empty()) throw std::exception("Expected <> argument in buffer declaration");
            if (data->TemplateArgs.size() != 2) throw std::exception("Expected 2 template arguments in buffer declaration");
            
            if (data->TemplateArgs[0].Type != NodeType::Identifier) throw std::exception("Expected identifier for template argument 0 of buffer declaration");
            if (data->TemplateArgs[1].Type != NodeType::Literal) throw std::exception("Expected literal for template argument 1 of buffer declaration");

            std::string structName = ParseIdentifier(data->TemplateArgs[0]);
            if (!IsStructDefined(structName))
                throw std::exception("Undefined struct type passed to buffer template args");

            m_CompileState->ScopeStack.back().Buffers.emplace_back(data->Name);
        }
        else if (data->Type == "tex2d")
        {
            if (m_CompileState->ScopeStack.size() != 1) throw std::exception("Textures can only be defined in the outermost scope");

            if (data->TemplateArgs.empty()) throw std::exception("Expected <> argument in tex2d declaration");
            if (data->TemplateArgs.size() != 1) throw std::exception("Expected 1 template argument in tex2d declaration");
            
            if (data->TemplateArgs[0].Type != NodeType::Literal) throw std::exception("Expected literal for template argument 0 of tex2d declaration");
        }
        else if (data->Type == "texCube")
        {
            if (m_CompileState->ScopeStack.size() != 1) throw std::exception("Textures can only be defined in the outermost scope");

            if (data->TemplateArgs.empty()) throw std::exception("Expected <> argument in texCube declaration");
            if (data->TemplateArgs.size() != 1) throw std::exception("Expected 1 template argument in texCube declaration");
            
            if (data->TemplateArgs[0].Type != NodeType::Literal) throw std::exception("Expected literal for template argument 0 of texCube declaration");
        }
        else if (data->Type == "subpassTex")
        {
            if (m_CompileState->ScopeStack.size() != 1) throw std::exception("Textures can only be defined in the outermost scope");

            if (data->TemplateArgs.empty()) throw std::exception("Expected <> argument in subpassTex declaration");
            if (data->TemplateArgs.size() != 2) throw std::exception("Expected 2 template arguments in subpassTex declaration");
            
            if (data->TemplateArgs[0].Type != NodeType::Literal) throw std::exception("Expected literal for template argument 0 of subpassTex declaration");
            if (data->TemplateArgs[1].Type != NodeType::Literal) throw std::exception("Expected literal for template argument 1 of subpassTex declaration");
        }

        if (data->KeywordStatus.Flat)
        {
            if (m_CompileState->ScopeStack.size() != 1) throw std::exception("The 'flat' keyword can only be used in the outermost scope");

            if (IsSpecialType(data->Type)) throw std::exception("The 'flat' keyword cannot be used on special types");
        }
        else if (data->KeywordStatus.In)
        {
            if (m_CompileState->ScopeStack.size() != 1) throw std::exception("The 'in' keyword can only be used in the outermost scope");

            if (IsSpecialType(data->Type)) throw std::exception("The 'in' keyword cannot be used on special types");
            
            // Flat MUST be true when using in with int variables
            // TODO: warn about this
            if (data->Type == "int" || data->Type == "uint") data->KeywordStatus.Flat = true;
        }
        else if (data->KeywordStatus.Out)
        {
            if (m_CompileState->ScopeStack.size() != 1) throw std::exception("The 'out' keyword can only be used in the outermost scope");

            if (IsSpecialType(data->Type)) throw std::exception("The 'out' keyword cannot be used on special types");
        }

        if (data->KeywordStatus.In && data->KeywordStatus.Out) throw std::exception("The 'in' and 'out' keyword cannot both be in a declaration");
        
        std::string declaration;
        switch (m_Target)
        {
            default: throw std::exception("Unsupported target");
            case CompileTarget::OpenGLSL:
            case CompileTarget::VulkanGLSL:
            {
                if (data->Type == "buffer")
                {
                    declaration += "layout(";
                    if (m_Target == CompileTarget::VulkanGLSL)
                        declaration += "set=0, ";
                    declaration += "binding=";
                    declaration += ParseLiteral(data->TemplateArgs[1]);
                    declaration += ") ";
                    declaration += ParseType(data->Type, data->KeywordStatus.Uniform);
                    declaration += ' ';
                    declaration += std::string("BUFFER") + std::to_string(m_CompileState->BufferCount++);
                    declaration += "{ ";
                    declaration += ParseIdentifier(data->TemplateArgs[0]);
                    declaration += " data[]; } ";
                    declaration += data->Name;
                }
                else if (data->Type == "tex2d" || data->Type == "texCube")
                {
                    declaration += "layout(";
                    if (m_Target == CompileTarget::VulkanGLSL)
                        declaration += "set=0, ";
                    declaration += "binding=";
                    declaration += ParseLiteral(data->TemplateArgs[0]);
                    declaration += ") ";
                    declaration += ParseType(data->Type, data->KeywordStatus.Uniform);
                    declaration += ' ';
                    declaration += data->Name;
                }
                else if (data->Type == "subpassTex")
                {
                    declaration += "layout(";
                    if (m_Target == CompileTarget::VulkanGLSL)
                    {
                        declaration += "set=0, input_attachment_index=";
                        declaration += ParseLiteral(data->TemplateArgs[1]);
                        declaration += ", ";
                    }
                    declaration += "binding=";
                    declaration += ParseLiteral(data->TemplateArgs[0]);
                    declaration += ") ";
                    declaration += ParseType(data->Type, data->KeywordStatus.Uniform);
                    declaration += ' ';
                    declaration += data->Name;
                }
                else
                {
                    if (data->KeywordStatus.In)
                    {
                        declaration += "layout(location=";
                        declaration += std::to_string(m_CompileState->InVariableContext++);
                        declaration += ") ";
                        declaration += "in ";
                    }
                    else if (data->KeywordStatus.Out)
                    {
                        declaration += "layout(location=";
                        declaration += std::to_string(m_CompileState->OutVariableContext++);
                        declaration += ") ";
                        declaration += "out ";
                    }
                    if (data->KeywordStatus.Const)
                        declaration += "const ";
                    if (data->KeywordStatus.Flat)
                        declaration += "flat ";
                    declaration += ParseType(data->Type, data->KeywordStatus.Uniform);
                    declaration += ' ';
                    declaration += data->Name;

                    if (data->ArrayCount > 0)
                    {
                        declaration += '[';
                        declaration += std::to_string(data->ArrayCount);
                        declaration += ']';
                    }
                    
                    if (data->Init.Type != NodeType::None)
                    {
                        std::string init = ParseNodeData(data->Init);
                        declaration += " = ";
                        declaration += init;
                    }
                }
            } break;
        }

        return declaration;
    }

    std::string Compiler::ParseFunctionDeclaration(const ParseNode& node)
    {
        auto data = (HSL::ParseData::FunctionDeclaration*)node.Data.get();

        if (m_CompileState->ScopeStack.back().ContainsFunction(data->Name))
            throw std::exception("Function already defined in this scope");

        m_CompileState->ScopeStack.back().Functions.emplace_back(data->Name);

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

    std::string Compiler::ParseElseStatement(const ParseNode& node)
    {
        auto data = (HSL::ParseData::ElseStatement*)node.Data.get();

        std::string body = ParseNodeData(data->Body);

        return "else\n" + body;
    }

    std::string Compiler::ParseElseIfStatement(const ParseNode& node)
    {
        auto data = (HSL::ParseData::ElseIfStatement*)node.Data.get();

        std::string condition = ParseNodeData(data->Condition);
        std::string body = ParseNodeData(data->Body);

        return "else if (" + condition + ")\n" + body;
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

        if (m_CompileState->ScopeStack.back().ContainsStruct(data->Name))
            throw std::exception("Struct already defined in this scope");

        m_CompileState->ScopeStack.back().Structs.emplace_back(data->Name);

        std::string body = ParseNodeData(data->Body);

        return "struct " + data->Name + "\n" + body;
    }

    std::string Compiler::GeneratePreDefinitions()
    {
        // Universal predefined identifiers
        m_CompileState->ScopeStack.front().Variables.emplace_back("hl_OutPosition");
        m_CompileState->ScopeStack.front().Variables.emplace_back("hl_PixelPosition");
        m_CompileState->ScopeStack.front().Variables.emplace_back("hl_VertexId");
        m_CompileState->ScopeStack.front().Variables.emplace_back("hl_InstanceIndex");

        m_CompileState->ScopeStack.front().Functions.emplace_back("saturate");

        std::string finalString = "// BEGIN PREDEFINITIONS\n// #########################\n";

        switch (m_Target)
        {
            default: throw std::exception("Unsupported target");
            case CompileTarget::OpenGLSL:
            case CompileTarget::VulkanGLSL:
            {
                finalString += (
                    "float saturate(float v) { return clamp(v, 0.f, 1.f); }\n"
                    "vec2 saturate(vec2 v) { return clamp(v, vec2(0.f), vec2(1.f)); }\n"
                    "vec3 saturate(vec3 v) { return clamp(v, vec3(0.f), vec3(1.f)); }\n"
                    "vec4 saturate(vec4 v) { return clamp(v, vec4(0.f), vec4(1.f)); }\n"
                );
            }
        }

        finalString += "// #########################\n\n";

        return finalString;
    }

    std::string Compiler::ParseType(const std::string& type, bool isUniform)
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
                case CompileTarget::OpenGLSL:
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
                case CompileTarget::OpenGLSL:
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
                case CompileTarget::OpenGLSL:
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
                case CompileTarget::OpenGLSL:
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
                case CompileTarget::OpenGLSL:
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
                case CompileTarget::OpenGLSL:
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
                case CompileTarget::OpenGLSL:
                case CompileTarget::VulkanGLSL: return isUniform ? "uniform sampler2D" : "sampler2D";
                case CompileTarget::HLSL: return "Texture2D";
                //case CompileTarget::Metal:
            }
        }
        else if (type == "texCube")
        {
            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL:
                case CompileTarget::VulkanGLSL: return isUniform ? "uniform samplerCube" : "samplerCube";
                //case CompileTarget::HLSL: return "Texture2D";
                //case CompileTarget::Metal:
            }
        }
        else if (type == "subpassTex")
        {
            switch (m_Target)
            {
                default: throw std::exception("Unsupported target");
                case CompileTarget::OpenGLSL: return isUniform ? "uniform sampler2D" : "sampler2D";
                case CompileTarget::VulkanGLSL: return isUniform ? "uniform subpassInput" : "subpassInput";
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
                case CompileTarget::VulkanGLSL: return isUniform ? "uniform" : "buffer";
                case CompileTarget::HLSL: return isUniform ? "ConstantBuffer" : "StructuredBuffer";
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

    bool Compiler::IsSpecialType(const std::string& type)
    {
        return (
            type == "buffer"     ||
            type == "tex2d"      ||
            type == "texCube"    ||
            type == "subpassTex"
        );
    }
}
