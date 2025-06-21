#include "booking.h"
#include "utils.h"
#include <stdbool.h>

#ifndef PARKINGSLOT_H
#define PARKINGSLOT_H
#define PARKING_SLOTS_COUNT 3

#define TIME_SLOT_DAYS 30
#define TIME_SLOT_HOURS 24

typedef struct
{
    ProcessedBooking* bookedTimeslot[TIME_SLOT_DAYS][TIME_SLOT_HOURS];
    int essentials[TIME_SLOT_DAYS][TIME_SLOT_HOURS]; // bit array
} ParkingSlot;
// void initializeParkingSlots(ParkingSlot* parkingSlots[], int size);
ParkingSlot *initializeParkingSlotBookings(ParkingSlot *ptr);
bool bookParkingSlot(ParkingSlot *instance, ProcessedBooking *booking, enum Algorithm algorithm);
bool hasOverlappedTimeSlots(ParkingSlot *instance, ProcessedBooking *booking);
void unMarkParkingSlot(ParkingSlot *instance, ProcessedBooking *unMarkedBooking);
int getTargetTimeslots(const Timestamp* startTs, const Timestamp* endTs, int targetTimeSlots[][2]);
// bool isOverlap(const Booking *booking1, const Booking *booking2);

// get the least priority required to overwrite given timeslots in this parking slot
int getLeastOverwrittingPriority(ParkingSlot *instance, enum Algorithm algorithm, const Timestamp* startTs, const Timestamp* endTs);

#endif