#ifndef BUILDBUILDER_H
#define BUILDBUILDER_H 1

/* BSD 3-Clause License
Copyright 2025 JazTB

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * This file provides a simple, single-header, customizable build system.
 * It is intended for C projects but may be used for anything.
 * Simply provide your own build.c file and write your build script using this
 * library, and then compile the build script and run it.
 * I reccomend you try to keep your build script as portable as possible
 * to make there be as little compiler friction as possible.
 * (C89 or C99 without extensions)
 *
 * This header should compile with any standard C89 compiler.
 * It has been tested with GCC and Clang. I could not get TCC to work.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

struct unit;
typedef struct {
  void* ptr;
} BuildBuilder;
typedef void (*step)(BuildBuilder*, struct unit*);

typedef struct unit {
  const char* infile;
  const char* outfile;
  const char* flags;
  step step;

  bool built;
  
  struct unit* dependencies[256];
  struct unit* next; /* linkedlist */
} unit;

void BB_build(BuildBuilder* bb);
void BB_build_all_deps(BuildBuilder* bb, unit* u);
void BB_build_unit(BuildBuilder* bb, unit* u);

unit* BB_add_file(
  BuildBuilder* bb,
  step step,
  const char* flags,
  const char* infile,
  const char* outfile
);
void BB_add_dependency(
  unit* dependant,
  unit* dependency
);

void BB_runcmd(const char* cmd);

/* DEFINITIONS */

void BB_build_unit(BuildBuilder* bb, unit* u) {
  if (!u->built) u->step(bb, u);
  u->built = true;
}

void BB_build_all_deps(BuildBuilder* bb, unit* u) {
  size_t i;
  for (i = 0; u->dependencies[i] != NULL; i++) {
    BB_build_all_deps(bb, u->dependencies[i]);
    BB_build_unit(bb, u->dependencies[i]);
  }
}

void BB_deinit(BuildBuilder* bb) {
  unit* ptr = ((unit*)bb->ptr)->next;
  unit* last_ptr = (unit*)bb->ptr;

  while(last_ptr != NULL) {
    free(last_ptr);
    last_ptr = ptr;
    if (ptr != NULL) ptr = ptr->next;
  }
}

void BB_build(BuildBuilder* bb) {
  unit* ptr = (unit*)bb->ptr;

  while (ptr != NULL) {
    BB_build_all_deps(bb, ptr);
    BB_build_unit(bb, ptr);
    ptr = ptr->next;
  }
  /* TODO: cleanup */

  BB_deinit(bb);
}

unit* BB_add_file(
  BuildBuilder* bb,
  step step,
  const char* flags,
  const char* infile,
  const char* outfile
) {
  unit* new = (unit*)malloc(sizeof(unit));
  unit* ptr = (unit*)bb->ptr;
  new->step = step;
  new->flags = flags;
  new->infile = infile;
  new->outfile = outfile;
  new->dependencies[0] = NULL;

  if (bb->ptr == NULL) {
    bb->ptr = (void*)new;
    ptr = (unit*)bb->ptr;
  } else {
    while (ptr->next != NULL) ptr = ptr->next;
    ptr->next = new;
  }

  return new;
}

void BB_add_dependency(
  unit* dependant,
  unit* dependency
) {
  size_t i;
  for (i = 0; dependant->dependencies[i] != NULL; i++);
  dependant->dependencies[i] = dependency;
  dependant->dependencies[i+1] = NULL;
}

void BB_runcmd(const char* cmd) {
  /* Who cares about safety :D */
  fprintf(stderr, "[CMD] %s\n", cmd); fflush(stderr);
  system(cmd);
}

#endif
