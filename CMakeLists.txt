cmake_minimum_required(VERSION 3.25)
project(Zenith)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-omit-frame-pointer -fno-inline -fno-inline-functions -fno-inline-small-functions")
#set(LLVM_INCLUDE_DIRS "C:\\Program Files\\LLVM\\include")
#set(LLVM_DIR "C:\\Program Files\\LLVM\\lib\\cmake\\llvm")
#find_package(LLVM 18.1.7 REQUIRED CONFIG)
#include_directories(${LLVM_INCLUDE_DIRS})

#target_link_libraries(PRIVATE
#	LLVMCore
#	LLVMSupport)

add_executable(Zenith
#	src/test.cpp
	src/main.cpp
	src/utils/mainargs.hpp
	src/lexer/lexer.hpp src/lexer/lexer.cpp
	src/parser/parser.hpp src/parser/parser.cpp
	src/ast/ASTNode.hpp
	src/ast/Declarations.hpp src/ast/Declarations.hpp
	src/ast/Statements.hpp src/ast/Expressions.hpp
	src/parser/ParseError.cpp src/parser/ParseError.hpp
	src/ast/TypeNodes.hpp
	src/ast/Other.hpp src/utils/ReadFile.cpp src/utils/ReadFile.hpp
	src/utils/RemovePadding.cpp src/utils/RemovePadding.hpp
	src/lexer/LexError.cpp src/lexer/LexError.hpp src/utils/small_vector.cpp src/utils/small_vector.hpp src/ErrorReporter.cpp src/ErrorReporter.hpp)

#add_compile_options(-fsanitize=address)