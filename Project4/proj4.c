/**
 * CSDS 325 Project 4 (2022 Fall Semester)
 * 
 * Author: Allard Quek
 * Case network ID: axq54
 * Filename: proj4.c
 * Date Created: 9 November 2022
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
#define ERROR_PREFIX "ERROR: "

// Define option flags
static bool is_option_t = false;
static bool is_option_s = false;
static bool is_option_l = false;
static bool is_option_p = false;
static bool is_option_m = false;
static bool is_option_v = false;
static bool is_single_opt_provided = false;

// put ':' in the starting of the string so that program can distinguish between '?' and ':'
static const char *OPT_STRING = "slpmv:t:";


/**
 * Prints usage information for this program.
 * */
void usage(char *progname)
{
    fprintf(stderr, "%s -t trace_file -s|-l|-p|-m\n", progname);
    fprintf(stderr, "   -s specifies the tool should run in \"summary mode\"\n");
    fprintf(stderr, "   -l specifies the tool will run in \"length analysis mode\"\n");
    fprintf(stderr, "   -p specifies the tool will run in \"packet printing mode\"\n");
    fprintf(stderr, "   -m specifies the tool will run in \"traffic matrix mode\"\n");
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
void parse_args(int argc, char *argv [], char **TRACE_FILENAME) 
{
    int opt;
    
    while((opt = getopt(argc, argv, OPT_STRING)) != -1)
    {	
        switch(opt)
        {
            case 't':
                *TRACE_FILENAME = optarg;
                is_option_t = true;
                break;
            case 's':
                if (is_single_opt_provided)
                    usage(argv[0]);
                is_single_opt_provided = true;
                is_option_s = true;
                break;
            case 'l':
                if (is_single_opt_provided)
                    usage(argv[0]);
                is_single_opt_provided = true;
                is_option_l = true;
                break;
            case 'p':
                if (is_single_opt_provided)
                    usage(argv[0]);
                is_single_opt_provided = true;
                is_option_p = true;
                break;
            case 'm':
                if (is_single_opt_provided)
                    usage(argv[0]);
                is_single_opt_provided = true;
                is_option_m = true;
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
 * Checks if exactly one of the s, l, p, m options is set.
 * */
void check_required_args() 
{
    if (!is_option_t) {
        errexit("Required option: -t", NULL);
    }
}


/**
 * Main entry point of program.
 * */
int main(int argc, char *argv[])
{
    char *TRACE_FILENAME;
    printv("Starting project 4...\n", NULL);
    parse_args(argc, argv, &TRACE_FILENAME);
    check_required_args();
    printv("Trace filename: %s\n", TRACE_FILENAME);
    exit(0);
}
