//
// Created by gogop on 2/23/2026.
//


#include <gtest/gtest.h>
#include <lexer/lexer.hpp>

using namespace zenith;

static std::vector<Token> lex(const std::string& src) {
    return Lexer(src, "<test>").tokenize();
}


static Token lexOne(const std::string& src, TokenType expected) {
    auto toks = lex(src);
    // strip trailing EOF if present
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    EXPECT_EQ(toks.size(), 1u) << "Expected exactly 1 token for: " << src;
    if (!toks.empty()) {
        EXPECT_EQ(toks[0].type, expected)
            << "For input '" << src << "': got "
            << Lexer::tokenToString(toks[0].type)
            << " expected " << Lexer::tokenToString(expected);
        return toks[0];
    }
    return {};
}

// ===========================================================================
// 1. Identifiers
// ===========================================================================

TEST(LexerIdentifiers, SimpleIdentifier) {
    auto t = lexOne("hello", TokenType::IDENTIFIER);
    EXPECT_EQ(t.lexeme, "hello");
}

TEST(LexerIdentifiers, UnderscoreStart) {
    auto t = lexOne("_myVar", TokenType::IDENTIFIER);
    EXPECT_EQ(t.lexeme, "_myVar");
}

TEST(LexerIdentifiers, MixedCase) {
    auto t = lexOne("CamelCase42", TokenType::IDENTIFIER);
    EXPECT_EQ(t.lexeme, "CamelCase42");
}

TEST(LexerIdentifiers, SingleUnderscore) {
    // '_' alone is the wildcard token, not an identifier
    auto t = lexOne("_", TokenType::WILDCARD);
    EXPECT_EQ(t.lexeme, "_");
}

TEST(LexerIdentifiers, DigitCannotStartIdentifier) {
    // '9hello' should produce NUMBER then IDENTIFIER, not one token
    auto toks = lex("9hello");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_GE(toks.size(), 2u);
    EXPECT_EQ(toks[0].type, TokenType::NUMBER);
    EXPECT_EQ(toks[1].type, TokenType::IDENTIFIER);
}

// ===========================================================================
// 2. Keywords — each keyword must NOT produce IDENTIFIER
// ===========================================================================

struct KeywordCase { const char* src; TokenType expected; };

class LexerKeywords : public ::testing::TestWithParam<KeywordCase> {};

TEST_P(LexerKeywords, ProducesCorrectToken) {
    auto [src, expected] = GetParam();
    auto toks = lex(src);
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 1u) << "Input: " << src;
    EXPECT_EQ(toks[0].type, expected) << "Input: " << src;
    EXPECT_EQ(toks[0].lexeme, std::string(src));
}

