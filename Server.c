//
//  Server.c
//  CSC_Server
//
//  Created by Matt Windham on 11/30/17.
//  Copyright © 2017 Matt Windham. All rights reserved.
//

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <tgmath.h>

int main (int argc,char* argv[]) {
    
    int serverSocket;                   //Listening socket file descriptor
    int newSocket;                      //New socket file descriptor
    int sPort;                          //Port
    int valRead;                        //Value to read
    struct sockaddr_in server;          //server address
    struct sockaddr_in client;          //agent address
    int addrlen = sizeof(server);
    char buffer[256];                   //Buffer
    socklen_t agentLength;              //Defines length of agent for socket
    char serverResponse [10];           //Value to hold server reponses to agents
    int queue = 5;                      //Number of agents allowed in queue
    
    //Determines if the correct number of arguments are used
    if (argc < 2)
    {
        printf ("Usage:  No port selected \r\n");
        return(0);
    }
    
    //create socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        fprintf (stderr, "ERROR: Failed to create socket\n");
        exit(-1);
    }

    bzero((char *) &server, sizeof(server));    //Adds server to memory
    sPort = atoi(argv[1]);                      //Places port value as command line argument
    server.sin_family = AF_INET;                //Designates address the server can communicate with
    server.sin_addr.s_addr = INADDR_ANY;        //Socket no binded to specific IP
    server.sin_port = htons(sPort);             //Adds ports to server socket
    
    //bind socket to address and port
    if (bind(serverSocket, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        fprintf (stderr, "ERROR:  Socket bind failed\n");
        exit(-1);
    }
    
    //Socket listen (listens for agents in queue)
    if (listen(serverSocket, queue) < 0)
    {
        fprintf (stderr, "ERROR:  Queue is full\n");
        exit(-1);
    }
    
    struct agentList {                  //Structure for agents
        const char *server;
        time_t duration;
        
    };
    
    struct agentList log[256];
    int agentCount = 0;                 //Value to determine is agent is active
    int activeAgent = 0;    //determines if agent is already a member
    FILE *fp;                           //File pointer
    char fileName[] = "log.txt";        //File name
    int i = 0;                          //Index used in C for loops
    
    while (1) {     //Loop to keep server running while it recieves packets sent from agents
        
        char *agent;
        char *currentAddress;
        currentAddress = malloc(sizeof(client));
        agentLength = sizeof(client);
        //creates a new socket, and returns a new file descriptor.
        newSocket = accept(serverSocket, (struct sockaddr *) &client, &agentLength);
        
        agent = inet_ntoa(client.sin_addr); //converts an (Ipv4) internet network address into an ASCII string
        strcpy(currentAddress, agent);      //copies agent address string to currentAddress
        bzero(serverResponse, 10);          //Adds serverResponse to memory
        
        char tBuff[26];         //buffer for time
        int millisec;           //value for calculating milliseconds
        struct tm* tm_info;     //structure pointer for time values
        struct timeval tv;      //value passed for gettimeofday
        
        gettimeofday(&tv, NULL);
        millisec = (tv.tv_usec/1000); // Round to nearest millisec
        if (millisec>=1000) // Allow for rounding up to nearest second
        {
            millisec -=1000;
            tv.tv_sec++;
        }
        tm_info = localtime(&tv.tv_sec);
        strftime(tBuff, 26, "%m/%d/%Y %H:%M:%S", tm_info);  //Puts time value in time buffer
        
        //Accept (exctracts first connection request in queue for the listening socket)
        if (newSocket < 0)
        {
            fprintf (stderr, "ERROR: Accept failed\n");
            exit(-1);
        }
        
        bzero(buffer, 256);                         //add buffer to memory
        valRead = read(newSocket, buffer, 256);     //Gets read value
        
        //No value to be read
        if (valRead < 0)
        {
            fprintf (stderr, "ERROR: Failed to read");
            exit(-1);
        }
        
        //Agent request #JOIN
        if (strcmp(buffer, "#JOIN") == 0)
        {
            fp = fopen(fileName, "a+");     //File open for reading/writing (append if exist)
            fprintf(fp, "%s:%03d: Received a %s action from agent %s. \n", tBuff, millisec, buffer, currentAddress);
            printf("%s:%03d: Received a %s action from agent %s. \n", tBuff, millisec, buffer, currentAddress);
            
            if(agentCount == 0)
            {
                log[0].server = currentAddress;
                log[0].duration = time(NULL);       //Start agent join timer
                agentCount = agentCount + 1;          //Increment number of agents
                
                sprintf(serverResponse, "%s", "$OK");
                
                write(newSocket, serverResponse, strlen(serverResponse));
                
                fprintf(fp, "%s:%03d: Responded to agent %s with %s. \n", tBuff, millisec, currentAddress, serverResponse);
                printf("%s:%03d: Responded to agent %s with %s. \n", tBuff, millisec, currentAddress, serverResponse);
            }
        
            else if (agentCount > 0)    //If agent exists
            {
                for (i = 0; i < agentCount; i++)
                {
                    if (strcmp(log[i].server, currentAddress) == 0)
                    {
                        activeAgent = 1;
                        
                        sprintf(serverResponse, "%s", "$ALREADY MEMBER");
                        
                        write(newSocket, serverResponse, strlen(serverResponse));
                        
                        fprintf(fp, "%s:%03d: Responded to agent %s with %s. \n", tBuff, millisec, currentAddress, serverResponse);
                        printf("%s:%03d: Responded to agent %s with %s. \n", tBuff, millisec, currentAddress, serverResponse);

                    }
                }
                
                if (activeAgent < 1)        //If not an active agent
                {
                    log[agentCount].server = currentAddress;
                    log[agentCount].duration = time(NULL);      //Start agent join timer
                    agentCount = agentCount + 1;
                    
                    sprintf(serverResponse, "%s", "$OK");
                    
                    write(newSocket, serverResponse, strlen(serverResponse));
                    
                    fprintf(fp, "Responded to agent %s with %s. \n", currentAddress, serverResponse);
                    printf("%s:%03d: Responded to agent %s with %s. \n", tBuff, millisec, currentAddress, serverResponse);
                }
            }
            fclose(fp);
        }

        //Agent request #LEAVE
        if (strcmp(buffer, "#LEAVE") == 0)
        {
            fp = fopen(fileName, "a+");
            fprintf(fp,"%s:%03d: Received %s action from agent %s. \n", tBuff, millisec, buffer, currentAddress);
            printf("%s:%03d: Received a %s action from agent %s. \n", tBuff, millisec, buffer, currentAddress);
            int agentFound = 0;
        
            for (i = 0; i < agentCount; i++)
            {
                if (strcmp(log[i].server, currentAddress) == 0)
                {
                    log[i].server = log[agentCount - 1].server;
                    agentCount = agentCount - 1;
                
                    sprintf(serverResponse, "%s", "$OK");
                
                    write(newSocket, serverResponse, strlen(serverResponse));
                
                    fprintf(fp,"%s:%03d: Responded to agent %s with %s. \n", tBuff, millisec, currentAddress, serverResponse);
                    printf("%s:%03d: Responded to agent %s with %s. \n", tBuff, millisec, currentAddress, serverResponse);
                
                    agentFound = 1;
                
                }
            }
            if (agentFound == 0)
            {
                sprintf(serverResponse, "%s", "$NOT MEMBER");
                
                write(newSocket, serverResponse, strlen(serverResponse));
                
                fprintf(fp,"%s:%03d: Responded to agent %s with %s. \n", tBuff, millisec, currentAddress, serverResponse);
                printf("%s:%03d: Responded to agent %s with %s. \n", tBuff, millisec, currentAddress, serverResponse);
                
            }
            fclose(fp);
        }
        
        //Agent request #LIST
        if (strcmp(buffer, "#LIST") == 0)
        {
            fp = fopen(fileName, "a+");
            fprintf(fp, "%s:%03d: Received a %s action from agent %s. \n", tBuff, millisec, buffer, currentAddress);
            printf("%s:%03d: Received a %s action from agent %s. \n", tBuff, millisec, buffer, currentAddress);
            
            double elapsedTime = 0;     //Used to find agent time since joining server
            
            for (i = 0; i < agentCount; i++)
            {
                if (strcmp(log[i].server, currentAddress) == 0)
                {
                    int j = 0;
                    
                    for (j = 0; j < agentCount; j++)
                    {
                        elapsedTime = difftime(time(NULL),log[j].duration); //Calculate active agent time
                        
                        sprintf(serverResponse, "\n<%s, %f>", log[j].server, elapsedTime);
                        
                        write(newSocket, serverResponse, strlen(serverResponse));
                    }
                    fprintf(fp, "%s:%03d: Responded to agent %s with list. \n", tBuff, millisec ,currentAddress);
                    printf("%s:%03d: Responded to agent %s with list. \n", tBuff, millisec, currentAddress);
                }
                
            }
            if (elapsedTime == 0)
            {
                fprintf(fp,"%s:%03d: No response is supplied to agent %s (agent not active) \n", tBuff, millisec, currentAddress);
                printf("%s:%03d: No response is supplied to agent %s (agent not active) \n", tBuff, millisec, currentAddress);
            }
            fclose(fp);
        }
        
        //Agent request #LOG
        if (strcmp(buffer, "#LOG") == 0)
        {
            FILE *fp2;
            char *logName = "log.txt";
            int sendLog = 0;                //Value to determine is agent is active
            char logBuff[256];              //second log buffer
            bzero(logBuff, 256);            //add logBuff to memory
            fp = fopen(fileName, "a+");
            
            fprintf(fp,"%s:%03d: Received a %s action from agent %s. \n", tBuff, millisec, buffer, currentAddress);
            printf("%s:%03d: Received a %s action from agent %s. \n", tBuff, millisec, buffer, currentAddress);

            for (i = 0; i < agentCount; i++)    //Loop to add active agent
            {
                if (strcmp(log[i].server, currentAddress) == 0)
                {
                    sendLog = 1;
                }
            }
            
            if (sendLog == 0)   //Agent is not active
            {
                fprintf(fp,"%s:%03d: No log is supplied to inactive agent %s.\n",tBuff, millisec, currentAddress);
                printf("%s:%03d: No log is supplied to inactive agent %s.\n",tBuff, millisec, currentAddress);
            }
            
            if (sendLog == 1)   //Agent is active (send log)
            {
                fprintf(fp,"%s:%03d: Sent log to agent %s. \n", tBuff, millisec, currentAddress);
                printf("%s:%03d: Sent log to agent %s. \n", tBuff, millisec, currentAddress);
                
                if ((fp2 = fopen(logName, "r")) == NULL)    //File is not open for reading
                {
                    fprintf(stderr, "ERROR: Failed to open file %s", logName);
                    exit(-1);
                }
                
                while (!feof(fp2))
                {
                    fscanf(fp2, "%s \n", logBuff);
                    write(newSocket, logBuff, strlen(logBuff));
                }
                fclose(fp2);    //Close second log
            }
            fclose(fp);
        }
        close(newSocket);   //Close socket
    }
}
