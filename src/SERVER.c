/*-------------------------------------------------------------------------*
 * File:  SERVER.C
 *-------------------------------------------------------------------------*/
/**
 * The Server code responds to requests from the players to perform
 * actions.  This top level code handles those requests usually passing
 * off the requests to others.
 *
 * @addtogroup SERVER
 * @brief Server/World Code
 * @see http://www.amuletsandarmor.com/AALicense.txt
 * @{
 *
 *<!-----------------------------------------------------------------------*/
#include "3D_COLLI.H"
#include "3D_TRIG.H"
#include "AREASND.H"
#include "CMDQUEUE.H"
#include "CRELOGIC.H"
#include "DOOR.H"
#include "MAP.H"
#include "MEMORY.H"
#include "MESSAGE.H"
#include "OBJECT.H"
#include "PLAYER.H"
#include "RANDOM.H"
#include "SERVER.H"
#include "SERVERSH.H"
#include "SMCCHOOS.H"
#include "SOUND.H"
#include "SOUNDS.H"
#include "STATS.H"
#include "SYNCTIME.H"
#include "TICKER.H"
#include "VIEWFILE.H"

#define MAX_PLAYERS 4
#define SYNC_TIMING (70*1)
#define SERVER_PLAYER_NONE 0xFFFF

#define SERVER_TIME_BETWEEN_OBJECT_UPDATES 7

static E_Boolean G_serverInit = FALSE ;

E_Boolean G_serverActive = FALSE ;

static T_word32 G_nextMap = 1 ;

T_sword32 G_sourceX, G_sourceY, G_sourceZ ;

static T_word32 G_serverID = 1 ;

/* Internal prototypes: */
E_Boolean IServerCheckSectorSoundsForObject(T_3dObject *p_obj, T_word32 data) ;

static T_void IServerCheckSectorSounds(T_void) ;


/*-------------------------------------------------------------------------*
 * Routine:  ServerInit
 *-------------------------------------------------------------------------*/
/**
 *  ServerInit starts up the server and gets everything in order before
 *  any other actions can occur.
 *
 *  NOTE: 
 *  Must be called before all other ServerXXX commands.
 *
 *<!-----------------------------------------------------------------------*/