INSTANTIATE_TEST_SUITE_P(AllKeywords, LexerKeywords, ::testing::Values(
    KeywordCase{"package",    TokenType::PACKAGE},
    KeywordCase{"import",     TokenType::IMPORT},
    KeywordCase{"java",       TokenType::JAVA},
    KeywordCase{"extern",     TokenType::EXTERN},
    KeywordCase{"class",      TokenType::CLASS},
    KeywordCase{"struct",     TokenType::STRUCT},
    KeywordCase{"actor",      TokenType::ACTOR},
    KeywordCase{"enum",       TokenType::ENUM},
    KeywordCase{"template",   TokenType::TEMPLATE},
    KeywordCase{"typename",   TokenType::TYPENAME},
    KeywordCase{"annotation", TokenType::ANNOTATION},
    KeywordCase{"fun",        TokenType::FUN},
    KeywordCase{"on",         TokenType::ON},
    KeywordCase{"new",        TokenType::NEW},
    KeywordCase{"return",     TokenType::RETURN},
    KeywordCase{"if",         TokenType::IF},
    KeywordCase{"else",       TokenType::ELSE},
    KeywordCase{"for",        TokenType::FOR},
    KeywordCase{"while",      TokenType::WHILE},
    KeywordCase{"do",         TokenType::DO},
    KeywordCase{"break",      TokenType::BREAK},
    KeywordCase{"continue",   TokenType::CONTINUE},
    KeywordCase{"auto",       TokenType::AUTO},
    KeywordCase{"true",       TokenType::TRUE},
    KeywordCase{"false",      TokenType::FALSE},
    KeywordCase{"null",       TokenType::NULL_LIT},
    KeywordCase{"this",       TokenType::THIS},
    KeywordCase{"super",      TokenType::SUPER},
    KeywordCase{"match",      TokenType::MATCH},
    KeywordCase{"is",         TokenType::IS},
    KeywordCase{"as",         TokenType::AS},
    KeywordCase{"unsafe",     TokenType::UNSAFE},
    KeywordCase{"scope",      TokenType::SCOPE},
    KeywordCase{"freeobj",    TokenType::FREEOBJ},
    KeywordCase{"operator",   TokenType::OPERATOR},
    KeywordCase{"getter",     TokenType::GETTER},
    KeywordCase{"setter",     TokenType::SETTER},
    KeywordCase{"public",     TokenType::PUBLIC},
    KeywordCase{"private",    TokenType::PRIVATE},
    KeywordCase{"protected",  TokenType::PROTECTED},
    KeywordCase{"privatew",   TokenType::PRIVATEW},
    KeywordCase{"protectedw", TokenType::PROTECTEDW},
    KeywordCase{"const",      TokenType::CONST},
    KeywordCase{"static",     TokenType::STATIC},
    KeywordCase{"signed",     TokenType::SIGNED},
    KeywordCase{"unsigned",   TokenType::UNSIGNED},
    KeywordCase{"int",        TokenType::INT},
    KeywordCase{"long",       TokenType::LONG},
    KeywordCase{"short",      TokenType::SHORT},
    KeywordCase{"byte",       TokenType::BYTE},
    KeywordCase{"float",      TokenType::FLOAT},
    KeywordCase{"double",     TokenType::DOUBLE},
    KeywordCase{"string",     TokenType::STRING},
    KeywordCase{"bool",       TokenType::BOOL},
    KeywordCase{"void",       TokenType::VOID},
    KeywordCase{"dynamic",    TokenType::DYNAMIC}
));

// Keyword prefix must not be stolen from a longer identifier
TEST(LexerKeywords, KeywordPrefixIsIdentifier) {
    auto t = lexOne("integer", TokenType::IDENTIFIER);
    EXPECT_EQ(t.lexeme, "integer");
}

TEST(LexerKeywords, KeywordSuffixIsIdentifier) {
    auto t = lexOne("myint", TokenType::IDENTIFIER);
    EXPECT_EQ(t.lexeme, "myint");
}

// ===========================================================================
// 3. Integer & Float Numbers
// ===========================================================================

TEST(LexerNumbers, DecimalInteger) {
    auto t = lexOne("42", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "42");
}

TEST(LexerNumbers, Zero) {
    auto t = lexOne("0", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "0");
}

TEST(LexerNumbers, FloatWithFSuffix) {
    auto t = lexOne("3.14f", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "3.14f");
}

TEST(LexerNumbers, FloatNoSuffix) {
    auto t = lexOne("1.5", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "1.5");
}

TEST(LexerNumbers, LongSuffix) {
    auto t = lexOne("9999999999l", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "9999999999l");
}

TEST(LexerNumbers, ZeroFloat) {
    auto t = lexOne("0.0f", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "0.0f");
}

// ===========================================================================
// 4. Hex Literals
// ===========================================================================

TEST(LexerHex, SimpleHex) {
    auto t = lexOne("0xFF", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "0xFF");
}

TEST(LexerHex, UpperCaseX) {
    auto t = lexOne("0XFF", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "0XFF");
}

TEST(LexerHex, DeadBeef) {
    auto t = lexOne("0xDEADBEEF", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "0xDEADBEEF");
}

