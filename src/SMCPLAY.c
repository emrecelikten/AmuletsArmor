/*-------------------------------------------------------------------------*
 * File:  SMCPLAY.C
 *-------------------------------------------------------------------------*/
/**
 * Playing a game in a simple state machine.
 * @addtogroup SMCPLAY
 * @brief Playing Game State Machine
 * @see http://www.amuletsandarmor.com/AALicense.txt
 * @{
 *
 *<!-----------------------------------------------------------------------*/
#include "CLIENT.H"
#include "CMDQUEUE.H"
#include "CONTROL.H"
#include "CRELOGIC.H"
#include "CSYNCPCK.H"
#include "MEMORY.H"
#include "SMCPLAY.H"
#include "SMMAIN.H"
#include "TICKER.H"
#include "UPDATE.H"
#include "VIEW.H"

//#undef DebugRoutine
//#define DebugRoutine(name) { puts(name) ; DebugAddRoutine(name, __FILE__, __LINE__) ; }
typedef struct {
    E_Boolean stateFlags[NUMBER_SMCPLAY_GAME_FLAGS] ;
} T_SMCPlayGameData ;

/* Internal prototypes: */
T_void SMCPlayGameDataInit(T_stateMachineHandle handle) ;

T_void SMCPlayGameDataFinish(T_stateMachineHandle handle) ;

T_void SMCPlayGameWaitForGoToEnter(
           T_stateMachineHandle handle,
           T_word32 extraData) ;

T_void SMCPlayGameGoToPlaceEnter(
           T_stateMachineHandle handle,
           T_word32 extraData) ;

T_void SMCPlayGameTimedOutEnter(
           T_stateMachineHandle handle,
           T_word32 extraData) ;

T_void SMCPlayGameEndGameEnter(
           T_stateMachineHandle handle,
           T_word32 extraData) ;

T_void SMCPlayGameDoGameEnter(
           T_stateMachineHandle handle,
           T_word32 extraData) ;

T_void SMCPlayGameDoGameIdle(
           T_stateMachineHandle handle,
           T_word32 extraData) ;

T_void SMCPlayGameDoGameExit(
           T_stateMachineHandle handle,
           T_word32 extraData,
           E_Boolean isDestroyed) ;

/* Internal global variables: */
static T_stateMachineHandle G_smHandle ;
static E_Boolean G_init = FALSE ;


static T_stateMachineConditional SMCPlayGameWaitForGoToCond[] = {
    {
        SMCPlayGameCheckFlag,                         /* conditional callback */
        SMCPLAY_GAME_FLAG_GOTO_RECEIVED,              /* extra data */
        SMCPLAY_GAME_GO_TO_PLACE_STATE                /* next state */
    },
    {
        SMCPlayGameCheckFlag,                         /* conditional callback */
        SMCPLAY_GAME_FLAG_TIMED_OUT,                  /* extra data */
        SMCPLAY_GAME_TIMED_OUT_STATE                  /* next state */
    },
    {
        SMCPlayGameCheckFlag,                         /* conditional callback */
        SMCPLAY_GAME_FLAG_END_GAME,                   /* extra data */
        SMCPLAY_GAME_END_GAME_STATE                   /* next state */
    }
} ;
/*-------------------------------------------------------------------------*/
static T_stateMachineConditional SMCPlayGameGoToPlaceCond[] = {
    {
        SMCPlayGameCheckFlag,                         /* conditional callback */
        SMCPLAY_GAME_FLAG_TIMED_OUT,                  /* extra data */
        SMCPLAY_GAME_TIMED_OUT_STATE                  /* next state */
    },
    {
        SMCPlayGameCheckFlag,                         /* conditional callback */
        SMCPLAY_GAME_FLAG_START_GAME,                 /* extra data */
        SMCPLAY_GAME_DO_GAME_STATE                    /* next state */
    }
} ;
/*-------------------------------------------------------------------------*/
static T_stateMachineConditional SMCPlayGameDoGameCond[] = {
    {
        SMCPlayGameCheckFlag,                         /* conditional callback */
        SMCPLAY_GAME_FLAG_TIMED_OUT,                  /* extra data */
        SMCPLAY_GAME_TIMED_OUT_STATE                  /* next state */
    },
    {
        SMCPlayGameCheckFlag,                         /* conditional callback */
        SMCPLAY_GAME_FLAG_LEAVE_PLACE,                /* extra data */
        SMCPLAY_GAME_WAIT_FOR_GO_TO_STATE             /* next state */
    }
} ;
/*-------------------------------------------------------------------------*/


