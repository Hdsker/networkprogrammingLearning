#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h> 


/* Subtract the ‘struct timeval’ values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0. */

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y){
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}


int main(int argc, char *argv[]){
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes,numread;
	char buffer[1450];
	
	struct timeval GTOD_before, GTOD_after, difference; 
	
	char** tokens;
	FILE *fptr;
	
	if (argc != 3) {
		fprintf(stderr,"usage: %s <HOSTNAME:PORT> <FILENAME>\n",argv[0]);
		exit(1);
	}
	char delim[]=":";
	char *Desthost=strtok(argv[1],delim);
	char *Destport=strtok(NULL,delim);
	char *filename=argv[2];

	printf("Opening %s sending to %s:%s \n",filename,Desthost,Destport);

//connect
	memset(&hints, 0, sizeof hints);
	memset(&buffer,0, sizeof(buffer));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM; //SOCK_STREAM FOR TCP
	
	if ((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "%s: failed to create socket\n",argv[0]);
		return 2;
	}
	printf("connected,");

	fptr = fopen(filename,"r");
	if (fptr == NULL) {
		fprintf(stderr, "%s: failed to open file.\n",argv[0]);
		close(sockfd);
		exit(0);
	}
	numread=fread(buffer,1,sizeof(buffer),fptr);
	int readTotByte=0;
	int sentTotByte=0;

	gettimeofday(&GTOD_before,NULL);
//transfer data
	while( numread>0){
		if ((numbytes = send(sockfd, buffer, numread, 0)) == -1) {
			perror("talker: sendto");
			exit(1);
		}

		readTotByte+=numread;
		sentTotByte+=numbytes;
		numread=fread(buffer,1,sizeof(buffer),fptr);

	}
	gettimeofday(&GTOD_after,NULL); 
	
	timeval_subtract(&difference,&GTOD_after,&GTOD_before);
	freeaddrinfo(servinfo);

	printf(" sending, Sent %d bytes, in ", sentTotByte);
	if (difference.tv_sec > 0) {
		printf("%ld.%06ld [s]\n", difference.tv_sec, difference.tv_usec);
	} else {
		printf("%6ld [us]\n", difference.tv_usec);
	}
	close(sockfd);

	return 0;
}
