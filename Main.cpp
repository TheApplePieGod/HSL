#include "pch.h"

#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"

int main(char* argv, int argc)
{
    std::string finalCode = HSL::Compiler::CompileFromFile("../../../test/PBRVert.hsl", HSL::CompileTarget::OpenGLSL);

    std::ofstream o("../../../test/Out.glsl", std::ios::binary);
    o << finalCode;

    return 0;
}