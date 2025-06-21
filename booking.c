#include "booking.h"
#include "utils.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Note, the order is IMPORTANT, specifying their priorities, where smaller index = greater priority

const char *getBookingTypeString(enum BookingType x)
{
    switch (x)
    {
    case EVENT:
        return "Event";
    case RESERVATION:
        return "Reservation";
    case PARKING:
        return "Parking";
    case ESSENTIALS:
        return "Essentials";
    default:
        return NULL;
    }
}

int getEssentialsListBitArray(const enum EssentialsType essentials[])
{
    int temp[ESSENTIALS_DEVICE_COUNT];
    int i;
    for (i = 0; i < ESSENTIALS_DEVICE_COUNT; i++)
    {
        temp[i] = 0;
    }
    for (i = 0; i < ESSENTIALS_DEVICE_COUNT; i++)
    {
        if (essentials[i] != NONE_ESSENTIALS)
        {
            temp[essentials[i]] = 1;
        }
    }
    int result = getBitArray(temp, ESSENTIALS_DEVICE_COUNT);
    return result;
}

int pairEssentials(int essentialsBitArray)
{
    int original = essentialsBitArray;
    int pairMask1 = ((1 << BATTERY) | (1 << CABLES));
    if ((pairMask1 & original )> 0)
        essentialsBitArray |= pairMask1;

    int pairMask2 = ((1 << LOCKER) | (1 << UMBRELLA));
    if ((pairMask2 & original) > 0)
        essentialsBitArray |= pairMask2;

    int pairMask3 = ((1 << INFLATION_SERVICE) | (1 << VALET_PARK));
    if ((pairMask3 & original) > 0)
        essentialsBitArray |= pairMask3;
    // printf("result: %d\n", essentialsBitArray);
    return essentialsBitArray;
}

bool checkRemainingEssentials(int availableEssentials, const Booking* currBooking)
{
    int i;
    int essentialsBitMask = currBooking->essentials;
    
    return (essentialsBitMask == 0) || ((essentialsBitMask & availableEssentials) > 0);
}

const char *getEssentialsTypesString(enum EssentialsType x)
{
    switch (x)
    {
    case BATTERY:
        return "battery";
    case CABLES:
        return "cables";
    case LOCKER:
        return "locker";
    case UMBRELLA:
        return "umbrella";
    case INFLATION_SERVICE:
        return "inflationService";
    case VALET_PARK:
        return "valetParking";
    case NONE_ESSENTIALS:
    default:
        return NULL;
    }
}

enum EssentialsType getEssentialsTypeEnum(char* essentialsTypeString) {
    int i;
    enum EssentialsType returnEnum = NONE_ESSENTIALS;
    char* inputString = strdup(essentialsTypeString);
    toLower(inputString, strlen(inputString));
    for (i = 0; i < 6; i++) {
        char* currString = strdup(getEssentialsTypesString(i));
        toLower(currString, strlen(currString));
        if (strstr(currString, inputString) != NULL) {
            returnEnum = (enum EssentialsType) i;
            free(currString);
            break;
        }
        free(currString);
    }
    free(inputString);
    return returnEnum;
}

void freeBookingMemory(const Booking *booking)
{
    freeTimestamp(booking->startTs);
    free(booking->startTs);
    freeTimestamp(booking->endTs);
    free(booking->endTs);
    free((char *)booking);
}

int getTimeSlotSpanned(const Timestamp* startTs, const Timestamp* endTs)
{
    int startHour = atoi(startTs->time->hour);
    int endHour = atoi(endTs->time->hour);
    if (atoi(endTs->time->minute) > 0)
    {
        endHour += 1;
    }

    int hoursSpanned;
    if (startHour > endHour)
    {
        // the booking spanned a day, (e.g. from 23:00 to 02:30)
        hoursSpanned = (endHour - startHour + 24);
    }
    else
    {
        hoursSpanned = endHour - startHour;
    }
    return hoursSpanned;
}

bool containsEssentialsEnum(const enum EssentialsType essentials[], int size, enum EssentialsType target)
{
    if (size == 0) return false;
    int temp[size];
    int x;
    for (x = 0; x < size; x++) temp[x] = (int) essentials[x];
    return contains(temp, size, (int)target);
}

const Booking *createBooking(int bookingId, int memberId, Timestamp *ts, enum BookingType type, char *duration, enum EssentialsType essentials[], int essentialsCount)
{
    Booking *b = malloc(sizeof(Booking));

    if (getBookingTypeString(type) == NULL)
    {
        printf("Invalid booking type encountered during booking creation, program terminating");
        exit(1);
    }
    
    b->essentials = pairEssentials(getEssentialsListBitArray(essentials));
    // printf("memberId: %d\n", memberId);
    // printf("b->essentiasl: %d\n", b->essentials);

    b->memberId = memberId;
    b->bookingId = bookingId;
    b->startTs = ts;
    b->endTs = calculateTimestamp(ts, duration);
    b->type = type;
    b->optimizedPriority = type + 10 * (essentialsCount * getTimeSlotSpanned(b->startTs, b->endTs));
    // printf("optimizedPriority: %d\n", b->optimizedPriority);
    // strcpy(b->duration, duration);
    return b;
}

// absolute (out of total timeslot)
// battery: 3 / (7*24)

// 10 * (essentialsCount * getTimeSlotSpanned(b->startTs, b->endTs))
// relative (out of accepted booking timeslot)
// battery: 6/50

// b1: 1hr cable battery
// b2: 1hr cable battery
// b3: 1hr cable battery (priority = 20)

// b4: addEvent 2hr cable (priority = 23)
// b5: addEvent 2hr cable (priority = 23)
// b6: addEvent 2hr cable (priority = 23)
