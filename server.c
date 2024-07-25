#include <stdio.h>
#include <unistd.h>
#include <string.h> 
#include <bits/getopt_core.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <limits.h>
#define MAX_CONN 3

struct Player{
    int fd;
    int score;
    char name[128];
};
struct Entry
{
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};
void helpmsg(char* program_name){
    printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n", program_name);
    printf("\n");
    printf("-f question_file Default to \"questions.txt\";\n");
    printf("-i IP_address Default to \"127.0.0.1\";\n");
    printf("-p port_number Default to 25555;\n");
    printf("-h Display this help info.\n");
}
int read_questions(struct Entry* all_questions, char* filename){
    FILE* stream;
    char* line = NULL;
    size_t len = 0;
    ssize_t nread = 0;

    int inc = 0;

    if ((stream = fopen(filename, "r")) == NULL){
        //fprintf(stderr, "Error in opening file.\n");
        perror("Error in opening file.\n");
        exit(EXIT_FAILURE);
    }

    // Fills in the struct Entry array
    while ((nread = getline(&line, &len, stream)) != -1) {
        strcpy(all_questions[inc].prompt, line);
        nread = getline(&line, &len, stream);
        int nargc = 0;
        char* nargv[4096];

        char* token;
        char* delimeter = " ";
        token = strtok(line, delimeter);
        while (token != NULL){
            nargv[nargc] = token;
            token = strtok(NULL, delimeter);
            nargc += 1;
        }
        nargv[nargc] = NULL;

        for(int c = 0; c < 3; c++){
            strcpy(all_questions[inc].options[c], nargv[c]);
            all_questions[inc].options[c][strcspn(all_questions[inc].options[c], "\n")] = '\0';
        }

        nread = getline(&line, &len, stream);
        line[strcspn(line, "\n")] = '\0';

        for (int c = 0; c < 3; c++){
            if (strcmp(line, all_questions[inc].options[c]) == 0){
                all_questions[inc].answer_idx = c;
            }
        }
        nread = getline(&line, &len, stream);
        inc++;
    }
    free(line);
    fclose(stream);
    return inc;
}

