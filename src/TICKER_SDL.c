/*-------------------------------------------------------------------------*
 * File:  TICKER.C
 *-------------------------------------------------------------------------*/
/**
 * Track the time in the system in 70ths of second since the start of
 * the application.
 *
 * @addtogroup TICKER
 * @brief Ticker Timer System
 * @see http://www.amuletsandarmor.com/AALicense.txt
 * @{
 *
 *<!-----------------------------------------------------------------------*/
#include "SOUND.H"
#include "TICKER.H"

static T_word32 G_lastMillisecondCount;
static T_word32 G_tickMilli;
static T_word32 G_tickCount;
static E_Boolean F_tickerOn = FALSE;
static T_sword16 G_pauseLevel = 0;

T_void
TickerOn(T_void)
{
    DebugRoutine("TickerOn");
    DebugCheck(F_tickerOn == FALSE);

    /* Note that the ticker is now on. */
    F_tickerOn = TRUE;
    G_lastMillisecondCount = SDL_GetTicks(); // clock() ;
    G_tickMilli = 0;

    DebugEnd();
}
T_void
TickerOff(T_void)
{
    DebugRoutine("TickerOff");
    DebugCheck(F_tickerOn == TRUE);

    /* Note that the ticker is now off. */
    F_tickerOn = FALSE;

    DebugEnd();
}

T_void
TickerUpdate(T_void)
{
    T_word32 time;

    time = SDL_GetTicks(); // clock() ;
    G_tickMilli += time - G_lastMillisecondCount;
    G_lastMillisecondCount = time;

    G_tickCount = G_tickMilli / 14;
}

T_word32
TickerGet(T_void)
{
    /* Update the tick count if we are not paused */
    if (!G_pauseLevel)
        TickerUpdate();
    return G_tickCount;
}

T_word32
TickerGetAccurate(T_void)
{
    return G_tickCount;
}

T_void
TickerPause(T_void)
{
    /* Add one to the pause level. */
    G_pauseLevel++;
}

T_void
TickerContinue(T_void)
{
    /* Sub one from the pause level. */
    G_pauseLevel--;
}


/** @} */
/*-------------------------------------------------------------------------*
 * End of File:  TICKER_SDL.C
 *-------------------------------------------------------------------------*/
