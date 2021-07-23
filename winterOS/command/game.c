#include "stdio.h"
#include "keyboard.h"

int main(int argc, char *argv[])
{
    clear(1);
    while (TRUE)
    {
        int _char = getch();
        switch (_char)
        {
        case LEFT:
            printf("LEFT ... \n");
            break;
        case RIGHT:
            printf("RIGHT ... \n");
            break;
        case UP:
            printf("UP ... \n");
            break;
        case DOWN:
            printf("DOWN ... \n");
            break;
        case ESC:
            printf("[Game exit]\n");
            return 0;
        default:
            break;
        }
    }
    return 0;
}
