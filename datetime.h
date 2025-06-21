#include "utils.h"
#ifndef DATETIME_H
#define DATETIME_H

typedef struct
{
    char year[5];
    char month[3];
    char day[3];
} Date;

typedef struct
{
    char hour[3];
    char minute[3];
} Time;

typedef struct
{
    Date *date;
    Time *time;
} Timestamp;



void freeTimestamp(Timestamp *ts);
void buildTimestampString(Timestamp* ts, char* result);
Date *createDateFromString(char *date);
Date *createDate(char year[5], char month[3], char day[3]);
Time *createTimeFromString(char *time);
Time *createTime(char hour[3], char minute[3]);
Timestamp *createTimestamp(Date *date, Time *time);
Timestamp *calculateTimestamp(const Timestamp *ts, char *duration);



#endif // TIMESLOT_H
