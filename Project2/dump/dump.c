#include <ctype.h>
#include <stdio.h>

int main (int argc, char *argv [])
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
}

