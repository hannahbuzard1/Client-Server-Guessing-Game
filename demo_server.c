/* demo_server.c - code for example server program that uses TCP */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define QLEN 6 /* size of request queue */

/*------------------------------------------------------------------------
* Program: demo_server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for the next connection from a client
* (2) fork and respond to client guesses
* (3) close the connection
* (4) go back to step (1)
*
* Syntax: ./demo_server server_port secret_number
*
* server_port - protocol port number to use
* secret_number - number for clients to guess.
*------------------------------------------------------------------------
*/

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd, sd2; /* socket descriptors */
	int port; /* protocol port number */
	int alen; /* length of address */
	int optval = 1; /* boolean value when we set socket option */
	uint32_t secret_number; /* number for clients to guess */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port secret_number\n");
		exit(EXIT_FAILURE);
	}

	secret_number = strtoul(argv[2], 0, 10);

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	port = atoi(argv[1]); /* convert argument to binary */
	if (port > 0) { /* test for illegal value */
		sad.sin_port = htons((u_short)port);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(sd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	/* Main server loop - accept and handle requests */
	while (1) {
		alen = sizeof(cad);
		if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) { //wait for connection
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}

		fprintf(stdout, "Successfully connected new client, forking...\n");

		int pid = fork();
		if(pid == 0) {
			//get guess from client
			int clientreply = 0;
			char reply[1000];
			clientreply = recv(sd, reply, 1000,0);
			//compare client's guess to actual number
			int clientnumber1 = atoi(reply);
			uint32_t clientnumber = clientnumber1;
			char buf[1000]; // buffer for string the server sends
			if(clientnumber < secret_number) {
				sprintf(buf, "%s", "-1");
			} else if (clientnumber > secret_number) {
				sprintf(buf, "%s", "1");
			} else {
				sprintf(buf, "%s", "0");
			}
			send(sd2,buf,strlen(buf),0);
			//close connection
			close(sd2);
			exit(EXIT_SUCCESS);

		} else {
			/* parent process */
			close(sd2); //close connection because fork unsuccessful
		}
	}
}