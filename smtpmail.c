#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_BUFFER_SIZE 1024

void handleClient(int clientSocket, in_addr_t addr);
void sendCommand(int Socket, const char *command) ;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <my_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int myPort = atoi(argv[1]);

    int sockfd, newsockfd; /* Socket descriptors */
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;

    /* Create socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }

    /* Set up server address */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    in_addr_t addr = serv_addr.sin_addr.s_addr;
    serv_addr.sin_port = htons(myPort);

    /* Bind socket */
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Unable to bind local address");
        exit(EXIT_FAILURE);
    }

    listen(sockfd, 5);

    printf("SMTP Server listening on port %d...\n", myPort);
    char buffer[MAX_BUFFER_SIZE];
    char username[MAX_BUFFER_SIZE];
    char password[MAX_BUFFER_SIZE];

    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (newsockfd < 0) {
            perror("Accept error");
            exit(EXIT_FAILURE);
        }

        if (fork() == 0) {
            close(sockfd);
            while(1)
            {
                handleClient(newsockfd, addr);
            }
            close(newsockfd);
            exit(0);
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}


void sendCommand(int Socket, const char *command) {
    send(Socket, command, strlen(command), 0);
}

void handleClient(int clientSocket, in_addr_t serv_addr ) {
    char buffer[MAX_BUFFER_SIZE];
    char username[MAX_BUFFER_SIZE];
    char sender[MAX_BUFFER_SIZE];
    char recipient[MAX_BUFFER_SIZE];
    FILE *mailbox = NULL;
    FILE *userFile = NULL;


    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);

    if (strncmp(buffer, "START", 5) != 0) {
        printf("Error: Expected START command, but received: %s", buffer);
        sendCommand(clientSocket, "550  Expected START command\r\n");
        return;
    }

    // Send initial service ready message
    char serviceReady[100] ;
    snprintf(serviceReady, sizeof(serviceReady), "220 %d Service Ready\r\n", serv_addr);
    send(clientSocket, serviceReady, sizeof(serviceReady), 0);

    // Receive HELO command
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "HELO", 4) != 0) {
        printf("Error: Expected HELO command, but received: %s", buffer);
        sendCommand(clientSocket, "550  Expected HELO command\r\n");
        return;
    }
    sscanf(buffer, "HELO %s", username);
    // printf("local host: %s.\n", username);

    // Send HELO response
    char heloResponse[MAX_BUFFER_SIZE];
    snprintf(heloResponse, sizeof(heloResponse), "250 OK Hello %s\r\n", username);
    // printf("response: %s\n", heloResponse);
    send(clientSocket, heloResponse, strlen(heloResponse), 0);

    // Receive MAIL FROM command
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "QUIT",4) == 0) {
        return;  // Return from the function
    }  
    sscanf(buffer, "MAIL FROM: %s", sender);

    // printf(" sender: %s\n",sender);

    // Cross-check sender against user.txt
    userFile = fopen("./user.txt", "r");
    if (!userFile) {
        char invalid[] = "550 user file not existing.\r\n";
        send(clientSocket, invalid, sizeof(invalid), 0);
        perror("Error opening mailbox file");
        return;
    }

    int validSender = 0;

    while (fscanf(userFile, "%s", buffer) == 1) {
        if (strcmp(sender, buffer) == 0) {
            validSender = 1;
            break;
        }
    }

    fclose(userFile);

    if (!validSender) {
        char invalidSender[] = "550 No such user\r\n";
        send(clientSocket, invalidSender, sizeof(invalidSender), 0);
        return;
    }

    // Send MAIL FROM response
    char mailFromResponse[MAX_BUFFER_SIZE];
    snprintf(mailFromResponse, sizeof(mailFromResponse), "250 %s... Sender ok\r\n", sender);
    send(clientSocket, mailFromResponse, strlen(mailFromResponse), 0);

    // Receive RCPT TO command
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "QUIT",4) == 0) {
        return;  // Return from the function
    }  
    sscanf(buffer, "RCPT TO: %s", recipient);

    // printf(" recipient:%s.\n",recipient);

    // Cross-check recipient against user.txt
    userFile = fopen("./user.txt", "r");
    if (!userFile) {
        char invalid_file[] = "550 user file not existing.\r\n";
        send(clientSocket, invalid_file, sizeof(invalid_file), 0);
        perror("Error opening mailbox file");
        return;
    }
    
    int validRecipient = 0;

    while (fscanf(userFile, "%s", buffer) == 1) {
        if (strcmp(recipient, buffer) == 0) {
            validRecipient = 1;
            break;
        }
    }

    fclose(userFile);

    if (!validRecipient) {
        char invalidRecipient[] = "550 No such user\r\n";
        send(clientSocket, invalidRecipient, sizeof(invalidRecipient), 0);
        printf("invalid: %s", invalidRecipient);
        return;
    }

    // Send RCPT TO response
    char rcptToResponse[MAX_BUFFER_SIZE];
    snprintf(rcptToResponse, sizeof(rcptToResponse), "250 %s... Recipient ok\r\n", recipient);
    send(clientSocket, rcptToResponse, strlen(rcptToResponse), 0);

    // Receive DATA command
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "QUIT",4) == 0) {
        return;  // Return from the function
    }  

    // Receive and append the email content to the mailbox file
    // printf("./users/%s/mymailbox.txt\n",recipient);
    char mailboxPath[MAX_BUFFER_SIZE];
    snprintf(mailboxPath, sizeof(mailboxPath), "./%s/mymailbox.txt", recipient);

    mailbox = fopen(mailboxPath, "a+");
    if (!mailbox) {
        perror("Error opening mailbox file");
        char Response[] = "550 No user directory\r\n";
        send(clientSocket, Response, sizeof(Response), 0);
        return;
    }

    // Send DATA response
    char dataResponse[] = "354 Enter mail, end with \".\" on a line by itself\r\n";
    send(clientSocket, dataResponse, sizeof(dataResponse), 0);

    int count = 0;
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        if (strncmp(buffer, "QUIT",4) == 0) {
            return;  // problem
        }
        fprintf(mailbox, "%s", buffer);
        // printf("line: %s\n", buffer);
        if (strcmp(buffer, ".\n") == 0) {
            break;  // End of message
        }
        count++;
        if(count == 2)
        {
            memset(buffer, 0, sizeof(buffer));
            time_t t;
            struct tm *tm_info;
            time(&t);
            tm_info = localtime(&t);
            strftime(buffer, sizeof(buffer), "Received: %Y-%m-%d %H:%M\n", tm_info);
            fprintf(mailbox, "%s", buffer);
        }
    }

    fclose(mailbox);

    // Send acknowledgment to the client
    char acknowledgment[] = "250 OK Message accepted for delivery\r\n";
    send(clientSocket, acknowledgment, sizeof(acknowledgment), 0);

    // Receive QUIT command
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "QUIT", 4) != 0) {
        printf("%s",buffer);
        return;  // Return from the function
    }  
    return;
}
