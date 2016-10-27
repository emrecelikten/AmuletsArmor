/*-------------------------------------------------------------------------*
 * File:  KEYBOARD.C
 *-------------------------------------------------------------------------*/
/**
 * Routines for tracking which keys are being pressed.  In DOS, we even
 * tried to make so we could tell how long a key was pressed.  This is
 * less of an issue these days.
 *
 * @addtogroup KEYBOARD
 * @brief Keyboad Controls
 * @see http://www.amuletsandarmor.com/AALicense.txt
 * @{
 *
 *<!-----------------------------------------------------------------------*/
#include <SDL.h>

#include "OPTIONS.H"
#include "DBLLINK.H"
#include "KEYBOARD.H"
#include "KEYSCAN.H"
#include "TICKER.H"

#define KEY_IS_DOWN 0x80
#define KEY_IS_CHANGED 0x01

#define KEYBOARD_HIGHEST_SCAN_CODE 255

/* Number of characters that can be held in keyboard buffer. */
/* Must be a power of two. */
#define MAX_SCAN_KEY_BUFFER 128

/* Note if the keyboard is on: */
static E_Boolean F_keyboardOn = FALSE;

/* Note if we are using the keyboard buffer: */
static E_Boolean F_keyboardBufferOn = TRUE;

/* Table to keep track of which keys are pressed: */
static E_Boolean G_keyTable[256];

/* Also keep a table to track which events for what keys have occured: */
static E_Boolean G_keyEventTable[256];

/* Information about how long the key was pressed. */
static T_word32 G_keyHoldTime[256];
static T_word32 G_keyPressTime[256];

/* Internal buffer of press and release keys: */
static T_word16 G_scanKeyBuffer[MAX_SCAN_KEY_BUFFER];
static T_word16 G_scanKeyStart = 0;
static T_word16 G_scanKeyEnd = 0;

/* Number of keys being pressed simultaneously: */
static T_word16 G_keysPressed = 0;

/* Keep track of the event handler for the keyboard. */
static T_keyboardEventHandler G_keyboardEventHandler;

/* Flag to tell if KeyboardGetScanCode works */
static E_Boolean G_allowKeyScans = TRUE;

static T_void
IKeyboardClear(T_void);

static T_void
IKeyboardSendCommand(T_byte8 keyboardCommand);

static E_Boolean G_escapeCode = FALSE;

static T_doubleLinkList G_eventStack = DOUBLE_LINK_LIST_BAD;

static T_word16 G_pauseLevel = 0;

char G_keyboardToASCII[256] = {
    '\0', '\0', '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\r', '\0', 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', '\0', '\0',
    '\0', '\040', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '!', '@', '#', '$', '%', '^',
    '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', '\r', '\0', 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '\"', '~', '\0', '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', '\0', '\0',
    '\0', '\040', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
};

static char G_asciiBuffer[32];
static T_word16 G_asciiStart = 0;
static T_word16 G_asciiEnd = 0;

/*-------------------------------------------------------------------------*
 * Routine:  KeyboardOn
 *-------------------------------------------------------------------------*/
/**
 *  KeyboardOn installs the keyboard functions of the following commands.
 *
 *  NOTE: 
 *  This routine can not be called twice in a row and must be called
 *  once for the rest of the routines to work correctly (Except the
 *  keyboard buffer commands).
 *
 *<!-----------------------------------------------------------------------*/
T_void
KeyboardOn(T_void)
{
    DebugRoutine("KeyboardOn");
    DebugCheck(F_keyboardOn == FALSE);

    /* Note that the keyboard is now on. */
    F_keyboardOn = TRUE;

    /* Clear the keyboard and event keyboard before we let things go. */
    IKeyboardClear();

    G_eventStack = DoubleLinkListCreate();
    DebugCheck(G_eventStack != DOUBLE_LINK_LIST_BAD);

    DebugEnd();
}

/*-------------------------------------------------------------------------*
 * Routine:  KeyboardOff
 *-------------------------------------------------------------------------*/
/**
 *  When you are done with the keyboard, you can turn it off by calling
 *  KeyboardOff.  This will remove the keyboard driver from memory.
 *
 *  NOTE: 
 *  This routine MUST be called before exiting or DOS will crash.
 *
 *<!-----------------------------------------------------------------------*/
T_void
KeyboardOff(T_void)
{
    DebugRoutine("KeyboardOff");
    DebugCheck(F_keyboardOn == TRUE);

    DebugCheck(G_eventStack != DOUBLE_LINK_LIST_BAD);
    DoubleLinkListDestroy(G_eventStack);

    /* Note that the keyboard is now off. */
    F_keyboardOn = FALSE;

    DebugEnd();
}

/*-------------------------------------------------------------------------*
 * Routine:  KeyboardGetScanCode
 *-------------------------------------------------------------------------*/
/**
 *  KeyboardGetScanCode is used to check if the key for the corresponding
 *  scan code is being pressed.  If it is, a TRUE is returned, or else
 *  false is returned.
 *
 *  @param scanCode -- Scan code to check
 *
 *  @return TRUE  = Key is pressed
 *      FALSE = Key is not pressed
 *
 *<!-----------------------------------------------------------------------*/
static E_Boolean
IGetAdjustedKey(T_word16 scanCode)
{
    switch (scanCode)
    {
        case 0x38 /** Left alt key **/ :
        case 0xB8 /** Right alt key **/ :
            return G_keyTable[0x38] |
                G_keyTable[0xB8];
            break;

        case KEY_SCAN_CODE_RIGHT_SHIFT:
        case KEY_SCAN_CODE_LEFT_SHIFT:
            return G_keyTable[KEY_SCAN_CODE_RIGHT_SHIFT] |
                G_keyTable[KEY_SCAN_CODE_LEFT_SHIFT];
            break;

        case 0x9D /** Right control key **/ :
        case 0x1D /** Left control key **/ :
            return G_keyTable[0x9D] |
                G_keyTable[0x1D];
            break;

        default:break;
    }
    return G_keyTable[scanCode];
}

