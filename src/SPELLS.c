/*-------------------------------------------------------------------------*
 * File:  SPELLS.C
 *-------------------------------------------------------------------------*/
/**
 * All spell casting goes through here.
 *
 * @addtogroup SPELLS
 * @brief Spells System
 * @see http://www.amuletsandarmor.com/AALicense.txt
 * @{
 *
 *<!-----------------------------------------------------------------------*/
#include "BANNER.H"
#include "CLIENT.H"
#include "COLOR.H"
#include "DBLLINK.H"
#include "EFFECT.H"
#include "KEYSCAN.H"
#include "KEYMAP.H"
#include "MESSAGE.H"
#include "PICS.H"
#include "SOUND.H"
#include "SOUNDS.H"
#include "SPELLS.H"
#include "STATS.H"

/* define if all spells available for any class */
#define MANA_BACKCOLOR 225

static T_byte8 G_curspell[4]={0,0,0,0};
static T_byte8 G_curSound=0;
static T_doubleLinkList G_spellsList;
static T_sword16 G_beaconX=0;
static T_sword16 G_beaconY=0;
static T_word16  G_facing=0;
static E_Boolean G_clearSpells=FALSE;
static E_Boolean G_isInit = FALSE ;

/*-------------------------------------------------------------------------*
 * Routine:  SpellsInitSpells
 *-------------------------------------------------------------------------*/
/**
 *  Initializes variables associated with spell casting.
 *  Must be called prior to client login.
 *
 *<!-----------------------------------------------------------------------*/
T_void SpellsInitSpells (T_void)
{
	T_word16 count=0;
    T_byte8 stmp[32];
    T_word16 i;
    T_resource res;
    T_spellStruct *p_spell;
    E_spellsSpellSystemType system;
    E_Boolean unload=FALSE;

	DebugRoutine ("SpellsInitSpells");
    DebugCheck(G_isInit == FALSE) ;

    G_isInit = TRUE ;

    /* now, create new linked list for storage of spells */
    G_spellsList=DoubleLinkListCreate();
    DebugCheck (G_spellsList!=DOUBLE_LINK_LIST_BAD);

    /* get current character spell system */
    system = StatsGetPlayerSpellSystem();

    /* scan through resource files and lock in all available spells */
    /* for current class type */
    sprintf (stmp,"SPELDESC/SPL%05d",count++);
    while (PictureExist(stmp))
    {
        /* lock in a new spell structure */
        p_spell = (T_spellStruct *)PictureLockData (stmp,&res);

        DebugCheck (p_spell != NULL);

        /* remap casting code to keyboard */
        for (i=0;i<4;i++)
        {
            if (p_spell->code[i]==1) p_spell->code[i]=KeyMap(KEYMAP_RUNE1);
            else if (p_spell->code[i]==2) p_spell->code[i]=KeyMap(KEYMAP_RUNE2);
            else if (p_spell->code[i]==3) p_spell->code[i]=KeyMap(KEYMAP_RUNE3);
            else if (p_spell->code[i]==4) p_spell->code[i]=KeyMap(KEYMAP_RUNE4);
            else if (p_spell->code[i]==5) p_spell->code[i]=KeyMap(KEYMAP_RUNE5);
            else if (p_spell->code[i]==6) p_spell->code[i]=KeyMap(KEYMAP_RUNE6);
            else if (p_spell->code[i]==7) p_spell->code[i]=KeyMap(KEYMAP_RUNE7);
            else if (p_spell->code[i]==8) p_spell->code[i]=KeyMap(KEYMAP_RUNE8);
            else if (p_spell->code[i]==9) p_spell->code[i]=KeyMap(KEYMAP_RUNE9);
        }
/*
        printf ("\n\nloading spell:\n");
        printf ("code = %d,%d,%d,%d\n",p_spell->code[0],
                                       p_spell->code[1],
                                       p_spell->code[2],
                                       p_spell->code[3]);
        printf ("effect type =%d\n",p_spell->type);
        printf ("effect sub  =%d\n",p_spell->subtype);
        printf ("duration    =%d\n",p_spell->duration);
        printf ("duration mod=%d\n",p_spell->durationmod);
        printf ("power       =%d\n",p_spell->power);
        printf ("power mod   =%d\n",p_spell->powermod);
        printf ("cost        =%d\n",p_spell->cost);
        printf ("cost mod    =%d\n",p_spell->costmod);
        printf ("hardness    =%d\n",p_spell->hardness);
        printf ("hardness mod=%d\n",p_spell->hardnessmod);
        printf ("minimum lev =%d\n",p_spell->minlevel);
        printf ("system      =%d\n",p_spell->system);
        printf ("splash red  =%d\n",p_spell->filtr);
        printf ("splash grn  =%d\n",p_spell->filtg);
        printf ("splash blu  =%d\n",p_spell->filtb);
        printf ("\n\n\n");
        fflush (stdout);
*/
        /* determine if the character can cast this spell */
#ifndef COMPILE_OPTION_LOAD_ALL_SPELLS
        unload = TRUE;
        switch (system)
        {
            case SPELL_SYSTEM_MAGE:
            if (p_spell->system == SPELL_SYSTEM_MAGE ||
                p_spell->system == SPELL_SYSTEM_MAGE_AND_CLERIC ||
                p_spell->system == SPELL_SYSTEM_MAGE_AND_ARCANE ||
                p_spell->system == SPELL_SYSTEM_ANY) unload=FALSE;
            break;

            case SPELL_SYSTEM_CLERIC:
            if (p_spell->system == SPELL_SYSTEM_CLERIC ||
                p_spell->system == SPELL_SYSTEM_MAGE_AND_CLERIC ||
                p_spell->system == SPELL_SYSTEM_CLERIC_AND_ARCANE ||
                p_spell->system == SPELL_SYSTEM_ANY) unload=FALSE;
            break;

            case SPELL_SYSTEM_ARCANE:
            if (p_spell->system == SPELL_SYSTEM_ARCANE ||
                p_spell->system == SPELL_SYSTEM_MAGE_AND_ARCANE ||
                p_spell->system == SPELL_SYSTEM_CLERIC_AND_ARCANE ||
                p_spell->system == SPELL_SYSTEM_ANY) unload=FALSE;
            break;

            default:
            /* fail! improper spell system returned */
            DebugCheck (-1);
        }
#endif

        if (unload == TRUE)
        {
            /* nevermind, we don't want this spell anyway. */
//            printf ("unloading this spell, improper casting class\n");
//            fflush (stdout);
            PictureUnlockAndUnfind (res);
        }
        else
        {
//            printf ("saving this spell in list\n");
//            fflush (stdout);

            /* add spell pointer to linked list */
            DoubleLinkListAddElementAtEnd (G_spellsList, res);
        }
        sprintf (stmp,"SPELDESC/SPL%05d",count++);
    }

	DebugEnd();
}



