/****************************************************************************/
/*    FILE:  FILE.H                                                         */
/****************************************************************************/
#ifndef _FILE_H_
#define _FILE_H_

#include "GENERAL.H"

#define MDAT_RESOURCE_FILENAME "res/MDAT.RES"
#define PICTURE_RESOURCE_FILENAME "res/PICS.RES"
#define SAMPLE_RESOURCE_FILENAME "res/SAMPLE.RES"
#define SOUNDS_RESOURCE_FILENAME "res/SOUNDS.RES"
#define SOUNDS16_RESOURCE_FILENAME "res/SOUNDS16.RES"


typedef T_word32 T_file;

#define FILE_BAD -1

/* All modes, except FILE_MODE_READ, creates a new file if it does */
/* not already exist. */
typedef enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_READ_WRITE,
    FILE_MODE_UNKNOWN
} E_fileMode;

T_file
FileOpen(T_byte8 *p_filename, E_fileMode mode);

T_void
FileClose(T_file file);

/* All seeks are from the beginning of the file. */
T_void
FileSeek(T_file file, T_word32 position);

T_sword32
FileRead(T_file file, T_void *p_buffer, T_word32 size);

T_sword32
FileWrite(T_file file, T_void *p_buffer, T_word32 size);

T_void *
FileLoad(T_byte8 *p_filename, T_word32 *p_size);

T_word32
FileGetSize(T_byte8 *p_filename);

E_Boolean
FileFindFirst(T_byte8 *searchPattern, T_byte8 *filename);

E_Boolean
FileFindNext(T_byte8 *filename);

E_Boolean
FileExist(T_byte8 *p_filename);

#if defined(TARGET_UNIX)
#define ConcatenatePaths(parent, sub) parent "/" sub
#elif defined(TARGET_DOS) || defined(TARGET_WIN32)
#define ConcatenatePaths(parent, sub) parent "\" sub
#endif

#endif

/****************************************************************************/
/*    END OF FILE:  FILE.H                                                  */
/****************************************************************************/
