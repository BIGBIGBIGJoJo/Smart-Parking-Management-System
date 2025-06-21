#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "member.h"

MemberList * initializeMembers(const char* members[], const int membersCount)
{
    MemberList *memberList = malloc(sizeof(MemberList));
    memberList->count = 0;
    int i;
    for (i  =0; i < membersCount; i++)
    {
        Member *m = malloc(sizeof(Member));
        m->id = i;
        strcpy(m->name, members[i]);
        memberList->members[memberList->count++] = m;
    }
    return memberList;
}

void freeMemberList(MemberList* memberList)
{
    int i;
    for (i = 0; i < memberList->count; i++)
    {
        free(memberList->members[i]);
    }
    free(memberList);
}

int getMemberId(MemberList* memberList, const char* member)
{
    int i;
    for (i = 0; i < memberList->count; i++)
    {
        // printf("members[i]: (%s)\n", members[i]);
        // printf("member: (%s)\n", member);
        // printf("result: %d\n", strcmp(members[i], member));
        if (strcmp(memberList->members[i]->name, member) == 0)
        {
            return i;
        }
    }
    return -1;
}

const char* getMemberName(MemberList* memberList, int id)
{
    return getMember(memberList, id)->name;
}

Member* getMember(MemberList* memberList, int id)
{
    return memberList->members[id];
}