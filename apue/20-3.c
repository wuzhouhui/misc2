/*
 * =====================================================================================
 *
 *       Filename:  20-3.c
 *
 *    Description:  construct a database and write 3 recored
 *
 *        Version:  1.0
 *        Created:  01/16/2015 04:05:51 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "apue.h"
#include "apue_db.h"
#include <fcntl.h>

int main(void)
{
	DBHANDLE	db;

	if ((db = db_open("db4", O_RDWR | O_CREAT | O_TRUNC, FILE_MODE))
			== NULL)
		err_sys("db_open error");

	if (db_store(db, "Alpha", "data1", DB_INSERT) != 0)
		err_quit("db_store error for alpha");
	if (db_store(db, "beta", "Data for beta", DB_INSERT) != 0)
		err_quit("db_store error for beta");
	if (db_store(db, "gamma", "record", DB_INSERT) != 0)
		err_quit("db_store error for gamma");

	db_close(db);
	exit(0);
}
