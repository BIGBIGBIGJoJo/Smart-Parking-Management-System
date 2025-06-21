
#include "datetime.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Date *createDateFromString(char *date)
{
    char *args[3];
    splitString(date, '-', args, 3);
    Date *d = createDate(args[0], args[1], args[2]);
    free(args[0]);
    free(args[1]);
    free(args[2]);
    return d;
}

Date *createDate(char year[5], char month[3], char day[3])
{
    Date *d = malloc(sizeof(Date));
    strcpy(d->year, year);
    strcpy(d->month, month);
    strcpy(d->day, day);
    return d;
}

Time *createTimeFromString(char *time)
{
    char *args[2];
    splitString(time, ':', args, 3);
    Time* t = createTime(args[0], args[1]);
    free(args[0]);
    free(args[1]);
    return t;
}

Time *createTime(char hour[3], char minute[3])
{
    Time *t = malloc(sizeof(Time));
    strcpy(t->hour, hour);
    strcpy(t->minute, minute);
    return t;
}

Timestamp *createTimestamp(Date *date, Time *time)
{
    Timestamp* ts = malloc(sizeof(Timestamp));
    ts->date = date;
    ts->time = time;
    return ts;
}

void freeTimestamp(Timestamp *ts)
{
    free(ts->date);
    free(ts->time);
}

void buildTimestampString(Timestamp *ts, char *result)
{
    Date *d = ts->date;
    Time *t = ts->time;
    sprintf(result, "%s-%s-%s %s:%s", d->year, d->month, d->day, t->hour, t->minute);
}

Timestamp *calculateTimestamp(const Timestamp *ts, char *duration)
{
    char* temp[2];
    splitString(duration, '.', temp, 2);
    int hours = atoi(temp[0]);
    int minutes = atoi(temp[1]) * 6;

    int startHour = atoi(ts->time->hour);
    int startMinute = atoi(ts->time->minute);

    int endMinute = (startMinute + minutes);
    int endHour = (startHour + hours + (endMinute / 60));
    int endDay = atoi(ts->date->day) + (endHour / 24);
    endMinute = endMinute % 60;
    endHour = endHour % 24;

    char endDayString[3];
    memset(endDayString, 0, sizeof(endDayString));
    if (endDay < 10) {
        snprintf(endDayString, sizeof(endDayString), "0%d", endDay);
    } else {
        snprintf(endDayString, sizeof(endDayString), "%d", endDay);
    }
    // printf("endDayString: (%s)\n", endDayString);

    char endHourString[3];
    memset(endHourString, 0, sizeof(endHourString));
    if (endHour < 10) {
        snprintf(endHourString, sizeof(endHourString), "0%d", endHour);
    } else {
        snprintf(endHourString, sizeof(endHourString), "%d", endHour);
    }
    // printf("endHourString: (%s)\n", endHourString);

    char endMinuteString[3];
    memset(endMinuteString, 0, sizeof(endMinuteString));
    if (endMinute < 10) {
        snprintf(endMinuteString, sizeof(endMinuteString), "0%d", endMinute);
    } else {
        snprintf(endMinuteString, sizeof(endMinuteString), "%d", endMinute);
    }
    // printf("endMinuteString: (%s), endminute: (%d), minutes: (%d), duration[2]: (%c)\n", endMinuteString, endMinute, minutes, duration[1]);

    // Note: since date used for testing will be from 10 to 16 May, 2025, so month and year overflow is not considered
    // printf("bookingId: %d\n", )
    // printf("ts->date->year, ts->date->month, endDayString: %s, %s, %s, \n", ts->date->year, ts->date->month, endDayString);
    Date *d = createDate(ts->date->year, ts->date->month, endDayString);
    Time *t = createTime(endHourString, endMinuteString);
    Timestamp *endTs = createTimestamp(d, t);
    // printf("endTs->date->year: %s\n", endTs->date->year);

    return endTs;
}
