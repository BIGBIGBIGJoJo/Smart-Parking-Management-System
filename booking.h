// booking.h
#include "datetime.h"
#include <stdbool.h>
#ifndef BOOKING_H
#define BOOKING_H
#define MAX_BOOKINGS_COUNT 1024
#define ESSENTIALS_COUNT 3
#define ESSENTIALS_DEVICE_COUNT 6

typedef enum Algorithm
{
    FCFS,
    PRIORITY,
    OPTIMIZED,
    SUMMARY_REPORT,
    UNKNOWN_ALGORITHM,
} Algorithm;

typedef enum BookingStatus {
    PENDING,
    ACCEPTED,
    REJECTED,
    DELETED,
    FULFILED,
} BookingStatus;

typedef enum EssentialsType
{
    BATTERY,
    CABLES,
    LOCKER,
    UMBRELLA,
    INFLATION_SERVICE,
    VALET_PARK,
    NONE_ESSENTIALS
} EssentialsType;

typedef enum BookingType
{
    ESSENTIALS,
    PARKING,
    RESERVATION,
    EVENT,
} BookingType;



typedef struct Booking
{
    int bookingId; // equivalent to index in the current implementation, due to no booking deletion
    int memberId; // id of the member who initiated this booking
    int optimizedPriority; // priority value for 
    Timestamp *startTs; // starting timestamp
    Timestamp *endTs; // ending timestamp
    BookingType type; // also stands for the priority
    int essentials; // bit array
} Booking;

typedef struct ProcessedBooking
{
    const Booking* data;
    int parkingSlotId;
    BookingStatus status;
} ProcessedBooking;

int getTimeSlotSpanned(const Timestamp* startTs, const Timestamp* endTs);
void freeBookingMemory(const Booking *booking);
const Booking *createBooking(int bookingId, int memberId, Timestamp *ts, enum BookingType type, char *duration, enum EssentialsType essentials[], int essentialsCount);

bool checkRemainingEssentials(int availableEssentials, const Booking* currBooking);
const char *getBookingTypeString(enum BookingType x);
const char *getEssentialsTypesString(enum EssentialsType x);
enum EssentialsType getEssentialsTypeEnum(char* essentialsTypeString);

#endif // BOOKING_H
