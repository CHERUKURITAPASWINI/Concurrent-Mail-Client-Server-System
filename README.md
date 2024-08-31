# Mail Client-Server System

## Setup

### Server Machine

1. SERVER  {folder}
     - smtpmail.c
     - popserver.c
     - user.txt
     - for user{i} in user.txt: create a folder named with username { eg: user1, user2 } in the SERVER directory.
     - **Server Directory Structure:**
         ```
         SERVER
         ├── smtpmail.c
         ├── popserver.c
         ├── user.txt
         ├── user1
         ├── user2
         ├── user3
         └── ...
         ```     
2. Compile the SMTP server:

    ```bash
    cd path/to/server/folder
    gcc smtpmail.c -o smtp
    ```

    Run the SMTP server:

    ```bash
    ./smtp <smtp_port>
    ```

3. In another terminal, compile the POP3 server:

    ```bash
    cd path/to/server/folder
    gcc popserver.c -o pop3
    ```

    Run the POP3 server:

    ```bash
    ./pop3 <pop3_port>
    ```

4. Ensure that both SMTP and POP3 servers are running.

### Client Machine

1. CLIENT  {folder}
   - mailclient.c

2. Compile the mail client:

    ```bash
    cd path/to/client/folder
    gcc mailclient.c -o client
    ```

    Run the client:

    ```bash
    ./client <server_IP> <smtp_port> <pop3_port>
    ```

3. The client will prompt for a username and password. Enter the login details.

4. The client will show three options:
   - 1. Manage Mail
   - 2. Send Mail
   - 3. Quit

## Usage

- **Manage Mail:**
    - Authentication check (password check) occurs here.
    - Lists received mails, allows viewing and deleting mails.
    - If no mails received, an error message is displayed.
    
- **Send Mail:**
    - Sends mail to another user.
    - Both the sender and receiver must exist in `user.txt`.
    - Both login user_name and from user_name should match.
    - No authentication check (password check) occurs here.

- **Quit:**
    - Exits the mail client.

## Notes

- Both "from" and "to" mail formats are `<user_name>@<domain_name>`.
- For sending mail, users must exist in `user.txt`.
- Authentication is not checked for sending mail.
- Authentication is checked for managing mails.
- Mails are listed with the following information:
    -  Sl. No. <Sender’s email id> <When received, in date : hour : minute> <Subject>
- **Handled Multiple Users:**
    - Multiple users can connect to the same mail server to access their mailboxes.
    - Multiple mail servers can send mail to the single mail server at the same  time.
    - Even if multiple users write to the same user at the same time, the system handles it seamlessly.

## Function Descriptions

### popserver.c

1. **`sendCommand(int Socket, const char *command)`**

   - **Parameters:**
      - `Socket`: The socket descriptor.
      - `command`: The command to be sent to the client.

   - **Description:**
      - Sends a command to the connected client using the specified socket.

2. **`receiveResponse(int Socket, char *buffer)`**

   - **Parameters:**
      - `Socket`: The socket descriptor.
      - `buffer`: A character array to store the received response.

   - **Description:**
      - Receives a response from the connected client using the specified socket and stores it in the provided buffer.

3. **`handleClient(int clientSocket, in_addr_t addr)`**

   - **Parameters:**
      - `clientSocket`: The socket descriptor for the connected client.
      - `addr`: The IP address of the connected client.

   - **Description:**
      - Handles the POP3 protocol communication with the client, managing user authentication, processing commands, and handling mailbox operations.
      - The function continuously listens for commands from the client and responds accordingly.
      - Manages user authentication using the provided username and password.
      - Handles the `LIST`, `RETR`, `DELE`, and `QUIT` commands.
      - Manages a lock file to ensure that only one process can access the mailbox at a time.
      - Performs mailbox operations such as listing mails, retrieving mails, and deleting mails based on client commands.
      - The function ensures proper error handling and responds to the client accordingly.

