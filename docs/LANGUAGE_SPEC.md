# Zenith Language Specification

> **Note:** This specification is a work in progress. Some features described here are not yet fully implemented in the compiler.

---

## Table of Contents

1. [Keywords](#keywords)
2. [Types](#types)
3. [Blocks and Control Flow](#blocks-and-control-flow)
4. [Object-Oriented Programming](#object-oriented-programming)
5. [Metaprogramming](#metaprogramming)
6. [Error Handling](#error-handling)
7. [Pipelines](#pipelines)
8. [Unique Features](#unique-features)
9. [Grammar Highlights](#grammar-highlights)

---

## Keywords

### General

| Keyword | Description |
|---------|-------------|
| `auto` | Type inference |
| `dynamic` | Dynamic (heap-allocated) type |
| `fun` | Declares a function (works with both static and dynamic return types) |
| `unsigned` | Makes a numeric variable unsigned |
| `signed` | Makes a numeric variable signed |
| `class` | Declares a class |
| `struct` | Declares a struct |
| `unsafe` | Opens a block where unsafe operations are allowed |
| `new` | Creates a new object instance |
| `let` | Declares a dynamic variable |
| `var` | Declares a variable (dynamic, hoistable) |
| `hoist` *(deprecated)* | Hoists a variable to the top of its scope |

### Class Access Modifiers

| Modifier | Description |
|----------|-------------|
| `public` | Accessible from anywhere |
| `protected` | Accessible within the class and subclasses |
| `private` | Accessible only within the class |
| `privatew` | Readable everywhere, writable only within the class |
| `protectedw` | Readable everywhere, writable only within protected/private scope |

### Imports

| Keyword | Description |
|---------|-------------|
| `import` | Imports a Zenith file |
| `import java` | Imports a Java class (requires JVM target) |
| `extern "C"` | Declares C functions (must be inside an `unsafe` block) |
| `package` | Sets a package namespace (required for JVM compilation) |

---

## Types

### Primitive Number Types

| Type | Size | Description |
|------|------|-------------|
| `byte` | 1 byte | Integer |
| `short` | 2 bytes | Integer |
| `int` | 4 bytes | Integer |
| `long` | 8 bytes | Integer |
| `float` | 4 bytes | Decimal number |
| `double` | 8 bytes | Decimal number |
| `dynamic` | heap | Heap-allocated dynamic data |

### Other Primitive Types

| Type | Description |
|------|-------------|
| `string` | String (not a class) |
| `freeobj` | JavaScript-like object; lowered to a struct at compile time if its shape is static |
| `bool` | Boolean |

### `freeobj` Examples

```zenith
// Inferred as freeobj
auto obj = {
    name: "Zenith",
    version: 1.0f,
    getInfo: () => `"${this.name} v${this.version}"`
}

// Explicit freeobj
freeobj obj = freeobj {
    name: "Zenith",
    version: 1.0f
}

// Edge cases
struct SomeStruct { ... }
SomeStruct a = {}          // becomes a struct
SomeStruct b = freeobj {}  // ERROR: explicit freeobj cannot be assigned to struct type
```

### Built-in Class Types

- `IO` — basic console and file I/O

> The standard library is not yet complete.

---

## Blocks and Control Flow

### Functions

```zenith
fun returnType name(type argName) {
    // body
}
```

### `for` Loop

```zenith
// C-style
for (let i = 0; i < 123; i++) {
    // ...
}

// Range-based
int arr[1024] = ...
for (int i : arr) {
    // iterate over arr
}
```

### `while` Loop

```zenith
while (condition) {
    // ...
}
```

### `do-while` Loop

```zenith
do {
    // ...
} while (condition)
```

### `if` / `else`

```zenith
if (condition) {
    // ...
} else {
    // ...
}

if (condition) {
    // ...
} else if (otherCondition) {
    // ...
}
```

---

## Object-Oriented Programming

### Classes

Classes encapsulate data and behaviour. They support:

- Inheritance
- Operator overloading
- Constructors and destructors
- Method overriding
- `const` fields (immutable after construction)
- `static` members (no instance required)

```zenith
class Dog {
    privatew double age;
    protectedw string breed;
    const public string name;

    public Dog(string name, string breed) : name(name) {
        this.age = 0;
        this.breed = breed;
    }

    public void describe() {
        IO.print(`"${this.name} is a ${this.breed} aged ${this.age}."`);
    }

    protected void setBreed(string newBreed) {
        this.breed = newBreed;
    }
}

class Mallinois : Dog {
    public Mallinois(string name) : Dog(name, "Mallinois") {}

    public void train() {
        this.age = 3;
        this.breed = "Trained Mallinois";
        IO.print(`"${super.name} is now a ${this.breed}!"`);
    }
}
```

#### Operator Overloading

```zenith
class Vector3D {
    privatew double x;
    privatew double y;
    privatew double z;

    public Vector3D(double x, double y, double z) {
        this.x = x; this.y = y; this.z = z;
    }

    public Vector3D operator+(Vector3D other) {
        return new Vector3D(this.x + other.x, this.y + other.y, this.z + other.z);
    }

    public Vector3D operator*(double scalar) {
        return new Vector3D(this.x * scalar, this.y * scalar, this.z * scalar);
    }

    public bool operator==(Vector3D other) {
        return this.x == other.x && this.y == other.y && this.z == other.z;
    }

    public string toString() {
        return `"(${this.x}, ${this.y}, ${this.z})"`;
    }
}
```

### Structs

Structs are like classes but default to `public` access. They do not have a constructor by default.

```zenith
struct Vector3 {
    int x;
    int y;
    int z;
}

Vector3 a = {1, 2, 4}
Vector3 b = { x: 1, y: 2, z: 4 }
```

---

## Metaprogramming

### Annotations

Annotations attach metadata to any declaration. Built-in annotations may be processed at compile time; user-defined annotations provide runtime metadata.

```zenith
@Breedable
class Cat { ... }

@Register(id="some_entry", registry="block")
class Cube { ... }

@NotNull
let value = 1

@IsAGreatFunction
fun givesGreatResponses() { ... }
```

#### Defining Annotations

```zenith
annotation ComesFromNet              // marker annotation

annotation Name {
    string value;
}

annotation Register {
    string registry;
    string id;
}
```

### Decorators

Decorators wrap functions/methods with additional behaviour at compile time.

```zenith
@@Memoize
fun fibonacci(int n) -> int { ... }

@@Log
fun compute() { ... }
```

> Custom decorators are a work in progress.

---

## Error Handling

### Enum-Based Model

```zenith
enum ReadFileRV {
    SUCCESS(data: string),
    ERR_NO_ACCESS,
    ERR_IN_USE
}
```

Use with `match`:

```zenith
match value {
    SUCCESS => {
        // handle success, data is available here
    }
    ERR_NO_ACCESS => {
        IO.print("Access denied")
    }
    ERR_IN_USE => {
        IO.print("File is in use")
    }
}
```

### JVM Exceptions

```zenith
@Throws
fun riskyOperation() { ... }
```

---

## Pipelines

> Pipelines are a work in progress.

```zenith
path
|> readFile
|> fileType
|> match {
    XML  |> parseXml  |> docFromXml
    JSON |> parseJson |> docFromJson
    _ |=> { IO.print("wrong type") } |> break
}
|> modifyDoc
|> saveDoc(path, _)   // _ is the result from the previous step
```

---

## Unique Features

| Feature | Example | Notes |
|---------|---------|-------|
| Write restrictions | `privatew int x;` | Class-only write, public read |
| Hoisting *(deprecated)* | `hoist var x = 10;` | Like JavaScript |
| Configurable braces | `--braces=optional` | Compiler flag |
| Free objects | `let obj = { x: 10 };` | Optimised to struct when shape is static |
| Dual GC | `--gc=none` | Manual and GC modes can coexist |

---

## Grammar Highlights (EBNF)

```ebnf
freeobj      = "{" [ IDENT ":" expr { "," IDENT ":" expr } ] "}" ;
unsafe_block = "unsafe" "{" { stmt } "}" ;
async_func   = "@Async" "fun" IDENT "(" params ")" "->" "Future" "<" type ">" ;
```

---

## JVM Interop Example

```zenith
@JVMExport
class Main {
    static void main() {
        let obj = { msg: "Hello, JVM!" };
        IO.print(obj.msg);
    }
}
```

> To export to Java you must set a `package`. When exported, standalone functions appear as methods in a `functions` class on the Java side.

## Low-Level Control Example

```zenith
unsafe {
    let buf = malloc(1024);
    buf[0] = 42;  // pointer arithmetic
    free(buf);
}
```
