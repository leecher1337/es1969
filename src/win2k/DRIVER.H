/****************************************************************************
 *
 *   driver.h
 *
 *   Copyright (c) 2023 leecher@dose.0wnz.at  All rights reserved.
 *
 ***************************************************************************/

//
// Porting stuff
//

#define BCODE

#define fEnabled TRUE

#define AsULMUL(a, b) ((DWORD)((DWORD)(a) * (DWORD)(b)))
#define AsLSHL(a, b) ((DWORD)((DWORD)(a) << (DWORD)(b)))
#define AsULSHR(a, b) ((DWORD)((DWORD)(a) >> (DWORD)(b)))

#define AsMemCopy CopyMemory

#define SYNTH_DATA_SIZE 80

#define NUM2VOICES              (18)            /* # 2operator voices */
#define OPS_PER_CHAN			(4)				/* Operators per channel */

extern BYTE gbVelocityAtten[32];

//
// End of porting stuff
//

/*
 * midi device type - determined by kernel driver
 */
extern UINT gMidiType;
/*
 * values for gMidiType - set in MidiOpenDevice
 */
#define TYPE_ADLIB	1
#define TYPE_OPL3	2


/* volume defines */
#define VOL_MIDI                (0)
#define VOL_NUMVOL              (1)

#define VOL_LEFT                (0)
#define VOL_RIGHT               (1)

/* MIDI defines */

#define NUMCHANNELS                     (16)
#define NUMPATCHES                      (256)
#define DRUMCHANNEL                     (9)     /* midi channel 10 */

/****************************************************************************

       typedefs

 ***************************************************************************/

#define VOICEFLAG_SETUP	1
#define VOICEFLAG_VOICEOFF 2
#define VOICEFLAG_HOLD	4
#define VOICEFLAG_2NDVOICE 8

typedef struct _voiceStruct {
        BYTE    flags1;					/* Flags from above */
        USHORT  wTime;                  /* time that was turned on/off;
                                           0 time indicates that its not in use */
        BYTE    bChannel;               /* channel played on */
        BYTE    bNote;                  /* note played */
        BYTE    bVelocity;              /* velocity */
        USHORT  detune[OPS_PER_CHAN];   /* original pitch, for pitch bend */
        BYTE    bPatch;                 /* what patch is the note,
                                           drums patch = drum note + 128 */
        BYTE    reg5[OPS_PER_CHAN];		/* Delay, block */
        BYTE    reg1[OPS_PER_CHAN];		/* KSL and Attenuation*/
} voiceStruct;

// The PINs
enum
{
    WAVEIN_DEST = 0,
    SYNTH_SOURCE,
    CD_SOURCE,
    LINEIN_SOURCE,
    MIC_SOURCE,
    AUX_SOURCE,
    IIS_SOURCE,
    MIXER_SOURCE,
    PCSPEAKER_SOURCE,
    PHONE_SOURCE,
    LINEOUT_DEST,
    WAVEIN_PIN,
    PIN_TOP_ELEMENT
};

// The Volumes
enum
{
	WAVEOUT_VOLUME = 0,
	WAVEOUT_MUTE,
	SYNTH_VOLUME,
	SYNTH_WAVEIN_MUTE,
	CD_VOLUME,
	CD_LINEOUT_MUTE,
	LINEIN_VOLUME,
	LINEIN_LINEOUT_MUTE,
	MIC_VOLUME,
	MIC_LINEOUT_MUTE,
	AUX_LINEOUT_VOLUME,
	AUX_LINEOUT_MUTE,
	IIS_LINEOUT_VOLUME,
	IIS_MUTE,
	PC_SPEAKER_VOLUME,
	PC_SPEAKER_MUTE,
	PHONE_VOLUME,
	PHONE_MUTE,
	MIDI_IN_VOLUME,
	CD_IN_VOLUME,
	LINE_IN_VOLUME,
	MIC_IN_VOLUME,
	GAIN_20DB,
	AUX_IN_VOLUME,
	IIS_IN_VOLUME,
	MIXER_VOLUME,
	LINEOUT_MIX,
	LINEOUT_VOL,
	LINEOUT_MUTE,
	GAIN_3DEFFECT,
	RECORDING_SOURCE,
	RECMON
};



/****************************************************************************

       strings

 ***************************************************************************/

#define INI_STR_PATCHLIB L"Patches"


// synth.h 
 // indexed FM registers

#define AD_LSI                          (0x000)
#define AD_LSI2                         (0x101)
#define AD_TIMER1                       (0x001)
#define AD_TIMER2                       (0x002)
#define AD_MASK                         (0x004)
#define AD_CONNECTION                   (0x104)
#define AD_NEW                          (0x105)
#define AD_NTS                          (0x008)
#define AD_MULT                         (0x020)
#define AD_MULT2                        (0x120)
#define AD_LEVEL                        (0x040)
#define AD_LEVEL2                       (0x140)
#define AD_AD                           (0x060)
#define AD_AD2                          (0x160)
#define AD_SR                           (0x080)
#define AD_SR2                          (0x180)
#define AD_FNUMBER                      (0x0a0)
#define AD_FNUMBER2                     (0x1a0)
#define AD_BLOCK                        (0x0b0)
#define AD_BLOCK2                       (0x1b0)
#define AD_DRUM                         (0x0bd)
#define AD_FEEDBACK                     (0x0c0)
#define AD_FEEDBACK2                    (0x1c0)
#define AD_WAVE                         (0x0e0)
#define AD_WAVE2                        (0x1e0)


/*
 *  Stucture for passing synth data
 *  Why on earth isn't there a type for sharing short data?
 */

typedef struct {
    unsigned short IoPort;
    unsigned short PortData;
} SYNTH_DATA, * PSYNTH_DATA;