/*-------------------------------------------------------------------------*
 * Routine:  SpellsFinish
 *-------------------------------------------------------------------------*/
/**
 *  SpellsFinish gets rid of any memory or resources associated with
 *  all the spells, either in effect, or able to be used.
 *
 *<!-----------------------------------------------------------------------*/
T_void SpellsFinish(T_void)
{
	T_word16 i ;
    T_doubleLinkListElement element, nextElement ;
    T_resource res ;

	DebugRoutine ("SpellsFinish");
    DebugCheck(G_isInit == TRUE) ;

    G_isInit = FALSE ;

//    printf ("spells finish called\n");
//    fflush (stdout);

    /* unlock and unfind all spell structure resources */

    element = DoubleLinkListGetFirst(G_spellsList) ;
    while (element != DOUBLE_LINK_LIST_ELEMENT_BAD)
    {
        nextElement = DoubleLinkListElementGetNext(element) ;

        res = DoubleLinkListElementGetData(element) ;

        PictureUnlockAndUnfind(res) ;
        DoubleLinkListRemoveElement(element) ;

        element = nextElement ;
    }

    /* clear globals */
    for (i=0;i<4;i++) G_curspell[i]=0;
    G_clearSpells=FALSE;

	DebugEnd();

}

T_byte8 SpellKeyToRuneID(T_byte8 scankey)
{
    if (scankey == KeyMap(KEYMAP_RUNE1))
        return 1;
    if (scankey == KeyMap(KEYMAP_RUNE2))
        return 2;
    if (scankey == KeyMap(KEYMAP_RUNE3))
        return 3;
    if (scankey == KeyMap(KEYMAP_RUNE4))
        return 4;
    if (scankey == KeyMap(KEYMAP_RUNE5))
        return 5;
    if (scankey == KeyMap(KEYMAP_RUNE6))
        return 6;
    if (scankey == KeyMap(KEYMAP_RUNE7))
        return 7;
    if (scankey == KeyMap(KEYMAP_RUNE8))
        return 8;
    if (scankey == KeyMap(KEYMAP_RUNE9))
        return 9;

    // Unknown
    return 0;
}
/*-------------------------------------------------------------------------*
 * Routine:  SpellsAddRune
 *-------------------------------------------------------------------------*/
