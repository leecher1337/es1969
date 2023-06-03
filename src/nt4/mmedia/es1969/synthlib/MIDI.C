/*
 *
 * Copyright (c) 1992 Microsoft Corporation
 *
 */

/*
 * midi.c
 *
 * Midi FM Synthesis routines. converts midi messages into calls to
 * FM Synthesis functions  - currently supports base adlib (in adlib.c)
 * and opl3 synthesisers (in opl3.c).
 *
 * 15 Dec 92 Geraint Davies - based on a combination of the adlib
 *                            and WSS midi drivers.
 */


#include <windows.h>
#include <mmsystem.h>

#include "mmddk.h"
#include "driver.h"
#include "natv.h"

/* typedefs for MIDI patches */
#define PATCH_SIZE              8258

/***********************************************************
global memory */


PORTALLOC gMidiInClient;     // input client information structure

BYTE  status = 0;
BYTE    gbMidiInUse = 0;                /* if MIDI is in use */

static WORD  wMidiOutEntered = 0;    // reentrancy check
static PORTALLOC gMidiOutClient;     // client information


BYTE * gBankMem = NULL;  /* points to the patches */

/* --- interface functions ---------------------------------- */


/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | midiSynthCallback | This calls DriverCallback for a midi device.
 *
 * @parm NPPORTALLOC| pPort | Pointer to the PORTALLOC.
 *
 * @parm WORD | msg | The message to send.
 *
 * @parm DWORD | dw1 | Message-dependent parameter.
 *
 * @parm DWORD | dw2 | Message-dependent parameter.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void NEAR PASCAL midiSynthCallback(NPPORTALLOC pPort, WORD msg, DWORD dw1, DWORD dw2)
{

    // invoke the callback function, if it exists.  dwFlags contains driver-
    // specific flags in the LOWORD and generic driver flags in the HIWORD
    if (pPort->dwCallback)
        DriverCallback(pPort->dwCallback,       // client's callback DWORD
            HIWORD(pPort->dwFlags) | DCB_NOSWITCH,  // callback flags
            (HDRVR)pPort->hMidi,     // handle to the wave device
            msg,                     // the message
            pPort->dwInstance,       // client's instance data
            dw1,                     // first DWORD
            dw2);                    // second DWORD
}

/****************************************************************************

    This function conforms to the standard MIDI output driver message proc
    modMessage, which is documented in mmddk.d.

 ***************************************************************************/
