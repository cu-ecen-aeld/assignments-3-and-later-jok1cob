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
#include <pthread.h>
#include <time.h>
#include "queue.h"

struct node {
    pthread_t tid;
    bool thread_completed;
    int  accept_fd;
    FILE* txt_fd;
    SLIST_ENTRY(node) nodes;
};
SLIST_HEAD(slisthead, node);

#define BUFF_SIZE 512
static bool kill_app = false;
pthread_mutex_t file_access_mtx = PTHREAD_MUTEX_INITIALIZER;

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

void* handle_connection(void* arg) {
    FILE* txtfd = NULL;
    char* ret = NULL;
    char buff[BUFF_SIZE] = {0};
    int rc;

    struct node* n = (struct node *)arg;

    pthread_mutex_lock(&file_access_mtx);
    txtfd = fopen("/var/tmp/aesdsocketdata", "a+");
    n->txt_fd = txtfd;
    do {
        ret = NULL;   
        rc = recv(n->accept_fd, (void*) buff, (BUFF_SIZE - 1), 0);  
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
    n->txt_fd = NULL;
    pthread_mutex_unlock(&file_access_mtx);

    pthread_mutex_lock(&file_access_mtx);
    txtfd = fopen("/var/tmp/aesdsocketdata", "r");
    n->txt_fd = txtfd;
    do {
        memset(buff, 0, BUFF_SIZE);
        rc = fread(buff, 1, BUFF_SIZE, txtfd );            
        send(n->accept_fd, (void*) buff, rc, 0 );
    } while(rc == BUFF_SIZE);
    fclose(txtfd);
    n->txt_fd = NULL;
    pthread_mutex_unlock(&file_access_mtx);
    close(n->accept_fd);

    n->thread_completed = true;
    return arg;
}

void timer_thread(union sigval arg)
{
  FILE* txtfd = NULL;
  char outstr[200] = {0};
  time_t t;
  struct tm *tmp;
  pthread_mutex_lock(&file_access_mtx);
  txtfd = fopen("/var/tmp/aesdsocketdata", "a+");
  t = time(NULL);
  tmp = localtime(&t);
  strftime(outstr, sizeof(outstr),"%a, %d %b %Y %T %z", tmp);
  fprintf(txtfd, "timestamp:%s\n", (char*)outstr);
  fflush(txtfd);
  fclose(txtfd);
  pthread_mutex_unlock(&file_access_mtx);
}


int main(int argc, char* argv[]) {

    struct slisthead head;

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


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // use IPv4 
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    sa.sa_handler = SigHandler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    SLIST_INIT(&head);

    openlog(NULL, 0, LOG_USER);

    timer_t timer_id;
    struct itimerspec ts;
    struct sigevent se;
    /*
    * Set the sigevent structure to cause the signal to be
    * delivered by creating a new thread.
    */
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = &timer_id;
    se.sigev_notify_function = timer_thread;
    se.sigev_notify_attributes = NULL;

    ts.it_value.tv_sec =10;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 10;
    ts.it_interval.tv_nsec = 0;

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

    //spawn timestamp logger thread? or timer
    rc = timer_create(CLOCK_REALTIME, &se, &timer_id);
    if (rc == -1)
        perror("Create timer");

    rc = timer_settime(timer_id, 0, &ts, 0);
    if (rc == -1)
        perror("Set timer");

    while(1) {
        if(kill_app)
            break;

        accept_fd = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);
        syslog(LOG_DEBUG, "Accepted connection from %s", inet_ntoa(((struct sockaddr_in*)&their_addr)->sin_addr));    
        
        struct node *nn;
        nn = calloc(sizeof(struct node), 1);
        nn->thread_completed = false;
        nn->accept_fd = accept_fd;
        nn->txt_fd = NULL;
        SLIST_INSERT_HEAD(&head, nn, nodes);

        pthread_create(&nn->tid, NULL, handle_connection, nn);

       
        // syslog(LOG_DEBUG, "Closed connection from %s", inet_ntoa(((struct sockaddr_in*)&their_addr)->sin_addr));
    }
    timer_delete(timer_id);
    struct node* n1;
    while (!SLIST_EMPTY(&head)) {          
        n1 = SLIST_FIRST(&head);
        pthread_join(n1->tid, NULL);
        SLIST_REMOVE_HEAD(&head, nodes);
        free(n1);
    }

    close(socketfd);

    closelog();
    exit(0);
}