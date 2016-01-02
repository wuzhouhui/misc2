#include "opend.h"

#define NALLOC 10 /* # client struct to alloc/realloc for */

/*
 * alloc more entries in the client[] array
 */
static void client_alloc(void)
{
	int i;

	if (client == NULL)
		client = malloc(NALLOC * sizeof(Client));
	else
		client = realloc(client, (client_size + NALLOC) * sizeof(Client));

	/* initialize the new entries */
	for (i = client_size; i < client_size + NALLOC; i++)
		client[i].fd = -1; /* fd of -1 means entry available */
	
	client_size += NALLOC;
}

/*
 * called by loop() when connection request from a new client arrives.
 */
int client_add(int fd, uid_t uid)
{
	int i;

	if (client == NULL) /* frist time we're called */
		client_alloc();

again:
	for (i = 0; i < client_size; i++) {
		if (client[i].fd == -1) { /* find a available entry */
			client[i].fd	= fd;
			client[i].uid	= uid;
			return i;	/* return index in client[] array */
		}
	}

	/* client array full, time to realloc for more */
	client_alloc();
	goto again; /* and search again (will work this time) */
}

/*
 * called by loop() when we're done with a client.
 */
void client_del(int fd)
{
	int i;

	for (i = 0; i < client_size; i++) {
		if (client[i].fd == fd) {
			client[i].fd = -1;
			return;
		}
	}

	log_quit("can't find client entry for fd %d", fd);
}