E_Boolean
KeyboardGetScanCode(T_word16 scanCodes)
{
    E_Boolean isPressed;
    T_byte8 scanCode, scanCode2;

    DebugRoutine("KeyboardGetScanCode");
    DebugCheck(F_keyboardOn == TRUE);

    /* Check to see if key scans are allowed */
    /* (ESC and ALT and ENTER key break this rule) */
    /* Also function keys */
    if ((G_allowKeyScans == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_ESC) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_ENTER) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_ALT) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F1) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F2) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F3) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F4) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F5) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F6) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F7) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F8) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F9) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F10) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F11) == FALSE)
        && (IGetAdjustedKey(KEY_SCAN_CODE_F12) == FALSE))
    {
        /* Pretend the key is not pressed if in this mode. */
        isPressed = FALSE;
    }
    else
    {
        scanCode = scanCodes & 0xFF;
        scanCode2 = (scanCodes >> 8);

        DebugCheck(scanCode <= KEYBOARD_HIGHEST_SCAN_CODE);
        DebugCheck(scanCode2 <= KEYBOARD_HIGHEST_SCAN_CODE);

        if (scanCode2 == 0)
            isPressed = IGetAdjustedKey(scanCode);
        else
            isPressed = IGetAdjustedKey(scanCode) |
                IGetAdjustedKey(scanCode);
    }

    DebugCheck(isPressed < BOOLEAN_UNKNOWN);
    DebugEnd();

    return isPressed;
}

/*-------------------------------------------------------------------------*
 * Routine:  KeyboardDebounce
 *-------------------------------------------------------------------------*/
/**
 *  Sometimes it is useful to wait for all keys to be released.
 *  KeyboardDebounce is a simple routine that just waits until the user
 *  has released all pressed keys.
 *
 *<!-----------------------------------------------------------------------*/
T_void
KeyboardDebounce(T_void)
{
    DebugRoutine("KeyboardDebounce");
    DebugCheck(F_keyboardOn == TRUE);

#if 0
    T_word32 time ;

    time = TickerGet() ;

  //puts("Waiting") ;
    while (G_keysPressed != 0)
        { /* wait */
            if ((TickerGet() - time) > 210)
                break ;
        }
  //puts("Done") ;

    /* Clear out the keyboard stats. */
  //    IKeyboardClear() ;
#endif

    DebugEnd();
}

/*-------------------------------------------------------------------------*
 * Routine:  KeyboardSetEventHandler
 *-------------------------------------------------------------------------*/
/**
 *  KeyboardSetEventHandler declares the function to be called for
 *  each keyboard event as dispatched by the KeyboardUpdateEvents routine.
 *  If you no longer need a keyboard handler, give this routine an
 *  input parameter of NULL.
 *
 *  @param keyboardEventHandler -- function to call on events, or NULL
 *      for none.
 *
 *<!-----------------------------------------------------------------------*/
T_void
KeyboardSetEventHandler(T_keyboardEventHandler keyboardEventHandler)
{
    DebugRoutine("KeyboardSetEventHandler");
    /* Nothing to do a DebugCheck on ... */

    /* Record the event handler for future use. */
    G_keyboardEventHandler = keyboardEventHandler;

    DebugEnd();
}

T_keyboardEventHandler
KeyboardGetEventHandler(T_void)
{
    return (G_keyboardEventHandler);
}



/*-------------------------------------------------------------------------*
 * Routine:  KeyboardUpdateEvents
 *-------------------------------------------------------------------------*/
/**
 *  KeyboardUpdateEvents should be called often for it to work correctly
 *  with a keyboard event handler.  Events are sent by this routine to
 *  tell what keys are being pressed and released.
 *
 *<!-----------------------------------------------------------------------*/
T_void
KeyboardUpdateEvents(T_void)
{
    T_word16 scanCode;

    DebugRoutine("KeyboardUpdateEvents");
    DebugCheck(F_keyboardOn == TRUE);

    while (G_scanKeyStart != G_scanKeyEnd)
    {
        scanCode = G_scanKeyBuffer[G_scanKeyStart];
        /* Is this a press or a release? */
        if (scanCode & 0x100)
        {
            /* THis is a release. */
            G_keyboardEventHandler(KEYBOARD_EVENT_RELEASE, scanCode & 0xFF);
        }
        else
        {
            G_keyboardEventHandler(KEYBOARD_EVENT_PRESS, scanCode);
        }

        /* Special case for the right ctrl key -- we want it to act */
        /* just like the left control key. */
        if ((scanCode & 0xFF) == KEY_SCAN_CODE_RIGHT_CTRL)
        {
            if (scanCode & 0x100)
            {
                /* THis is a release. */
                G_keyboardEventHandler(KEYBOARD_EVENT_RELEASE, KEY_SCAN_CODE_CTRL);
            }
            else
            {
                G_keyboardEventHandler(KEYBOARD_EVENT_PRESS, KEY_SCAN_CODE_CTRL);
            }
        }
        _disable();
        G_scanKeyStart = (G_scanKeyStart + 1) & (MAX_SCAN_KEY_BUFFER - 1);
        _enable();
    }

    /* Now we will look for all keys that are being held and send */
    /* events appropriately. */
    for (scanCode = 0; scanCode <= KEYBOARD_HIGHEST_SCAN_CODE; scanCode++)
        /* Check to see if key is being held. */
        if (G_keyTable[scanCode] == TRUE)
            /* We need to send a event. */
            G_keyboardEventHandler(KEYBOARD_EVENT_HELD, scanCode);

    /* Add an event for the right ctrl to be the left ctrl. */
    if (G_keyTable[KEY_SCAN_CODE_RIGHT_CTRL] == TRUE)
        G_keyboardEventHandler(KEYBOARD_EVENT_HELD, KEY_SCAN_CODE_CTRL);

    /* Check to see if we are in buffered mode. */
    if (F_keyboardBufferOn == TRUE)
    {
        /* If we are, loop while there are characters in the buffer. */
        while (KeyboardBufferReady())
        {
            /* Send a special buffered event for each */
            /* of these characters. */
            G_keyboardEventHandler(
                KEYBOARD_EVENT_BUFFERED,
                KeyboardBufferGet());
        }
    }

    DebugEnd();
}

