#include <stdio.h>
#include "pgfindlib.h"
int main (int argc, char *argv[])
{
  if (argc <= 1)
  {
    printf("Expected statement. Example: main 'where libmariadb.so,libmariadbclient.so,libmysqlclient.so,libtarantool.so,libcrypto.so\n");
    return 1;
  }
  char buffer[65536]; /* Must be big enough! */ 
  int rval= pgfindlib(argv[1], buffer, 65536);
  printf("%s\n", buffer);
  printf("rval=%d\n", rval);
  return 0;
}
