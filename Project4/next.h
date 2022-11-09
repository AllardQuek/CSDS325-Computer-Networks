
#define MAX_PKT_SIZE        1600

/* meta information, using same layout as trace file */
struct meta_info
{
    unsigned short caplen;
    unsigned short ignored;
    unsigned int secs;
    unsigned int usecs;
};

/* record of information about the current packet */
struct pkt_info
{
    unsigned short caplen;      /* from meta info */
    double now;                 /* from meta info */
    unsigned char pkt [MAX_PKT_SIZE];
    struct ether_header *ethh;  /* ptr to ethernet header, if fully present,
                                   otherwise NULL */
    struct iphdr *iph;          /* ptr to IP header, if fully present, 
                                   otherwise NULL */
    struct tcphdr *tcph;        /* ptr to TCP header, if fully present,
                                   otherwise NULL */
    struct udphdr *udph;        /* ptr to UDP header, if fully present,
                                   otherwise NULL */
};

// void errexit ();
unsigned short next_packet ();