/*-------------------------------------------------------------------------*
 * Routine:  KeyboardBufferOn
 *-------------------------------------------------------------------------*/
/**
 *  KeyboardBufferOn will allow keys to be buffered up for use later.
 *  Although useful in word processors, when the buffer gets full it
 *  causes a beeping noise that is not wonderful in games.  Depending on
 *  your need, you may wnat the buffer on.
 *
 *  NOTE:
 *  KeyboardBufferOn CAN be called even if KeyboardOn is not.
 *
 *<!-----------------------------------------------------------------------*/
T_void
KeyboardBufferOn(T_void)
{
    DebugRoutine("KeyboardBufferOn");

    F_keyboardBufferOn = TRUE;

    DebugEnd();
}

/*-------------------------------------------------------------------------*
 * Routine:  KeyboardBufferOff
 *-------------------------------------------------------------------------*/
/**
 *  KeyboardBufferOff will not allow keys to be buffered up for use later.
 *  Although useful in word processors, when the buffer gets full it
 *  causes a beeping noise that is not wonderful in games.  Depending on
 *  your need, you may wnat the buffer off.
 *
 *  NOTE:
 *  KeyboardBufferOff CAN be called even if KeyboardOn is not.
 *  The default is for the buffer to be on.
 *
 *<!-----------------------------------------------------------------------*/
T_void
KeyboardBufferOff(T_void)
{
    DebugRoutine("KeyboardBufferOff");

//    F_keyboardBufferOn = FALSE ;

    DebugEnd();
}

/*-------------------------------------------------------------------------*
 * Routine:  KeyboardBufferGet
 *-------------------------------------------------------------------------*/
/**
 *  If you are using a buffer, you can get keystrokes from the buffer
 *  by using this routine.
 *
 *  @return ASCII character of gained character
 *      or 0 for none.
 *
 *<!-----------------------------------------------------------------------*/
T_byte8
KeyboardBufferGet(T_void)
{
    T_byte8 key;
    T_word16 next;

    DebugRoutine("KeyboardBufferGet");

    /* Are there any keys waiting? */
    if (G_asciiStart != G_asciiEnd)
    {
        /* Find where the next position will be */
        next = (G_asciiStart + 1) & 31;

        /* Get this character */
        key = G_asciiBuffer[G_asciiStart];

        /* Advance in the queue. */
        G_asciiStart = next;
    }
    else
    {
        /* No keys waiting. */
        key = 0;
    }

    DebugEnd();

    return key;
}

/*-------------------------------------------------------------------------*
 * Routine:  IKeyboardClear
 *-------------------------------------------------------------------------*/
/**
 *  IKeyboardClear is a routine that is called to clear the whole
 *  key tables of their state and assume that all keys are NOT being
 *  pressed (even if they are).
 *
 *  NOTE: 
 *  If this routine is called and a key IS being pressed, it will be
 *  ignored until the key is released and then pressed again.  In other
 *  words, you should only call this routine on startup.
 *
 *<!-----------------------------------------------------------------------*/
static T_void
IKeyboardClear(T_void)
{
    T_word16 scanCode;

    DebugRoutine("IKeyboardClear");

    _disable();

    /* Clear all scan codes stored in the key table and key event table. */
    for (scanCode = 0; scanCode <= KEYBOARD_HIGHEST_SCAN_CODE; scanCode++)
    {
        G_keyTable[scanCode] = FALSE;
        G_keyEventTable[scanCode] = FALSE;
        G_keyHoldTime[scanCode] = 0;
        G_keyPressTime[scanCode] = 0;
    }

    memset(G_scanKeyBuffer, 0, sizeof(G_scanKeyBuffer));
    G_scanKeyStart = G_scanKeyEnd = 0;

    /* Note that none of the keys are pressed. */
    G_keysPressed = 0;

    G_asciiStart = G_asciiEnd = 0;

    _enable();

    DebugEnd();
}

/*-------------------------------------------------------------------------*
 * Routine:  KeyboardGetHeldTimeAndClear
 *-------------------------------------------------------------------------*/
/**
 *  KeyboardGetHeldTimeAndClear tells how long a key has been held
 *  down since the key was last cleared.  This capability is useful
 *  for getting accurate readings of how long the user desired the key
 *  to be pressed, and not based on when calls were able to be made.
 *
 *  NOTE: 
 *  Calling this routine can be a problem if it has been a long time
 *  since it was previously called on a key.  If you are unsure, be sure
 *  to make a call to KeyboardDebounce which clears the timing values.
 *
 *<!-----------------------------------------------------------------------*/