TEST(LexerHex, HexWithLongSuffix) {
    auto t = lexOne("0x123456789ABCDEFl", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "0x123456789ABCDEFl");
}

TEST(LexerHex, HexAllLowerDigits) {
    auto t = lexOne("0xabcdef", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "0xabcdef");
}

// ===========================================================================
// 5. String Literals
// ===========================================================================

TEST(LexerStrings, EmptyString) {
    auto t = lexOne(R"("")", TokenType::STRING_LIT);
    EXPECT_EQ(t.lexeme, "\"\"");
}

TEST(LexerStrings, SimpleString) {
    auto t = lexOne(R"("hello")", TokenType::STRING_LIT);
    EXPECT_EQ(t.lexeme, "\"hello\"");
}

TEST(LexerStrings, EscapeSequences) {
    // "a\nb" — backslash followed by any char is a valid string_char
    auto t = lexOne(R"("a\nb")", TokenType::STRING_LIT);
    EXPECT_EQ(t.lexeme, R"("a\nb")");
}

TEST(LexerStrings, EscapedQuote) {
    auto t = lexOne(R"("say \"hi\"")", TokenType::STRING_LIT);
    EXPECT_EQ(t.lexeme, R"("say \"hi\"")");
}

// ===========================================================================
// 6. Template Strings (string interpolation)
// ===========================================================================

// A template string contains at least one ${...} interpolation.
// The lexer may emit it as a single TEMPLATE_STRING token or break it into
// parts; adjust TokenType below if your lexer uses a different strategy.

TEST(LexerTemplateStrings, SimpleInterpolation) {
    // "${x}"
    auto toks = lex(R"(`${x}`)");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_FALSE(toks.empty());
    // At least one token; the first (or only) must signal template string
    EXPECT_EQ(toks[0].type, TokenType::TEMPLATE_LIT);
}

TEST(LexerTemplateStrings, InterpolationWithText) {
    // "Value is: ${pi}"
    auto toks = lex(R"(`Value is: ${pi}`)");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_FALSE(toks.empty());
    EXPECT_EQ(toks[0].type, TokenType::TEMPLATE_LIT);
}

TEST(LexerTemplateStrings, NoInterpolationIsPlainString) {
    auto t = lexOne(R"("no interp here")", TokenType::STRING_LIT);
    EXPECT_EQ(t.lexeme, R"("no interp here")");
}

// ===========================================================================
// 7. Operators — single and multi-character
// ===========================================================================

struct OpCase { const char* src; TokenType expected; };

class LexerOperators : public ::testing::TestWithParam<OpCase> {};

TEST_P(LexerOperators, ProducesCorrectToken) {
    auto [src, expected] = GetParam();
    auto toks = lex(src);
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 1u) << "Input: " << src;
    EXPECT_EQ(toks[0].type, expected) << "Input: " << src;
}

INSTANTIATE_TEST_SUITE_P(ArithmeticOps, LexerOperators, ::testing::Values(
    OpCase{"+",  TokenType::PLUS},
    OpCase{"-",  TokenType::MINUS},
    OpCase{"*",  TokenType::STAR},
    OpCase{"/",  TokenType::SLASH},
    OpCase{"%",  TokenType::PERCENT}
));

INSTANTIATE_TEST_SUITE_P(AssignOps, LexerOperators, ::testing::Values(
    OpCase{"=",   TokenType::EQUAL},
    OpCase{"+=",  TokenType::PLUS_EQUALS},
    OpCase{"-=",  TokenType::MINUS_EQUALS},
    OpCase{"*=",  TokenType::STAR_EQUALS},
    OpCase{"/=",  TokenType::SLASH_EQUALS},
    OpCase{"%=",  TokenType::PERCENT_EQUALS},
    OpCase{"&=",  TokenType::AMP_ASSIGN},
    OpCase{"|=",  TokenType::PIPE_ASSIGN},
    OpCase{"^=",  TokenType::CARET_ASSIGN},
    OpCase{"<<=", TokenType::LSHIFT_ASSIGN},
    OpCase{">>=", TokenType::RSHIFT_ASSIGN}
));

