#ifndef _COMMON_H_
#define _COMMON_H_

#define PC_IMPLEMENTATION
#include "stdunk.h"
#include "portcls.h"
#include "DMusicKS.h"
#include "ksdebug.h"
#include "kcom.h"

#pragma warning(disable:4127) //warning C4127: conditional expression is constant -DbgPrintF


// Reference: https://www.alsa-project.org/files/pub/manuals/ess/DsSolo1.pdf 

/* IO device offsets */
#define ESSIO_REG_AUDIO2DMAADDR         0
#define ESSIO_REG_AUDIO2DMACOUNT        4
#define ESSIO_REG_AUDIO2MODE            6
#define ESSIO_REG_IRQCONTROL            7

/* DMA offsets */
#define ESSDM_REG_DMAADDR               0x00
#define ESSDM_REG_DMACOUNT              0x04
#define ESSDM_REG_DMACOMMAND            0x08
#define ESSDM_REG_DMASTATUS             0x08
#define ESSDM_REG_DMAMODE               0x0b
#define ESSDM_REG_DMACLEAR              0x0d
#define ESSDM_REG_DMAMASK               0x0f

/* DSP offsets */
#define ESSSB_REG_FMLOWADDR             0x00    /* 20-voice FM synthesizer. */
#define ESSSB_REG_FMHIGHADDR            0x02
#define ESSSB_REG_MIXERADDR             0x04    /* Mixer Address register (port for address of mixer controller registers). */      
#define ESSSB_REG_MIXERDATA             0x05    /* Mixer Data register (port for data to/from mixer controller registers). */

#define ESSSB_REG_RESET                 0x06    /* Audio reset and status flags */
#define ESSSB_REG_POWER                 0x07    /* Power Management register. Suspend request and FM reset. */

#define ESSSB_REG_READDATA              0x0a    /* Input data from read buffer for command/data I/O. */
#define ESSSB_REG_WRITEDATA             0x0c    /* Output data to write buffer for command/data I/O. */
#define ESSSB_REG_READSTATUS            0x0c    /* Read embedded processor status */

#define ESSSB_REG_STATUS                0x0e    /* Data available flag from embedded processor */

/* SB Commands */
#define ESS_CMD_EXTSAMPLERATE           0xa1
#define ESS_CMD_FILTERDIV               0xa2
#define ESS_CMD_DMACNTRELOADL           0xa4
#define ESS_CMD_DMACNTRELOADH           0xa5
#define ESS_CMD_ANALOGCONTROL           0xa8
#define ESS_CMD_IRQCONTROL              0xb1
#define ESS_CMD_DRQCONTROL              0xb2
#define ESS_CMD_RECLEVEL                0xb4
#define ESS_CMD_SETFORMAT               0xb6
#define ESS_CMD_SETFORMAT2              0xb7
#define ESS_CMD_DMACONTROL              0xb8
#define ESS_CMD_DMATYPE                 0xb9
#define ESS_CMD_OFFSETLEFT              0xba
#define ESS_CMD_OFFSETRIGHT             0xbb
#define ESS_CMD_READREG                 0xc0
#define ESS_CMD_ENABLEEXT               0xc6
#define ESS_CMD_PAUSEDMA                0xd0
#define ESS_CMD_ENABLEAUDIO1            0xd1
#define ESS_CMD_STOPAUDIO1              0xd3
#define ESS_CMD_AUDIO1STATUS            0xd8
#define ESS_CMD_CONTDMA                 0xd4
#define ESS_CMD_TESTIRQ                 0xf2

//
// MPU401 ports (page 31)
//
#define MPU401_REG_STATUS   0x01    // Status register
#define MPU401_DRR          0x40    // Output ready (for command or data)
#define MPU401_DSR          0x80    // Input ready (for data)
#define MPU401_ACK          0xFE    // ACKnowledge

#define MPU401_REG_DATA     0x00    // Data in
#define MPU401_REG_COMMAND  0x01    // Commands
#define MPU401_CMD_RESET    0xFF    // Reset command
#define MPU401_CMD_UART     0x3F    // Switch to UART mod




/* PCI Register (page 20) */
#define ESM_CMD                     0x04   /* Command */
#define ESM_GAMEPORT                0x20   /* Game port I/O space base address for native PCI audio */
#define ESM_IRQLINE                 0x3C   /* Interrput line */
#define ESM_LEGACY_AUDIO_CONTROL    0x40   /* Legacy audio control */
#define ESM_CONFIG		            0x50   /* Solo-1 configuration */
#define ESM_ACPI_COMMAND	        0x54
#define ESM_DDMA		            0x60   /* Distributed DMA control */
#define ESM_PMSTATUS                0xC4   /* Power-Management control/status */