T_word32
KeyboardGetHeldTimeAndClear(T_word16 scanCode)
{
    T_word32 timeHeld;
    T_word32 time;

    DebugRoutine("KeyboardGetHeldTimeAndClear");

    /* No interrupts, please. */
    _disable();

    /* Is the key being currently pressed? */
    if (G_keyTable[scanCode])
    {
        /* It is being pressed. */
        /* Return the time held in the past plus the time currently held */
        time = TickerGet();
        timeHeld = G_keyHoldTime[scanCode] + (time - G_keyPressTime[scanCode]);
        G_keyPressTime[scanCode] = time;
    }
    else
    {
        /* It is not being pressed. */
        /* Return the time held in the register. */
        timeHeld = G_keyHoldTime[scanCode];
    }

    /* Always clear the time held. */
    G_keyHoldTime[scanCode] = 0;

    /* Allow interrupts again. */
    _enable();

    /* If no key scans are allowed, pretend the key was not hit. */
    if (G_allowKeyScans == FALSE)
        timeHeld = 0;

    DebugEnd();

    return timeHeld;
}

/*-------------------------------------------------------------------------*
 * Routine:  KeyboardPushEventHandler
 *-------------------------------------------------------------------------*/
/**
 *  KeyboardPushEventHandler removes the old handler by placing it on
 *  a stack and sets up the new event handler.  The old event handler
 *  will be restored when a call to KeyboardPopEventHandler is called.
 *
 *  @param keyboardEventHandler -- function to call on events, or NULL
 *      for none.
 *
 *<!-----------------------------------------------------------------------*/
T_void
KeyboardPushEventHandler(T_keyboardEventHandler keyboardEventHandler)
{
    DebugRoutine("KeyboardPushEventHandler");

    /* Store the old event handler. */
    DoubleLinkListAddElementAtFront(
        G_eventStack,
        (T_void *) KeyboardGetEventHandler);

    /* set up the new handler. */
    KeyboardSetEventHandler(keyboardEventHandler);

    DebugEnd();
}


/*-------------------------------------------------------------------------*
 * Routine:  KeyboardPopEventHandler
 *-------------------------------------------------------------------------*/
/**
 *  KeyboardPopEventHandler restores the last handler on the stack
 *  (if there is one).
 *
 *<!-----------------------------------------------------------------------*/
T_void
KeyboardPopEventHandler(T_void)
{
    T_doubleLinkListElement first;
    T_keyboardEventHandler handler;

    DebugRoutine("KeyboardPopEventHandler");

    /* Get the old event handler. */
    first = DoubleLinkListGetFirst(G_eventStack);
    DebugCheck(first != DOUBLE_LINK_LIST_ELEMENT_BAD);

    if (first != DOUBLE_LINK_LIST_ELEMENT_BAD)
    {
        handler = (T_keyboardEventHandler)
            DoubleLinkListElementGetData(first);
        KeyboardSetEventHandler(handler);
        DoubleLinkListRemoveElement(first);
    }

    DebugEnd();
}

T_void
KeyboardDisallowKeyScans(T_void)
{
    G_allowKeyScans = FALSE;
}

T_void
KeyboardAllowKeyScans(T_void)
{
    G_allowKeyScans = TRUE;
}

E_Boolean
KeyboardBufferReady(T_void)
{
    if (G_asciiStart != G_asciiEnd)
        return TRUE;
    else
        return FALSE;
}

