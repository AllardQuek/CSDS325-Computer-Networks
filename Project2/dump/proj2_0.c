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
#include <unistd.h>

#define ARG_U 0x1
#define ARG_O 0x2
#define ARG_I 0x3
#define ARG_C 0x4
#define ARG_S 0x5
#define ARG_HELP 0x6
#define ARG_MSG 0x7
#define ARG_HELLO 0x8

unsigned short cmd_line_flags = 0;
char *msg = NULL;

void usage(char *progname)
/**
 * Prints usage information for this program.
 * */
{
  fprintf(stderr, "%s -u URL [-i] [-c] [-s] -o filename \n", progname);
  fprintf(stderr, "   -u <url>         URL the web client will access\n");
  fprintf(stderr, "   -o <filename>    Filename where the downloaded contents of the supplied URL will be written\n");
  fprintf(stderr, "   -i               Print information about the given command line parameters to standard output\n");
  fprintf(stderr, "   -c               Print the HTTP request sent by the web client to the web server to standard output\n");
  fprintf(stderr, "   -s               Print the HTTP response header received from the web server to standard output\n");
  exit(1);
}

void parseargs(int argc, char *argv[])
/**
 * Parses the different possible arguments, else by default display usage.
 * */
{ 
  int opt;

  // ? Keep parsing any arguments the user inputs
  while ((opt = getopt(argc, argv, "uoicsm:")) != -1)
  {
    switch (opt)
    {
    case 'u':
      cmd_line_flags |= ARG_U;
      break;
    case 'o':
      cmd_line_flags |= ARG_O;
      break;
    case 'i':
      cmd_line_flags |= ARG_I;
      break;
    case 'c':
      cmd_line_flags |= ARG_C;
      break;
    case 's':
      cmd_line_flags |= ARG_S;
      break;
    case 'm':
      cmd_line_flags |= ARG_MSG;
      msg = optarg;
      break;
    case '?':
    default:
      usage(argv[0]);
    }
  }
  if (cmd_line_flags == 0)
  {
    fprintf(stderr, "error: no command line option given\n");
    usage(argv[0]);
  }
}

int main(int argc, char *argv[])
/**
 * Starting entry point to this program.
 * */
{
  parseargs(argc, argv);

  if (cmd_line_flags == ARG_HELP)
    usage(argv[0]);
  else if (cmd_line_flags == ARG_HELLO)
    fprintf(stdout, "hello world\n");
  else if (cmd_line_flags == ARG_MSG)
    fprintf(stdout, "%s\n", msg);
  else if (cmd_line_flags == ARG_U)
    fprintf(stdout, "U option\n");
  else if (cmd_line_flags == ARG_O)
    fprintf(stdout, "O option\n");
  else if (cmd_line_flags == ARG_I)
    fprintf(stdout, "I option\n");
  else if (cmd_line_flags == ARG_C)
    fprintf(stdout, "C option\n");
  else if (cmd_line_flags == ARG_S)
    fprintf(stdout, "S option\n");
  else
  {
    // TODO: Allow multiple options
    fprintf(stderr, "error: only one option at a time allowed\n");
    exit(1);
  }
  exit(0);
}