/* MIXER Register (page 51) */
#define ESM_MIXER_AUDIO1_VOL        0x14   /* Audio 1 play volume */
#define ESM_MIXER_MICMIX_VOL        0x1A   /* Mic mix volume */
#define ESM_MIXER_EXTENDEDRECSRC    0x1C   /* Extended record source */
#define ESM_MIXER_MASTER_VOL        0x32   /* Master volume */
#define ESM_MIXER_FM_VOL            0x36   /* FM volume */
#define ESM_MIXER_AUXA_VOL          0x38   /* AuxA(CD) volume */
#define ESM_MIXER_AUXB_VOL          0x3A   /* AuxB volume */
#define ESM_MIXER_PCSPKR_VOL        0x3C   /* PC Speaker volume */
#define ESM_MIXER_LINE_VOL          0x3E   /* Line volume */

#define ESM_MIXER_SERIALMODE_CTL    0x48   /* Serial mode control */
#define ESM_MIXER_SPATIALIZER_EN    0x50   /* Spatializer enable and mode control */
#define ESM_MIXER_SPATIALIZER_LV    0x52   /* Spatializer level */
#define ESM_MIXER_LEFT_MASTER_VOL   0x60   /* Left master volume and mute */
#define ESM_MIXER_RIGHT_MASTER_VOL  0x62   /* Right master volume and mute */
#define ESM_MIXER_MASTER_VOL_CTL    0x64   /* Master Volume control */
#define ESM_MIXER_OPAMP_CALIB       0x65   /* Opamp Calibration Control */
#define ESM_MIXER_CLRHWVOLIRQ       0x66   /* Clear Hardware Volume Interrupt Request  */
#define ESM_MIXER_MIC_RECVOL        0x68   /* Audio 2 record volume */
#define ESM_MIXER_AUDIO2_RECVOL     0x69   /* Audio 2 record volume */
#define ESM_MIXER_AUXA_RECVOL       0x6A   /* AuxA record volume */
#define ESM_MIXER_DAC_RECVOL        0x6B   /* Music DAC record volume */
#define ESM_MIXER_AUXB_RECVOL       0x6C   /* AuxB record volume */
#define ESM_MIXER_MONOIN_PLAYMIX    0x6D   /* Mono_In play mix */    
#define ESM_MIXER_LINE_RECVOL       0x6E   /* Line record volume */
#define ESM_MIXER_MONOIN_RECVOL     0x6F   /* Mono_In record volume */
#define ESM_MIXER_AUDIO2_SR         0x70   /* Audio 2 Sample rate */
#define ESM_MIXER_AUDIO2_MODE       0x71   /* Audio 2 mode */
#define ESM_MIXER_AUDIO2_CLKRATE    0x72   /* Audio 2 clock rate */
#define ESM_MIXER_AUDIO2_TCOUNT     0x74   /* Audio 2 transfer count reload */
#define ESM_MIXER_AUDIO2_CTL1       0x78   /* Audio 2 Control 1 */
#define ESM_MIXER_AUDIO2_CTL2       0x7A   /* Audio 2 Control 2 */
#define ESM_MIXER_AUDIO2_VOL        0x7C   /* Audio 2 DAC mixer volume */
#define ESM_MIXER_MIC_PREAMP        0x7D   /* Mic preamp, Mono_In and Mono_Out */
#define ESM_MIXER_MDR               0x7F   /* Music digital record */

/* Values for ESSIO_REG_AUDIO2MODE */
#define ESSA2M_DIR               0x01   /* Audio 2 DMA Direction. 0 = Memory to DAC. */
#define ESSA2M_DMAEN             0x02   /* Audio 2 DMA enable */
#define ESSA2M_BCLKEN            0x04   /* BCLK select */
#define ESSA2M_AIEN              0x08   /* Auto-Initialize enable for Audio 2 DMA */

/* Values for the ESM_LEGACY_AUDIO_CONTROL */
#define ESS_DISABLE_AUDIO	     0x8000
#define ESS_ENABLE_SERIAL_IRQ	 0x4000
#define IO_ADRESS_ALIAS		     0x0020
#define MPU401_IRQ_ENABLE	     0x0010
#define MPU401_IO_ENABLE	     0x0008
#define GAME_IO_ENABLE		     0x0004
#define FM_IO_ENABLE		     0x0002
#define SB_IO_ENABLE		     0x0001