INSTANTIATE_TEST_SUITE_P(CompareOps, LexerOperators, ::testing::Values(
    OpCase{"==", TokenType::EQUAL_EQUAL},
    OpCase{"!=", TokenType::BANG_EQUAL},
    OpCase{"<",  TokenType::LESS},
    OpCase{"<=", TokenType::LESS_EQUAL},
    OpCase{">",  TokenType::GREATER},
    OpCase{">=", TokenType::GREATER_EQUAL}
));

INSTANTIATE_TEST_SUITE_P(LogicalOps, LexerOperators, ::testing::Values(
    OpCase{"&&", TokenType::AND},
    OpCase{"||", TokenType::OR},
    OpCase{"!",  TokenType::BANG}
));

INSTANTIATE_TEST_SUITE_P(BitwiseOps, LexerOperators, ::testing::Values(
    OpCase{"&",  TokenType::AMP},
    OpCase{"|",  TokenType::PIPE},
    OpCase{"^",  TokenType::CARET},
    OpCase{"~",  TokenType::TILDE},
    OpCase{"<<", TokenType::LSHIFT},
    OpCase{">>", TokenType::RSHIFT}
));

INSTANTIATE_TEST_SUITE_P(IncrDecrOps, LexerOperators, ::testing::Values(
    OpCase{"++", TokenType::PLUS_PLUS},
    OpCase{"--", TokenType::MINUS_MINUS}
));

INSTANTIATE_TEST_SUITE_P(PipelineOps, LexerOperators, ::testing::Values(
    OpCase{"|>",  TokenType::PIPE_GT},
    OpCase{"|=>", TokenType::PIPE_FAT_ARROW}
));

INSTANTIATE_TEST_SUITE_P(MiscOps, LexerOperators, ::testing::Values(
    OpCase{"=>",  TokenType::LAMBARROW},
    OpCase{"->",  TokenType::ARROW},
    OpCase{"...", TokenType::ELLIPSIS},
    OpCase{"?",   TokenType::QUESTION},
    OpCase{":",   TokenType::COLON}
));

// ===========================================================================
// 8. Punctuation
// ===========================================================================

struct PunctCase { const char* src; TokenType expected; };

class LexerPunctuation : public ::testing::TestWithParam<PunctCase> {};

TEST_P(LexerPunctuation, ProducesCorrectToken) {
    auto [src, expected] = GetParam();
    auto toks = lex(src);
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 1u) << "Input: " << src;
    EXPECT_EQ(toks[0].type, expected) << "Input: " << src;
}

INSTANTIATE_TEST_SUITE_P(Brackets, LexerPunctuation, ::testing::Values(
    PunctCase{"(",  TokenType::LPAREN},
    PunctCase{")",  TokenType::RPAREN},
    PunctCase{"{",  TokenType::LBRACE},
    PunctCase{"}",  TokenType::RBRACE},
    PunctCase{"[",  TokenType::LBRACKET},
    PunctCase{"]",  TokenType::RBRACKET}
));

INSTANTIATE_TEST_SUITE_P(Other, LexerPunctuation, ::testing::Values(
    PunctCase{";",  TokenType::SEMICOLON},
    PunctCase{",",  TokenType::COMMA},
    PunctCase{".",  TokenType::DOT},
    PunctCase{"@",  TokenType::AT},
    PunctCase{"@@", TokenType::AT_AT}
));

// ===========================================================================
// 9. Annotations and decorators
// ===========================================================================

