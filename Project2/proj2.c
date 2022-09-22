/**
 * CSDS 325 Project 2
 *
 * The second project of the semester involves writing a simple command line-based web client.
 * The aim of this project is
 * (i) to get your feet wet with writing C/C++,
 * (ii) to write a program that exchanges information with another computer over a network and
 * (iii) to start concretely thinking about protocols.
 * */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned short cmd_line_flags = 0;
char *msg = NULL;
static const char *URL_PREFIX = "http://";

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
 * Handles the -u option.
 * */
void handle_u(char* optarg) 
{
	printf("HANDLING OPTION U...\n");
	printf("url passed: %s\n", optarg);
	if (strncasecmp(optarg, URL_PREFIX, strlen(URL_PREFIX)) != 0) { 
		printf("url must start with %s\n", URL_PREFIX);
		exit(1);
	}

	printf("So far so good!\n");
}


int main(int argc, char *argv[])
{
	int opt;
	
	// put ':' in the starting of the
	// string so that program can
	//distinguish between '?' and ':'
	while((opt = getopt(argc, argv, ":u:o:csi")) != -1)
	{
		switch(opt)
		{
			case 'u':
				handle_u(optarg);
				break;
			case 'o':
				printf("filename: %s\n", optarg);
				break;
			case 'i':
				printf("option: i\n");
				break;
			case 'c':
				printf("option: c\n");
				break;
			case 's':
				printf("option: s\n");
				break;
			case ':':
				printf("option %c is missing a value \n", optopt);
				break;
			case '?':
				printf("unknown option: %c\n", optopt);
				usage(argv[0]);
				break;
		}
	}
	
	// optind is for the extra arguments
	// which are not parsed
	for(; optind < argc; optind++){	
		printf("extra arguments: %s\n", argv[optind]);
	}
	
	exit(0);
}
