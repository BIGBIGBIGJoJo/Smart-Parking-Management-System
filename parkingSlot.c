#include "parkingSlot.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool containsEssentialsForReplacement(ParkingSlot *instance, int essentials, int targetTimeSlots[][2], int targetTimeSlotsCount)
{
    int i;
    int bitMask = 0;
    // if there are not enough essentials, check if the target booking(booking2) occupied enough essentials for booking1
    for (i = 0; i < targetTimeSlotsCount; i++)
    {
        if (instance->bookedTimeslot[targetTimeSlots[i][0]][targetTimeSlots[i][1]] != NULL)
            bitMask |= instance->bookedTimeslot[targetTimeSlots[i][0]][targetTimeSlots[i][1]]->data->essentials;
    }

    return (bitMask & essentials) == essentials;
}

bool comparePriority(enum Algorithm algorithm, const Booking *booking, ProcessedBooking *overlappedBookings[], int overlappedBookingsCount)
{
    // booking2 is the booking that is targeted to be overwritten
    switch (algorithm)
    {
    case FCFS:
        return false;
    case PRIORITY:
    {
        int i;
        for (i = 0; i < overlappedBookingsCount; i++)
        {
            if (overlappedBookings[i]->data->type >= booking->type)
                return false;
        }
        return true;
    }
    case OPTIMIZED:
    {
        int i;
        int cumulativePriority = 0;
        for (i = 0; i < overlappedBookingsCount; i++)
        {
            cumulativePriority += overlappedBookings[i]->data->optimizedPriority;
        }
        return booking->optimizedPriority > cumulativePriority;
    }
    default:
        return false;
    }
    return true;
}

// return 1 if greater than all overlapped, 0 if equal to all, -1 if less than any one
int getLeastOverwrittingPriority(ParkingSlot *instance, enum Algorithm algorithm, const Timestamp *startTs, const Timestamp *endTs)
{
    int overlappedBookingsCount = 0;
    int overlappedBookingsId[10];

    int targetTimeslots[TIME_SLOT_DAYS * TIME_SLOT_HOURS][2];
    int targetTimeslotsCount = getTargetTimeslots(startTs, endTs, targetTimeslots);

    int leastPriorityRequired = 0;

    int i;
    for (i = 0; i < targetTimeslotsCount; i++)
    {
        ProcessedBooking *overlappedBooking = instance->bookedTimeslot[targetTimeslots[i][0]][targetTimeslots[i][1]];
        if (overlappedBooking != NULL)
        {
            int overlappedBookingPriority = 0;
            if (algorithm == PRIORITY)
                overlappedBookingPriority = overlappedBooking->data->type;
            else if (algorithm == OPTIMIZED) // note: for optimized, priority is cumulative
                overlappedBookingPriority = leastPriorityRequired + overlappedBooking->data->optimizedPriority;

            leastPriorityRequired = leastPriorityRequired >= overlappedBookingPriority ? leastPriorityRequired : overlappedBookingPriority;
        }
    }
    return leastPriorityRequired;
}

ParkingSlot *initializeParkingSlotBookings(ParkingSlot *ptr)
{
    if (ptr == NULL)
    {
        ParkingSlot *temp = malloc(sizeof(ParkingSlot));
        ptr = temp;
    }
    int i, j;
    for (i = 0; i < TIME_SLOT_DAYS; i++)
    {
        // memset(ptr->bookedTimeslot[i], 0, sizeof(ProcessedBooking*) * TIME_SLOT_HOURS);
        // memset(ptr->essentials[i], 0, sizeof(ProcessedBooking*) * TIME_SLOT_HOURS);
        for (j = 0; j < TIME_SLOT_HOURS; j++)
        {
            ptr->bookedTimeslot[i][j] = NULL;
            ptr->essentials[i][j] = 1 + 2 + 4 + 8 + 16 + 32;
        }
    }

    return ptr;
}

void unMarkParkingSlot(ParkingSlot *instance, ProcessedBooking *unMarkedBooking)
{
    unMarkedBooking->status = REJECTED;
    unMarkedBooking->parkingSlotId = -1;
    int targetTimeSlots[20][2];
    int targetTimeSlotsCount = getTargetTimeslots(unMarkedBooking->data->startTs, unMarkedBooking->data->endTs, targetTimeSlots);
    int i;
    int j;
    for (i = 0; i < targetTimeSlotsCount; i++)
    {
        // release their occupied essentials
        instance->essentials[targetTimeSlots[i][0]][targetTimeSlots[i][1]] |= unMarkedBooking->data->essentials;
    }

    // reject previously accepted bookings which have lower priority
    for (i = 0; i < TIME_SLOT_DAYS; i++)
    {
        for (j = 0; j < TIME_SLOT_HOURS; j++)
        {
            if (instance->bookedTimeslot[i][j] == unMarkedBooking)
                instance->bookedTimeslot[i][j] = NULL;
        }
    }
}

