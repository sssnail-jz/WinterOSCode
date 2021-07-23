#include "stdio.h"
#include "string.h"

int main(int argc, char *argv[])
{
    // ls -a, print '-a'
    if (argc == 2)
    {
        if (!strcmp(argv[1], "-l") || !strcmp(argv[1], "/"))
            __asm__ __volatile__("NOP");
        else
        {
            printf("ls: cannot access  %s: No such directory\n", argv[1]);
            return 0;
        }
    }

    // ls -l a bc, print 'a', 'bc'
    if (argc > 2)
    {
        if ((argc == 3) && !strcmp(argv[1], "-l") && !strcmp(argv[2], "/"))
            __asm__ __volatile__("NOP");
        else
        {
            int i = 1;
            while (--argc)
            {
                if (strcmp(argv[i], "-l") && strcmp(argv[i], "/"))
                    printf("ls: cannot access  %s: No such directory\n", argv[i]);
                ++i;
            }
            return 0;
        }
    }

    char _buf[1024 * 5];         // 能容纳 70 个文件的信息
    int _cnt = files_info(_buf); // go
    struct file_info *_pFileInfo = (struct file_info *)_buf;
    char *_strType = NULL;
    int _total;
    int _cnt_record = _cnt;
    while (_cnt--)
        _total += (_pFileInfo++)->size;
    _pFileInfo = (struct file_info *)_buf;
    while (_cnt_record--)
    {
        if (argc == 1 || ((argc == 2) && !strcmp(argv[1], "/")))
        {
            printf("%s  ", _pFileInfo->name);
            if (_cnt_record == 0)
                printf("\n");
        }

        if ((argc == 2 && !strcmp(argv[1], "-l")) || argc == 3)
        {
            if (_pFileInfo->type == I_REGULAR)
                _strType = "*REGULAR";
            else if (_pFileInfo->type == I_DIRECTORY)
                _strType = "+DIRECTORY";
            else if (_pFileInfo->type == I_CHAR_SPECIAL)
                _strType = "~CHAR_SPECIAL";

            if (_pFileInfo == (struct file_info *)_buf)
                printf("total %d\n", _total);
            printf("Type %-13s iNr %-2d refCnt %-2d size %-8d  %s\n",
                   _strType, _pFileInfo->inode_nr, _pFileInfo->ref_cnt,
                   _pFileInfo->size, _pFileInfo->name);
        }
        _pFileInfo++;
    }

    return 0;
}
