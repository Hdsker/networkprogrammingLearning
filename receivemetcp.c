#include<stdio.h>
#include<sys/types.h>//socket
#include<sys/socket.h>//socket
#include<string.h>//memset
#include<stdlib.h>//sizeof
#include<netinet/in.h>//INADDR_ANY
#include <sys/time.h> 
#include <netdb.h>

int childCnt;

/* 
  Calculate the difference between to timevals; store result in an timeval. 
  syntax: a,b,c.
  Result: c=a-b;
*/
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


void *get_in_addr(struct sockaddr *sa){
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


//Convert a struct sockaddr address to a string, IPv4 and IPv6:
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen){
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    s, maxlen);
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                    s, maxlen);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }

    return s;
}

static const char* program_name;


int main(int argc, char *argv[]){
  int listenfd;//to create socket
  int connfd;//to accept connection
  childCnt=0;// Will increment for each spawned child.
 
  struct addrinfo hints, *servinfo, *p;
  int rv;

  struct sockaddr_in serverAddress;//server receive on this address
  struct sockaddr_in clientAddress;//server sends to client on this address
  struct sockaddr_storage their_addr;
  struct timeval GTOD_before, GTOD_after, difference;

  int n;
  char msg[1450];
  char cli[INET6_ADDRSTRLEN];
  char s[INET6_ADDRSTRLEN];
  int clientAddressLength;
  int pid;

  const char* separator = strrchr(argv[0], '/');
  if ( separator ){
    program_name = separator + 1;
  } else {
    program_name = argv[0];
  }

  if (argc != 2) {
    fprintf(stderr,"usage: %s <PORT> \n",argv[0]);
    exit(1);
  }
  int PORT = atoi(argv[1]);
  //create socket
  listenfd=socket(AF_INET,SOCK_STREAM,0);
  //initialize the socket addresses
  memset(&serverAddress,0,sizeof(serverAddress));
  serverAddress.sin_family=AF_INET;
  serverAddress.sin_addr.s_addr=htonl(INADDR_ANY);
  serverAddress.sin_port=htons(PORT);
  
  //bind the socket with the server address and port
  bind(listenfd,(struct sockaddr *)&serverAddress, sizeof(serverAddress));

  //listen for connection from client
  listen(listenfd,5);
  while(1) {
   //parent process waiting to accept a new connection

   clientAddressLength=sizeof(clientAddress);
   connfd=accept(listenfd,(struct sockaddr*)&clientAddress,&clientAddressLength);
   childCnt++;
   
   //child process is created for serving each new clients
   pid=fork();
   if(pid==0)//child process rec and send
     {
       
       gettimeofday(&GTOD_before,NULL);
       int msgLen = 0;

       //rceive from client
       get_ip_str((struct sockaddr*)&clientAddress,&cli,&clientAddressLength);
       

       while(1){
        while((n=recv(connfd,msg,sizeof(msg),0))!=0){
          msgLen += n;
        }
          
         // printf("Child[%d] (%s:%d): recv(%d) .\n", childCnt,cli,ntohs(clientAddress.sin_port),n);
         if(n==0){
           close(connfd);

           gettimeofday(&GTOD_after,NULL); 
           timeval_subtract(&difference,&GTOD_after,&GTOD_before);
           if(msgLen>0){
            printf("%ld|%s:%d",time(0),
           inet_ntop(clientAddress.sin_family,
            get_in_addr((struct sockaddr *)&clientAddress),
            s, sizeof s),ntohs(clientAddress.sin_port));
            if (getnameinfo((struct sockaddr*)&clientAddress, clientAddressLength, cli, sizeof(cli),
                       NULL, 0, NI_NAMEREQD)){
                  printf("(N/A)");
            }else{
                   printf("(%s)", cli);
                 }
           char secTemp[1024] = {0};
           if (difference.tv_sec > 0) {
              sprintf(secTemp, "%.3f",(float)difference.tv_sec+(float)difference.tv_usec*0.000001);
              printf("|%d|%.0f|%s [s]\n",msgLen,msgLen*8/atof(secTemp), secTemp);
            } else {
              sprintf(secTemp, "%.3f",(float)difference.tv_usec*0.000001);
              printf("|%d|%.0f|%s [s]\n",msgLen,msgLen*8/atof(secTemp), secTemp);
            }
           }
           break;
         }
        
     }//close interior while
       
       exit(0);
   }else
    {
      // printf("Parent, close connfd().\n");
      close(connfd);//sock is closed BY PARENT
      if(childCnt>5){
      printf("Being a bad parent.\n");
      sleep(10);
      }
    }
  }//close exterior while
 
  return 0;
}
