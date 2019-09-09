#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "straightline/prog1.h"

int main(int argc, char **argv) {
  int args;
  int test_num;

  assert(argc == 2);
  test_num = atoi(argv[1]);

  switch (test_num) {
    case 0:
      printf("prog\n");
      args = prog()->MaxArgs();
      printf("args: %d\n", args);
      prog()->Interp(nullptr);

      printf("prog_prog\n");
      args = prog_prog()->MaxArgs();
      printf("args: %d\n", args);
      prog_prog()->Interp(nullptr);
      break;
    case 1:
      printf("prog_prog\n");
      args = prog_prog()->MaxArgs();
      printf("args: %d\n", args);
      prog_prog()->Interp(nullptr);

      printf("prog\n");
      args = prog()->MaxArgs();
      printf("args: %d\n", args);
      prog()->Interp(nullptr);
      break;
    default:
      printf("unexpected case\n");
      exit(-1);
  }
  printf("right_prog\n");
  args = right_prog()->MaxArgs();
  printf("args: %d\n", args);
  right_prog()->Interp(nullptr);

  return 0;
}
