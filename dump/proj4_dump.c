// * ? -s: turn two timestamps into double
printf("%f %d %d %d %c %d %d\n", pinfo.now, pinfo.caplen, ip_len, iphl, transport, trans_hl, ip_len - iphl - trans_hl);

// Check for presence of tcp/udp header: if (caplen <= 34)

printf("map size: %d\n", traffic_matrix.size());