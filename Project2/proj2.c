/**
 * CSDS 325 Project 2 (2022 Fall Semester)
 * 
 * Author: Allard Quek
 *
 * The second project of the semester involves writing a simple command line-based web client.
 * The aim of this project is to
 * (i) get your feet wet with writing C/C++,
 * (ii) write a program that exchanges information with another computer over a network and
 * (iii) start concretely thinking about protocols.
 * */


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Added networking libraries
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Define constant macros (from sample code)
#define ERROR 1
#define REQUIRED_ARGC 3
#define HOST_POS 1
#define HTTP_PORT 80
#define PROTOCOL "tcp"
#define BUFLEN 1024

// Define option flags
static bool is_option_u = false;
static bool is_option_o = false;
static bool is_option_i = false;
static bool is_option_c = false;
static bool is_option_s = false;

// Define pointers for required information
static char *url;
static char *hostname;
static char *url_filename;
static char *output_filename;
static char http_request[BUFLEN];
// static char *http_response;

// Define constants
static const char *OPT_STRING = ":u:o:csi";
static const char *URL_PREFIX = "http://";
static const char *PATH_DELIMITER = "/";
static const char *ERROR_PREFIX = "ERROR: ";


/**
 * Prints usage information for this program.
 * */
void usage(char *progname)
{
  fprintf(stderr, "%s -u URL [-i] [-c] [-s] -o filename \n", progname);
  fprintf(stderr, "   -u <url>         URL the web client will access\n");
  fprintf(stderr, "   -o <filename>    Filename where the downloaded contents of the supplied URL will be written\n");
  fprintf(stderr, "   -i               Print information about the given command line parameters to standard output\n");
  fprintf(stderr, "   -c               Print the HTTP request sent by the web client to the web server to standard output\n");
  fprintf(stderr, "   -s               Print the HTTP response header received from the web server to standard output\n");
  exit(1);
}


/**
 * Prints error message to stderr and exits the program.
 * */
int errexit(char *format, char *arg)
{
    fprintf(stderr,format,arg);
    fprintf(stderr,"\n");
    exit(ERROR);
}


/**
 * Keeps track of which options are being passed.
 * */
void parse_args(int argc, char *argv []) 
{
	int opt;
	
	// put ':' in the starting of the string so that program can distinguish between '?' and ':'
	while((opt = getopt(argc, argv, OPT_STRING)) != -1)
	{	
		switch(opt)
		{
			case 'u':
				url = optarg;
				is_option_u = true;
				break;
			case 'o':
				output_filename = optarg;
				is_option_o = true;
				break;
			case 'i':
				is_option_i = true;
				break;
			case 'c':
				is_option_c = true;
				break;
			case 's':
				is_option_s = true;
				break;
			case ':':
				printf("%s Option %c is missing a value \n", ERROR_PREFIX, optopt);
				break;
			case '?':
				printf("%s Unknown option %c\n", ERROR_PREFIX, optopt);
				usage(argv[0]);
				break;
		}
	}

	// optind is for the extra arguments which are not parsed
	for(; optind < argc; optind++){	
		printf("extra arguments: %s\n", argv[optind]);
	}
}


/**
 * Exits if required arguments are not set to true.
 * */
void check_required_args() 
{
	if (!is_option_u || !is_option_o) {
		printf("%s Required options: -u -o\n", ERROR_PREFIX);
		exit(1);
	}
}


/**
 * Sends a HTTP GET request to the server.
 * */
