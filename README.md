# SanitizerSymbolizerTool

Fuzzers from the AFL family strive to avoid the step of *symbolize* which turns virtual addresses to file/line locations when working with [Sanitizers](https://github.com/google/sanitizers). Because

> Similarly, include symbolize=0, since without it, AFL++ may have difficulty telling crashes and hangs apart.

(according to [12) Third-party variables set by afl-fuzz & other tools](https://aflplus.plus/docs/env_variables/))

They do this by

> set abort_on_error and symbolize for all the four sanitizer flags

(see [#1618](https://github.com/AFLplusplus/AFLplusplus/discussions/1618) and [#1624](https://github.com/AFLplusplus/AFLplusplus/issues/1624))

However, sometimes we may still want to check symbol names in the report provided by sanitizer when fuzzing, such as utilizing backtrace info as feedback. 
One possibe way to do this is symbolizing addresses and offsets data outside of sanitizer runtime linked in fuzz target, 
i.e., use symbolizer in fuzzer itself when necessary.

*SanitizerSymbolizerTool* helps to implement this. 
We strip `__sanitizer::SymbolizerTool` and related dependencies from [`compiler-rt`](https://github.com/llvm/llvm-project/tree/main/compiler-rt), 
and wrapper them as a standalone library. After introducing it, the fuzzer can use **external** **individual** "tools" that can perform symbolication
by statically analysing target binary (currently, only *llvm-symbolizer*, *atos* and *addr2line* are supported), with a similar style which implemented in sanitizer runtime.

Currently it doesn't support *Windows* platform. Fully migrating compiler-rt across-platform features will be done in future.

*SanitizerSymbolizerTool* is under the Apache License v2.0 with LLVM Exceptions (same as [llvm/llvm-project](https://github.com/llvm/llvm-project)). 
See `LICENSE` for more details.