int main(int argc, char* argv[])
{
    // ESTABLISH THE SERVER WITH GETOPT 1.1
    int op;
    char question_file[256] = "questions.txt";
    char ip_address[256] = "127.0.0.1";
    int port_number = 25555;

    while((op = getopt(argc, argv, ":f:i:p:h")) != -1)
    {
        switch(op)
        {
            case 'f':
                if (strcmp(optarg, "-i") == 0 || strcmp(optarg, "-p") == 0 || strcmp(optarg, "-h") == 0 || strcmp(optarg, "-f") == 0){
                    fprintf(stderr, "Error: Cannot use flag as argument\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'i':
                if (strcmp(optarg, "-f") == 0 || strcmp(optarg, "-p") == 0 || strcmp(optarg, "-h") == 0 || strcmp(optarg, "-i") == 0){
                    fprintf(stderr, "Error: Cannot use flag as argument\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
                if (strcmp(optarg, "-i") == 0 || strcmp(optarg, "-f") == 0 || strcmp(optarg, "-h") == 0 || strcmp(optarg, "-p") == 0){
                    fprintf(stderr, "Error: Cannot use flag as argument\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                helpmsg(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            case ':':
                fprintf(stderr, "Error: No argument supplied\n");
                exit(EXIT_FAILURE);
                break;
            case '?':
                fprintf(stderr, "Error: Unknown option \'%c\' received\n", optopt);
                exit(EXIT_FAILURE);
                break;
        }
    }

    int server_fd;
    int client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);

    // Creating the socket
    //server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(ip_address);

    //Bind the socket
    int i = bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (i < 0) {
        perror("bind"); 
        exit(EXIT_FAILURE);
    }

    //Listen to 2 connections
    if (listen(server_fd, MAX_CONN) == 0){
        printf("Welcome to 392 Trivia!\n");
    }
    else{
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    // READ THE QUESTION DATABASE 1.2
    struct Entry all_questions[50];
    int numberofquestions = read_questions(all_questions, question_file);

    // ACCEPTING PLAYERS 1.3, Accept connections from clients for communication
    fd_set myset;
    FD_SET(server_fd, &myset);
    int max_fd = server_fd;
    int n_conn = 0;
    int n_names = 0;
    int cfds[MAX_CONN]; // stores all the file descriptors

    int recvbytes = 0;
    char buffer[1024];

    for (int i = 0; i < MAX_CONN; i++){
        cfds[i] = -1;
    }

    struct Player players[MAX_CONN];
    int gamestarted = 0;

    while(1)
    {
        //reset the buffer
        memset(buffer, '\0', sizeof(buffer));
        // need to add all the file descriptors to the set
        //Re-initialization
        if (gamestarted == 0)
        {
            FD_SET(server_fd, &myset);
            max_fd = server_fd;
            for (int i = 0; i < MAX_CONN; i++){
                if (cfds[i] != -1){
                    FD_SET(cfds[i], &myset);
                    if (cfds[i] > max_fd){
                        max_fd = cfds[i];
                    }
                }
            }
            // monitoring fds
            //select(max_fd + 1, &myset, NULL, NULL, NULL); // where we stop our program, to wait for answers
            if ((select(max_fd + 1, &myset, NULL, NULL, NULL)) == -1){
                perror("select");
                exit(EXIT_FAILURE);
            }
            //where we accept a new connection
            if (FD_ISSET(server_fd, &myset))
            {
                client_fd = accept(server_fd, (struct sockaddr*)&in_addr, &addr_size);
                if (n_conn < MAX_CONN){
                    printf("New connection detected!\n");
                    n_conn++;
                    for (int i = 0; i < MAX_CONN; i++){
                        if (cfds[i] == -1){
                            cfds[i] = client_fd;
                            break;
                        }
                    }
                    char typename[1024] = "Please type your name: ";
                    //write(client_fd, typename, 1024);
                    if ((write(client_fd, typename, 1024)) == -1){
                        perror("Error in write.\n");
                        exit(EXIT_FAILURE);
                    }
                }
                else{
                    printf("Max Connection reached!\n");
                    close(client_fd);

                }
            }
            //One of the clients sent a message
            for (int i = 0; i < MAX_CONN; i++)
            {
                //need to check
                if (cfds[i] != -1 && FD_ISSET(cfds[i], &myset))
                {
                    //read the name from client
                    recvbytes = read(cfds[i], buffer, 1024);
                    if (recvbytes <= 0){ // lost the connection
                            printf("Lost Connection!\n");
                            memset(players[cfds[i]].name, '\0', 128);
                            players[cfds[i]].fd = -1;
                            close(cfds[i]);
                            cfds[i] = -1;
                            n_conn--;
                            n_names--;
                    }
                    else
                    {
                        buffer[recvbytes] = 0;
                        strcpy(players[cfds[i]].name, buffer);
                        players[cfds[i]].fd = cfds[i];
                        n_names++;
                        printf("Hi %s\n", buffer);

                        if (n_names == MAX_CONN){
                            gamestarted = 1;
                            printf("The game starts now!\n"); 
                            printf("\n");
                        }
                    }
                }
            }
        }
        else
        {
            break;
        }
    }

    fd_set gameset;
    int max_fd2;
    int iforallquestions = 0;
    char buffer2[4096];

    while(iforallquestions < numberofquestions) 
    {
        FD_ZERO(&myset);
        for (int i = 0; i < MAX_CONN; i++) {
            if (cfds[i] != -1) {
                FD_SET(cfds[i], &myset);
                if (cfds[i] > max_fd2) {
                    max_fd2 = cfds[i];
                }
            }
        }

        printf("Question %d: %s\n", iforallquestions + 1, all_questions[iforallquestions].prompt);
        char question[4096];
        int question_number = iforallquestions;
        sprintf(question, "Question %d: %s", question_number+1, all_questions[iforallquestions].prompt);
        for (int i = 0; i < MAX_CONN; i++) {
            if (cfds[i] != -1) {
                //write(cfds[i], question, 4096);
                if ((write(cfds[i], question, 4096)) == -1){
                    perror("Error in write.\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        char options_combined[4096];
        for (int qs = 0; qs < 3; qs++) {
            printf("%d: %s\n", qs + 1, all_questions[iforallquestions].options[qs]);
            int option_number = qs + 1;

            char option_string[100];
            sprintf(option_string, "Press %d: %s\n", option_number, all_questions[iforallquestions].options[qs]);
            strcat(options_combined, option_string);
        }
        for (int i = 0; i < MAX_CONN; i++) {
            if (cfds[i] != -1) {
                //write(cfds[i], options_combined, 4096);
                if ((write(cfds[i], options_combined, 4096)) == -1){
                    perror("Error in write.\n");
                    exit(EXIT_FAILURE);
                }
            }
        }

        //select(max_fd2 + 1, &myset, NULL, NULL, NULL);
            if ((select(max_fd2 + 1, &myset, NULL, NULL, NULL)) == -1){
                perror("select");
                exit(EXIT_FAILURE);
            }

        for (int i = 0; i < MAX_CONN; i++) {
            if (cfds[i] != -1 && FD_ISSET(cfds[i], &myset)) {
                memset(buffer2, '\0', strlen(buffer2));
                // Process client response
                int answerfromclientbytes = read(cfds[i], buffer2, 2);
                int num = atoi(buffer2);

                if (answerfromclientbytes <= 0) {
                    for(int z = 0; z < MAX_CONN; z++){
                        if (cfds[z] != -1){
                            close(cfds[z]);
                            close(players[cfds[z]].fd);
                            FD_CLR(players[cfds[z]].fd, &gameset);
                        }
                    }
                    close(server_fd);
                    exit(EXIT_SUCCESS);
                }
                if ((num-1) == (all_questions[iforallquestions].answer_idx)) {
                    players[cfds[i]].score++;
                } else {
                    players[cfds[i]].score--;
                }
                // Send correct answer to the clients
                char correctanswer[4096];
                sprintf(correctanswer, "Correct answer: %s\n", all_questions[iforallquestions].options[all_questions[iforallquestions].answer_idx]);
                printf("%s\n", correctanswer);
                for (int j = 0; j < MAX_CONN; j++) {
                    if (cfds[j] != -1) {
                        //write(cfds[j], correctanswer, 4096);
                        if ((write(cfds[j], correctanswer, 4096)) == -1){
                            perror("Error in write.\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                memset(correctanswer, '\0', strlen(correctanswer));
                break;
            }
        }
        memset(options_combined, '\0', strlen(options_combined));
        memset(question, '\0', strlen(question));
        iforallquestions++;
    }

    int max_score = INT_MIN;
    for (int i = 0; i < MAX_CONN; i++){
        if (players[cfds[i]].score >= max_score){
            max_score = players[cfds[i]].score;
        }
    }
    for (int i = 0; i < MAX_CONN; i++){
        if (players[cfds[i]].score == max_score){
            printf("Congrats, %s! You are the winner!\n", players[cfds[i]].name);
        }
    }
    for (int i = 0; i < MAX_CONN; i++){
        if (cfds[i] != -1){
            close(cfds[i]);
        }
    }
    close(server_fd);
    return 0;
}