#include "stdio.h"

int main(int argc, char * argv[])
{
    if(argc == 1)
    {
        printf("touch : missing file operand\n");
        return 0;
    }
    if(argc > 2)
    {
        printf("touch : too many files ");
        int i = 1;
        while(i != argc)
            printf("%s  ", argv[i++]);
        printf("\n");
        return 0;
    }
    if(argc == 2)
    {
        int _hd = open(argv[1], O_CREAT);
        if(_hd == -1)
            printf("touch : file create faild\n");
        else
            printf("touch : file create successful\n");
    }
    return 0;
}
