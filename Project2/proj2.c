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
#define SUCCESS_CODE "200"

// Define option flags
static bool is_option_u = false;
static bool is_option_o = false;
static bool is_option_i = false;
static bool is_option_c = false;
static bool is_option_s = false;
static bool is_option_v = false;

// Define pointers for required information
static char http_request[BUFLEN];
static char http_headers[BUFLEN];

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
				printf("%sOption %c is missing a value \n", ERROR_PREFIX, optopt);
				usage(argv[0]);
			case '?':
				// ? Should we still print output if at least one option is unknown?
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
	if (!is_option_u || !is_option_o) {
		errexit("Required options: -u -o", NULL);
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
	// ? Zero out the socket address
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

	if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        errexit("Cannot connect!", NULL);
    }

	return sd;
}


/**
 * Sends a HTTP GET request to the server.
 * */
void send_http_request(int sd, char *hostname, char *url_filename)
{
	// Note that C concatenates adjacent string literals
	snprintf(http_request, BUFLEN, 
			"GET %s HTTP/1.0\r\n"
			"Host: %s\r\n"
			"User-Agent: CWRU CSDS 325 Client 1.0\r\n\r\n", 
			url_filename, hostname);
	
	printv("Here is the request:\n----------\n\%s----------\n", http_request);

	// ? Why need third parameter if it is just length of buffer?
	// size_t request_size = strlen(http_request);
	// if (write(sd, http_request, request_size) != request_size) { 
	if (write(sd, http_request, BUFLEN) != BUFLEN) {
        errexit("Error writing to socket!", NULL);
    }
}


/**
 * Writes the HTTP response to the output file.
 * Use fread() and fwrite() to allow downloading both binary and HTML files
 * */
void write_to_file(char *output_filename, char *http_response, FILE *fd) 
{
	size_t bytes_read;
	FILE *output_file = fopen(output_filename, "wb");
	int byte_size = 1;

	if (output_file == NULL)
	{
		printf("%sFailed to open output file!\n", ERROR_PREFIX);
		fclose(output_file);
		return;
	} 
	// ? bytes_read correct?
	while ((bytes_read = fread(http_response, byte_size, sizeof(http_response), fd)) > 0) {
		// If we wrote fewer bytes than we read, there was an error
		if (fwrite(http_response, byte_size, bytes_read, output_file) != bytes_read) 
			printf("%sFailed to write to file!", ERROR_PREFIX);
			// ? Should we just return here? How to make sure we close the file pointers?
	}

	if (ferror(fd))
		printf("%sCould not read from socket!", ERROR_PREFIX); 

	// Remember to close file pointer
	fclose(output_file);
	printv("Done writing to %s!\n", output_filename);
}


/**
 * Reads the HTTP response header line by line, saving them in a variable.
 * */
void read_header_lines(char *http_response, FILE *fd) 
{
	for (;;) {
		// ? Use BUFLEN, not sizeof(http_response) but should be length of str used?
		// https://www.tutorialspoint.com/c_standard_library/c_function_fgets.htm
		if (fgets(http_response, BUFLEN, fd) == NULL) {
			errexit("Could not get HTTP response!", NULL);
		}

		// Build header string with current header line
		strcat(http_headers, http_response);
		if (strcmp(CRLF, http_response) == 0) {
			printv("Reached end of header!\n", NULL);
			break;
		}
	}
	printv("Here is the header:\n----------\n%s----------\n", http_headers);
}


/**
 * Reads the HTTP response from the server.
 * Note that we also write the response out to a file if response status is 200 OK.
 * */
void read_http_response(int sd, char *output_filename)
{
	char *http_response = malloc(BUFLEN);

	// Associate socket stream to a file pointer so we can use fetgs() to read the socket
	FILE *fd;
	if ((fd = fdopen(sd, "r")) == NULL) {
		errexit("Failed to open file pointer for socket!", NULL);
	}

	read_header_lines(http_response, fd);

	// If response was not 200 OK, skip writing to file
    if (strstr(http_headers, SUCCESS_CODE) == NULL)
    {
        printf("Response from server was not OK. Output will not be written to file.\n");
        return;
    }

	// * Read and write file contents to output file
	write_to_file(output_filename, http_response, fd);

	// Close fd pointer, which also closes the socket
	fclose(fd);
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
 * Sets the variable for storing the hostname and path.
 * */
void set_host_and_path(char *url, char **host_and_path)
{
	*host_and_path = url + strlen(URL_PREFIX);
	printv("Host and path: %s\n", *host_and_path);
}


/**
 * Sets the variable for storing the hostname.
 * */
void set_hostname(char *host_and_path, char **hostname)
{
	// We should avoid moving the original pointers else the values would change
	// ? This step seems to change `url` and `host_and_path` if *host_and_path is used instead
	// *hostname = strtok(host_and_path, PATH_DELIMITER);
	char *host_and_path_copy = strdup(host_and_path);
	*hostname = strtok(host_and_path_copy, PATH_DELIMITER);
	printv("Hostname: %s\n", *hostname);
}


/**
 * Sets the variable for storing the url filename.
 * */
void set_url_filename(char **url_filename, char *host_and_path, char *hostname) 
{
	*url_filename = host_and_path + strlen(hostname);

	if (strlen(*url_filename) == 0) {
		*url_filename = PATH_DELIMITER;
	}
	printv("URL filename: %s\n", *url_filename);
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
	printf("INF: web_filename = %s\n", url_filename);
	printf("INF: output_filename = %s\n", output_filename);
}


/**
 * Prints each individual line with a prefix from the given lines.
 * */
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
void handle_required_args(char *url, char *output_filename, char **host_and_path, char **hostname, char **url_filename)
{
	printv("\n========== Handling required options ==========\n", NULL);
	if (is_valid_url(url) == 0) 
		errexit("Url must start with %s", URL_PREFIX);
	
	printv("Valid URL received: %s\n", url);

	// * Split url into hostname and url_filename
	set_host_and_path(url, host_and_path);
	set_hostname(*host_and_path, hostname);
	set_url_filename(url_filename, *host_and_path, *hostname);

	// * Make HTTP request
	// Pass the pointer to the required values, not the double pointers
	int sd = connect_to_socket(*hostname);
	send_http_request(sd, *hostname, *url_filename);
	read_http_response(sd, output_filename);
	close(sd);
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

	parse_args(argc, argv, &url, &output_filename);
	printv("Starting command-line based web client...\n", NULL);
	check_required_args();
	handle_required_args(url, output_filename, &host_and_path, &hostname, &url_filename);
	handle_optional_args(hostname, url_filename, output_filename);
	exit(0);
}
