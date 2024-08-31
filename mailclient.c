#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <regex.h>

#define MAX_BUFFER_SIZE 1024

int validateEmailFormat(char *email) ;
void sendMail(int smtpSocket, const char *saved_username) ;
void ManageMail(int pop3Socket, const char *saved_username, const char* saved_password); 

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_IP> <smtp_port> <pop3_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *serverIP = argv[1];
    int smtpPort = atoi(argv[2]);
    printf("smtp port: %d\n", smtpPort);
    int pop3Port = atoi(argv[3]);

    int smtpSocket, pop3Socket;
    struct sockaddr_in smtpAddr, pop3Addr;

    // Create sockets
    if ((smtpSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0 || (pop3Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Unable to create socket\n");
        exit(EXIT_FAILURE);
    }

    // Setup server addresses
    memset(&smtpAddr, 0, sizeof(smtpAddr));
    smtpAddr.sin_family = AF_INET;
	inet_aton(serverIP, &smtpAddr.sin_addr);
    smtpAddr.sin_port = htons(smtpPort);

    memset(&pop3Addr, 0, sizeof(pop3Addr));
    pop3Addr.sin_family = AF_INET;
	inet_aton(serverIP, &pop3Addr.sin_addr);
    pop3Addr.sin_port = htons(pop3Port);

    // Connect to SMTP server
    if (connect(smtpSocket, (struct sockaddr *)&smtpAddr, sizeof(smtpAddr)) < 0) {
        perror("Error connecting to SMTP server\n");
        exit(EXIT_FAILURE);
    }

    // Connect to POP3 server 
    if (connect(pop3Socket, (struct sockaddr *)&pop3Addr, sizeof(pop3Addr)) < 0) {
        perror("Error connecting to POP3 server\n");
        exit(EXIT_FAILURE);
    }

    char username[MAX_BUFFER_SIZE];
    char password[MAX_BUFFER_SIZE];
    char buffer[MAX_BUFFER_SIZE];
    
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);

    int choice;

    do {
        // Display menu and get user choice
        printf("\nOptions:\n");
        printf("1. Manage Mail\n");
        printf("2. Send Mail\n");
        printf("3. Quit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                // Implement logic for managing mails (display stored mails)
                ManageMail(pop3Socket, username, password);
                break;
            case 2:
                // Send Mail
                sendMail(smtpSocket,username); 
                break;
            case 3:
                // Quit
                printf("Goodbye\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 3);

    // Cleanup and close sockets

    close(smtpSocket);
    close(pop3Socket);

    return 0;
}


// Function to validate email format
int validateEmailFormat(char *email) {
    regex_t regex;
    int reti;

    // Define the regular expression for email validation
    char pattern[] = "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9._%+-]";

    // Compile the regular expression
    reti = regcomp(&regex, pattern, REG_EXTENDED);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        exit(EXIT_FAILURE);
    }

    // Execute the regular expression
    reti = regexec(&regex, email, 0, NULL, 0);
    regfree(&regex);

    return reti;  // 0 for valid format, non-zero for invalid format
}


void sendMail(int smtpSocket, const char *saved_username) {
    char from[MAX_BUFFER_SIZE];
    char to[MAX_BUFFER_SIZE];
    char subject[MAX_BUFFER_SIZE];
    char fromUsername[MAX_BUFFER_SIZE];
    char toUsername[MAX_BUFFER_SIZE];

    // Send mail using SMTP commands
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    strcpy(buffer, "START\n");
    send(smtpSocket, buffer, strlen(buffer), 0);

    memset(buffer, 0, sizeof(buffer));
    recv(smtpSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "550", 3) == 0) {
        printf("Error: %s", buffer);
        return;
    }

    // HELO command
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "HELO localhost\r\n");
    send(smtpSocket, buffer, strlen(buffer), 0);

    memset(buffer, 0, sizeof(buffer));
    recv(smtpSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "550", 3) == 0) {
        printf("Error: %s", buffer);
        return;
    }

    // Take user input for mail details
    printf("From: ");
    scanf("%s", from);

    if (validateEmailFormat(from) != 0) {
        printf("Incorrect format. Please enter valid email addresses.\n");
                memset(buffer, 0, sizeof(buffer));
                snprintf(buffer, sizeof(buffer), "QUIT\r\n");  
                send(smtpSocket, buffer, strlen(buffer), 0);
        return;
    }
    sscanf(from, "%[^@]", fromUsername);  // Extracts characters until '@' into fromUsername


    if (strcmp(fromUsername, saved_username) != 0) {
        printf("login User: %s\nFrom User:%s\nDon't Match\nCan't send mail if both don't match.\n",saved_username,fromUsername);
                memset(buffer, 0, sizeof(buffer));
                snprintf(buffer, sizeof(buffer), "QUIT\r\n");  
                send(smtpSocket, buffer, strlen(buffer), 0);
        return;  // Return from the function
    }  


    // MAIL FROM command
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "MAIL FROM: %s\r\n", fromUsername);
    send(smtpSocket, buffer, strlen(buffer), 0);

    memset(buffer, 0, sizeof(buffer));
    recv(smtpSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "550", 3) == 0) {
        printf("%s not found\n",fromUsername);
        return;  // Return from the function
    }  

    printf("To: ");
    scanf("%s", to);

    if (validateEmailFormat(to) != 0) {
        printf("Incorrect format. Please enter valid email addresses.\n");
                memset(buffer, 0, sizeof(buffer));
                snprintf(buffer, sizeof(buffer), "QUIT\r\n");  
                send(smtpSocket, buffer, strlen(buffer), 0);
        return;
    }
    sscanf(to, "%[^@]", toUsername);      // Extracts characters until '@' into toUsername

    // RCPT TO command
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "RCPT TO: %s\r\n", toUsername);
    send(smtpSocket, buffer, strlen(buffer), 0);

    memset(buffer, 0, sizeof(buffer));
    recv(smtpSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "550", 3) == 0) {
        printf("%s",buffer);
        return;  // Return from the function
    }    


    // DATA command
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "DATA\r\n");
    send(smtpSocket, buffer, strlen(buffer), 0);

    memset(buffer, 0, sizeof(buffer));
    recv(smtpSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "354", 3) != 0) {
        printf("%s",buffer);
        return;  // Return from the function
    }   

    // Send mail content
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "From: %s\nTo: %s\r\n", from, to);
    send(smtpSocket, buffer, strlen(buffer), 0);

    printf("Subject: ");
    getchar(); 
    fgets(subject, sizeof(subject), stdin);

    // Validate length of subject
    if (strlen(subject) > 50) {
        printf("Subject string exceeds maximum length of 50 characters.\n");
                memset(buffer, 0, sizeof(buffer));
                snprintf(buffer, sizeof(buffer), "QUIT\r\n");  
                send(smtpSocket, buffer, strlen(buffer), 0);
        return;
    }

    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Subject: %s",subject);
    send(smtpSocket, buffer, strlen(buffer), 0);

    printf("Enter your message (end with a line containing only a dot):\n");

    int lineCount = 0;
    while (1) {
        char line[MAX_BUFFER_SIZE];
        memset(line, 0, MAX_BUFFER_SIZE);
        fgets(line, MAX_BUFFER_SIZE, stdin);

        // Validate message body line length
        if (strlen(line) > 80) {
            printf("Line %d exceeds maximum length of 80 characters.\n", lineCount + 1);
            memset(buffer, 0, sizeof(buffer));
            snprintf(buffer, sizeof(buffer), "QUIT\r\n");  
            send(smtpSocket, buffer, strlen(buffer), 0);
            return;
        }

        send(smtpSocket, line, MAX_BUFFER_SIZE, 0);

        if (strcmp(line, ".\n") == 0) {
            break;  // End of message
        }
        lineCount++;

        // Validate maximum number of lines
        if (lineCount > 50) {
            printf("Exceeded maximum number of lines (50).\n");
                memset(buffer, 0, sizeof(buffer));
                snprintf(buffer, sizeof(buffer), "QUIT\r\n");  
                send(smtpSocket, buffer, strlen(buffer), 0);
            return;
        }
    }

    printf("Message ended.\n");

    memset(buffer, 0, sizeof(buffer));
    recv(smtpSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "250", 3) != 0) {
        printf("%s",buffer);
        return;  // Return from the function
    }  

    // QUIT command
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "QUIT\r\n");
    send(smtpSocket, buffer, strlen(buffer), 0);
    printf("Mail sent successfully.\n");

    return;
}


