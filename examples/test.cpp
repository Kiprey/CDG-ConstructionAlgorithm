#include <iostream>
using namespace std;

void guess(int num)
{
    for(;;)
    {
        int n;
        cout << "Please input your guess num: ";
        cin >> n;

        if(n > num)
            cout << "The guess number is bigger than my number!\n";
        else if(n < num)
            cout << "The guess number is smaller than my number!\n";
        else
        {
            cout << "Guess correct!\n";
            break;
        }
    }
}

int main()
{
    cout << "Now try to guess my number !\n";
    int num = rand() % 100;

    guess(num);

    return 0;
}