/**
 *  Callback routine assigned to a rune button, adds rune to spell box
 *
 *<!-----------------------------------------------------------------------*/
T_void SpellsAddRune (T_buttonID buttonID)
{
	T_buttonStruct *p_button;
	T_word16 i;
	T_word16 manasuck;

	DebugRoutine ("SpellsAddRune");
	DebugCheck (buttonID != NULL);

    if (G_clearSpells==TRUE)
    {
        G_clearSpells=FALSE;
    	SpellsClearRunes (buttonID);
    }

	if (G_curspell[3]!=0)
	{
		ColorSetColor (MANA_BACKCOLOR,55,0,0);
	}
	else if (buttonID != NULL)
	{
		p_button=(T_buttonStruct *)buttonID;
		for (i=0;i<4;i++)
		{
			if (G_curspell[i]==0) //if slot is empty
			{
				manasuck=25;
				StatsChangePlayerMana(-manasuck);
				if (StatsGetPlayerMana()>0)
				{
				  ColorSetColor (MANA_BACKCOLOR,30,0,30);
				  G_curspell[i]=SpellKeyToRuneID((T_byte8)p_button->scancode);   //add scancode to spell key
				  SpellsDrawRuneBox();
				} else ColorSetColor (MANA_BACKCOLOR,55,0,0);
				break;
			}
		}
	}
	DebugEnd();
}


/*-------------------------------------------------------------------------*
 * Routine:  SpellsClearRunes
 *-------------------------------------------------------------------------*/
/**
 *  Callback routine assigned to the rune clearbox button, clears runes
 *  in spell box
 *
 *<!-----------------------------------------------------------------------*/
T_void SpellsClearRunes (T_buttonID buttonID)
{
	T_word16 i;
	T_bitmap *pic ;
	T_resource res ;
	DebugRoutine ("SpellsClearRunes");

	for (i=0;i<4;i++) G_curspell[i]=0;

	pic = (T_bitmap *)PictureLockData("UI/3DUI/SPSTRIP", &res) ;
	DebugCheck(pic != NULL) ;

	if (pic != NULL)
	{
		GrScreenSet(GRAPHICS_ACTUAL_SCREEN) ;
        MouseHide();
		GrDrawBitmap(pic, 176, 181) ;
        MouseShow();
		PictureUnlock(res) ;
	PictureUnfind(res) ;
	}

	DebugEnd();
}



/*-------------------------------------------------------------------------*
 * Routine:  SpellsBackSpace
 *-------------------------------------------------------------------------*/
/**
 *  Callback routine assigned to the rune backspace button, removes last
 *  tune entered.
 *
 *  NOTE: 
 *  Currently same as SpellsClear
 *
 *<!-----------------------------------------------------------------------*/
T_void SpellsBackspace (T_buttonID buttonID)
{
	T_word16 i;
	T_bitmap *pic ;
	T_resource res ;
	DebugRoutine ("SpellsBackspace");

	for (i=0;i<4;i++) G_curspell[i]=0;

	pic = (T_bitmap *)PictureLockData("UI/3DUI/SPSTRIP", &res) ;
	DebugCheck(pic != NULL) ;

	if (pic != NULL)
	{
		GrScreenSet(GRAPHICS_ACTUAL_SCREEN) ;
		GrDrawBitmap(pic, 176, 181) ;
		PictureUnlock(res) ;
		PictureUnfind(res) ;
	}

	DebugEnd();
}


T_byte8 RuneIDToKey(T_byte8 aRuneID)
{
    T_byte8 key = 0;

    DebugRoutine("RundeIDToKey");
    DebugCheck(aRuneID <= 9);

    switch (aRuneID) {
        case 1: key = KeyMap(KEYMAP_RUNE1); break;
        case 2: key = KeyMap(KEYMAP_RUNE2); break;
        case 3: key = KeyMap(KEYMAP_RUNE3); break;
        case 4: key = KeyMap(KEYMAP_RUNE4); break;
        case 5: key = KeyMap(KEYMAP_RUNE5); break;
        case 6: key = KeyMap(KEYMAP_RUNE6); break;
        case 7: key = KeyMap(KEYMAP_RUNE7); break;
        case 8: key = KeyMap(KEYMAP_RUNE8); break;
        case 9: key = KeyMap(KEYMAP_RUNE9); break;
        default: key = 0; break;
    }

    DebugEnd();

    return key;
}