T_void ServerInit(T_void)
{
    DebugRoutine("ServerInit") ;
    DebugCheck(G_serverInit == FALSE) ;

    /** Register my callback routines. **/

    DebugEnd() ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerFinish
 *-------------------------------------------------------------------------*/
/**
 *  ServerFinish cleans up the server.
 *
 *<!-----------------------------------------------------------------------*/
T_void ServerFinish(T_void)
{
    DebugRoutine("ServerFinish") ;
//    DebugCheck(G_serverInit == TRUE) ;

    G_serverInit = FALSE ;

    DebugEnd() ;
}

/* 03/22/96 */
T_void ServerPlayerLeft(T_player player)
{
    DebugRoutine("ServerPlayerLeft") ;

//    StatsSaveCharacter(StatsGetActive()) ;

    DebugEnd() ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerProjectileAdd
 *-------------------------------------------------------------------------*/
/**
 *  A project add command is used as an alternative to an
 *  OBJECT_CREATE when the new object is thrown, fired, or otherwise
 *  projected from a known origin object.
 *
 *  @param [in] objectType -- Type of object to create
 *  @param [in] initialSpeed -- Speed of object, 0=none
 *  @param [in] angle -- For initial speed and facing
 *  @param [in] x -- Starting X position on map
 *  @param [in] y -- Starting Y position on map
 *  @param [in] z -- Starting Z position on map
 *  @param [in] vz -- Velocity along the z for upward/downward velocity
 *  @param [in] target -- Aimed Target, if any (0=none)
 *  @param [in] ownerObjID -- Owner of this created object (0=none)
 *
 *  @return Created object
 *<!-----------------------------------------------------------------------*/
T_3dObject *ServerProjectileAdd(
        T_word16 objectType,
        T_byte8 initialSpeed,
        T_word16 angle,
        T_sword16 x,
        T_sword16 y,
        T_sword16 z,
        T_sword16 vz,
        T_word16 target,
        T_word16 ownerObjID)
{
    T_3dObject *p_obj;
    T_sword32 startX, startY;
    E_Boolean creationOK = TRUE;

    DebugRoutine ("ServerProjectileAdd");
//printf("SRProjectileAdd: called by %s\n", DebugGetCallerName()) ;

    /** Create a new object in my world. **/
    p_obj = ObjectCreate ();

    /** Set its type according to the packet instructions. **/
    ObjectSetType (p_obj, objectType);

    /** Set up its movement parameters. **/
    ObjectSetX16(p_obj, x) ;
    ObjectSetY16(p_obj, y) ;
    ObjectSetZ16(p_obj, z) ;
    ObjectSetAngle(p_obj, angle) ;
    ObjectSetZVel(p_obj, (((T_sword32)vz)<<16)) ;

    /* Attach an owner ID */
    ObjectSetOwnerID(p_obj, ownerObjID) ;

    startX = (x << 16) ;
    startY = (y << 16) ;

    /** Did everything go alright? **/
    if (creationOK == TRUE)
    {
        /** Yes.  Now that it's ready, add it to the world. **/
        ObjectAdd (p_obj);
        ObjectSetUpSectors(p_obj);
        ObjectSetZ16(p_obj, z) ;
        ObjectSetAngularVelocity (
            p_obj,
            angle,
            (T_word16)initialSpeed);

        /* Try setting up the target for this object. */
        if (target != 0)
            ObjectSetTarget(p_obj, target) ;
    }

    DebugEnd ();

    return p_obj ;
}


/*-------------------------------------------------------------------------*
 * Routine:  ServerShootProjectile
 *-------------------------------------------------------------------------*/
/**
 *  ServerShootProjectile is a routine that will create a projectile
 *  by setting up a projectile packet and then sending it to itself.
 *  This will cause the packet to be sent to all the clients and the object
 *  to be created (with an appropriate server id).
 *
 *  @param p_objSource -- Object doing the shooting
 *  @param angle -- Angle to shoot the projectile
 *  @param typeObj -- Object type (fireball?)
 *  @param initSpeed -- How fast.  Note this is a small value
 *  @param p_target -- Target to shoot at (or NULL for none)
 *
 *  @return Created projectile, NULL for none
 *
 *<!-----------------------------------------------------------------------*/
T_3dObject *ServerShootProjectile(
          T_3dObject *p_objSource,
          T_word16 angle,
          T_word16 typeObj,
          T_word16 initSpeed,
          T_3dObject *p_target)
{
    T_sword32 distance ;
    T_sword32 x, y ;
    T_sword16 tx, ty ;
    T_sword32 deltaHeight;
    T_sword16 heightTarget ;
    T_3dObject *p_obj ;
    T_sword32 obj_x, obj_y, obj_z, obj_vz ;
    T_word16 obj_ownerObjID;
    T_word16 obj_target;

    DebugRoutine("ServerShootProjectile") ;
    DebugCheck(p_objSource != NULL) ;
//printf("ServerShootProj: called by %s\n", DebugGetCallerName()) ;

    distance = (ObjectGetRadius(p_objSource) << 1) ;

    ObjectGetForwardPosition(
         p_objSource,
         distance,
         &x,
         &y) ;

    obj_x = (x>>16) ;
    obj_y = (y>>16) ;
    obj_z = ObjectGetZ16(p_objSource) ;
    obj_z += ((ObjectGetHeight(p_objSource)*2)/3) ;
    obj_z -= 10 ;
    obj_vz = 0 ;

    if (Collide3dObjectToXYCheckLineOfSightWithZ(
                      p_objSource,
                      obj_x,
                      obj_y,
                      obj_z) == TRUE)  {
        obj_x = ObjectGetX16(p_objSource) ;
        obj_y = ObjectGetY16(p_objSource) ;
    }

    obj_ownerObjID = ObjectGetServerId(p_objSource) ;

    if (p_target != NULL)  {
        /** Target. **/
        obj_target = ObjectGetServerId (p_target);

        /* Find where the target is located. */
        tx = ObjectGetX16(p_target) ;
        ty = ObjectGetY16(p_target) ;

        /* Calculate the distance between here and there. */
        distance = CalculateDistance(x >> 16, y >> 16, tx, ty) ;

        /* How high is the target? */
        heightTarget = ObjectGetMiddleHeight(p_target)+10 ;

        /* Calculate the steps necessary to draw a straight */
        /* line to the target. */
        if (distance > 3)
            deltaHeight =
                (((T_sword32)
                    (heightTarget - obj_z
                        /* ObjectGetZ16(p_objSource) */))<<16) / distance ;
        else
            deltaHeight = 0 ;
        deltaHeight *= ((T_sword32)initSpeed) ;

        /* Don't allow more than 45 degrees up. */
        if (deltaHeight >= 0x400000)
            deltaHeight = 0x400000 ;

        /* Don't allow more than 45 degrees down. */
        if (deltaHeight <= -0x400000)
            deltaHeight = -0x400000 ;

        obj_vz = ((T_sword16)(deltaHeight>>16)) ;

        /* If the target is invisible or translucent, randomly turn a */
        /* little to make it harder to hit. */
        if (ObjectIsStealthy(p_target))  {
            angle += (RandomValue() & 0x3FF) - 0x200 ;
        }
    } else {
    }

    /* This is a cheat, send it to ourself! */
    p_obj = ServerProjectileAdd(
                typeObj,
                (T_byte8)initSpeed,
                angle,
                obj_x,
                obj_y,
                obj_z,
                obj_vz,
                obj_target,
                obj_ownerObjID) ;

    DebugEnd() ;

    return p_obj ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerUpdate
 *-------------------------------------------------------------------------*/
/**
 *  ServerUpdate goes through all the code execution necessary to update
 *  the server actions in one time slice.
 *
 *<!-----------------------------------------------------------------------*/
T_void ServerUpdate(T_void)
{
    static T_word32 lastCount = 0 ;
    T_word32 delta;
    T_word16 lastbit = 0;
    T_word32 time ;
    TICKER_TIME_ROUTINE_PREPARE() ;

    TICKER_TIME_ROUTINE_START() ;
    DebugRoutine("ServerUpdate") ;
    INDICATOR_LIGHT(38, INDICATOR_GREEN) ;

    /* Don't do any actions if the server is in a state where the level */
    /* is not ready. */
    if (G_serverActive == TRUE)  {
        /* Server will now do one iteration of whatever needs to be done */
        /* by the server. */

        /** Keep track of the deltas. **/
        if (lastCount == 0)
        {
           lastCount = SyncTimeGet ();
        }
        else
        {
           time = SyncTimeGet() ;
           delta = time - lastCount + lastbit;
           lastbit = delta & 1 ;
           delta = delta + (delta >> 1) ;
           lastCount = time ;

           /** Apply all velocities to the objects. **/
           ObjectsUpdateMovement (delta);
        }

        /* See if any objects have caused sector sounds and send them. */
        IServerCheckSectorSounds() ;

        /** Then, send out all necessary movement packets. **/
//        CreaturesUpdate() ;
//        ObjectGeneratorUpdate() ;

        /* Check for objects that need destroying. */
        ServerDestroyDestroyObjects() ;
    }

    DebugEnd() ;
    INDICATOR_LIGHT(38, INDICATOR_RED) ;
    TICKER_TIME_ROUTINE_END(stdout, "ServerUpdate", 500) ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerGetNextPlayer
 *-------------------------------------------------------------------------*/
/**
 *  ServerGetNextPlayer is used to traverse through all the players in
 *  the game.  Use -1 to get the first player and then continue.  When a
 *  -1 is returned, you know that you are at the end of the list.
 *
 *  @param lastPlayer -- Player to start from or -1 for first
 *
 *  @return next player.
 *
 *<!-----------------------------------------------------------------------*/
T_sword16 ServerGetNextPlayer(T_player lastPlayer)
{
    T_player nextPlayer ;
    T_3dObject *p_obj ;

    DebugRoutine("ServerGetNextPlayer") ;

/*
printf("\nlastplayer: %d\n", lastPlayer) ;

for (p_obj=ObjectsGetFirst(); p_obj!=NULL; p_obj=ObjectGetNext(p_obj))
  if (ObjectIsPlayer(p_obj))
    printf("p_obj: %p (%d %d) attr: %04X\n", p_obj, ObjectGetServerId(p_obj), ObjectGetType(p_obj), ObjectGetAttributes(p_obj)) ;
*/

    if (lastPlayer == PLAYER_BAD)  {
        /* Start at the beginning of the list. */
        p_obj = ObjectsGetFirst() ;
    } else {
        /* Find where we were. */
        p_obj = ObjectFind(lastPlayer) ;
        /* Skip immediately to the next one. */
        if (p_obj != NULL)
            p_obj = ObjectGetNext(p_obj) ;

    }

    /* Loop until we find a player. */
    while (p_obj != NULL)  {
//if (ObjectIsPiecewise(p_obj))  {
//    printf("piecewise p_obj: %p (%d %d) attr: %04X\n", p_obj, ObjectGetServerId(p_obj), ObjectGetType(p_obj), ObjectGetAttributes(p_obj)) ;
//}
//if (ObjectIsBodyPart(p_obj))  {
//    printf("bodyPart p_obj: %p (%d %d) attr: %04X\n", p_obj, ObjectGetServerId(p_obj), ObjectGetType(p_obj), ObjectGetAttributes(p_obj)) ;
//}
        if ((ObjectIsPlayerHead(p_obj)) && (ObjectGetStance(p_obj) != STANCE_DIE))  {
//puts("found") ;
            break ;
        }

        p_obj = ObjectGetNext(p_obj) ;
    }
/*
if (p_obj != NULL)  {
   printf("found: ") ;
    printf("p_obj: %p (%d %d) attr: %04X\n", p_obj, ObjectGetServerId(p_obj), ObjectGetType(p_obj), ObjectGetAttributes(p_obj)) ;
}
*/
    /* Get the id of this player (if there is one). */
    if (p_obj == NULL)
        nextPlayer = PLAYER_BAD ;
    else
        nextPlayer = ObjectGetServerId(p_obj) ;

/*
if (nextPlayer == 0)  {
    printf("\n\nfakemode: %s\n", PlayerInFakeMode()?"FAKE":"REAL") ;
    printf("%s player.\n", ObjectIsPlayer(p_obj)?"IS":"IS NOT") ;
    ObjectPrint(stdout, p_obj) ;
    fflush(stdout) ;
}
*/

    DebugCheck(nextPlayer != 0) ;
    DebugEnd() ;

    return nextPlayer ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerGetPlayerObject
 *-------------------------------------------------------------------------*/
/**
 *  ServerGetPlayerObject finds a player and returns its object.  If none
 *  is found, NULL is returned.
 *
 *  @param playerId -- Player's Id
 *
 *  @return Player's object, or NULL if not found
 *
 *<!-----------------------------------------------------------------------*/
T_3dObject *ServerGetPlayerObject(T_word16 playerId)
{
    T_3dObject *p_obj ;

    DebugRoutine("ServerGetPlayerObject") ;

    /** Just always return the object pointer. **/
//    p_obj = players[playerId].p_obj ;
    p_obj = ObjectFind(playerId) ;

    DebugEnd() ;

    return p_obj ;
}

/*-------------------------------------------------------------------------*
 * Routine:  IServerCheckSectorSounds
 *-------------------------------------------------------------------------*/
/**
 *  IServerCheckSectorSounds sees if all the objects have caused any
 *  sounds to occur.
 *
 *  NOTE: 
 *  This routine is to be immediately called after the ObjectsUpdateMove
 *  routine is called.  Don't send packets until this routine is called.
 *  The IsMoved flag on the objects is used to determine if a check is
 *  needed or not.
 *
 *<!-----------------------------------------------------------------------*/
static T_void IServerCheckSectorSounds(T_void)
{
    DebugRoutine("IServerCheckSectorSounds") ;

    /* Check all the objects.  Pass 0 since we don't have any */
    /* other data to pass. */
    ObjectsDoToAll(IServerCheckSectorSoundsForObject, 0) ;

    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  IServerCheckSectorSoundsForObject
 *-------------------------------------------------------------------------*/
/**
 *  IServerCheckSectorSoundsForObject sees if an object is on a group
 *  of sectors with the same "entry sound."  If it is, the sound is produced
 *  given that the object has not recently created the sound.  This is
 *  used for making splashing sounds.
 *
 *  @param p_obj -- Player object to identify
 *  @param data -- This is currently always zero
 *
 *<!-----------------------------------------------------------------------*/
E_Boolean IServerCheckSectorSoundsForObject(T_3dObject *p_obj, T_word32 data)
{
    T_word16 i, sound1=0, sound2 ;
    T_word16 sector ;

    DebugRoutine("IServerCheckSectorSoundsForObject") ;
    DebugCheck(p_obj != NULL) ;

    /* Only bother with objects that have moved. */
    if (!ObjectIsBodyPart(p_obj))  {
        /* Also, only bother with objects that are not flying. */
//        if (ObjectIsAboveGround(p_obj) == FALSE)  {
        if (ObjectGetZ16(p_obj) < MapGetFloorHeight(ObjectGetAreaSector(p_obj)))  {
            /* See if there are sounds on each of the sectors. */
            sector = ObjectGetNthAreaSector(p_obj, 0) ;
            sound1 = View3dGetSectorEnterSound(sector) ;

            /* If this sound is zero, there is no need to go on. */
            if (sound1)  {
                /* Loop through all the other sectors and see if they */
                /* are the same sound.  Otherwise, stop immediately. */
                for (i=1; i<ObjectGetNumAreaSectors(p_obj); i++)  {
                    sound2 = View3dGetSectorEnterSound(
                                 ObjectGetNthAreaSector(p_obj, i)) ;
                    if (sound2 != sound1)  {
                        sound1 = 0 ;
                        break ;
                    }
                }

                /* Are we in agreement? (sound1 will be non-zero if we are) */
                if (sound1)  {
                    /* Is this sound non-zero and different than last time? */
                    if ((sound1) && (ObjectGetLastSound(p_obj) != sound1))  {
                        /* Play the sound at this location. */
                        AreaSoundCreate(
                            ObjectGetX16(p_obj),
                            ObjectGetY16(p_obj),
                            View3dGetSectorEnterSoundRadius(sector),
                            180,
                            AREA_SOUND_TYPE_ONCE,
                            0,
                            AREA_SOUND_BAD,
                            NULL,
                            0,
                            sound1) ;

                    }
                }
            }
            /* Note that this is the last sound (even if zero). */
            ObjectSetLastSound(p_obj, sound1) ;
        } else {
            ObjectSetLastSound(p_obj, 0) ;
        }
    }

    DebugEnd() ;

    return FALSE;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerGotoPlace
 *-------------------------------------------------------------------------*/
/**
 *  ServerGotoPlace is called to force all logged-in clients to go to
 *  a map or form.  It has the same effect as receiving a goto-place packet
 *  from a client.
 *  New behavior (AMT, 9/13/95)  Place numbers fit into the following
 *  groups, specified by their high 2 bytes.
 *  0000 - 00FF   Levels.  This allows for an absurd number of levels.
 *  Whenever a master receives one, a new slave server
 *  is forked off and begins an adventure.
 *  Whenever a slave receives one, it just advances to that
 *  level.
 *  0100 - 01FF   Forms.  Mail rooms, inns, etc.  Whenever one of these
 *  is received by a slave, it terminates and returns
 *  control to the master.  Masters handle normally.
 *  0200          Currently-running adventures.  I.e. currently active
 *  slaves which the client wishes to join.
 *  Slave receives and ignores.  Master receives and tells
 *  the appropriate slave.
 *
 *  @param placeNumber -- the place to go to.
 *  @param startLocation -- Numerical start location.
 *
 *<!-----------------------------------------------------------------------*/
T_void ServerGotoPlace(T_word32 placeNumber, T_word16 startLocation)
{
//    T_packetShort packet ;
//    T_gotoPlacePacket *p_packet ;
//
//    DebugRoutine("ServerGotoPlace") ;
//
////printf("ServerGotoPlace: %ld %d\n", placeNumber, startLocation) ; fflush(stdout) ;
//
//    /** Normal mode. **/
//    /** Is this a next-map instruction? **/
////printf("Server attempting to go to %d\n", placeNumber) ;
//
////printf("Server going to %d (was %d)\n", placeNumber, G_nextMap) ;
//    G_nextMap = placeNumber ;
//
//    /* Mark updates for the current level as inactive. */
//    G_serverActive = FALSE ;
//
//    /** Create a GOTO_PLACE packet. **/
//    packet.header.packetLength = SHORT_PACKET_LENGTH;
//    p_packet = (T_gotoPlacePacket *)packet.data;
//
//    p_packet->command = PACKET_COMMANDCSC_GOTO_PLACE;
//    p_packet->placeNumber = placeNumber;
//    p_packet->startLocation = startLocation ;
////printf("start location set to %d\n", startLocation) ;
//
//    /** Just send an identical packet to each client. **/
//    CmdQSendShortPacket(&packet, 140, 0, NULL) ;
//
//    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  ServerReceiveGotoPlacePacket
 *-------------------------------------------------------------------------*/
/**
 *  ServerReceiveGotoPlacePacket is called when a client reqests a map
 *  or form change.  It basically loads in the new map and sends a copy
 *  of the request to all the other clients.
 *
 *  @param p_gotoPacket -- the goto place packet.
 *
 *<!-----------------------------------------------------------------------*/
T_void ServerReceiveGotoPlacePacket(T_packetEitherShortOrLong *p_gotoPacket)
{
//    T_gotoPlacePacket *p_packet ;
//
//    DebugRoutine("ServerReceiveGotoPlacePacket") ;
////puts("ServerReceiveGotoPlacePacket") ;
//
//    /* Get a quick pointer. */
//    p_packet = (T_gotoPlacePacket *)p_gotoPacket->data ;
//
//    ServerGotoPlace(p_packet->placeNumber, p_packet->startLocation);
//
//    if (p_packet->placeNumber == 0)
//        ServerPlayerLeft(CmdQGetActivePortNum()) ;
//
//    DebugEnd() ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerGetServerID
 *-------------------------------------------------------------------------*/
/**
 *  ServerGetServerID returns the server id of this server.
 *
 *  @return Server id
 *
 *<!-----------------------------------------------------------------------*/
T_word32 ServerGetServerID(T_void)
{
    return G_serverID ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerSetServerID
 *-------------------------------------------------------------------------*/
/**
 *  ServerSetServerID sets    the server id of this server.
 *
 *  @param newID -- Server id
 *
 *<!-----------------------------------------------------------------------*/
T_void ServerSetServerID(T_word32 newID)
{
    G_serverID = newID ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerReceiveRequestCharacterListPacket
 *-------------------------------------------------------------------------*/
/**
 *  ServerReceiveRequestCharacterListPacket is called when the client
 *  wishes to know what characters are available for this account.
 *
 *  NOTE:
 *  This code is intended for the Self Server mode ONLY.  It just
 *  makes a call to StatsGetSavedCharacterList and starts a memory transfer.
 *
 *  @param p_packet -- request enter packet
 *
 *<!-----------------------------------------------------------------------*/
T_void ServerReceiveRequestCharacterListPacket(
           T_packetEitherShortOrLong *p_packet)
{
    T_statsSavedCharArray *p_charArray ;

    DebugRoutine("ServerReceiveRequestCharacterListPacket") ;

    p_charArray = StatsGetSavedCharacterList() ;

    StatsSetSavedCharacterList(p_charArray) ;

    SMCChooseSetFlag(
        SMCCHOOSE_FLAG_ENTER_COMPLETE,
        TRUE) ;

    DebugEnd() ;
}

T_void ServerReceiveSyncPacket(T_packetEitherShortOrLong *p_packet)
{
    DebugRoutine("ServerReceiveSyncPacket") ;

    /* For the self server, just send it back. */
//    ServerSendToAll(p_packet) ;
    CmdQSendPacket(p_packet, &p_packet->header.sender, 140, 0, NULL) ;

    DebugEnd() ;
}

/* LES: 05/15/96 -- Create an object anywhere in the world. */
T_3dObject * ServerCreateObjectGlobal(
           T_word16 objType,
           T_sword16 x,
           T_sword16 y,
           T_sword16 z)
{
    T_3dObject *p_obj ;

    DebugRoutine("ServerCreateObjectGlobal") ;

    p_obj = ObjectCreate() ;
    if (p_obj)  {
        ObjectDeclareStatic(
            p_obj,
            x,
            y) ;
        ObjectSetType(p_obj, objType) ;
        ObjectSetAngle(p_obj, 0) ;
        ObjectSetUpSectors(p_obj) ;
        ObjectForceUpdate(p_obj) ;
        ObjectSetZ16(p_obj, z) ;

        ObjectAdd(p_obj) ;
    } else {
        DebugCheck(FALSE) ;
    }

    DebugEnd() ;

    return p_obj ;
}

/* LES: 05/15/96 -- Create an object anywhere in the world. */
T_3dObject * ServerCreateFakeObjectGlobal(
           T_word16 objType,
           T_sword16 x,
           T_sword16 y,
           T_sword16 z)
{
    T_3dObject *p_obj ;

    DebugRoutine("ServerCreateFakeObjectGlobal") ;

    p_obj = ObjectCreateFake() ;
    if (p_obj)  {
        ObjectDeclareStatic(
            p_obj,
            x,
            y) ;
        ObjectSetType(p_obj, objType) ;
        ObjectSetAngle(p_obj, 0) ;
        ObjectSetUpSectors(p_obj) ;
        ObjectForceUpdate(p_obj) ;
        ObjectSetZ16(p_obj, z) ;

//        ObjectAdd(p_obj) ;
    } else {
        DebugCheck(FALSE) ;
    }

    DebugEnd() ;

    return p_obj ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerShootBasicProjectile
 *-------------------------------------------------------------------------*/
/**
 *  ServerShootBasicProjectile is called to create an object at a
 *  specific location and with specific velocities.
 *
 *  @param objectType -- Type of object projectile
 *  @param x -- X Position to start projectile
 *  @param y -- Y Position to start projectile
 *  @param z -- Z Position to start projectile
 *  @param targetX -- X Target position for missile
 *  @param targetY -- Y Target position for missile
 *  @param targetZ -- Z Target position for missile
 *  @param initialSpeed -- How fast to shoot the missile
 *
 *<!-----------------------------------------------------------------------*/
T_void ServerShootBasicProjectile(
           T_word16 objectType,
           T_sword32 x,
           T_sword32 y,
           T_sword32 z,
           T_sword32 targetX,
           T_sword32 targetY,
           T_sword32 targetZ,
           T_word16 initialSpeed)
{
    T_sword16 tx, ty ;
    T_sword32 deltaHeight;
    T_sword16 heightTarget ;
    T_word16 angle ;
    T_word16 distance ;
    T_sword32 obj_x, obj_y, obj_z, obj_vz ;
    T_word16 obj_ownerObjID;
    T_word16 obj_target;

    DebugRoutine("ServerShootBasicProjectile") ;

    obj_x = (x>>16) ;
    obj_y = (y>>16) ;
    obj_z = (z>>16) ;

    angle = MathArcTangent(
                (T_sword16)((targetX-x)>>16),
                (T_sword16)((targetY-y)>>16)) ;
    tx = (targetX >> 16) ;
    ty = (targetY >> 16) ;

    obj_ownerObjID = 0 ;
    obj_target = 0 ;

    /* Find where the target is located. */
    tx = (targetX>>16) ;
    ty = (targetY>>16) ;

    /* Calculate the distance between here and there. */
    distance = CalculateDistance(x >> 16, y >> 16, tx, ty) ;

    /* How high is the target? */
    heightTarget = (targetZ >> 16) ;

    /* Calculate the steps necessary to draw a straight */
    /* line to the target. */
    if (distance != 0)
        deltaHeight =
            (((T_sword32)
                (heightTarget - obj_z))<<16) / distance ;
    else
        deltaHeight = 0 ;
    deltaHeight *= 40 ;

    /* Don't allow more than 45 degrees up. */
    if (deltaHeight >= 0x320000)
        deltaHeight = 0x320000 ;

    /* Don't allow more than 45 degrees down. */
    if (deltaHeight <= -0x320000)
        deltaHeight = -0x320000 ;

    obj_vz = ((T_sword16)(deltaHeight>>16)) ;

    /* This is a cheat, send it to ourself! */
    ServerProjectileAdd(objectType, (T_byte8)initialSpeed, angle, obj_x, obj_y, obj_z, obj_vz,
            obj_target, obj_ownerObjID);

    DebugEnd() ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerDestroyDestroyObjects
 *-------------------------------------------------------------------------*/
/**
 *  ServerDestroyDestroyObjects goes through the list of objects and
 *  checks if there any objects that are requesting self-destruction.  If
 *  there is, an appropriate packet is sent and the object is eliminated.
 *
 *<!-----------------------------------------------------------------------*/
T_void ServerDestroyDestroyObjects(T_void)
{
    T_3dObject *p_obj ;
    T_3dObject *p_objNext ;

    DebugRoutine("ServerDestroyDestroyObjects") ;

    p_obj = ObjectsGetFirst() ;
    while ((ObjectsGetNumMarkedForDestroy() != 0) &&
           (p_obj != NULL))  {
        p_objNext = ObjectGetNext(p_obj) ;

        if (ObjectIsMarkedForDestroy(p_obj))  {
            ObjectRemove(p_obj) ;
            ObjectDestroy(p_obj) ;
        }

        p_obj = p_objNext ;
    }

    DebugEnd() ;
}

/*-------------------------------------------------------------------------*
 * Routine:  ServerDamageAtWithType
 *-------------------------------------------------------------------------*/
/**
 *  ServerDamageAtWithType does damage to a given location.
 *  If the given radius is nonzero, does damage to anything within the
 *  radius.
 *
 *  @param x -- X Exact location
 *  @param y -- Y Exact location
 *  @param z -- Z Exact location
 *  @param radius -- Sphereical radius
 *  @param damage -- Amount of damage to do
 *  @param ownerID -- Owner of damage being done
 *  @param type -- Type of damage to do
 *
 *<!-----------------------------------------------------------------------*/
T_void ServerDamageAtWithType(
           T_sword16 x,
           T_sword16 y,
           T_sword16 z,
           T_word16 radius,
           T_word16 damage,
           T_word16 ownerID,
           T_byte8 type)
{
    T_damageObjInfo damageInfo ;
    T_word16 locked ;

    DebugRoutine("ServerDamageAtWithType") ;

    G_sourceX = (((T_sword32)x)<<16) ;
    G_sourceY = (((T_sword32)y)<<16) ;
    G_sourceZ = (((T_sword32)z)<<16) ;

    damageInfo.ownerID = ownerID ;
    damageInfo.damage = damage ;
    damageInfo.type = type ;

    if (type == (EFFECT_DAMAGE_SPECIAL | EFFECT_DAMAGE_SPECIAL_LOCK)) {
        locked = ServerLockDoorsInArea(
                     G_sourceX,
                     G_sourceY,
                     40) ;
        if (ownerID == ObjectGetServerId(PlayerGetObject()))  {
            if ((locked) && (locked != 0x100))  {
                MessageAdd("Lock on door is increased") ;
            } else {
                MessageAdd("Cannot lock") ;
            }
        }
    } else if (type == (EFFECT_DAMAGE_SPECIAL | EFFECT_DAMAGE_SPECIAL_UNLOCK))  {
        locked = ServerUnlockDoorsInArea(
            G_sourceX,
            G_sourceY,
            40) ;
        if (ownerID == ObjectGetServerId(PlayerGetObject()))  {
            if ((locked == 0x100) || (locked == DOOR_ABSOLUTE_LOCK))  {
                MessageAdd("Cannot unlock") ;
            } else if ((locked) &&
                (locked != DOOR_ABSOLUTE_LOCK) &&
                (locked != 0x100))  {
                MessageAdd("Lock on door is decreased") ;
            } else if (locked == 0)  {
                SoundDing() ;
                MessageAdd("Door is now unlocked") ;
            }
        }
    } else {
        ObjectsDoToAllAtXYZRadius(
            x,
            y,
            z,
            radius,
            ServerDamageObjectXYZ,
            (T_word32)(&damageInfo));
    }

    DebugEnd() ;
}

/** @} */
/*-------------------------------------------------------------------------*
 * End of File:  SERVER.C
 *-------------------------------------------------------------------------*/
