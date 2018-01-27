#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXBUF 1024
#define MAXHOSTNAME 200
#define MAXACTION  10
#define MAXPORT 6

int main (int argc,char* argv[]) {
    
    char cServer[MAXHOSTNAME];
    char cPort[MAXPORT];
    char cAction [MAXACTION];
    int nPort;
    
    if (argc < 4)
    {
        printf ("Usage: agent server_name server_port action_requested \r\n");
        return(0);
    }
    memset(cServer,0,MAXHOSTNAME);
    sprintf(cServer,"%s",argv[1]);
    
    memset(cPort, 0, MAXPORT);
    sprintf(cPort,"%s",argv[2]);
    nPort = atoi(cPort);
    
    memset(cAction,0,MAXACTION);
    sprintf(cAction,"%s",argv[3]);
    
    struct sockaddr_in name;
    struct hostent *hent;
    
    int sd;
    // create a new socket to use for communication
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf (stderr, "ERROR: socket() failed\n");
        exit (-1);
    }
    
    fprintf(stdout,"Socket Created\n");
    
    // Used to correctly fill in the server host
    if ((hent = gethostbyname (cServer)) == NULL) {
        fprintf (stderr, "ERROR: Host not found\n");
        exit(-1);
    }
    else
        bcopy (hent->h_addr, &name.sin_addr, hent->h_length);
    
    name.sin_family = AF_INET;
    name.sin_port = htons (nPort); // notice the host to network conversion.
    
    // connect to the server
    if (connect (sd, (struct sockaddr *)&name, sizeof(name)) < 0) {
        fprintf (stderr, "ERROR: connect() failed\n");
        exit (-1);
    }
    
    printf("Connect OK\n");
    
    // send the action to the server
    int byteswritten = write (sd, cAction, strlen(cAction));
    
    printf("wrote %d\n",byteswritten);
    
    char buffer[MAXBUF];
    int bytes_read = 0;
    int total_bytes_read = 0;
    
    do {
        memset(buffer,0,MAXBUF);
        bytes_read = read(sd, buffer, MAXBUF);
        if (bytes_read < 0)
            break;
        fprintf(stdout,"Read %d bytes: [%s]\r\n", bytes_read,buffer);
        total_bytes_read += bytes_read;
    }
    while ( bytes_read > 0 );
    
    
    printf("Total Bytes Read = %d\r\n",total_bytes_read);
    
    close(sd);
    
    return 0;
}