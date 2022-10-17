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
#define ERROR_400_MSG "HTTP/1.1 400 Malformed Request\r\n\r\n"
#define ERROR_501_MSG "HTTP/1.1 501 Protocol Not Implemented\r\n\r\n"
#define ERROR_405_MSG "HTTP/1.1 405 Unsupported Method\r\n\r\n"
#define SHUTTING_DOWN_MSG "HTTP/1.1 200 Server Shutting Down\r\n\r\n"
#define ERROR_403_MSG "HTTP/1.1 403 Operation Forbidden\r\n\r\n"
#define ERROR_404_MSG "HTTP/1.1 404 File Not Found\r\n\r\n"

// Define option flags
static bool is_option_p = false;
static bool is_option_r = false;
static bool is_option_t = false;
static bool is_option_v = false;

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

bool starts_with(const char *str, const char *prefix)
{
   if (strncmp(str, prefix, strlen(prefix)) == 0) return 1;
   return 0;
}


void parse_request(char *request, char *method, char *argument, char *http_version)
{
    char *token;

    // 1. Parse method
    token = strtok(request, " ");
    strcpy(method, token);
    if (strcmp(method, "GET") != 0 && strcmp(method, "TERMINATE") != 0) 
    {
        errexit(ERROR_405_MSG, NULL);
    }

    // 2. Parse argument
    token = strtok(NULL, " ");
    if (token == NULL) 
    {
        printf("token for argument is null\n");
        errexit(ERROR_400_MSG, NULL);
    }
    strcpy(argument, token);

    // 3. Parse HTTP version
    token = strtok(NULL, " ");
    if (token == NULL) 
    {
        printf("token for http_version is null\n");
        errexit(ERROR_400_MSG, NULL);
    }
    strcpy(http_version, token);

    // Check if http_version ends with \r\n
    int len = strlen(http_version);
    if (!(http_version[len - 2] == '\r') || !(http_version[len - 1] = '\n'))
    {
        errexit(ERROR_400_MSG, NULL);
    }

    // Check if HTTP/ portion is present in http_version
    if (!starts_with(http_version, "HTTP/"))
    {
        errexit(ERROR_501_MSG, NULL);
    }
}


void accept_connection(int sd) {
    struct sockaddr addr;
    unsigned int addrlen;
    int sd2;
    
    // Accept a connection
    addrlen = sizeof(addr);
    sd2 = accept(sd, &addr, &addrlen);
    if (sd2 < 0)
        errexit ("error accepting connection", NULL);

    printv("Accepted connection!\n", NULL);

    // Read the request from the client
    char request[BUFLEN];
    char http_request[BUFLEN];
    bool has_read_request = false;
    memset(http_request, 0, BUFLEN);

    // Associate socket stream to a file pointer so we can use fetgs() to read the socket
    FILE *fd;
    if ((fd = fdopen(sd2, "r")) == NULL) {
        errexit("Failed to open file pointer for socket!", NULL);
    }

    for (;;) {
        // BUFLEN just needs to be big enough for current line
        if (fgets(http_request, BUFLEN, fd) == NULL) {
            errexit("Could not get HTTP response!", NULL);
        }
        printf("http_request: %s", http_request);

        // Build header string with current header line
        if (!has_read_request) {
            strcpy(request, http_request);
            has_read_request = true;
        }

        if (strcmp(http_request, CRLF) == 0) {
            printf("Reached end of request!\n", NULL);
            break;
        }
    }

    printf("Now parse request: %s\n", request);
    char method[BUFLEN];
    char argument[BUFLEN];
    char http_version[BUFLEN];
    parse_request(request, method, argument, http_version);
    printf("Method: %s\n", method);
    printf("Argument: %s\n", argument);
    printf("HTTP Version: %s\n", http_version);


    // Write message to the connection 
    if (write(sd2, "LOOKS GOOD\n", strlen("LOOKS GOOD")) < 0)
        errexit ("error writing message: %s", "LOOKS GOOD");

    // Close connections and exit
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