T_byte8 RuneIDToOriginalScanKey(T_byte8 aRuneID)
{
    T_byte8 scankey = 0;

    DebugRoutine("RuneIDToOriginalScanKey");
    DebugCheck(aRuneID <= 9);

    switch (aRuneID) {
        case 1: scankey = KEY_SCAN_CODE_KEYPAD_1; break;
        case 2: scankey = KEY_SCAN_CODE_KEYPAD_2; break;
        case 3: scankey = KEY_SCAN_CODE_KEYPAD_3; break;
        case 4: scankey = KEY_SCAN_CODE_KEYPAD_4; break;
        case 5: scankey = KEY_SCAN_CODE_KEYPAD_5; break;
        case 6: scankey = KEY_SCAN_CODE_KEYPAD_6; break;
        case 7: scankey = KEY_SCAN_CODE_KEYPAD_7; break;
        case 8: scankey = KEY_SCAN_CODE_KEYPAD_8; break;
        case 9: scankey = KEY_SCAN_CODE_KEYPAD_9; break;
        default: scankey = 0; break;
    }

    DebugEnd();

    return scankey;
}

/*-------------------------------------------------------------------------*
 * Routine:  SpellsDrawRuneBox
 *-------------------------------------------------------------------------*/
/**
 *  This routine draws the selected icons for the current spell in the
 *  spell box
 *
 *<!-----------------------------------------------------------------------*/
T_void SpellsDrawRuneBox (T_void)
{
	T_word16 i;
	T_buttonID runebutton;
    E_Boolean pushed=FALSE;
	T_graphicID runepic;

	DebugRoutine ("SpellsDrawRuneBox");

    if (BannerButtonsOk())
    {
	    for (i=0;i<4;i++)
	    {
		    if (G_curspell[i]!=0)
		    {
		         runebutton=ButtonGetByKey(RuneIDToKey(G_curspell[i]));
                 pushed=ButtonIsPushed(runebutton);
                 if (pushed==FALSE)
                 {
                    ButtonDownNoAction (runebutton);
                 }
		         runepic=ButtonGetGraphic(runebutton);
                 GraphicSetShadow (runepic,255);
                 MouseHide();
		         GraphicDrawAt (runepic,177+(i*15),182);
                 MouseShow();
                 if (pushed==FALSE)
                 {
                    ButtonUpNoAction (runebutton);
                 }
		    }
	    }
    }
	DebugEnd();
}


/*-------------------------------------------------------------------------*
 * Routine:  SpellsCastSpell
 *-------------------------------------------------------------------------*/
/**
 *  This routine attempts to cast the currently selected spell
 *
 *<!-----------------------------------------------------------------------*/
#if 0
T_void SpellsCastSpell (T_buttonID buttonID)
{
    DebugRoutine ("SpellsCastSpell");

    /* start sound sequence */
    G_curSound=0;

    MessagePrintf ("code=%d %d %d %d",G_curspell[0],G_curspell[1],G_curspell[2],G_curspell[3]);

    SpellsMakeNextSound (NULL);

    DebugEnd();
}

T_void SpellsMakeNextSound (T_void *p_data)
{
    T_word16 soundNum;

    DebugRoutine ("SpellsMakeNextSound");


    if (StatsGetPlayerSpellSystem()==SPELL_SYSTEM_CLERIC )
    {
        if (G_curSound==3)
        {
            soundNum=G_curspell[G_curSound]+SOUND_SPELL_CLERIC_B_SET - 70;
        }
        else if (G_curspell[G_curSound+1]==0)
        {
            soundNum=G_curspell[G_curSound]+SOUND_SPELL_CLERIC_B_SET - 70;
        }
        else
        {
            soundNum=G_curspell[G_curSound]+SOUND_SPELL_CLERIC_A_SET - 70;
        }
    }
    else
    {
        if (G_curSound==3)
        {
            soundNum=G_curspell[G_curSound]+SOUND_SPELL_MAGE_B_SET - 70;
        }
        else if (G_curspell[G_curSound+1]==0)
        {
            soundNum=G_curspell[G_curSound]+SOUND_SPELL_MAGE_B_SET - 70;
        }
        else
        {
            soundNum=G_curspell[G_curSound]+SOUND_SPELL_MAGE_A_SET - 70;
        }
    }

    if (G_curSound == 3)
    {
        SoundPlayByNumber(soundNum,255);
        SpellsFinishSpell();
    }
    else if (G_curspell[G_curSound+1]==0)
    {
        SoundPlayByNumber(soundNum,255);
        SpellsFinishSpell();
    }
    else
    {
        MessagePrintf ("cs=%d G_cursp=%d",G_curSound,G_curspell[G_curSound]);
        MessagePrintf ("playing sound %d",soundNum);
        SoundPlayByNumberWithCallback(
            soundNum,
            255,
            SpellsMakeNextSound,
            NULL) ;
    }

    G_curSound++;

    DebugEnd();
}

