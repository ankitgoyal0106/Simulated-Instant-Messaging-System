#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

// defining global variables
#define NUM_USERS 3
#define NUM_MSG 3

// Advanced sleep which will not be interfered by signals.
struct timespec req, rem;

// initializing 6 pipes for two-way communication between the user process and the server process.
int server_to_user[NUM_USERS][2];
int user_to_server[NUM_USERS][2];
// initializing an array to store the pids.
int user_arr[NUM_USERS];

// initializing an array of variables to count the number of messages sent by server to each user
volatile sig_atomic_t msg_num_server[NUM_USERS];

// initializing an array of variables to count the number of messages sent by each user.
volatile sig_atomic_t msg_num_usrs[NUM_USERS];

// the count is incremented once a message and a signal is sent.
// extracting the user_id since sig_num - SIGRTMIN = user_id.
void Handle_new_messages_from_user(int sig_num){
    ++msg_num_usrs[sig_num - SIGRTMIN];
}

// extracting the user_id since sig - SIGRTMIN - NUM_USERS = user_id.
// To handle the messages from the server back to the users, we need a separate set of signals,
// to avoid collision with the first set of signals. Therefore i am doing (- NUM_USERS).
void Handle_new_messages_from_server(int sig){
    ++msg_num_server[sig - SIGRTMIN - NUM_USERS];
}

// these functions are used to register signal handlers.
void setup_signal_handlers() {
    struct sigaction sa;
    for(int i = 0; i < NUM_USERS; i++){
        msg_num_usrs[i] = 0;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = Handle_new_messages_from_user;
        sa.sa_flags = SA_RESTART;
        if(sigaction(SIGRTMIN + i, &sa, NULL)){
            // error calling sigaction.
            exit(1);
        }
    }
}
void setup_signal_handlers_server() {
    struct sigaction sa;
    for(int i = 0; i < NUM_USERS; i++){
        msg_num_server[i] = 0;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = Handle_new_messages_from_server;
        sa.sa_flags = SA_RESTART;
        if(sigaction(SIGRTMIN + NUM_USERS + i, &sa, NULL)){
            // error calling sigaction.
            exit(1);
        }
    }
}
// initializing an array representing to get sender's id.
// user 1 will send message to user 1, 2 and 3.
// user 2 will send message to user 1 and 3.
// user 3 will send message to user 1. 
int receiver[NUM_USERS][NUM_MSG] = {
    {1, 2, 3},
    {1, 3, 0},
    {1, 0, 0}
};
// user handler function
void user_handler_function(int user_id, int write_pipe, int read_pipe){
    // calling the signal handler for server to user.
    setup_signal_handlers_server(); 
    // array of messages each user want to send.
    char *user_message[NUM_USERS][NUM_MSG] = {
        {"All men are mortal.", "Socrates is a man.", "Therefore, Socrates is mortal."},
        {"We know what we are,", "But know not what we may be.", NULL},
        {"We wish you a prosperous Year of the Dragon!", NULL, NULL}
    };
    
    char buff_msg[256];
    for(int i = 0; i < NUM_MSG; i++){
        if(user_message[user_id][i] != NULL){
            char msg[128];
            // copying the user_message to msg.
            strcpy(msg,user_message[user_id][i]);
            sprintf(buff_msg, "%d_to_%d: %s", user_id + 1, receiver[user_id][i], msg);
            // padding the buff_msg if the length of buff_msg < 128.
            int len1 = strlen(buff_msg);
            for(int j = len1; j < 128; j++){
                buff_msg[j] = '\0';
            }

            int x = write(write_pipe, buff_msg, 128);  
            if(x == -1){
                perror("parent failed to write to pipe");
                exit(1);
            }   
            // send signal to the server.
            kill(getppid(),SIGRTMIN + user_id);     

            // sleep for some time.
            req.tv_sec = 2; // time to sleep in seconds.
            req.tv_nsec = 0; // Additional time to sleep in nanoseconds.
            while(nanosleep(&req, &rem) == -1){
                if(errno == EINTR){
                    req = rem;
                }
                else{
                    printf("sleep failed.\n");
                    exit(1);
                }
            }    
        }
        else{
            break;
        }
    } 
    // closing the write end of the pipe and sending a signal to the server. 
    // (for this signal the read() will return 0.)
    close(write_pipe);
    kill(getppid(),SIGRTMIN + user_id); 
    // reading the incoming message from the server.
    char to_msg[128];
    int total_users = 1;
    // initializing a variable to get the sender id from the incoming message.
    int ssender;
    while(total_users > 0){
        for(int j = 0; j < NUM_USERS; j++){
            if(msg_num_server[j] > 0){
                int r = read(read_pipe, to_msg, sizeof(to_msg));
                if(r > 0){
                    ssender = to_msg[0] - '0';
                    printf("user %d received message from user %d: %s\n\n",j+1, ssender, &to_msg[8]);
                }
                else{
                    // No more messages from the server to user.
                    close(read_pipe);
                    total_users--;
                }
                --msg_num_server[j];
            }
        }
    }
}
// server handler function.
void server_handler_function(){
    // sleep for some time
    req.tv_sec = 2; // time to sleep in seconds.
    req.tv_nsec = 0; // Additional time to sleep in nanoseconds.
    while(nanosleep(&req, &rem) == -1){
        if(errno == EINTR){
            req = rem;
        }
        else{
            printf("sleep failed.\n");
            exit(1);
        }
    }

    char from_msg[128];
    int total_users = NUM_USERS;
    // initializing two variables to get the sender's id and the receiver's id from the incoming message.
    int receiver;
    int sender;
    while(total_users > 0){
        for(int i = 0; i < NUM_USERS; i++){
            if(msg_num_usrs[i] > 0){
                // read the message sent by the user
                if(read(user_to_server[i][0], from_msg, sizeof(from_msg)) > 0){
                    // store the sender's and receiver's id.
                    // '1' - '0' = 1 (in int)
                    sender = from_msg[0] - '0';
                    receiver = from_msg[5] - '0';

                    printf("user %d sent message to user %d: %s\n\n",sender,receiver,&from_msg[8]);

                    // send the message back to the receiver.
                    // write to receiver's pipe.
                    int w = write(server_to_user[receiver - 1][1], from_msg, 128);
                    if(w == -1){
                        perror("failed to write to the pipe");
                        exit(1);
                    }  
                    // send the signal.
                    kill(user_arr[receiver - 1],SIGRTMIN + NUM_USERS + (receiver - 1));

                }
                else {
                    //printf("Not getting any message from the user %d.\n\n",i);
                    // closing the reading end 
                    close(user_to_server[i][0]);
                    
                    --total_users;
                }
                --msg_num_usrs[i];
            }
        }
    }
    // closing the write end of the server_to_user pipe and sending the signal.
    for(int i = 0; i < NUM_USERS; i++){
        close(server_to_user[i][1]);
        kill(user_arr[i],SIGRTMIN + NUM_USERS + i);
    }
}

