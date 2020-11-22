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
* D compiler (DMD or LDC2) with DUB build system (usually shipped with compiler)
##### Libraries
* LLVM with Clang (versions 7.0-11.0)
* RapidJSON

*Clang 6 might work as well, but is not supported.

##### Instructions
*Please note that building LLVM alone will take about 45 minutes on my machine (AMD FX-8350, 16GB, SATA-3 SSD), and over 1.5 hours with Clang*
* Build LLVM with Clang
    * Set-up LLVM_DIR environment variable with _/path/to/your_llvm_build_
    * Starting with LLVM 9 it is now also requires to be installed and LLVM_DIR set to /install_dir/lib/cmake/llvm which takes even more disk space, yay \0/
* **OR** you can also try your OS package manager prebuilt binaries *(for example 'libclang-7-dev llvm-7-dev' packages on Ubuntu)*, this can save you hours of waiting and up to 40 GB of disk space
* Copy RapidJSON *include* folder to gentool *include* folder
* Build using CMake!
```
mkdir build
cd build
cmake ../ -DCMAKE_GENERATOR_PLATFORM=x64 -DLLVM_DIR=/path/to/llvm/build
cmake --build . --config Release
```
*`-DCMAKE_GENERATOR_PLATFORM=x64` here is not required, just shows how to quickly generate x64 on Windows*

*`-DGENTOOL_LIB=ON` can be used to build library usable with D main executable, doing this will generate `dubBuild.{sh|bat}` file with set up paths and variables for **building with dub***

**Linux note:**  
    It may fail to link due to lib order, in this case try using lld linker

**Note**  
    Since Clang 9 some Linux package management systems taken a step to use LLVM/Clang as dynamic library, this is not recommended approach, in case if you have issues with it build and use static libs instead. Even though for personal use this might work, you are on your own.

After these steps you will have nice and compact 66 MB executable.

~~Relax and enjoy~~ Embrace the new pain and suffering possibilities when making extern(C)/extern(C++) bindings!