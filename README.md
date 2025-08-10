# BuildBuilder
A simple, customizable, portable build system written in C89

## Usage
The build system works in `unit`s.
Each `unit` has a `step` (arbitrary function that acceps a pointer to the `BuildBuilder` and one to the `unit` itself), some flags (any string), an input file, and an output file.

Create a new `unit` like this:

```c
void c_build_single_file(BuildBuilder* bb, unit* u) {
  /* You can define any arbitrary behaviour for a build step (including having none at all) */
  char cmd[1024];
  sprintf(cmd, "clang %s %s -o %s", u->flags, u->infile, u->outfile);
  BB_runcmd(cmd);
}

/* ... */

BuildBuilder bb;

unit* main_c = BB_add_file(&bb, c_build_single_file, "-Wall -Wextra -Werror", "./src/main.c", "./main");
```

`unit`s can depend on each other. 
These dependencies can be chained, and one `unit` can depend on multiple other `unit`s. 

*NOTE: units directly or indirectly depending on themselves will likely cause an infinite loop, don't do that.*

Create dependencies like this:
```c
unit* main_c = BB_add_file(/*...*/);
unit* foo_h = BB_add_file(/*...*/);
unit* foo_c = BB_add_file(/*...*/);

BB_add_dependency(main_c, foo_h); // main.c directly depends on foo.h
BB_add_dependency(foo_h, foo_c); // foo.h declares functions defined in foo.c
```

Once you're ready in the script to build the project, just run `BB_build(&bb);`

Then compile your script with any c compiler and run it

*NOTE: TCC tends to segfault. I will try to fix this in the future if possible.*

For a full example, check [this file](example/build.c) (also written in c89, which is why it's ugly)
