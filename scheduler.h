#include <stdio.h>
#include "booking.h"
#include "utils.h"
#include "parkingSlot.h"
#ifndef SCHEDULER_H
#define SCHEDULER_H

#define ALGORITHM_COUNT 3

typedef struct Scheduler
{
    int processingIndex; // the index of the last booking that this scheduler has scheduled 
    int bookingsCount; // how many bookings in total
    Algorithm algorithm; // what algorithm this scheduler uses
    ProcessedBooking* bookings[MAX_BOOKINGS_COUNT]; // bookings with status (e.g. pending/accepted/rejected) and parking slot id
    ParkingSlot* parkingSlots[PARKING_SLOTS_COUNT]; // parking slots of this scheduler
} Scheduler;

enum Algorithm getAlgorithmEnum(char *x);
const char *getAlgorithmName(enum Algorithm x);
void acceptBooking(Scheduler *instance, ProcessedBooking *booking, int parkingSlotId);
void addBooking(Scheduler *instance, const Booking *booking);
void freeSchedulerMemory(Scheduler* instance);
Scheduler* schedulerConstructor(Algorithm algorithm);
// void initializeScheduler(Scheduler* instance);
int getEssentialsListBitArray(const enum EssentialsType essentials[]);
// int bookParkingSlotWithRetrivedEssentials(Scheduler *instance, int heapIndex, const ProcessedBooking *currBooking, int essentialsBitMask, int cumulativePriority, ProcessedBooking *result[]);

#endif