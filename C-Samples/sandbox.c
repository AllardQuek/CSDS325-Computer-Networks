#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	char *http_response = malloc(1024);
	printf("Size of http_response: %zu", sizeof(http_response));	// ? Not 1024?

	// char sample_str[6] = "abcde";
	// printf("length of sample_str: %zu\n", strlen(sample_str));
	// free(sample_str);
	// printf("length of sample_str: %zu\n", strlen(sample_str));
}
