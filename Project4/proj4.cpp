/**
 * CSDS 325 Project 4 (2022 Fall Semester)
 * 
 * Author: Allard Quek
 * Case network ID: axq54
 * Filename: proj4.c
 * Date Created: 9 November 2022
 *
 * The aim of this project is to think about packets the way devices (e.g. computers, routers, switches, etc.)
 * process packets. Further, this project will deepen your understanding of protocol layering by making you 
 * navigate the protocol stack.
 * */


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <utility>
#include <iterator>
#include <unordered_map>
#include <string>

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
#include "arpa/inet.h"
#include <inttypes.h>

// Define constant macros (from sample code)
#define ERROR 1
#define ERROR_PREFIX "ERROR: "
#define MICRO_FACTOR 1 / 1000000.0
#define WORD_SIZE 4
#define TCP 'T'
#define UDP 'U'
#define MISSING '-'
#define UNKNOWN '?'

using namespace std;

typedef std::pair<std::string, std::string> SrcDstPair;
typedef map<SrcDstPair, int> TrafficMatrix;

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
static const int ETHER_HEADER_SIZE = sizeof(struct ether_header);


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
void printv(const char *msg_format, char *arg)
{
    if (is_option_v)
        fprintf(stdout, msg_format, arg);
}


/**
 * Prints error message to stderr and exits the program.
 * */
