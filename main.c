#include <stdio.h>
#include "findmyso.h"
int main (int argc, char *argv[])
{
  if (argc <= 1)
  {
    printf("Expected colon-delimited .so list. Example: main libmariadb.so:libmariadbclient.so:libmysqlclient.so:libtarantool.so:libcrypto.so\n");
    return 1;
  }
  char buffer[65536]; /* Must be big enough! */ 
  int rval= findmyso(argv[1], buffer, 65536, argv[2]);
  printf("%s\n", buffer);
  printf("rval=%d\n", rval);
  return 0;
}
