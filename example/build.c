#define BB_MAX_DEPENDENCIES 64
#include "../BuildBuilder.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CMDLEN 1024

void ccm_recurse(unit* u, size_t* len, const char* (*depnames)[BB_MAX_DEPENDENCIES]) {
  size_t i;
  size_t j;
  size_t k;
  const char* fname;
  for(i = 0; u->dependencies[i] != NULL; i++) {
    fname = u->dependencies[i]->outfile;
    /* if it's a .o object file */
    if (strlen(fname) < 2) {
    } else if (fname[strlen(fname)-2] == '.' && fname[strlen(fname)-1] == 'o') {
      for (j = 0; *depnames[j] != NULL; j++);
      *depnames[j] = fname;
    }
    for (k = 0; u->dependencies[i]->dependencies[k] != NULL; k++) {
      ccm_recurse(u->dependencies[i], len, depnames);
    }
  }
}

void c_compile_main(BuildBuilder* bb, unit* u) {
  char* dependencies;
  const char* depnames[BB_MAX_DEPENDENCIES] = {0};
  char cmd[CMDLEN] = {0};
  size_t i;
  size_t len = 0;

  ccm_recurse(u, &len, &depnames);

  /* flatten depnames */
  i = 0;
  while(true) {
    if (depnames[i] == NULL) break;
    len += strlen(depnames[i]);
    i++;
  }
  dependencies = (char*)malloc(len + 2);
  len = 0;
  for (i = 0; depnames[i] != NULL; i++) {
    strcpy(dependencies + len, depnames[i]);
    len += strlen(depnames[i]);
    dependencies[len] = ' ';
    len += 1;
  }

  sprintf(cmd, "clang %s %s %s-o %s", u->flags, u->infile, dependencies, u->outfile);
  BB_runcmd(cmd);
}

void c_compile_o(BuildBuilder* bb, unit* u) {
  char cmd[CMDLEN];

  sprintf(cmd, "clang %s -c %s -o %s", u->flags, u->infile, u->outfile);
  BB_runcmd(cmd);
}

void c_header(BuildBuilder* bb, unit* u) {}

void new_dir(BuildBuilder* bb, unit* u) {
  char cmd[CMDLEN];

  sprintf(cmd, "mkdir %s -p %s", u->flags, u->outfile);
  BB_runcmd(cmd);
}

int main(void) {
  BuildBuilder bb = {0};
  unit* _main; unit* _lib; unit* _lib_h; unit* _outdir;
  const char* cflags = "-std=c89 -Wall -Wextra -Werror -pedantic";

  _main = BB_add_file(&bb, c_compile_main, cflags, "./src/main.c", "./out/main");
  _lib = BB_add_file(&bb, c_compile_o, cflags, "./src/lib.c", "./out/lib.o");
  _lib_h = BB_add_file(&bb, c_header, "", "./src/lib.h", "");
  _outdir = BB_add_file(&bb, new_dir, "", "", "./out/");

  /* main directly uses lib.h */
  BB_add_dependency(_main, _lib_h);
  BB_add_dependency(_main, _outdir);
  /* lib.h declares stuff defined in lib.c */
  BB_add_dependency(_lib_h, _lib);
  BB_add_dependency(_lib_h, _outdir);

  BB_add_dependency(_lib, _outdir);

  BB_build(&bb);
  return 0;
}
