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
#define MICRO_FACTOR 1 / 1000000.0

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
    memset(pinfo, 0x0, sizeof(struct pkt_info));
    memset(&meta, 0x0, sizeof(struct meta_info));

    /* 1. read the meta information (12 bytes) */
    bytes_read = read(fd, &meta, sizeof(meta));
    if (bytes_read == 0)
        return (0);
    if (bytes_read < sizeof(meta))
        errexit("cannot read meta information", NULL);

    // 2. Set caplen attribute of pkt_info struct
    pinfo->caplen = ntohs(meta.caplen);  

    // 3. Set now attribute based on meta.secs & meta.usecs
    pinfo->now = ntohl(meta.secs) + (ntohl(meta.usecs) * MICRO_FACTOR);

    if (pinfo->caplen == 0)
        return (1);
    if (pinfo->caplen > MAX_PKT_SIZE)
        errexit("packet too big", NULL);

    // 3. Read packet contents (caplen bytes)
    bytes_read = read(fd, pinfo->pkt, pinfo->caplen);

    // Some error checking
    if (bytes_read < 0)
        errexit("error reading packet", NULL);
    if (bytes_read < pinfo->caplen)
        errexit("unexpected end of file encountered", NULL);
    if (bytes_read < sizeof(struct ether_header))
        return (1);

    // a. Set ethernet header (first 14 bytes right after meta info)
    pinfo->ethh = (struct ether_header *) pinfo->pkt;

    // Simply use ntohs to convert the initial value
    pinfo->ethh->ether_type = ntohs(pinfo->ethh->ether_type);

    // Ignore anything that is not IP
    if (pinfo->ethh->ether_type != ETHERTYPE_IP)
        // Nothing more to do with non-IP packets
        // printv("Non-IP packet ignored\n", NULL);
        return (1);
    if (pinfo->caplen == sizeof(struct ether_header))
        // We don't have anything beyond the ethernet header to process
        return (1);

    // b. Set iph to start of IP header by skipping ethernet header (struct ip or struct iphdr)
    pinfo->iph = (struct ip *) (pinfo->pkt + sizeof(struct ether_header));

    // Convert network byte orders (note that we do not need to set the ip_len attribute)
    pinfo->iph->ip_len = ntohs(pinfo->iph->ip_len);
    // printf("IP len: %d", pinfo->iph->ip_len);
    // pinfo->iph->ip_hl = pinfo->iph->ip_hl * 4;

    /* c. if TCP packet, 
          set pinfo->tcph to the start of the TCP header
          setup values in pinfo->tcph, as needed */
    if (pinfo->iph->ip_p == IPPROTO_TCP) 
    {
        pinfo->tcph = (struct tcphdr *) (pinfo->pkt + sizeof(struct ether_header) + pinfo->iph->ip_hl * 4);
        // pinfo->tcph->th_sport = ntohs(pinfo->tcph->th_sport);
        // pinfo->tcph->th_dport = ntohs(pinfo->tcph->th_dport);
    }

    /* d. if UDP packet, 
          set pinfo->udph to the start of the UDP header,
          setup values in pinfo->udph, as needed */
    else if (pinfo->iph->ip_p == IPPROTO_UDP) 
    {
        // NOT: sizeof(struct ip) at the end
        pinfo->udph = (struct udphdr *) (pinfo->pkt + sizeof(struct ether_header) + pinfo->iph->ip_hl * 4);
        // pinfo->udph->uh_ulen = ntohl(pinfo->udph->uh_ulen);
        // pinfo->udph->uh_sport = ntohs(pinfo->udph->uh_sport);
        // pinfo->udph->uh_dport = ntohs(pinfo->udph->uh_dport);
    }

    return (1);
}


/**
 * Handles -s option by printing a high-level summary of the trace file.
*/
void summary_mode(int fd, struct pkt_info pinfo)
{
    int total_pkts = 0;
    int ip_pkts = 0;
    double first_pkt = 0.0;
    double last_pkt = 0.0;

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
}



/**
 * Handles -l option by printing length information about each IPv4 paket in the packet trace file.
 * Format: ts | caplen | ip_len | iphl | transport | trans_hl | payload_len
*/
void length_mode(int fd, struct pkt_info pinfo)
{
    while (next_packet(fd, &pinfo) == 1)
    {
        // Remember 1 is still returned for non-ip packets
        if (pinfo.ethh->ether_type != ETHERTYPE_IP)
            // ? Why cannot print here
            continue;

        int ip_len = pinfo.iph->ip_len;
        int iphl = pinfo.iph->ip_hl * 4;    // ? Why multiply by 4 here
        int transport = pinfo.iph->ip_p;
        int trans_hl = '-';

        if (pinfo.iph == NULL)
        {
            // printf("NO HEADER\n");
            ip_len = '-'; 
            iphl = '-';
        }
        
        if (transport == IPPROTO_TCP) 
        {
            transport = 'T';
            trans_hl = pinfo.tcph->th_off * 4;
            // ? why not trans_hl = sizeof(struct tcphdr);
        } 
        else if (transport == IPPROTO_UDP)
        {
            transport = 'U';
            // ? why not trans_hl = pinfo.udph->uh_ulen * 4;
            trans_hl = sizeof(struct udphdr);
        }
        else {
            transport = '?';
            trans_hl= '?';
        }

        // print this format: Format: ts caplen ip_len iphl transport trans_hl payload_len
        printf("%f %d %d %d %c %d %d\n", pinfo.now, pinfo.caplen, ip_len, iphl, transport, trans_hl, ip_len - iphl - trans_hl);
    }
}


/**
 * Handles -p option by printing a single line of information about each TCP packet in the packet trace file.
*/
void packet_printing_mode(int fd, struct pkt_info pinfo)
{
    printf("Printing single line information about packet...\n");
}


/**
 * Handles -m option by operating in "traffic matrix mode".
*/
void traffic_matrix_mode(int fd, struct pkt_info pinfo)
{
    printf("Operating in traffic matrix mode...\n");
}


/**
 * Main entry point of program.
 * */
int main(int argc, char *argv[])
{
    char *TRACE_FILENAME;
    int fd;
    struct pkt_info pinfo = {0};

    printv("Starting project 4...\n", NULL);
    parse_args(argc, argv, &TRACE_FILENAME);
    check_required_args();

    // Open trace file
    if ((fd = open(TRACE_FILENAME, O_RDONLY)) < 0)
        errexit("cannot open trace file %s", TRACE_FILENAME);

    if (is_option_s) {
        summary_mode(fd, pinfo);
    }

    if (is_option_l) {
        length_mode(fd, pinfo);
    }

    if (is_option_p) {
        printf("Option p selected.\n");
    }

    if (is_option_m) {
        printf("Option m selected.\n");
    }

    close(fd);
    exit(0);
}