static T_stateMachineState SMCPlayGameStates[] = {
    /* SMCPLAY_GAME_WAIT_FOR_GO_TO_STATE */
    {
        SMCPlayGameWaitForGoToEnter,            /* Enter callback */
        NULL,                                   /* Exit callback */
        NULL,                                   /* Idle callback */
        0,                                      /* Extra data */
        3,                                      /* Num conditionals */
        SMCPlayGameWaitForGoToCond              /* conditinal list. */
    },

    /* SMCPLAY_GAME_GO_TO_PLACE_STATE */
    {
        SMCPlayGameGoToPlaceEnter,              /* Enter callback */
        NULL,                                   /* Exit callback */
        NULL,                                   /* Idle callback */
        0,                                      /* Extra data */
        2,                                      /* Num conditionals */
        SMCPlayGameGoToPlaceCond                /* conditinal list. */
    },

    /* SMCPLAY_GAME_DO_GAME_STATE */
    {
        SMCPlayGameDoGameEnter,                 /* Enter callback */
        SMCPlayGameDoGameExit,                  /* Exit callback */
        SMCPlayGameDoGameIdle,                  /* Idle callback */
        0,                                      /* Extra data */
        2,                                      /* Num conditionals */
        SMCPlayGameDoGameCond                   /* conditinal list. */
    },

    /* SMCPLAY_GAME_TIMED_OUT_STATE */
    {
        SMCPlayGameTimedOutEnter,               /* Enter callback */
        NULL,                                   /* Exit callback */
        NULL,                                   /* Idle callback */
        0,                                      /* Extra data */
        0,                                      /* Num conditionals */
        NULL                                    /* conditinal list. */
    },

    /* SMCPLAY_GAME_END_GAME_STATE */
    {
        SMCPlayGameEndGameEnter,                /* Enter callback */
        NULL,                                   /* Exit callback */
        NULL,                                   /* Idle callback */
        0,                                      /* Extra data */
        0,                                      /* Num conditionals */
        NULL                                    /* conditinal list. */
    }
} ;

/***************************************************************************/
/*                             STATE MACHINE                               */
/***************************************************************************/
static T_stateMachine SMCPlayGameStateMachine = {
    STATE_MACHINE_TAG,              /* Tag to identify the structure. */
    SMCPlayGameDataInit,        /* Init. callback */
    SMCPlayGameDataFinish,      /* Finish callback */
    NUMBER_SMCPLAY_GAME_STATES,   /* Number states */
    SMCPlayGameStates           /* State list */
} ;



/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameInitialize
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameInitialize
 *
 *  @return Handle to state machine created
 *
 *<!-----------------------------------------------------------------------*/
