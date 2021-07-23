#include "stdio.h"
#include "string.h"
#define MAX_STUDENT 10

struct _student _temp;
char _buf[20];
int _ID;
int _score;
int _choice;

struct _student
{
    int ID;
    int score;
    char name[10];
};

struct _stuSystem
{
    struct _student stu_array[MAX_STUDENT];
    int count;
};
struct _stuSystem StuSystem;

void stu_add()
{
    printf("name  :  ");
    scanf("%s", _buf);
    strcpy(_temp.name, _buf);
    _temp.name[9] = 0;
    printf("ID    :  ");
    scanf("%d", &_ID);
    _temp.ID = _ID;
    printf("score :  ");
    scanf("%d", &_score);
    _temp.score = _score;

    if (StuSystem.count == MAX_STUDENT)
        printf("--StuSystem is full !\n");
    else
    {
        StuSystem.stu_array[StuSystem.count] = _temp;
        StuSystem.count++;
    }
}

void stu_browse()
{
    if (StuSystem.count == 0)
    {
        printf("--node : StuSystem is empty!\n");
        return;
    }
    int i = 0;
    while (i != StuSystem.count)
    {
        printf("\n");
        printf("name  : %s\n", StuSystem.stu_array[i].name);
        printf("ID    : %d\n", StuSystem.stu_array[i].ID);
        printf("score : %d\n", StuSystem.stu_array[i].score);
        ++i;
    }
    printf("\n");
}

void stu_delete()
{
    if (StuSystem.count == 0)
        printf("--StuSystem already empty!\n");
    else
        StuSystem.count--;
}

void stu_counter()
{
    printf("cnter :%d\n", StuSystem.count);
}

void stu_sch_name()
{
    int i = 0;
    int _find = FALSE;
    printf("name  :");
    scanf("%s", _buf);
    while (i != StuSystem.count)
    {
        if (!strcmp(StuSystem.stu_array[i].name, _buf))
        {
            printf("\n");
            printf("name  : %s\n", StuSystem.stu_array[i].name);
            printf("ID    : %d\n", StuSystem.stu_array[i].ID);
            printf("score : %d\n", StuSystem.stu_array[i].score);
            _find = TRUE;
        }
        ++i;
    }
    if (!_find)
        printf("--not find !\n");
    else
        printf("\n");
}

void stu_sch_ID()
{
    int i = 0;
    int _find = FALSE;
    printf("ID    :");
    scanf("%d", &_ID);
    while (i != StuSystem.count)
    {
        if (StuSystem.stu_array[i].ID == _ID)
        {
            printf("\n");
            printf("name  : %s\n", StuSystem.stu_array[i].name);
            printf("ID    : %d\n", StuSystem.stu_array[i].ID);
            printf("score : %d\n", StuSystem.stu_array[i].score);
            _find = TRUE;
        }
        ++i;
    }
    if (!_find)
        printf("--not find !\n");
    else
        printf("\n");
}

void stu_save()
{
    if (StuSystem.count == 0)
    {
        printf("save faild : empty!\n");
        return;
    }
    else
    {
        int i = 0;
        int _cnt = 0;
        int _fd = -1;
        _fd = open("result.txt", O_CREAT | O_RDWT);
        char _buf[100];
        assert(_fd != -1);
        clear(_fd);
        while (i != StuSystem.count)
        {
            _cnt = sprintf(_buf, "name : %-10s   ID : %-10d    score : %-3d\n",
                           StuSystem.stu_array[i].name, StuSystem.stu_array[i].ID, StuSystem.stu_array[i].score);
            _buf[_cnt] = 0;
            write(_fd, _buf, _cnt);
            i++;
        }
        printf("save sucessful!\n");
    }
}

int main(int argc, char *argv[])
{
    printf("\n*******Welcome to stuSystem on WinterOS*******\n");
    printf("**     1, input new info                    **\n");
    printf("**     2, browse total                      **\n");
    printf("**     3, delete one info                   **\n");
    printf("**     4, count the counter                 **\n");
    printf("**     5, sort by score                     **\n");
    printf("**     6, sort by ID                        **\n");
    printf("**     7, search by name                    **\n");
    printf("**     8, search by ID                      **\n");
    printf("**     9, save and exit                     **\n");
    printf("*******Welcome to stuSystem on WinterOS*******\n\n");
    StuSystem.count = 0;

    int _fd = -1;
    int _cnt = 0;
    char _buf[80 * 100];
    _fd = open("result.txt", O_CREAT | O_RDWT);
    if (_fd == -1)
    {
        printf("open result.txt faild!\n");
        return 0;
    }
    else
        printf("<<load the result.txt successful !!!>>\n\n");
    seek(_fd, 0, SEEK_SET); // 移动文件指针到头
    _cnt = read(_fd, _buf, sizeof(_buf));
    struct stat _stat;
    stat("result.txt", &_stat);

    _buf[_cnt] = 0;
    char *_iterator = _buf;
    int _index;
    char *_subStr_1 = "name : ";
    char *_subStr_2 = "ID : ";
    char *_subStr_3 = "score : ";

    while (TRUE)
    {

        _index = subIndex_tail(_iterator, _subStr_1);
        if (_index == -1)
            break;
        memcpy(_temp.name, _iterator + _index, 9);
        _temp.name[9] = 0;
        _index = subIndex_tail(_iterator, _subStr_2);
        _temp.ID = a2i(_iterator + _index);
        _index = subIndex_tail(_iterator, _subStr_3);
        _temp.score = a2i(_iterator + _index);

        StuSystem.stu_array[StuSystem.count] = _temp;
        StuSystem.count++;
        _iterator += (_index + 4); // next colum
    }

    while (TRUE)
    {
        printf("Please input your choice :  ");
        scanf("%d", &_choice);
        switch (_choice)
        {
        case 1: // input new info
            stu_add();
            break;
        case 2: // browse total
            stu_browse();
            break;
        case 3: // delete one info
            stu_delete();
            break;
        case 4: // count the counter
            stu_counter();
            break;
        case 7: // search by name
            stu_sch_name();
            break;
        case 8: // search by ID
            stu_sch_ID();
            break;
        case 9: // exit and save
            stu_save();
            goto _return;
        default:
            break;
        }
    }

_return:
    return 0;
}
