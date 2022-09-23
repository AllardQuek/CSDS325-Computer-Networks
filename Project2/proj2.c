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
#define HTTP_PORT 80
#define PROTOCOL "tcp"
#define BUFLEN 1024
#define DEFAULT_STR_LEN 128
#define URL_PREFIX "http://"
#define ERROR_PREFIX "ERROR: "
#define PATH_DELIMITER "/"
#define CRLF "\r\n"
#define END_OF_HEADER "\r\n\r\n"

// Define option flags
static bool is_option_u = false;
static bool is_option_o = false;
static bool is_option_i = false;
static bool is_option_c = false;
static bool is_option_s = false;

// Define pointers for required information
static char url[DEFAULT_STR_LEN];
static char hostname[DEFAULT_STR_LEN];
static char url_filename[DEFAULT_STR_LEN];
static char output_filename[DEFAULT_STR_LEN];
static char http_request[BUFLEN];
static char http_response[BUFLEN];
static char http_headers[BUFLEN];
static char *http_content;

// ? Cannot use #define ?
static const char *OPT_STRING = ":u:o:csi";


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
	fprintf(stderr, ERROR_PREFIX);
    fprintf(stderr, format, arg);
    fprintf(stderr, "\n");
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
				strcpy(url, optarg);
				is_option_u = true;
				break;
			case 'o':
				strcpy(output_filename, optarg);
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
	printf("Here is the request:\n----------\n\%s----------\n", http_request);

	// Take note of value for msgSize
	if (write(sd, http_request, request_size) != request_size) { //Send bytes
        errexit("Error writing to socket!", NULL);
    }
    printf("HTTP request successfully made!\n");

	// * Read HTTP response from server
	// ? Why cannot place `read` outside of while loop?
    while ((bytes_read = read(sd, recvline, BUFLEN)) > 0) 
    {
		strcpy(http_response, recvline);
        printf("Here is the HTTP response:\n----------\n%s----------\n", recvline);

		// ? Zero out after printing current line
		memset(recvline, 0, BUFLEN);	
    }   

	if (bytes_read < 0)
		errexit("Error reading from socket!", NULL);

    close (sd);
}


/**
 * Handles the -u option.
 * Makes HTTP request using the provided URL
 * */
void handle_u() 
{
	// Check for valid URL as long as it was passed in
	if (strncasecmp(url, URL_PREFIX, strlen(URL_PREFIX)) != 0) { 
		printf("%s Url must start with %s\n", ERROR_PREFIX, URL_PREFIX);
		exit(1);
	}

	printf("Valid URL received: %s\n", url);
	char host_and_path[DEFAULT_STR_LEN];
	strcpy(host_and_path, url + strlen(URL_PREFIX));

	// * 1. Get hostname
	char *token;
	token = strtok(host_and_path, PATH_DELIMITER);
	strcpy(hostname, token);

	// * 2. Get url filename
	strcpy(url_filename, host_and_path + strlen(hostname));
	if (strlen(url_filename) == 0) {
		strcpy(url_filename, PATH_DELIMITER);
	}

	printf("Sending HTTP request...\n");
	send_http_request();

	// * 3. Get content and headers from HTTP response
	http_content = strstr(http_response, END_OF_HEADER);	
	
	// Remember: content will include end of header (need to strip new line)
	http_content += strlen(END_OF_HEADER);
	strncpy(http_headers, http_response, strlen(http_response) - strlen(http_content));
}


/**
 * Handles the -o option.
 * Writes HTTP response content to the provided file.
 * */
void handle_o()
{
	// 1. Open file for writing
	FILE *fp = fopen(output_filename, "w");
	if (fp == NULL) {
		errexit("Error opening file!", NULL);
	}

	// 2. Write HTTP response content to file
	fprintf(fp, "%s", http_content);

	// 3. Remember to close file
	fclose(fp);
	printf("Successfully saved HTTP content to %s!\n", output_filename);
}


/**
 * Handles the -i option.
 * Prints information about the given command line parameters to standard output.
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
 * Handles the -c option.
 * Prints the HTTP request sent by the web client to the web server to standard output.
 * Note that HTTP request must still be made regardless of this option.
 * */
void handle_c() 
{
	char *line;

	// Terminate request string before new blank line
	http_request[strlen(http_request) - strlen(CRLF)] = 0;
	
	/* get the first token */
	line = strtok(http_request, CRLF);
	
	/* walk through other tokens */
	while (line != NULL ) {
		printf( "REQ: %s\n", line );
		line = strtok(NULL, CRLF);
	}
}


/**
 * Handles the -s option.
 * Prints the HTTP response header received from the web server to standard output.
 * */
void handle_s()
{
	char* line;

	/* get the first token */
	line = strtok(http_headers, CRLF);
	
	/* walk through other tokens */
	while (line != NULL ) {
		printf("RSP: %s\n", line );
		line = strtok(NULL, CRLF);
	}
}

/**
 * Main entry point of program.
 * */
int main(int argc, char *argv[])
{
	printf("Starting command line-based web client...\n");
	parse_args(argc, argv);
	check_required_args();
	
	// Handle required options: -u and -o
	printf("\n========== Handling -u option ==========\n");
	handle_u();
	printf("\n========== Handling -o option ==========\n");
	handle_o();

	// Handle optional arguments in specified order; simply to print output.
	if (is_option_i) {
		printf("\n========== Handling -i option ==========\n");
		handle_i();
	}

	if (is_option_c) {
		printf("\n========== Handling -c option ==========\n");
		handle_c();
	}

	if (is_option_s) {
		printf("\n========== Handling -s option ==========\n");
		handle_s();
	}

	exit(0);
}
