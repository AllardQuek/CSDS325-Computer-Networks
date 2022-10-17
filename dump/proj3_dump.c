if (http_request[bytes_read - 1] == '\n') 
{
	break;
}

if (strstr(http_request, "\r\n\r\n") != NULL) {
	break;
}