/* server.c
   Sample code of
   Assignment L1: Simple multi-threaded key-value server
   for the course MYY601 Operating Systems, University of Ioannina
   (c) S. Anastasiadis, G. Kappes 2016
*/
#include <signal.h>
#include <sys/stat.h>
#include "utils.h"
#include "kissdb.h"
#include "queue.h"
#include <sys/time.h>

#define MY_PORT                 6767
#define BUF_SIZE                1160
#define KEY_SIZE                 128
#define HASH_SIZE               1024
#define VALUE_SIZE              1024
#define MAX_PENDING_CONNECTIONS   10


/*Declare global variables*/
int writerscounter=0;
int readerscounter=0;
pthread_mutex_t mutex;
pthread_cond_t cond;
Queue* q;
double total_waiting_time=0.0;
double total_service_time=0.0;
int completed_requests=0;
/*Declare the thread pool array*/
pthread_t threadPool[MAX_PENDING_CONNECTIONS ];


// Definition of the operation type.
typedef enum operation {
    PUT,
    GET
} Operation;

// Definition of the request.
typedef struct request {
    Operation operation;
    char key[KEY_SIZE];
    char value[VALUE_SIZE];
} Request;

// Definition of the database.
KISSDB *db = NULL;

/**
 * @name parse_request - Parses a received message and generates a new request.
 * @param buffer: A pointer to the received message.
 *
 * @return Initialized request on Success. NULL on Error.
 */
Request *parse_request(char *buffer) {
    char *token = NULL;
    Request *req = NULL;

    // Check arguments.
    if (!buffer)
        return NULL;

    // Prepare the request.
    req = (Request *) malloc(sizeof(Request));
    memset(req->key, 0, KEY_SIZE);
    memset(req->value, 0, VALUE_SIZE);

    // Extract the operation type.
    token = strtok(buffer, ":");
    if (!strcmp(token, "PUT")) {
        req->operation = PUT;
    } else if (!strcmp(token, "GET")) {
        req->operation = GET;
    } else {
        free(req);
        return NULL;
    }

    // Extract the key.
    token = strtok(NULL, ":");
    if (token) {
        strncpy(req->key, token, KEY_SIZE);
    } else {
        free(req);
        return NULL;
    }

    // Extract the value.
    token = strtok(NULL, ":");
    if (token) {
        strncpy(req->value, token, VALUE_SIZE);
    } else if (req->operation == PUT) {
        free(req);
        return NULL;
    }
    return req;
}

/*
 * @name process_request - Process a client request.
 * @param socket_fd: The accept descriptor.
 *
 * @return
 */
void process_request(const int socket_fd) {
    char response_str[BUF_SIZE], request_str[BUF_SIZE];
    int numbytes = 0;
    Request *request = NULL;

    // Clean buffers.
    memset(response_str, 0, BUF_SIZE);
    memset(request_str, 0, BUF_SIZE);

    // receive message.
    numbytes = read_str_from_socket(socket_fd, request_str, BUF_SIZE);

    // parse the request.
    if (numbytes) {
        request = parse_request(request_str);
        if (request) {
            switch (request->operation) {
                case GET:
                    /*Locks the mutex*/
                    pthread_mutex_lock(&mutex);
                    readerscounter++;
                    // Read the given key from the database.
                    if (KISSDB_get(db, request->key, request->value))
                        sprintf(response_str, "GET ERROR\n");
                    else
                        sprintf(response_str, "GET OK: %s\n", request->value);
                    pthread_mutex_unlock(&mutex);
                    break;
                case PUT:
                    if(writerscounter==0) {
                        /*Locks the mutex*/
                        pthread_mutex_lock(&mutex);
                        writerscounter++;
                        // Write the given key/value pair to the database.
                        if (KISSDB_put(db, request->key, request->value))
                            sprintf(response_str, "PUT ERROR\n");
                        else
                            sprintf(response_str, "PUT OK\n");
                        /*Unlocks the mutex*/
                        writerscounter--;
                        pthread_mutex_unlock(&mutex);
                    }
                    break;
                default:
                    // Unsupported operation.
                    sprintf(response_str, "UNKOWN OPERATION\n");
            }
            // Reply to the client.
            write_str_to_socket(socket_fd, response_str, strlen(response_str));
            close(socket_fd);

            if (request)
                free(request);
            request = NULL;
            return;
        }
    }
    // Send an Error reply to the client.
    sprintf(response_str, "FORMAT ERROR\n");
    write_str_to_socket(socket_fd, response_str, strlen(response_str));

}
/*
 * This method locks down the connection queue then utilizes pthread_cond_wait() and waits
 * for a signal to indicate that there is an element in the queue. Then it proceeds to pop the
 * connection off the queue and return it
 */
qElement queue_get(){
    struct timeval tvwaitinq;
    double secs_now;
    double msecs_now;

    double secs_req;
    double msecs_req;

    /*Locks the mutex*/
    pthread_mutex_lock(&mutex);

    /*Wait for element to become available*/
    while(empty(q) == 1){
        printf("Thread %lu: \tWaiting for Connection\n", pthread_self());
        int k;
        if((k=pthread_cond_wait(&cond, &mutex)) != 0)
        {
            //printf("%d\n",k);
            perror("Cond Wait Error");
        }
    }

    /*We got an element in the queue*/
    qElement request;
    request=peek(q);
    pop(q);
    //Request time of pop
    gettimeofday(&tvwaitinq,NULL);
    secs_now=tvwaitinq.tv_sec;
    msecs_now=tvwaitinq.tv_usec;
    double conversion;
    conversion=msecs_now/(float)1000000;
    secs_now+=conversion;
    //Request start time
    secs_req=request.tv.tv_sec;
    msecs_req=request.tv.tv_usec;
    double conversion2;
    conversion2=msecs_req/(float)1000000;
    secs_req+=conversion2;
    total_waiting_time+=secs_now-secs_req;

    /*Unlocks the mutex*/
    pthread_mutex_unlock(&mutex);

    return request;
}

