#include "apue.h"

#define	WHITE	" \t\n"	/* white space for tokenizing arguments */

/*
 * buf[] contains white-space-separated arguments. We convert it to an
 * argv-style array of pointers, and call the user's function (optfunc)
 * to process the array. We return -1 if there's a problem parsing buf,
 * else we return whatever optfunc() returns. Note that user's buf[]
 * array is modified (nulls placed after each boken).
 */
int buf_args(char *buf, int (*optfunc)(int, char **))
{
	char	*ptr, **argv;
	int	argc, rval;

	if (strtok(buf, WHITE) == NULL)	/* an argv[0] is required */
		return -1;
	if ((argv = malloc(sizeof(char *))) == NULL)
		err_sys("malloc error");
	argv[argc = 0] = buf;
	while ((ptr = strtok(NULL, WHITE)) != NULL) {
		if ((argv = realloc(argv, (++argc + 1) * sizeof(char *))) == NULL)
			err_sys("realloc error");
		argv[argc] = ptr;
	}
	if ((argv = realloc(argv, (++argc + 1) * sizeof(char *))) == NULL)
		err_sys("realloc error");
	argv[argc] = NULL;

	/*
	 * since argv[] pointers point into the user's buf[],
	 * user's function can just copy the pointers, even
	 * though argv[] array will disapper on return.
	 */
	rval = (*optfunc)(argc, argv);
	free(argv);
	return rval;
}
