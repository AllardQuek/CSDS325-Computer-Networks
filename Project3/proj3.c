/**
 * CSDS 325 Project 3 (2022 Fall Semester)
 * 
 * Author: Allard Quek
 * Case network ID: axq54
 * Filename: proj2.c
 * Date Created: 16 October 2022
 *
 * The third project of the semester involves writing a simple web server. 
 * The aim of this project is to think about and implement the server half of client-server applications.
 * */


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Add networking libraries
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Define constant macros (from sample code)
#define ERROR 1
#define HTTP_PORT 80
#define PROTOCOL "tcp"
#define BUFLEN 1024
#define ERROR_PREFIX "ERROR: "
#define PATH_DELIMITER "/"
#define CRLF "\r\n"
#define END_OF_HEADER "\r\n\r\n"
#define SUCCESS_CODE "200"
#define QLEN 1
#define ERROR_400_MSG "HTTP /1.1 400 Malformed Request\r\n\r\n"
#define ERROR_501_MSG "HTTP /1.1 501 Protocol Not Implemented\r\n\r\n"
#define ERROR_405_MSG "“HTTP/1.1 405 Unsupported Method\r\n\r\n"
#define SHUTTING_DOWN_MSG "HTTP/1.1 200 Server Shutting Down\r\n\r\n"
#define ERROR_403_MSG "HTTP/1.1 403 Operation Forbidden\r\n\r\n"
#define ERROR_404_MSG "HTTP/1.1 404 File Not Found\r\n\r\n"

// Define option flags
static bool is_option_p = false;
static bool is_option_r = false;
static bool is_option_t = false;
static bool is_option_v = false;

// Define pointers for required information
static char http_request[BUFLEN];
// static char http_headers[BUFLEN];

// put ':' in the starting of the string so that program can distinguish between '?' and ':'
static const char *OPT_STRING = ":p:r:t:v";


/**
 * Prints usage information for this program.
 * */
void usage(char *progname)
{
    fprintf(stderr, "%s -p port -r document_directory -t authentication_token\n", progname);
    fprintf(stderr, "   -p <port>                 Port number on which the server should listen for incoming conncetions from web clients\n");
    fprintf(stderr, "   -r <document_diretory>    Root directory from which files will be served\n");
    fprintf(stderr, "   -t <auth_token>           Authentication token that the new HTTP TERMINATE method will use\n");
    fprintf(stderr, "   -v                        Print debug info\n");
    exit(1);
}


/**
 * Prints debug information if verbose flag is set to true.
 * */
void printv(char *msg_format, char *arg)
{
    if (is_option_v)
        fprintf(stdout, msg_format, arg);
}


/**
 * Prints error message to stderr and exits the program.
 * */
int errexit(char *msg_format, char *arg)
{
    fprintf(stderr, ERROR_PREFIX);
    fprintf(stderr, msg_format, arg);
    fprintf(stderr, "\n");
    exit(ERROR);
}


/**
 * Keeps track of which options are being passed.
 * */
void parse_args(int argc, char *argv [], char **port, char **document_directory, char **auth_token) 
{
    int opt;
    
    while((opt = getopt(argc, argv, OPT_STRING)) != -1)
    {	
        switch(opt)
        {
            case 'p':
                *port = optarg;
                printf("port: %s\n", *port);
                is_option_p = true;
                break;
            case 'r':
                *document_directory = optarg;
                printf("document_directory: %s\n", *document_directory);
                is_option_r = true;
                break;
            case 't':
                *auth_token = optarg;
                printf("auth_token: %s\n", *auth_token);
                is_option_t = true;
                break;
            case 'v':
                is_option_v = true;
                break;
            case ':':
                printf("%sOption %c is missing a value \n", ERROR_PREFIX, optopt);
                usage(argv[0]);
            case '?':
                printf("%sUnknown option %c\n", ERROR_PREFIX, optopt);
                usage(argv[0]);
        }
    }
}


/**
 * Exits if required arguments are not set to true.
 * */
void check_required_args() 
{
    if (!is_option_p || !is_option_r || !is_option_t) {
        errexit("Required options: -p -r -t", NULL);
    }
}


/**
 * Starts listening for connections on a socket.
 * Returns a socket descriptor.
 * */
int start_listening(char *port)
{
    struct sockaddr_in sin;
    struct protoent *protoinfo;
    int sd;
    
    /* determine protocol */
    if ((protoinfo = getprotobyname (PROTOCOL)) == NULL)
        errexit ("cannot find protocol information for %s", PROTOCOL);

    /* setup endpoint info */
    memset ((char *)&sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons((u_short) atoi(port));

    /* allocate a socket */
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("cannot create socket", NULL);

    /* bind the socket */
    if (bind (sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit ("cannot bind to port %s", port);

    /* listen for incoming connections */
    if (listen (sd, QLEN) < 0)
        errexit ("cannot listen on port %s\n", port);

    printf("Listening for connections...\n");
    return sd;
}


void accept_connection(int sd) {
    struct sockaddr addr;
    unsigned int addrlen;
    int sd2, bytes_read;
    
    /* accept a connection */
    addrlen = sizeof(addr);
    sd2 = accept(sd, &addr, &addrlen);
    if (sd2 < 0)
        errexit ("error accepting connection", NULL);

    printv("Accepted connection!\n", NULL);

    // * Read the request from the client
    memset(http_request, 0, BUFLEN);

    while ((bytes_read = read(sd2, http_request, BUFLEN-1)) > 0) 
    {
        printf("Request: %s\n", http_request);

        // Detect end of HTTP request
        if (http_request[bytes_read - 1] == '\n' && http_request[bytes_read - 2] == '\r') {
            break;
        }
        memset(http_request, 0, BUFLEN);
    }
    if (bytes_read < 0)
    {
        errexit("error reading request", NULL);
    }

    printf("Now parse request: %s\n", http_request);

    // * Send response to the client
    
    /* write message to the connection */
    if (write(sd2, "LOOKS GOOD\n", strlen("LOOKS GOOD")) < 0)
        errexit ("error writing message: %s", "LOOKS GOOD");

    /* close connections and exit */
    close(sd2);
}


/**
 * Main entry point of program.
 * */
int main(int argc, char *argv[])
{
    char *port, *document_directory, *auth_token;

    parse_args(argc, argv, &port, &document_directory, &auth_token);
    printv("Starting command-line based web client...\n", NULL);
    check_required_args();
    printv("\n========== Handling required options ==========\n", NULL);
    int sd = start_listening(port);
    while (1)
    {
        accept_connection(sd);
    }
    close(sd);
    exit(0);
}
