#include <ctype.h>
#include <stdio.h>

int main(int argc, char *argv [])
{
	// * Upper to lower case
	char str[] = "HELLO world";

	for(int i = 0; str[i]; i++){
		str[i] = tolower(str[i]);
	}
	printf("%s\n", str);

	// * `strtok` example
	char *path = strtok(NULL, PATH_DELIMITER);

	// Print other tokens in the path
	while (path != NULL ) {
		printf( "PATH: %s\n", path );
		path = strtok(NULL, PATH_DELIMITER);
	}

	// * Snarfing
	/* snarf whatever server provides and print it */
	printf("Snarfing whatver server provides...\n");
    memset (buffer, 0x0, BUFLEN);
    ret = read (sd, buffer, BUFLEN - 1);

    if (ret < 0)
        errexit ("reading error", NULL);

    fprintf (stdout, "%s\n", buffer);
	printf("PRINTED BUFFER\n");


	// TODO: Fix segmentation fault
	while ((n = read(sd, buffer, BUFLEN)) > 0) 
	{
		buffer[n] = '\0';

		if(fputs(buffer, stdout) == EOF)
		{
			printf("fputs() error\n");
		}

		/// Remove the trailing chars
		ptr = strstr(buffer, "\r\n\r\n");

		// check len for OutResponse here ?
		snprintf(http_response, BUFLEN,"%s", ptr);
	}

	// * Stackoverflow example
	http_response = readMsg(sd, &msgSize); // response from server
    if (!http_response) {
        close(sd);
        return 1;
    }
    printf("# of printable characters: %.*s\n", (int)msgSize, http_response);
    free(http_response);


    // * optind is for the extra arguments which are not parsed
	for(; optind < argc; optind++){	
		printf("extra arguments: %s\n", argv[optind]);
	}

    // * Attempt to malloc and initalize string
    url = malloc(strlen(optarg) + 1);

    Get the elements of the array
    for (int i = 0; i < strlen(optarg); i++) {
        url[i] = optarg[i];
    }

    url pointer should point to address of local_url
    *url = local_url;


    // * Print debug
    printf("BEFORE URL is: %s\n", url);
	printf("BEFORE HOST PATH is: %s\n", *host_and_path);
	printf("NOW URL is: %s\n", url);
	printf("NOW HOST PATH is: %s\n", *host_and_path);


    // * Naive read of HTTP response string
    // * Read HTTP response from server
	// ? Why cannot place `read` outside of while loop?
	// ? What if the while loop is executed multiple times?
    while ((bytes_read = read(sd, recvline, BUFLEN)) > 0) 
    {
		strcpy(http_response, recvline);
        printv("Here is the HTTP response:\n----------\n%s----------\n", recvline);

		// ? Zero out after printing current line
		memset(recvline, 0, BUFLEN);	
    }   

	if (bytes_read < 0)
		errexit("Error reading from socket!", NULL); 


    // * Sample read HTTP response code
	// read the 1st line
	if (fgets(http_response, sizeof(http_response) + 1, fd) == NULL) {
		if (ferror(fd))
			errexit("IO error", NULL);
		else {
			errexit("server terminated connection without response", NULL);
		}
	} 

	// Check if we get "HTTP/1.0" or "HTTP/1.1"
	printf("HERE http_response: %s\n", http_response);
	if (strncmp("HTTP/1.0", http_response, 9) != 0 && strncmp("HTTP/1.1", http_response, 9) != 0) {
		errexit("unknown protocol response: %s\n", http_response);
	}

	// DO NOT PROCEED TO WRITE if we don't get response code 200
	if (strncmp("200", http_response + 9 + 1, 3) != 0) {
		printf("PRINTING: RESPONSE CODE IS NOT 200, NO WRITING\n");
		return;
	}

	// If we're here, it means we have a successful HTTP
	// response (i.e., response code 200).


	// * Get content and headers from HTTP response
	// Remember: content will include end of header (need to strip end of header)
	http_content = strstr(http_response, END_OF_HEADER) + strlen(END_OF_HEADER);	
	strncpy(http_headers, http_response, strlen(http_response) - strlen(http_content));
	free(http_response);
}



char* readMsg(int sockfd, size_t *msgSize)
{
    *msgSize = 0;

    unsigned int length = 0;
    int bytes_read = read(sockfd, &length, sizeof(length)); //Receive number of bytes
    if (bytes_read <= 0) {
        perror("Error in receiving message from server\n");
        return NULL;
    }
    length = ntohl(length);

    char *buffer = malloc(length+1);
    if (!buffer) {
        perror("Error in allocating memory to receive message from server\n");
        return NULL;
    }

    char *pbuf = buffer;
    unsigned int buflen = length;
    while (buflen > 0) {
        bytes_read = read(sockfd, pbuf, buflen); // Receive bytes
        if (bytes_read <= 0) {
            perror("Error in receiving message from server\n");
            free(buffer);
            return NULL;
        }
        pbuf += bytes_read;
        buflen -= bytes_read;
    }

    *msgSize = length;
    return buffer;
}


void handle_c() 
{
	char *final_output = "REQ: GET %s HTTP/1.0 \r\nREQ: Host: %s\r\nREQ: User-Agent: = %s\r\n";
	printf(final_output, url_filename, hostname, user_agent);
}



/**
 * Writes content to filename.
 * */
void write_to_file(char *filename, char *content) 
{
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		errexit("Error opening file!", NULL);
	}

	fprintf(fp, "%s", content);		// Write content to file
	fclose(fp);						// Remember to close file
	printv("Successfully saved HTTP content to %s!\n", filename);
}


/**
 * Handles the -o option.
 * Writes HTTP response content to the provided file, if response status was 200 OK.
 * */
void handle_o(char *output_filename)
{
	printv("\n========== Handling -o option ==========\n", NULL);
    // Check if status code was 200 OK
    if (strstr(http_headers, SUCCESS_CODE) == NULL)
    {
        printf("Response from server was not OK. Output will not be written to file.\n");
        return;
    }

	write_to_file(output_filename, http_content);
}


/**
 * Handles the -u option.
 * Makes HTTP request using the provided URL
 * */
void handle_u(char *url, char *output_filename, char **host_and_path, char **hostname, char **url_filename) 
{
	printv("\n========== Handling -u option ==========\n", NULL);
	if (is_valid_url(url) == 0) 
		errexit("Url must start with %s", URL_PREFIX);
	
	printv("Valid URL received: %s\n", url);

	// * Split url into hostname and url_filename
	set_host_and_path(url, host_and_path);
	set_hostname(*host_and_path, hostname);
	set_url_filename(url_filename, *host_and_path, *hostname);

	// * Make HTTP request
	// Pass the pointer to the required values, not the double pointers
	int sd = connect_to_socket(*hostname);
	send_http_request(sd, *hostname, *url_filename);
	read_http_response(sd, output_filename);
	close(sd);
}
