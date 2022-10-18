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
#define PROTOCOL "tcp"
#define BUFLEN 512
#define ERROR_PREFIX "ERROR: "
#define CRLF "\r\n"
#define END_OF_HEADER "\r\n\r\n"
#define QLEN 1
#define OK_200_MSG "HTTP/1.1 200 OK\r\n\r\n"
#define TERMINATE_200_MSG "HTTP/1.1 200 Server Shutting Down\r\n\r\n"
#define ERROR_400_MSG "HTTP/1.1 400 Malformed Request\r\n\r\n"
#define ERROR_403_MSG "HTTP/1.1 403 Operation Forbidden\r\n\r\n"
#define ERROR_404_MSG "HTTP/1.1 404 File Not Found\r\n\r\n"
#define ERROR_405_MSG "HTTP/1.1 405 Unsupported Method\r\n\r\n"
#define ERROR_406_MSG "HTTP/1.1 406 Invalid Filename\r\n\r\n"
#define ERROR_501_MSG "HTTP/1.1 501 Protocol Not Implemented\r\n\r\n"

// Define option flags
static bool is_option_p = false;
static bool is_option_r = false;
static bool is_option_t = false;
static bool is_option_v = false;

// put ':' in the starting of the string so that program can distinguish between '?' and ':'
static const char *OPT_STRING = ":p:r:t:v";
const char *DEFAULT_FILENAME = "/homepage.html";


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
* Writes message to socket.
*/
void write_to_socket(char *msg_format, int sd)
{
    if (write(sd, msg_format, strlen(msg_format)) < 0)
        errexit("error writing message: %s", msg_format);
}


/**
 * Writes message to socket and exits the program.
 * */
