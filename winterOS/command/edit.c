#include "const.h"
#include "console.h"
#include "stdio.h"

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("edit : missing file operand\n");
        return 0;
    }
    if (argc > 2)
    {
        printf("edit : too many files ");
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
            printf("edit : file open faild\n");
        else
        {
            clear(1); // 清屏 

            set_color((u8)(MAKE_COLOR(BLACK, GREEN) | BRIGHT));
            printf("       ~~~~~~~~~~~~~~~~~~~~~~~~~"
                   " winterOS EDIT!"
                   " ~~~~~~~~~~~~~~~~~~~~~~~~~       \n");
            set_color((u8)(MAKE_COLOR(BLACK, WHITE)));

            struct stat _stat;
            int _cnt;
            stat(argv[1], &_stat); // get file stat ...

            char _buf[NR_DEFAULT_FILE_SECTS << SECTOR_SIZE_SHIFT];
            _cnt = read(_fd, _buf, _stat.st_size);
            _buf[_cnt] = 0;

            MESSAGE msg;
            msg.type = READ;
            msg.FD = 1;
            msg.BUF = _buf;
            msg.CNT = NR_DEFAULT_FILE_SECTS << SECTOR_SIZE_SHIFT; // so we request so many ...
            msg.REQUEST = EDIT;
            send_recv(BOTH, TASK_FS, &msg);
            _buf[msg.CNT] = 0;

            clear(_fd); // clear the file
            clear(1); // clear the screen
            _cnt = write(_fd, _buf, msg.CNT);
            assert(_cnt == msg.CNT);
        }
    }
    return 0;
}
