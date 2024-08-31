
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
void receiveResponse(int Socket, char *buffer) ;

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

    printf("POP3 Server listening on port %d...\n", myPort);

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

void receiveResponse(int Socket, char *buffer) {
    memset(buffer, 0, MAX_BUFFER_SIZE);
    recv(Socket, buffer, MAX_BUFFER_SIZE, 0);
}

void handleClient(int clientSocket, in_addr_t addr) {
    char buffer[MAX_BUFFER_SIZE];
    FILE *mailbox = NULL;
    FILE *userFile = NULL;
    FILE *lockFile = NULL;
    char username[MAX_BUFFER_SIZE];
    char password[MAX_BUFFER_SIZE], correct_password[MAX_BUFFER_SIZE];
    char filePath[MAX_BUFFER_SIZE], newfilePath[MAX_BUFFER_SIZE];
    int messageNumber = 1, validUser = 0;
    // Receive "START" command
    receiveResponse(clientSocket, buffer);
    if (strncmp(buffer, "START", 5) != 0) {
        printf("Error: Expected START command, but received: %s", buffer);
        sendCommand(clientSocket, "-ERR Invalid command\r\n");
        return;
    }

    // Send greeting message
    sendCommand(clientSocket, "+OK POP3 server ready\r\n");

    // Receive USER command
    receiveResponse(clientSocket, buffer);
    if (strncmp(buffer, "USER", 4) != 0) {
        printf("Error: Expected USER command, but received: %s", buffer);
        sendCommand(clientSocket, "-ERR Invalid command\r\n");
        return;
    }

    // Extract username from the USER command
    sscanf(buffer, "USER <%[^>]", username);

    // printf("user name: %s\n", username);

    userFile = fopen("./user.txt", "r");
    if (!userFile) {
        char invalid_file[] = "-ERR user file not existing.\r\n";
        send(clientSocket, invalid_file, sizeof(invalid_file), 0);
        perror("Error opening mailbox file");
        return;
    }
    validUser = 0;
    memset(buffer, 0, MAX_BUFFER_SIZE);
    while (fscanf(userFile, "%s %s", buffer, correct_password) == 2) {
        if (strcmp(username, buffer) == 0) {
            validUser = 1;
            break;
        }
    }
    fclose(userFile);

    if (!validUser) {
        char invalidSender[] = "-ERR No such user\r\n";
        sendCommand(clientSocket, invalidSender);
        return;
    }

    // Send OK response
    sendCommand(clientSocket, "+OK\r\n");

    // Receive PASS command
    receiveResponse(clientSocket, buffer);
    if (strncmp(buffer, "PASS", 4) != 0) {
        printf("Error: Expected PASS command, but received: %s", buffer);
        sendCommand(clientSocket, "-ERR Invalid command\r\n");
        return;
    }

    // Extract password from the PASS command
    sscanf(buffer, "PASS <%[^>]", password);

    // printf("password entered: %s\n", password);
    // printf("correct one: %s\n", correct_password);

    // Check if the password matches
    if (strcmp(password, correct_password) != 0) {
        char invalidPassword[] = "-ERR Invalid password\r\n";
        sendCommand(clientSocket, invalidPassword);
        return;
    }

    // Send OK response
    sendCommand(clientSocket, "+OK Authentication successful\r\n");

    // Enter TRANSACTION state with locking
    memset(filePath, 0, MAX_BUFFER_SIZE);
    snprintf(filePath, sizeof(filePath), "./%s/lock_file.txt", username);

    // Open the lock file in read mode
    lockFile = fopen(filePath, "r");

    if (lockFile == NULL) {
        // Lock file doesn't exist, mailbox is not locked
        lockFile = fopen(filePath, "w+");
        if (lockFile == NULL) {
            perror("Error opening lock file");
            sendCommand(clientSocket, "-ERR Unable to open lock file\r\n");
            return;
        }

        // Acquire the lock
        fprintf(lockFile, "1\n");
        fflush(lockFile);
        sendCommand(clientSocket, "+OK Lock acquired, entering TRANSACTION state\r\n");
    } else {
        // Lock file exists, check the lock status
        int lockStatus;
        lockFile = fopen(filePath, "r+");
        if (fscanf(lockFile, "%d", &lockStatus) != 1) {
            // Unable to read lock status
            perror("Error reading lock status");
            sendCommand(clientSocket, "-ERR Unable to read lock status\r\n");
            fclose(lockFile);
            return;
        }

        if (lockStatus == 1) {
            // Mailbox is already locked
            sendCommand(clientSocket, "-ERR Mailbox is locked by another process\r\n");
            fclose(lockFile);
            return;
        }

        // Acquire the lock
        rewind(lockFile);
        fprintf(lockFile, "1\n");
        fflush(lockFile);

        // Close the lock file
        fclose(lockFile);
        sendCommand(clientSocket, "+OK Lock acquired, entering TRANSACTION state\r\n");
    }

    int continue_list = 1, mailNumber;

    while (continue_list) {
        receiveResponse(clientSocket, buffer);
        printf("received %s\n", buffer);
        if (strncmp(buffer, "LIST", 4) == 0) {

        // Read the mymailbox file and get the list of mails
        memset(filePath, 0, MAX_BUFFER_SIZE);
        snprintf(filePath, sizeof(filePath), "./%s/mymailbox.txt", username);
        mailbox = fopen(filePath, "r");
        if (mailbox == NULL) {
            sendCommand(clientSocket, "-ERR Unable to open mailbox\r\n");
            fclose(mailbox);
            continue_list = 0;
            continue;
        }

        // Send initial OK response for LIST command
        sendCommand(clientSocket, "+OK\r\n");

        // Iterate through the mailbox and send the list of mails
        messageNumber = 0;
        while (fgets(buffer, MAX_BUFFER_SIZE, mailbox) != NULL) {
            char senderEmail[MAX_BUFFER_SIZE];
            char timestamp[MAX_BUFFER_SIZE];
            char subject[MAX_BUFFER_SIZE];

            // Process each line to get the required information
            if (strncmp(buffer, "From:", 5) == 0) {
                memset(senderEmail, 0, MAX_BUFFER_SIZE);
                sscanf(buffer, "From: %[^\n]", senderEmail);
            } else if (strncmp(buffer, "Received:", 9) == 0) {
                memset(timestamp, 0, MAX_BUFFER_SIZE);
                sscanf(buffer, "Received: %[^\n]", timestamp);
            } else if (strncmp(buffer, "Subject:", 8) == 0) {
                memset(subject, 0, MAX_BUFFER_SIZE);
                sscanf(buffer, "Subject: %[^\n]", subject);
            } else if (strcmp(buffer, ".\n") == 0) {
                // End of mail entry, send the formatted list entry
                messageNumber++;
                memset(buffer, 0, MAX_BUFFER_SIZE);
                snprintf(buffer, sizeof(buffer), "%d %s %s %s\r\n", messageNumber, senderEmail, timestamp, subject);
                sendCommand(clientSocket, buffer);
                // printf("sent message %d\n", messageNumber);
            }
        }

        printf("sent full list\n");
        // Mark the end of the list with a dot
        sendCommand(clientSocket, ".\n");
        fclose(mailbox);
        } 
        else if (strncmp(buffer, "RETR", 4) == 0) {
        // Extract mail number from the RETR command

        sscanf(buffer, "RETR %d\r\n", &mailNumber);
        // printf("got %d\n", mailNumber);

        // Check if the mail number is within the valid range
        if (mailNumber <= 0 || mailNumber > messageNumber) {
            // Send an error response for out-of-range mail number
            sendCommand(clientSocket, "-ERR Mail number out of range\r\n");
            continue_list = 0;
            break;
        } else {
            // Send +OK response
            sendCommand(clientSocket, "+OK\r\n");

        memset(filePath, 0, MAX_BUFFER_SIZE);
        snprintf(filePath, sizeof(filePath), "./%s/mymailbox.txt", username);
        mailbox = fopen(filePath, "r");

            // Seek to the beginning of the mailbox file
            fseek(mailbox, 0, SEEK_SET);

            // Iterate through the mailbox to find the specified mail
            int currentMail = 1, val = 1;
            while (currentMail < mailNumber) {
                memset(buffer, 0, MAX_BUFFER_SIZE);
                fgets(buffer, MAX_BUFFER_SIZE, mailbox) ;
                if (strcmp(buffer, ".\n") == 0) {
                    currentMail++;
                }
            }
            memset(buffer, 0, MAX_BUFFER_SIZE);
            // printf("starting to send\n");
            while ( val && fgets(buffer, MAX_BUFFER_SIZE, mailbox) != NULL) {
                sendCommand(clientSocket, buffer);
                // Check if the received line contains a single dot
                if (strcmp(buffer, ".\n") == 0) {
                    val = 0;
                }
                memset(buffer, 0, MAX_BUFFER_SIZE);
            }
            fclose(mailbox);
        }
        } 
        else if (strncmp(buffer, "DELE", 4) == 0) {
        // Handle DELE command

        // Open the mailbox file
        memset(filePath, 0, MAX_BUFFER_SIZE);
        snprintf(filePath, sizeof(filePath), "./%s/mymailbox.txt", username);
        mailbox = fopen(filePath, "r");

        fseek(mailbox, 0, SEEK_SET);        
        memset(newfilePath, 0, MAX_BUFFER_SIZE);
        snprintf(newfilePath, sizeof(newfilePath), "./%s/temp_mymailbox.txt", username);
        FILE *newMailbox = fopen(newfilePath, "w+");

        if (newMailbox == NULL) {
            perror("Error opening mailbox file");
            sendCommand(clientSocket, "-ERR Unable to open mailbox file\r\n");
            fclose(mailbox);
            continue_list = 0;
            break;
        }

        // Copy contents of the mailbox to a new file, excluding the marked mail
        int currentMail = 1;
        // copy before mails
        while (currentMail < mailNumber) {
            memset(buffer, 0, MAX_BUFFER_SIZE);
            fgets(buffer, MAX_BUFFER_SIZE, mailbox) ;
            fputs(buffer, newMailbox);
            if (strcmp(buffer, ".\n") == 0) {
                currentMail++;
            }
        }
        // leave that mail
        while (currentMail == mailNumber) {
            memset(buffer, 0, MAX_BUFFER_SIZE);
            fgets(buffer, MAX_BUFFER_SIZE, mailbox) ;
            if (strcmp(buffer, ".\n") == 0) {
                currentMail++;
            }
        }
        // copy after mails 
        memset(buffer, 0, MAX_BUFFER_SIZE);
        while ( fgets(buffer, MAX_BUFFER_SIZE, mailbox) != NULL) {
            fputs(buffer, newMailbox);
            memset(buffer, 0, MAX_BUFFER_SIZE);
        }

        // Close the files
        fclose(mailbox);
        fclose(newMailbox);

        // Replace the original mailbox file with the new file
        remove(filePath);
        rename(newfilePath, filePath);

        // Inform the client about the deletion
        sendCommand(clientSocket, "+OK Message deleted\r\n");
        }
        else if (strncmp(buffer, "QUIT", 4) == 0) {
            continue_list = 0;
        } else {
            // Invalid command
            sendCommand(clientSocket, "-ERR Invalid command\r\n");
            continue_list = 0;
        }
    }

    // Before leaving TRANSACTION state, unlock by setting the lock to 0
    memset(filePath, 0, MAX_BUFFER_SIZE);
    snprintf(filePath, sizeof(filePath), "./%s/lock_file.txt", username);
    lockFile = fopen(filePath, "w");
    if (lockFile == NULL) {
        perror("Error opening lock file");
        sendCommand(clientSocket, "-ERR Unable to open lock file for unlocking\r\n");
        return;
    }
    fprintf(lockFile, "0\n");
    fclose(lockFile);

    return; 
}

