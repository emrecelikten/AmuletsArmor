/*
 * A small set of functions to read C structs from disk.
 *
 * The reason for these functions is that some fields suchs as pointers in structs have different size,
 * besed on the operating system or architecture. For example, a pointer should have the size of 8 bytes under a 64-bit
 * system, where it is 4 bytes under 32-bit. This causes issues while reading records, which have a fixed size.
 *
 * Also, there might be padding issues.
 *
 * The solution is reading all of the file to a char buffer, then filling up the struct elements one by one.
 *
 * Created by Emre Çelikten on 23/10/2016.
 */

#include <GENERAL.H>
#if defined(TARGET_UNIX)
#include <unistd.h>
#elif defined(TARGET_OS_WIN32)
// TODO: Win
#include <direct.h>
#endif
#include <IRESOURC.H>
#include "platform/deserialization.h"

T_sword32
LoadResourceFileHeader(T_file file, T_resourceFileHeader *resourceFileHeader)
{
    T_sword32 result;
    char buf[sizeof(T_resourceFileHeader)];
    ssize_t offset = 0;

    DebugRoutine("LoadResourceFileHeader");
    DebugCheck(file != FILE_BAD);
    //    DebugCheck(size > 0) ;

    result = read(file, buf, sizeof(T_resourceFileHeader));

    memcpy(&(resourceFileHeader->uniqueID), buf, sizeof(T_word32));

    offset += sizeof(T_word32);

    memcpy(&(resourceFileHeader->indexOffset), buf + offset, sizeof(T_word32));

    offset += sizeof(T_word32);

    memcpy(&(resourceFileHeader->indexSize), buf + offset, sizeof(T_word32));

    offset += sizeof(T_word32);

    memcpy(&(resourceFileHeader->numEntries), buf + offset, sizeof(T_word16));

    DebugEnd();

    return result;
}

T_sword32
LoadResourceEntries(T_file file, T_resourceEntry *resourceEntry, T_word32 numEntries)
{
    T_sword32 result;
    char *buf;
    ssize_t offset = 0;
    int i;

    buf = (char *) calloc(numEntries, sizeof(T_resourceEntry));

    DebugRoutine("LoadResourceEntries");
    DebugCheck(file != FILE_BAD);

    result = read(file, buf, sizeof(T_resourceEntry) * numEntries);
    for (i = 0; i < numEntries; i++)
    {
        memcpy(&(resourceEntry[i].resID), buf + offset, sizeof(T_byte8) * 4);
        offset += sizeof(T_byte8) * 4;

        memcpy(&(resourceEntry[i].p_resourceName), buf + offset, sizeof(T_byte8) * 14);
        offset += sizeof(T_byte8) * 14;

        memcpy(&(resourceEntry[i].fileOffset), buf + offset, sizeof(T_word32));
        offset += sizeof(T_word32);

        memcpy(&(resourceEntry[i].size), buf + offset, sizeof(T_word32));
        offset += sizeof(T_word32);

        memcpy(&(resourceEntry[i].lockCount), buf + offset, sizeof(T_word16));
        offset += sizeof(T_word16);

        memcpy(&(resourceEntry[i].resourceType), buf + offset, sizeof(T_byte8));
        offset += sizeof(T_byte8);

        resourceEntry[i].p_data = NULL;
        resourceEntry[i].resourceFile = (T_word16) 0;
        resourceEntry[i].ownerDir = NULL;

        offset += 8 + sizeof(T_resourceFile); // Size of 2 pointers in a 32-bit system is 8 bytes
    }

    free(buf);

    DebugEnd();

    return result;
}
