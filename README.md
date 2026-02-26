# Zenith

Zenith is an in-development, hybrid programming language compiler written in C++23. It aims to combine JVM interoperability, low-level memory control, and dynamic scripting in a single language.

> **Status:** Early development. The lexer and parser are functional. The semantic analyser is partially implemented. Code generation is not yet available.

## Features

- **Static and dynamic typing** — explicit types (`int`, `float`, `string`, …) or inferred with `let`/`auto`
- **OOP** — classes with access modifiers, inheritance, operator overloading, constructors/destructors
- **Structs** — public-by-default value types
- **Unsafe blocks** — manual memory management and C FFI
- **Annotations and decorators** — runtime metadata (`@Route`) and compile-time wrappers (`@@Memoize`)
- **Enum-based error handling** with pattern matching via `match`
- **Multiple compile targets** — `native` (in progress), `jvm` (planned)

For the full language reference see [docs/LANGUAGE_SPEC.md](docs/LANGUAGE_SPEC.md).

## Prerequisites

| Tool | Minimum version |
|------|----------------|
| C++ compiler with C++23 support | GCC 13 / Clang 16 / MSVC 19.38 |
| CMake | 4.0 |
| Internet access (first build only) | — |

CMake downloads [GoogleTest](https://github.com/google/googletest) and [fmtlib](https://github.com/fmtlib/fmt) automatically on the first build.

## Building from Source

```bash
git clone https://github.com/Gogo64pro/Zenith.git
cd Zenith
cmake -B build
cmake --build build
```

The compiler binary is placed at `build/Zenith` (or `build\Zenith.exe` on Windows).

## Usage

```
Zenith [options] <input-file>
```

### Options

| Flag | Values | Default | Description |
|------|--------|---------|-------------|
| `--target=` | `native`, `jvm` | `native` | Compilation target |
| `--braces=` | `required`, `optional` | `required` | Whether braces are mandatory around blocks |
| `--gc=` | `generational`, `refcounting`, `none` | `generational` | Garbage-collection strategy |

### Example

```bash
./build/Zenith --target=native hello.zn
```

After a successful run you will find:

- `lexerout.log` — token stream produced by the lexer
- `parserout.log` — AST dump produced by the parser

## Running the Tests

```bash
cmake --build build --target ptest
cd build && ctest --output-on-failure
```

The test suite uses [GoogleTest](https://github.com/google/googletest) and covers the lexer/parser pipeline.

## Project Structure

```
Zenith/
├── CMakeLists.txt
├── docs/
│   └── LANGUAGE_SPEC.md   # Full language reference
└── src/
    ├── main.cpp            # Compiler entry point
    ├── lexer/              # Tokeniser
    ├── parser/             # Recursive-descent parser → AST
    ├── ast/                # AST node definitions
    ├── SemanticAnalysis/   # Type checking and symbol resolution
    ├── visitor/            # Visitor pattern for AST traversal
    ├── core/               # Polymorphic type utilities
    ├── exceptions/         # LexError, ParseError, SemanticAnalysisError
    ├── utils/              # File I/O, argument parsing, helpers
    └── test/               # GoogleTest unit tests
```

## Roadmap

- [ ] Complete parser
- [ ] Finish semantic analysis
- [ ] Native code generation
- [ ] JVM compilation target
- [ ] Standard library (`IO`, collections, …)
- [ ] IDE / LSP support

## License

No license file is present in this repository. All rights are reserved by the author unless otherwise stated. Contact the repository owner for licensing enquiries.

---


The full language reference has moved to **[docs/LANGUAGE_SPEC.md](docs/LANGUAGE_SPEC.md)**.
