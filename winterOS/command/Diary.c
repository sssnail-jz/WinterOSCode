#include "stdio.h"
#include "const.h"
#include "string.h"

#define A_WEEK 7

struct _term
{
    char date[20];        // 我们不怕浪费内存或者磁盘
    char score[5];        // your score
    char string[80 * 25]; // 你最好写得不要超过 1 个屏幕
};

struct _diary
{
    struct _term term_arr[A_WEEK]; // OK，我们一周一清
    int count;                     // 目前存放了多少条日记了
} Diary;

void diary_init();
void diary_add();
void diary_modify();
void diary_delete();
void diary_browse();
void diary_find();
void diary_exit();
struct _term *_find(char *_date);
void _show_options();

int main(int argc, char *argv[])
{
    // 初始化
    diary_init();

    while (TRUE)
    {
        int _choice;
        set_color((u8)(MAKE_COLOR(BLACK, GREEN) | BRIGHT));
        printf("--Diary Choice  ");
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        scanf("%d", &_choice);

        switch (_choice)
        {
        case 1: // add
            diary_add();
            break;
        case 2: // modify
            diary_modify();
            break;
        case 3: // delete
            diary_delete();
            break;
        case 4: // browse
            diary_browse();
            break;
        case 5: // find
            diary_find();
            break;
        case 6: // exit
            diary_exit();
            return 0;
        default:
            break;
        }

        set_color((u8)(MAKE_COLOR(BLACK, GREEN) | BRIGHT));
        printf("--[Finish]");
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        printf(" press any key to continue  ");

        // OK, 这个函数暂时还没有做好，但是能用先用着
        getch();

        // 显示选项 + 清屏
        _show_options();
    }

    return 0;
}

void diary_init()
{
    _show_options();

    int i = 0;
    for (i = 0; i < A_WEEK; i++)
    {
        Diary.term_arr[i].date[0] = 0;
        Diary.term_arr[i].score[0] = 0;
        Diary.term_arr[i].string[0] = 0;
    }
    Diary.count = 0;

    int _fd = -1;
    _fd = open("diary.txt", O_CREAT | O_RDWT);
    if (_fd == -1)
    {
        printf("--open diary.txt faild!\n");
        return;
    }
    else
        printf("--begin load..\n");
    struct stat _stat;
    stat("diary.txt", &_stat);
    if (_stat.st_size == 0) // 文件中没有数据，在加载之前文件不存在的情况下会发生
    {
        printf("--file is new..\n");
        return;
    }
    char _buf[20 + 5 + 80 * 25]; // 一条记录的大小
    char *_iterator = NULL;
    for (i = 0; i < A_WEEK; i++)
    {
        memset(_buf, 0, sizeof(_buf));
        read(_fd, _buf, sizeof(_buf)); // 从磁盘中读出一条消息
        if (_buf[0] == 0)              // 结束了
            break;
        _iterator = _buf;
        strcpy(Diary.term_arr[i].date, _iterator);
        _iterator += 20;
        strcpy(Diary.term_arr[i].score, _iterator);
        _iterator += 5;
        strcpy(Diary.term_arr[i].string, _iterator);
        Diary.count++;
    }
    printf("--load the diary.txt successful!\n");
}

void diary_add()
{
    if (Diary.count == A_WEEK)
        printf("--Dirary full.\n");
    else
    {
        char _score[5];
        char _date[20];
        char _string[80 * 25];
        int _cnt = 0;
        set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
        printf("date    ");
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        _cnt = scanf("%s", _date);
        _date[_cnt] = 0;
        set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
        printf("score   ");
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        _cnt = scanf("%s", _score);
        _score[_cnt] = 0;
        set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
        printf("string  ");
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        _cnt = scanf("%s", _string);
        _string[_cnt] = 0;
        strcpy(Diary.term_arr[Diary.count].score, _score);
        strcpy(Diary.term_arr[Diary.count].date, _date);
        strcpy(Diary.term_arr[Diary.count].string, _string);
        printf("--Sucessful--\n");

        printf("\n");
        set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
        printf("[%s]\n", Diary.term_arr[Diary.count].date);
        set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
        printf("%s\n", Diary.term_arr[Diary.count].score);
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        printf("%s\n", Diary.term_arr[Diary.count].string);
        printf("\n");

        Diary.count++;
    }
    return;
}