TEST(LexerAnnotations, SingleAt) {
    auto toks = lex("@Serializable");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].type, TokenType::AT);
    EXPECT_EQ(toks[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(toks[1].lexeme, "Serializable");
}

TEST(LexerAnnotations, DoubleAt) {
    auto toks = lex("@@Memoize");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].type, TokenType::AT_AT);
    EXPECT_EQ(toks[1].type, TokenType::IDENTIFIER);
}

// ===========================================================================
// 10. Comments are silently consumed
// ===========================================================================

TEST(LexerComments, BlockCommentProducesNoToken) {
    auto toks = lex("(* this is a comment *)");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    EXPECT_TRUE(toks.empty());
}

TEST(LexerComments, CommentBetweenTokens) {
    auto toks = lex("a (* comment *) b");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].lexeme, "a");
    EXPECT_EQ(toks[1].lexeme, "b");
}

TEST(LexerComments, NestedBlockComment) {
    // Some languages support nested comments; verify no tokens are emitted.
    auto toks = lex("(* outer (* inner *) still comment *)");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    EXPECT_TRUE(toks.empty());
}

// ===========================================================================
// 11. Whitespace is consumed
// ===========================================================================

TEST(LexerWhitespace, SpacesTabsNewlines) {
    auto toks = lex("   \t\n  42  \n");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].type, TokenType::NUMBER);
}

// ===========================================================================
// 12. Operator ambiguity / maximal-munch
// ===========================================================================

TEST(LexerMaximalMunch, LessEqNotTwoTokens) {
    auto toks = lex("<=");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].type, TokenType::LESS_EQUAL);
}

TEST(LexerMaximalMunch, LShiftAssignNotThreeTokens) {
    auto toks = lex("<<=");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].type, TokenType::LSHIFT_ASSIGN);
}

TEST(LexerMaximalMunch, PipeFatArrowNotPipeAndArrow) {
    // |=> must be one token, not | then =>
    auto toks = lex("|=>");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].type, TokenType::PIPE_FAT_ARROW);
}

TEST(LexerMaximalMunch, PipeGtNotPipeAndGt) {
    auto toks = lex("|>");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].type, TokenType::PIPE_GT);
}

TEST(LexerMaximalMunch, IncrementNotTwoPluses) {
    auto toks = lex("++");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].type, TokenType::PLUS_PLUS);
}

// ===========================================================================
// 13. Line and column tracking
// ===========================================================================

TEST(LexerPositions, LineNumbersIncrement) {
    auto toks = lex("a\nb\nc");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 3u);
    EXPECT_EQ(toks[0].loc.line, 1);
    EXPECT_EQ(toks[1].loc.line, 2);
    EXPECT_EQ(toks[2].loc.line, 3);
}

TEST(LexerPositions, ColumnNumbersIncrease) {
    auto toks = lex("abc def");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].loc.column, 1);
    EXPECT_GT(toks[1].loc.column, toks[0].loc.column);
}

// ===========================================================================
// 14. tokenToString — sanity checks
// ===========================================================================

TEST(LexerTokenToString, IdentifierHasName) {
    auto s = Lexer::tokenToString(TokenType::IDENTIFIER);
    EXPECT_FALSE(s.empty());
}

TEST(LexerTokenToString, KeywordHasName) {
    auto s = Lexer::tokenToString(TokenType::CLASS);
    EXPECT_FALSE(s.empty());
}

TEST(LexerTokenToString, OperatorHasName) {
    auto s = Lexer::tokenToString(TokenType::PLUS);
    EXPECT_FALSE(s.empty());
}

// ===========================================================================
// 15. Multi-token sequences from the test program
// ===========================================================================

