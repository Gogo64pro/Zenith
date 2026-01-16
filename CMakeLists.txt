cmake_minimum_required(VERSION 3.31)

project(Zenith)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)
# --- Logging ---
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
# --- GoogleTest Setup ---
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/373af2e3df71599b87a40ce0e37164523849166b.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
# --- libFmt Setup ---
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG        add164f6b3f5deb800443b87ba812fb19ad7cd5b) # 10.2.1
FetchContent_MakeAvailable(fmt)


# --- Define ALL Source Files ---
set(TUs
        src/lexer/lexer.cpp
        src/parser/parser.cpp
        src/exceptions/ParseError.cpp
        src/utils/ReadFile.cpp
        src/utils/RemovePadding.cpp
        src/exceptions/LexError.cpp
        src/utils/small_vector.cpp
        src/exceptions/ErrorReporter.cpp
        src/SemanticAnalysis/SemanticAnalyzer.cpp
        src/SemanticAnalysis/SymbolTable.cpp
        src/ast/acceptMethods.cpp
        src/visitor/Visitor.cpp
        src/ast/AST.hpp
        src/ast/SourceLocation.hpp
        src/core/polymorphic.hpp
        src/core/polymorphic_ref.hpp
        src/core/polymorphic_variant.hpp
)

add_executable(Zenith ${TUs} src/main.cpp
)
target_include_directories(Zenith PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_sources(Zenith
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${MODULES}
        )

target_link_libraries(Zenith PRIVATE fmt::fmt)

# Testing

add_executable(ptest
        src/test/test.cpp
        src/test/PolymorphicTest.cpp
        ${ZENITH_SOURCES}
        )
target_precompile_headers(ptest PRIVATE ${PCH_HEADERS})
target_include_directories(ptest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ptest PRIVATE gtest gtest_main)
enable_testing()
add_test(NAME ptest COMMAND ptest)