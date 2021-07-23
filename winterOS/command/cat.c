#include "const.h"
#include "stdio.h"

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("cat : missing file operand\n");
        return 0;
    }
    if (argc > 2)
    {
        printf("cat : too many files ");
        int i = 1;
        while (i != argc)
            printf("%s  ", argv[i++]);
        printf("\n");
        return 0;
    }
    if (argc == 2)
    {
        int _fd = open(argv[1], O_RDWT);
        if (_fd == -1)
            printf("cat : file open faild\n");
        else
        {
            struct stat _stat;
            int _cnt;
            stat(argv[1], &_stat); // get file stat ...
            if(_stat.st_size == 0)
                return 0;
            char _buf[NR_DEFAULT_FILE_SECTS << SECTOR_SIZE_SHIFT];
            _cnt = read(_fd, _buf, _stat.st_size);
            assert(_cnt == _stat.st_size);
            _buf[_cnt] = '\n'; // we add an new line when output ...
            _buf[_cnt + 1] = 0;
            _cnt = write(1, _buf, sizeof(_buf));
            assert(_cnt == sizeof(_buf));
        }
    }
    return 0;
}
