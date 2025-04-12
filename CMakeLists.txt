cmake_minimum_required(VERSION 3.15)
project(Zenith)

set(CMAKE_CXX_STANDARD 23)

add_executable(Zenith
	src/ErrorReporter.cpp
	src/lexer/lexer.cpp
	src/lexer/LexError.cpp
	src/main.cpp
	src/parser/ParseError.cpp
	src/parser/parser.cpp
	src/utils/ReadFile.cpp
	src/utils/RemovePadding.cpp
	src/utils/small_vector.cpp)