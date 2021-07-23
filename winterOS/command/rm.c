#include "stdio.h"

int main(int argc, char * argv[])
{
    if(argc == 1)
    {
        printf("rm : missing file operand\n");
        return 0;
    }
    if(argc > 2)
    {
        printf("rm : too many files ");
        int i = 1;
        while(i != argc)
            printf("%s  ", argv[i++]);
        printf("\n");
        return 0;
    }
    if(argc == 2)
    {
        int _hd = unlink(argv[1]);
        if(_hd == -1)
            printf("rm : file rm faild\n");
        else
            printf("rm : file rm successful\n");
    }
    return 0;
}
