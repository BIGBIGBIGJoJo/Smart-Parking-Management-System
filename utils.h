#ifndef UTILS_H
#define UTILS_H

char *stripString(const char *inputString, char *result);
void toLower(char* string, int length);
// void addUniqueElements(int source[], int sourceSize, int dest[], int *destSize);
int contains(int arr[], int size, int value);
void combineTerminatedString(const char *segments, const int bufferLength, char *result, int resultBufferLength);
int splitString(const char *inputString, char delimiter, char *result[], int maxSize);
void closePipe(int pipeEntries[2]);
int getBitArray(int arr[], int size);
int getBit(int bitArray, int index);

#endif // UTILS_H