int getTargetTimeslots(const Timestamp *startTs, const Timestamp *endTs, int targetTimeSlots[][2])
{
    // refer to the requirement:
    // "We test the booking schedule for a period, from 10 to 16 May, 2025."
    int startDay = atoi(startTs->date->day);
    int endDay = atoi(endTs->date->day);

    int startHour = atoi(startTs->time->hour);

    int daysSpanned = 0;
    int hoursSpanned = getTimeSlotSpanned(startTs, endTs);

    int i;
    int currHour = startHour;

    int targetTimeslotsCount = 0;

    for (i = 0; i < hoursSpanned; i++)
    {
        if (currHour >= 24)
        {
            currHour = currHour % 24;
            daysSpanned++;
        }
        targetTimeSlots[targetTimeslotsCount][0] = (startDay + daysSpanned) % TIME_SLOT_DAYS;
        targetTimeSlots[targetTimeslotsCount++][1] = (currHour) % TIME_SLOT_HOURS;
        currHour++;
    }
    return targetTimeslotsCount;
}

bool checkEssentials(ParkingSlot *instance, int essentials, int targetTimeSlots[][2], int targetTimeSlotsCount)
{
    int i;
    for (i = 0; i < targetTimeSlotsCount; i++)
    {
        if (((instance->essentials[targetTimeSlots[i][0]][targetTimeSlots[i][1]]) & essentials) != essentials)
        {
            return false;
        }
    }
    return true;
}

bool retrieveEssentials(ParkingSlot *instance, int essentials, int targetTimeSlots[][2], int targetTimeSlotsCount)
{
    if (checkEssentials(instance, essentials, targetTimeSlots, targetTimeSlotsCount) == false)
        return false;
    int i;
    for (i = 0; i < targetTimeSlotsCount; i++)
    {
        instance->essentials[targetTimeSlots[i][0]][targetTimeSlots[i][1]] &= (~essentials);
    }
    return true;
    // printf("z: %d\n", instance->essentials[targetTimeSlots[i][0]][targetTimeSlots[i][1]]);
}

bool hasOverlappedTimeSlots(ParkingSlot *instance, ProcessedBooking *booking)
{
    int targetTimeslots[TIME_SLOT_DAYS * TIME_SLOT_HOURS][2];
    int targetTimeslotsCount = getTargetTimeslots(booking->data->startTs, booking->data->endTs, targetTimeslots);

    int i;
    for (i = 0; i < targetTimeslotsCount; i++)
    {
        ProcessedBooking *currTimeSlotBooking = instance->bookedTimeslot[targetTimeslots[i][0]][targetTimeslots[i][1]];
        if (currTimeSlotBooking != NULL)
            return true;
    }
    return false;
}

bool bookParkingSlot(ParkingSlot *instance, ProcessedBooking *booking, enum Algorithm algorithm)
{
    int targetTimeslots[TIME_SLOT_DAYS * TIME_SLOT_HOURS][2];
    int targetTimeslotsCount = getTargetTimeslots(booking->data->startTs, booking->data->endTs, targetTimeslots);

    ProcessedBooking *overlappedBookings[12];
    int overlappedBookingsCount = 0;
    int i;
    for (i = 0; i < targetTimeslotsCount; i++)
    {
        // printf("day: %d\n", targetTimeslots[i][0]);
        // printf("hour: %d\n", targetTimeslots[i][1]);
        int day = targetTimeslots[i][0] % TIME_SLOT_DAYS;
        int hour = targetTimeslots[i][1] % TIME_SLOT_HOURS;
        ProcessedBooking *currTimeSlotBooking = instance->bookedTimeslot[day][hour];
        if (currTimeSlotBooking == NULL)
            continue;
        else if (algorithm == FCFS)
            return false;
        // simply add this booking to overlappedBookings list if it is not inside the list yet
        int j;
        bool found = 0;
        for (j = 0; j < overlappedBookingsCount; j++)
        {
            if (overlappedBookings[j] == currTimeSlotBooking)
            {
                found = 1;
                break;
            }
        }
        if (found == 0)
            overlappedBookings[overlappedBookingsCount++] = currTimeSlotBooking;
        }
    
    if (overlappedBookingsCount > 0)
    {
        if (comparePriority(algorithm, booking->data, overlappedBookings, overlappedBookingsCount) == false)
            return 0;
    }

    if (checkEssentials(instance, booking->data->essentials, targetTimeslots, targetTimeslotsCount) == false)
    {
        if (containsEssentialsForReplacement(instance, booking->data->essentials, targetTimeslots, targetTimeslotsCount) == false)
            return 0;
    }

    // printf("overlappedBookingsCount: %d\n", overlappedBookingsCount);

    
    for (i = 0; i < overlappedBookingsCount; i++)
    {
        // printf("unmarking\n");
        unMarkParkingSlot(instance, overlappedBookings[i]);
    }
    // mark this booking on the parking slot timeslots
    for (i = 0; i < targetTimeslotsCount; i++)
    {
        int day = targetTimeslots[i][0];
        int hour = targetTimeslots[i][1];
        instance->bookedTimeslot[day][hour] = booking;
    }
    retrieveEssentials(instance, booking->data->essentials, targetTimeslots, targetTimeslotsCount);
    // Note: in case algorithm=fcfs, overlappedBookingsCount won't be greater than 0
    

    return true;
}