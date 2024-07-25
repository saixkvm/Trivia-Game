#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h> 
#include <bits/getopt_core.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

void helpmsg(char* program_name){
    printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n", program_name);
    printf("\n");
    printf("-i IP_address Default to \"127.0.0.1\";\n");
    printf("-p port_number Default to 25555;\n");
    printf("-h Display this help info.\n");
}

void parse_connect(int argc, char** argv, int* server_fd){
    int op;
    char ip_address[256] = "127.0.0.1";
    int port_number = 25555;

    while((op = getopt(argc, argv, ":i:p:h")) != -1)
    {
        switch(op)
        {
            case 'i':
                if (strcmp(optarg, "-p") == 0 || strcmp(optarg, "-h") == 0 || strcmp(optarg, "-i")==0){
                    fprintf(stderr, "Error: Cannot use flag as argument\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
                if (strcmp(optarg, "-i") == 0 || strcmp(optarg, "-p") == 0 || strcmp(optarg, "-h") == 0){
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
    struct sockaddr_in server_addr;
    socklen_t addr_size = sizeof(server_addr);

    *server_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(ip_address);

    int i = connect(*server_fd, (struct sockaddr *) &server_addr, addr_size);
}

int main(int argc, char* argv[]) {
    //PARSE CONNECT FUNCTION
    int serve_fd;
    parse_connect(argc, argv, &serve_fd);

    char buffer[4096];
    if (read(serve_fd, buffer, 4096) <= 0) 
    {
        //perror("Max Connection reached cannot join");
        exit(EXIT_FAILURE);
    }
    printf("%s", buffer);
    memset(buffer, '\0', sizeof(buffer));
    scanf("%s", buffer);
    if ((send(serve_fd, buffer, strlen(buffer), 0)) == -1){
        perror("send");
        exit(EXIT_FAILURE);
    }

    fd_set myset;
    FD_SET(serve_fd, &myset);
    int max_fd = serve_fd;

    while(1)
    {
        memset(buffer, '\0', sizeof(buffer));
        FD_ZERO(&myset);
        FD_SET(serve_fd, &myset);
        FD_SET(0, &myset);
        max_fd = serve_fd;

        select(max_fd + 1, &myset, NULL, NULL, NULL);

        if (FD_ISSET(serve_fd, &myset)){
            int rebytes = read(serve_fd, buffer, 4096);
            if (rebytes <= 0){
                break;
            }
            buffer[rebytes] = 0;
            printf("%s\n", buffer);
            fflush(stdout);
        }
        else if (FD_ISSET(0, &myset)){
            char ans[2];
            int ansbytes = read(0, ans, 2);
            if ((ansbytes) <= 0){
                perror("Error in read.\n");
                exit(EXIT_FAILURE);
            }
            buffer[ansbytes] = 0;
            //write(serve_fd, ans, 2);
            if ((write(serve_fd, ans, 2)) == -1){
                perror("Error in write.\n");
                exit(EXIT_FAILURE);
            }
            
        }
    }

    close(serve_fd);
    return 0;
}
