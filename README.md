# **UNFINISHED**


# Language Specification: Zenith

A hybrid programming language combining JVM interoperability, low-level control, and dynamic scripting.

## Core Features

### 1. Memory Management
- **Free Objects**  
  - `let obj = {}` → Inferred as `freeobj` (dynamic properties).  
  - Optimized via hidden classes/static access.  
- **Manual Memory**  
  - `unsafe { let ptr = malloc(size); free(ptr); }`  
  - `scope` blocks for auto-free.  
- **Garbage Collection** (Optional)  
  - `@GC("generational")` (default for JVM).  
  - `@GC("refcounting")` or `@GC("none")`.

### 2. Types
- **Static Types**: `int`, `long`, `struct`, etc.  
- **Dynamic Types**: `dynamic` or inferred (`let x = 10`).  
- **First-Class Objects**: Everything is an object.  

### 3. OOP & Structs
- **Classes**:  
  ```
  class Example { 
    privatew int x; // Write-restricted 
    protectedw int y; 
  }
  ```
- **Structs/Unions**:
    ```
    struct Point { int x, y; }
    union Data { int i; float f; } //Can't contain dynamic variables
    ```

### 4. Concurrency

- **Async/Await**:
    ```
    @Async fun fetch() -> Future<String> { 
      let data = await http.get(url); 
    }
    ```
- **Actors**: Isolated state with message-passing.

### 5. Error Handling

- **Dual Model**:

  - ```Result<T, E>``` (Rust-like).

  - ```@Throws``` for JVM exceptions.

### 6. FFI (Foreign Function Interface)

- C/JVM/Wasm:
```extern "C" { fn printf(format: *const char, ...); }```

### 7. Metaprogramming

- Decorators: @@Log (applied at compile-time).

- Annotations: @Route("/path") (runtime metadata).

### 8. Syntax Sugar
- Braces: Configurable (--braces=optional).

- String Interpolation: ```\`"Hello, {name}!"\````.

## Unique Features
| Feature            | Example              | Notes                           |
| ------------------ | -------------------- | ------------------------------- |
| Write Restrictions | privatew int x;      | Class-only write, external read |
| Hoisting           | hoist var x = 10;    | Like JavaScript.                |
| Dual GC            | @GC("none")          | Manual + GC modes coexist.      |
| Free Objects       | let obj = { x: 10 }; | Optimized as structs if static. |

## Compiler Optimizations

1. Hidden Classes: Speed up property access.

2. Escape Analysis: Stack-allocate free objects.

3. Inline Caching: Cache frequent property lookups.

## Grammar Highlights (EBNF)
```ebnf
freeobj      = "{" [ IDENT ":" expr { "," IDENT ":" expr } ] "}" ;
unsafe_block = "unsafe" "{" { stmt } "}" ;
async_func   = "@Async" "fun" IDENT "(" params ")" "->" "Future" "<" type ">" ;
```

## Examples
 **JVM Interop**
```
@JVMExport or @@JVMExport
class Main {
  static void main() {
    let obj = { msg: "Hello, JVM!" };
    print(obj.msg);
  }
}
```
- **!! To export to Java you must have a `package` set. You can export to java even if you are trying to export a normal function. From Java's side the function would be a method in the `functions` class**

 **Low-Level Control**
```
unsafe {
  let buf = malloc(1024);
  buf[0] = 42; // Pointer arithmetic
}
```
## Tooling

- Compiler Flags:
 --target=jvm|native|wasm
 --braces=required|optional

- Package Manager: zenith-pkg (WIP). RIP if you think this will exist

## Roadmap

- JVM target.

- WASM support. (NOT PLANNED YET)

- IDE plugins (VS Code/LSP).


# Specific language specification

### Keywords
- ```let``` - Declares a dynamic variable
- ```var``` - Declares a dynamic variable, same as let
- ```dyanmic``` - Marks a variable as dynamic, alternative to `let` and `var`
- ```fun``` - Declares a dynamic function, can be used with static return type functions too
- ```hoist``` - Hoists a variable to the top of it's scope. Defines at the normal point
- `unsigned` - Makes a number variable unsigned
- `signed` - Makes a number variable signed
- ```class``` - Makes a class
- ```struct``` - Makes a struct
- ```unsafe``` - Creates a block where you can use unsafe functions
- ```new``` - Optional new keyword, used when creating objects


