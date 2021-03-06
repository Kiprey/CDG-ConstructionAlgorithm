#include <stdio.h>
#include <stdlib.h>

void guess(int num)
{
    for (;;)
    {
        int n;
        printf("Please input your guess num: ");
        scanf("%d", &n);

        if (n > num)
            printf("The guess number is bigger than my number!\n");
        else if (n < num)
            printf("The guess number is smaller than my number!\n");
        else
        {
            printf("Guess correct!\n");
            break;
        }
    }
}

int main()
{
    printf("Now try to guess my number !\n");
    int num = rand() % 100;

    guess(num);

    return 0;
}