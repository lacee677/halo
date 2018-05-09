#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <time.h>

int main(int argc, char *argv[] ){ 

    if(!argv[1] || !argv[2]){
        printf("Usage: ./server buffersize port \n");
        exit(1);
    }

    int buffersize 	= strtol(argv[1], NULL, 10);
    int portnumber 	= strtol(argv[2], NULL, 10);
    printf("buffer: %d \n", buffersize);
    printf("port: %d \n", portnumber);

    struct sockaddr_in server;      	    // socket name (addr) of server
    struct sockaddr_in client;	            // socket name of client
    struct sockaddr_in cliento;	            // socket name of client
    
    char on 		= 1;
    int flags 		= 0;
    int bytes 		= buffersize;
    int server_size     = sizeof server;
    int client_size     = sizeof client;
    int client_sizeo     = sizeof cliento;
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port        = htons(portnumber);

    int fd;	        	           	        // socket endpt
    int fdc, fdco;                        	    // socket endpt
    char buffer[buffersize+1];   			// datagram dat buffer area 
    int err;  
    int trnmsize, rcvsize;

    /* GAME */ 

    const int cards[8] = { 7, 8, 9,	10,	2, 3, 4, 11 };
    int random[32];
    int cardcount[8] = {0};
    int r, counter;
    int user1sum, user2sum, banksum, user1bet, user2bet, newgame;

    srand(time(NULL));

    /* SOCKET START */

    fd = socket(AF_INET, SOCK_STREAM, 0 );
    if (fd < 0) {
        fprintf(stderr, "%s: Socket creation error\n", argv[0]);
        exit(2);
        }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);
       setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);

    err = bind(fd, (struct sockaddr *) &server, server_size);
    if (err < 0) {
        fprintf(stderr, "%s: Cannot bind to the socket\n",argv[0]);
        exit(3);
        }

    err = listen(fd, 10);
    if (err < 0) {
        fprintf(stderr, "%s: Cannot listen to the socket\n",argv[0]);
        exit(4);
        }

    fdc = accept(fd, (struct sockaddr *) &client, &client_size);
    if (fdc < 0) {
        fprintf(stderr, "%s: Cannot accept on socket\n",argv[0]);
        exit(5);
        }

    printf("First client -> connected \n");
    printf("Waiting for second client");

    fdco = accept(fd, (struct sockaddr *) &cliento, &client_sizeo);
    if (fdco < 0) {
        fprintf(stderr, "%s: Cannot accept on socket\n",argv[0]);
        exit(5);
        }

    printf(" -> connected \n");

    /* SOCKET END */

    /* MAIN GAME LOOP */
    while(1){

        /* SHUFFLE CARDS */
        for(int i = 0; i < 8; i++){
            cardcount[i] = 0;
        }
        for(int i = 0; i < 32; i++){
            r = rand() % 8;
            while(cardcount[r] == 4){
                r = rand() % 8;
            }
            cardcount[r]++;
            random[i] = r;
        }

        /* SET DEFAULT VALUES */
        counter = 0;
        user1sum = 0;
        user2sum = 0;
        banksum = 0;

        /* FIRST USER GETS FIRST CARD */
        user1sum += cards[ random[counter] ];
        sprintf(buffer,"1%d0", random[counter]);
        counter++;
        bytes = strlen(buffer) + 1;
        trnmsize = send(fdc, buffer, bytes, flags);
        if (trnmsize < 0) {
            fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
            exit(6);
        }
        printf("bytes from server: %d, %s \n", trnmsize-1, buffer);

        /* SECOND USER GETS FIRST CARD */
        user2sum += cards[ random[counter] ];
        sprintf(buffer,"1%d4", random[counter]);
        counter++;
        bytes = strlen(buffer) + 1;
        trnmsize = send(fdco, buffer, bytes, flags);
        if (trnmsize < 0) {
            fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
            exit(6);
        }
        printf("bytes from server: %d, %s \n", trnmsize-1, buffer);


        /* BANK GETS FIRST CARD */
        banksum += cards[ random[counter] ];
        counter++;

        /* USER 1 TURN START */
        while(1){

            // [0]who(0 bank, 1 you, 2 other client),
            // [1]card_number(0-7),
            // [2]status(0 nothing, 1 win, 2 lose, 3 draw, 4 waiting),

            if(user1sum < 21){
                rcvsize = recv( fdc, buffer, buffersize, flags );
                if (rcvsize <= 0) {
                    fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
                    exit(7);
                }

                printf("bytes for server: %d,  %s\n", rcvsize-1, buffer);

                if(strcmp(buffer, "enough") == 0){
                    break;
                }
            }
            else{
                break;
            }
            
            user1sum += cards[ random[counter] ];
            if(user1sum == 21){
                sprintf(buffer,"1%d1", random[counter]);
            }
            else if(user1sum > 21){
                sprintf(buffer,"1%d2", random[counter]);
            }
            else{
                sprintf(buffer,"1%d0", random[counter]);
            }
            counter++;
            bytes = strlen(buffer) + 1;
            trnmsize = send(fdc, buffer, bytes, flags);
            if (trnmsize < 0) {
                fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                exit(6);
            }
            printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                        
            printf("loop end \n");
        }
        /* USER 1 TURN END */

        /* SEND USER2 SIGNAL FOR HIS/HER TURN */
        sprintf(buffer,"%s", "your turn");
        bytes = strlen(buffer) + 1;
        trnmsize = send(fdco, buffer, bytes, flags);
        if (trnmsize < 0) {
            fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
            exit(6);
        }
        printf("bytes from server: %d, %s \n", trnmsize-1, buffer);

        rcvsize = recv( fdco, buffer, buffersize, flags );
        if (rcvsize <= 0) {
            fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
            exit(7);
        }
        user2bet = strtol(buffer, NULL, 10);
        printf("bytes for server: %d,  %s\n", rcvsize-1, buffer);

        /* SEND USER2 SIGNAL FOR HIS/HER TURN TO START */
        sprintf(buffer,"%s", "bet accepted");
        bytes = strlen(buffer) + 1;
        trnmsize = send(fdco, buffer, bytes, flags);
        if (trnmsize < 0) {
            fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
            exit(6);
        }
        printf("bytes from server: %d, %s \n", trnmsize-1, buffer);

        /* USER 2 TURN */
        while(1){

            // [0]who(0 bank, 1 you, 2 other client),
            // [1]card_number(0-7),
            // [2]status(0 nothing, 1 win, 2 lose, 3 draw, 4 waiting),

            if(user2sum < 21){
                rcvsize = recv( fdco, buffer, buffersize, flags );
                if (rcvsize <= 0) {
                    fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
                    exit(7);
                }

                printf("bytes for server: %d,  %s\n", rcvsize-1, buffer);

                if(strcmp(buffer, "enough") == 0){
                    break;
                }
            }
            else{
                break;
            }
            
            user2sum += cards[ random[counter] ];
            if(user2sum == 21){
                sprintf(buffer,"1%d1", random[counter]);
            }
            else if(user2sum > 21){
                sprintf(buffer,"1%d2", random[counter]);
            }
            else{
                sprintf(buffer,"1%d0", random[counter]);
            }
            counter++;
            bytes = strlen(buffer) + 1;
            trnmsize = send(fdco, buffer, bytes, flags);
            if (trnmsize < 0) {
                fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                exit(6);
            }
            printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                        
            printf("loop end \n");
        }

        /* EVALUATE GAME */
        if(user1sum < 21 && user2sum < 21){
            if( (user1sum == user2sum && user1sum > banksum && user2sum > banksum )  || 
                ( user1sum == banksum && user2sum == banksum ) ){
                sprintf(buffer,"1%d3", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if( user1sum > banksum && user2sum < banksum){
                sprintf(buffer,"1%d1", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                
                sprintf(buffer,"1%d2", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if( user1sum < banksum && user2sum > banksum){
                sprintf(buffer,"1%d2", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                
                sprintf(buffer,"1%d1", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if( user2sum > banksum && user1sum < banksum){
                sprintf(buffer,"1%d2", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                
                sprintf(buffer,"1%d1", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if( user2sum < banksum && user1sum > banksum){
                sprintf(buffer,"1%d1", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                
                sprintf(buffer,"1%d2", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if( user1sum > banksum && user2sum == banksum){
                    sprintf(buffer,"1%d1", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                
                sprintf(buffer,"1%d3", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if( user2sum > banksum && user1sum == banksum){
                sprintf(buffer,"1%d3", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                
                sprintf(buffer,"1%d1", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if( user2sum < banksum && user1sum < banksum){
                sprintf(buffer,"1%d2", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                
                sprintf(buffer,"1%d2", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if( user2sum > banksum && user1sum > banksum){
                if(user2sum > user1sum){
                    sprintf(buffer,"1%d2", 9);
                    bytes = strlen(buffer) + 1;
                    trnmsize = send(fdc, buffer, bytes, flags);
                    if (trnmsize < 0) {
                        fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                        exit(6);
                    }
                    printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                    
                    sprintf(buffer,"1%d1", 9);
                    bytes = strlen(buffer) + 1;
                    trnmsize = send(fdco, buffer, bytes, flags);
                    if (trnmsize < 0) {
                        fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                        exit(6);
                    }
                    printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                }
                else{
                    sprintf(buffer,"1%d1", 9);
                    bytes = strlen(buffer) + 1;
                    trnmsize = send(fdc, buffer, bytes, flags);
                    if (trnmsize < 0) {
                        fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                        exit(6);
                    }
                    printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                    
                    sprintf(buffer,"1%d2", 9);
                    bytes = strlen(buffer) + 1;
                    trnmsize = send(fdco, buffer, bytes, flags);
                    if (trnmsize < 0) {
                        fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                        exit(6);
                    }
                    printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
                }
            }
            
        }
        else{
            if(user1sum == 21 && user2sum < banksum){
                sprintf(buffer,"1%d2", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if(user2sum == 21 && user1sum < banksum){
                sprintf(buffer,"1%d2", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if(user1sum == 21 && user2sum == banksum){
                sprintf(buffer,"1%d3", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if(user2sum == 21 && user1sum == banksum){
                sprintf(buffer,"1%d3", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if(user1sum < 21 && user1sum < banksum){
                sprintf(buffer,"1%d1", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if(user1sum < 21 && user1sum == banksum){
                sprintf(buffer,"1%d3", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdc, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if(user2sum < 21 && user2sum < banksum){
                sprintf(buffer,"1%d1", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
            else if(user2sum < 21 && user2sum == banksum){
                sprintf(buffer,"1%d3", 9);
                bytes = strlen(buffer) + 1;
                trnmsize = send(fdco, buffer, bytes, flags);
                if (trnmsize < 0) {
                    fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                    exit(6);
                }
                printf("bytes from server: %d, %s \n", trnmsize-1, buffer);
            }
        }

        /* ASK FOR NEW GAME */

        rcvsize = recv( fdc, buffer, buffersize, flags );
        if (rcvsize <= 0) {
            fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
            exit(7);
        }

        newgame += strcmp(buffer, "yes");
        
        printf("bytes for server: %d,  %s\n", rcvsize-1, buffer);

        rcvsize = recv( fdco, buffer, buffersize, flags );
        if (rcvsize <= 0) {
            fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
            exit(7);
        }

        newgame += strcmp(buffer, "yes");
        
        printf("bytes for server: %d,  %s\n", rcvsize-1, buffer);

        if(newgame == 0){
            sprintf(buffer,"%s", "new game");
            bytes = strlen(buffer) + 1;

            trnmsize = send(fdc, buffer, bytes, flags);
            if (trnmsize < 0) {
                fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                exit(6);
            }
            printf("bytes from server: %d, %s \n", trnmsize-1, buffer);

            trnmsize = send(fdco, buffer, bytes, flags);
            if (trnmsize < 0) {
                fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                exit(6);
            }
            printf("bytes from server: %d, %s \n", trnmsize-1, buffer);

            rcvsize = recv( fdc, buffer, buffersize, flags );
            if (rcvsize <= 0) {
                fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
                exit(7);
            }
            printf("bytes from server: %d, %s \n", trnmsize-1, buffer);

            rcvsize = recv( fdco, buffer, buffersize, flags );
            if (rcvsize <= 0) {
                fprintf(stderr, "%s: Cannot receive from the socket\n",argv[0]);
                exit(7);
            }
            printf("bytes from server: %d, %s \n", trnmsize-1, buffer);

            printf("New game\n");
        }
        else{
            sprintf(buffer,"%s", "close");
            bytes = strlen(buffer) + 1;

            trnmsize = send(fdc, buffer, bytes, flags);
            if (trnmsize < 0) {
                fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                exit(6);
            }
            printf("bytes from server: %d, %s \n", trnmsize-1, buffer);

            trnmsize = send(fdco, buffer, bytes, flags);
            if (trnmsize < 0) {
                fprintf(stderr, "%s: Cannot send data to the client.\n",argv[0]);
                exit(6);
            }
            printf("bytes from server: %d, %s \n", trnmsize-1, buffer);

            break;
        }

        printf("Main loop end\n");
    }

    printf("Closing server.\n");

    close(fdc);
    close(fdco);
    close(fd);
    exit(0);

}