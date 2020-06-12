/************************************************************************************
*  Project Name: Project 6: You Got Served
*  Description: Client Server Project using HTTP Protocol for webpages
*  File names: server.c 
*  Date: May 4,2018
*  Authors: Anh D Nguyen 
************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>

//defines the content type for supporting MIME
#define FILE_FOUND "HTTP/1.0 200 OK\r\n"
#define ERROR "HTTP/1.0 404 Not Found\r\n\r\n"
#define HTML "Content-Type: text/html\r\n"
#define JPEG "Content-Type: image/jpeg\r\n"
#define GIF "Content-Type: image/gif\r\n"
#define PNG "Content-Type: image/png\r\n"
#define PLAIN "Content-Type: text/plain\r\n"
#define PDF "Content-Type: application/pdf\r\n"
#define UNKNOWN "Content-Type: application/octet-stream\r\n"

//funtion prototypes
FILE* getHead(int clientFD, char* command, char* filePath, char* protocol);
void send404(int clientFD, char* filePath);

int main(int argc, char** argv)
{
	//Quit the program if the input is not exactly 3 arguments in the command line 
	if(argc != 3)
		printf("Usage: ./server <port> <path to root>\n");
	else
	{
		char* portNum = argv[1];
		const char* directory = argv[2];
		int checkDirectory = chdir(directory);
		//check if the user input is a valid directory
		if(checkDirectory == -1)
			printf("Could not change to the directory: %s\n", directory);
		else
		{	int port = atoi(portNum);
			//make socket
			int sockFD = -1;
			
			sockFD = socket(AF_INET, SOCK_STREAM, 0);
			//bind socket
			struct sockaddr_in address;
			memset(&address, 0, sizeof(address));
			address.sin_family = AF_INET;
			address.sin_port = htons(port);

			address.sin_addr.s_addr = INADDR_ANY;
			
			//check if it is a bad port (portNum >= 0 && portNum <=65535)
			if(bind(sockFD, (struct sockaddr*)&address, sizeof(address)) == -1) 
			{
				printf("Bad Port: %d\n", port);
				return 0;
			}
			//listen to the socket
			listen(sockFD, 1);
			//To make the socket resuable for multiple connections
			int value = 1; 
			setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

			//Buffers for holding client requests
			char buffer[1024];//information from client
			char command[1024];// client request ( GET & HEAD )
			char filePath[1024];// specified file path/name
			char protocol[1024];// client's protocol
		
			//Infinite while loop to continue the process of requesting 
			//server can not be crashed -> infinity loop
			for(;;)
			{
				//Accept the request
				printf("Waiting for a connection\n");
				struct sockaddr_storage otherAddress;
				socklen_t otherSize = sizeof(otherAddress);
				int clientFD = accept(sockFD, (struct sockaddr*)&otherAddress, &otherSize);
				printf("Connection received\n");
				//Recieve request from client
				int checkBuffer = recv(clientFD, buffer, 1024, 0);
				//To make sure things have been recieved
				if(checkBuffer > 0)
				{
					buffer[checkBuffer] = '\0';
					int checkInput = sscanf(buffer,"%s %s %s", command, filePath, protocol); //request for writing buffers
					//Checks to make sure command, filepath and protocol has read
					if(checkInput == 3)
					{
						printf("Got a Client:\n\t%s %s %s\n", command, filePath, protocol);
						//Checks to see GET or HEAD request otherwise skips
						if(strcmp(command, "GET") == 0)
						{
							//Sends the header data to client
							FILE* file = getHead(clientFD, command, filePath, protocol);
							//check if file exists
							if(file != NULL)
							{
								char sending[1024];
								int readData = fread(sending, 1, 1024, file);
								while(readData != 0)
								{
									send(clientFD, sending, readData, 0);
									readData = fread(sending, 1, 1024, file);
								}
								printf("Sent file: %s\n\n", filePath);
								fclose(file);	
							} 

						}
						else if(strcmp(command, "HEAD") == 0)
							getHead(clientFD, command, filePath, protocol);
					}
				}
				close(clientFD);
			}
			close(sockFD);
		}
	}
}
FILE* getHead(int clientFD, char* command, char* filePath, char* protocol)
{
    //Get the length of the file path
	int length = strlen(filePath);

    //Concat index.html if the file path ends with a /
	if(filePath[length - 1] == '/')
		strcat(filePath, "index.html");

    //Find the new length of the file path
	length = strlen(filePath);

	//Remove leading slash	
	int i;
	for(i = 0; i < length; i++)
		filePath[i] = filePath[i+1];

	//Try to open file
	FILE* file = fopen(filePath, "rb");

	//If file exists, process data in it
	if(file)
	{
		//Sends to the client that the file was found
		send(clientFD, FILE_FOUND, strlen(FILE_FOUND), 0);
		
		//Finds the point in the path in which there is the '.' for the extention
		while(filePath[length] != '.')
			length--;
		
		//Sets a pointer to the start of the file type
		char* pointer = &(filePath[length+1]);

        //Sends the appropriate Content Type to the client
		if(strcmp(pointer, "html") == 0 || strcmp(pointer, "htm") == 0)
			send(clientFD, HTML, strlen(HTML), 0);
		else if(strcmp(pointer, "jpg") == 0 || strcmp(pointer, "jpeg") == 0)
			send(clientFD, JPEG, strlen(JPEG), 0);
		else if (strcmp(pointer, "gif") == 0)
			send(clientFD, GIF, strlen(GIF), 0);
		else if (strcmp(pointer, "png") == 0)
			send(clientFD, PNG, strlen(PNG), 0);
		else if(strcmp(pointer, "txt") == 0 || strcmp(pointer, "c") == 0 || strcmp(pointer, "h") == 0)	
			send(clientFD, PLAIN, strlen(PLAIN), 0);
		else if (strcmp(pointer, "pdf") == 0)
			send(clientFD, PDF, strlen(PDF), 0);
		else
			send(clientFD, UNKNOWN, strlen(UNKNOWN), 0);
		
		//Used to find the file size
		struct stat fileInfo;
		stat(filePath, &fileInfo);

		//Stores the file size as a string so it can be sent
		char fileSize[1024];
		int size = sprintf(fileSize, "Content-Length: %ld\r\n\r\n", fileInfo.st_size);

		//Sends the file size as per the HTTP protocol
		send(clientFD, fileSize, size, 0);
        	return file;
    	}
	else
	{
        	//Sends client an error if the file requested was not found
        	send404(clientFD, filePath);
		printf("File not found: %s\n\n", filePath);
        	return NULL;
    	}									
}
//send an Error message to the browser if User is not looking for an approprate file
void send404(int clientFD, char* filePath)
{
	send(clientFD, ERROR , strlen(ERROR), 0);
	char* error = "<html>\n<body>\n<h1>404: Page NOT FOUND!</h1>\n<p>The requested URL %s was not found on this server.</p>\n</body>\n</html>\n";
	char sending[1024];
    	sprintf(sending, error, filePath);

    	//Sends the actual content
    	send(clientFD, sending, strlen(sending), 0);
}

				
