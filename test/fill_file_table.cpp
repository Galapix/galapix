#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

int main(int argc, char** argv)
{
  if( argc < 2 )
    {
      fprintf(stderr, "Usage: %s add    FILE...\n", argv[0]);
      fprintf(stderr, "       %s remove FILE...\n", argv[0]);
      fprintf(stderr, "       %s list\n", argv[0]);
      fprintf(stderr, "       %s find   GLOBPATTERN...\n", argv[0]);
      return 1;
    } 
  else 
    {
      sqlite3* db;

      if (sqlite3_open("test.db", &db))
        {
          fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
          sqlite3_close(db);
          return 1;
        }
      else
        {
          printf("Evaluating command: %s\n", argv[1]);
          if (strcmp(argv[1], "add") == 0)
            {
              sqlite3_stmt* stmt;
              if (sqlite3_prepare_v2(db,
                                     "INSERT INTO files (filename, md5, filesize, width, height, mtime)"
                                     "           VALUES (      ?1,  ?2,       ?3,    ?4,     ?5,    ?6);", -1, &stmt,  0)
                  != SQLITE_OK)
                {
                  fprintf(stderr, "Something went wrong2: %s\n", sqlite3_errmsg(db));                  
                }
              else
                {                           
                  for(int i = 2; i < argc; ++i)
                    {
                      sqlite3_bind_blob(stmt, 1, argv[i], strlen(argv[i]), SQLITE_STATIC); // filename
                      sqlite3_bind_text(stmt, 2, "einszweidreitest", -1, SQLITE_STATIC); // md5
                      sqlite3_bind_int (stmt, 3, 0); // filesize
                      sqlite3_bind_null(stmt, 4);    // width
                      sqlite3_bind_null(stmt, 5);    // height
                      sqlite3_bind_int (stmt, 6, 0); // mtime

                      // execute
                      int rc = sqlite3_step(stmt);
                      if (rc != SQLITE_DONE)
                        {
                          fprintf(stderr, "Something went wrong: %s\n", sqlite3_errmsg(db));
                        }
                      else
                        {
                          printf("added %s\n", argv[i]);
                        }

                    }
                  sqlite3_reset(stmt);
                }

              sqlite3_finalize(stmt);
            }
          else if (strcmp(argv[1], "remove") == 0)
            {
              for(int i = 2; i < argc; ++i)
                {
                  
                }
            }
          else if (strcmp(argv[1], "find") == 0) 
            {
              for(int i = 2; i < argc; ++i)
                {
                  //                   if (rc == SQLITE_ROW)
                  //                     {
                  //                       // Column access functions
                  //                       int sqlite3_column_int(stmt, iCol);
                  //                       const void* sqlite3_column_blob(stmt, iCol);

                  //                       sqlite3_step(stmt);
                  //                     }                  
                  //                   else
                  //                     {

                  //                     }
                  
                }
            }
          else if (strcmp(argv[1], "list") == 0) 
            {
              sqlite3_stmt* stmt;
              if (sqlite3_prepare_v2(db, "SELECT filename,md5 from files;", -1, &stmt, 0) != SQLITE_OK)
                {
                  fprintf(stderr, "Something went wrong2: %s\n", sqlite3_errmsg(db));                  
                }
              else
                {    
                  int rc = sqlite3_step(stmt);
                  while (rc == SQLITE_ROW)
                    {
                      const void* filename     = sqlite3_column_blob(stmt, 0);
                      const unsigned char* md5 = sqlite3_column_text(stmt, 1);
                     
                     std::cout << "Column: "
                               << filename
                               << sqlite3_column_bytes(stmt, 0) << " " 
                               << sqlite3_column_bytes(stmt, 1) << std::endl;

                     rc = sqlite3_step(stmt);
                    }
                  
                  if (rc != SQLITE_DONE)
                    {
                      fprintf(stderr, "Something went wrong3: %s\n", sqlite3_errmsg(db));
                    }
                }

              sqlite3_finalize(stmt);
            }
          else
            {
              fprintf(stderr, "Unknown command '%s'", argv[1]);
              sqlite3_close(db);
              return 1;
            }

          sqlite3_close(db);
          return 0;
        }
    }
}

/* EOF */

