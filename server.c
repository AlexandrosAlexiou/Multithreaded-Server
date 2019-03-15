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
pthread_mutex_t mutex;
pthread_cond_t cond;
queue* q;
long int ttotal_waiting_time;
int total_service_time;
int completed_requests;
struct timeval tv0;

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
                    // Read the given key from the database.
                    if (KISSDB_get(db, request->key, request->value))
                        sprintf(response_str, "GET ERROR\n");
                    else
                        sprintf(response_str, "GET OK: %s\n", request->value);
                    break;
                case PUT:
                    // Write the given key/value pair to the database.
                    if (KISSDB_put(db, request->key, request->value))
                        sprintf(response_str, "PUT ERROR\n");
                    else
                        sprintf(response_str, "PUT OK\n");
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
qelement queue_get(){
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

    /*We got an element, pass it back and unblock*/
    qelement request;
    request=peek(q);
    pop(q);
    gettimeofday(&tv0, NULL);
    ttotal_waiting_time=ttotal_waiting_time+(tv0.tv_sec-request.start_time);
    /*Unlocks the mutex*/
    pthread_mutex_unlock(&mutex);

    return request;
}

static void* connectionHandler(){
    int connfd = 0;

    /*Wait until tasks is available*/
    while(1){
        connfd = queue_get().newfd;
        printf("Handler %lu: \tProcessing\n", pthread_self());
        /*Execute*/
        process_request(connfd);
    }
    return NULL;

}

/*
 * This method locks down the connection queue then utilizes the queue.h push function
 * to add a connection to the queue. Then the mutex is unlocked and cond_signal is set
 * to alarm threads in cond_wait that a connection as arrived for reading
 */
void queue_add(qelement request){
    /*Locks the mutex*/
    pthread_mutex_lock(&mutex);

    push(q,request);

    /*Unlocks the mutex*/
    pthread_mutex_unlock(&mutex);

    /* Signal waiting threads */
    pthread_cond_signal(&cond);
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
    /*Initialize the mutex variable*/
    pthread_mutex_init(&mutex,NULL);
    /*Initialize the struct to get the statistics*/
    struct timeval tv;
    qelement currert_request;
    /*Declare the thread pool array*/
    pthread_t threadPool[10];
    //create the queue
    q = createQueue(10);
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
    for( i= 0; i < 10; i++){
        pthread_create(&threadPool[i], NULL, connectionHandler, (void *) NULL);
    }
    // start listening to socket for incoming connections

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
    listen(socket_fd, MAX_PENDING_CONNECTIONS);
    fprintf(stderr, "(Info) main: Listening for new connections on port %d ...\n", MY_PORT);

    // main loop: wait for new connection/requests
    while (1) {
        // wait for incomming connection
        if ((new_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &clen)) == -1) {
            ERROR("accept()");
        }
        gettimeofday(&tv, NULL);
        //add request to queue
        currert_request.newfd=new_fd;
        currert_request.start_time=tv.tv_sec;

        queue_add(currert_request);


        fprintf(stderr, "(Info) main: Got connection from '%s'\n", inet_ntoa(client_addr.sin_addr));
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