T_stateMachineHandle SMCPlayGameInitialize(T_void)
{
    DebugRoutine("SMCPlayGameInitialize") ;

    DebugCheck(G_init == FALSE) ;

    G_init = TRUE ;

    KeyboardPushEventHandler(ClientHandleKeyboard) ;
    ViewSetPalette(VIEW_PALETTE_STANDARD) ;

    G_smHandle = StateMachineCreate(&SMCPlayGameStateMachine) ;
    StateMachineGotoState(G_smHandle, SMCPLAY_GAME_INITIAL_STATE) ;

    UpdateStart3dView() ;

    DebugEnd() ;

    return G_smHandle ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameFinish
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameFinish
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameFinish(T_void)
{
    DebugRoutine("SMCPlayGameFinish") ;

    DebugCheck(G_init == TRUE) ;

    /* Destroy the state machine. */
    StateMachineDestroy(G_smHandle) ;
    G_smHandle = STATE_MACHINE_HANDLE_BAD ;
    KeyboardPopEventHandler() ;

    UpdateEnd3dView() ;

    G_init = FALSE ;

    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameUpdate
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameUpdate
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameUpdate(T_void)
{
    DebugRoutine("SMCPlayGameUpdate") ;

    StateMachineUpdate(G_smHandle) ;

    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameDataInit
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameDataInit
 *
 *  @param handle -- Handle to state machine
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameDataInit(T_stateMachineHandle handle)
{
    T_SMCPlayGameData *p_data ;

    DebugRoutine("SMCPlayGameDataInit") ;

    p_data = MemAlloc(sizeof(T_SMCPlayGameData)) ;

    DebugCheck(p_data != NULL) ;
    memset(p_data, 0, sizeof(T_SMCPlayGameData)) ;

    StateMachineSetExtraData(handle, p_data) ;

    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameDataFinish
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameDataFinish
 *
 *  @param handle -- Handle to state machine
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameDataFinish(T_stateMachineHandle handle)
{
    T_SMCPlayGameData *p_data ;

    DebugRoutine("SMCPlayGameDataFinish") ;

    /* Destroy the extra data attached to the state machine. */
    p_data = (T_SMCPlayGameData *)StateMachineGetExtraData(G_smHandle) ;

    DebugCheck(p_data != NULL) ;
    MemFree(p_data) ;
    StateMachineSetExtraData(handle, NULL) ;

    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameCheckFlag
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameCheckFlag
 *
 *  @param handle -- Handle to state machine
 *  @param flag -- Flag to change
 *
 *<!-----------------------------------------------------------------------*/
E_Boolean SMCPlayGameCheckFlag(
              T_stateMachineHandle handle,
              T_word32 flag)
{
    E_Boolean stateFlag = FALSE ;        /* Return status will default */
                                         /* to FALSE. */
    T_SMCPlayGameData *p_data ;

    DebugRoutine("SMCPlayGameCheckFlag") ;
    DebugCheck(G_smHandle != STATE_MACHINE_HANDLE_BAD) ;

    p_data = (T_SMCPlayGameData *)StateMachineGetExtraData(G_smHandle) ;
    DebugCheck(p_data != NULL) ;

    /* If a valid flag, get the state */
    DebugCheck(flag < SMCPLAY_GAME_FLAG_UNKNOWN) ;
    if (flag < SMCPLAY_GAME_FLAG_UNKNOWN)
        stateFlag = p_data->stateFlags[flag] ;

    DebugEnd() ;
    return stateFlag ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameSetFlag
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameSetFlag
 *
 *  @param flag -- Flag to change
 *  @param state -- New state of flag
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameSetFlag(
           T_word16 flag,
           E_Boolean state)
{
    T_SMCPlayGameData *p_data ;

    DebugRoutine("SMCPlayGameSetFlag") ;

    if (G_smHandle != STATE_MACHINE_HANDLE_BAD)  {
        p_data = (T_SMCPlayGameData *)StateMachineGetExtraData(G_smHandle) ;
        DebugCheck(p_data != NULL) ;

        /* If a valid index, set to the new state */
        DebugCheck(flag < SMCPLAY_GAME_FLAG_UNKNOWN) ;
        if (flag < SMCPLAY_GAME_FLAG_UNKNOWN)
            p_data->stateFlags[flag] = state ;
    }

    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameWaitForGoToEnter
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameWaitForGoToEnter
 *
 *  @param handle -- Handle to state machine
 *  @param extraData -- Not used
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameWaitForGoToEnter(
           T_stateMachineHandle handle,
           T_word32 extraData)
{
    T_SMCPlayGameData *p_data ;

    DebugRoutine("SMCPlayGameWaitForGoToEnter") ;

    p_data = (T_SMCPlayGameData *)StateMachineGetExtraData(G_smHandle) ;
    DebugCheck(p_data != NULL) ;

    SMCPlayGameSetFlag(
        SMCPLAY_GAME_FLAG_GOTO_RECEIVED,
        FALSE) ;
    SMCPlayGameSetFlag(
        SMCPLAY_GAME_FLAG_TIMED_OUT,
        FALSE) ;
    SMCPlayGameSetFlag(
        SMCPLAY_GAME_FLAG_END_GAME,
        FALSE) ;

/* TESTING */
SMCPlayGameSetFlag(
    SMCPLAY_GAME_FLAG_GOTO_RECEIVED,
    TRUE) ;

    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameGoToPlaceEnter
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameGoToPlaceEnter
 *
 *  @param handle -- Handle to state machine
 *  @param extraData -- Not used
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameGoToPlaceEnter(
           T_stateMachineHandle handle,
           T_word32 extraData)
{
    T_SMCPlayGameData *p_data ;

    DebugRoutine("SMCPlayGameGoToPlaceEnter") ;

    p_data = (T_SMCPlayGameData *)StateMachineGetExtraData(G_smHandle) ;
    DebugCheck(p_data != NULL) ;

    SMCPlayGameSetFlag(
        SMCPLAY_GAME_FLAG_TIMED_OUT,
        FALSE) ;
    SMCPlayGameSetFlag(
        SMCPLAY_GAME_FLAG_START_GAME,
        FALSE) ;

/* TESTING */
SMCPlayGameSetFlag(
    SMCPLAY_GAME_FLAG_START_GAME,
    TRUE) ;

    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameTimedOutEnter
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameTimedOutEnter
 *
 *  @param handle -- Handle to state machine
 *  @param extraData -- Not used
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameTimedOutEnter(
           T_stateMachineHandle handle,
           T_word32 extraData)
{
    T_SMCPlayGameData *p_data ;

    DebugRoutine("SMCPlayGameTimedOutEnter") ;

    p_data = (T_SMCPlayGameData *)StateMachineGetExtraData(G_smHandle) ;
    DebugCheck(p_data != NULL) ;

    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameEndGameEnter
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameEndGameEnter
 *
 *  @param handle -- Handle to state machine
 *  @param extraData -- Not used
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameEndGameEnter(
           T_stateMachineHandle handle,
           T_word32 extraData)
{
    T_SMCPlayGameData *p_data ;

    DebugRoutine("SMCPlayGameEndGameEnter") ;

    p_data = (T_SMCPlayGameData *)StateMachineGetExtraData(G_smHandle) ;
    DebugCheck(p_data != NULL) ;

    DebugEnd() ;
}

/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameDoGameEnter
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameDoGameEnter
 *
 *  @param handle -- Handle to state machine
 *  @param extraData -- Not used
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameDoGameEnter(
           T_stateMachineHandle handle,
           T_word32 extraData)
{
    T_SMCPlayGameData *p_data ;

    DebugRoutine("SMCPlayGameDoGameEnter") ;

    p_data = (T_SMCPlayGameData *)StateMachineGetExtraData(G_smHandle) ;
    DebugCheck(p_data != NULL) ;

    SMCPlayGameSetFlag(
        SMCPLAY_GAME_FLAG_TIMED_OUT,
        FALSE) ;
    SMCPlayGameSetFlag(
        SMCPLAY_GAME_FLAG_LEAVE_PLACE,
        FALSE) ;

    /* Set up the 3d view. */
    ControlInitForGamePlay() ;

    DebugEnd() ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameDoGameIdle
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameDoGameIdle
 *
 *  @param handle -- Handle to state machine
 *  @param extraData -- Not used
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameDoGameIdle(
           T_stateMachineHandle handle,
           T_word32 extraData)
{
    T_SMCPlayGameData *p_data ;
    TICKER_TIME_ROUTINE_PREPARE() ;

    TICKER_TIME_ROUTINE_START() ;
    DebugRoutine("SMCPlayGameDoGameIdle") ;

    p_data = (T_SMCPlayGameData *)StateMachineGetExtraData(G_smHandle) ;
    DebugCheck(p_data != NULL) ;

    /* Update all the regular game stuff. */
    if (ClientIsActive())   {
        ClientUpdate() ;
        CreaturesCheck() ;
        ClientSyncEnsureSend() ;
    }

    /* Make sure the sync signal is being sent. */
//printf("Player xy: %d %d\n", PlayerGetX16(), PlayerGetY16()) ;

    DebugEnd() ;
    TICKER_TIME_ROUTINE_ENDM("SMMain/SMCPlayGameDoGameIdle", 500) ;
}


/*-------------------------------------------------------------------------*
 * Routine:  SMCPlayGameDoGameExit
 *-------------------------------------------------------------------------*/
/**
 *  SMCPlayGameDoGameExit
 *
 *  @param handle -- Handle to state machine
 *  @param extraData -- Not used
 *  @param isDestroyed -- TRUE if state machine is being destroy
 *
 *<!-----------------------------------------------------------------------*/
T_void SMCPlayGameDoGameExit(
           T_stateMachineHandle handle,
           T_word32 extraData,
           E_Boolean isDestroyed)
{
    T_SMCPlayGameData *p_data ;

    DebugRoutine("SMCPlayGameDoGameExit") ;

    p_data = (T_SMCPlayGameData *)StateMachineGetExtraData(G_smHandle) ;
    DebugCheck(p_data != NULL) ;

    /* Shut down the 3d view and associated members. */
    ControlFinish() ;

    SMMainSetFlag(SMMAIN_FLAG_END_GAME, TRUE) ;

    DebugEnd() ;
}


/** @} */
/*-------------------------------------------------------------------------*
 * End of File:  SMCPLAY.C
 *-------------------------------------------------------------------------*/