void send_http_request()
{
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *protoinfo;
    int sd;
	char recvline[BUFLEN];
	int bytes_read;

	// Lookup the hostname
	printf("Looking up %s...\n", hostname);
    hinfo = gethostbyname(hostname);
    if (hinfo == NULL)
        errexit ("Cannot find name: %s!", hostname);

    // Set endpoint information 
	printf("Setting endpoint info...\n");

	// Zero out the socket address
    memset((char *)&sin, 0x0, sizeof(sin));	
    sin.sin_family = AF_INET;
    sin.sin_port = htons(HTTP_PORT);
    memcpy((char *)&sin.sin_addr,hinfo->h_addr,hinfo->h_length);

    if ((protoinfo = getprotobyname(PROTOCOL)) == NULL)
        errexit ("Cannot find protocol information for %s!", PROTOCOL);

    // Allocate a socket (would be SOCK_DGRAM for UDP)
	printf("Allocating a socket...\n");
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("Cannot create socket!\n",NULL);

    // Connect the socket
	printf("Connecting the socket...\n");
    if (connect (sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit ("Cannot connect!", NULL);

	// * Send HTTP request to server
	snprintf(http_request, BUFLEN, 
		"GET %s HTTP/1.0 \r\nHost: %s\r\nUser-Agent: CWRU CSDS 325 Client 1.0\r\n\r\n", url_filename, hostname);
	size_t request_size = strlen(http_request);
	printf("\nHere is the request:\n\%s\n", http_request);

	// Take note of value for msgSize
	if (write(sd, http_request, request_size) != request_size) { //Send bytes
        errexit("Error writing to socket!", NULL);
    }
    printf("MESSAGE SENT. Printing HTTP response...\n\n");

	// * Read HTTP response from server
	// ? Why cannot place `read` outside of while loop?
    while ((bytes_read = read(sd, recvline, BUFLEN-1)) > 0) 
    {
        printf("%s", recvline);

		// ? Zero out after printing current line
		memset(recvline, 0, BUFLEN);	
    }   

	if (bytes_read < 0)
		errexit("Error reading from socket!", NULL);

    close (sd);
}


/**
 * Handles the -u option.
 * */
void handle_u() 
{
	// We should check for valid URL as long as it was passed in
	if (strncasecmp(url, URL_PREFIX, strlen(URL_PREFIX)) != 0) { 
		printf("%s Url must start with %s\n", ERROR_PREFIX, URL_PREFIX);
		exit(1);
	}

	char *host_and_path	= url + strlen(URL_PREFIX);
	char host_and_path_copy[strlen(host_and_path)];
	strcpy(host_and_path_copy, host_and_path);
	printf("Valid URL received: %s\n", url);

	// 1. Get hostname
	hostname = strtok(host_and_path, PATH_DELIMITER);

	// 2. Get url filename
	url_filename = host_and_path_copy + strlen(hostname);
	if (strlen(url_filename) == 0) {
		strcpy(url_filename, PATH_DELIMITER);
	}

	printf("Sending HTTP request...\n");
	send_http_request();
}


/**
 * Handles the -i option.
 * 
 * Example:
 * url: http://www.icir.org/home/index.html
 * INF: hostname = [hostname]
 * INF: web_filename = [url_filename]
 * INF: output_filename = [local_filename]
 * */
void handle_i() 
{
	printf("INF: hostname = %s\n", hostname);
	printf("INF: url_filename = %s\n", url_filename);
	printf("INF: output_filename = %s\n", output_filename);
}


/**
 * Handles the -c option. (local version)
 * Note that HTTP request must still be made regardless of this option.
 * */
void handle_c() 
{
	char *line;
	const char *new_line_char = "\r\n";

	// Terminate request string before new blank line
	http_request[strlen(http_request) - strlen(new_line_char)] = 0;
	
	/* get the first token */
	line = strtok(http_request, new_line_char);
	
	/* walk through other tokens */
	while (line != NULL ) {
		printf( "REQ: %s\n", line );
		line = strtok(NULL, new_line_char);
	}
}


/**
 * Handles the -s option.
 * */
void handle_s()
{
	printf("RSP: hostname = %s\n", hostname);
	printf("RSP: url_filename = %s\n", url_filename);
	printf("RSP: output_filename = %s\n", output_filename);
}


int main(int argc, char *argv[])
{
	printf("Starting command line-based web client...\n");
	parse_args(argc, argv);
	check_required_args();
	
	// * Handle -u: Make HTTP request using the provided URL
	printf("\n========== Handling -u option ==========\n");
	handle_u();

	// * Handle -o: Output content from HTTP response to the provided file
	
	// * Handle optional arguments in specified order; simply to print output.
	// Handle -i: Print information about the given command line parameters to standard output
	if (is_option_i) {
		printf("\n========== Handling -i option ==========\n");
		handle_i();
	}

	// Handle -c: Print the HTTP request sent by the web client to the web server to standard output
	if (is_option_c) {
		printf("\n========== Handling -c option ==========\n");
		handle_c();
	}

	// Handle -s: Print the HTTP response header received from the web server to standard output
	if (is_option_s) {
		printf("\n========== Handling -s option ==========\n");
		handle_s();
	}

	exit(0);
}