TEST(LexerSequences, PackageDecl) {
    // package engine.core;
    auto toks = lex("package engine.core;");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 5u);
    EXPECT_EQ(toks[0].type, TokenType::PACKAGE);
    EXPECT_EQ(toks[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(toks[1].lexeme, "engine");
    EXPECT_EQ(toks[2].type, TokenType::DOT);
    EXPECT_EQ(toks[3].type, TokenType::IDENTIFIER);
    EXPECT_EQ(toks[3].lexeme, "core");
    EXPECT_EQ(toks[4].type, TokenType::SEMICOLON);
}

TEST(LexerSequences, JavaImport) {
    // import java org.utils.Logger;
    auto toks = lex("import java org.utils.Logger;");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    EXPECT_EQ(toks[0].type, TokenType::IMPORT);
    EXPECT_EQ(toks[1].type, TokenType::JAVA);
    EXPECT_EQ(toks[2].type, TokenType::IDENTIFIER);
    EXPECT_EQ(toks[2].lexeme, "org");
}

TEST(LexerSequences, StringImport) {
    auto toks = lex(R"(import "platform/native_gl.h";)");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    EXPECT_EQ(toks[0].type, TokenType::IMPORT);
    EXPECT_EQ(toks[1].type, TokenType::STRING_LIT);
}

TEST(LexerSequences, TemplateBrackets) {
    // Container<int>
    auto toks = lex("Container<int>");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 4u);
    EXPECT_EQ(toks[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(toks[1].type, TokenType::LESS);
    EXPECT_EQ(toks[2].type, TokenType::INT);
    EXPECT_EQ(toks[3].type, TokenType::GREATER);
}

TEST(LexerSequences, VariadicEllipsis) {
    auto toks = lex("typename ...");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].type, TokenType::TYPENAME);
    EXPECT_EQ(toks[1].type, TokenType::ELLIPSIS);
}

TEST(LexerSequences, SignedUnsignedTypes) {
    auto toks = lex("signed int");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].type, TokenType::SIGNED);
    EXPECT_EQ(toks[1].type, TokenType::INT);
}

TEST(LexerSequences, AssignmentChain) {
    // d <<= 1;
    auto toks = lex("d <<= 1;");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 4u);
    EXPECT_EQ(toks[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(toks[1].type, TokenType::LSHIFT_ASSIGN);
    EXPECT_EQ(toks[2].type, TokenType::NUMBER);
    EXPECT_EQ(toks[3].type, TokenType::SEMICOLON);
}

TEST(LexerSequences, FatArrowLambda) {
    // (x) => x * 2
    auto toks = lex("(x) => x * 2");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 6u);
    EXPECT_EQ(toks[2].type, TokenType::LAMBARROW);
}

TEST(LexerSequences, PipelineChain) {
    // raw |> (x) => x * 2 |> clamp 
    auto toks = lex("raw |> (x) => x * 2 |> clamp");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    // Verify at least two PIPE_GT tokens appear
    int pipeCount = 0;
    for (auto& t : toks)
        if (t.type == TokenType::PIPE_GT) pipeCount++;
    EXPECT_EQ(pipeCount, 2);
}

// ===========================================================================
// 16. Edge cases
// ===========================================================================

TEST(LexerEdgeCases, EmptySource) {
    auto toks = lex("");
    // Should produce only EOF (or an empty list), not crash
    for (auto& t : toks)
        EXPECT_EQ(t.type, TokenType::EOF_TOKEN);
}

TEST(LexerEdgeCases, OnlyWhitespace) {
    auto toks = lex("   \t\n\r\n   ");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    EXPECT_TRUE(toks.empty());
}

TEST(LexerEdgeCases, OnlyComment) {
    auto toks = lex("(* just a comment *)");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    EXPECT_TRUE(toks.empty());
}

TEST(LexerEdgeCases, FloatZeroPoint) {
    auto t = lexOne("0.016f", TokenType::NUMBER);
    EXPECT_EQ(t.lexeme, "0.016f");
}

TEST(LexerEdgeCases, PostfixIncVsAssignment) {
    // a++ must NOT consume the ++ as two '+' tokens
    auto toks = lex("a++");
    while (!toks.empty() && toks.back().type == TokenType::EOF_TOKEN)
        toks.pop_back();
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(toks[1].type, TokenType::PLUS_PLUS);
}