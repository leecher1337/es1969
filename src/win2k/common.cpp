/*****************************************************************************
 * common.cpp - Common code used by all the ESS miniports.
 *****************************************************************************
 * Copyright (c) 2023 leecher@dose.0wnz.at  All rights reserved.
 *
 * Implmentation of the common code object.  This class deals with interrupts
 * for the device, and is a collection of common code used by all the
 * miniports.
 */

#include "common.h"
#include "DRIVER.H"

#define STR_MODULENAME "es1969Adapter: "

#define FLAG_PNP                0x01
#define FLAG_TWOBUTTONVOLMODE   0x02
#define FLAG_SIDSVIDUPDATE      0x10
#define FLAG_IISON              0x20
#define FLAG_HWVOLUMEON         0x40
#define FLAG_COUNTBY3           0x80
#define FLAG_NOGAMEPORT         0x100

static
MIXERSETTING DefaultMixerSettings[] =
{
    { L"PNP",                 FLAG_PNP              },
    { L"TwoButtonVolumeMode", FLAG_TWOBUTTONVOLMODE },
    { L"SidSvidUpdate",       FLAG_SIDSVIDUPDATE    },
    { L"IISOn",               FLAG_IISON            },
    { L"HwVolumeOn",          FLAG_HWVOLUMEON       },
    { L"CountBy3",            FLAG_COUNTBY3         },
    { L"NoGamePort",          FLAG_NOGAMEPORT       }
};

static
CONFIGSETTING ConfigSettings[] =
{
    {ESS_CMD_IRQCONTROL,        0, FALSE},
    {ESS_CMD_DRQCONTROL,        0, FALSE},
    {ESS_CMD_EXTSAMPLERATE,     0, FALSE},
    {ESS_CMD_FILTERDIV,         0, FALSE},
    {0x40,                      0, TRUE },
    {ESM_MIXER_MASTER_VOL_CTL,   0, TRUE },
    {ESM_MIXER_MIC_PREAMP,      0, TRUE },
    {ESM_MIXER_MDR,             0, TRUE },
    {ESM_MIXER_AUDIO2_MODE,     0, TRUE },
    {ESM_MIXER_EXTENDEDRECSRC,  0, TRUE },
    {ESM_MIXER_AUDIO2_VOL,      0, TRUE },
    {0,                         0, FALSE}
};

static 
SAVED_CONFIG SavedConfig[] =
{
    {ESM_PMSTATUS,              0},
    {ESM_CMD,                   0},
    {ESM_IRQLINE,               0},
    {ESM_LEGACY_AUDIO_CONTROL,  0},
    {ESM_CONFIG,                0},
    {ESM_DDMA,                  0}
};

DWORD RunTime = 0;
extern BOOL NoJoy;
BOOLEAN bMPU401IntEnable = FALSE;

#define RUNTIME_FLAG_NODRM  0x4000

BOOLEAN SidSvidUpdate = FALSE;


/*****************************************************************************
 * CAdapterCommon
 
 *****************************************************************************
 * Adapter common object.
 */
class CAdapterCommon
:   public IAdapterCommon,
    public IAdapterPowerManagement,
    public CUnknown
    
{
private:
    PINTERRUPTSYNC          m_pInterruptSync;
    PPORTWAVECYCLIC         m_pPortWave;
    PPORTMIDI               m_pPortMidi;
    PSERVICEGROUP           m_pWaveServiceGroup;
    PDEVICE_OBJECT          m_pDeviceObject;
    
    PUCHAR                  m_pIOBase;
    PUCHAR                  m_pSBBase;
    PUCHAR                  m_pFMBase;
    PUCHAR                  m_pDMABase;
    PUCHAR                  m_pMPU401Base;
    PUCHAR                  m_pJoystickBase;
    
    BOOLEAN                 m_Recording;
    SETTING                 *m_pMixerVolumesIn;
    SETTING                 *m_pMixerVolumesOut;
    DWORD                   m_BoardId;
    DEVICE_POWER_STATE      m_PowerState;
    DWORD                   m_Flags;
    BOOLEAN                 m_MidiActive;
    BOOL                    m_MuteInput;
    BOOLEAN                 m_DmaStarted;
    PDWORD                  m_pRecordingSource;
    BOOLEAN                 m_CPE;
    
    PPORTEVENTS             m_pPortEvents;
    
    BYTE                    m_HardwareVolumeFlag;
    BOOLEAN                 m_MuteOutput;
    USHORT                  m_DeviceID;


    void AcknowledgeIRQ
    (   void
    );

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CAdapterCommon);
    ~CAdapterCommon();

    /*****************************************************************************
     * IAdapterCommon methods
     */
    STDMETHODIMP_(NTSTATUS) Init
    (
        PRESOURCELIST ResourceList, 
        PDEVICE_OBJECT DeviceObject
    );

    STDMETHODIMP_(void) PostProcessing
    (   void
    );

    STDMETHODIMP_(NTSTATUS) SoloPciInit
    (
        IN      PDEVICE_OBJECT  DeviceObject,
        OUT     PBOOLEAN        pIs1969
    );

    STDMETHODIMP_(PPORTWAVECYCLIC *) WavePortDriverDest
    (   void
    );

    STDMETHODIMP_(void) SetPortEventsDest
    (
        IN      PPORTEVENTS  PortEvents
    );

    STDMETHODIMP_(PINTERRUPTSYNC) GetInterruptSync
    (   void
    );

    STDMETHODIMP_(PPORTMIDI*) MidiPortDriverDest
    (   void
    );

    STDMETHODIMP_(void) SetWaveServiceGroup
    (
        IN      PSERVICEGROUP  WaveServiceGroup
    );

    STDMETHODIMP_(DWORD) GetFlags
    (   void
    );

    STDMETHODIMP_(void) SetHardwareVolumeFlag
    (
        IN      BYTE    Flag
    );

    STDMETHODIMP_(DWORD) GetHardwareVolumeFlag
    (
        IN      BYTE    Mask
    );

    STDMETHODIMP_(void) SetMute
    (
        IN      BYTE     Mute
    );

    STDMETHODIMP_(UCHAR) dspRead
    (   void
    );

    STDMETHODIMP_(UCHAR) dspWrite
    (
        IN  UCHAR       Value
    );

    STDMETHODIMP_(void) StartESFM
    (   
        IN  BOOLEAN     MidiActive
    );

    STDMETHODIMP_(void) SetDacToMidi
    (   
        IN  BOOLEAN     MidiActive
    );

    STDMETHODIMP_(BOOLEAN) IsMidiActive
    (   void
    );

    STDMETHODIMP_(void) Init689
    (   void
    );

    STDMETHODIMP_(void) Enable_Irq
    (   void
    );

    STDMETHODIMP_(void) StartDma
    (
        IN      BOOLEAN    Capture,
        IN      ULONG      DMAChannelAddress,
        IN      ULONG      BufferSize,
        IN      BOOLEAN    Format16Bit,
        IN      BOOLEAN    FormatStereo,
        IN      ULONG      NotificationInterval,
        IN      ULONG      SamplingFrequency
    );

    STDMETHODIMP_(void) PauseDma
    (
        IN      BOOLEAN    Capture
    );

    STDMETHODIMP_(void) StopDma
    (
        IN      BOOLEAN    Capture
    );

    STDMETHODIMP_(PUCHAR) SetDspBase
    (
        IN      PUCHAR  DspBase
    );

    STDMETHODIMP_(void) DRM_SetFlags
    (
        IN      BOOL    Mute,
        IN      BOOL    a3
    );

    STDMETHODIMP_(void) RestoreConfig
    (   void
    );

    STDMETHODIMP_(void) SaveConfig
    (   void
    );

    STDMETHODIMP_(void) SetClkRunEnable
    (
        IN      BOOLEAN    Enable
    );

    STDMETHODIMP_(void) SetRecordingSource
    (
        IN      PDWORD     RecordingSource
    );

    STDMETHODIMP_(DWORD) BoardId
    (   void
    );

    STDMETHODIMP_(BOOL) IsRecording
    (   void
    );

    STDMETHODIMP_(void) StartRecording
    (
        IN      BOOLEAN    Recording
    );

    STDMETHODIMP_(void) SetMixerTables
    (
        IN      PUCHAR     Table1,
        IN      PUCHAR     Table0
    );

    STDMETHODIMP_(USHORT) GetDeviceID
    (   void
    );

    STDMETHODIMP_(USHORT) GetPosition
    (
        IN      BOOLEAN    Capture,
        IN      UINT       DmaBufferSize
    );

    STDMETHODIMP_(UCHAR) dspReadMixer
    (
        IN  UCHAR   Address
    );

    STDMETHODIMP_(void) dspWriteMixer
    (
        IN  UCHAR   Address,
        IN  UCHAR   Value
    );
    
    STDMETHODIMP_(void) GetRegistrySettings
    (   void
    );

    STDMETHODIMP_(void) SetRegistrySettings
    (   void
    );
    
    STDMETHODIMP_(void) dspReset
    (   void
    );
    
    inline STDMETHODIMP_(void) SysExMessage
    (   IN      BYTE    Command
    );


    /*************************************************************************
     * IAdapterPowerManagement implementation
     *
     * This macro is from PORTCLS.H.  It lists all the interface's functions.
     */
    IMP_IAdapterPowerManagement;

    friend
    NTSTATUS
    InterruptServiceRoutine
    (
        IN      PINTERRUPTSYNC  InterruptSync,
        IN      PVOID           DynamicContext
    );
};




