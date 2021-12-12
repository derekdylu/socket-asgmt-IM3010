#include <stdio.h>

int main()
{
    char *message;
    char input[1024];
    scanf("%s", input);
    message = input;
    printf("%s", message);
    return 0;
}