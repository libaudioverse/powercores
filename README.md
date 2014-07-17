lambdatask
==========

Automatically parallelize tasks with dependencies.  This library uses only C++11 features, and thus runs anywhere C++11 runs.  Also includes some miscellaneous concurrent containers.

Note: This is definitely a work in progress and will develop alongside [Libaudioverse](http://github.com/camlorn/libaudioverse).  unlike Libaudioverse, it is released under the unlicense, so you can use it or modify it for any reason and any purpose.  See the LICENSE file for specifics.

the purpose of this library is to provide some higher level primitives needed for [Libaudioverse](http://github.com/camlorn/libaudioverse).  While C++11 provides some lower-level features, the higher level abstractions are missing.  This library aims to fill that gap.

The most useful feature here is the task parallelizer.  A problem that recurs often and one which must be solved for [Libaudioverse](http://github.com/camlorn/libaudioverse) is stated as follows.  Given N tasks and a list of dependencies between them, automatically determine an efficient way to run them across T threads.  To provide a concrete example, Libaudioverse's parent-child relationships already form a dependency graph, which can be translated into this library's format as a "plan", and then executed.

the name comes from the fact that it is easiest to represent tasks as lambdas and that the entire point of this library is being C++11.  Technically, anything with `operator()` will function as a task.

Finally, as a special case, running across 0 threads is supported.  This will run tasks in the executing thread.  This is the desired mode of operation for devices without many cores: the overhead of creating and swapping the threads in this case will dwarf any gain of making them in the first place.