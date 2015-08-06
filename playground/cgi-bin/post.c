#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void getvalue(char *s, char *value){
    char *p=strchr(s, '=');
    strcpy(value, p+1);
}

int main(int argc, char *argv[]){
    char *buf;
    char name[BUFSIZ], content[BUFSIZ];
    buf = getenv("QUERY_STR");
    getvalue(buf, name);

    sprintf(content, "<p>Welcome to my castle!</p>\r\n<p>Nice to meet you %s!</p>\r\n", name);

    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);
    return 0;
}

