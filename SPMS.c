// #define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>   // for atoi
#include <string.h>   // for strcpy(), strdup() etc.
#include <stdbool.h>  // for boolean
#include <unistd.h>   //
#include <errno.h>    // for non-blocking pipe read (errno variables etc.)
#include <sys/wait.h> // wait()
#include <sys/resource.h>
#include <time.h>  // for clock()
#include <fcntl.h> // for non-blocking pipe read
#include "booking.h"
#include "datetime.h"
#include "member.h"
#include "parkingSlot.h"
#include "scheduler.h"
#include "utils.h"

#define ADD_PARKING "addParking"
#define ADD_RESERVATION "addReservation"
#define ADD_EVENT "addEvent"
#define BOOK_ESSENTIALS "bookEssentials"
#define ADD_BATCH "addBatch"
#define PRINT_BOOKINGS "printBookings"
#define END_PROGRAM "endProgram"

const char *COMMANDS[7] = {
    BOOK_ESSENTIALS, // 0
    ADD_PARKING,     // 1
    ADD_RESERVATION, // 2
    ADD_EVENT,       // 3
    ADD_BATCH,       // 4
    PRINT_BOOKINGS,  // 5
    END_PROGRAM      // 6
};
const int COMMANDS_COUNT = sizeof(COMMANDS) / sizeof(COMMANDS[0]);

enum BookingType getCommandBookingType(int commandIndex)
{
    switch (commandIndex)
    {
    case 0:
        return ESSENTIALS;
    case 1:
        return PARKING;
    case 2:
        return RESERVATION;
    case 3:
        return EVENT;
    }
}

typedef enum MODULE
{
    INPUT_MODULE,
    SCHEDULER_MODULE,
    ANALYZER_MODULE,
    OUTPUT_MODULE,
} MODULE;

void inputModuleProcess(char *command, int SPMSToParent[], int inputToScheduler[], bool *running, MemberList *memberList, bool debug);

int getCommandIndex(const char *command);

void commandToMessage(char *args[], int commandDeterminant, int argsCount, char *res, int SPMSToParent[], int inputToScheduler[], bool *running, MemberList *memberList, bool debug)
{
    char returnMessage[512];
    memset(returnMessage, 0, sizeof(returnMessage));
    switch (commandDeterminant)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    {
        int memberId = getMemberId(memberList, args[1] + 1);
        // printf("memberId: %d\n", memberId);
        if (memberId < 0)
        {
            printf("member not found!\n");
            return;
        }

        if (argsCount < 5)
        {
            printf("necessary arguments are not provided!\n");
            return;
        }

        sprintf(returnMessage, "%d|%d|%s|%s|%s", commandDeterminant, memberId, args[2], args[3], args[4]);
        if (argsCount > 5)
        {
            strcat(returnMessage, "|");
        }
        else
        {
            if (commandDeterminant != getCommandIndex(ADD_PARKING) && memberId != getMemberId(memberList, "member_E"))
            {
                printf("All booking types except parking has to provide essentials!\n");
                return;
            }
        }
        int i;
        int essentialsList[ESSENTIALS_DEVICE_COUNT];
        memset(essentialsList, 0, sizeof(essentialsList));
        int count = 0;
        for (i = 5; i < argsCount; i++)
        {
            // printf("args[%d]: %s\n", i, args[i]);
            toLower(args[i], strlen(args[i])); // ensure essentials in lower case for checking below (not optimized)
            char temp[3];
            memset(temp, 0, sizeof(temp));
            int essentialsEnum = getEssentialsTypeEnum(args[i]);
            if (essentialsEnum == NONE_ESSENTIALS || contains(essentialsList, count, essentialsEnum)) // invalid essentials type
            {
                return;
            }
            essentialsList[count++] = essentialsEnum;
            sprintf(temp, "%d", essentialsEnum);
            if (i > 5)
            {
                strcat(returnMessage, ",");
            }
            strcat(returnMessage, temp);
        }
        // printf("returnMessage in commandToMessage: %s\n", returnMessage);
        strcpy(res, returnMessage);
        break;
    }

    case 4:
    {
        char fileName[30];
        memset(fileName, 0, sizeof(fileName));
        strcpy(fileName, args[1] + 1);
        FILE *file = fopen(fileName, "r");
        if (file == NULL)
        {
            perror("Error opening file");
            return;
        }
        fseek(file, 0, SEEK_END);
        long filesize = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *buffer = malloc(filesize + 1); // +1 for the null terminator
        if (buffer == NULL)
        {
            perror("Memory allocation failed");
            fclose(file);
            return;
        }

        // Read the entire file into the buffer
        size_t bytesRead = fread(buffer, 1, filesize, file);
        if (bytesRead != filesize)
        {
            perror("Error reading file");
            free(buffer);
            fclose(file);
            return;
        }

        // Null-terminate the buffer to treat it as a string
        buffer[filesize] = '\0';
        char *commands[512];
        memset(commands, 0, sizeof(commands));
        int commandsCount = splitString(buffer, ';', commands, 1024);
        int i;
        for (i = 0; i < commandsCount; i++)
        {
            char temp[70];
            memset(temp, 0, sizeof(temp));
            stripString(commands[i], temp);
            // printf("temp = (%s)\n", temp);
            if (strlen(temp) == 0) {
                continue;
            }
            stripString(commands[i], temp);
            if (debug)
                printf("processing command: %s\n", temp);
            inputModuleProcess(temp, SPMSToParent, inputToScheduler, running, memberList, debug);
            free(commands[i]);
            if ((*running) == false)
            {
                break;
            }
        }
        free(buffer);
        fclose(file);
        break;
    }

    case 5:
    {
        sprintf(returnMessage, "%d", commandDeterminant);

        char algorithm[10] = "";
        // strcpy(algorithm, args[1] + 1);
        // printf("args[1]: (%s)\n", args[1]);
        strncpy(algorithm, (char *)(args[1] + 1), strlen(args[1]) - 1);
        // printf("algorithm: %s\n", algorithm);
        enum Algorithm algorithmEnum = getAlgorithmEnum(algorithm);
        // strcat(returnMessage, algorithm);
        // res = strdup(returnMessage);
        // printf("res in commandToMessage: %s\n", res);
        // printf("returnMessage in commandToMessage: %s\n", returnMessage);
        snprintf(res, 1024, "%s|%d", returnMessage, (int)algorithmEnum);
        break;
    }
    default:
    {
        printf("Invalid command index\n");
        break;
    };
    }
}