#endif

T_void SpellsCastSpell (T_buttonID buttonID)
{
	E_Boolean success;
    T_doubleLinkListElement element;
    T_resource res;
    T_spellStruct *p_spell;
    T_word32 spellpower, spellduration, spellcost;
    T_sword16 spelldif;
    T_byte8 charlevel;
	T_word32 i;

	DebugRoutine ("SpellsCastSpell");

    /* search through spell list and see if any spell */
    /* matches current spell code */

    if (ClientIsPaused())  {
        MessageAdd("Cannot cast spells while paused.") ;
    } else if (ClientIsDead())  {
        MessageAdd("Dead players do not cast spells.") ;
    } else {
        element = DoubleLinkListGetFirst (G_spellsList);

        while (element != DOUBLE_LINK_LIST_ELEMENT_BAD)
        {
            /* get a pointer to this spell's spell struct */
            res = DoubleLinkListElementGetData(element);
            p_spell = (T_spellStruct *)PictureLockDataQuick (res);
            DebugCheck (p_spell != NULL);

            /* check to see if code matches */
            success=TRUE;
            for (i=0;i<4;i++)
            {
                if (p_spell->code[i]!=RuneIDToKey(G_curspell[i]))
                {
                    success=FALSE;
                    break;
                }
            }

            if (success==TRUE)
            {
                /* get level of character */
                charlevel = StatsGetPlayerLevel();

                /* figure duration of spell */
                spellduration = p_spell->duration + (p_spell->durationmod*charlevel);
                if (spellduration > MAX_EFFECT_DURATION) spellduration = MAX_EFFECT_DURATION;

                /* figure power of spell */
                spellpower = p_spell->power + (p_spell->powermod*charlevel);
                if (spellpower > MAX_EFFECT_POWER) spellpower = MAX_EFFECT_POWER;

                /* figure casting cost of spell */
                spellcost = p_spell->cost + (p_spell->costmod*charlevel);

                /* figure difficulty of spell */
                spelldif = StatsGetPlayerAttribute(ATTRIBUTE_MAGIC) +
                           p_spell->hardness + (2 * charlevel);
                if (spelldif < 0) spelldif = 0;

                /* cast this spell if possible */
                if (((StatsGetManaLeft() > (T_sword32)spellcost) && (charlevel >= p_spell->minlevel)) ||
                    (EffectPlayerEffectIsActive (PLAYER_EFFECT_GOD_MODE) == TRUE))
                {
                    success=TRUE;

                    /* check for a d100 agains hardness for sucess */
                    if (EffectPlayerEffectIsActive (PLAYER_EFFECT_GOD_MODE) == FALSE)
                    {
                        if (rand()%100 > spelldif)
                        {
                            /* failed roll */
    //                        success = FALSE;
                            MessageAdd ("Your spell fizzled");
                            ColorAddGlobal (10,10,-10);
                            SoundPlayByNumber (SOUND_SPELL_FIZZLE,255);
                            /* remove half the spell cost */
                            StatsChangePlayerMana (-p_spell->cost>>1);
                            PictureUnlock(res);
                            break;
                        }
                    }

                    if (success==TRUE)
                    {
                        if (EffectPlayerEffectIsActive(PLAYER_EFFECT_GOD_MODE)==FALSE)
                        {
                            StatsChangePlayerMana (-p_spell->cost);
                        }
                        /* do a color effect */
                        ColorAddGlobal ((T_sbyte8)p_spell->filtr>>1,(T_sbyte8)p_spell->filtg>>1,(T_sbyte8)p_spell->filtb>>1);

                        /* create spell effect */
                        Effect (p_spell->type,
                            EFFECT_TRIGGER_CAST,
                            p_spell->subtype,
                            (T_word16)spellduration,
                            (T_word16)spellpower,
                            p_spell);

                        /* trigger the spell sound */
                        if (p_spell->sound != 0)
                        {
                            SoundPlayByNumber (p_spell->sound,255);
                        }
                        PictureUnlock(res);
                        break;
                    }
                }
                else
                {
                    MessageAdd ("^003You are too exhausted to cast this spell.");
                    success=TRUE;
                    PictureUnlock(res);
                    break;
                }

                /* spell success, break from while loop */
                /* nevermind, check for other spells with same code */
    //			break;
		    }

            PictureUnlock(res);
            element = DoubleLinkListElementGetNext (element);
	    }
        if (success==FALSE && G_curspell[0]!=0)
        {
            MessageAdd ("^003You feel something is just not right.");
            SpellsClearRunes(NULL);
        }
        /* set flag to clear runes on next rune entered */
        else G_clearSpells=TRUE;
    }

	DebugEnd();
}


