#include <ctype.h>
#include <stdio.h>

int main (int argc, char *argv [])
{
	char str[] = "HELLO world";

	for(int i = 0; str[i]; i++){
		str[i] = tolower(str[i]);
	}
	printf("%s\n", str);
}