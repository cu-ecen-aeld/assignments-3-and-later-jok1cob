#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <stdbool.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#define BUFF_SIZE 512
static bool kill_app = false;

void SigHandler(int signo){
    switch (signo) {
        case SIGINT:
            syslog(LOG_DEBUG, "Caught signal SIGINT, exiting aesdsocket");
            printf("Caught signal SIGINT, exiting aesdsocket\n");
            kill_app = true;
            break;
        case SIGTERM : 
            syslog(LOG_DEBUG, "Caught signal SIGTERM, exiting aesdsocket");
            printf("Caught signal SIGTERM, exiting aesdsocket\n");
            kill_app = true;
            break;  
        default :
            break;

    }
}

int main(int argc, char* argv[]) {

    bool daemon = false;
    int rc;
    int status;
    pid_t pid;
    struct sigaction sa = {0};
    int socketfd;
    
    struct addrinfo *res; 
    struct addrinfo hints;
    int yes = 1;

    struct sockaddr_storage their_addr = {0};
    socklen_t addr_size = sizeof (their_addr);
    int accept_fd = 0;
    FILE* txtfd = NULL;
    char* ret = NULL;
    char buff[BUFF_SIZE] = {0};

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // use IPv4 
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    sa.sa_handler = SigHandler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    openlog(NULL, 0, LOG_USER);

    if( (argc == 2) && (strcmp(argv[1],"-d") == 0) ) {
        syslog(LOG_DEBUG, "Trying to run aesdsocket as daemon");
        daemon = true;
    }
    else if(argc == 1){
        syslog(LOG_DEBUG, "aesdsocket process started");
    }
    else {
        fprintf(stderr, "\nIncorrect options use ./aesdsocket -d to run as daemon\n");
        fprintf(stderr, "or ./aesdsocket to run as process\n");
        return 0;
    }
    

    if ((status = getaddrinfo(NULL, "9000", &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(socketfd == -1) {
        fprintf(stderr, "could not create socket %s", strerror(errno));
        exit(1);
    }
    

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        exit(1);
    }


    rc = bind(socketfd, res->ai_addr, res->ai_addrlen);
    if(rc < 0) {
        fprintf(stderr, "could not bind socket %s", strerror(errno));
        exit(1);
    }
    
    freeaddrinfo(res);

    if (daemon){
       pid = fork();
       /* parent */
       if(pid > 0) {
        fprintf(stdout, "aesdsocket - Daemon running on PID %d\n", pid);
        return 0;
       }
       else if(pid < 0){
         fprintf(stderr, "Fork failed  %s", strerror(errno));
         return -1;
       }
    }


    rc = listen(socketfd, 5);
    if(rc == -1) {
        fprintf(stderr, "could not bind socket %s", strerror(errno));
        exit(1);
    }

    remove("/var/tmp/aesdsocketdata");

    while(1) {
        if(kill_app)
            break;

        accept_fd = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);
        syslog(LOG_DEBUG, "Accepted connection from %s", inet_ntoa(((struct sockaddr_in*)&their_addr)->sin_addr));        

        txtfd = fopen("/var/tmp/aesdsocketdata", "a+");
        do {
            ret = NULL;   
            rc = recv(accept_fd, (void*) buff, (BUFF_SIZE - 1), 0);  
            fprintf(txtfd, "%.*s", rc, (char*)buff);
            buff[BUFF_SIZE] = '\0'; // adding null at last since valgrind was complaining for strchr not having null termination
            ret = strchr(buff,'\n');
            if(ret != NULL)
                break;            
            else 
                memset(buff, 0, BUFF_SIZE);
        }while(rc > 0);
        fflush(txtfd);
        fclose(txtfd);

        txtfd = fopen("/var/tmp/aesdsocketdata", "r");
        do {
            memset(buff, 0, BUFF_SIZE);
            rc = fread(buff, 1, BUFF_SIZE, txtfd );            
            send(accept_fd, (void*) buff, rc, 0 );
        } while(rc == BUFF_SIZE);
        fclose(txtfd);
        close(accept_fd);
        syslog(LOG_DEBUG, "Closed connection from %s", inet_ntoa(((struct sockaddr_in*)&their_addr)->sin_addr));
    }   
    close(socketfd);

    closelog();
    exit(0);
}