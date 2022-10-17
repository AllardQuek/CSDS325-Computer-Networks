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

// Define option flags
static bool is_option_p = false;
static bool is_option_r = false;
static bool is_option_t = false;
static bool is_option_v = false;

// Define pointers for required information
// static char http_request[BUFLEN];
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
 * Connects to socket given a hostname.
 * Returns a socket descriptor.
 * */
int connect_to_socket(char *hostname)
{
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *protoinfo;
    int sd;

    // Lookup the hostname
    hinfo = gethostbyname(hostname);
    if (hinfo == NULL)
        errexit("Cannot find name: %s", hostname);

    // Set endpoint information 
    memset((char *)&sin, 0x0, sizeof(sin));	
    sin.sin_family = AF_INET;
    sin.sin_port = htons(HTTP_PORT);
    memcpy((char *)&sin.sin_addr,hinfo->h_addr,hinfo->h_length);

    if ((protoinfo = getprotobyname(PROTOCOL)) == NULL)
        errexit("Cannot find protocol information for %s!", PROTOCOL);

    // Allocate a socket (would be SOCK_DGRAM for UDP)
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
    {
        errexit("Cannot create socket!\n",NULL);
    }

    if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        errexit("Cannot connect!", NULL);
    }

    return sd;
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
    // int sd = connect_to_socket(*hostname);
    // send_http_request(sd, *hostname, *url_filename);
    // read_http_response(sd, output_filename);
    // close(sd);
    exit(0);
}
