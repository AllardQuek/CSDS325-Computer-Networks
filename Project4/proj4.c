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
#include <fcntl.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "next.h"


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


/* fd - an open file to read packets from
   pinfo - allocated memory to put packet info into for one packet

   returns:
   1 - a packet was read and pinfo is setup for processing the packet
   0 - we have hit the end of the file and no packet is available 
 */
unsigned short next_packet(int fd, struct pkt_info *pinfo)
{
    struct meta_info meta;
    int bytes_read;

    // Clear out memory in both structs
    memset (pinfo,0x0,sizeof (struct pkt_info));
    memset (&meta,0x0,sizeof (struct meta_info));

    /* 1. read the meta information (12 bytes) */
    bytes_read = read(fd, &meta, sizeof(meta));
    if (bytes_read == 0)
        return (0);
    if (bytes_read < sizeof(meta))
        errexit("cannot read meta information", NULL);

    // Set caplen attribute of pkt_info struct
    pinfo->caplen = ntohs(meta.caplen);  

    /* 2. set pinfo->now based on meta.secs & meta.usecs */
    pinfo->now = ntohl(meta.secs) + (ntohl(meta.usecs) / 1000000.0);

    if (pinfo->caplen == 0)
        return (1);
    if (pinfo->caplen > MAX_PKT_SIZE)
        errexit("packet too big", NULL);

    /* 3. read the packet contents (caplen bytes) */
    bytes_read = read(fd, pinfo->pkt,pinfo->caplen);

    // Some error checking
    if (bytes_read < 0)
        errexit("error reading packet", NULL);
    if (bytes_read < pinfo->caplen)
        errexit("unexpected end of file encountered", NULL);
    if (bytes_read < sizeof(struct ether_header))
        return (1);

    // Set ethernet header (first 14 bytes)
    pinfo->ethh = (struct ether_header *)pinfo->pkt;
    pinfo->ethh->ether_type = ntohs (pinfo->ethh->ether_type);

    // Ignore anything that is not IP
    if (pinfo->ethh->ether_type != ETHERTYPE_IP)
        /* nothing more to do with non-IP packets */
        return (1);
    if (pinfo->caplen == sizeof (struct ether_header))
        /* we don't have anything beyond the ethernet header to process */
        return (1);

    /* set pinfo->iph to start of IP header */

    /* if TCP packet, 
          set pinfo->tcph to the start of the TCP header
          setup values in pinfo->tcph, as needed */

    /* if UDP packet, 
          set pinfo->udph to the start of the UDP header,
          setup values in pinfo->udph, as needed */

    // * TODO -s: turn two timestamps into double
    return (1);
}

void print_packet(struct pkt_info *pinfo)
{
    printf("Packet: %d bytes\n", pinfo->caplen);
}


/**
 * Main entry point of program.
 * */
int main(int argc, char *argv[])
{
    char *TRACE_FILENAME;
    int fd;
    struct pkt_info pinfo = {0};
    int total_pkts = 0;
    int ip_pkts = 0;
    double first_pkt = 0.0;
    double last_pkt = 0.0;

    printv("Starting project 4...\n", NULL);
    parse_args(argc, argv, &TRACE_FILENAME);
    check_required_args();

    // Open trace file
    if ((fd = open(TRACE_FILENAME, O_RDONLY)) < 0)
        errexit("cannot open trace file %s", TRACE_FILENAME);
    
    // Start reading packets
    while (next_packet(fd, &pinfo) == 1)
    {
        // printf("Read packet: %d\n", total_packets);
        if (total_pkts == 0)
            first_pkt = pinfo.now;

        last_pkt = pinfo.now;

        if (pinfo.ethh->ether_type == ETHERTYPE_IP)
            ip_pkts++;
        
        total_pkts++;
    }

    printf("FIRST PKT: %f\n", first_pkt);
    printf("LAST PKT: %f\n", last_pkt);
    printf("TOTAL PACKETS: %d\n", total_pkts);
    printf("IP PACKETS: %d\n", ip_pkts);

    exit(0);
}
