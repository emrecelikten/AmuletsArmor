/****************************************************************************/
/*    FILE:  OBJTYPE.H                                                      */
/****************************************************************************/
#ifndef _OBJTYPE_H_
#define _OBJTYPE_H_

#include "GENERAL.h"
#include "RESOURCE.h"

/*---------------------------------------------------------------------------
 * Constants:
 *--------------------------------------------------------------------------*/
#define OBJECT_TYPE_INSTANCE_BAD NULL

/*---------------------------------------------------------------------------
 * Types:
 *--------------------------------------------------------------------------*/
typedef enum
{
    ORIENTATION_NORMAL,
    ORIENTATION_REVERSE,
    ORIENTATION_UNKNOWN
} T_orientation;

/** !!! Apparently, GNU CC does not allow you to specify the storage type **/
/** for an enum!  It doesn't issue a warning, it just makes all enums into **/
/** ints, which in our world are T_word32's.  Thus the structure no longer **/
/** works with the same res file.  For now, I just redefined things as **/
/** follows, to get it to work. **/

typedef T_word16 E_objectAnimateType;

#define OBJECT_ANIMATE_ORDERED 0
#define OBJECT_ANIMATE_BOUNCE  1
#define OBJECT_ANIMATE_RANDOM  2
#define OBJECT_ANIMATE_UNKNOWN 3

/*-------------------------------------------------------------------------*
 * Typedef:  T_objectStance
 *-------------------------------------------------------------------------*/
/**
 *  An animated object can be broken down into the number of stances
 *  that makes up the animation.  A stance is in itself an animation
 *  state (e.g., walking, talking, flying, etc.) or transition
 *  (e.g. landing, getting hit, taking off, etc.).
 *
 *  @param numFrames -- Number of frames that make up this stance.
 *  @param speed -- Interval in between animation frames.
 *      A speed of 0 means not to animate at all.  This
 *      is useful for objects with only one frame.
 *  @param type -- What type of animation
 *      (See E_objectAnimateType)
 *  @param nextStance -- Stance to go to after completion of animation.
 *  @param offsetFrameList -- Offset in bytes from the beginning of
 *      T_objectType structure that this stance is in to
 *      the list of frames that make up this stance.
 *
 *<!-----------------------------------------------------------------------*/
typedef struct
{
    T_word16 numFrames;
    T_word16 speed;
    E_objectAnimateType type;
    T_word16 nextStance;
    T_word16 offsetFrameList;
} PACK T_objectStance;

/*-------------------------------------------------------------------------*
 * Typedef:  T_objectFrame
 *-------------------------------------------------------------------------*/
/**
 *  An object Frame is a part of a stance animation at one moment.
 *  A frame can have either 1 or 8 pictures for each angle.
 *
 *  @param numAngles -- how many views does this frame have.   Only
 *      1 or 8 is allowed.
 *  @param offsetPicList -- Offset in bytes from the beginning of
 *      T_objectType structure that this is in to the
 *      list of pics (T_objectPic) that make up this
 *      frame.
 *
 *<!-----------------------------------------------------------------------*/
typedef struct
{
    T_byte8 numAngles;
    T_word16 offsetPicList;
    T_word16 soundNum; /* 0 = no sound */
    T_word16 soundRadius;
    T_word16 objectAttributes;
} PACK T_objectFrame;

/*-------------------------------------------------------------------------*
 * Typedef:  T_objectPic
 *-------------------------------------------------------------------------*/
/**
 *  Picture information block for one angle of a frame.
 *
 *  @param number -- Index number of picture to use in resource file
 *      for this object.
 *  @param resource -- Resource for picture when locked in memory.
 *      NULL when not locked in memory.
 *  @param p_pic -- Pointer to picture when locked in memory.
 *      NULL when not locked in memory.
 *
 *<!-----------------------------------------------------------------------*/
typedef struct
{
    T_sword16 number;
    T_resource resource;
    T_void *p_pic;
} PACK T_objectPic;