DWORD APIENTRY modSynthMessage(UINT id,
        UINT msg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    LPMIDIHDR    lpHdr;
    LPSTR           lpBuf;          /* current spot in the long msg buf */
    DWORD           dwBytesRead;    /* how far are we in the buffer */
    DWORD           dwMsg = 0;      /* short midi message sent to synth */
    BYTE            bBytePos=0;     /* shift current byte by dwBytePos*s */
    BYTE            bBytesLeft = 0; /* how many dat bytes needed */
    BYTE            curByte;        /* current byte in long buffer */
    UINT            mRc;            /* Return code */


    // this driver only supports one device
    if (id != 0) {
        D1(("invalid midi device id"));
        return MMSYSERR_BADDEVICEID;
    }

    switch (msg) {
        case MODM_GETNUMDEVS:
            D1(("MODM_GETNUMDEVS"));
        //
        // Check if the kernel driver got loaded OK
        //
        mRc = sndGetNumDevs(SYNTH_DEVICE);
                break;

        case MODM_GETDEVCAPS:
            D1(("MODM_GETDEVCAPS"));
            mRc = midiGetDevCaps(0, SYNTH_DEVICE, (LPBYTE)dwParam1, (WORD)dwParam2);
            break;

        case MODM_OPEN:
            D1(("MODM_OPEN"));

            /* open the midi */
            if (MidiOpen())
                return MMSYSERR_ALLOCATED;

            // save client information
            gMidiOutClient.dwCallback = ((LPMIDIOPENDESC)dwParam1)->dwCallback;
            gMidiOutClient.dwInstance = ((LPMIDIOPENDESC)dwParam1)->dwInstance;
            gMidiOutClient.hMidi      = (HMIDIOUT)((LPMIDIOPENDESC)dwParam1)->hMidi;
            gMidiOutClient.dwFlags    = dwParam2;

            // notify client
            midiSynthCallback(&gMidiOutClient, MOM_OPEN, 0L, 0L);

            /* were in use */
            gbMidiInUse = TRUE;

            mRc = 0L;
                break;

        case MODM_CLOSE:
            D1(("MODM_CLOSE"));

            /* shut up the FM synthesizer */
            MidiClose();

            // notify client
            midiSynthCallback(&gMidiOutClient, MOM_CLOSE, 0L, 0L);

            /* were not used any more */
            gbMidiInUse = FALSE;

            mRc = 0L;
                break;

        case MODM_RESET:
            D1(("MODM_RESET"));

            //
            //  turn off FM synthesis
            //
            //  note that we increment our 're-entered' counter so that a
            //  background interrupt handler doesn't mess up our resetting
            //  of the synth by calling midiOut[Short|Long]Msg.. just
            //  practicing safe midi. NOTE: this should never be necessary
            //  if a midi app is PROPERLY written!
            //
            wMidiOutEntered++;
            {
                if (wMidiOutEntered == 1)
                {
                    MidiReset();
                    dwParam1 = 0L;
                }
                else
                {
                    D1(("MODM_RESET reentered!"));
                    dwParam1 = MIDIERR_NOTREADY;
                }
            }
            wMidiOutEntered--;
            mRc = (dwParam1);
                break;

        case MODM_DATA:                    // message is in dwParam1
            MidiCheckVolume();             // See if the volume has changed

            // make sure we're not being reentered
            wMidiOutEntered++;
            if (wMidiOutEntered > 1) {
                        D1(("MODM_DATA reentered!"));
                        wMidiOutEntered--;
                        return MIDIERR_NOTREADY;
                    }

            /* if have repeated messages */
            if (dwParam1 & 0x00000080)  /* status byte */
                        status = LOBYTE(LOWORD(dwParam1));
            else
                        dwParam1 = (dwParam1 << 8) | ((DWORD) status);

            /* if not, have an FM synthesis message */
            MidiMessage (dwParam1);

            wMidiOutEntered--;
            mRc = 0L;
                break;

        case MODM_LONGDATA:      // far pointer to header in dwParam1

            MidiCheckVolume();   // See if the volume has changed

            // make sure we're not being reentered
            wMidiOutEntered++;
            if (wMidiOutEntered > 1) {
                        D1(("MODM_LONGDATA reentered!"));
                        wMidiOutEntered--;
                        return MIDIERR_NOTREADY;
                        }

            // check if it's been prepared
            lpHdr = (LPMIDIHDR)dwParam1;
            if (!(lpHdr->dwFlags & MHDR_PREPARED)) {
                        wMidiOutEntered--;
                        return MIDIERR_UNPREPARED;
                        }

            lpBuf = lpHdr->lpData;
            dwBytesRead = 0;
            curByte = *lpBuf;

            while (TRUE) {
                        /* if its a system realtime message send it and continue
                                this does not affect the running status */

                        if (curByte >= 0xf8)
                                MidiMessage (0x000000ff & curByte);
                        else if (curByte >= 0xf0) {
                                status = 0;     /* kill running status */
                                dwMsg = 0L;     /* throw away any incomplete data */
                                bBytePos = 0;   /* start at beginning of message */

                                switch (curByte) {
                                        case 0xf0:      /* sysex - ignore */
                                        case 0xf7:
                                                break;
                                        case 0xf4:      /* system common, no data */
                                        case 0xf5:
                                        case 0xf6:
                                                MidiMessage (0x000000ff & curByte);
                                                break;
                                        case 0xf1:      /* system common, one data byte */
                                        case 0xf3:
                                                dwMsg |= curByte;
                                                bBytesLeft = 1;
                                                bBytePos = 1;
                                                break;
                                        case 0xf2:      /* system common, 2 data bytes */
                                                dwMsg |= curByte;
                                                bBytesLeft = 2;
                                                bBytePos = 1;
                                                break;
                                        };
                                }
                    /* else its a channel message */
                    else if (curByte >= 0x80) {
                                status = curByte;
                                dwMsg = 0L;

                                switch (curByte & 0xf0) {
                                        case 0xc0:      /* channel message, one data */
                                        case 0xd0:
                                                dwMsg |= curByte;
                                                bBytesLeft = 1;
                                                bBytePos = 1;
                                                break;
                                        case 0x80:      /* two bytes */
                                        case 0x90:
                                        case 0xa0:
                                        case 0xb0:
                                        case 0xe0:
                                                dwMsg |= curByte;
                                                bBytesLeft = 2;
                                                bBytePos = 1;
                                                break;
                                        };
                                }

                        /* else if its an expected data byte */
                    else if (bBytePos != 0) {
                                dwMsg |= ((DWORD)curByte) << (bBytePos++ * 8);
                                if (--bBytesLeft == 0) {

                                                MidiMessage (dwMsg);

                                        if (status) {
                                                dwMsg = status;
                                                bBytesLeft = bBytePos - (BYTE)1;
                                                bBytePos = 1;
                                                }
                                        else {
                                                dwMsg = 0L;
                                                bBytePos = 0;
                                                };
                                        };
                                };

            /* read the next byte if there is one */
            /* remember we have already read and processed one byte that
             * we have not yet counted- so we need to pre-inc, not post-inc
             */
            if (++dwBytesRead >= lpHdr->dwBufferLength) break;
                curByte = *++lpBuf;
            };      /* while TRUE */

            /* return buffer to client */
            lpHdr->dwFlags |= MHDR_DONE;
            midiSynthCallback (&gMidiOutClient, MOM_DONE, dwParam1, 0L);

            wMidiOutEntered--;
            mRc = 0L;
                break;

        case MODM_SETVOLUME:
                mRc = MidiSetVolume(LOWORD(dwParam1) << 16, HIWORD(dwParam1) << 16);
                break;

        case MODM_GETVOLUME:
                mRc = MidiGetVolume((LPDWORD)dwParam1);
                break;

        default:
            return MMSYSERR_NOTSUPPORTED;
    }
    MidiFlush();

    return mRc;


// should never get here...
return MMSYSERR_NOTSUPPORTED;
}

