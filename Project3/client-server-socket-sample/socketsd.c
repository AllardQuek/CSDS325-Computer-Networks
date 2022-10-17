
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define REQUIRED_ARGC 3
#define PORT_POS 1
#define MSG_POS 2
#define ERROR 1
#define QLEN 1
#define PROTOCOL "tcp"
#define BUFLEN 1024

int usage (char *progname)
{
    fprintf (stderr,"usage: %s port msg\n", progname);
    exit (ERROR);
}

int errexit (char *format, char *arg)
{
    fprintf (stderr,format,arg);
    fprintf (stderr,"\n");
    exit (ERROR);
}

int main (int argc, char *argv [])
{
    struct sockaddr_in sin;
    struct sockaddr addr;
    struct protoent *protoinfo;
    unsigned int addrlen;
    int sd, sd2;
    
    if (argc != REQUIRED_ARGC)
        usage (argv [0]);

    /* determine protocol */
    if ((protoinfo = getprotobyname (PROTOCOL)) == NULL)
        errexit ("cannot find protocol information for %s", PROTOCOL);

    /* setup endpoint info */
    memset ((char *)&sin,0x0,sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons ((u_short) atoi (argv [PORT_POS]));
    printf("SET UP ENDPOINT INFO\n");

    /* allocate a socket */
    /*   would be SOCK_DGRAM for UDP */
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("cannot create socket", NULL);

    printf("ALLOCATED SOCKET\n");
    /* bind the socket */
    if (bind (sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit ("cannot bind to port %s", argv [PORT_POS]);

    printf("BINDED\n");
    /* listen for incoming connections */
    if (listen (sd, QLEN) < 0)
        errexit ("cannot listen on port %s\n", argv [PORT_POS]);

    printf("LISTENING\n");
    /* accept a connection */
    addrlen = sizeof(addr);
    sd2 = accept (sd,&addr,&addrlen);
    if (sd2 < 0)
        errexit ("error accepting connection", NULL);

    printf("ACCEPTED CONNECTION\n");
    
    /* write message to the connection */
    if (write (sd2,argv [MSG_POS],strlen (argv [MSG_POS])) < 0)
        errexit ("error writing message: %s", argv [MSG_POS]);

    /* close connections and exit */
    close (sd);
    close (sd2);
    exit (0);
}