int write_and_exit(char *msg_format, int sd)
{
    write_to_socket(msg_format, sd);
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
                is_option_p = true;
                break;
            case 'r':
                *document_directory = optarg;
                is_option_r = true;
                break;
            case 't':
                *auth_token = optarg;
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
    
    // Determine protocol
    if ((protoinfo = getprotobyname (PROTOCOL)) == NULL)
        errexit ("cannot find protocol information for %s", PROTOCOL);

    // Setup endpoint info
    memset ((char *)&sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons((u_short) atoi(port));

    // Allocate a socket
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("cannot create socket", NULL);

    // Bind the socket 
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit("cannot bind to port %s", port);

    // Listen for incoming connections
    if (listen(sd, QLEN) < 0)
        errexit("cannot listen on port %s\n", port);

    printf("Listening for connections...\n");
    return sd;
}


/**
 * Checks whether a given string starts with the given prefix.
*/
bool starts_with(const char *str, const char *prefix)
{
   if (strncmp(str, prefix, strlen(prefix)) == 0) return 1;
   return 0;
}


/**
 * Parses the HTTP request.
*/
void parse_request(int sd2, char *request, char *method, char *argument, char *http_version)
{
    printf("Parsing request: %s\n", request);
    char *token;

    // 1. Parse method
    token = strtok(request, " ");
    strcpy(method, token);

    // 2. Parse argument
    token = strtok(NULL, " ");
    if (token == NULL) 
    {
        printv("token for argument is null\n", NULL);
        write_and_exit(ERROR_400_MSG, sd2);
    }
    strcpy(argument, token);

    // 3. Parse HTTP version
    token = strtok(NULL, " ");
    if (token == NULL) 
    {
        printv("token for http_version is null\n", NULL);
        write_and_exit(ERROR_400_MSG, sd2);
    }
    strcpy(http_version, token);

    // Check if http_version ends with \r\n
    int len = strlen(http_version);
    if (!(http_version[len - 2] == '\r') || !(http_version[len - 1] = '\n'))
        write_and_exit(ERROR_400_MSG, sd2);

    // Check if HTTP/ portion is present in http_version
    if (!starts_with(http_version, "HTTP/"))
        write_and_exit(ERROR_501_MSG, sd2);
}



/**
* Returns a socket desciptor after accepting a connection from a client.
*/
int accept_connection(int sd) {
    struct sockaddr addr;
    unsigned int addrlen;
    int sd2;
    
    // Accept a connection
    addrlen = sizeof(addr);
    sd2 = accept(sd, &addr, &addrlen);
    if (sd2 < 0)
        errexit ("error accepting connection", NULL);

    printv("Accepted connection!\n", NULL);
    return sd2;
}



/**
* Reads the HTTP request sent by the client.
*/
void read_http_request(int sd2, char *request)
{
    char http_request[BUFLEN];
    bool has_read_request = false;
    memset(http_request, 0, BUFLEN);

    // Associate socket stream to a file pointer so we can use fetgs() to read the socket
    FILE *fd;
    if ((fd = fdopen(sd2, "r")) == NULL)
        errexit("Failed to open file pointer for socket!", NULL);

    for (;;) 
    {
        if (fgets(http_request, BUFLEN, fd) == NULL)
            errexit("Could not get HTTP response!", NULL);

        if (!has_read_request) 
        {
            strcpy(request, http_request);
            has_read_request = true;
        }

        if (strcmp(http_request, CRLF) == 0) 
        {
            printv("Reached end of request!\n", NULL);
            break;
        }
    }
}


/**
* Handles TERMINATE requests.
*/
void handle_terminate(int sd2, char *argument, char *AUTH_TOKEN)
{
    printv("Handling TERMINATE request...\n", NULL);
    if (strcmp(argument, AUTH_TOKEN) != 0) 
        write_to_socket(ERROR_403_MSG, sd2);
    else 
        write_and_exit(TERMINATE_200_MSG, sd2);
}


/**
* Handles GET requests.
*/
void handle_get(int sd2, char *argument, char *DOC_DIR)
{
    printv("Handling GET request...\n", NULL);
    if (!starts_with(argument, "/")) 
        write_and_exit(ERROR_406_MSG, sd2);

    FILE *fp;
    char *content = malloc(BUFLEN);
    char filepath[BUFLEN];
    strcpy(filepath, DOC_DIR);

    // If argument is "/" set argument to the default filename
    if (strcmp(argument, "/") == 0) 
        strcat(filepath, DEFAULT_FILENAME);
    else 
        strcat(filepath, argument);
    printf("Filepath: %s\n", filepath);

    // 404 error if cannot open requested file (e.g. because it does not exist)
    if ((fp = fopen(filepath, "r")) == NULL)
        write_and_exit(ERROR_404_MSG, sd2);

    int byte_size = 1;
    int bytes_read;
    bool has_written_success = false;

    // Read file contents 
    while ((bytes_read = fread(content, byte_size, sizeof(content), fp)) > 0) 
    {
        // Write success message
        if (!has_written_success)
        {
            if (write(sd2, OK_200_MSG, strlen(OK_200_MSG)) < 0)
                errexit("error writing message!", NULL);
            has_written_success = true;
        }

        // Write file contents
        if (write(sd2, content, bytes_read) < 0)
            errexit("error writing message!", NULL);
    }
    free(content);
    fclose(fp); 
}


/**
* Processes the HTTP request. Only need to handle TERMINATE and GET.
*/
void process_request(int sd2, char *request, char *method, char *argument, char *DOC_DIR, char *AUTH_TOKEN) 
{
    if (strcmp(method, "TERMINATE") == 0) 
        handle_terminate(sd2, argument, AUTH_TOKEN);
    else if (strcmp(method, "GET") == 0) 
        handle_get(sd2, argument, DOC_DIR);
    else 
        write_and_exit(ERROR_405_MSG, sd2);
}


/**
 * Main entry point of program.
 * */
int main(int argc, char *argv[])
{
    char *PORT, *DOC_DIR, *AUTH_TOKEN;
    char request[BUFLEN];
    char method[BUFLEN];
    char argument[BUFLEN];
    char http_version[BUFLEN];
    printv("Starting command-line based web client...\n", NULL);
    parse_args(argc, argv, &PORT, &DOC_DIR, &AUTH_TOKEN);
    check_required_args();
    printf("Port: %s\n", PORT);
    printf("Document Directory: %s\n", DOC_DIR);
    printf("Auth Token: %s\n", AUTH_TOKEN);

    int sd = start_listening(PORT);
    while (1)
    {
        int sd2 = accept_connection(sd);
        read_http_request(sd2, request);
        parse_request(sd2, request, method, argument, http_version);
        process_request(sd2, request, method, argument, DOC_DIR, AUTH_TOKEN);
        close(sd2);
    }
    close(sd);
    exit(0);
}
