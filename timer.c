#include "raylib.h"
#include "timer.h"
#include <time.h>

//timer stuff
Timer CreateTimer(float dur)
{
    return (Timer){dur,false,0,true};
}
void StartTimer(Timer *t)
{
    t->virgin = false;
    t->wasStarted = true;
    t->start_time = time(NULL); //initial set of start time
}
bool HasTimerElapsed(Timer *t, time_t cur)
{
    if(t->virgin){return true;}
    if(!t->wasStarted){return false;}
    time_t elapsed_time = difftime(cur, t->start_time);// Calculate the elapsed time
    return elapsed_time > t->duration;
}
void ResetTimer(Timer *t)
{
    t->wasStarted =false;
}