#pragma code_seg("PAGE")

NTSTATUS 
ConfigAccessCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp, 
    PRKEVENT Event
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
AccessConfigSpace(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Read,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Reads or writes PCI config space for the specified device.

Arguments:

    DeviceObject - Supplies the device object

    Read - if TRUE, this is a READ IRP
           if FALSE, this is a WRITE IRP

    Buffer - Returns the PCI config data

    Offset - Supplies the offset into the PCI config data where the read should begin

    Length - Supplies the number of bytes to be read

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    PIRP Irp;
    KEVENT Event;
    PIO_STACK_LOCATION pStack;
    
    PAGED_CODE();
    ASSERT(DeviceObject);
    ASSERT(Buffer);

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if ( !Irp ) return STATUS_INSUFFICIENT_RESOURCES;

    pStack = IoGetNextIrpStackLocation (Irp);
    pStack->MajorFunction = IRP_MJ_PNP;
    pStack->MinorFunction = Read ? IRP_MN_READ_CONFIG : IRP_MN_WRITE_CONFIG;
    pStack->Parameters.ReadWriteConfig.Offset = Offset;
    pStack->Parameters.ReadWriteConfig.Buffer = Buffer;
    pStack->Parameters.ReadWriteConfig.Length = Length;
    pStack->Parameters.ReadWriteConfig.WhichSpace = PCI_WHICHSPACE_CONFIG;
    pStack->DeviceObject = DeviceObject;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    pStack = IoGetNextIrpStackLocation (Irp);
    pStack->Context = &Event;
    pStack->CompletionRoutine = (PIO_COMPLETION_ROUTINE)ConfigAccessCompletionRoutine;
    pStack->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
    Status = IoCallDriver(DeviceObject, Irp);
    if ( Status == STATUS_PENDING ) {
        return KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );
    }
    return Status;
}

/*****************************************************************************
 * CAdapterCommon::GetRegistrySettings()
 *****************************************************************************
 * Restores the mixer settings based on settings stored in the registry.
 */
void
CAdapterCommon::
GetRegistrySettings
(   void
)
{
    PREGISTRYKEY    DriverKey;
    PVOID           KeyInfo;

    PAGED_CODE();
    
    // open the driver registry key
    NTSTATUS ntStatus = PcNewRegistryKey( &DriverKey,               // IRegistryKey
                                          NULL,                     // OuterUnknown
                                          DriverRegistryKey,        // Registry key type
                                          KEY_ALL_ACCESS,           // Access flags
                                          m_pDeviceObject,          // Device object
                                          NULL,                     // Subdevice
                                          NULL,                     // ObjectAttributes
                                          0,                        // Create options
                                          NULL );                   // Disposition
    if(NT_SUCCESS(ntStatus))
    {
        UNICODE_STRING  KeyName;
        ULONG           ResultLength;
        
        KeyInfo = ExAllocatePool(PagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD));
        if(NULL != KeyInfo)
        {
            // make a unicode strong for the subkey name
            RtlInitUnicodeString( &KeyName, L"RunTime" );

            // query the value key
            ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                   KeyValuePartialInformation,
                                                   KeyInfo,
                                                   sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                   &ResultLength );
            if(NT_SUCCESS(ntStatus))
            {
                PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                if(PartialInfo->DataLength == sizeof(DWORD))
                {
                    RunTime = *(PDWORD(PartialInfo->Data));
                }

                // loop through all mixer settings
                for(UINT i = 0; i < SIZEOF_ARRAY(DefaultMixerSettings); i++)
                {
                    // init key name
                    RtlInitUnicodeString( &KeyName, DefaultMixerSettings[i].KeyName );
    
                    // query the value key
                    ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                           KeyValuePartialInformation,
                                                           KeyInfo,
                                                           sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                           &ResultLength );
                    if(NT_SUCCESS(ntStatus))
                    {
                        PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                        if(PartialInfo->DataLength == sizeof(DWORD) && *(PDWORD(PartialInfo->Data)))
                        {
                            m_Flags |= DefaultMixerSettings[i].RegisterSetting;
                        }
                    }
                }

                // free the key info
                ExFreePool(KeyInfo);
            }
        }

        // release the driver key
        DriverKey->Release();
    }
}

/*****************************************************************************
 * CAdapterCommon::SetRegistrySettings()
 *****************************************************************************
 * Applies settings from Flags to card
 */
void
CAdapterCommon::
SetRegistrySettings
(   void
)
{
    BYTE PortData;

    PAGED_CODE();
    
    NoJoy = (m_Flags & FLAG_NOGAMEPORT) ? TRUE : FALSE;
    
    if (m_Flags & FLAG_HWVOLUMEON)
    {
        ESMMIXERMVC Mixer;
        
        Mixer.b = dspReadMixer(ESM_MIXER_MASTER_VOL_CTL);
        Mixer.f.Mode = (m_Flags & FLAG_TWOBUTTONVOLMODE)?1:2;
        Mixer.f.HMVmask = 1;
        Mixer.f.CountBy3 = (m_Flags & FLAG_COUNTBY3)?1:0;
        dspWriteMixer(ESM_MIXER_MASTER_VOL_CTL, Mixer.b);
    }
    
    if (m_Flags & FLAG_SIDSVIDUPDATE) SidSvidUpdate = TRUE;
    
    PortData = dspReadMixer(ESM_MIXER_MDR) & 0xFE;
    if (m_Flags & FLAG_IISON) PortData |= 1;
    dspWriteMixer(ESM_MIXER_MDR, PortData);
}


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
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY_
    (
        CAdapterCommon,
        Unknown,
        UnknownOuter,
        PoolType,
        PADAPTERCOMMON
    );
}

/*****************************************************************************
 * CAdapterCommon::AcknowledgeIRQ()
 *****************************************************************************
 * Acknowledge interrupt request.
 */
void
CAdapterCommon::
AcknowledgeIRQ
(   void
)
{
    PAGED_CODE();
    
    ASSERT(m_pSBBase);
    READ_PORT_UCHAR (m_pSBBase + ESSSB_REG_STATUS);
    dspWriteMixer(ESM_MIXER_CLRHWVOLIRQ, 0x66);
    dspWriteMixer(ESM_MIXER_AUDIO2_CTL2, 0x7F);
}


