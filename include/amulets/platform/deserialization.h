//
// Created by Emre Çelikten on 23/10/2016.
//

#ifndef AMULETSARMOR_SERIALIZATION_H
#define AMULETSARMOR_SERIALIZATION_H
#include "IRESOURC.H"

#define POINTER_SIZE_DIFF (sizeof(void*) - sizeof(int))

T_sword32
LoadResourceFileHeader(T_file file, T_resourceFileHeader *resourceFileHeader);

T_sword32
LoadResourceEntries(T_file file, T_resourceEntry *resourceEntry, T_word32 numEntries);

T_void
DeserializeObjectType(T_file file, T_resourceEntry *p_resourceEntry);

#endif //AMULETSARMOR_SERIALIZATION_H
