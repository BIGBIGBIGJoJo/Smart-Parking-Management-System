#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

int getBitArray(int arr[], int size)
{
    // {1, 1, 0, 0, 0, 0} -> 000011
    int i;
    int result = 0;
    for (i = 0; i < size; i++)
    {
        if (arr[i] == 1)
        {
            result = result | (1 << i);
        }
    }
    return result;
}

int getBit(int bitArray, int index)
{
    return (bitArray >> index) & 1;
}

void toLower(char *string, int length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        string[i] = tolower(string[i]);
    }
}

int contains(int arr[], int size, int value)
{
    int i;
    for (i = 0; i < size; i++)
    {
        if (arr[i] == value)
        {
            return 1;
        }
    }
    return 0;
}

void combineTerminatedString(const char *segments, const int bufferLength, char *result, int resultBufferLength)
{
    // what this function does:
    // combine "abcsada\0abadadda\0aaaaa\0" into "abcsadaabadaddaaaaaa\0"
    // printf("received bufferLength: %d\n", bufferLength);
    if (resultBufferLength <= 0)
    {
        return;
    }
    int segmentLength = 0;
    int segmentOffset = 0;
    int resultIndex = 0;
    while (segmentOffset < bufferLength && (segmentLength = strlen(segments + segmentOffset)) > 0)
    {
        if (resultIndex + segmentLength > resultBufferLength)
        {
            break;
        }
        strcpy(result + resultIndex, segments + segmentOffset);
        resultIndex += segmentLength;
        segmentOffset += segmentLength + 1;
    }
    result[resultIndex] = '\0';
    // printf("result in combineTerminatedString:%s\n", result);
}

char *stripString(const char *inputString, char *result)
{
    if (inputString == NULL)
    {
        return NULL;
    }
    char *str = strdup(inputString);
    if (str == NULL)
    {
        return NULL;
    }

    while (isspace((unsigned char)*str) || ((unsigned char)*str) == '\n')
    {
        str++;
    }

    char *end = str + strlen(str) - 1;
    while (end > str && (isspace((unsigned char)*end) || ((unsigned char)*str) == '\n'))
    {
        end--;
    }

    *(end + 1) = '\0';
    // printf("str is %s\n", str);
    // printf("inputString is %s\n", inputString);
    if (result != NULL)
    {
        strcpy(result, str);
    }

    return str;
}

int splitString(const char *inputString, char delimiter, char *result[], int maxSize)
{
    // printf("inputString: %s\n", inputString);
    int i = 0;
    char *inputCopy = strdup(inputString);
    if (inputCopy == NULL)
    {
        return 0;
    }

    char *token = strtok(inputCopy, &delimiter);
    while (token != NULL && i < maxSize)
    {
        result[i] = strdup(token);
        if (result[i] == NULL)
        {
            free(inputCopy);
            return 0;
        }
        i++;
        token = strtok(NULL, &delimiter);
    }

    free(inputCopy);
    return i;
}

void closePipe(int p[2])
{
    close(p[0]);
    close(p[1]);
}