/*****************************************************************************
 * CAdapterCommon::dspReset()
 *****************************************************************************
 * Resets the DSP
 */
void
CAdapterCommon::
dspReset
(   void
)
{
    PAGED_CODE();
    
    WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_RESET, 1);
    KeStallExecutionProcessor(3);
    WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_RESET, 0);
    
    if (dspRead() == 0xAA) dspWrite(ESS_CMD_ENABLEEXT);
}


BOOL Setup689(PUCHAR Port)
{   
    PAGED_CODE();

    WRITE_PORT_UCHAR(Port + MPU401_REG_COMMAND, MPU401_CMD_RESET);
    if ((READ_PORT_UCHAR(Port + MPU401_REG_STATUS) & MPU401_DSR) != 0 )
        WRITE_PORT_UCHAR(Port + MPU401_REG_COMMAND, MPU401_CMD_RESET);
    if ((READ_PORT_UCHAR(Port + MPU401_REG_STATUS) & MPU401_DSR) != 0 )
        return FALSE;
    if (READ_PORT_UCHAR(Port + MPU401_REG_DATA) != MPU401_ACK)
        return FALSE;

    WRITE_PORT_UCHAR(Port + MPU401_REG_COMMAND, MPU401_CMD_UART);
    if ( READ_PORT_UCHAR(Port + MPU401_REG_DATA) != MPU401_ACK)
        return FALSE;
    
    // SysEx message 04
    WRITE_PORT_UCHAR(Port + MPU401_REG_DATA, 0xF0);
    WRITE_PORT_UCHAR(Port + MPU401_REG_DATA, 0x00);
    WRITE_PORT_UCHAR(Port + MPU401_REG_DATA, 0x00);
    WRITE_PORT_UCHAR(Port + MPU401_REG_DATA, 0x7B);
    WRITE_PORT_UCHAR(Port + MPU401_REG_DATA, 0x04);
    WRITE_PORT_UCHAR(Port + MPU401_REG_DATA, 0xF7);
    
    return TRUE;
}

/*****************************************************************************
 * CAdapterCommon::Init()
 *****************************************************************************
 * Initialize an adapter common object.
 */
NTSTATUS
CAdapterCommon::
Init
(
    IN      PRESOURCELIST   ResourceList,
    IN      PDEVICE_OBJECT  DeviceObject
)
{
    PAGED_CODE();

    ASSERT(ResourceList);
    ASSERT(DeviceObject);
    ASSERT(ResourceList->NumberOfPorts() == 5);
    ASSERT(ResourceList->NumberOfInterrupts() == 1);
    ASSERT(ResourceList->NumberOfDmas() == 0);
    
    BOOLEAN  Is1969;
    NTSTATUS ntStatus;
    DWORD    PortData;

   
    m_pDeviceObject = DeviceObject;

    ntStatus = SoloPciInit(DeviceObject, &Is1969);
  
    if(NT_SUCCESS(ntStatus))
    {
        
        //
        // Get the base address for the devices.
        //
        ASSERT(ResourceList->FindTranslatedPort(0));
        m_pIOBase   = (PUCHAR)(ResourceList->FindTranslatedPort(0)->u.Port.Start.QuadPart);

        ASSERT(ResourceList->FindTranslatedPort(1));
        m_pSBBase      = (PUCHAR)(ResourceList->FindTranslatedPort(1)->u.Port.Start.QuadPart);

        ASSERT(ResourceList->FindTranslatedPort(2));
        m_pFMBase       = (PUCHAR)(ResourceList->FindTranslatedPort(2)->u.Port.Start.QuadPart);

        ASSERT(ResourceList->FindTranslatedPort(3));
        m_pMPU401Base   = (PUCHAR)(ResourceList->FindTranslatedPort(3)->u.Port.Start.QuadPart);

        ASSERT(ResourceList->FindTranslatedPort(4));
        m_pJoystickBase = (PUCHAR)(ResourceList->FindTranslatedPort(4)->u.Port.Start.QuadPart);
        
        m_pDMABase      = Is1969 ? m_pFMBase : m_pIOBase + 16;

        //
        // Hook up the interrupt.
        //
        ntStatus = PcNewInterruptSync(                              // See portcls.h
                                        &m_pInterruptSync,          // Save object ptr
                                        NULL,                       // OuterUnknown(optional).
                                        ResourceList,               // He gets IRQ from ResourceList.
                                        0,                          // Resource Index
                                        InterruptSyncModeNormal     // Run ISRs once until we get SUCCESS
                                     );
        if (NT_SUCCESS(ntStatus) && m_pInterruptSync)
        {                                                                       //  run this ISR first
            ntStatus = m_pInterruptSync->RegisterServiceRoutine(InterruptServiceRoutine,PVOID(this),FALSE);
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = m_pInterruptSync->Connect();
            }

            // if we could not connect or register the ISR, release the object.
            if (!NT_SUCCESS (ntStatus))
            {
                m_pInterruptSync->Release();
                m_pInterruptSync = NULL;
            }
        }
    } else
    {
        _DbgPrintF(DEBUGLVL_TERSE,("ResetController Failure"));
    }
    
    m_MuteInput = FALSE;
    RunTime = 0;

    if(NT_SUCCESS(ntStatus))
    {
        ESMCONFIG Config;
        ESMMIXERMVC Mixer;
        
        PortData = ESS_DISABLE_AUDIO | MPU401_IRQ_ENABLE | MPU401_IO_ENABLE | GAME_IO_ENABLE | FM_IO_ENABLE | SB_IO_ENABLE;
        AccessConfigSpace(DeviceObject, FALSE, &PortData, ESM_LEGACY_AUDIO_CONTROL, sizeof(PortData));
        
        AccessConfigSpace(DeviceObject, TRUE , &Config, ESM_CONFIG, sizeof(Config));
        Config.S2     = 0; // SB Base = 220h
        Config.M4D    = 0; // MPU401  = 330h
        Config.DMMAP  = 0; // DMA Policy = Distributed DMA
        Config.IRQP   = 0; // ISA IRQ Emulation is disabled
        Config.ISAIRQ = 0; // ISA IRQ Enable = 0
        m_CPE = Config.CPE;
        AccessConfigSpace(DeviceObject, FALSE, &Config, ESM_CONFIG, sizeof(Config));
        
        PortData = LOWORD(m_pDMABase) | 1; // Enable distributed DMA
        AccessConfigSpace(DeviceObject, FALSE, &PortData, ESM_DDMA, sizeof(PortData));
        
        // Turn on interrupts
        WRITE_PORT_UCHAR(m_pIOBase + ESSIO_REG_IRQCONTROL, ESS_ALLIRQ);
        
        GetRegistrySettings();
        SetRegistrySettings();
        dspReset();
        
        dspWriteMixer(ESM_MIXER_OPAMP_CALIB, 1);
        Mixer.b = dspReadMixer(ESM_MIXER_MASTER_VOL_CTL);
        if (m_Flags & FLAG_HWVOLUMEON)
        {
            Mixer.f.SbProVol = 1;
            Mixer.f.HMVmask = 1;
        }
        else Mixer.f.HMVmask = 0;
        Mixer.f.MPU401Int = 0;
        dspWriteMixer(ESM_MIXER_MASTER_VOL_CTL, Mixer.b);
        AcknowledgeIRQ();
        Setup689(m_pMPU401Base);
        
        m_BoardId = 0x1869;
        StartESFM(FALSE);
        
        //
        // Set initial device power state
        //
        m_PowerState = PowerDeviceD0;
    }
    
    return ntStatus;
}

/*****************************************************************************
 * CAdapterCommon::PostProcessing()
 *****************************************************************************
 * Do some postprocessing
 */
STDMETHODIMP_(void)
CAdapterCommon::
PostProcessing
(   void
)
{
    PAGED_CODE();
    
    // Enable Interrupts
    dspWriteMixer(ESM_MIXER_MASTER_VOL_CTL, dspReadMixer(ESM_MIXER_MASTER_VOL_CTL) | 0x42);
    SetDacToMidi(FALSE);
}

STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
SoloPciInit
(
    IN      PDEVICE_OBJECT  DeviceObject,
    OUT     PBOOLEAN        pIs1969
)
{
    PAGED_CODE();

    DWORD PortData;
    
    AccessConfigSpace(DeviceObject, TRUE, &PortData, 0, sizeof(PortData));
    
    m_DeviceID = HIWORD(PortData);
    *pIs1969 = m_DeviceID == 0x1938 || m_DeviceID == 0x1969;
    
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAdapterCommon::~CAdapterCommon()
 *****************************************************************************
 * Destructor.
 */
CAdapterCommon::
~CAdapterCommon
(   void
)
{
    PAGED_CODE();

    // Disable Interrupts
    dspWriteMixer(ESM_MIXER_MASTER_VOL_CTL, dspReadMixer(ESM_MIXER_MASTER_VOL_CTL) & ~(0x42));
    WRITE_PORT_UCHAR(m_pIOBase + ESSIO_REG_IRQCONTROL, 0);
    
    if (m_pInterruptSync)
    {
        m_pInterruptSync->Disconnect();
        m_pInterruptSync->Release();
    }
    if (m_pPortWave)
        m_pPortWave->Release();
    if (m_pPortMidi)
        m_pPortMidi->Release();
    if (m_pWaveServiceGroup)
        m_pWaveServiceGroup->Release();
}

/*****************************************************************************
 * CAdapterCommon::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP
CAdapterCommon::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PADAPTERCOMMON(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IAdapterCommon))
    {
        *Object = PVOID(PADAPTERCOMMON(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IAdapterPowerManagement))
    {
        *Object = PVOID(PADAPTERPOWERMANAGEMENT(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*****************************************************************************
 * CAdapterCommon::WavePortDriverDest()
 *****************************************************************************
 * Get the Wave Port Driver object
 */
STDMETHODIMP_(PPORTWAVECYCLIC *)
CAdapterCommon::
WavePortDriverDest
(   void
)
{
    PAGED_CODE();
    
    return &m_pPortWave;
}

/*****************************************************************************
 * CAdapterCommon::SetPortEventsDest()
 *****************************************************************************
 * Get the Wave Port Driver object
 */
STDMETHODIMP_(void)
CAdapterCommon::
SetPortEventsDest
(
    IN  PPORTEVENTS  PortEvents
)
{
    PAGED_CODE();
    
    m_pPortEvents = PortEvents;
}

/*****************************************************************************
 * CAdapterCommon::GetInterruptSync()
 *****************************************************************************
 * Get a pointer to the interrupt synchronization object.
 */
STDMETHODIMP_(PINTERRUPTSYNC)
CAdapterCommon::
GetInterruptSync
(   void
)
{
    PAGED_CODE();
    
    return m_pInterruptSync;
}

/*****************************************************************************
 * CAdapterCommon::MidiPortDriverDest()
 *****************************************************************************
 * Get Midi Port driver class instance
 */
STDMETHODIMP_(PPORTMIDI*)
CAdapterCommon::
MidiPortDriverDest
(   void
)
{
    PAGED_CODE();
    
    return &m_pPortMidi;
}

/*****************************************************************************
 * CAdapterCommon::SetWaveServiceGroup()
 *****************************************************************************
 * Sets wave service group
 */
STDMETHODIMP_(void)
CAdapterCommon::
SetWaveServiceGroup
(
    IN  PSERVICEGROUP  WaveServiceGroup
)
{
    PAGED_CODE();
    
    if ( m_pWaveServiceGroup )
        m_pWaveServiceGroup->Release();
    
    m_pWaveServiceGroup = WaveServiceGroup;
    
    if ( WaveServiceGroup )
        WaveServiceGroup->AddRef();
}


/*****************************************************************************
 * CAdapterCommon::GetFlags()
 *****************************************************************************
 * Get the configuration flags currently set
 */
STDMETHODIMP_(DWORD)
CAdapterCommon::
GetFlags
(   void
)
{
    PAGED_CODE();
    
    return m_Flags;
}

/*****************************************************************************
 * CAdapterCommon::SetHardwareVolumeFlag()
 *****************************************************************************
 * Sets hardware volume flag
 */
STDMETHODIMP_(void)
CAdapterCommon::
SetHardwareVolumeFlag
(
    IN      BYTE    Flag
)
{
    m_HardwareVolumeFlag &= ~Flag;
    
    if ( m_HardwareVolumeFlag == 2 )
        m_pPortEvents->GenerateEventList(NULL, KSEVENT_CONTROL_CHANGE, FALSE, 
            ULONG(-1), TRUE, 0x1B);

}

/*****************************************************************************
 * CAdapterCommon::GetHardwareVolumeFlag()
 *****************************************************************************
 * Get the Hardware volume flag currently set
 */
STDMETHODIMP_(DWORD)
CAdapterCommon::
GetHardwareVolumeFlag
(
    IN      BYTE    Mask
)
{
    return m_HardwareVolumeFlag & Mask;
}

/*****************************************************************************
 * CAdapterCommon::SetMute()
 *****************************************************************************
 * Mutes the Soundcard
 */
STDMETHODIMP_(void)
CAdapterCommon::
SetMute
(
    IN      BYTE     Mute
)
{
    m_MuteOutput = Mute;
}


/*****************************************************************************
 * CAdapterCommon::dspRead()
 *****************************************************************************
 * Reads from the DSP
 */
STDMETHODIMP_(UCHAR)
CAdapterCommon::
dspRead
(   void
)
{
    ULONGLONG TimeInterval, i;

    PAGED_CODE();
    
    ASSERT(m_pSBBase);
    
    TimeInterval = PcGetTimeInterval(0);
    
    if ( (READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_STATUS) & 0x80) != 0 )
        return READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_READDATA);
    
    for ( i = PcGetTimeInterval(TimeInterval); i < 2000000; i = PcGetTimeInterval(TimeInterval) )
    {
        if ( (READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_STATUS) & 0x80) != 0 )
            return READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_READDATA);
    }
    
    ASSERT("dspRead timeout!");
    
    return 0xFF;
    
}

/*****************************************************************************
 * CAdapterCommon::dspWrite()
 *****************************************************************************
 * Writes to the DSP
 */
STDMETHODIMP_(UCHAR)
CAdapterCommon::
dspWrite
(
    IN  UCHAR   Value
)
{
    ULONGLONG TimeInterval, i;

    PAGED_CODE();
    
    ASSERT(m_pSBBase);

    TimeInterval = PcGetTimeInterval(0);
    
    if ( (READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_WRITEDATA) & 0x80) == 0 )
    {
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_WRITEDATA, Value);
        return TRUE;
    }
    
    for ( i = PcGetTimeInterval(TimeInterval); i < 5000000; i = PcGetTimeInterval(TimeInterval) )
    {
        if ( (READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_WRITEDATA) & 0x80) == 0 )
        {
            WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_WRITEDATA, Value);
            return TRUE;
        }
        
        ASSERT("dspWrite timeout");
    }
    
    return FALSE;
}

/*****************************************************************************
 * CAdapterCommon::StartESFM()
 *****************************************************************************
 * Starts up the ESS FM Module
 */
STDMETHODIMP_(void)
CAdapterCommon::
StartESFM
(   
    IN  BOOLEAN     MidiActive
)
{
    BYTE SerialMode;

    PAGED_CODE();
    
    SerialMode = dspReadMixer(ESM_MIXER_SERIALMODE_CTL);
    if ( MidiActive )
        SerialMode &= ~0x10;
    else
        SerialMode |= 0x10;
    dspWriteMixer(ESM_MIXER_SERIALMODE_CTL, SerialMode);
    
    SetDacToMidi(MidiActive);
    
    if ( MidiActive )
    {
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_POWER, READ_PORT_UCHAR((PUCHAR)ESSSB_REG_POWER) & (~0x20));
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_FMHIGHADDR, 5);
        KeStallExecutionProcessor(25);
        // We want ESFM mode!
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_FMLOWADDR + 1, 0x80);
        KeStallExecutionProcessor(25);
    }
    else
    {
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_POWER, READ_PORT_UCHAR((PUCHAR)ESSSB_REG_POWER) | 0x20);
    }
}

