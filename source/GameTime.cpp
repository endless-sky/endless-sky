// GameTime.cpp

#include "GameTime.h"

int main(int argc, char **argv) {
    int gameStatus;     // if game is running, both these two will be set to 1
    int timeStatus;     // if game is closed, gameStatus set to 0 and timeStatus remain 1

    int hour, minute, second;
    long totalTime;

    time_t begin, end;

    while (true) {
        gameStatus = IsProcessRunning(argv[1]);

        // The game is running now 
        if (gameStatus == 1 && timeStatus == 0) {
            timeStatus = 1;
            localtime(&begin);
        }

        // The game is stopped now
        if (gameStatus == 0 && timeStatus == 1) {
            timeStatus = 0;
            localtime(&end);

            totalTime = end-begin;
            hour = totalTime / 3600;
            minute = (totalTime - hour * 3600) / 60;
            second = (totalTime - hour * 3600 - minute * 60);

            printf ("The total time you spent on this game is: \n");
            printf ("   %d hour(s), %d minute(s), %d second(s)\n", hour, minute, second);
        }

        sleep(1000);    // Occupy less CPU time
    }

    return 0;
}