4. **`main(int argc, char *argv[])`**

   - **Parameters:**
      - `argc`: The number of command-line arguments.
      - `argv`: An array of command-line arguments.

   - **Description:**
      - The main function responsible for setting up the POP3 server.
      - Parses the command-line arguments to get the port number for the server.
      - Creates a socket, binds it to the specified port, and starts listening for incoming connections.
      - Accepts incoming client connections and forks a new process to handle each client.
      - Uses the `handleClient` function to manage the communication with each connected client.

   - **Note:**
      - The main function runs in an infinite loop, continuously accepting client connections and forking processes to handle them.

### smtpmail.c

1. **`sendCommand(int Socket, const char *command)`**

   - **Parameters:**
      - `Socket`: The socket descriptor.
      - `command`: The command to be sent to the client.

   - **Description:**
      - Sends a command to the connected client using the specified socket.

2. **`handleClient(int clientSocket, in_addr_t serv_addr)`**

   - **Parameters:**
      - `clientSocket`: The socket descriptor for the connected client.
      - `serv_addr`: The IP address of the SMTP server.

   - **Description:**
      - Handles the SMTP protocol communication with the client, managing the email sending process.
      - The function continuously listens for commands from the client and responds accordingly.
      - Manages the `START`, `HELO`, `MAIL FROM`, `RCPT TO`, `DATA`, and `QUIT` commands.
      - Cross-checks sender and recipient against the `user.txt` file to verify their existence.
      - Appends the email content to the recipient's mailbox file.
      - Sends appropriate responses to the client based on the commands received.
      - Handles the acknowledgment of the received message.

3. **`main(int argc, char *argv[])`**

   - **Parameters:**
      - `argc`: The number of command-line arguments.
      - `argv`: An array of command-line arguments.

   - **Description:**
      - The main function responsible for setting up the SMTP server.
      - Parses the command-line arguments to get the port number for the server.
      - Creates a socket, binds it to the specified port, and starts listening for incoming connections.
      - Accepts incoming client connections and forks a new process to handle each client.
      - Uses the `handleClient` function to manage the communication with each connected client.

   - **Note:**
      - The main function runs in an infinite loop, continuously accepting client connections and forking processes to handle them.


### mailclient.c

1. **`validateEmailFormat(char *email)` function:**
   - Validates the format of an email address using a regular expression.
   - Returns 0 for a valid format, non-zero for an invalid format.

2. **`sendMail(int smtpSocket, const char *saved_username)` function:**
   - Sends an email using SMTP commands.
   - Collects necessary details from the user (From, To, Subject, and Message).
   - Validates the email format.
   - Communicates with the SMTP server using various SMTP commands.
   - Validates the length of the subject and the number of lines in the message.
   - Sends the email content to the SMTP server.
   - Handles the acknowledgment and termination of the SMTP session.

3. **`sendCommand(int Socket, const char *command)` function:**
   - Sends a command to the server using the specified socket.

4. **`receiveResponse(int Socket, char *buffer)` function:**
   - Receives a response from the server and stores it in the buffer.

5. **`ManageMail(int pop3Socket, const char *saved_username, const char *saved_password)` function:**
   - Manages emails using POP3 commands.
   - Connects to the POP3 server and authenticates the user.
   - Lists the available emails and allows the user to view and delete them.
   - Implements functionalities like listing mails, retrieving mail content, and deleting mails.
   - Handles user input to navigate through the mail management system.
   - Communicates with the POP3 server using various POP3 commands.
   - Supports quitting the mail management system.

6. **`main(int argc, char *argv[])` function:**
   - Takes the server IP, SMTP port, and POP3 port as command-line arguments.
   - Creates sockets for SMTP and POP3 connections.
   - Connects to the SMTP and POP3 servers.
   - Takes the user's username and password.
   - Displays a menu for managing and sending emails, including options to quit the program.

    - **Note:**
        - Ensure that the SMTP and POP3 servers are running and accessible at the specified IP addresses and ports before  running this client.
---