/*-------------------------------------------------------------------------*
 * Routine:  SpellsStopAll
 *-------------------------------------------------------------------------*/
/**
 *  SpellsStopAll turns off all the spells that are in effect.
 *
 *<!-----------------------------------------------------------------------*/
T_void SpellsStopAll (T_void)
{
    DebugRoutine("SpellsStopAll") ;

//    for (i=0; i<NUM_SPELLS; i++)
//        SpellsStop(G_spells+i) ;

	DebugEnd();
}



T_void SpellsDrawInEffectRunes (T_word16 left,
								T_word16 right,
								T_word16 top,
								T_word16 bottom)
{
	E_Boolean drawed=FALSE;

	DebugRoutine  ("SpellsDrawInEffectRunes");
	DebugEnd();
}

T_void SpellsSetRune (E_spellsRuneType type)
{
    DebugRoutine ("SpellsAddRune");


    /* check to see if this rune is valid for the characters */
    /* current spell system */
    if (StatsGetPlayerSpellSystem()==SPELL_SYSTEM_MAGE)
    {
        /* valid runes are >=RUNE_ARCANE_1, <=RUNE_MAGE_5 */
        if (type >=RUNE_ARCANE_1 && type <= RUNE_MAGE_5)
        {
            /* valid rune, increment counter */
            StatsIncrementRuneCount (type);
        }
    } else if (StatsGetPlayerSpellSystem()==SPELL_SYSTEM_CLERIC)
    {
        if (type >= RUNE_ARCANE_5 && type <= RUNE_CLERIC_4)
        {
            StatsIncrementRuneCount (type - RUNE_ARCANE_5);
        }
    } else
    {
        /* arcane spell system */
        if (type >= RUNE_ARCANE_1 && type <= RUNE_ARCANE_4)
        {
            StatsIncrementRuneCount (type - RUNE_ARCANE_1);
        }
        else if (type >= RUNE_ARCANE_5 && type <= RUNE_ARCANE_9)
        {
            StatsIncrementRuneCount (type - RUNE_ARCANE_5 + 4);
        }
    }

    DebugEnd();
}


T_void SpellsClearRune (E_spellsRuneType type)
{
    DebugRoutine ("SpellsClearRune");

    /* check to see if this rune is valid for the characters */
    /* current spell system */
    if (StatsGetPlayerSpellSystem()==SPELL_SYSTEM_MAGE)
    {
        /* valid runes are >=RUNE_ARCANE_1, <=RUNE_MAGE_5 */
        if (type >=RUNE_ARCANE_1 && type <= RUNE_MAGE_5)
        {
            /* valid rune, increment counter */
            StatsDecrementRuneCount (type);
        }
    } else if (StatsGetPlayerSpellSystem()==SPELL_SYSTEM_CLERIC)
    {
        if (type >= RUNE_ARCANE_5 && type <= RUNE_CLERIC_4)
        {
            StatsDecrementRuneCount (type - RUNE_ARCANE_5);
        }
    } else
    {
        /* arcane spell system */
        if (type >= RUNE_ARCANE_1 && type <= RUNE_ARCANE_4)
        {
            StatsDecrementRuneCount (type - RUNE_ARCANE_1);
        }
        else if (type >= RUNE_ARCANE_5 && type <= RUNE_ARCANE_9)
        {
            StatsDecrementRuneCount (type - RUNE_ARCANE_5 + 4);
        }
    }

    DebugEnd();
}

/* @} */
/*-------------------------------------------------------------------------*
 * End of File:  SPELLS.C
 *-------------------------------------------------------------------------*/
