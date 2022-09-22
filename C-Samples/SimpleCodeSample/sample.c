
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ARG_HELP  0x1
#define ARG_MSG   0x2
#define ARG_HELLO 0x4

unsigned short cmd_line_flags = 0;
char *msg = NULL;


void usage (char *progname)
{
    fprintf (stderr,"%s [-h] [-s] [-m message]\n", progname);
    fprintf (stderr,"   -h    show program usage\n");
    fprintf (stderr,"   -s    print standard greeting\n");
    fprintf (stderr,"   -m X  print message \'X\'\n");
    exit (1);
}


void parseargs (int argc, char *argv [])
{
    int opt;

    while ((opt = getopt (argc, argv, "hsm:")) != -1)
    {
        switch (opt)
        {
            case 'h':
              cmd_line_flags |= ARG_HELP;
              break;
            case 's':
              cmd_line_flags |= ARG_HELLO;
              break;
            case 'm':
              cmd_line_flags |= ARG_MSG;
              msg = optarg;
              break;
            case '?':
            default:
              usage (argv [0]);
        }
    }
    if (cmd_line_flags == 0)
    {
        fprintf (stderr,"error: no command line option given\n");
        usage (argv [0]);
    }
}


int main (int argc, char *argv [])
{
    parseargs (argc,argv);

    if (cmd_line_flags == ARG_HELP)
        usage (argv [0]);
    else if (cmd_line_flags == ARG_HELLO)
        fprintf (stdout,"hello world\n");
    else if (cmd_line_flags == ARG_MSG)
        fprintf (stdout,"%s\n", msg);
    else
    {
        fprintf (stderr,"error: only one option at a time allowed\n");
        exit (1);
    }
    exit (0);
}
