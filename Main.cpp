#include "pch.h"

#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"

int main(char* argv, int argc)
{
    std::ifstream f("../../../test/Test.hsl");
    std::stringstream buffer;
    buffer << f.rdbuf();

    auto tokens = HSL::Lexer::Lexify(buffer.str());
    auto parsed = HSL::Parser::Parse(tokens);

    HSL::Compiler comp(HSL::CompileTarget::OpenGLSL);
    auto finalCode = comp.Compile(parsed);

    std::ofstream o("../../../test/Out.glsl", std::ios::binary);
    o << finalCode;

    return 0;
}