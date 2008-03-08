#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

int main(int argc, char** argv)
{
  if( argc!=3 )
    {
      fprintf(stderr, "Usage: %s DATABASE SQL-STATEMENT\n", argv[0]);
      return 1;
    } 
  else 
    {
      sqlite3* db;

      if( sqlite3_open(argv[1], &db) )
        {
          fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
          sqlite3_close(db);
          return 1;
        }
      else
        {
          char* zErrMsg = 0;
          if( sqlite3_exec(db, argv[2], callback, 0, &zErrMsg) != SQLITE_OK )
            {
              fprintf(stderr, "SQL error: %s\n", zErrMsg);
              sqlite3_free(zErrMsg);
            }

          sqlite3_close(db);
          return 0;
        }
    }
}

/* EOF */
