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
 * I recommend you try to keep your build script as portable as possible
 * to make there be as little compiler friction as possible.
 * (C89 or C99 without extensions)
 *
 * This header should compile with any standard C89 compiler.
 * It has been tested with TCC, GCC, and Clang.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#ifndef BB_NOT_STATIC
#define __BB_STATIC static
#else
#define __BB_STATIC
#endif

struct unit;
typedef struct {
  void* ptr;
} BuildBuilder;
typedef void (*step)(BuildBuilder*, struct unit*);

#ifndef BB_MAX_DEPENDENCIES
#define BB_MAX_DEPENDENCIES 256
#endif
typedef struct unit {
  const char* infile;
  const char* outfile;
  const char* flags;
  step step;

  int /*bool*/ built;
  
  struct unit* dependencies[BB_MAX_DEPENDENCIES];
  struct unit* next; /* linkedlist */
} unit;

__BB_STATIC void BB_build_unit(BuildBuilder* bb, unit* u) {
  if (!u->built) u->step(bb, u);
  u->built = true;
}

__BB_STATIC void BB_build_all_deps(BuildBuilder* bb, unit* u) {
  size_t i;

  for (i = 0; u->dependencies[i] != NULL; i++) {
    BB_build_all_deps(bb, u->dependencies[i]);
    BB_build_unit(bb, u->dependencies[i]);
  }
}

__BB_STATIC void BB_deinit(BuildBuilder* bb) {
  unit* ptr = ((unit*)bb->ptr)->next;
  unit* last_ptr = (unit*)bb->ptr;

  while(last_ptr != NULL) {
    free(last_ptr);
    last_ptr = ptr;
    if (ptr != NULL) ptr = ptr->next;
  }
}

__BB_STATIC void BB_build(BuildBuilder* bb) {
  unit* ptr = (unit*)bb->ptr;

  while (ptr != NULL) {
    BB_build_all_deps(bb, ptr);
    BB_build_unit(bb, ptr);
    ptr = ptr->next;
  }
  /* TODO: cleanup */

  BB_deinit(bb);
}

__BB_STATIC unit* BB_add_file(
  BuildBuilder* bb,
  step step,
  const char* flags,
  const char* infile,
  const char* outfile
) {
  unit* newu = (unit*)malloc(sizeof(unit));
  unit* ptr = (unit*)bb->ptr;

  if (newu == NULL) {
    fprintf(stderr, "[ERROR] Failed to allocate memory\n"); fflush(stderr);
    exit(EXIT_FAILURE);
  }
  newu->next = NULL;
  newu->built = false;
  newu->step = step;
  newu->flags = flags;
  newu->infile = infile;
  newu->outfile = outfile;
  newu->dependencies[0] = NULL;

  if (bb->ptr == NULL) {
    bb->ptr = (void*)newu;
    ptr = (unit*)bb->ptr;
  } else {
    while (ptr->next != NULL) ptr = ptr->next;
    ptr->next = newu;
  }

  return newu;
}

__BB_STATIC void BB_add_dependency(
  unit* dependant,
  unit* dependency
) {
  size_t i;

  for (i = 0; dependant->dependencies[i] != NULL; i++);
  dependant->dependencies[i] = dependency;
  dependant->dependencies[i+1] = NULL;
}

__BB_STATIC void BB_runcmd(const char* cmd) {
  int err;

  fprintf(stderr, "[CMD] %s\n", cmd); fflush(stderr);
  err = system(cmd);
  if (err != 0) {
    fprintf(stderr, "[ERROR] Command failed\n"); fflush(stderr);
    exit(err);
  }
}

/* self-rebuilding */

#define ARGS_SIZE 64
#define CMD_SIZE 1024

#define BUILD_C_NAME "./BuildBuilder.c"
#define BUILD_EXE_NAME "./BuildBuilder"

__BB_STATIC void __BB_rebuild(void) {
  char cmd[CMD_SIZE] = {0};
  char cccmd[CMD_SIZE] = {0};
  char* args[ARGS_SIZE] = {NULL};
  int e;

  #ifdef BB_DEBUG
  fprintf(stderr, "[BB_DEBUG] Rebuilding self\n");
  fflush(stderr);
  #endif
  sprintf(cccmd, "cc -o %s %s -std=c89 -Wall -Wextra -Werror -pedantic", BUILD_EXE_NAME, BUILD_C_NAME);
  e = system(cccmd);
  if (e) {
    fputs("Failed to compile\n", stderr);
    fflush(stderr);
    exit(1);
  }

  #ifdef BB_DEBUG
  fprintf(stderr, "[BB_DEBUG] Calling new version of self\n");
  fflush(stderr);
  #endif

  fflush(stdout);

  strcpy(cmd, BUILD_EXE_NAME);
  args[0] = cmd;
  args[1] = NULL;
  execvp(cmd, args);

  perror("[ERROR] execvp() failed");
  exit(1);
}

__BB_STATIC void BB_rebuild(void) {
  struct stat s;
  struct stat s_c;
  long time = 0;
  long time_c;
  int should_rebuild = false;
  FILE* f;

  f = fopen(BUILD_C_NAME, "r");
  if (f == NULL) {
  #ifdef BB_DEBUG
  fprintf(stderr, "[BB_DEBUG] Build script named `%s` isn't found, skipping rebuild.\n", BUILD_C_NAME);
  fflush(stderr);
  #endif
    return;
  }
  fclose(f);

  if (stat(BUILD_C_NAME,&s_c) != 0) {
    perror("[ERROR] Failed to stat build source file");
    exit(EXIT_FAILURE);
  }
  time_c = s_c.st_mtime;
  if (stat(BUILD_EXE_NAME, &s) != 0) {
    should_rebuild = true;
  } else {
    time = s.st_mtime;
  }

  if (time_c > time) should_rebuild = true;

  if (should_rebuild) {
    __BB_rebuild();
  }
}


#undef __BB_STATIC
#undef BB_NOT_STATIC

#endif
