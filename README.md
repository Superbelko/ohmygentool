# GenTool 
Lets you automate your average C/C++ binding creation process!

Now with D language target support!

Just as you expect, it takes your C/C++ headers and writes extern(C)/extern(C++) bindings!

---
#### What it is?
Merely a ~~simple script~~ tool for parsing C/C++ source files based on clang compiler. In short - it searches for top level declarations and converts them to D usable form.

##### Features
* JSON based config files
* [C] Can do stuff
* [C++] Can do stuff, just not any stuff
* [C++] Also writes inline functions bodies for you
* [C++] (Experimental) Operator overloading support
* (Experimental) Extracts simple macros
* Basic bitfields support
* Does de-anonimizing for structs and unions for D target

##### Limitations
* MS C++ compiler handles "someType * const" (const pointer-to-non const) function parameters differently, requires mangling override, planned
* Operator precedence order is not converted, one should validate the formulas after conversion
* C++ move and copy constructors are not handled properly yet
* Multiple inheritance is not handled yet
* "virtual" detection to filter struct vs class usage is not smart enough
* Does limited inline functions conversion
* Preprocessor stuff are completely invisible to this tool *(see[1])*, what this means in practice - you will see a lot of garbage from expanded macro, such as ```(void(0));```
* Any non-trivial code will require manual adjustments, and sometimes even trivial code as well
* Not production ready, and doubtly will ever be
* Just too much 'yet'
* This list may grow over time
* Does not makes coffee

[1] - *It operates on AST level, which is done AFTER preprocessing stage, and it seems there is no way to modify such stuff without writting temp copy and mix these two passes.*

### **Note:**
This tool is just a personal script for automating my daily binding making process. It may contain bugs, vulnerabilities, security holes, or holes. Use of this software may cause cancer *(whatever this means)*, and/or your hamster's death, if you does not have a hamster it will summon one from another dimension and then kill it. 

USE ON YOUR OWN RISK!

**AT THIS POINT I AM NOT READY TO ACCEPT PULL REQUESTS, COME BACK LATER**

---
    
## Building
### Requirements:
##### Tools
* C++17 compliant compiler
* CMake (3.10+ on Windows)
##### Libraries
* LLVM (7.0 recommended)
* Clang (7.0 recommended)
* RapidJSON

LLVM 7 / Clang 7 recommended, but Clang 6 might work as well.

##### Instructions
*Please note that building LLVM alone will take about 45 minutes on my machine, and over 1.5 hours with Clang*
* Build LLVM, preferably Debug configuration (might require configuring as static library target on non-Windows)
    * Set-up LLVM_DIR environment variable with path to _(your_llvm_build)     /lib/cmake/llvm_
* **OR** just try your OS package manager prebuilt binaries *(for example 'libclang-7-dev' package on Ubuntu)*, this saves you hours of time and over 40 GB
disk space
* Copy RapidJSON *include* folder to gentool *include* folder
* Build!
```
mkdir whatever-dir
cd whatever-dir
cmake ../ -DCMAKE_GENERATOR_PLATFORM=x64
cmake --build . --config Debug
```
*-DCMAKE_GENERATOR_PLATFORM=x64 here is not required, just shows how to quickly generate x64 on Windows*

After these steps you will have nice and compact 66 MB executable.

Relax and enjoy the new way of extern(C)/extern(C++) making!