int errexit(const char *msg_format, char *arg)
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
    fd - an open file to read packets from
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
        errexit("Packet too big", NULL);

    // 3. Read packet contents (caplen bytes)
    bytes_read = read(fd, pinfo->pkt, pinfo->caplen);

    // Some error checking
    if (bytes_read < 0)
        errexit("Error reading packet", NULL);
    if (bytes_read < pinfo->caplen)
        errexit("Unexpected end of file encountered", NULL);
    if (bytes_read < ETHER_HEADER_SIZE)
        return (1);

    // a. Set ethernet header (first 14 bytes right after meta info)
    pinfo->ethh = (struct ether_header *) pinfo->pkt;
    pinfo->ethh->ether_type = ntohs(pinfo->ethh->ether_type);   // Convert network byte order

    // Ignore anything that is not IP and has nothing beyond ethernet header to process
    bool is_ip = (pinfo->ethh->ether_type == ETHERTYPE_IP);
    bool has_only_ethernet_header = (pinfo->caplen == ETHER_HEADER_SIZE);
    if (!is_ip || has_only_ethernet_header)
        return (1);

    // b. Set iph to start of IP header by skipping ethernet header (struct ip or struct iphdr)
    pinfo->iph = (struct ip *) (pinfo->pkt + ETHER_HEADER_SIZE);
    pinfo->iph->ip_len = ntohs(pinfo->iph->ip_len);
    int ip_header_size = pinfo->iph->ip_hl * WORD_SIZE;

    
    if (pinfo->iph->ip_p == IPPROTO_TCP) 
    {
        /* ci. if TCP packet, 
            set pinfo->tcph to the start of the TCP header
            setup values in pinfo->tcph, as needed */
        pinfo->tcph = (struct tcphdr *) (pinfo->pkt + ETHER_HEADER_SIZE + ip_header_size);
        pinfo->tcph->th_sport = ntohs(pinfo->tcph->th_sport);
        pinfo->tcph->th_dport = ntohs(pinfo->tcph->th_dport);
        pinfo->tcph->th_win = ntohs(pinfo->tcph->th_win);
        pinfo->tcph->th_seq = ntohl(pinfo->tcph->th_seq);
        pinfo->tcph->th_ack = ntohl(pinfo->tcph->th_ack);
        pinfo->tcph->th_flags = pinfo->tcph->th_flags & 0x3f;
    }
    else if (pinfo->iph->ip_p == IPPROTO_UDP) 
    {
        /* cii. if UDP packet, 
            set pinfo->udph to the start of the UDP header, (NOT: sizeof(struct ip) at the end)
            setup values in pinfo->udph, as needed */
        pinfo->udph = (struct udphdr *) (pinfo->pkt + ETHER_HEADER_SIZE + ip_header_size);
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
    while (next_packet(fd, &pinfo))
    {
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
 * Format: ts caplen ip_len iphl transport trans_hl payload_len
*/
void length_mode(int fd, struct pkt_info pinfo)
{
    while (next_packet(fd, &pinfo) == 1)
    {
        // Remember 1 is still returned for non-ip packets
        if (pinfo.ethh->ether_type != ETHERTYPE_IP)
            continue;

        double ts = pinfo.now;
        int caplen = pinfo.caplen;

        if (pinfo.iph == NULL)
        {
            printf("%f %d %c %c %c %c %c\n", ts, caplen, MISSING, MISSING, MISSING, MISSING, MISSING);
            continue;
        }
        
        int ip_len = pinfo.iph->ip_len;
        int iphl = pinfo.iph->ip_hl * WORD_SIZE;
        
        if (pinfo.iph->ip_p == IPPROTO_TCP) 
        {
            // th_off is the data offset
            if (pinfo.tcph->th_off == 0)
            {
                printf("%f %d %d %d %c %c %c\n", ts, caplen, ip_len, iphl, TCP, MISSING, MISSING);
            }
            else
            {
                int trans_hl = pinfo.tcph->th_off * 4;
                int payload_len = ip_len - iphl - trans_hl;
                printf("%f %d %d %d %c %d %d\n", ts, caplen, ip_len, iphl, TCP, trans_hl, payload_len);
            }
            
        } 
        else if (pinfo.iph->ip_p == IPPROTO_UDP)
        {
            if (pinfo.udph->uh_ulen == 0)
            {
                printf("%f %d %d %d %c %c %c\n", ts, caplen, ip_len, iphl, UDP, MISSING, MISSING);
            }
            else
            {
                int trans_hl = sizeof(struct udphdr);
                int payload_len = ip_len - iphl - trans_hl;
                printf("%f %d %d %d %c %d %d\n", ts, caplen, ip_len, iphl, UDP, trans_hl, payload_len);
            }
        }
        else 
        {
            printf("%f %d %d %d %c %c %c\n", ts, caplen, ip_len, iphl, UNKNOWN, UNKNOWN, UNKNOWN);
        }
    }
}


/**
 * Handles -p option by printing a single line of information about each TCP packet in the packet trace file.
 * Format: ts | src_ip | dst_ip | ip_tll | src_port | dst_port | window | seqno | ackno
*/
void packet_printing_mode(int fd, struct pkt_info pinfo)
{
    while (next_packet(fd, &pinfo) == 1)
    {
        if (pinfo.ethh->ether_type != ETHERTYPE_IP)
            continue;

        if (pinfo.iph->ip_p != IPPROTO_TCP) 
        {
            continue;
        }

        double ts = pinfo.now;
        char src_ip[INET_ADDRSTRLEN];
        char dst_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(pinfo.iph->ip_src), src_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(pinfo.iph->ip_dst), dst_ip, INET_ADDRSTRLEN);
        int ip_ttl = pinfo.iph->ip_ttl;
        int src_port = pinfo.tcph->th_sport;
        int dst_port = pinfo.tcph->th_dport;
        int window = pinfo.tcph->th_win;
        int seqno = pinfo.tcph->th_seq;
        
        // Check if flags field shows that ACK bit is set to 1
        if (pinfo.tcph->th_flags & TH_ACK)
        {
            int ackno = pinfo.tcph->th_ack;
            printf("%f %s %s %d %d %d %d %" PRIu32 " %" PRIu32 "\n", ts, src_ip, dst_ip, ip_ttl, src_port, dst_port, window, seqno, ackno);
        }
        else
        {
            printf("%f %s %s %d %d %d %d %" PRIu32 " %c\n", ts, src_ip, dst_ip, ip_ttl, src_port, dst_port, window, seqno, MISSING);
        }
    }
}


/**
 * Prints the keys and values of the traffic matrix map.
*/
void print_traffic_matrix(TrafficMatrix traffic_matrix)
{
    for (const auto &entry: traffic_matrix)
    {
        auto key_pair = entry.first;
        std::cout << key_pair.first << " " 
                  << key_pair.second << " "
                  << entry.second << std::endl;
    }
}


/**
 * Handles -m option by operating in "traffic matrix mode".
*/
void traffic_matrix_mode(int fd, struct pkt_info pinfo)
{
    TrafficMatrix traffic_matrix;

    while (next_packet(fd, &pinfo) == 1)
    {
        if (pinfo.ethh->ether_type != ETHERTYPE_IP)
            continue;

        // Ignore non-TCP packets
        if (pinfo.iph->ip_p != IPPROTO_TCP || pinfo.tcph->th_off == 0) 
            continue;

        // Keep track of payload length between source and destination ip addresses in traffix_matrix
        char src_ip[INET_ADDRSTRLEN];
        char dst_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(pinfo.iph->ip_src), src_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(pinfo.iph->ip_dst), dst_ip, INET_ADDRSTRLEN);

        int ip_len = pinfo.iph->ip_len;
        int iphl = pinfo.iph->ip_hl * WORD_SIZE;
        int trans_hl = pinfo.tcph->th_off * 4;
        int payload_len = ip_len - iphl - trans_hl;

        // Keep track of payload_len traffic between (src_ip, dst_ip) pair
        SrcDstPair pair = std::make_pair(src_ip, dst_ip);
        TrafficMatrix::iterator it = traffic_matrix.find(pair);

        // Increment payload length if src-dst pair already exists in traffic_matrix
        if (it != traffic_matrix.end()) 
        {
            it->second += payload_len;
        }
        else 
        {
            traffic_matrix.insert(std::make_pair(pair, payload_len));
        }
    }

    print_traffic_matrix(traffic_matrix);
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

    // Handle single option provided
    if (is_option_s) 
    {
        summary_mode(fd, pinfo);
    }
    else if (is_option_l) 
    {
        length_mode(fd, pinfo);
    }
    else if (is_option_p) 
    {
        packet_printing_mode(fd, pinfo);
    }
    else if (is_option_m) 
    {
        traffic_matrix_mode(fd, pinfo);
    }
    else 
    {
        errexit("No valid option was provided.", NULL);
    }

    close(fd);
    exit(0);
}