void sendCommand(int Socket, const char *command) {
    send(Socket, command, strlen(command), 0);
}

void receiveResponse(int Socket, char *buffer) {
    memset(buffer, 0, MAX_BUFFER_SIZE);
    char c;
    int i = 0;

    // Read one character at a time until a newline is encountered
    while (recv(Socket, &c, 1, 0) > 0) {
        buffer[i++] = c;
        if (c == '\n') {
            break;
        }
    }
}

void ManageMail(int pop3Socket, const char *saved_username, const char *saved_password) {
    char buffer[MAX_BUFFER_SIZE];

    // Send "START" command
    sendCommand(pop3Socket, "START\r\n");

    // Listen for POP3 server readiness
    receiveResponse(pop3Socket, buffer);

    // If ready, proceed with authentication
    if (strncmp(buffer, "+OK", 3) == 0) {
        
        // Send USER command with <username> (enclosed with <>)
        memset(buffer, 0, MAX_BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer), "USER <%s>\r\n", saved_username);
        sendCommand(pop3Socket, buffer);

        // Receive OK response (check)
        receiveResponse(pop3Socket, buffer);
        if (strncmp(buffer, "+OK", 3) != 0) {
            printf("Error: Authentication failed. Response: %s", buffer);
            return;
        }

        // Send PASS command with <password> (enclosed with <>)
        memset(buffer, 0, MAX_BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer), "PASS <%s>\r\n", saved_password);
        sendCommand(pop3Socket, buffer);

        // Receive OK response (check)
        receiveResponse(pop3Socket, buffer);
        if (strncmp(buffer, "+OK", 3) != 0) {
            printf("Error: Authentication failed. Response: %s", buffer);
            return;
        }

        // Receive response for transition to TRANSACTION state
        receiveResponse(pop3Socket, buffer);
        if (strncmp(buffer, "+OK", 3) != 0) {
            printf("Error: Failed to enter TRANSACTION state. Response: %s", buffer);
            return;
        }

        // Now, we are in the TRANSACTION state

        // List mails
        do {

        sendCommand(pop3Socket, "LIST\r\n");
        receiveResponse(pop3Socket, buffer);
        if (strncmp(buffer, "+OK", 3) != 0) {
            printf("Error: Failed to list mails. Response: %s", buffer);
            return;
        }
        printf("List of mails:\n");
        int val = 1; 
        while (val) {
            // printf("before : %d\n", val);
            receiveResponse(pop3Socket, buffer);
            // printf("after : %d\n", val);
            // Check if the received line contains a single dot
            if (strcmp(buffer, ".\n") == 0) {
                   val = 0;
            }
            else 
            { printf("%s", buffer); val++; }
        }

        // Get user input for mail number
        int mailNumber;
        printf("Enter mail no. to see (-1 to go back to main menu):\n");
        scanf("%d", &mailNumber);

       //  printf("mail number: %d\n", mailNumber);

        if (mailNumber == -1) {
             sendCommand(pop3Socket, "QUIT\r\n");
            /*
            receiveResponse(pop3Socket, buffer);
            if (strncmp(buffer, "+OK", 3) != 0) {
                printf("Error: Response: %s", buffer);
            } 
            */
            return ;
        }
        else {
        // Send RETR command for the chosen mail number
        memset(buffer, 0, MAX_BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer), "RETR %d\r\n", mailNumber);
        sendCommand(pop3Socket, buffer);
        // printf("sent mail number\n");

        // Receive the mail content
        // printf("waiting for mail content \n");

        receiveResponse(pop3Socket, buffer);
        if (strncmp(buffer, "+OK", 3) != 0) {
            printf("Error: Failed to retrieve mail. Response: %s", buffer);
            return;
        }
        printf("Mail Content: \n");
        val = 1;
        // Display the mail content
        while (val) {
            receiveResponse(pop3Socket, buffer);
            // Check if the received line contains a single dot
            // printf("before %d\n", val);
            printf("%s", buffer);
            if (strcmp(buffer, ".\n") == 0) {
                val = 0;
                // printf("out\n");
            }
            // printf("after %d\n", val);
        }

        // Wait for user input after showing the mail
        printf("Press 'd' to delete or any other key to go back to the list: ");
        char userChoice;
        scanf(" %c", &userChoice);

        if (userChoice == 'd') {
            // User wants to delete the mail
            // Send DELE command
            memset(buffer, 0, MAX_BUFFER_SIZE);
            snprintf(buffer, sizeof(buffer), "DELE %d\r\n", mailNumber);
            sendCommand(pop3Socket, buffer);

            // Receive the response
            receiveResponse(pop3Socket, buffer);
            if (strncmp(buffer, "+OK", 3) != 0) {
                printf("Error: Failed to delete mail. Response: %s", buffer);
                return;
            }
            printf("Mail deleted. Response: %s", buffer);
        }
        } 
        } while(1); 

        // closing connection.
        sendCommand(pop3Socket, "QUIT\r\n");
        return ;
}
        printf("Pop server not Ready: %s", buffer);
        return ;
}