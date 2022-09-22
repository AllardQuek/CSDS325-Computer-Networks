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

// Define option flags
static bool is_option_u = false;
static bool is_option_o = false;
static bool is_option_i = false;
static bool is_option_c = false;
static bool is_option_s = false;
static char *url;
static char *output_filename;
static char *PATH_DELIMITER = "/";

// Define constants
static const char *OPT_STRING = ":u:o:csi";
static const char *URL_PREFIX = "http://";
static const char *ERROR_PREFIX = "ERROR: ";
static const char *OPTION_I_PREFIX = "INF:";
// static const char *OPTION_C_PREFIX = "REQ:";
// static const char *OPTION_S_PREFIX = "RSP: ";

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
 * Handles the -u option.
 * */
void handle_u() 
{
	// We should check for valid URL as long as it was passed in
	if (strncasecmp(url, URL_PREFIX, strlen(URL_PREFIX)) != 0) { 
		printf("%s Url must start with %s\n", ERROR_PREFIX, URL_PREFIX);
		exit(1);
	}
	printf("Valid URL received: %s\n", url);
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
	char *host_and_path	= url + strlen(URL_PREFIX);
	char host_and_path_copy[strlen(host_and_path)];
	strcpy(host_and_path_copy, host_and_path);

	// 1. Get hostname
	char *hostname = strtok(host_and_path, PATH_DELIMITER);

	// 2. Get url filename
	char *url_filename = host_and_path_copy + strlen(hostname);
	if (strlen(url_filename) == 0) {
		strcpy(url_filename, PATH_DELIMITER);
	}

	printf("%s hostname = %s\n", OPTION_I_PREFIX, hostname);
	printf("%s url_filename = %s\n", OPTION_I_PREFIX, url_filename);
	printf("%s output_filename = %s\n", OPTION_I_PREFIX, output_filename);	// 3. Output filename already set
}

/**
 * Handles the -c option.
 * */
void handle_c() 
{
	// TODO
}


int main(int argc, char *argv[])
{
	printf("Starting command line-based web client...\n");
	parse_args(argc, argv);
	check_required_args();
	
	// * Handle -u: Make HTTP request using the provided URL
	printf("\n========== Handling -u option ==========\n");
	handle_u();
	
	// * Handle -i: Print information about the given command line parameters to standard output
	if (is_option_i) {
		printf("\n========== Handling -i option ==========\n");
		handle_i();
	}

	// * Handle -c: Print the HTTP request sent by the web client to the web server to standard output
	if (is_option_c) {
		printf("\n========== Handling -c option ==========\n");
		handle_c();
	}

	// * Handle -s: Print the HTTP response header received from the web server to standard output
	

	// * Handle -o: Output content from HTTP response to the provided file

	exit(0);
}
