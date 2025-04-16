#ifndef TIMER_H
#define TIMER_H

#include <time.h>

typedef struct {
    float duration;
    bool wasStarted;
    time_t start_time;
    bool virgin;
} Timer;

//timer stuff
Timer CreateTimer(float dur);
void StartTimer(Timer *t);
bool HasTimerElapsed(Timer *t, time_t cur);
void ResetTimer(Timer *t);

#endif // FUNCTIONS_H