/*****************************************************************************
 * CAdapterCommon::SetDacToMidi()
 *****************************************************************************
 * Set DAC to Midi Mode
 */
STDMETHODIMP_(void)
CAdapterCommon::
SetDacToMidi
(   
    IN  BOOLEAN     MidiActive
)
{
    PAGED_CODE();

    m_MidiActive = MidiActive;
    
    if ( MidiActive )
    {
        dspWriteMixer(ESM_MIXER_MDR, dspReadMixer(ESM_MIXER_MDR) & (~0x01));
        
        if (m_pMixerVolumesOut)
            dspWriteMixer(m_pMixerVolumesOut[1].Register, m_pMixerVolumesOut[1].Value);
        
        if (m_pMixerVolumesIn)
            dspWriteMixer(m_pMixerVolumesIn[0].Register, m_pMixerVolumesIn[0].Value);
    }
    else if ( m_Flags & FLAG_IISON )
    {
        dspWriteMixer(ESM_MIXER_MDR, dspReadMixer(ESM_MIXER_MDR) | 0x01);

        if (m_pMixerVolumesOut)
            dspWriteMixer(m_pMixerVolumesOut[6].Register, m_pMixerVolumesOut[6].Value);

        if (m_pMixerVolumesIn)
            dspWriteMixer(m_pMixerVolumesIn[5].Register, m_pMixerVolumesIn[5].Value);
    }
}

/*****************************************************************************
 * CAdapterCommon::IsMidiActive()
 *****************************************************************************
 * Checks if Midi active 
 */
STDMETHODIMP_(BOOLEAN)
CAdapterCommon::
IsMidiActive
(   void
)
{
    PAGED_CODE();
    
    return m_MidiActive;
}

BOOLEAN Is_MPU401_Ready(PUCHAR Port)
{
    int i;
    
    PAGED_CODE();
    
    for ( i = 0; (READ_PORT_UCHAR(Port + MPU401_REG_STATUS) & MPU401_DRR) != 0; i++ )
    {
        if (i >= 1000) return FALSE;
    }
    return TRUE;
}


/*****************************************************************************
 * CAdapterCommon::SysExMessage()
 *****************************************************************************
 * Sends a vendor specific SysEx message
 */
inline
void
CAdapterCommon::
SysExMessage
(   IN      BYTE    Command
)
{
    if ( Is_MPU401_Ready(m_pMPU401Base) ) WRITE_PORT_UCHAR(m_pMPU401Base + MPU401_REG_COMMAND, 0xF0);
    if ( Is_MPU401_Ready(m_pMPU401Base) ) WRITE_PORT_UCHAR(m_pMPU401Base + MPU401_REG_COMMAND, 0x00);
    if ( Is_MPU401_Ready(m_pMPU401Base) ) WRITE_PORT_UCHAR(m_pMPU401Base + MPU401_REG_COMMAND, 0x00);
    if ( Is_MPU401_Ready(m_pMPU401Base) ) WRITE_PORT_UCHAR(m_pMPU401Base + MPU401_REG_COMMAND, 0x7B);
    if ( Is_MPU401_Ready(m_pMPU401Base) ) WRITE_PORT_UCHAR(m_pMPU401Base + MPU401_REG_COMMAND, Command);
    if ( Is_MPU401_Ready(m_pMPU401Base) ) WRITE_PORT_UCHAR(m_pMPU401Base + MPU401_REG_COMMAND, 0xF7);
}

/*****************************************************************************
 * CAdapterCommon::Init689()
 *****************************************************************************
 * Initialize the ES689.
 */
STDMETHODIMP_(void)
CAdapterCommon::
Init689
(   void
)
{
    PAGED_CODE();

    UCHAR PortData;

    // Enable ES689 Interface
    dspWriteMixer(ESM_MIXER_SERIALMODE_CTL, dspReadMixer(ESM_MIXER_SERIALMODE_CTL) | 0x10);
    
    // Disable MPU401 Interrupt (MPU401Int = 0)
    PortData = dspReadMixer(ESM_MIXER_MASTER_VOL_CTL);
    dspWriteMixer(ESM_MIXER_MASTER_VOL_CTL, PortData & ~(0x40));
    
    dspWrite(0xC6); // Enable extended mode command
    dspWrite(0xBC);
    dspWrite(0x36);
    
    // Reset
    if ( Is_MPU401_Ready(m_pMPU401Base) )
    {
        WRITE_PORT_UCHAR(m_pMPU401Base + MPU401_REG_COMMAND, MPU401_CMD_RESET);
        READ_PORT_UCHAR(m_pMPU401Base + MPU401_REG_DATA);
    }

    // Enter UART mode
    if ( Is_MPU401_Ready(m_pMPU401Base) )
    {
        WRITE_PORT_UCHAR(m_pMPU401Base + MPU401_REG_COMMAND, MPU401_CMD_UART);
        READ_PORT_UCHAR(m_pMPU401Base + MPU401_REG_DATA);
    }

    SysExMessage(0x01);
    SysExMessage(0x04);
    SysExMessage(0x06);
    KeStallExecutionProcessor(50);
    
    // Reset
    if ( Is_MPU401_Ready(m_pMPU401Base) )
    {
        WRITE_PORT_UCHAR(m_pMPU401Base + MPU401_REG_COMMAND, MPU401_CMD_RESET);
        READ_PORT_UCHAR(m_pMPU401Base + MPU401_REG_DATA);
    }

    // Re-enable MPU401 Interrupt
    dspWriteMixer(ESM_MIXER_MASTER_VOL_CTL, PortData);
}

/*****************************************************************************
 * CAdapterCommon::Enable_Irq()
 *****************************************************************************
 * Enable required Interrupts
 */
STDMETHODIMP_(void)
CAdapterCommon::
Enable_Irq
(   void
)
{
    UCHAR IrqMask;

    IrqMask = A1IRQ | A2IRQ ;
    if ( m_Flags & FLAG_HWVOLUMEON ) IrqMask |= HVIRQ;
    if ( bMPU401IntEnable ) IrqMask |= MPUIRQ;
    WRITE_PORT_UCHAR(m_pIOBase + ESSIO_REG_IRQCONTROL, IrqMask);
}

/*****************************************************************************
 * CAdapterCommon::StartDma()
 *****************************************************************************
 * Start of DMA 
 */
