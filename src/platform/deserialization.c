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

#include "GENERAL.h"
#include "OBJTYPE.h"
#if defined(TARGET_UNIX)
#include <unistd.h>
#elif defined(TARGET_OS_WIN32)
// TODO: Win
#include <direct.h>
#endif
#include <IRESOURC.H>
#include <MEMORY.h>
#include <OBJTYPE.h>
#include <IRESOURC.h>
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

        offset +=
            2 * POINTER_SIZE_DIFF + sizeof(T_resourceFile); // Size of 2 pointers in a 32-bit system is 8 bytes
    }

    free(buf);

    DebugEnd();

    return result;
}

static const T_word32 oldSize = 2;

/**
 * A function for loading object type data from RES files.
 *
 * The records in RES files contain cross-platform incompatible data - the space allocated for pointers is 4 bytes.
 * This method reads raw data into a buffer, then aligns the memory properly.
 *
 * @param file resource file from which the data will be loaded
 * @param p_resourceEntry resource that points to the data
 */
T_void
DeserializeObjectType(T_file file, T_resourceEntry *p_resourceEntry)
{
    T_word32 numPictures;
    T_byte8 *buf;
    T_word16 i, j;
    T_word16 numStances;
    T_word16 numTotalFrames;
    T_word16 totalShift;
    T_word16 picListOffset;
    T_objectFrame *p_frame;
    T_byte8 *p_src;
    T_byte8 *p_dst;


    /* Read the raw data to buffer */
    buf = MemAlloc(p_resourceEntry->size);

    FileSeek(file, p_resourceEntry->fileOffset);
    FileRead(file, buf, p_resourceEntry->size);

    p_src = buf;

    /* Since the file is variable length now (we don't know the amount of stances, frames, etc. in advance,
     * we have to do a discovery step of computing the memory size on the target platform. */
    numStances = ((T_objectType *) buf)->numStances;
    p_src += sizeof(T_objectType) - sizeof(T_objectStance); /* Shift to stance part */

    numTotalFrames = 0;

    /* Iterate over all stances to find number of frames.*/
    for (i = 0; i < numStances; i++)
    {
        numTotalFrames += ((T_objectStance *) p_src)->numFrames;
        p_src += sizeof(T_objectStance);
    }

    p_src = buf + ((T_objectType *) buf)->stances[0].offsetFrameList;
    numPictures = (p_resourceEntry->size - ((T_objectFrame *) p_src)->offsetPicList) / 10;
    picListOffset = ((T_objectFrame *) p_src)->offsetPicList;

    if ((p_resourceEntry->size - ((T_objectFrame *) p_src)->offsetPicList) % 10 != 0)
    {
        printf("Warning: The records cannot be split properly for resource %s!\n", p_resourceEntry->p_resourceName);
    }

    /* Now that we have total number of pictures, we have enough data to find out the size of memory we need. */
    /* Start by allocating a spot for this resource. */
    p_resourceEntry->p_data = MemAlloc(
        sizeof(T_resourceEntry *) +
            sizeof(T_objectType) +
            sizeof(T_objectStance) * (numStances - 1) + /* The first one is implicitly counted in objectType */
            sizeof(T_objectFrame) * numTotalFrames +
            sizeof(T_objectPic) * numPictures
    );

    DebugCheck(p_resourceEntry->p_data);

    /* So we can have a backward reference to the resource handle */
    /* when given the data, we will store a prefix pointer back */
    /* to the reference handle.  However, we will return only a */
    /* pointer to the actual data. */
    *((T_resourceEntry **) (p_resourceEntry->p_data)) = p_resourceEntry;

#ifdef RESOURCE_OUTPUT
    printf("!A %ld lock_%s\n", p_resource->size, p_resource->p_resourceName) ;
#endif
    /* skip past the entry reference. */
    p_resourceEntry->p_data += sizeof(T_resourceEntry *);

    /* Re-initialize our moving pointers */
    p_src = buf;
    p_dst = p_resourceEntry->p_data;

    /* We can copy the bytes up until the picture list directly. */
    memcpy(p_dst, p_src, picListOffset);

    /* Initialize frame pointer to the start of frames in the new memory address */
    p_frame = (T_objectFrame*)(p_dst + ((T_objectType *) p_src)->stances[0].offsetFrameList);

    p_src += picListOffset;
    p_dst += picListOffset;
    totalShift = 0;

    /* Now start filling the structs based on the raw data in buffer, seeing the memory as T_objectPic entries */
    /* We have to update the offsets in frames too, since they are different now. */
    for (i = 0; i < numTotalFrames; i++, p_frame++)
    {
        p_frame->offsetPicList += totalShift;
        for (j = 0; j < p_frame->numAngles; j++, p_src += 10, p_dst += sizeof(T_objectPic))
        {
            ((T_objectPic *) p_dst)->number = *((T_word16 *) p_src);
            ((T_objectPic *) p_dst)->resource = NULL;
            ((T_objectPic *) p_dst)->p_pic = NULL;
            totalShift += 8;
        }
    }

    MemFree(buf);
}