// This maps the SDL keysym list to the list of scancodes in the original game
const uint8_t G_sdlToScancode[] = {
    0, // 0
    0, // 1
    0, // 2
    0, // 3
    KEY_SCAN_CODE_A, // SDL_SCANCODE_a			                = 4,
    KEY_SCAN_CODE_B, // SDL_SCANCODE_b			                = 5,
    KEY_SCAN_CODE_C, // SDL_SCANCODE_c			                = 6,
    KEY_SCAN_CODE_D, // SDL_SCANCODE_d			                = 7,
    KEY_SCAN_CODE_E, // SDL_SCANCODE_e			                = 8,
    KEY_SCAN_CODE_F, // SDL_SCANCODE_f			                = 9,
    KEY_SCAN_CODE_G, // SDL_SCANCODE_g			                = 10,
    KEY_SCAN_CODE_H, // SDL_SCANCODE_h			                = 11,
    KEY_SCAN_CODE_I, // SDL_SCANCODE_i			                = 12,
    KEY_SCAN_CODE_J, // SDL_SCANCODE_j			                = 13,
    KEY_SCAN_CODE_K, // SDL_SCANCODE_k			                = 14,
    KEY_SCAN_CODE_L, // SDL_SCANCODE_l			                = 15,
    KEY_SCAN_CODE_M, // SDL_SCANCODE_m			                = 16,
    KEY_SCAN_CODE_N, // SDL_SCANCODE_n			                = 17,
    KEY_SCAN_CODE_O, // SDL_SCANCODE_o			                = 18,
    KEY_SCAN_CODE_P, // SDL_SCANCODE_p			                = 19,
    KEY_SCAN_CODE_Q, // SDL_SCANCODE_q			                = 20,
    KEY_SCAN_CODE_R, // SDL_SCANCODE_r			                = 21,
    KEY_SCAN_CODE_S, // SDL_SCANCODE_s			                = 22,
    KEY_SCAN_CODE_T, // SDL_SCANCODE_t			                = 23,
    KEY_SCAN_CODE_U, // SDL_SCANCODE_u			                = 24,
    KEY_SCAN_CODE_V, // SDL_SCANCODE_v			                = 25,
    KEY_SCAN_CODE_W, // SDL_SCANCODE_w			                = 26,
    KEY_SCAN_CODE_X, // SDL_SCANCODE_x			                = 27,
    KEY_SCAN_CODE_Y, // SDL_SCANCODE_y			                = 28,
    KEY_SCAN_CODE_Z, // SDL_SCANCODE_z			                = 29,
    KEY_SCAN_CODE_1, // SDL_SCANCODE_1			                = 30,
    KEY_SCAN_CODE_2, // SDL_SCANCODE_2                          = 31,
    KEY_SCAN_CODE_3, // SDL_SCANCODE_3                          = 32,
    KEY_SCAN_CODE_4, // SDL_SCANCODE_4                          = 33,
    KEY_SCAN_CODE_5, // SDL_SCANCODE_5                          = 34,
    KEY_SCAN_CODE_6, // SDL_SCANCODE_6                          = 35,
    KEY_SCAN_CODE_7, // SDL_SCANCODE_7                          = 36,
    KEY_SCAN_CODE_8, // SDL_SCANCODE_8                          = 37,
    KEY_SCAN_CODE_9, // SDL_SCANCODE_9                          = 38,
    KEY_SCAN_CODE_0, // SDL_SCANCODE_0			                = 39,
    KEY_SCAN_CODE_ENTER, // SDL_SCANCODE_RETURN	                = 40,
    KEY_SCAN_CODE_ESC, // SDL_SCANCODE_ESCAPE	                = 41,
    KEY_SCAN_CODE_BACKSPACE, // SDL_SCANCODE_BACKSPACE		    = 42,
    KEY_SCAN_CODE_TAB, // SDL_SCANCODE_TAB		                = 43,
    KEY_SCAN_CODE_SPACE, // SDL_SCANCODE_SPACE		            = 44,
    KEY_SCAN_CODE_MINUS, // SDL_SCANCODE_MINUS		            = 45,
    KEY_SCAN_CODE_EQUAL, // SDL_SCANCODE_EQUALS		            = 46,
    KEY_SCAN_CODE_SB_OPEN, // SDL_SCANCODE_LEFTBRACKET	        = 47,
    KEY_SCAN_CODE_SB_CLOSE, // SDL_SCANCODE_RIGHTBRACKET	    = 48,
    KEY_SCAN_CODE_BACKSLASH, // SDL_SCANCODE_BACKSLASH		    = 49,
    0, //                                                       = 50,
    KEY_SCAN_CODE_SEMI_COLON, // SDL_SCANCODE_SEMICOLON		    = 51,
    KEY_SCAN_CODE_APOSTROPHE, // SDL_SCANCODE_APOSTROPHE        = 52,
    KEY_SCAN_CODE_GRAVE, // SDL_SCANCODE_GRAVE                  = 53,
    KEY_SCAN_CODE_COMMA, // SDL_SCANCODE_COMMA		            = 54,
    KEY_SCAN_CODE_PERIOD, // SDL_SCANCODE_PERIOD		        = 55,
    KEY_SCAN_CODE_SLASH, // SDL_SCANCODE_SLASH		            = 56,
    KEY_SCAN_CODE_CAPS_LOCK, // SDL_SCANCODE_CAPSLOCK		    = 57,
    KEY_SCAN_CODE_F1, // SDL_SCANCODE_F1	            		= 58,
    KEY_SCAN_CODE_F2, // SDL_SCANCODE_F2	            		= 59,
    KEY_SCAN_CODE_F3, // SDL_SCANCODE_F3	            		= 60,
    KEY_SCAN_CODE_F4, // SDL_SCANCODE_F4	            		= 61,
    KEY_SCAN_CODE_F5, // SDL_SCANCODE_F5	            		= 62,
    KEY_SCAN_CODE_F6, // SDL_SCANCODE_F6	            		= 63,
    KEY_SCAN_CODE_F7, // SDL_SCANCODE_F7	            		= 64,
    KEY_SCAN_CODE_F8, // SDL_SCANCODE_F8	            		= 65,
    KEY_SCAN_CODE_F9, // SDL_SCANCODE_F9	            		= 66,
    KEY_SCAN_CODE_F10, // SDL_SCANCODE_F10		                = 67,
    KEY_SCAN_CODE_F11, // SDL_SCANCODE_F11		                = 68,
    KEY_SCAN_CODE_F12, // SDL_SCANCODE_F12		                = 69,
    0, // SDL_SCANCODE_PRINTSCREEN                              = 70,
    KEY_SCAN_CODE_SCROLL_LOCK, // SDL_SCANCODE_SCROLLOCK		= 71,
    KEY_SCAN_CODE_PAUSE, // SDL_SCANCODE_PAUSE		            = 72,
    KEY_SCAN_CODE_INSERT, // SDL_SCANCODE_INSERT	        	= 73,
    KEY_SCAN_CODE_HOME, // SDL_SCANCODE_HOME		            = 74,
    KEY_SCAN_CODE_PGUP, // SDL_SCANCODE_PAGEUP		            = 75,
    KEY_SCAN_CODE_DELETE, // SDL_SCANCODE_DELETE	        	= 76,
    KEY_SCAN_CODE_END, // SDL_SCANCODE_END		                = 77,
    KEY_SCAN_CODE_PGDN, // SDL_SCANCODE_PAGEDOWN	        	= 78,
    KEY_SCAN_CODE_RIGHT, // SDL_SCANCODE_RIGHT		            = 79,
    KEY_SCAN_CODE_LEFT, // SDL_SCANCODE_LEFT		            = 80,
    KEY_SCAN_CODE_DOWN, // SDL_SCANCODE_DOWN		            = 81,
    KEY_SCAN_CODE_UP, // SDL_SCANCODE_UP			            = 82,
    KEY_SCAN_CODE_NUM_LOCK, // SDL_SCANCODE_NUMLOCK		        = 83,
    KEY_SCAN_CODE_KEYPAD_SLASH, // SDL_SCANCODE_KP_DIVIDE		= 84,
    KEY_SCAN_CODE_KEYPAD_STAR, // SDL_SCANCODE_KP_MULTIPLY	    = 85,
    KEY_SCAN_CODE_KEYPAD_MINUS, // SDL_SCANCODE_KP_MINUS		= 86,
    KEY_SCAN_CODE_KEYPAD_PLUS, // SDL_SCANCODE_KP_PLUS		    = 87,
    KEY_SCAN_CODE_KEYPAD_ENTER, // SDL_SCANCODE_KP_ENTER		= 88,
    KEY_SCAN_CODE_KEYPAD_1, // SDL_SCANCODE_KP1		            = 89,
    KEY_SCAN_CODE_KEYPAD_2, // SDL_SCANCODE_KP2		            = 90,
    KEY_SCAN_CODE_KEYPAD_3, // SDL_SCANCODE_KP3		            = 91,
    KEY_SCAN_CODE_KEYPAD_4, // SDL_SCANCODE_KP4		            = 92,
    KEY_SCAN_CODE_KEYPAD_5, // SDL_SCANCODE_KP5		            = 93,
    KEY_SCAN_CODE_KEYPAD_6, // SDL_SCANCODE_KP6		            = 94,
    KEY_SCAN_CODE_KEYPAD_7, // SDL_SCANCODE_KP7		            = 95,
    KEY_SCAN_CODE_KEYPAD_8, // SDL_SCANCODE_KP8		            = 96,
    KEY_SCAN_CODE_KEYPAD_9, // SDL_SCANCODE_KP9		            = 97,
    KEY_SCAN_CODE_KEYPAD_0, // SDL_SCANCODE_KP0		            = 98,
    KEY_SCAN_CODE_KEYPAD_PERIOD, // SDL_SCANCODE_KP_PERIOD		= 99,
    0, // SDL_SCANCODE_NONUSBACKSLASH                           = 100,
    0, // SDL_SCANCODE_APPLICATION                              = 101,
    0, // SDL_SCANCODE_POWER                                    = 102,
    0, // SDL_SCANCODE_KP_EQUALS                                = 103,
    0, // SDL_SCANCODE_F13                                      = 104,
    0, // SDL_SCANCODE_F14                                      = 105,
    0, // SDL_SCANCODE_F15                                      = 106,
    0, // SDL_SCANCODE_F16                                      = 107,
    0, // SDL_SCANCODE_F17                                      = 108,
    0, // SDL_SCANCODE_F18                                      = 109,
    0, // SDL_SCANCODE_F19                                      = 110,
    0, // SDL_SCANCODE_F20                                      = 111,
    0, // SDL_SCANCODE_F21                                      = 112,
    0, // SDL_SCANCODE_F22                                      = 113,
    0, // SDL_SCANCODE_F23                                      = 114,
    0, // SDL_SCANCODE_F24                                      = 115,
    0, // SDL_SCANCODE_EXECUTE                                  = 116,
    0, // SDL_SCANCODE_HELP                                     = 117,
    0, // SDL_SCANCODE_MENU                                     = 118,
    0, // SDL_SCANCODE_SELECT                                   = 119,
    0, // SDL_SCANCODE_STOP                                     = 120,
    0, // SDL_SCANCODE_AGAIN                                    = 121,
    0, // SDL_SCANCODE_UNDO                                     = 122,
    0, // SDL_SCANCODE_CUT                                      = 123,
    0, // SDL_SCANCODE_COPY                                     = 124,
    0, // SDL_SCANCODE_PASTE                                    = 125,
    0, // SDL_SCANCODE_FIND                                     = 126,
    0, // SDL_SCANCODE_MUTE                                     = 127,
    0, // SDL_SCANCODE_VOLUMEUP                                 = 128,
    0, // SDL_SCANCODE_VOLUMEDOWN                               = 129,
    0, // SDL_SCANCODE_LOCKINGCAPSLOCK                          = 130,
    0, // SDL_SCANCODE_LOCKINGNUMLOCK                           = 131,
    0, // SDL_SCANCODE_LOCKINGSCROLLLOCK                        = 132,
    0, // SDL_SCANCODE_KP_COMMA                                 = 133,
    0, // SDL_SCANCODE_KP_EQUALSAS400                           = 134,
    0, // SDL_SCANCODE_INTERNATIONAL1                           = 135,
    0, // SDL_SCANCODE_INTERNATIONAL2                           = 136,
    0, // SDL_SCANCODE_INTERNATIONAL3                           = 137,
    0, // SDL_SCANCODE_INTERNATIONAL4                           = 138,
    0, // SDL_SCANCODE_INTERNATIONAL5                           = 139,
    0, // SDL_SCANCODE_INTERNATIONAL6                           = 140,
    0, // SDL_SCANCODE_INTERNATIONAL7                           = 141,
    0, // SDL_SCANCODE_INTERNATIONAL8                           = 142,
    0, // SDL_SCANCODE_INTERNATIONAL9                           = 143,
    0, // SDL_SCANCODE_LANG1                                    = 144,
    0, // SDL_SCANCODE_LANG2                                    = 145,
    0, // SDL_SCANCODE_LANG3                                    = 146,
    0, // SDL_SCANCODE_LANG4                                    = 147,
    0, // SDL_SCANCODE_LANG5                                    = 148,
    0, // SDL_SCANCODE_LANG6                                    = 149,
    0, // SDL_SCANCODE_LANG7                                    = 150,
    0, // SDL_SCANCODE_LANG8                                    = 151,
    0, // SDL_SCANCODE_LANG9                                    = 152,
    0, // SDL_SCANCODE_ALTERASE                                 = 153,
    0, // SDL_SCANCODE_SYSREQ                                   = 154,
    0, // SDL_SCANCODE_CANCEL                                   = 155,
    0, // SDL_SCANCODE_CLEAR                                    = 156,
    0, // SDL_SCANCODE_PRIOR                                    = 157,
    0, // SDL_SCANCODE_RETURN2                                  = 158,
    0, // SDL_SCANCODE_SEPARATOR                                = 159,
    0, // SDL_SCANCODE_OUT                                      = 160,
    0, // SDL_SCANCODE_OPER                                     = 161,
    0, // SDL_SCANCODE_CLEARAGAIN                               = 162,
    0, // SDL_SCANCODE_CRSEL                                    = 163,
    0, // SDL_SCANCODE_EXSEL                                    = 164,
    0, //                                                       = 165,
    0, //                                                       = 166,
    0, //                                                       = 167,
    0, //                                                       = 168,
    0, //                                                       = 169,
    0, //                                                       = 170,
    0, //                                                       = 171,
    0, //                                                       = 172,
    0, //                                                       = 173,
    0, //                                                       = 174,
    0, //                                                       = 175,
    0, // SDL_SCANCODE_KP_00                                    = 176,
    0, // SDL_SCANCODE_KP_000                                   = 177,
    0, // SDL_SCANCODE_THOUSANDSSEPARATOR                       = 178,
    0, // SDL_SCANCODE_DECIMALSEPARATOR                         = 179,
    0, // SDL_SCANCODE_CURRENCYUNIT                             = 180,
    0, // SDL_SCANCODE_CURRENCYSUBUNIT                          = 181,
    0, // SDL_SCANCODE_KP_LEFTPAREN                             = 182,
    0, // SDL_SCANCODE_KP_RIGHTPAREN                            = 183,
    0, // SDL_SCANCODE_KP_LEFTBRACE                             = 184,
    0, // SDL_SCANCODE_KP_RIGHTBRACE                            = 185,
    0, // SDL_SCANCODE_KP_TAB                                   = 186,
    0, // SDL_SCANCODE_KP_BACKSPACE                             = 187,
    0, // SDL_SCANCODE_KP_A                                     = 188,
    0, // SDL_SCANCODE_KP_B                                     = 189,
    0, // SDL_SCANCODE_KP_C                                     = 190,
    0, // SDL_SCANCODE_KP_D                                     = 191,
    0, // SDL_SCANCODE_KP_E                                     = 192,
    0, // SDL_SCANCODE_KP_F                                     = 193,
    0, // SDL_SCANCODE_KP_XOR                                   = 194,
    0, // SDL_SCANCODE_KP_POWER                                 = 195,
    0, // SDL_SCANCODE_KP_PERCENT                               = 196,
    0, // SDL_SCANCODE_KP_LESS                                  = 197,
    0, // SDL_SCANCODE_KP_GREATER                               = 198,
    0, // SDL_SCANCODE_KP_AMPERSAND                             = 199,
    0, // SDL_SCANCODE_KP_DBLAMPERSAND                          = 200,
    0, // SDL_SCANCODE_KP_VERTICALBAR                           = 201,
    0, // SDL_SCANCODE_KP_DBLVERTICALBAR                        = 202,
    0, // SDL_SCANCODE_KP_COLON                                 = 203,
    0, // SDL_SCANCODE_KP_HASH                                  = 204,
    0, // SDL_SCANCODE_KP_SPACE                                 = 205,
    0, // SDL_SCANCODE_KP_AT                                    = 206,
    0, // SDL_SCANCODE_KP_EXCLAM                                = 207,
    0, // SDL_SCANCODE_KP_MEMSTORE                              = 208,
    0, // SDL_SCANCODE_KP_MEMRECALL                             = 209,
    0, // SDL_SCANCODE_KP_MEMCLEAR                              = 210,
    0, // SDL_SCANCODE_KP_MEMADD                                = 211,
    0, // SDL_SCANCODE_KP_MEMSUBTRACT                           = 212,
    0, // SDL_SCANCODE_KP_MEMMULTIPLY                           = 213,
    0, // SDL_SCANCODE_KP_MEMDIVIDE                             = 214,
    0, // SDL_SCANCODE_KP_PLUSMINUS                             = 215,
    0, // SDL_SCANCODE_KP_CLEAR                                 = 216,
    0, // SDL_SCANCODE_KP_CLEARENTRY                            = 217,
    0, // SDL_SCANCODE_KP_BINARY                                = 218,
    0, // SDL_SCANCODE_KP_OCTAL                                 = 219,
    0, // SDL_SCANCODE_KP_DECIMAL                               = 220,
    0, // SDL_SCANCODE_KP_HEXADECIMAL                           = 221,
    0, //                                                       = 222,
    0, //                                                       = 223,
    KEY_SCAN_CODE_LEFT_CTRL, // SDL_SCANCODE_LCTRL              = 224,
    KEY_SCAN_CODE_LEFT_SHIFT, // SDL_SCANCODE_LSHIFT            = 225,
    KEY_SCAN_CODE_ALT, // SDL_SCANCODE_LALT                     = 226,
    0, // SDL_SCANCODE_LGUI                                     = 227,
    KEY_SCAN_CODE_RIGHT_CTRL, // SDL_SCANCODE_RCTRL             = 228,
    KEY_SCAN_CODE_RIGHT_SHIFT, // SDL_SCANCODE_RSHIFT           = 229,
    KEY_SCAN_CODE_ALT, // SDL_SCANCODE_RALT                     = 230,
    0, // SDL_SCANCODE_RGUI                                     = 231,
    0, // SDL_SCANCODE_MODE                                     = 257,
    0, // SDL_SCANCODE_AUDIONEXT                                = 258,
    0, // SDL_SCANCODE_AUDIOPREV                                = 259,
    0, // SDL_SCANCODE_AUDIOSTOP                                = 260,
    0, // SDL_SCANCODE_AUDIOPLAY                                = 261,
    0, // SDL_SCANCODE_AUDIOMUTE                                = 262,
    0, // SDL_SCANCODE_MEDIASELECT                              = 263,
    0, // SDL_SCANCODE_WWW                                      = 264,
    0, // SDL_SCANCODE_MAIL                                     = 265,
    0, // SDL_SCANCODE_CALCULATOR                               = 266,
    0, // SDL_SCANCODE_COMPUTER                                 = 267,
    0, // SDL_SCANCODE_AC_SEARCH                                = 268,
    0, // SDL_SCANCODE_AC_HOME                                  = 269,
    0, // SDL_SCANCODE_AC_BACK                                  = 270,
    0, // SDL_SCANCODE_AC_FORWARD                               = 271,
    0, // SDL_SCANCODE_AC_STOP                                  = 272,
    0, // SDL_SCANCODE_AC_REFRESH                               = 273,
    0, // SDL_SCANCODE_AC_BOOKMARKS                             = 274,
    0, // SDL_SCANCODE_BRIGHTNESSDOWN                           = 275,
    0, // SDL_SCANCODE_BRIGHTNESSUP                             = 276,
    0, // SDL_SCANCODE_DISPLAYSWITCH                            = 277,
    0, // SDL_SCANCODE_KBDILLUMTOGGLE                           = 278,
    0, // SDL_SCANCODE_KBDILLUMDOWN                             = 279,
    0, // SDL_SCANCODE_KBDILLUMUP                               = 280,
    0, // SDL_SCANCODE_EJECT                                    = 281,
    0, // SDL_SCANCODE_SLEEP                                    = 282,
    0, // SDL_SCANCODE_APP1                                     = 283,
    0  // SDL_SCANCODE_APP2                                     = 284,
};

