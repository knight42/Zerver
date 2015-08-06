#include <stdio.h>
#include <stdlib.h>
//#include <ctype.h>
#include <string.h>
//#include <math.h>
//#include <unistd.h>
//#define 
//typedef 

void getvalue(char *s, char *value){
    char *p=strchr(s, '=');
    strcpy(value, p+1);
}

int main(int argc, char *argv[]){
    char *buf, *p;
    char arg1[BUFSIZ], arg2[BUFSIZ], content[BUFSIZ];
    int n1=0, n2=0;
    buf = getenv("QUERY_STR");
    p=strchr(buf, '&');
    *p=0;
    getvalue(buf, arg1);
    getvalue(p+1, arg2);
    n1=atoi(arg1); n2=atoi(arg2);

    sprintf(content, "<p>Welcome! </p>\r\n");
    sprintf(content, "%s<p>The answer is %d + %d = %d</p>\r\n", content, n1, n2, n1+n2);

    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);
    return 0;
}

