/*Take in a port number(ports > 5000)
Wait for UDP connection after binding to requested port 
Respond to clients request 
    get - transmit requested file to the client 
    put - receives transmitted file from client and stores locally 
    delete - delete file if it exists 
    ls - search all files in current working dir and send a list to client 
    exit - acknowledge that the client is done with a goodbye message 
    other - respond that command was not understood 
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  //struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
    char cmd[BUFSIZE]; /*users command*/
    char file[BUFSIZE];/*users desired file name*/
    char fileBuf[BUFSIZE];/*buffer to read in files*/

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);
    //Check valid port numbers 
    if(portno < 5001 || portno > 65535){
        error("Port out of range. Must be between 5000 and 65535");
    }
  

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    /*
     hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    */
   
   sscanf(buf, "%s %s", cmd, file);

   //if/else if tree to process users command 
        if (strcmp(cmd, "get") == 0){
            /*Check to see if file exists*/
            FILE* f = fopen(file, "rb");
            if(f == NULL){
                printf("File Not Found Error\n");
                continue;
            }

            int size;
            bzero(fileBuf, sizeof(fileBuf));

            while(!feof(f)){
              size = fread(buf, 1, BUFSIZE, f);
              //Send packet size to client 
              n = sendto(sockfd, &size, sizeof(int), 0, (struct sockaddr *) &clientaddr, clientlen);
              if (n < 0) 
                error("ERROR in sendto");
              //Send data to client
              n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
              if (n < 0) 
                error("ERROR in sendto");
            }
            fclose(f);
            //Tell client we are done sending data
            size = -1;
            n = sendto(sockfd, &size, sizeof(int), 0, (struct sockaddr *) &clientaddr, clientlen);
              if (n < 0) 
                error("ERROR in sendto");
            
        }
        else if (strcmp(cmd, "put") == 0){
          int size = 0;
          FILE *f = fopen(file, "w");

          while(1){
            //Get packet size
            n = recvfrom(sockfd, &size, sizeof(int), 0, (struct sockaddr *) &clientaddr, &clientlen);
            if (n < 0)
              error("ERROR in recvfrom");
            else if(size == -1){
              break;
            }

            bzero(buf, BUFSIZE);
            //Get transmitted data
            n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
            if (n < 0)
              error("ERROR in recvfrom");
            if(fwrite(buf, 1, size, f) < 0){
              error("ERROR writing file\n");
              exit(1);
            }

            if(size < BUFSIZE-1){
              n = recvfrom(sockfd, &size, sizeof(int), 0, (struct sockaddr *) &clientaddr, &clientlen);
              if (n < 0)
                error("ERROR in recvfrom");
                break;
            }
          }
          fclose(f);
            
            
        }
        else if (strcmp(cmd, "delete") == 0){
          /*Check to see if file exists*/
          FILE* f = fopen(file, "rb");
          if(f == NULL){
              char* ret = "File Not Found Error\n";
              n = sendto(sockfd,ret, strlen(ret), 0, (struct sockaddr *) &clientaddr, clientlen);
              if (n < 0) 
                error("ERROR in sendto");
          }else {
            if(remove(file) == 0){
              char* ret = "File deleted successfully\n";
              n = sendto(sockfd,ret, strlen(ret), 0, (struct sockaddr *) &clientaddr, clientlen);
                if (n < 0) 
                  error("ERROR in sendto");
            }else{
              char* ret = "Failed to delete file\n";
              n = sendto(sockfd,ret, strlen(ret), 0, (struct sockaddr *) &clientaddr, clientlen);
                if (n < 0) 
                  error("ERROR in sendto");
            }
          }

        }
        else if (strcmp(cmd, "ls") == 0){
            /*Pointer for directory entry*/
            /*Pointer of DIR type*/
            bzero(buf, sizeof(buf));
            DIR *d = opendir(".");
            
            while(1){
              struct dirent *dir = readdir(d);
              if(dir == NULL){
                break;
              }
              strcat(buf, dir->d_name);
              strncat(buf, "\n", 1);
            }
            n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
            if (n < 0) 
              error("ERROR in sendto");


        }
        else if (strcmp(cmd, "exit") == 0){
          n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
          if (n < 0) 
            error("ERROR in sendto");
          break;
        }
        else{
            printf("User requested unknown command\n");
        }
    
    /* 
     * sendto: echo the input back to the client 
     n = sendto(sockfd, buf, strlen(buf), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
      error("ERROR in sendto");
     */
    
  }
}
