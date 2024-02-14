/* User enters an IP address and port number 
Prompt user to enter 
get[file_name]
put[file_name]
delete[file_name]
ls exit

If all info correct 
send datagram to server with which operation is required and any other data 
wait for server response
if no response throw error 
if response: print message
    put: source the indicated file in working directory 
    get: store returned file in the current working directory 
    exit: exit gracefully after server acknoldgement (DONE)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char cmd[BUFSIZE];
    char file[BUFSIZE];
    int numBytes;
    char fileBuf[BUFSIZE];/*buffer to read in files*/

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);
    //Check valid port numbers 
    if(portno < 5001 || portno > 65535){
        error("Port out of range. Must be between 5000 and 65535");
    }

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    //Loop to process users requests until exit is called
    while(1){
        /* get a message from the user */
        bzero(buf, BUFSIZE);
        printf("Enter one of the following commands:\n get [filename]\n put [filename]\n delete [filename]\n ls\n exit\n");
        fgets(buf, BUFSIZE, stdin);
        sscanf(buf, "%s %s", cmd, file);

        //if/else if tree to process users command 
        if (strcmp(cmd, "get") == 0){

            int size = 0;
            /* send the message to the server */
            serverlen = sizeof(serveraddr);
            n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
            if (n < 0) 
                error("ERROR in sendto");
    
            FILE *f = fopen(file, "w");

            while(1){
                //Get packet size
                n = recvfrom(sockfd, &size, sizeof(int), 0, &serveraddr, &serverlen);
                if (n < 0)
                    error("ERROR in recvfrom");
                else if(size == -1){
                    break;
                }
                bzero(buf, BUFSIZE);
                //Get transmitted data
                n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
                if (n < 0)
                    error("ERROR in recvfrom");

                if(fwrite(buf, 1, size, f) < 0){
                    error("ERROR writing file\n");
                    exit(1);
                }

                if(size < BUFSIZE-1){
                    n = recvfrom(sockfd, &size, sizeof(int), 0, &serveraddr, &serverlen);
                    if (n < 0)
                        error("ERROR in recvfrom");
                        break;
                }
            }
            fclose(f);
        }
        else if (strcmp(cmd, "put") == 0){
            /*Check to see if file exists*/
            FILE* f = fopen(file, "rb");
            if(f == NULL){
                printf("File Not Found Error\n");
                continue;
            }

            /* send put command and filename to server */
            serverlen = sizeof(serveraddr);
            n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
            if (n < 0) 
                error("ERROR in sendto");
            
            int size;
            bzero(buf, BUFSIZE);

            while(!feof(f)){
                size = fread(buf, 1, BUFSIZE-1, f);
                //Send packet size to server
                n = sendto(sockfd, &size, sizeof(int), 0, &serveraddr, serverlen);
                if (n < 0)
                    error("ERROR in sendto");
                //Send data to server 
                n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
                if (n < 0)
                    error("ERROR in sendto");
            }
            fclose(f);
            //Tell server we are done sending data
            size = -1;
            n = sendto(sockfd, &size, sizeof(int), 0, &serveraddr, serverlen);
              if (n < 0) 
                error("ERROR in sendto");
        }
        else if (strcmp(cmd, "delete") == 0){
            /* send command and file to be deleted to server */
            serverlen = sizeof(serveraddr);
            n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
            if (n < 0) 
                error("ERROR in sendto");
            n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
            if (n < 0) 
                error("ERROR in recvfrom");
            printf("Echo from server: %s ", buf);
        }
        else if (strcmp(cmd, "ls") == 0){
           /* send the message to the server */
            serverlen = sizeof(serveraddr);
            n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
            if (n < 0) 
                error("ERROR in sendto");
            /* print the server's reply */
            
            n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
            if (n < 0) 
                error("ERROR in recvfrom");
            printf("%s", buf);
                
        }
        else if (strcmp(cmd, "exit") == 0){
            /* send the message to the server */
            serverlen = sizeof(serveraddr);
            n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
            if (n < 0) 
                error("ERROR in sendto");
    
            /* print the server's reply */
            n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
            if (n < 0) 
                error("ERROR in recvfrom");
            printf("Echo from server: %s", buf);
            break;
        }
        else{
            printf("User requested unknown command\n");
        }
        
    }

    
}