/* Values for ESSIO_REG_IRQCONTROL */
#define MPUIRQ      0x80
#define HVIRQ       0x40
#define A2IRQ       0x20
#define A1IRQ       0x10
#define ESS_ALLIRQ  (MPUIRQ | HVIRQ | A2IRQ | A1IRQ)

/* Values for ESSDM_REG_DMAMODE */
#define ESSDM_DMAMODE_TTYPE_VERIFY    0x00 /* Verify transfer */
#define ESSDM_DMAMODE_TTYPE_WRITE     0x04 /* Write transfer */
#define ESSDM_DMAMODE_TTYPE_READ      0x08 /* Read transfer */
#define ESSDM_DMAMODE_AI              0x10 /* Auto Initialize */
#define ESSDM_DMAMODE_DIR             0x20 /* Transfer direction */
#define ESSDM_DMAMODE_TMODE_DEMAND    0x00 /* Demand transfer */
#define ESSDM_DMAMODE_TMODE_SINGLE    0x40 /* Single transfer */
#define ESSDM_DMAMODE_TMODE_BLOCK     0x80 /* Block transfer */


#pragma pack(1)

/* Solo1 configuration, page 25 */
typedef struct stESMCONFIG {
    DWORD SID:1;    /* PCI Subsystem ID */
    DWORD r1:1;     /* Reserved */
    DWORD S2:1;     /* SB base address select */
    DWORD M4D:2;    /* MPU base address select */
    DWORD r2:2;     /* Reserved. Returns 0 when read. */
    DWORD r3:1;     /* Reserved. Write 0. */
    DWORD DMMAP:3;  /* DMA policy */
    DWORD r4:2;     /* Reserved */
    DWORD IRQP:2;   /* ISA IRQ emulation policy */
    DWORD r5:1;     /* Reserved */
    DWORD ISAIRQ:1; /* ISA IRQ enable. Point-to-point or serialized IRQ. */
    DWORD r6:13;    /* Reserved */
    DWORD CPE:1;    /* CLKRUN protocol enable. */
    DWORD r7:1;     /* Reserved */
} ESMCONFIG, *PESMCONFIG;

/* Master Volume Control, page 55 */
typedef union {
    struct {
        BYTE SbProVol:1;  /* Disable SB Pro master vol emulation */
        BYTE HMVmask:1;   /* HMV int mask */
        BYTE Mode:2;      /* Operation mode */
        BYTE HMVreq:1;    /* Readonly HMV int request */
        BYTE CountBy3:1;  /* Count by 3 */
        BYTE MPU401Int:1; /* MPU401 int mask */
        BYTE SplitMode:1; /* Split mode */
    } f;
    BYTE b;
} ESMMIXERMVC, *PESMMIXERMVC;

#pragma pack()

#define MAXLEN_DMA_BUFFER       0x8000

typedef struct
{
    PWCHAR   KeyName;
    DWORD    RegisterSetting;
} MIXERSETTING,*PMIXERSETTING;

typedef struct
{
    BYTE Register;
    BYTE Value;
} SETTING;

typedef struct
{
    SETTING s;
    BOOLEAN IsMixer;
} CONFIGSETTING;

typedef struct 
{
  DWORD Offset;
  DWORD Data;
} SAVED_CONFIG;

DEFINE_GUID(IID_IAdapterCommon,
0x80489FE1, 0x730C, 0x11d1, 0x88, 0xb4, 0x0, 0xc0, 0x9f, 0x0, 0x2b, 0x8f);

/*****************************************************************************
 * IAdapterCommon
 *****************************************************************************
 * Interface for adapter common object.
 */