#ifdef LOAD_PATCHES
static TCHAR BCODE gszIniKeyPatchLib[]       = INI_STR_PATCHLIB;
static TCHAR BCODE gszIniDrvSection[]        = INI_DRIVER;
static TCHAR BCODE gszIniDrvFile[]           = INI_SOUND;
static TCHAR BCODE gszSysIniSection[]        = TEXT("synth.dll");
static TCHAR BCODE gszSysIniFile[]           = TEXT("System.Ini");

/** static DWORD NEAR PASCAL DrvGetProfileString(LPSTR szKeyName, LPSTR szDef, LPSTR szBuf, UINT wBufLen)
 *
 *  DESCRIPTION:
 *
 *
 *  ARGUMENTS:
 *      (LPSTR szKeyName, LPSTR szDef, LPSTR szBuf, WORD wBufLen)
 *              HINT    wSystem - if TRUE write/read to system.ini
 *
 *  RETURN (static DWORD NEAR PASCAL):
 *
 *
 *  NOTES:
 *
 ** cjp */

static DWORD NEAR PASCAL DrvGetProfileString(LPTSTR szKeyName, LPTSTR szDef, LPTSTR szBuf, UINT wBufLen,
        UINT wSystem)
{
    return GetPrivateProfileString(wSystem ? gszSysIniSection : gszIniDrvSection, szKeyName, szDef,
                szBuf, wBufLen, wSystem ? gszSysIniFile : gszIniDrvFile);
} /* DrvGetProfileString() */
#endif // LOAD_PATCHES



/****************************************************************
MidiInit - Initializes the FM synthesis chip and internal
        variables. This assumes that HwInit() has been called
        and that a card location has been found. This loads in
        the patch information.

inputs
        none
returns
        WORD - 0 if successful, else error
*/
WORD FAR PASCAL MidiInit (VOID)
{
    HANDLE      hFile;
    DWORD       dwRead;
    TCHAR       szPatchLib[STRINGLEN];     /* patch libarary */

    D1 (("\nMidiInit"));

    /* Check we haven't already initialized */
    if (gBankMem != NULL) {
        return 0;
    }


    /* allocate the memory, and fill it up from the patch
     * library.
     */
    gBankMem = (BYTE*)GlobalLock(GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE|GMEM_ZEROINIT, PATCH_SIZE));
    if (!gBankMem) {
        D1 (("Opl3_Init: could not allocate patch container memory!"));
        return ERROR_OUTOFMEMORY;
    }