STDMETHODIMP_(void)
CAdapterCommon::
StartDma
(
    IN      BOOLEAN    Capture,
    IN      ULONG      DMAChannelAddress,
    IN      ULONG      BufferSize,
    IN      BOOLEAN    Format16Bit,
    IN      BOOLEAN    FormatStereo,
    IN      ULONG      NotificationInterval,
    IN      ULONG      SamplingFrequency
)
{
    ULONG BufferSizeCalc, DMATransferCountReload, BufferSizePerChan;
    BYTE Control;
    int i;

    PAGED_CODE();

    BufferSizePerChan = Format16Bit ? BufferSize / 2 : BufferSize;
    BufferSizeCalc = SamplingFrequency * NotificationInterval / 1000;
    if ( FormatStereo ) BufferSizeCalc *= 2;
    if ( BufferSizeCalc > BufferSizePerChan) BufferSizeCalc = BufferSizePerChan;
    DMATransferCountReload = 0x10001 - BufferSizeCalc;
    
    if ( Capture )
    {
        WRITE_PORT_UCHAR(m_pDMABase + ESSDM_REG_DMAMASK, 1); // Mask the DREQ
        
        WRITE_PORT_UCHAR(m_pDMABase + ESSDM_REG_DMAMODE, ESSDM_DMAMODE_AI | ESSDM_DMAMODE_TTYPE_WRITE);
        WRITE_PORT_USHORT((PUSHORT)(m_pDMABase + ESSDM_REG_DMAADDR), (USHORT)DMAChannelAddress);
        WRITE_PORT_USHORT((PUSHORT)(m_pDMABase + ESSDM_REG_DMAADDR + 2), HIWORD(DMAChannelAddress));
        WRITE_PORT_USHORT((PUSHORT)(m_pDMABase + ESSDM_REG_DMACOUNT), (USHORT)BufferSize - 1);
        WRITE_PORT_USHORT((PUSHORT)(m_pDMABase + ESSDM_REG_DMACOUNT + 2), 0);
        
        WRITE_PORT_UCHAR(m_pDMABase + ESSDM_REG_DMAMASK, 0); // Unmask the DREQ
        
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_RESET, 2);
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_RESET, 0);
        
        dspWrite(ESS_CMD_ENABLEEXT);
        
        dspWrite(ESS_CMD_DMATYPE);
        dspWrite(2);
        
        dspWrite(ESS_CMD_DMACNTRELOADL);
        dspWrite(LOBYTE(DMATransferCountReload));
        dspWrite(ESS_CMD_DMACNTRELOADH);
        dspWrite(HIBYTE(DMATransferCountReload));

        dspWrite(ESS_CMD_READREG);
        dspWrite(ESS_CMD_DMACONTROL);
        Control = dspRead() & 0x30 | 0x0E;
        dspWrite(ESS_CMD_DMACONTROL);
        dspWrite(Control);
        
        dspWrite(ESS_CMD_READREG);
        dspWrite(ESS_CMD_ANALOGCONTROL);
        Control = dspRead() & 8 | 0xF4;
        if ( FormatStereo ) Control |= 1; else Control |= 2;
        dspWrite(ESS_CMD_ANALOGCONTROL);
        dspWrite(Control);
        
        dspWrite(ESS_CMD_READREG);
        dspWrite(ESS_CMD_IRQCONTROL);
        Control = dspRead() | 0x50;
        dspWrite(ESS_CMD_IRQCONTROL);
        dspWrite(Control);
        
        dspWrite(ESS_CMD_READREG);
        dspWrite(ESS_CMD_DRQCONTROL);
        Control = dspRead() | 0x50;
        dspWrite(ESS_CMD_DRQCONTROL);
        dspWrite(Control);

        Control = FormatStereo ? 0x98 : 0xD0;
        if ( Format16Bit ) Control |= 0x24;
        dspWrite(ESS_CMD_SETFORMAT2);
        dspWrite(Control);
        
        dspWrite(ESS_CMD_READREG);
        dspWrite(ESS_CMD_DMACONTROL);
        Control = dspRead() | 1;
        dspWrite(ESS_CMD_DMACONTROL);
        dspWrite(Control);
        
        m_DmaStarted = TRUE;
        for (i=0; i<8; i++) KeStallExecutionProcessor(50);
    }
    else
    {
        UCHAR Mode;
        
        Mode = READ_PORT_UCHAR(m_pIOBase + ESSIO_REG_AUDIO2MODE);
        WRITE_PORT_UCHAR(m_pIOBase + ESSIO_REG_AUDIO2MODE, Mode & (~(ESSA2M_AIEN | ESSA2M_DMAEN)));
        WRITE_PORT_USHORT((PUSHORT)(m_pIOBase + ESSIO_REG_AUDIO2DMAADDR), LOWORD(DMAChannelAddress));
        WRITE_PORT_USHORT((PUSHORT)(m_pIOBase + ESSIO_REG_AUDIO2DMAADDR + 2), HIWORD(DMAChannelAddress));
        WRITE_PORT_USHORT((PUSHORT)(m_pIOBase + ESSDM_REG_DMACOUNT), (USHORT)BufferSize);
        
        dspWriteMixer(ESM_MIXER_AUDIO2_TCOUNT + 0, LOBYTE(DMATransferCountReload));
        dspWriteMixer(ESM_MIXER_AUDIO2_TCOUNT + 2, HIBYTE(DMATransferCountReload));
        
        Control = Format16Bit ? 5 : 0;
        if ( FormatStereo ) Control |= 2;
        dspWriteMixer(ESM_MIXER_AUDIO2_CTL2, Control | 0x40);
        dspWriteMixer(ESM_MIXER_AUDIO2_MODE, dspReadMixer(ESM_MIXER_AUDIO2_MODE) | 0x32);
        dspWriteMixer(ESM_MIXER_AUDIO2_CTL1, 0x93);
        WRITE_PORT_UCHAR(m_pIOBase + ESSIO_REG_AUDIO2MODE, Mode | (ESSA2M_AIEN | ESSA2M_DMAEN));
    }
}


/*****************************************************************************
 * CAdapterCommon::PauseDma()
 *****************************************************************************
 * Pauses DMA transfer
 */
STDMETHODIMP_(void)
CAdapterCommon::
PauseDma
(
    IN      BOOLEAN    Capture
)
{
    BYTE Control;
    
    PAGED_CODE();

    if ( Capture )
    {
        dspWrite(ESS_CMD_READREG);
        dspWrite(ESS_CMD_DMACONTROL);
        Control = dspRead() & (~0x04);
        dspWrite(ESS_CMD_DMACONTROL);
        dspWrite(Control);

        dspWrite(ESS_CMD_READREG);
        dspWrite(ESS_CMD_DMACONTROL);
        Control = dspRead() & (~0x01);
        dspWrite(ESS_CMD_DMACONTROL);
        dspWrite(Control);
    }
    else
    {
        WRITE_PORT_UCHAR(m_pIOBase + ESSIO_REG_AUDIO2MODE, 
            READ_PORT_UCHAR(m_pIOBase + ESSIO_REG_AUDIO2MODE) & (~(ESSA2M_AIEN | ESSA2M_DMAEN)));
        dspWriteMixer(ESM_MIXER_AUDIO2_CTL1, dspReadMixer(ESM_MIXER_AUDIO2_MODE) & (~0x10));
    }
}    

/*****************************************************************************
 * CAdapterCommon::StopDma()
 *****************************************************************************
 * Stops DMA transfer
 */
STDMETHODIMP_(void)
CAdapterCommon::
StopDma
(
    IN      BOOLEAN    Capture
)
{   
    int i;

    PAGED_CODE();

    if ( Capture )
    {
        WRITE_PORT_UCHAR(m_pDMABase + ESSDM_REG_DMAMASK, 1); // Mask the DREQ
        READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_READDATA);
        READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_STATUS);
        m_DmaStarted = FALSE;
    }
    else
    {
        WRITE_PORT_UCHAR(m_pIOBase + ESSIO_REG_AUDIO2MODE, 
            READ_PORT_UCHAR(m_pIOBase + ESSIO_REG_AUDIO2MODE) & (~(ESSA2M_AIEN | ESSA2M_DMAEN)));
        dspWriteMixer(ESM_MIXER_AUDIO2_CTL1, dspReadMixer(ESM_MIXER_AUDIO2_MODE) & (~0x10));
        
        for (i=0; i<20; i++) KeStallExecutionProcessor(50);
        
        dspWriteMixer(ESM_MIXER_AUDIO2_CTL1, 0);
        dspWriteMixer(ESM_MIXER_AUDIO2_CTL2, dspReadMixer(ESM_MIXER_AUDIO2_CTL2) & (~0x80));
    }
}    

/*****************************************************************************
 * CAdapterCommon::SetDspBase()
 *****************************************************************************
 * Set DSP base address
 */
STDMETHODIMP_(PUCHAR)
CAdapterCommon::
SetDspBase
(
    IN      PUCHAR  DspBase
)
{
    PUCHAR ret;
    
    PAGED_CODE();
    
    ret = m_pSBBase;
    m_pSBBase = DspBase;
    return ret;
}

