#include "pch.h"

#include "Lexer.h"
#include "Parser.h"

int main(char* argv, int argc)
{
    std::ifstream f("../../../test/Test.hsl");
    std::stringstream buffer;
    buffer << f.rdbuf();

    auto tokens = HSL::Lexer::Lexify(buffer.str());
    auto parsed = HSL::Parser::Parse(tokens);
    auto nodes = ((HSL::ParseData::BlockStatement*)parsed.Data.get())->Body;

    return 0;
}