static T_byte8 G_lastKeyState[SDL_NUM_SCANCODES];
T_void
KeyboardUpdate(E_Boolean updateBuffers)
{
    //T_byte8 keys[256] ;
    const T_byte8 *keys;
    T_word32 time;
    T_word16 scanCode;
    T_byte8 c;
    T_word16 next;
    T_word16 i;
    E_Boolean changed;
    E_Boolean newValue;

    DebugRoutine("KeyboardUpdate");
    time = TickerGet();
    //GetKeyboardState(keys) ;
    keys = SDL_GetKeyboardState(NULL);

    /* Only care about up/down status */
//    for (scanCode=0; scanCode<256; scanCode++)
//        keys[scanCode] &= KEY_IS_DOWN ;

    if (updateBuffers)
    {
        for (i = 1; i < SDL_NUM_SCANCODES; i++)
        {
            // Skip keys we don't know how to process
            if (G_sdlToScancode[i] == 0)
                continue;

            changed = (keys[i] != G_lastKeyState[i]) ? TRUE : FALSE;

            //scanCode = MapVirtualKey(i, 0) ; // old windows version
            scanCode = G_sdlToScancode[i];

            /* Record the state of the keypress */
            newValue = (keys[i]) ? TRUE : FALSE;
//            changed = (newValue != G_keyTable[scanCode]) ? TRUE : FALSE;

            /* Find keys that have changed */
            if (changed)
            {
/*printf("scancode %d = %d\n", scanCode, newValue);*/
                G_keyTable[scanCode] = newValue;
                /* Store the key in the scan key buffer */
                G_scanKeyBuffer[G_scanKeyEnd] = scanCode | ((G_keyTable[scanCode]) ? 0 : 0x100);
                next = (G_scanKeyEnd + 1) & (MAX_SCAN_KEY_BUFFER - 1);
                if (next != G_scanKeyStart)
                    G_scanKeyEnd = next;

                /* If a press and an ascii character, store that in the */
                /* keyboard buffer. */
                if ((G_keyTable[scanCode] == TRUE) &&
                    (IGetAdjustedKey(KEY_SCAN_CODE_ALT) == FALSE) &&
                    (G_keyTable[KEY_SCAN_CODE_RIGHT_CTRL] == FALSE) &&
                    (G_keyTable[KEY_SCAN_CODE_LEFT_CTRL] == FALSE))
                {
                    c = scanCode & 0xFF;

                    /* Translate for the shift key */
                    if (G_keyTable[KEY_SCAN_CODE_RIGHT_SHIFT] |
                        G_keyTable[KEY_SCAN_CODE_LEFT_SHIFT])
                        c |= 0x80;

                    /* What ascii character is this? */
                    c = G_keyboardToASCII[c];

                    /* Where are we going to roll over to next */
                    next = (G_asciiEnd + 1) & 31;

                    // Must be non-zero
                    if (c)
                    {
                        /* If back at start, don't do */
                        if (next != G_asciiStart)
                        {
//printf("  buffer key %d, %d (%c), i=%d\n", scanCode, c, c, i);
                            /* Store in the buffer */
                            G_asciiBuffer[G_asciiEnd] = c;
                            G_asciiEnd = next;
                        }
                    }
                }

                /* Check to see if keystroke changed from pressed to released or */
                /* visa-versa.  If it has, update the key count appropriately. */
                if (G_keyTable[scanCode] == TRUE)
                {
                    G_keysPressed++;
                    /* Note the time that key is pressed. */
                    G_keyPressTime[scanCode] = time;
                }
                else if (G_keyTable[scanCode] == FALSE)
                {
                    G_keysPressed--;
                    /* Note how long the key has been held down. */
                    G_keyHoldTime[scanCode] += time - G_keyPressTime[scanCode];
                }
            }
        }
    }
    memcpy(G_lastKeyState, keys, sizeof(G_lastKeyState));
    DebugEnd();
}

/** @} */
/*-------------------------------------------------------------------------*
 * End of File:  KEYBOARD_SDL.C
 *-------------------------------------------------------------------------*/