/*****************************************************************************
 * CAdapterCommon::PowerChangeState()
 *****************************************************************************
 * Change power state for the device.
 */
STDMETHODIMP_(void)
CAdapterCommon::
PowerChangeState
(
    IN      POWER_STATE     NewState
)
{
    PAGED_CODE();

    if ( NewState.DeviceState > PowerDeviceD0 )
        NewState.DeviceState = PowerDeviceD3;

    // Is this actually a state change?
    if( NewState.DeviceState != m_PowerState )
    {
        // switch on new state
        switch( NewState.DeviceState )
        {
            case PowerDeviceD0:
                dspReset();
                dspWriteMixer(ESM_MIXER_OPAMP_CALIB, 1);
                Enable_Irq();
                WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_FMHIGHADDR, 5);
                KeStallExecutionProcessor(25);
                WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_FMLOWADDR + 1, 0x80);
                KeStallExecutionProcessor(25);
                WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERADDR, 0x64);
                if ( (READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERDATA) & 0x10) )
                {
                    WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERADDR, 0x66);
                    WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERDATA, 0x66);
                }
                RestoreConfig();
                break;

            case PowerDeviceD3:
                // This is a full hibernation state and is the longest latency sleep state.
                // The driver cannot access the hardware in this state and must cache any
                // hardware accesses and restore the hardware upon returning to D0 (or D1).                               

                SaveConfig();
                WRITE_PORT_UCHAR(m_pIOBase + ESSIO_REG_IRQCONTROL, 0);
                break;
    
            default:
                break;
        }
        

        // Save the new state.  This local value is used to determine when to cache
        // property accesses and when to permit the driver from accessing the hardware.
        m_PowerState = NewState.DeviceState;
    }
}


/*****************************************************************************
 * CAdapterCommon::QueryPowerChangeState()
 *****************************************************************************
 * Query to see if the device can
 * change to this power state
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
QueryPowerChangeState
(
    IN      POWER_STATE     NewStateQuery
)
{
    UNREFERENCED_PARAMETER(NewStateQuery);

    PAGED_CODE();
    
    _DbgPrintF( DEBUGLVL_TERSE, ("QueryPowerChangeState"));

    // Check here to see of a legitimate state is being requested
    // based on the device state and fail the call if the device/driver
    // cannot support the change requested.  Otherwise, return STATUS_SUCCESS.
    // Note: A QueryPowerChangeState() call is not guaranteed to always preceed
    // a PowerChangeState() call.

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CAdapterCommon::QueryDeviceCapabilities()
 *****************************************************************************
 * Called at startup to get the caps for the device.  This structure provides
 * the system with the mappings between system power state and device power
 * state.  This typically will not need modification by the driver.
 * 
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
QueryDeviceCapabilities
(
    IN      PDEVICE_CAPABILITIES    PowerDeviceCaps
)
{
    UNREFERENCED_PARAMETER(PowerDeviceCaps);

    PAGED_CODE();
    
    _DbgPrintF( DEBUGLVL_TERSE, ("QueryDeviceCapabilities"));

    return STATUS_UNSUCCESSFUL;
}

/*****************************************************************************
 * CAdapterCommon::DRM_SetFlags()
 *****************************************************************************
 * Sets the DRM flags
 */
STDMETHODIMP_(void)
CAdapterCommon::
DRM_SetFlags
(
    IN      BOOL    Mute,
    IN      BOOL    DigitalOutputDisable
)
{
    UNREFERENCED_PARAMETER(DigitalOutputDisable);

    if ( (RunTime & RUNTIME_FLAG_NODRM) == 0 && m_MuteInput != Mute )
    {
        m_MuteInput = Mute;
        dspWriteMixer(ESM_MIXER_EXTENDEDRECSRC, dspReadMixer(ESM_MIXER_EXTENDEDRECSRC));
    }
}

/*****************************************************************************
 * CAdapterCommon::RestoreConfig()
 *****************************************************************************
 * Restores saved card configuration
 */
STDMETHODIMP_(void)
CAdapterCommon::
RestoreConfig
(   void
)
{
    int i;
    
    PAGED_CODE();
    
    for (i=0; i<sizeof(SavedConfig)/sizeof(SavedConfig[0]); i++)
    {
        AccessConfigSpace(m_pDeviceObject, FALSE, &SavedConfig[i].Data, SavedConfig[i].Offset, sizeof(SavedConfig[i].Data));
    }
    
    dspWrite(ESS_CMD_ENABLEEXT);
    
    for (i=0; i<sizeof(ConfigSettings)/sizeof(ConfigSettings[0]); i++)
    {
        if ( ConfigSettings[i].IsMixer )
            dspWriteMixer(ConfigSettings[i].s.Register, ConfigSettings[i].s.Value);
        else
        {
            dspWrite((UCHAR)ConfigSettings[i].s.Register);
            dspWrite((UCHAR)ConfigSettings[i].s.Value);
        }
    }
    
    dspWriteMixer(ESM_MIXER_SERIALMODE_CTL, dspReadMixer(ESM_MIXER_SERIALMODE_CTL) | 0x10);
    if (!NoJoy)
    {
        DWORD Data = 0x201;
        
        AccessConfigSpace(m_pDeviceObject, FALSE, &Data, ESM_GAMEPORT, sizeof(Data));
    }
}

/*****************************************************************************
 * CAdapterCommon::SaveConfig()
 *****************************************************************************
 * Restores saved card configuration
 */
STDMETHODIMP_(void)
CAdapterCommon::
SaveConfig
(   void
)
{
    int i;
    
    PAGED_CODE();
    
    for (i=0; i<sizeof(SavedConfig)/sizeof(SavedConfig[0]); i++)
    {
        AccessConfigSpace(m_pDeviceObject, TRUE, &SavedConfig[i].Data, SavedConfig[i].Offset, sizeof(SavedConfig[i].Data));
    }
    
    dspWrite(ESS_CMD_ENABLEEXT);
    
    for (i=0; i<sizeof(ConfigSettings)/sizeof(ConfigSettings[0]); i++)
    {
        if ( ConfigSettings[i].IsMixer )
            ConfigSettings[i].s.Value = dspReadMixer(ConfigSettings[i].s.Register);
        else
        {
            dspWrite(ESS_CMD_READREG);
            dspWrite((UCHAR)ConfigSettings[i].s.Register);
            ConfigSettings[i].s.Value = dspRead();
        }
    }
}

/*****************************************************************************
 * CAdapterCommon::SetClkRunEnable()
 *****************************************************************************
 * Enable clock
 */
STDMETHODIMP_(void)
CAdapterCommon::
SetClkRunEnable
(
    IN      BOOLEAN    Enable
)
{   
    UNREFERENCED_PARAMETER(Enable);

    PAGED_CODE();

}    

/*****************************************************************************
 * CAdapterCommon::SetRecordingSource()
 *****************************************************************************
 * Sets a new recording source
 */
STDMETHODIMP_(void)
CAdapterCommon::
SetRecordingSource
(
    IN      PDWORD     RecordingSource
)
{   
    PAGED_CODE();

    m_pRecordingSource = RecordingSource;
}    

/*****************************************************************************
 * CAdapterCommon::BoardId()
 *****************************************************************************
 * Get the Board ID of the current sound board
 */
STDMETHODIMP_(DWORD)
CAdapterCommon::
BoardId
(   void
)
{
    PAGED_CODE();
    
    return m_BoardId;
}

/*****************************************************************************
 * CAdapterCommon::IsRecording()
 *****************************************************************************
 * Check if recording
 */
STDMETHODIMP_(BOOL)
CAdapterCommon::
IsRecording
(   void
)
{
    PAGED_CODE();
    
    return m_Recording;
}

/*****************************************************************************
 * CAdapterCommon::StartRecording()
 *****************************************************************************
 * Starts recording
 */
