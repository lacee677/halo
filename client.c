#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char *argv[] ){ 

	if(!argv[1] || !argv[2] || !argv[3]){
		printf("Usage: ./server server_ip buffersize port \n");
		exit(1);
	}

    int buffersize 	= strtol(argv[2], NULL, 10);
	int portnumber 	= strtol(argv[3], NULL, 10);
    printf("buffer: %d \n", buffersize);
	printf("port: %d \n", portnumber);

	struct sockaddr_in server;      	    // socket name (addr) of server
	struct sockaddr_in client;	            // socket name of client
	
	char on 	    	= 1;
	int flags 		    = 0;
	int server_size     = sizeof server;
	int client_size     = sizeof client;

	int fd;	        	           	        // socket endpt
	char buffer[buffersize+1];   			// datagram dat buffer area 
	int err;  
    int bytes;    	                // length of buffer 
    int ip;						    // ip address
    char server_addr[16];            // server address	
    int trnmsize, rcvsize;
    

    sprintf(server_addr, "%s", argv[1]);
    ip = inet_addr(server_addr);
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = ip;
    server.sin_port        = htons(portnumber);

    /* GAME */

    const char *cards[8] =
	{
		"VII",
		"VIII",
		"IX",
		"X",
		"Also",
		"Felso",
		"Kiraly",
		"Asz"	
	};

    const int card_values[8] = { 7, 8, 9, 10, 2, 3, 4, 11 };
    int counter, complete_value, settrigger;
    int card_number[5] = {0};

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "%s: Socket creation error.\n",argv[0]);
        exit(2);
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);

    err = connect(fd, (struct sockaddr *) &server, server_size);
    if (err < 0) {
        fprintf(stderr, "%s: Cannot connect to the server.\n", argv[0]);
        exit(3);
    }

    printf("Connected to the server %s:%i \n", server_addr, portnumber);

    while(1){
		counter = 0;
		complete_value = 0;
		settrigger = 0;
		while(1){
			rcvsize = recv( fd, buffer, buffersize, flags );
			if (rcvsize <= 0) {
				fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
				exit(5);
			}
			if(settrigger != 1){
				card_number[counter] = buffer[1] - '0';
				complete_value += card_values[ card_number[counter] ];
				printf("Cards in hand(%d):", counter+1);
				for(int i = 0; i <= counter; i++){
					printf(" %s(%d)", cards[ card_number[i] ], card_values[ card_number[i] ]);
				}
				printf("\nTotal: %d\n", complete_value);
			}
			if(strchr("1",buffer[2])){
				printf("You won!\n");
				break;
			}
			else if(strchr("2",buffer[2])){
				printf("You lost!\n");
				break; 
			}
			else if(strchr("4",buffer[2])){
				printf("Wait for your turn!\n");
				rcvsize = recv( fd, buffer, buffersize, flags );
				if (rcvsize <= 0) {
					fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
					exit(5);
				}
				
				printf("bet? ");
				scanf("%s", buffer);
				bytes = strlen(buffer) + 1;
				trnmsize = send(fd, buffer, bytes, flags);
				if (trnmsize < 0) {
					fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
					exit(6);
				}
				settrigger = 1;
				
			}
			else{		
				settrigger = 0;
				counter++;
				printf("\nmore or enough? ");
				scanf("%s", buffer);
				bytes = strlen(buffer) + 1;
				trnmsize = send(fd, buffer, bytes, flags);
				if (trnmsize < 0) {
					fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
					exit(6);
				}

				if(strcmp(buffer, "enough") == 0){
					break;				
				}
				
			}
		}
		if(complete_value < 21){
			rcvsize = recv( fd, buffer, buffersize, flags );
			if (rcvsize <= 0) {
				fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
				exit(5);
			}

			if(strchr("1",buffer[2])){
				printf("You won!\n");
			
			}
			else if(strchr("3",buffer[2])){
				printf("Draw!\n");
			
			}
			else if(strchr("2",buffer[2])){
				printf("You lost!\n");
			
			}
		}

		printf("New game (yes/no)? ");
		scanf("%s", buffer);
		printf("\nWaiting for other user...\n");
		bytes = strlen(buffer) + 1;
		trnmsize = send(fd, buffer, bytes, flags);
		if (trnmsize < 0) {
			fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
			exit(6);
		}

		rcvsize = recv( fd, buffer, buffersize, flags );
		if (rcvsize <= 0) {
			fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
			exit(5);
		}

		if(strcmp(buffer, "new game") == 0){
			sprintf(buffer, "%s", "ready");
			bytes = strlen(buffer) + 1;
			trnmsize = send(fd, buffer, bytes, flags);
			if (trnmsize < 0) {
				fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
				exit(6);
			}
			printf("\nNew game is starting\n");
		}
		else{
			printf("\nOne of you didn't wanted to play anymore!\n");
			break;
		}
		
	}

    close(fd);
    exit(0);
} 