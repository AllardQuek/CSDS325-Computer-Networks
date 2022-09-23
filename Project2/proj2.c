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
#define URL_PREFIX "http://"
#define ERROR_PREFIX "ERROR: "
#define PATH_DELIMITER "/"
#define CRLF "\r\n"
#define END_OF_HEADER "\r\n\r\n"
#define SUCCESS_CODE "301"

// Define option flags
static bool is_option_u = false;
static bool is_option_o = false;
static bool is_option_i = false;
static bool is_option_c = false;
static bool is_option_s = false;
static bool is_option_v = false;

// Define pointers for required information
static char http_request[BUFLEN];
static char http_response[BUFLEN];
static char http_headers[BUFLEN];
static char *http_content;

// ? Cannot use #define ?
// put ':' in the starting of the string so that program can distinguish between '?' and ':'
static const char *OPT_STRING = ":u:o:csiv";


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
	fprintf(stderr, "   -v               Print debug info\n");
	exit(1);
}


/**
 * Prints information if verbose flag is set to true.
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
void parse_args(int argc, char *argv [], char **url, char **output_filename) 
{
	int opt;
	
	while((opt = getopt(argc, argv, OPT_STRING)) != -1)
	{	
		switch(opt)
		{
			case 'u':
				*url = optarg;
				is_option_u = true;
				break;
			case 'o':
				*output_filename = optarg;
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
			case 'v':
				is_option_v = true;
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
		errexit("Required options: -u -o", NULL);
	}
}


/**
 * Sends a HTTP GET request to the server.
 * */
void send_http_request(char *hostname, char *url_filename)
{
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *protoinfo;
    int sd;
	char recvline[BUFLEN];
	int bytes_read;

	// Lookup the hostname
    hinfo = gethostbyname(hostname);
    if (hinfo == NULL)
        errexit("Cannot find name: %s", hostname);

    // Set endpoint information 
	// Zero out the socket address
    memset((char *)&sin, 0x0, sizeof(sin));	
    sin.sin_family = AF_INET;
    sin.sin_port = htons(HTTP_PORT);
    memcpy((char *)&sin.sin_addr,hinfo->h_addr,hinfo->h_length);

    if ((protoinfo = getprotobyname(PROTOCOL)) == NULL)
        errexit("Cannot find protocol information for %s!", PROTOCOL);

    // Allocate a socket (would be SOCK_DGRAM for UDP)
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("Cannot create socket!\n",NULL);

    // Connect the socket
    if (connect (sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        errexit ("Cannot connect!", NULL);
    }

	// * Send HTTP request to server
	snprintf(http_request, BUFLEN, "GET %s HTTP/1.0 \r\nHost: %s\r\nUser-Agent: CWRU CSDS 325 Client 1.0\r\n\r\n", url_filename, hostname);
	size_t request_size = strlen(http_request);
	printv("Here is the request:\n----------\n\%s----------\n", http_request);

	// Take note of value for msgSize
	if (write(sd, http_request, request_size) != request_size) { //Send bytes
        errexit("Error writing to socket!", NULL);
    }

	// * Read HTTP response from server
	// ? Why cannot place `read` outside of while loop?
	// ? What if the while loop is executed multiple times?
    while ((bytes_read = read(sd, recvline, BUFLEN)) > 0) 
    {
		strcpy(http_response, recvline);
        printv("Here is the HTTP response:\n----------\n%s----------\n", recvline);

		// ? Zero out after printing current line
		memset(recvline, 0, BUFLEN);	
    }   

	if (bytes_read < 0)
		errexit("Error reading from socket!", NULL);

    close (sd);
}


/**
 * Returns 1 (true) if URL starts with "http://"" (case-insensitive), else false.
 * */
int is_valid_url(char *url)
{
	if (strncasecmp(url, URL_PREFIX, strlen(URL_PREFIX)) != 0) 
		return 0;

	return 1;
}

/**
 * Handles the -u option.
 * Makes HTTP request using the provided URL
 * */
void handle_u(char *url, char **host_and_path, char **hostname, char **url_filename) 
{
	printv("\n========== Handling -u option ==========\n", NULL);
	if (is_valid_url(url) == 0) 
		errexit("Url must start with %s", URL_PREFIX);
	
	printv("Valid URL received: %s\n", url);

	*host_and_path = url + strlen(URL_PREFIX);
	printv("Host and path: %s\n", *host_and_path);

	// * 1. Get hostname
	// We should avoid moving the original pointers else the values would change
	// This step seems to change `url` and `host_and_path` if *host_and_path is used instead
	char *host_and_path_copy = strdup(*host_and_path);
	*hostname = strtok(host_and_path_copy, PATH_DELIMITER);
	printv("Hostname: %s\n", *hostname);

	// * 2. Get url filename
	*url_filename = *host_and_path + strlen(*hostname);
	printv("URL filename: %s\n", *url_filename);

	if (strlen(*url_filename) == 0) {
		*url_filename = PATH_DELIMITER;
	}

	// Pass the pointer to the required values, not the double pointers
	send_http_request(*hostname, *url_filename);

	// * 3. Get content and headers from HTTP response
	http_content = strstr(http_response, END_OF_HEADER);	
	
	// Remember: content will include end of header (need to strip new line)
	http_content += strlen(END_OF_HEADER);
	strncpy(http_headers, http_response, strlen(http_response) - strlen(http_content));
}


/**
 * Writes content to filename.
 * */
void write_to_file(char *filename, char *content) 
{
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		errexit("Error opening file!", NULL);
	}

	fprintf(fp, "%s", content);		// Write content to file
	fclose(fp);						// Remember to close file
	printv("Successfully saved HTTP content to %s!\n", filename);
}