STDMETHODIMP_(void)
CAdapterCommon::
StartRecording
(
    IN      BOOLEAN    Recording
)
{   
    SETTING *pMixerTables;
    BYTE Control;
    unsigned int i;
    
    PAGED_CODE();

    if (Recording)
        pMixerTables = m_pMixerVolumesIn;
    else
    {
        dspReset();
        pMixerTables = m_pMixerVolumesOut;
    }
    
    for (i=0; i<10; i++)
    {
        if (m_Recording)
        {
            switch (i)
            {
                case 0:
                    if (!IsMidiActive()) break;
                case 1:
                case 2:
                case 3:
                case 4:
                    if ( *m_pRecordingSource == i + 1 )
                        dspWriteMixer(pMixerTables[i].Register, pMixerTables[i].Value);
                    break;
                case 5:
                    if ( *m_pRecordingSource == i + 1 && !IsMidiActive() )
                        dspWriteMixer(pMixerTables[i].Register, pMixerTables[i].Value);
                    break;
                case 6:
                    if ( *m_pRecordingSource == i + 1 )
                    {
                        dspWrite(ESS_CMD_ENABLEEXT);
                        dspWrite(ESS_CMD_RECLEVEL);
                        dspWrite(pMixerTables[i].Value);
                        dspWriteMixer(ESM_MIXER_AUDIO1_VOL, 0);
                    }
                    break;
                case 7:
                    dspWrite(ESS_CMD_ENABLEEXT);
                    dspWrite(ESS_CMD_READREG);
                    dspWrite(ESS_CMD_ANALOGCONTROL);
                    Control = dspRead() | 0x70;
                    if (*m_pRecordingSource == i || pMixerTables[i].Value == 0)
                        Control &= ~8;
                    else
                        Control |= 8;
                    dspWrite(ESS_CMD_ANALOGCONTROL);
                    dspWrite(Control);
                    break;
                default:
                    if (pMixerTables[i].Register)
                        dspWriteMixer(pMixerTables[i].Register, pMixerTables[i].Value);
                    break;
            }
        }
        else
        {
            if ( (i == 1 && IsMidiActive()) || (i == 6 && !IsMidiActive()) || pMixerTables[i].Register )
                dspWriteMixer(pMixerTables[i].Register, pMixerTables[i].Value);
        }
    }
}

/*****************************************************************************
 * CAdapterCommon::SetMixerTables()
 *****************************************************************************
 * Sets the Mixer tables
 */
STDMETHODIMP_(void)
CAdapterCommon::
SetMixerTables
(
        IN      PUCHAR     Table1,
        IN      PUCHAR     Table0
)
{   
    PAGED_CODE();
    
    m_pMixerVolumesOut = (SETTING*)Table1;
    m_pMixerVolumesIn = (SETTING*)Table0;
}

/*****************************************************************************
 * CAdapterCommon::GetDeviceID()
 *****************************************************************************
 * Get the Board ID of the current sound board
 */
STDMETHODIMP_(USHORT)
CAdapterCommon::
GetDeviceID
(   void
)
{
    return m_DeviceID;
}


/*****************************************************************************
 * CAdapterCommon::GetPosition()
 *****************************************************************************
 * Get the current position
 */
STDMETHODIMP_(USHORT)
CAdapterCommon::
GetPosition
(
    IN      BOOLEAN    Capture,
    IN      UINT       DmaBufferSize
)
{
    int i, j;
    USHORT Pos;

    if ( !Capture )
        return READ_PORT_USHORT((PUSHORT)(m_pIOBase + ESSIO_REG_AUDIO2DMACOUNT));
    
    WRITE_PORT_UCHAR(m_pDMABase + ESSDM_REG_DMAMASK, 1);

    for (i=0; i<6; i++)
    {
        for (j=0; j<10; j++)
            READ_PORT_USHORT((PUSHORT)(m_pDMABase + ESSDM_REG_DMACOUNT));
        Pos = READ_PORT_USHORT((PUSHORT)(m_pDMABase + ESSDM_REG_DMACOUNT));
        if (DmaBufferSize - Pos - 1 < DmaBufferSize ) break;
    }
    if (i >= 6) Pos = 0;
    
    WRITE_PORT_UCHAR(m_pDMABase + ESSDM_REG_DMAMASK, 0);

    return Pos;
}



#pragma code_seg()

/*****************************************************************************
 * CAdapterCommon::dspReadMixer()
 *****************************************************************************
 * Reads from the Mixer
 */
STDMETHODIMP_(UCHAR)
CAdapterCommon::
dspReadMixer
(
    IN  UCHAR   Address
)
{
    WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERADDR, Address);
    return READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERDATA);
}


/*****************************************************************************
 * CAdapterCommon::dspWriteMixer()
 *****************************************************************************
 * Writes to the Mixer
 */
STDMETHODIMP_(void)
CAdapterCommon::
dspWriteMixer
(
    IN  UCHAR Address,
    IN  UCHAR Value
)
{
    if (Address == ESM_MIXER_EXTENDEDRECSRC)
    {
        if (m_MuteInput)
            Value |= 0x10;
        else
            Value &= ~0x10;
    }
    WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERADDR, Address);
    WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERDATA, Value);
}

/*****************************************************************************
 * InterruptServiceRoutine()
 *****************************************************************************
 * ISR.
 */
NTSTATUS
InterruptServiceRoutine
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    UCHAR MixerAddr, Control;
    
    UNREFERENCED_PARAMETER(InterruptSync);
    ASSERT(DynamicContext);

    CAdapterCommon *that = (CAdapterCommon *) DynamicContext;

    Control = READ_PORT_UCHAR(that->m_pIOBase + ESSIO_REG_IRQCONTROL);
    if ( Control == 0xFF || (Control & 0xF0) == 0 )
        return STATUS_UNSUCCESSFUL;   
    if (!that->m_pSBBase) return STATUS_SUCCESS;
    
    MixerAddr = READ_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERADDR);
    WRITE_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERADDR, ESM_MIXER_MASTER_VOL_CTL);
    if ((READ_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERDATA) & 0x10) == 0)
    {
        if (that->m_pPortWave && that->m_pWaveServiceGroup)
            that->m_pPortWave->Notify(that->m_pWaveServiceGroup);
        WRITE_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERADDR, ESM_MIXER_AUDIO2_CTL2);
        Control = READ_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERDATA);
        if (!(Control & 0x80))
        {
            READ_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_STATUS);
        }
        else
        {
            WRITE_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERADDR, ESM_MIXER_AUDIO2_CTL2);
            WRITE_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERDATA, Control & (~0x80));
        }
        WRITE_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERADDR, MixerAddr);
        return STATUS_SUCCESS;
    }

    WRITE_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERADDR, ESM_MIXER_CLRHWVOLIRQ);
    WRITE_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERDATA, ESM_MIXER_CLRHWVOLIRQ);
    WRITE_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERADDR, ESM_MIXER_CLRHWVOLIRQ);
    Control = READ_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERDATA) & 0x40;
    WRITE_PORT_UCHAR(that->m_pSBBase + ESSSB_REG_MIXERADDR, MixerAddr);
    
    if (that->m_pPortEvents)
    {
        if (Control == that->m_MuteOutput )
        {
            that->m_pPortEvents->GenerateEventList(NULL, KSEVENT_CONTROL_CHANGE, FALSE, 
                ULONG(-1), TRUE, LINEOUT_VOL);
            that->m_HardwareVolumeFlag |= 2;
        }
        else
        {
            that->m_pPortEvents->GenerateEventList(NULL, KSEVENT_CONTROL_CHANGE, FALSE, 
                ULONG(-1), TRUE, LINEOUT_MUTE);
            that->m_HardwareVolumeFlag = Control == 0 ? 3 : 1;
        }
    }
    
    return 1;  // BUGBUG?
}
