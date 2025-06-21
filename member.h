#ifndef MEMBER_H
#define MEMBER_H

#define MAX_MEMBER_COUNT 5 // increase if needed in the future

typedef struct Member
{
    char name[50];
    int id;
} Member;

typedef struct MemberList
{
    Member* members[MAX_MEMBER_COUNT];
    int count;
} MemberList;

const char* getMemberName(MemberList* memberList, int id);
Member* getMember(MemberList* memberList, int id);
int getMemberId(MemberList* memberList, const char* member);
void freeMemberList(MemberList* memberList);
MemberList * initializeMembers(const char* members[], int membersCount);

#endif
