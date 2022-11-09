
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "next.h"

void errexit (char *msg)
{
    fprintf (stdout,"%s\n",msg);
    exit (1);
}

/* fd - an open file to read packets from
   pinfo - allocated memory to put packet info into for one packet

   returns:
   1 - a packet was read and pinfo is setup for processing the packet
   0 - we have hit the end of the file and no packet is available 
 */
unsigned short next_packet (int fd, struct pkt_info *pinfo)
{
    struct meta_info meta;
    int bytes_read;

    // Clear out memory in both structs
    memset (pinfo,0x0,sizeof (struct pkt_info));
    memset (&meta,0x0,sizeof (struct meta_info));

    /* 1. read the meta information (12 bytes) */
    bytes_read = read (fd,&meta,sizeof (meta));
    if (bytes_read == 0)
        return (0);
    if (bytes_read < sizeof (meta))
        errexit ("cannot read meta information");

    // Set caplen attribute of pkt_info struct
    pinfo->caplen = ntohs (meta.caplen);    

    /* 2. set pinfo->now based on meta.secs & meta.usecs */
    if (pinfo->caplen == 0)
        return (1);
    if (pinfo->caplen > MAX_PKT_SIZE)
        errexit ("packet too big");

    /* 3. read the packet contents (caplen bytes) */
    bytes_read = read (fd, pinfo->pkt,pinfo->caplen);

    // Some error checking
    if (bytes_read < 0)
        errexit ("error reading packet");
    if (bytes_read < pinfo->caplen)
        errexit ("unexpected end of file encountered");
    if (bytes_read < sizeof (struct ether_header))
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
