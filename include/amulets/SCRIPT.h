/****************************************************************************/
/*    FILE:  SCRIPT.H                                                       */
/****************************************************************************/
#ifndef _SCRIPT_H_
#define _SCRIPT_H_

#include "GENERAL.H"

typedef T_void *T_script;
#define SCRIPT_BAD  NULL

typedef T_byte8 E_scriptDataType;
#define SCRIPT_DATA_TYPE_8_BIT_NUMBER           0
#define SCRIPT_DATA_TYPE_16_BIT_NUMBER          1
#define SCRIPT_DATA_TYPE_32_BIT_NUMBER          2
#define SCRIPT_DATA_TYPE_STRING                 3
#define SCRIPT_DATA_TYPE_VARIABLE               4
#define SCRIPT_DATA_TYPE_EVENT_PARAMETER        5
#define SCRIPT_DATA_TYPE_FLAG                   6
#define SCRIPT_DATA_TYPE_NONE                   7
#define SCRIPT_DATA_TYPE_UNKNOWN                8

typedef T_byte8 E_scriptFlag;
#define SCRIPT_FLAG_EQUAL                  0
#define SCRIPT_FLAG_NOT_EQUAL              1
#define SCRIPT_FLAG_LESS_THAN              2
#define SCRIPT_FLAG_NOT_LESS_THAN          3
#define SCRIPT_FLAG_GREATER_THAN           4
#define SCRIPT_FLAG_NOT_GREATER_THAN       5
#define SCRIPT_FLAG_LESS_THAN_OR_EQUAL     6
#define SCRIPT_FLAG_GREATER_THAN_OR_EQUAL  7
#define SCRIPT_FLAG_UNKNOWN                8

#define SCRIPT_TAG             (*((T_word32 *)"SpT"))
#define SCRIPT_TAG_BAD         (*((T_word32 *)"sBd"))
#define SCRIPT_TAG_DISCARDABLE (*((T_word32 *)"DsP"))

#define SCRIPT_INSTANCE_TAG             (*((T_word32 *)"SiT"))
#define SCRIPT_INSTANCE_TAG_BAD         (*((T_word32 *)"sIb"))

#define SCRIPT_MAX_STRING 80

#define OBJECT_SCRIPT_ATTR_X                   0
#define OBJECT_SCRIPT_ATTR_Y                   1
#define OBJECT_SCRIPT_ATTR_Z                   2
#define OBJECT_SCRIPT_ATTR_MAX_VELOCITY        3
#define OBJECT_SCRIPT_ATTR_STANCE              4
#define OBJECT_SCRIPT_ATTR_WAS_BLOCKED         5
#define OBJECT_SCRIPT_ATTR_X16                 6
#define OBJECT_SCRIPT_ATTR_Y16                 7
#define OBJECT_SCRIPT_ATTR_Z16                 8
#define OBJECT_SCRIPT_ATTR_ANGLE               9
#define OBJECT_SCRIPT_ATTR_UNKNOWN             10

typedef struct
{
    T_byte8 length;
    T_byte8 data[SCRIPT_MAX_STRING];
} T_scriptString;

typedef union
{
    T_sword32 number;
    T_scriptString *p_string;
} T_scriptNumberOrString;

typedef struct
{
    E_scriptDataType type;
    T_scriptNumberOrString ns;
} T_scriptDataItem;

typedef struct T_scriptHeader_
{
    T_word16 highestEvent;            /* Number of events in this script */
    /* that might be handled. */
    T_word16 highestPlace;            /* Number of places in this script */
    /* that might be handled. */
    T_word32 sizeCode;                /* size of code. */
    T_word32 reserved[6];             /* Reserved for future use. */
    T_word32 number;                  /* Script number to identify it. */
    T_word32 tag;                     /* Tag to tell its memory state. */
    struct T_scriptHeader_ *p_next;          /* Pointer to next script. */
    struct T_scriptHeader_ *p_prev;          /* Pointer to previous script. */
    T_word32 lockCount;               /* Number of users of this script. */
    /* A lock count of zero means that */
    /* the script is discardable. */
    T_byte8 *p_code;                  /* Pointer to code area. */
    T_word16 *p_events;               /* Pointer to events list. */
    T_word16 *p_places;               /* Pointer to places list. */
} PACK T_scriptHeader;

typedef struct
{
    T_word32 instanceTag;
    T_scriptHeader *p_header;
    T_word32 owner;
    /* Identifier of owner (may be pointer) */

    T_scriptDataItem vars[256];
} T_scriptInstance;

typedef struct
{
    T_scriptHeader *p_script;
    T_word16 position;
} T_continueData;

typedef T_word16 (*T_scriptCommand)(
    T_scriptHeader *script,
    T_word16 position);

T_void
ScriptInitialize(T_void);

T_void
ScriptFinish(T_void);

T_script
ScriptLock(T_word32 number);

T_void
ScriptUnlock(T_script script);

E_Boolean
ScriptEvent(
    T_script script,
    T_word16 eventNumber,
    E_scriptDataType type1,
    T_void *p_data1,
    E_scriptDataType type2,
    T_void *p_data2,
    E_scriptDataType type3,
    T_void *p_data3);

E_Boolean
ScriptRunPlace(
    T_script script,
    T_word16 placeNumber);

T_word32
ScriptGetOwner(T_script script);

T_void
ScriptSetOwner(T_script script, T_word32 owner);

#endif // _SCRIPT_H_

/****************************************************************************/
/*    END OF FILE:  SCRIPT.H                                                */
/****************************************************************************/