#ifdef LOAD_PATCHES
    /* should the load patches be moved to the init section? */
    DrvGetProfileString(gszIniKeyPatchLib, TEXT(""),
        szPatchLib, sizeof(szPatchLib), FALSE);



   if (lstrcmpi( (LPTSTR) szPatchLib, TEXT("") ) == 0)
#endif // LOAD_PATCHES
   {
      HRSRC   hrsrcPatches ;
      HGLOBAL hPatches ;
      LPTSTR  lpPatches ;

      hrsrcPatches =
         FindResource( hDriverModule1, MAKEINTRESOURCE( DATA_FMPATCHES ),
                       RT_BINARY ) ;

      if (NULL != hrsrcPatches)
      {
         hPatches = LoadResource( hDriverModule1, hrsrcPatches ) ;
         lpPatches = LockResource( hPatches ) ;
         AsMemCopy( gBankMem, lpPatches,
                    PATCH_SIZE ) ;
         UnlockResource( hPatches ) ;
         FreeResource( hPatches ) ;
      }
      else
      {
         TCHAR  szAlert[ 50 ] ;
         TCHAR  szErrorBuffer[ 255 ] ;

         LoadString( hDriverModule1, SR_ALERT, szAlert, sizeof( szAlert ) / sizeof(TCHAR)) ;
         LoadString( hDriverModule1, SR_ALERT_NORESOURCE, szErrorBuffer,
                     sizeof( szErrorBuffer ) / sizeof(TCHAR) ) ;
         MessageBox( NULL, szErrorBuffer, szAlert, MB_OK|MB_ICONHAND ) ;
      }
   }
#ifdef LOAD_PATCHES
   else
   {

    hFile = CreateFile (szPatchLib, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile) {
        if (!ReadFile (hFile, gBankMem, PATCH_SIZE, &dwRead, NULL)) {
            D1(("Bad ReadFile"));
        }

        CloseHandle (hFile, 0);
    } else {

        TCHAR   szAlert[50];
        TCHAR   szErrorBuffer[255];

        LoadString(hDriverModule1, SR_ALERT, szAlert, sizeof(szAlert));
        LoadString(hDriverModule1, SR_ALERT_NOPATCH, szErrorBuffer, sizeof(szErrorBuffer));
        MessageBox(NULL, szErrorBuffer, szAlert, MB_OK|MB_ICONHAND);
        D1 (("Bad mmioOpen"));
    }

   }
#endif // LOAD_PATCHES


    return 0;       /* done */
}


/*****************************************************************
MidiOpen - This should be called when a midi file is opened.
        It initializes some variables and locks the patch global
        memories.

inputs
        none
returns
        UINT - 0 if succedes, else error
*/
UINT FAR PASCAL MidiOpen (VOID)
{
    MMRESULT mRc;

    D1(("MidiOpen"));


    //
    // For 32-bit we must open our kernel device
    //

    mRc = MidiOpenDevice(&MidiDeviceHandle, TRUE);

    if (mRc != MMSYSERR_NOERROR) {
            return mRc;
    }


    /*
     * reset the device (set default channel attenuation etc)
     */
    fmreset();

    return 0;
}

/***************************************************************
MidiClose - This kills the playing midi voices and closes the kernel driver

inputs
        none
returns
        none
*/
VOID FAR PASCAL MidiClose (VOID)
{

    D1(("MidiClose"));

    if (MidiDeviceHandle == NULL) {
        return;
    }

    /* make sure all notes turned off */
    MidiAllNotesOff();

    MidiCloseDevice(MidiDeviceHandle);

    MidiDeviceHandle = NULL;
}

/** void FAR PASCAL MidiReset(void)
 *
 *  DESCRIPTION:
 *
 *
 *  ARGUMENTS:
 *      (void)
 *
 *  RETURN (void FAR PASCAL):
 *
 *
 *  NOTES:
 *
 ** cjp */

void FAR PASCAL MidiReset(void)
{

    D1(("MidiReset"));

    /* silence the board and reset board-specific variables */
    fmreset();


} /* MidiReset() */