/**
 * Handles the -o option.
 * Writes HTTP response content to the provided file, if response status was 200 OK.
 * */
void handle_o(char *output_filename)
{
	printv("\n========== Handling -o option ==========\n", NULL);
    // Check if status code was 200 OK
    if (strstr(http_headers, SUCCESS_CODE) == NULL)
    {
        printf("Response from server was not OK. Output will not be written to file.\n");
        return;
    }

	write_to_file(output_filename, http_content);
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
void handle_i(char *hostname, char *url_filename, char *output_filename) 
{
	printv("\n========== Handling -i option ==========\n", NULL);
	printf("INF: hostname = %s\n", hostname);
	printf("INF: url_filename = %s\n", url_filename);
	printf("INF: output_filename = %s\n", output_filename);
}


void print_lines(char *lines, char *prefix)
{
	char *line;

	/* get the first token */
	line = strtok(lines, CRLF);
	
	/* walk through other tokens */
	while (line != NULL ) {
		printf("%s: %s\n", prefix, line);
		line = strtok(NULL, CRLF);
	}
}

/**
 * Handles the -c option.
 * Prints the HTTP request sent by the web client to the web server to standard output.
 * Note that HTTP request must still be made regardless of this option.
 * */
void handle_c() 
{
	printv("\n========== Handling -c option ==========\n", NULL);
	// Terminate request string before new blank line
	http_request[strlen(http_request) - strlen(CRLF)] = 0;
	print_lines(http_request, "REQ");
}


/**
 * Handles the -s option.
 * Prints the HTTP response header received from the web server to standard output.
 * */
void handle_s()
{
	printv("\n========== Handling -s option ==========\n", NULL);
	print_lines(http_headers, "RSP");
}


/**
 * Handles required arguments -u and -o.
 * */
void handle_required_args(char *url, char **host_and_path, char **hostname, char **url_filename, char *output_filename)
{
	handle_u(url, host_and_path, hostname, url_filename);
	handle_o(output_filename);
}


/**
 * Handles optional arguments in specified order; simply to print output.
 * */
void handle_optional_args(char *hostname, char *url_filename, char *output_filename)
{
	if (is_option_i) {
		handle_i(hostname, url_filename, output_filename);
	}

	if (is_option_c) {
		handle_c();
	}

	if (is_option_s) {
		handle_s();
	}
}


/**
 * Main entry point of program.
 * */
int main(int argc, char *argv[])
{
	char *url, *output_filename;
	char *host_and_path, *hostname, *url_filename;

	printv("Starting command-line based web client...\n", NULL);
	parse_args(argc, argv, &url, &output_filename);
	check_required_args();
	handle_required_args(url, &host_and_path, &hostname, &url_filename, output_filename);
	handle_optional_args(hostname, url_filename, output_filename);
	exit(0);
}