static void* connectionHandler(){
    int connfd = 0;
    qElement current_request;
    struct timeval tvprocessstart;
    struct timeval tvprocessdone;
    double sec_start;
    double usec_start;
    double sec_done;
    double usec_done;
    //signal(SIGTSTP,SIG_IGN);
    /*Wait until tasks is available*/
    while(1){
        current_request = queue_get();
        connfd=current_request.fd;
        printf("Handler %lu: \tProcessing\n", pthread_self());
        /*Execute*/
        gettimeofday(&tvprocessstart,NULL);
        process_request(connfd);
        gettimeofday(&tvprocessdone,NULL);

        /*Locks the mutex*/
        pthread_mutex_lock(&mutex);
        sec_start=tvprocessstart.tv_sec;
        usec_start=tvprocessstart.tv_usec;
        sec_done=tvprocessdone.tv_sec;
        usec_done=tvprocessdone.tv_usec;

        double conversion;
        conversion=usec_start/(float)1000000;
        sec_start+=conversion;
        //Request start time
        double conversion2;
        conversion2=usec_done/(float)1000000;
        sec_done+=conversion2;

        total_service_time+=sec_done-sec_start;
        completed_requests+=1;
        /*Unlocks the mutex*/
        pthread_mutex_unlock(&mutex);

    }
    return NULL;

}

/*
 * This method locks down the connection queue then utilizes the queue.h push function
 * to add a connection to the queue. Then the mutex is unlocked and cond_signal is set
 * to alarm threads in cond_wait that a connection as arrived for reading
 */
void queue_add(qElement request){
    /*Locks the mutex*/
    pthread_mutex_lock(&mutex);

    push(q,request);

    /*Unlocks the mutex*/
    pthread_mutex_unlock(&mutex);

    /* Signal waiting threads */
    pthread_cond_signal(&cond);
}
void signalHandler(){
    int i=0;
    for(i= 0; i < MAX_PENDING_CONNECTIONS ; i++){
        pthread_join(threadPool[i],NULL);
    }
    // Free memory.
    if (db)
        free(db);
    db = NULL;
    printf("\n\n");
    printf("\n\n");
    printf("Stats:\n");
    printf("Total waiting time in queue: %f seconds \n",total_waiting_time);
    printf("Total service time: %f in seconds \n",total_service_time);
    printf("Completed requests: %d \n",completed_requests);
    printf("Average time waiting in queue: %f seconds\n",total_waiting_time/(double)completed_requests );
    printf("Average time to process a request: %f seconds\n",total_service_time/(double)completed_requests );

    exit(1);

}
/*
 * @name main - The main routine.
 *
 * @return 0 on success, 1 on error.
 */
int main() {

    int socket_fd,              // listen on this socket for new connections
            new_fd;                 // use this socket to service a new connection
    socklen_t clen;
    struct sockaddr_in server_addr,  // my address information
            client_addr;  // connector's address information
    //initialize signal to terminate the server
    signal(SIGTSTP,signalHandler);
    /*Initialize the mutex variable*/
    pthread_mutex_init(&mutex,NULL);
    /*Initialize the time struct to generate statistics*/
    struct timeval tv0;
    qElement currert_request;
    //create the queue
    q = createQueue(MAX_PENDING_CONNECTIONS);
    // create socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        ERROR("socket()");

    // Ignore the SIGPIPE signal in order to not crash when a
    // client closes the connection unexpectedly.
    signal(SIGPIPE, SIG_IGN);

    // create socket adress of server (type, IP-address and port number)
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    // any local interface
    server_addr.sin_port = htons(MY_PORT);

    // bind socket to address
    if (bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
        ERROR("bind()");
    /*Make Thread Pool*/
    int i=0;
    for( i= 0; i < MAX_PENDING_CONNECTIONS ; i++){
        pthread_attr_t attrs;
        pthread_attr_init(&attrs);
        pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);
        pthread_create(&threadPool[i], &attrs, connectionHandler, NULL);
    }

    clen = sizeof(client_addr);

    // Allocate memory for the database.
    if (!(db = (KISSDB *)malloc(sizeof(KISSDB)))) {
        fprintf(stderr, "(Error) main: Cannot allocate memory for the database.\n");
        return 1;
    }

    // Open the database.
    if (KISSDB_open(db, "mydb.db", KISSDB_OPEN_MODE_RWCREAT, HASH_SIZE, KEY_SIZE, VALUE_SIZE)) {
        fprintf(stderr, "(Error) main: Cannot open the database.\n");
        return 1;
    }
    // start listening to socket for incoming connections
    listen(socket_fd, MAX_PENDING_CONNECTIONS);
    fprintf(stderr, "(Info) main: Listening for new connections on port %d ...\n", MY_PORT);

    // main loop: wait for new connection/requests
    while (1) {
        if(full(q) == 0){
            // wait for incomming connection
            if ((new_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &clen)) == -1) {
             ERROR("accept()");
            }

         //add request to queue
            gettimeofday(&tv0, NULL);
            currert_request.fd=new_fd;
            currert_request.tv.tv_sec=tv0.tv_sec;
            currert_request.tv.tv_usec=tv0.tv_usec;

            queue_add(currert_request);

            fprintf(stderr, "(Info) main: Got connection from '%s'\n", inet_ntoa(client_addr.sin_addr));
        }
    }
    // Destroy the database.
    // Close the database.
    KISSDB_close(db);

    // Free memory.
    if (db)
        free(db);
    db = NULL;

    return 0;
}