int getCommandIndex(const char *command)
{
    if (strlen(command) == 0)
    {
        return -1;
    }
    int i;
    for (i = 0; i < COMMANDS_COUNT; i++)
    {
        if (strcmp(command, COMMANDS[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

void inputModuleProcess(char *command, int SPMSToParent[], int inputToScheduler[], bool *running, MemberList *memberList, bool debug)
{
    char *args[12];
    memset(args, 0, sizeof(args));
    char temp[1024];
    memset(temp, 0, sizeof(temp));

    int argsCount = splitString(command, ' ', args, 12);
    int i;
    for (i = 0; i < argsCount; i++)
    {
        char *temp = strdup(args[i]);
        // memset(args[i], 0, sizeof(args[i]));
        stripString(temp, args[i]);
        free(temp);
        // printf("args[%d]: (%s)\n", i, args[i]);
    }
    char res[1024];
    memset(res, 0, sizeof(res));

    int commandIndex = getCommandIndex(args[0]);
    if (commandIndex < 0)
    {
        printf("Command Invalid!\n");
        return;
    }
    if (commandIndex == getCommandIndex(END_PROGRAM))
    {

        (*running) = false;
        return;
    }

    commandToMessage(args, commandIndex, argsCount, res, SPMSToParent, inputToScheduler, running, memberList, debug);
    for (i = 0; i < argsCount; i++)
    {
        // printf("freed args2[%d]: %s\n", i, args[i]);
        free(args[i]);
    }
    if ((*running) == false)
    {
        return;
    }
    int responseLength = strlen(res);
    if (responseLength == 0)
    {
        // printf("res: %s\n", res);
        if (commandIndex != getCommandIndex(ADD_BATCH)) // if
            printf("command arguments are invalid, command ignored. (ignored command: %s)\n", command);
        return;
    }
    strcat(res, ";");
    write(inputToScheduler[1], res, responseLength + 1);
}

void getQueuedArguments(const int fd, char *argsQueue[], int *queueLength, size_t bufferSize, bool debug)
{
    char *buffer = malloc(bufferSize);
    if (buffer == NULL)
    {
        perror("Failed to allocate buffer");
        return;
    }
    int n = read(fd, buffer, bufferSize);
    if (n <= 0)
    {
        if (n == 0)
        {
            // printf("buffersize = (%d), n = (%d)\n", bufferSize, n);
            (*queueLength) = -2; // -2 = EOF
        }
        else
            (*queueLength) = -1;
        free(buffer);
        return;
    }
    buffer[n - 1] = '\0';
    char *result = malloc(n);
    if (result == NULL)
    {
        perror("Failed to allocate result");
        free(buffer);
        return;
    }
    result[n - 1] = '\0';
    if (debug)
    {
        printf("received buffer before combine (n = %d)\n", n);
    }
    combineTerminatedString(buffer, n, result, n);
    free(buffer);
    (*queueLength) = splitString(result, ';', argsQueue, 100);
    free(result);
}

double schedule(Scheduler *instance)
{
    int pendingBookingsCount = instance->bookingsCount - instance->processingIndex;
    clock_t start = clock();
    for (; instance->processingIndex < instance->bookingsCount; instance->processingIndex++)
    {
        ProcessedBooking *currBooking = instance->bookings[instance->processingIndex];

        currBooking->status = REJECTED;
        currBooking->parkingSlotId = -1;
        int i;
        bool scheduled = false;
        // check if there is unoccupied parking slot, if yes, accept the booking
        for (i = 0; i < PARKING_SLOTS_COUNT; i++)
        {
            if (hasOverlappedTimeSlots(instance->parkingSlots[i], currBooking) == false)
            {
                if (bookParkingSlot(instance->parkingSlots[i], currBooking, instance->algorithm))
                {
                    acceptBooking(instance, currBooking, i);
                    scheduled = true;
                    break;
                }
            }
        }
        if (scheduled == true || instance->algorithm == FCFS)
            continue;

        // try to overwrite last accepted bookings
        for (i = PARKING_SLOTS_COUNT - 1; i >= 0; i--)
        {
            if (bookParkingSlot(instance->parkingSlots[i], currBooking, instance->algorithm))
            {
                acceptBooking(instance, currBooking, i);
                break;
            }
        }
    }

    clock_t end = clock();
    return ((double)(end - start));
}

int main(int argc, char const *argv[])
{
    // signal(SIGPIPE, SIG_IGN);
    bool debug = false;
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-debug") == 0) {
            debug = true;
        }
    }

    const char *members[] = {
        "member_A",
        "member_B",
        "member_C",
        "member_D",
        "member_E",
        // "member_F",
    };
    const int membersCount = sizeof(members) / sizeof(members[0]);
    MemberList *memberList = initializeMembers(members, membersCount);

    const MODULE MODULE_TYPES[] = {
        INPUT_MODULE,
        SCHEDULER_MODULE,
        ANALYZER_MODULE,
        OUTPUT_MODULE};

    int moduleIndex = 0;
    const int MODULE_COUNT = (sizeof(MODULE_TYPES) / sizeof(MODULE_TYPES[0]));
    // char *moduleType = malloc(sizeof(MODULE_TYPES[0])); // this might cause segmentation fault, test later
    enum MODULE moduleType;
    // entry point to the whole app
    int parentToSPMS[2];
    // pipe for SPMS to return values
    int SPMSToParent[2];

    // other pipes
    int inputToScheduler[2];
    int schedulerToAnalyzer[2];
    int analyzerToOutput[2];
    int schedulerToOutput[2];
    int outputToScheduler[2];
    int analyzerToScheduler[2];

    if ((pipe(analyzerToScheduler)) || (pipe(outputToScheduler)) || (pipe(parentToSPMS) || pipe(SPMSToParent) || pipe(inputToScheduler) || pipe(schedulerToAnalyzer) || pipe(analyzerToOutput) || pipe(schedulerToOutput)) == 1)
    {
        perror("error while creating pipes");
        return 1;
    }
    // int pipeSize = 1 * 1024 * 1024; // 1 MB

    // if (fcntl(schedulerToOutput[0], F_SETPIPE_SZ, pipeSize) == -1)
    //     exit(EXIT_FAILURE);
    // if (fcntl(schedulerToAnalyzer[0], F_SETPIPE_SZ, pipeSize) == -1)
    //     exit(EXIT_FAILURE);

    // struct rlimit rl;
    // long a = 1024;
    // long b = 2;
    // rl.rlim_cur = b * a * a * a; // 2 GB
    // rl.rlim_max = b * a * a * a; // 2 GB

    // if (setrlimit(RLIMIT_AS, &rl) == -1)
    // {
    //     perror("setrlimit");
    //     exit(EXIT_FAILURE);
    // }

    // if (getrlimit(RLIMIT_AS, &rl) == -1)
    // {
    //     perror("getrlimit");
    //     exit(EXIT_FAILURE);
    // }

    // creating processes for the entire SPMS (input, scheduler, anaylzer and output module)
    // int pids[MODULE_COUNT];
    while (moduleIndex < MODULE_COUNT)
    {
        int currPid = fork();
        if (currPid < 0)
        {
            perror("error while forking child processes\n");
            exit(1);
        }
        if (currPid == 0)
        {
            // child
            moduleType = MODULE_TYPES[moduleIndex];
            break;
        }
        else
        {
            // pids[moduleIndex] = currPid;
        }
        moduleIndex++;
    }

    if (moduleIndex == MODULE_COUNT)
    {
        // parent (finished, not tested)

        // responsiblity:
        // 1. send raw input command to SPMS (input module)
        // 2. listens to response from SPMS to decide next step ('0' = continue, '1' = terminate)

        closePipe(schedulerToAnalyzer);
        closePipe(schedulerToOutput);
        closePipe(outputToScheduler);
        closePipe(analyzerToOutput);
        closePipe(inputToScheduler);

        close(parentToSPMS[0]);
        close(SPMSToParent[1]);

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int bufferIndex = 0;
        bool sent = 0;
        printf("\nInput command: \n");
        while (1)
        {
            int in = getchar();
            if (in == EOF)
            {
                break;
            }
            char temp = (char)in;
            // printf("%c", temp);

            // ignore previous buffers that are not terminated with a ';'
            // e.g. command arg1 ... argN; asdfasdf abc \n
            // only "command arg1 ... argN;" will be sent, and " asdfasdf abc" are ignored
            if (temp == '\n')
            {
                if (bufferIndex > 0 && sent == 0)
                {
                    printf("(Note: unterminated input without ';' is ignored)\n");
                }
                sent = 0;
                bufferIndex = 0;
                continue;
            }
            else
            {
                buffer[bufferIndex++] = temp;
            }

            if (temp == ';')
            {
                if (bufferIndex == 1)
                {
                    printf("command should not be empty!\n");
                    bufferIndex = 0;
                    continue;
                }
                buffer[bufferIndex++] = '\0';

                // send raw input to the SPMS
                write(parentToSPMS[1], buffer, bufferIndex);
                sent = 1;
                bufferIndex = 0;
                char response[2];
                memset(response, 0, sizeof(response));
                // wait for response from SPMS before sending more
                if (read(SPMSToParent[0], response, 2) > 0)
                {
                    // printf("response: %s", response);
                    if (strcmp(response, "1") == 0)
                        break;
                    printf("\nInput command: \n");
                }
            }
        }
        // sleep(1);

        close(parentToSPMS[1]);
        close(SPMSToParent[0]);
        // printf("Program terminating...\n");
        // sleep(0.2);
        while (wait(NULL) > 0)
            ;
        fflush(stdout);
        printf("Program terminated.\n");
        return 0;
    }
    else if (moduleType == INPUT_MODULE)
    {
        // Input module (finished)

        // responsiblity:
        // 1. parse command and check if it's valid
        // 2. in case of batch input, read input from the specified file and send to scheduler one by one
        // 3. initiate bookings by sending input that aligns with protocol with scheduler module
        // 4. send response back to parent process ('0' = continue, '1' = terminate)

        closePipe(schedulerToAnalyzer);
        closePipe(schedulerToOutput);
        closePipe(analyzerToOutput);
        closePipe(outputToScheduler);

        close(inputToScheduler[0]);
        close(parentToSPMS[1]);
        close(SPMSToParent[0]);

        char *argsQueue[256]; // arguments may come in batch, like "0|0|...;1|0|...;", so we have to put them into a queue
        memset(argsQueue, 0, sizeof(argsQueue));
        int queueLength = 0;
        int queueIndex = 0;

        bool running = true;
        while (1)
        {
            if (queueIndex >= queueLength)
            {
                queueIndex = 0;
                getQueuedArguments(parentToSPMS[0], argsQueue, &queueLength, 4096, debug);
                // send response back to parent for it to continue sending input
                // if (queueLength == -2)
                //     break;
            }

            inputModuleProcess(argsQueue[queueIndex], SPMSToParent, inputToScheduler, &running, memberList, debug);
            free(argsQueue[queueIndex]);
            queueIndex++;
            if (running == false)
            {
                // printf("terminating\n");
                write(SPMSToParent[1], "1", 2);
                break;
            } else {
                write(SPMSToParent[1], "0", 2);
            }
        }
        close(inputToScheduler[1]);
        close(parentToSPMS[0]);
        close(SPMSToParent[1]);
        if (debug)
            printf("input module terminated\n");
    }
    else if (moduleType == SCHEDULER_MODULE)
    {
        // Scheduler module (finished)

        // expected input: (for each message read, separated by ';')
        // e.g. 0|1|2025-05-16|14:00|2.0|0,2,1,2;
        // e.g. 5|0

        // argument 0: command type index i, where (i=0,1,2,3,5)

        // 0 <= i <= 3:
        //      argument 1: member index (according to members[])
        //      argument 2: YYYY-MM-DD
        //      argument 3: hh:mm
        //      argument 4: n.n
        //      argument 5: additional essentials' enum (each separated by ',')
        //
        // i = 5:
        // argument 1: algorithm type enum (fcfs/prio/opti/ALL)

        // responsibility:
        // 1. receive input from input module
        // 2. stores all initiated booking requests
        // 3. send only necessary (calculated) data to output/analyzer module

        closePipe(SPMSToParent);
        closePipe(parentToSPMS);

        close(schedulerToAnalyzer[0]);
        close(schedulerToOutput[0]);
        close(analyzerToOutput[1]);
        close(inputToScheduler[1]);
        close(outputToScheduler[1]);

        const Booking *bookings[MAX_BOOKINGS_COUNT]; // this is mainly used for freeing memory
        Scheduler *schedulerFCFS = schedulerConstructor(FCFS);
        Scheduler *schedulerPriority = schedulerConstructor(PRIORITY);
        Scheduler *schedulerOptimized = schedulerConstructor(OPTIMIZED);
        int bookingsCount = 0;

        char *argsQueue[256]; // arguments may come in batch, like "0|0|...;1|0|...;", so we have to put them into a queue
        memset(argsQueue, 0, sizeof(argsQueue));
        int queueLength = 0;
        int queueIndex = 0;
        int debugCount = 0;


        while (1)
        {
            if (queueIndex >= queueLength)
            {
                queueIndex = 0;
                getQueuedArguments(inputToScheduler[0], argsQueue, &queueLength, 4096, debug);
            }
            if (queueLength < 0) // queueLength is set to -1 when eof is encountered
            {
                // printf("queuelength: %d\n", queueLength);
                break;
            }

            // printf("queue length immediately after get: %d\n", queueLength);
            // int i;
            // for (i = 0; i < queueLength; i++)
            //     printf("argsQueue[%d]: %s\n", i, argsQueue[i]);

            char *args[8];
            memset(args, 0, sizeof(args));
            // printf("queueLength: %d\n", queueLength);
            // printf("queueIndex: %d\n", queueIndex);
            if (debug)
            {
                printf("scheduler handling: %s\n", argsQueue[queueIndex]);
            }
            int argsCount = splitString(argsQueue[queueIndex], '|', args, 8);
            int x;
            free(argsQueue[queueIndex]);
            argsQueue[queueIndex] = NULL;
            queueIndex++;
            // for (x = 0; x < argsCount; x++)
            // printf("args[%d]: %s\n", x, args[x]);
            int commandIndex = atoi(args[0]); // only works for less than 10 commands
            if (commandIndex <= 3 && commandIndex >= 0)
            {
                int memberId = atoi(args[1]); // only works for less than 10 members
                char date[11];
                memset(date, 0, sizeof(date));
                strcpy(date, args[2]);
                char time[5];
                memset(time, 0, sizeof(time));
                strcpy(time, args[3]);
                char duration[5];
                memset(duration, 0, sizeof(duration));
                strcpy(duration, args[4]);
                char *temp[ESSENTIALS_DEVICE_COUNT]; // store the essentials enum from input (e.g. 0,2,1) to temp first
                char essentialsStr[20];
                memset(essentialsStr, 0, sizeof(essentialsStr));
                if (argsCount > 5)
                {
                    strcpy(essentialsStr, args[5]);
                }
                int essentialsCount = splitString(essentialsStr, ',', temp, ESSENTIALS_DEVICE_COUNT);
                enum EssentialsType essentials[ESSENTIALS_DEVICE_COUNT];
                int j = 0;
                while (j < essentialsCount)
                {
                    essentials[j] = atoi(temp[j]);
                    free(temp[j]);
                    j++;
                }
                while (j < ESSENTIALS_DEVICE_COUNT)
                    essentials[j++] = NONE_ESSENTIALS;

                Date *d = createDateFromString(date);
                Time *t = createTimeFromString(time);
                Timestamp *ts = createTimestamp(d, t);
                enum BookingType type = getCommandBookingType(commandIndex);
                const Booking *booking = createBooking(bookingsCount, memberId, ts, type, duration, essentials, essentialsCount);
                addBooking(schedulerFCFS, booking);
                addBooking(schedulerPriority, booking);
                addBooking(schedulerOptimized, booking);
                bookings[bookingsCount++] = booking;
                if (debug)
                {
                    printf("Booking added successfully! (%d in total)\n", bookingsCount);
                }
                // tempCount++;
                // printf("tempCount: %d", tempCount);
            }
            else if (commandIndex == 5)
            {

                enum Algorithm algorithm = (enum Algorithm)atoi(args[1]);
                if (algorithm == UNKNOWN_ALGORITHM)
                {
                    // supposedly input is already validated at input module
                    printf("Invalid algorithm!\n");
                    break;
                }
                if (algorithm == SUMMARY_REPORT)
                {
                    double runTime[ALGORITHM_COUNT];
                    runTime[0] = schedule(schedulerFCFS);
                    runTime[1] = schedule(schedulerPriority);
                    runTime[2] = schedule(schedulerOptimized);
                    if (debug == true)
                    {
                        printf("schedule time for FCFS: %.*f ms\n", 1, runTime[0]);
                        printf("schedule time for Priority: %.*f ms\n", 1, runTime[1]);
                        printf("schedule time for Optimized: %.*f ms\n", 1, runTime[2]);
                    }

                    int i;
                    
                    for (i = 0; i < bookingsCount; i++)
                    {
                        int totalTimeslotOccupiedFCFS = getTimeSlotSpanned(schedulerFCFS->bookings[i]->data->startTs, schedulerFCFS->bookings[i]->data->endTs);
                        int totalTimeslotOccupiedPriority = getTimeSlotSpanned(schedulerPriority->bookings[i]->data->startTs, schedulerPriority->bookings[i]->data->endTs);
                        int totalTimeslotOccupiedoptimized = getTimeSlotSpanned(schedulerOptimized->bookings[i]->data->startTs, schedulerOptimized->bookings[i]->data->endTs);

                        if (schedulerFCFS->bookings[i]->status != REJECTED && schedulerFCFS->bookings[i]->status != ACCEPTED)
                        {
                            printf("Invalid booking status: %d\n", schedulerFCFS->bookings[i]->status);
                            continue;
                        }
                        if (schedulerPriority->bookings[i]->status != REJECTED && schedulerPriority->bookings[i]->status != ACCEPTED)
                        {
                            printf("Invalid booking status: %d\n", schedulerPriority->bookings[i]->status);
                            continue;
                        }
                        char response[2];
                        memset(response, 0, sizeof(response));

                        char data1[256];
                        memset(data1, 0, sizeof(data1));
                        snprintf(data1, sizeof(data1), "%d|%d|%d|%d|%d;", 0, FCFS, ((int)schedulerFCFS->bookings[i]->status == ACCEPTED), totalTimeslotOccupiedFCFS, schedulerFCFS->bookings[i]->data->essentials);
                        // printf("data1: %s\n", data1);
                        write(schedulerToAnalyzer[1], data1, strlen(data1) + 1);
                        int n;
                        if (i == 0)
                        {
                            char* temp = malloc(2048* sizeof(char));
                            n = read(analyzerToScheduler[0], temp, sizeof(temp));
                            // printf("uncleared response in summary report shceduler: %d", n);
                            response[0] = *temp;
                            response[1] = '\0';
                            free(temp);
                            debugCount++;
                        }
                        else if ((n = read(analyzerToScheduler[0], response, sizeof(response)) > 0))
                        {
                            debugCount++;
                        }
                        // printf("response: (%s), n = (%d)\n", response, n);
                        char data2[256];
                        memset(data2, 0, sizeof(data2));
                        snprintf(data2, sizeof(data2), "%d|%d|%d|%d|%d;", 0, PRIORITY, ((int)schedulerPriority->bookings[i]->status == ACCEPTED), totalTimeslotOccupiedPriority, schedulerPriority->bookings[i]->data->essentials);
                        // printf("data2: %s\n", data2);
                        write(schedulerToAnalyzer[1], data2, strlen(data2) + 1);
                        if ((n = read(analyzerToScheduler[0], response, sizeof(response)) > 0))
                        {
                            debugCount++;
                            // printf("response: (%s), n = (%d)\n", response, n);
                        }
                        char data3[256];
                        memset(data3, 0, sizeof(data3));
                        snprintf(data3, sizeof(data3), "%d|%d|%d|%d|%d;", 0, OPTIMIZED, ((int)schedulerOptimized->bookings[i]->status == ACCEPTED), totalTimeslotOccupiedoptimized, schedulerOptimized->bookings[i]->data->essentials);
                        // printf("data3: %s\n", data3);
                        write(schedulerToAnalyzer[1], data3, strlen(data3) + 1);
                        if ((n = read(analyzerToScheduler[0], response, sizeof(response)) > 0))
                        {
                            debugCount++;
                            // printf("response: (%s), n = (%d)\n", response, n);
                        }
                    }
                    char response[2] = "";
                    write(schedulerToAnalyzer[1], "1;", 3);
                    if (read(analyzerToScheduler[0], response, sizeof(response)) > 0)
                    {
                        if (strcmp(response, "1") == 0)
                            printf("output module received errorneous input from scheduler module\n");
                    }
                }
                else
                {
                    Scheduler *targetScheduler = schedulerFCFS;
                    if (algorithm == FCFS)
                        targetScheduler = schedulerFCFS;
                    else if (algorithm == PRIORITY)
                        targetScheduler = schedulerPriority;
                    else if (algorithm == OPTIMIZED)
                        targetScheduler = schedulerOptimized;
                    else
                    {
                        printf("algorithm error\n");
                        continue;
                    }

                    schedule(targetScheduler);

                    int i;
                    for (i = 0; i < bookingsCount; i++)
                    {
                        const ProcessedBooking *currBooking = targetScheduler->bookings[i];
                        // const BookingStatus currStatus = targetScheduler->status[i];
                        char data[128];
                        memset(data, 0, sizeof(data));
                        char startTimestamp[17];
                        memset(startTimestamp, 0, sizeof(startTimestamp));
                        buildTimestampString(currBooking->data->startTs, startTimestamp);
                        char endTimestamp[17];
                        memset(endTimestamp, 0, sizeof(endTimestamp));
                        buildTimestampString(currBooking->data->endTs, endTimestamp);
                        bool accepted = 0;
                        if (currBooking->status == ACCEPTED)
                            accepted = 1;
                        else if (currBooking->status == REJECTED)
                            accepted = 0;
                        else
                        {
                            printf("Invalid booking status: %d\n", currBooking->status);
                            continue;
                        }
                        snprintf(data, sizeof(data), "%d|%d|%d|%d|%d|%s|%s|%d;", 0, currBooking->data->type, accepted, currBooking->data->memberId, currBooking->parkingSlotId, startTimestamp, endTimestamp, currBooking->data->essentials);
                        // send booking data one by one to output module
                        write(schedulerToOutput[1], data, strlen(data) + 1);
                        char res[2];
                        memset(res, 0, sizeof(res));
                        if (read(outputToScheduler[0], res, sizeof(res)) > 0)
                        {
                            if (strcmp(res, "0") != 0)
                            {
                                if (debug)
                                {
                                    printf("resending erroroneous entry\n");
                                }
                                i--; //resend
                                continue;
                            }
                        }
                        debugCount++;
                    }
                    char data[15];
                    memset(data, 0, sizeof(data));
                    // notify the output module which algorithm the previous bookings data belong to
                    snprintf(data, sizeof(data), "2|%d;", algorithm);

                    write(schedulerToOutput[1], data, strlen(data) + 1);

                    char response[2];
                    memset(response, 0, sizeof(response));
                    // wait for output module response before continuing with another round of data
                    if (read(outputToScheduler[0], response, sizeof(response)) > 0)
                    {
                        if (strcmp(response, "1") == 0)
                            printf("output module received errorneous input from scheduler module\n");
                    }
                }
                if (debug == true)
                {
                    struct rusage usage;
                    if (getrusage(RUSAGE_SELF, &usage) == -1)
                    {
                        perror("getrusage");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        printf("Memory usage: %ld KB\n", usage.ru_maxrss); // max resident set size in kilobytes
                    }
                    if (algorithm != SUMMARY_REPORT)
                        printf("entries sent from scheduler to output: %d\n", debugCount);
                    else
                        printf("entries sent from scheduler to analyzer: %d\n", debugCount);
                }
                debugCount = 0;
            }
            else
            {
                printf("invalid command index");
                continue;
            }
            int i;
            for (i = 0; i < argsCount; i++)
            {
                free(args[i]);
            }
        }
        freeSchedulerMemory(schedulerFCFS);
        freeSchedulerMemory(schedulerPriority);
        freeSchedulerMemory(schedulerOptimized);
        int i;
        for (i = 0; i < bookingsCount; i++)
        {
            freeBookingMemory(bookings[i]);
        }
        if (debug)
            printf("scheduler module terminated\n");
    }
    else if (moduleType == ANALYZER_MODULE)
    {
        // Analyzer module (Finished)
        //
        // expected input: (for each message read, separated by ';')
        //
        // - From scheduler module
        //      e.g. 0|0|1|5|bbb,ccc;
        //      e.g. 1;
        //
        //      argument 0: signal for decision (0 = read, 1 = finish, send analyzed data to output)
        //      argument 1: algorithm enum
        //      argument 2: accepeted or mot (0 = rejected, 1 = accepted)
        //      argument 3: number of timeslots this booking occupied
        //      argument 4: essentials enums

        // expected output:
        //
        // - To output module
        //      argument 0: signal for decision (1 = read summary report data; 2 = print the report)

        //      - when argument 0 == 1:
        //              argument 1: algorithm enum
        //              argument 2: accepted count
        //              argument 3: rejected count
        //              argument 4: total time slot count
        //              argument 5: time slot count for each essentials

        //      - when argument 0 == 2:
        //              argument 1: algorithm enum

        closePipe(outputToScheduler);
        closePipe(schedulerToOutput);
        closePipe(inputToScheduler);
        closePipe(SPMSToParent);
        closePipe(parentToSPMS);

        close(analyzerToScheduler[0]);
        close(schedulerToAnalyzer[1]);
        close(analyzerToOutput[0]);

        char *argsQueue[256]; // arguments may come in batch, like "0|0|...;1|0|...;", so we have to put them into a queue
        memset(argsQueue, 0, sizeof(argsQueue));
        int queueLength = 0;
        int queueIndex = 0;
        int bookingsResponse[ALGORITHM_COUNT][2]; // {{acceptedCountFCFS, rejectedCountFCFS}, {acceptedCountPrio, rejectedCountPrio}, ...}
        memset(bookingsResponse, 0, sizeof(bookingsResponse));
        int esssentialsTimeslot[ALGORITHM_COUNT][ESSENTIALS_DEVICE_COUNT];
        memset(esssentialsTimeslot, 0, sizeof(esssentialsTimeslot));
        int totalBookedTimeslot[ALGORITHM_COUNT];
        memset(totalBookedTimeslot, 0, sizeof(totalBookedTimeslot));
        int debugCount = 0;

        while (1)
        {
            if (queueIndex >= queueLength)
            {
                queueIndex = 0;
                getQueuedArguments(schedulerToAnalyzer[0], argsQueue, &queueLength, 4096, debug);
            }
            if (queueLength < 0)
                break; // queueLength is set to -1 when eof is encountered

            char *args[8];
            memset(args, 0, sizeof(args));
            int argsCount = splitString(argsQueue[queueIndex], '|', args, 8);
            free(argsQueue[queueIndex]);
            queueIndex++;

            if (argsCount == 0)
            {
                printf("error in format of argument received in analyzer module");
                continue;
            }

            int action = atoi(args[0]);
            if (action == 0)
            {
                int algorithm = atoi(args[1]);
                bool accepted = atoi(args[2]);
                int timeSlotSpanned = atoi(args[3]);
                int essentials = atoi(args[4]);
                int j = 0;
                bookingsResponse[algorithm][(int)accepted]++;
                for (j = 0; j < ESSENTIALS_DEVICE_COUNT; j++)
                {
                    int bit = ((essentials >> j) & 1);
                    // printf("");
                    // add the count of timeslot where this essentials is being used
                    esssentialsTimeslot[algorithm][j] += (bit * timeSlotSpanned * ((int)accepted));
                }
                if (accepted)
                    totalBookedTimeslot[algorithm] += timeSlotSpanned;
                debugCount++;
                write(analyzerToScheduler[1], "0", 2);
            }
            else
            {
                int i;
                for (i = 0; i < ALGORITHM_COUNT; i++)
                {
                    char data1[70];
                    char essentialsTimeslotString[30];
                    memset(data1, 0, sizeof(data1));
                    memset(essentialsTimeslotString, 0, sizeof(essentialsTimeslotString));
                    int j;
                    for (j = 0; j < ESSENTIALS_DEVICE_COUNT; j++)
                    {
                        char temp[30];
                        memset(temp, 0, sizeof(temp));

                        snprintf(temp, sizeof(temp), "%d", esssentialsTimeslot[i][j]);
                        esssentialsTimeslot[i][j] = 0;
                        if (j > 0)
                        {
                            strcat(essentialsTimeslotString, ",");
                        }
                        strcat(essentialsTimeslotString, temp);
                    }

                    snprintf(data1, sizeof(data1), "1|%d|%d|%d|%d|%s;", i, bookingsResponse[i][1], bookingsResponse[i][0], totalBookedTimeslot[i], essentialsTimeslotString);
                    // printf("data sent from analyzer to output: %s\n", data1);
                    write(analyzerToOutput[1], data1, strlen(data1) + 1);
                    bookingsResponse[i][0] = 0;
                    bookingsResponse[i][1] = 0;
                    totalBookedTimeslot[i] = 0;
                }
                write(analyzerToScheduler[1], "0", 2);
                if (debug)
                {
                    printf("number of entries received in analyzer: %d\n", debugCount);
                }
                char data2[8];
                memset(data2, 0, sizeof(data2));
                snprintf(data2, sizeof(data2), "2|%d;", SUMMARY_REPORT);
                // printf("data2: %s\n", data2);
                write(analyzerToOutput[1], data2, strlen(data2) + 1);
                debugCount = 0;
            }
        }

        if (debug)
            printf("analyzer module terminated\n");
    }
    else if (moduleType == OUTPUT_MODULE)
    {
        // Output module (finished)

        // (for each message read, separated by ';')
        //
        // expected input:
        // e.g. 0|0|1|2025-05-16 23:00|2025-05-16 3:00|0|bbb,ccc;
        // e.g. 1|0|500|320|%d|%s;
        // e.g. 2|0;
        // e.g. 2|2

        // argument 0: signal for decision

        // case arg0 = 2 (print current)
        // argument 1: algorithm enum

        // case arg0 = 0 (read booking schedule data)
        // arguemnt 1: booking type (according to enum BookingType)
        // argument 2: accepted or not (0 = rejected, 1 = accepted)
        // argument 3: member id
        // argument 4: parking slot id
        // argument 5: start date time
        // argument 6: start date time
        // argument 7: essentials

        // case arg0 = 1 (read summary report data)
        // argument 1: algorithm
        // argument 2: accepted count
        // argument 3: rejected count
        // argument 4: total time slot count
        // argument 5: time slot count for each essentials

        closePipe(schedulerToAnalyzer);
        // closePipe(analyzerToOutput);
        closePipe(inputToScheduler);
        closePipe(SPMSToParent);
        closePipe(parentToSPMS);

        close(schedulerToOutput[1]);
        close(analyzerToOutput[1]);
        close(outputToScheduler[0]);
        fcntl(schedulerToOutput[0], F_SETFL, fcntl(schedulerToOutput[0], F_GETFL) | O_NONBLOCK);
        fcntl(analyzerToOutput[0], F_SETFL, fcntl(analyzerToOutput[0], F_GETFL) | O_NONBLOCK);

        const char bookingScheduleFstring[] = "%18s %18s %18s %10s %40s\n";
        const int bookingScheduleHeaderLength = 130;

        char **schedulerArgsQueue = malloc(sizeof(char *) * 256); // arguments may come in batch, like "0|0|...;1|0|...;", so we have to put them into a queue
        char **analyzerArgsQueue = malloc(sizeof(char *) * 256);  // arguments may come in batch, like "0|0|...;1|0|...;", so we have to put them into a queue
        int schedulerQueueLength = 0;
        int schedulerQueueIndex = 0;
        int analyzerQueueLength = 0;
        int analyzerQueueIndex = 0;

        char *acceptedForMember[MAX_MEMBER_COUNT][1024];
        memset(acceptedForMember, 0, sizeof(acceptedForMember));
        char *rejectedForMember[MAX_MEMBER_COUNT][1024];
        memset(rejectedForMember, 0, sizeof(acceptedForMember));
        int acceptedCount[MAX_MEMBER_COUNT];
        memset(acceptedCount, 0, sizeof(acceptedCount));
        int rejectedCount[MAX_MEMBER_COUNT];
        memset(rejectedCount, 0, sizeof(rejectedCount));

        char *reportArguments[ALGORITHM_COUNT][5];
        memset(reportArguments, 0, sizeof(reportArguments));
        bool running = 1;
        int debugCounts[2] = {0, 0};

        while (running)
        {
            while (1) // queueLength is also set to -1 when no bytes are read in non-blocking mode
            {
                if (schedulerQueueIndex >= schedulerQueueLength)
                {
                    schedulerQueueIndex = 0;
                    getQueuedArguments(schedulerToOutput[0], schedulerArgsQueue, &schedulerQueueLength, 4096, debug);
                }
                if (schedulerQueueLength == -2 || (schedulerQueueLength <= 0 && running == false))
                { // -2 = eof
                    running = false;
                    break;
                }
                if (schedulerQueueLength <= 0 && errno == EAGAIN)
                {
                    // if no input in pipe yet, break and go check analyzer pipe
                    break;
                }
                char *args[12];
                memset(args, 0, sizeof(args));
                int argsCount = splitString(*(schedulerArgsQueue + schedulerQueueIndex), '|', args, 12);
                // printf("argsCount: %d\n", argsCount);
                free(schedulerArgsQueue[schedulerQueueIndex]);
                schedulerQueueIndex++;

                int outputType = atoi(args[0]);
                if (outputType != 2)
                {
                    bool accepted = atoi(args[2]);
                    int memberId = atoi(args[3]);
                    int parkingSlotId = atoi(args[4]);
                    int essentials = atoi(args[7]);
                    // printf("essentials: %d\n", essentials);
                    char line[512] = "";
                    char essentialsStr[256] = "";
                    int i;
                    for (i = 0; i < ESSENTIALS_DEVICE_COUNT; i++)
                    {
                        if (getBit(essentials, i) != 1)
                        {
                            continue;
                        }
                        char temp[30] = "";
                        snprintf(temp, sizeof(temp), "%s", getEssentialsTypesString(i));
                        if (strlen(essentialsStr) == 0)
                        {
                            strcat(essentialsStr, "[");
                        }
                        else if (i > 0)
                        {
                            strcat(essentialsStr, ", ");
                        }
                        strcat(essentialsStr, temp);
                    }
                    if (strlen(essentialsStr) > 0)
                        strcat(essentialsStr, "]");
                    char parkingSlotIdString[20] = "";
                    sprintf(parkingSlotIdString, "%d", parkingSlotId);
                    // build the line: startDateTime, endDateTime, Type, Essentials
                    snprintf(line, sizeof(line), bookingScheduleFstring, parkingSlotIdString, args[5], args[6], getBookingTypeString(atoi(args[1])), essentialsStr);
                    if (accepted)
                    {
                        acceptedForMember[memberId][acceptedCount[memberId]] = malloc(strlen(line) + 1);
                        strcpy(acceptedForMember[memberId][acceptedCount[memberId]++], line);
                    }
                    else
                    {
                        rejectedForMember[memberId][rejectedCount[memberId]] = malloc(strlen(line) + 1);
                        strcpy(rejectedForMember[memberId][rejectedCount[memberId]++], line);
                    }
                    write(outputToScheduler[1], "0", 2);
                    debugCounts[0]++;
                }
                else
                {
                    int i, j;
                    enum Algorithm algorithm = atoi(args[1]);
                    printf("\n\n*** Parking Booking – ACCEPTED / %s ***\n\n", getAlgorithmName(algorithm));
                    for (i = 0; i < MAX_MEMBER_COUNT; i++)
                    {
                        if (acceptedCount[i] == 0)
                            continue;
                        printf("\n%s has the following bookings:\n", getMemberName(memberList, i));
                        char header[90];
                        sprintf(header, bookingScheduleFstring, "Parking Slot ID", "Start Date & Time", "End Date & Time", "Type", "Device");
                        int x;
                        for (x = 0; x < bookingScheduleHeaderLength; x++)
                            printf("=");
                        printf("\n");
                        printf("%s", header);
                        for (j = 0; j < acceptedCount[i]; j++)
                        {
                            printf("%s", acceptedForMember[i][j]);
                            free(acceptedForMember[i][j]);
                        }
                        acceptedCount[i] = 0;
                        for (x = 0; x < bookingScheduleHeaderLength; x++)
                            printf("=");
                        printf("\n\n");
                    }

                    printf("\n\n*** Parking Booking – REJECTED / %s ***\n\n", getAlgorithmName(algorithm));
                    for (i = 0; i < MAX_MEMBER_COUNT; i++)
                    {
                        if (rejectedCount[i] == 0)
                            continue;
                        printf("\n%s has the following bookings:\n", getMemberName(memberList, i));
                        char header[90];
                        sprintf(header, bookingScheduleFstring, "Parking Slot ID", "Start Date & Time", "End Date & Time", "Type", "Device");
                        int x;
                        for (x = 0; x < bookingScheduleHeaderLength; x++)
                            printf("=");
                        printf("\n");
                        printf("%s", header);
                        for (j = 0; j < rejectedCount[i]; j++)
                        {
                            printf("%s", rejectedForMember[i][j]);
                            free(rejectedForMember[i][j]);
                        }
                        rejectedCount[i] = 0;
                        for (x = 0; x < bookingScheduleHeaderLength; x++)
                            printf("=");
                        printf("\n\n");
                    }
                    printf("-- END --\n");
                    write(outputToScheduler[1], "0", 2);
                    if (debug)
                    {
                        printf("entries received in output module from scheduler: %d\n", debugCounts[0]);
                    }
                    debugCounts[0] = 0;
                }
            }

            while (1)
            {
                if (analyzerQueueIndex >= analyzerQueueLength)
                {
                    analyzerQueueIndex = 0;
                    getQueuedArguments(analyzerToOutput[0], analyzerArgsQueue, &analyzerQueueLength, 8192,debug);
                }
                if (analyzerQueueLength == -2 || (analyzerQueueLength <= 0 && running == false))
                { // -2 = eof
                    running = false;
                    break;
                }
                if (analyzerQueueLength <= 0 && errno == EAGAIN)
                {
                    // if no input in pipe yet, break and go check scheduler pipe
                    break;
                }
                char *args[12];
                memset(args, 0, sizeof(args));
                int argsCount = splitString(analyzerArgsQueue[analyzerQueueIndex], '|', args, 12);
                analyzerArgsQueue[analyzerQueueIndex];
                analyzerQueueIndex++;

                // printf("received args in output module:\n");
                // int i;
                // for (i = 0; i < argsCount; i++)
                //     printf("arg[%d]: %s, ", i, args[i]);
                // printf("\n");

                int outputType = atoi(args[0]);
                if (outputType >= 3 || outputType < 0)
                {
                    printf("Invalid input sent to output module from analyzer\n");
                    continue;
                }
                if (outputType != 2)
                {
                    int algorithm = atoi(args[1]);
                    reportArguments[algorithm][0] = args[2];
                    reportArguments[algorithm][1] = args[3];
                    reportArguments[algorithm][2] = args[4];
                    reportArguments[algorithm][3] = args[5];
                }
                else
                {
                    printf("\n\n*** Parking Booking Manager – Summary Report ***\n\n");
                    printf("Performance:\n");
                    int i;
                    for (i = 0; i < 3; i++)
                    {
                        int acceptedCount = atoi(reportArguments[i][0]);
                        int rejectedCount = atoi(reportArguments[i][1]);
                        int totalBooking = acceptedCount + rejectedCount;
                        int totalTimeSlots = atoi(reportArguments[i][2]);
                        char *temp[ESSENTIALS_DEVICE_COUNT];
                        int essentialsTimeSlots[ESSENTIALS_DEVICE_COUNT];
                        splitString(reportArguments[i][3], ',', temp, ESSENTIALS_DEVICE_COUNT);
                        free(reportArguments[i][3]);
                        int j;
                        int totalEssentialsTimeSlot = 0;
                        for (j = 0; j < ESSENTIALS_DEVICE_COUNT; j++)
                        {
                            int x = atoi(temp[j]);
                            totalEssentialsTimeSlot += x;
                            essentialsTimeSlots[j] = x;
                        }

                        printf("\tFor %s:\n", getAlgorithmName(i));
                        printf("\t\t Total Number of Bookings Received: %d\n", totalBooking);
                        printf("\t\t Total Number of Bookings Accepted: %d (%.*f %%)\n", acceptedCount, 1, (float)(100.0 * acceptedCount / totalBooking));
                        printf("\t\t Total Number of Bookings Rejected: %d (%.*f %%)\n", rejectedCount, 1, (float)(100.0 * rejectedCount / totalBooking));
                        printf("\tRelative Utilization of time slots: (out of all booked time slots, usage %% of essentials)\n");
                        printf("\t\t %20s - %.*f%% (%d / %d)\n", "Total", 1, (100.0 * totalEssentialsTimeSlot / (totalTimeSlots * ESSENTIALS_DEVICE_COUNT * 1.0)), totalEssentialsTimeSlot, totalTimeSlots * ESSENTIALS_DEVICE_COUNT);
                        for (j = 0; j < ESSENTIALS_DEVICE_COUNT; j++)
                        {
                            printf("\t\t %20s - %.*f%% (%d / %d)\n", getEssentialsTypesString(j), 1, (100.0 * essentialsTimeSlots[j] / (totalTimeSlots * 1.0)), essentialsTimeSlots[j], totalTimeSlots);
                        }

                        printf("\tAbsolute Utilization of time slots: (out of all booked/unbooked time slots, usage %% of essentials)\n");
                        printf("\t\t %20s - %.*f%% (%d / %d)\n", "Total", 1, (100.0 * totalEssentialsTimeSlot / (PARKING_SLOTS_COUNT * TIME_SLOT_HOURS * 7 * ESSENTIALS_DEVICE_COUNT * 1.0)), totalEssentialsTimeSlot, PARKING_SLOTS_COUNT * TIME_SLOT_HOURS * 7 * ESSENTIALS_DEVICE_COUNT);
                        for (j = 0; j < ESSENTIALS_DEVICE_COUNT; j++)
                        {
                            printf("\t\t %20s - %.*f%% (%d / %d)\n", getEssentialsTypesString(j), 1, (100.0 * essentialsTimeSlots[j] / (1.0 * PARKING_SLOTS_COUNT * TIME_SLOT_HOURS * 7)), essentialsTimeSlots[j], PARKING_SLOTS_COUNT * TIME_SLOT_HOURS * 7); // Note: this project only tests timeslots in a week
                        }
                    }
                    printf("-- END -- \n");
                    write(outputToScheduler[1], "0", 2);
                    memset(reportArguments, 0, sizeof(reportArguments));
                }
            }
        }
        if (debug)
            printf("output module terminated\n");
    }
    freeMemberList(memberList);
    return 0;
}