DECLARE_INTERFACE_(IAdapterCommon,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()           // For IUnknown

    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        PRESOURCELIST ResourceList, 
        PDEVICE_OBJECT DeviceObject
    )   PURE;

    STDMETHOD_(PINTERRUPTSYNC,GetInterruptSync)
    (   THIS_
    )   PURE;

    STDMETHOD_(PPORTWAVECYCLIC *,WavePortDriverDest)
    (   THIS_
    )   PURE;

    STDMETHOD_(PPORTMIDI*,MidiPortDriverDest)
    (   THIS_
    )   PURE;

    STDMETHOD_(void,SetWaveServiceGroup)
    (   THIS_
        IN      PSERVICEGROUP  WaveServiceGroup
    )   PURE;

    STDMETHOD_(USHORT,GetPosition)
    (   THIS_
        IN      BOOLEAN    Capture,
        IN      UINT       DmaBufferSize
    )   PURE;

    STDMETHOD_(void,StartDma)
    (   THIS_
        IN      BOOLEAN    Capture,
        IN      ULONG      DMAChannelAddress,
        IN      ULONG      BufferSize,
        IN      BOOLEAN    Format16Bit,
        IN      BOOLEAN    FormatStereo,
        IN      ULONG      NotificationInterval,
        IN      ULONG      SamplingFrequency
    )   PURE;

    STDMETHOD_(void,StopDma)
    (   THIS_
        IN      BOOLEAN    Capture
    )   PURE;

    STDMETHOD_(void,PauseDma)
    (   THIS_
        IN      BOOLEAN    Capture
    )   PURE;

    STDMETHOD_(UCHAR,dspRead)
    (   THIS_
    )   PURE;

    STDMETHOD_(UCHAR,dspWrite)
    (   THIS_
        IN  UCHAR       Value
    )   PURE;

    STDMETHOD_(DWORD,GetFlags)
    (   THIS_
    )   PURE;

    STDMETHOD_(void,StartESFM)
    (   THIS_
        IN  BOOLEAN     MidiActive
    )   PURE;

    STDMETHOD_(void,Init689)
    (   THIS_
    )   PURE;

    STDMETHOD_(void,Enable_Irq)
    (   THIS_
    )   PURE;

    STDMETHOD_(UCHAR,dspReadMixer)
    (   THIS_
        IN  UCHAR   Address
    )   PURE;

    STDMETHOD_(void,dspWriteMixer)
    (   THIS_
        IN      UCHAR   Address,
        IN      UCHAR   Value
    )   PURE;

    STDMETHOD_(DWORD,BoardId)
    (   THIS_
    )   PURE;

    STDMETHOD_(BOOL,IsRecording)
    (   THIS_
    )   PURE;

    STDMETHOD_(void,StartRecording)
    (   THIS_
        IN      BOOLEAN    Recording
    )   PURE;

    STDMETHOD_(void,SetMixerTables)
    (   THIS_
        IN      PUCHAR     Table1,
        IN      PUCHAR     Table0
    )   PURE;

    STDMETHOD_(void,SetClkRunEnable)
    (   THIS_
        IN      BOOLEAN    Enable
    )   PURE;

    STDMETHOD_(void,SetRecordingSource)
    (   THIS_
        IN      PDWORD     RecordingSource
    )   PURE;

    STDMETHOD_(PUCHAR,SetDspBase)
    (   THIS_
        IN      PUCHAR  DspBase
    )   PURE;

    STDMETHOD_(void,PostProcessing)
    (   THIS_
    )   PURE;

    STDMETHOD_(void,SetDacToMidi)
    (   THIS_
        IN  BOOLEAN     MidiActive
    )   PURE;

    STDMETHOD_(BOOLEAN,IsMidiActive)
    (   THIS_
    )   PURE;

    STDMETHOD_(void,SetPortEventsDest)
    (   THIS_
        IN      PPORTEVENTS  PortEvents
    )   PURE;

    STDMETHOD_(DWORD,GetHardwareVolumeFlag)
    (   THIS_
        IN      BYTE    Mask
    )   PURE;

    STDMETHOD_(void,SetHardwareVolumeFlag)
    (   THIS_
        IN      BYTE    Flag
    )   PURE;

    STDMETHOD_(void,SetMute)
    (   THIS_
        IN      BYTE     Mute
    )   PURE;

    STDMETHOD_(void,DRM_SetFlags)
    (   THIS_
        IN      BOOL    Mute,
        IN      BOOL    a3
    )   PURE;

    STDMETHOD_(USHORT,GetDeviceID)
    (   THIS_
    )   PURE;
    
    
};

typedef IAdapterCommon *PADAPTERCOMMON;

extern BOOLEAN bMPU401IntEnable;

/*****************************************************************************
 * NewAdapterCommon()
 *****************************************************************************
 * Create a new adapter common object.
 */
NTSTATUS
NewAdapterCommon
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);

NTSTATUS
AccessConfigSpace(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Read,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

#endif  //_COMMON_H_