struct _term *_find(char *_date)
{
    int i = 0;
    for (i = 0; i < Diary.count; i++)
        if (!strcmp(Diary.term_arr[i].date, _date))
            return &Diary.term_arr[i];
    return NULL;
}

void _show_options()
{
    // 清屏
    clear(1);

    // 提示信息
    set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
    printf("\n--1, add.\n");
    printf("--2, modify.\n");
    printf("--3, delete.\n");
    printf("--4, browse.\n");
    printf("--5, find.\n");
    printf("--6, exit.\n\n");
    set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
}

void diary_modify()
{
    int _choice;
    char _date[20];
    char _score[5];
    char _string[80 * 25];
    int _cnt = 0;

    set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
    printf("[date] you want modify  ");
    set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
    _cnt = scanf("%s", _date);
    _date[_cnt] = 0;
    struct _term *_pModi = _find(_date);
    if (_pModi == NULL)
    {
        printf("not find [%s]", _date);
        return;
    }
    else
    {
        set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
        printf("\n[before modify]\n");
        set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
        printf("[%s]\n", _pModi->date);
        set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
        printf("%s\n", _pModi->score);
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        printf("%s\n", _pModi->string);
        printf("\n");
    }
    set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
    printf("[options]\n");
    set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
    printf("1, date.\n");
    printf("2, socre.\n");
    printf("3, string.\n");
    set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
    printf("your choice  ");
    set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
    scanf("%d", &_choice);
    switch (_choice)
    {
    case 1:
        set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
        printf("new date  ");
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        _cnt = scanf("%s", _date);
        _date[_cnt] = 0;
        strcpy(_pModi->date, _date);
        break;
    case 2:
        set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
        printf("new score  ");
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        _cnt = scanf("%s", _score);
        _score[_cnt] = 0;
        strcpy(_pModi->score, _score);
        break;
    case 3:
        set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
        printf("new string  ");
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        _cnt = scanf("%s", _string);
        _string[_cnt] = 0;
        strcpy(_pModi->string, _string);
        break;
    default:
        printf("--do nothing!\n");
        return;
    }

    printf("\n[modifed]\n");
    set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
    printf("[%s]\n", _pModi->date);
    set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
    printf("%s\n", _pModi->score);
    set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
    printf("%s\n", _pModi->string);
    printf("\n");
}

void diary_delete()
{
    printf("NO, you can't delete your foot .\n");
}

void diary_browse()
{
    if (Diary.count == 0)
        printf("--Diary empty.\n");
    else
    {
        int i = 0;
        for (i = 0; i < Diary.count; i++)
        {
            printf("\n");
            set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
            printf("%s\n", Diary.term_arr[i].date);
            set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
            printf("%s\n", Diary.term_arr[i].score);
            set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
            printf("%s\n", Diary.term_arr[i].string);
        }
        printf("\n");
    }
    return;
}

void diary_find()
{
    int _cnt = 0;
    char _date[20];
    set_color((u8)(MAKE_COLOR(BLACK, BLUE) | BRIGHT));
    printf("[date] you want find  ");
    set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
    _cnt = scanf("%s", _date);
    _date[_cnt] = 0;
    struct _term *_pModi = _find(_date);
    if (_pModi == NULL)
    {
        printf("not find [%s]", _date);
        return;
    }
    else
    {
        printf("\n");
        set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
        printf("[%s]\n", _pModi->date);
        set_color((u8)(MAKE_COLOR(BLACK, RED) | BRIGHT));
        printf("%s\n", _pModi->score);
        set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
        printf("%s\n", _pModi->string);
        printf("\n");
    }
}

void diary_exit()
{
    if (Diary.count == 0)
    {
        printf("--Diary empty.\n");
        return;
    }
    else
    {
        int i = 0;
        int _fd = -1;
        _fd = open("diary.txt", O_CREAT | O_RDWT);
        assert(_fd != -1);
        clear(_fd);
        while (i != Diary.count)
        {
            write(_fd, Diary.term_arr[i].date, 20);
            write(_fd, Diary.term_arr[i].score, 5);
            write(_fd, Diary.term_arr[i].string, 80 * 25);
            i++;
        }
        close(_fd);
        printf("--save sucessful!\n");
    }
}
