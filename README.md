# LLVMAndersenAnalysisPass
Copy the folder into llvm-src/lib/Transforms.
Then run 'make' command for compiling the pass into a shared library. 

For running the pass, use the command

opt -load LLVMAndersenPA.so -andpa hello.bc -o hello.out

The output will be of the format:

pointername pointee1 pointee2 pointee3 ... (one pointer per line.)

There will be many temporary pointers also in the output. These are llvm temporary values.