int main(){
    // creating the pipes.
    for(int i = 0; i < NUM_USERS; i++){
        if(pipe(server_to_user[i]) == -1 || pipe(user_to_server[i]) == -1){
            perror("could not create pipe");
            exit(1);
        }
    }
    
    pid_t pid;
    // registering signal handlers before fork()
    setup_signal_handlers(); // user to server.
    
    
    // using fork() for each user_processes.
    for(int i = 0; i < NUM_USERS; i++){
        pid = fork();
        if(pid < 0){
            perror("could not fork");
            exit(1);
        }

        if(pid == 0){
            // child
            // close unused end of the pipes.
            // closing all the pipe ends which are not used by a specific child.
            for(int x = 0; x < NUM_USERS; x++){
                if(x != i){
                    close(server_to_user[x][0]);
                    close(server_to_user[x][1]);
                    close(user_to_server[x][0]);
                    close(user_to_server[x][1]);
                }
            }
            
            close(server_to_user[i][1]); // close the write end of the server_to_user pipe
            close(user_to_server[i][0]); // close the read end of the user_to_server pipe 
            user_handler_function(i, user_to_server[i][1], server_to_user[i][0]);
            return 0;
        }
        else{
            // parent
            // close unused end of the pipes.
            // storing the user_id
            user_arr[i] = pid;
            close(server_to_user[i][0]); // close the read end of the server_to_user pipe 
            close(user_to_server[i][1]); // close the write end of the user_to_server pipe.
        }

        
    }   
    server_handler_function();
    // wait for child process to complete.
    int child_status;
    for(int i = NUM_USERS - 1; i >= 0; i--){
        pid_t wpid = waitpid(user_arr[i], &child_status, 0);
	    if (WIFEXITED(child_status)){
	        printf("Child %d terminated with exit status %d\n", wpid, WEXITSTATUS(child_status));
        }
	    else{
	        printf("Child %d terminated abnormally\n", wpid);
        }
    }
    return 0;
}
