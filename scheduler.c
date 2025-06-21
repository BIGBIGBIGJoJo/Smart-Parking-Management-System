#include "scheduler.h"
#include <stdlib.h>
#include <string.h>

const char *getAlgorithmName(enum Algorithm x)
{
    switch (x)
    {
    case FCFS:
        return "FCFS";
    case PRIORITY:
        return "Priority";
    case OPTIMIZED:
        return "Optimized";
    case SUMMARY_REPORT:
        return "All";
    case UNKNOWN_ALGORITHM:
    default:
        return NULL;
    }
}

const char *getAlgorithmAbbreviation(enum Algorithm x)
{
    switch (x)
    {
    case FCFS:
        return "FCFS";
    case PRIORITY:
        return "Prio";
    case OPTIMIZED:
        return "opti";
    case SUMMARY_REPORT:
        return "All";
    case UNKNOWN_ALGORITHM:
    default:
        return NULL;
    }
}

enum Algorithm getAlgorithmEnum(char *x)
{
    char *inputString = strdup(x);
    toLower(inputString, strlen(inputString));
    int i;
    for (i = 0; i < ALGORITHM_COUNT + 1; i++) // ALGORITHM_COUNT+1 for -ALL option
    {
        char *temp = strdup(getAlgorithmAbbreviation(i));
        toLower(temp, strlen(temp));
        // printf("temp: %s\n", temp);
        // printf("inputString: %s\n", inputString);
        bool equal = strcmp(inputString, temp) == 0;
        free(temp);
        if (equal)
        {
            break;
        }
    }
    free(inputString);
    return (enum Algorithm)i;
}

Scheduler *schedulerConstructor(Algorithm algorithm)
{
    Scheduler *instance = malloc(sizeof(Scheduler));
    if (instance == NULL) {
        perror("Failed to allocate Scheduler instance");
        return NULL;
    }
    instance->algorithm = algorithm;
    instance->processingIndex = 0;
    instance->bookingsCount = 0;
    
    int i, j;
    for (i = 0; i < MAX_BOOKINGS_COUNT; i++)
    {
        instance->bookings[i] = malloc(sizeof(ProcessedBooking));
        if (instance->bookings[i] == NULL)
        {
            perror("error during memory allocation for processed bookings list");
            for (j = 0; j < i; j++) {
                free(instance->bookings[j]);
            }
            free(instance);
            return NULL;
        }
    }
    for (i = 0; i < PARKING_SLOTS_COUNT; i++)
    {
        instance->parkingSlots[i] = malloc(sizeof(ParkingSlot));
        if (instance->parkingSlots[i] == NULL)
        {
            perror("error during memory allocation for parkingSlots list");
            for (j = 0; j < MAX_BOOKINGS_COUNT; j++) {
                free(instance->bookings[j]);
            }
            for (j = 0; j < i; j++) {
                free(instance->parkingSlots[j]);
            }
            free(instance);
            return NULL;
        }
        initializeParkingSlotBookings(instance->parkingSlots[i]);
    }

    return instance;
}

void acceptBooking(Scheduler *instance, ProcessedBooking *booking, int parkingSlotId)
{
    booking->parkingSlotId = parkingSlotId;
    booking->status = ACCEPTED;
}

void addBooking(Scheduler *instance, const Booking *booking)
{
    instance->bookings[instance->bookingsCount]->data = booking;
    instance->bookings[instance->bookingsCount]->parkingSlotId = -1;
    instance->bookings[instance->bookingsCount]->status = PENDING;

    instance->bookingsCount++;
}

void freeSchedulerMemory(Scheduler *instance)
{
    int i;
    for (i = 0; i < PARKING_SLOTS_COUNT; i++)
    {
        free(instance->parkingSlots[i]);
    }
    for (i = 0; i < MAX_BOOKINGS_COUNT; i++)
    {
        free(instance->bookings[i]);
    }
    free(instance);
}