/*-------------------------------------------------------------------------*
 * Typedef:  T_objectType
 *-------------------------------------------------------------------------*/
/**
 *  T_objectType is the description structure for an object used in the
 *  game.  It describes everything that needs to be know about this object
 *  and how it interacts with the world.   This is basically the class
 *  description for the object, and instances are made from this type.
 *
 *  @param numStances -- Number of stances that make up this object
 *  @param lockCount -- How many times this type of object has been
 *      locked into memory.  This feature is used to try
 *      to keep as much art in memory as possible.
 *
 *<!-----------------------------------------------------------------------*/
typedef struct
{
    T_word16 numStances;
    T_word32 lockCount;
    T_word16 radius;
    T_word16 attributes;
    T_word16 weight;
    T_word16 value;
    T_word32 script;
    T_word16 health;
    T_word16 objMoveAttr;
    T_objectStance stances[1];
} PACK T_objectType;


//typedef void *T_3dObject; /* Unknown here */

struct T_3dObject_;
typedef T_void *T_objTypeInstance;

/*---------------------------------------------------------------------------
 * Prototypes:
 *--------------------------------------------------------------------------*/

T_objTypeInstance
ObjTypeCreate(T_word16 objTypeNum, struct T_3dObject_ *p_obj);

T_void
ObjTypeDestroy(T_objTypeInstance objTypeInst);

E_Boolean
ObjTypeAnimate(
    T_objTypeInstance objTypeInst,
    T_word32 currentTime);

T_word16
ObjTypeGetRadius(T_objTypeInstance objTypeInst);

T_word16
ObjTypeGetAttributes(T_objTypeInstance objTypeInst);

T_void *
ObjTypeGetPicture(
    T_objTypeInstance objTypeInst,
    T_word16 angle,
    T_orientation *p_orient);

T_void
ObjTypesSetResolution(T_word16 resolution);

T_word16
ObjTypesGetResolution(T_void);

T_void
ObjTypeSetStance(T_objTypeInstance objTypeInst, T_word16 stance);

T_word16
ObjTypeGetStance(T_objTypeInstance objTypeInst);

T_word32
ObjTypeGetScript(T_objTypeInstance objTypeInst);

T_void
ObjTypeSetScript(T_objTypeInstance objTypeInst, T_word32 script);

T_word16
ObjTypeGetHealth(T_objTypeInstance objTypeInst);

T_word16
ObjTypeGetWeight(T_objTypeInstance objTypeInst);

T_word16
ObjTypeGetValue(T_objTypeInstance objTypeInst);

T_word16
ObjTypeGetMoveFlags(T_objTypeInstance objTypeInst);

T_word16
ObjTypeGetHeight(T_objTypeInstance objTypeInst);

T_word16
ObjTypeGetFrame(T_objTypeInstance objTypeInst);

T_word32
ObjTypeGetNextAnimationTime(T_objTypeInstance objTypeInst);

T_void
ObjTypeSetNextAnimationTime(
    T_objTypeInstance objTypeInst,
    T_word32 time);

T_word16
ObjTypeGetAnimData(T_objTypeInstance objTypeInst);

T_void
ObjTypeSetAnimData(T_objTypeInstance objTypeInst, T_word16 data);

#ifndef NDEBUG
T_void ObjTypePrint(FILE *fp, T_objTypeInstance objTypeInst) ;
#else
#define ObjTypePrint(fp, obj) ((T_void)0)
#endif

E_Boolean
ObjTypeIsActive(T_objTypeInstance objTypeInst);

T_void *
ObjTypeGetFrontFirstPicture(T_objTypeInstance objTypeInst);

T_void
ObjTypeDeclareSomewhatLowOnMemory(T_void);

E_Boolean
ObjTypeIsLowPiecewiseRes(T_void);

#endif

/****************************************************************************/
/*    END OF FILE:  OBJTYPE.H                                               */
/****************************************************************************/