**Class Scopes**  
- ```public``` - Class public scope  
- ```protected``` - Class protected scope  
- ```private``` - Class private scope  
- ```privatew``` - Class privatew scope (only private scope can write, everyone can read)  
- ```protectedw``` - Class protectedw scope (only protected/private scope can write, everyone can read

**Imports**
- ```import``` - Imports a file
- ```import java``` - Imports a java class(must be compiled to JVM or have a JVM it can refrence to)
- ```extern "C"``` - Creates a block where you can declare C functions, must be in an unsafe block
- ```package``` - Sets a package, needed when compiling to JVM or integrating with java

### Types

**<span style="font-size: 1.1em;">Built-in types</span>**

**<span style="font-size: 0.95em; margin-left: 0.5em; display: block; ">Number Types</span>**  
- `short` - 2 byte integer  
- `int` - 4 byte integer  
- `long` - 8 byte integer  
- `byte` - 1 byte integer  
- `float` - 4 byte decimal number
- `double` -  8 byte decimal nunber

**<span style="font-size: 0.95em; margin-left: 0.5em; display: block; ">Other primitive Types</span>**
- `string` - A string !! Not a `class`
- `freeobj` -  A javascript like object that can store data, functions, etc. If it is being used like a `struct` it will be compiled as if it is a `struct`. There are a couple of ways to use it 
- `Number` - A JS like number that supports decimals and whole numbers up to 8 bytes
- `BigInt` - A JS like dynamically allocated integer, up to 32 bytes
- `BigNumber` - A dynamically allocated number that can support numbers up to 32 bytes
```
//Easiest way
let dynamic = {
    name: "Zenith",
    version: 1.0f,
    getInfo: () => "${this.name} v${this.version}"
}
//Redundant freeobj way
let dynamic = freeobj {
    name: "Zenith",
    version: 1.0f,
    getInfo: () => "${this.name} v${this.version}"
}
//Type strict way
freeobj dynamic = freeobj { //second "freeobj" is not reqired here, it is optional
    name: "Zenith",
    version: 1.0f,
    getInfo: () => `"${this.name} v${this.version}"`
}
//Edge cases
struct somestruct{...}
let whatever = {} //will be a free obj
somestruct name = {} //will be a struct
somestruct nicename = freeobj {} //Type conversion error
```
**<span style="font-size: 1.1em;">Built-in `class` types</span>**

- `IO` - simple console and file IO
- `Result<T,E>` - for Rust-like error handling
- Some other ones I forgot

### Blocks
**Functions**
- Dynamic function(returns anything)
```
fun coolname(var dynamicarg1, long staticarg1, dynamicarg2){
    if(dynamicarg1==5) return true //requires the braces requirement to be OFF
    if(staticarg1==900) {return 9}
    if(dynamicarg2=="cats are good") {return new Cat("Mqucho")}
    return new Cat("Murcho")
}
```
- Static Function - returns a specific type
```
fun Cat catfactory(var dynamicarg1, long staticarg1, dynamicarg2){ // Keyword fun is not required
    if(dynamicarg1==5) return new Cat("bob ross") //requires the braces requirement to be OFF
    if(staticarg1==900) {return new Cat("snowball")}
    if(dynamicarg2=="cats are good") {return new Cat("Mqucho")}
    return new Cat("Murcho")
}
```
**Loops**
- `for` loop
```
for(let i=0;i<123;i++){
    //tell snowball something
}
int arr[1024] = ...
for(int i : arr){
    //Log i
}
```
- `while` loop
```
while(somecondition){
    //feed snowball, bob ross, Mqucho and Murcho
}
```
- `do-while` loop
```
do {
  // code block to be executed
}
while (condition)
```
**If statements**
```
if(condition){
    //do this
}else{
    //do that
}
if(condition){
    //do 1
}else if(condition){
    //do 2
}elif(condition){
    //do3
}
```
**Objects**

**<span style="font-size: 0.95em; margin-left: 0.5em; display: block; ">Classes</span>**

```
class Cat{
    privatew string name;
    privatew double age;
    public void pet(){
        IO.print(`"${this.name} feels very Loved!"`)
    }
    public void age(double age){
        if(age>0){
            this.age+=age
        }
    }

    public Cat(string name, Gender gender){
        this.name = name
        this.age = 0
    }

}
```
**<span style="font-size: 0.95em; margin-left: 0.5em; display: block; ">Structs</span>**
```
struct Vector3{
    int x;
    int y;
    int z;
}
```
**<span style="font-size: 0.95em; margin-left: 0.5em; display: block; ">Union</span>**
```
union Pet{
    Cat;
    Dog;
}
```
### Metaprogramming - Annotations, Decorators
- **Annotations** can be added to anything and hold some extra info, work the same as Java Annotations can hold anything, they give runtime metadata(in some cases with the built in ones it's compile time)
```
@Breedable
class Cat{...}

@Register(id="some_registry_entry",registry="block")
class Cube{...}

@NotNull
let somevarthatcantbenull = 1

@IsAGreatFunction
fun givesGreatResponses{...}
```
- **Decorators** - can be added to functions/methods and function as a wrapper before executing the main code. Built-in annotations have equivalent decorators
```
@@GC("refcounting") // Same as @GC("refcounting")
int a = 5
@@NotNull //Same as @NotNull
int a = 219
```
#### Creating
**Annotations**
```
annotation ComesFromNet
@ComesFromNet
/************************/
annotation Name{
    string value;
}
@Name("John Cena")
/************************/
annotation Register{
    string registry;
    string id;
}
@Name(id="actor",registry="con")
/************************/
annotation Register{
    string registry;
    Cat mother;
}
@Name(mother=new Cat("pissssi"),registry="con")
```
**Decorators**

//not sure yet

### Object-Oriented Programming (OOP) in Depth
**Classes**

Classes are the foundation of OOP, encapsulating data (fields) and behavior (methods). They support:

- Inheritance (single inheritance only, no multiple inheritance)

- Operator overloading (though not shown in this first example)

- Automatic getters/setters (though not shown in this first example)

- Constructors, destructors, and method overriding

Key Modifiers

- `const` – Makes a field immutable (must be initialized at declaration or in the constructor).
- ...

```
class Dog {
    // Fields with restricted write access
    privatew double age;         // Writable only in Dog
    protectedw string breed;    // Writable in Dog and subclasses
    const public string name;  // Immutable, readable everywhere

    public Dog(string name, string breed) : name(name) {
        this.age = 0;           
        this.breed = breed;     
    }

    public void describe() {
        IO.print(`"${this.name} is a ${this.breed} aged ${this.age}."`);
        this.age = 5
    }

    // Protected method (can modify protected fields)
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

// Example Usage
fun main() {
    Dog buddy = new Dog("Buddy", "Labrador");
    buddy.describe();  // "Buddy is a Labrador aged 0."

    // Can READ but NOT WRITE privatew/protectedw from public scope:
    // buddy.age = 5;   // ERROR: privatew write outside class!
    // buddy.breed = "Poodle"; // ERROR: protectedw write outside hierarchy!

    Mallinois rex = new Mallinois("Rex");
    rex.train();       // "Rex is now a Trained Mallinois!"
    rex.describe();    // "Rex is a Trained Mallinois aged 0."
}
```

```
class Vector3D {
    // Private fields (readable everywhere, writable only in Vector3D)
    privatew double x;
    privatew double y;
    privatew double z;

    // Constructor
    public Vector3D(double x, double y, double z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    // --- Setters (with validation) ---
    public void setter(x) setX(double x) { 
        if (!x.isNaN()) {  // Example validation
            this.x = x; 
        }
    }
    // (Similar for setY/setZ...)

    // --- Operator Overloading ---
    // Vector addition (+)
    public Vector3D operator "+"(Vector3D other) {
        return new Vector3D(
            this.x + other.x,
            this.y + other.y,
            this.z + other.z
        );
    }

    // Scalar multiplication (*)
    public Vector3D operator "*"(double scalar) {
        return new Vector3D(
            this.x * scalar,
            this.y * scalar,
            this.z * scalar
        );
    }

    // Equality check (==)
    public bool operator "=="(Vector3D other) {
        return this.x == other.x 
            && this.y == other.y 
            && this.z == other.z;
    }

    // --- String representation ---
    public string toString() {
        return `"(${this.x}, ${this.y}, ${this.z})"`;
    }
}

// Example Usage
fun main() {
    Vector3D v1 = new Vector3D(1.0, 2.0, 3.0);
    Vector3D v2 = new Vector3D(4.0, 5.0, 6.0);

    // Operator overloading
    Vector3D sum = v1 + v2;               // (5.0, 7.0, 9.0)
    Vector3D scaled = v1 * 2.0;           // (2.0, 4.0, 6.0)
    bool isEqual = (v1 == v2);            // false

    // Getters/setters
    IO.print(v1.getX());                   // 1.0
    v1.x = 10.0;                          // Valid write ↓
    // v1.x = 10.0;                      // Usually would error, but will automatically use setter, ERROR: privatew write outside class!

    IO.print(sum.toString());             // "(5.0, 7.0, 9.0)"
}
```
**Structs**
- Structs aren't the same as classes
- They still support Inheritance, Constructors, destructors, and method overriding, operator overloading. They don't have automatic getters and setters. They should be used in simpler data structures //Not sure about the getters and setters might remove that req
```
 struct Vector3{
     public int x;
     public int y;
     public int z;
     //By default they don't have a constuctor
 }
 Vector3 a = {1,2,4}
 Vector b = {
     a: 1,
     b: 2,
     c: 4
 }
 Vector c = {
     .a = 1,
     .b = 2,
     .c = 4
 }
```

**Union**
- Unions are the same as in C++
- A union is a special data type that lets you store different types of data in the same memory space, but only one at a time
Key Idea:
- Unlike a struct (where each field has its own memory), a union shares the same memory for all its fields
- Changing one field overwrites the others
- Size = largest member’s size (since memory is shared)
- Do not directly support dynamic variables
- If they contain an object that contains dynamic variables they will be ignored (in size)

```
union arbnum {
    int,
    float
}
```
