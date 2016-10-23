/****************************************************************************/
/*    FILE:  IRESOURC.H                                                     */
/****************************************************************************/
#ifndef _IRESOURC_H_
#define _IRESOURC_H_

#include "FILE.H"
#include "GENERAL.H"
#include "RESOURCE.H"

#define MAX_RESOURCE_FILES 10

#define RESOURCE_ENTRY_TYPE_MEMORY     0x00
#define RESOURCE_ENTRY_TYPE_DISK       0x01
#define RESOURCE_ENTRY_TYPE_DISCARDED  0x02

//#ifndef NDEBUG
#define RESOURCE_ENTRY_TYPE_FILE_LOADED 0x80
//#endif

#define RESOURCE_ENTRY_TYPE_FILE       0x00
#define RESOURCE_ENTRY_TYPE_DIRECTORY  0x10
#define RESOURCE_ENTRY_TYPE_LINK       0x20
#define RESOURCE_ENTRY_TYPE_APPEND     0x30

#define RESOURCE_ENTRY_TYPE_UNKNOWN    0xC3

#define RESOURCE_ENTRY_TYPE_MASK_WHERE    0x0F
#define RESOURCE_ENTRY_TYPE_MASK_TYPE     0xF0

#define RESOURCE_FILE_UNIQUE_ID (*((T_word32 *)("Res!")))

typedef struct
{
    T_byte8 resID[4]; /* Should contain "ReS"+'\0' id */
    T_byte8 p_resourceName[14]; /* Case sensitive, 13 characters + '\0' */
    T_word32 fileOffset;
    T_word32 size; /* Size in bytes. */
    T_word16 lockCount; /* 0 = unlocked. */
    T_byte8 resourceType;
    T_byte8 *p_data;
    T_resourceFile resourceFile; /* Resource file this is from. */
    T_void *ownerDir; /* Locked in owner directory (or NULL) */
} PACK T_resourceEntry;

/* General information stored for each resource directory in the system. */
typedef struct
{
    T_file fileHandle;
    T_word16 numberEntries;
    T_resourceEntry *p_entries;
    T_resource ownerRes;
    T_sword16 nextResource;
} PACK T_resourceDirInfo;

/* Header that is placed at the beginning of the file to locate */
/* the resource index. */
typedef struct
{
    T_word32 uniqueID;
    T_word32 indexOffset;
    T_word32 indexSize; /* Size of index in bytes. */
    T_word16 numEntries; /* Number of entries in the index. */
} PACK T_resourceFileHeader;

#endif // _IRESOURC_H_

/****************************************************************************/
/*    End of FILE:  IRESOURC.H                                              */
/****************************************************************************/

