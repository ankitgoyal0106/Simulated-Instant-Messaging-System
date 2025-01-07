# Simulated-Instant-Messaging-System

This repository contains the implementation of a simulated instant messaging system using processes, pipes, and signals in C. The project demonstrates process manipulation, inter-process communication, and signal handling concepts.

---

## Project Overview

This project simulates an instant messaging system where a central server acts as a relay node connecting three users. Messages sent from one user to another are routed through the server, which handles parsing and forwarding. The communication is achieved using Unix pipes for message transfer and real-time signals for notifications.

---

## Features

- **Process Creation**: The main server process spawns three child processes representing users.
- **Two-Way Communication**: Each user communicates with the server through a pair of pipes.
- **Real-Time Signal Handling**: Signals are used to notify the server of incoming messages and users of forwarded messages.
- **Message Formatting**: Messages include sender and receiver IDs for routing.
- **Error Handling**: Includes measures to prevent message loss, race conditions, and system call interruptions.
- **Post-Processing**: Ensures clean-up of all resources, including closing pipes and reaping child processes.

---

## Implementation Details

### Global Variables
- `NUM_USERS`, `NUM_MSG`: Constants defining the number of users and messages.
- `server_to_user`, `user_to_server`: Arrays representing pipes for communication.
- `user_arr`: Stores process IDs (PIDs) of users.
- `msg_num_server`, `msg_num_usrs`: Counters for messages sent by the server and users.

### Signal Handlers
- **User to Server**: `Handle_new_messages_from_user` increments the user message count when a signal is received.
- **Server to User**: `Handle_new_messages_from_server` increments the server message count when a signal is received.
- Handlers ensure minimal operation to avoid race conditions.

### Communication Logic
- **Users**:
  - Send predefined messages to the server via pipes.
  - Receive forwarded messages from the server.
  - Handle signals for notification of incoming messages.
- **Server**:
  - Reads messages from users, parses sender and receiver IDs, and forwards them appropriately.
  - Notifies users of incoming messages via signals.

### Post-Processing
- Ensures all pipes are closed.
- Reaps all child processes to prevent zombies.

---

## Setup and Compilation

1. Clone the repository.
2. Ensure you have a C compiler installed.
3. Run the following command to compile the project:
   ```bash
   make project5
   ```

---

## Execution

After compiling, execute the program with:
```bash
./process_signals
```

---

## File Structure

- **`process_signals.c`**: The main source code for the project.
- **`Makefile`**: Automates the compilation process.
- **`README.md`**: Project documentation.
---

## Learning Outcomes

This project provided hands-on experience with:
- Creating and managing processes in C.
- Using Unix pipes for inter-process communication.
- Implementing robust signal handling mechanisms.
- Understanding race conditions and implementing solutions.

---

## License

This project is licensed under the MIT License. See `LICENSE` for more details.

