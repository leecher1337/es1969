/*****************************************************************************
 * miniport.cpp - ESS wave miniport implementation
 *****************************************************************************
 * Copyright (c) 2023 leecher@dose.0wnz.at  All rights reserved.
 */

#include "minwave.h"

#define STR_MODULENAME "minwave: "



#pragma code_seg("PAGE")

/*****************************************************************************
 * CreateMiniportWaveSolo()
 *****************************************************************************
 * Creates a cyclic wave miniport object for the Solo adapter.  This uses a
 * macro from STDUNK.H to do all the work.
 */
NTSTATUS
CreateMiniportWaveSolo
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY_(CMiniportWaveSolo,Unknown,UnknownOuter,PoolType,PMINIPORTWAVECYCLIC);
}

/*****************************************************************************
 * CMiniportWaveSolo::ProcessResources()
 *****************************************************************************
 * Processes the resource list, setting up helper objects accordingly.
 */
NTSTATUS
CMiniportWaveSolo::
ProcessResources
(
    IN      PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();

    ASSERT(ResourceList);


    //
    // Get counts for the types of resources.
    //
    // FIXME: Nobody knows what it is good for, but it's in the original driver for some reason...
    /*
    ULONG   countIO     = ResourceList->NumberOfPorts();
    ULONG   countIRQ    = ResourceList->NumberOfInterrupts();
    */
#if (DBG)
    ULONG   countDMA    = ResourceList->NumberOfDmas();
#endif

    
    ULONG   lDMABufferLength;
    PHYSICAL_ADDRESS BufferAddress;

    // (sic) Yes, they really output "SB16 wave" in checked build of ESS driver!
#if (DBG)
    _DbgPrintF(DEBUGLVL_VERBOSE,("Starting SB16 wave on IRQ 0x%X",
        ResourceList->FindUntranslatedInterrupt(0)->u.Interrupt.Level) );

    _DbgPrintF(DEBUGLVL_VERBOSE,("Starting SB16 wave on Port 0x%X",
        ResourceList->FindTranslatedPort(0)->u.Port.Start.LowPart) );

    for (ULONG i = 0; i < countDMA; i++)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Starting SB16 wave on DMA 0x%X",
            ResourceList->FindUntranslatedDma(i)->u.Dma.Channel) );
    }
#endif

    NTSTATUS ntStatus = STATUS_SUCCESS;
    
    if (AdapterCommon->GetDeviceID() == 0x1938 || AdapterCommon->GetDeviceID() == 0x1969)
    {
        lDMABufferLength = MAXLEN_DMA_BUFFER - 4;
        BufferAddress.LowPart = MAXLEN_DMA_BUFFER - 1;
    }
    else
    {
        lDMABufferLength = MAXLEN_DMA_BUFFER;
        BufferAddress.LowPart = 0xFFFF;
    }
    BufferAddress.HighPart = 0;

    //
    // Instantiate a DMA channel for 8-bit transfers.
    //
    ntStatus =
        Port->NewMasterDmaChannel
        (
            &DmaChannel8,
            NULL,
            ResourceList,
            lDMABufferLength,
            FALSE,
            FALSE,
            Width8Bits,
            MaximumDmaSpeed
        );

    //
    // Allocate the buffer for 8-bit transfers.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = DmaChannel8->AllocateBuffer(lDMABufferLength, &BufferAddress);

        if (NT_SUCCESS(ntStatus))
        {
            if (!DmaChannel8->PhysicalAddress().LowPart) ntStatus = STATUS_UNSUCCESSFUL;

            if (NT_SUCCESS(ntStatus))
            {
                //
                // Instantiate a DMA channel for 16-bit transfers.
                //
                ntStatus =
                    Port->NewMasterDmaChannel
                    (
                        &DmaChannel16,
                        NULL,
                        ResourceList,
                        MAXLEN_DMA_BUFFER,
                        FALSE,
                        FALSE,
                        Width8Bits,
                        MaximumDmaSpeed
                    );

                //
                // Allocate the buffer for 16-bit transfers.
                //
                if (NT_SUCCESS(ntStatus))
                {
                    BufferAddress.QuadPart = 0xFFFFFi64;
                    ntStatus = DmaChannel16->AllocateBuffer(MAXLEN_DMA_BUFFER, &BufferAddress);
                }
            }
        }
    }

    //
    // In case of failure object gets destroyed and cleans up.
    //
    if (!NT_SUCCESS(ntStatus))
    {
        if (DmaChannel8)
        {
            DmaChannel8->Release();
            DmaChannel8 = NULL;
        }

        if (DmaChannel16)
        {
            DmaChannel16->Release();
            DmaChannel16 = NULL;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveSolo::ValidateFormat()
 *****************************************************************************
 * Validates a wave format.
 */
NTSTATUS
CMiniportWaveSolo::
ValidateFormat
(
    IN      PKSDATAFORMAT   Format
)
{
    PAGED_CODE();

    ASSERT(Format);

    NTSTATUS ntStatus;

    //
    // A WAVEFORMATEX structure should appear after the generic KSDATAFORMAT
    // if the GUIDs turn out as we expect.
    //
    PWAVEFORMATEX waveFormat = PWAVEFORMATEX(Format + 1);

    //
    // KSDATAFORMAT contains three GUIDs to support extensible format.  The
    // first two GUIDs identify the type of data.  The third indicates the
    // type of specifier used to indicate format specifics.  We are only
    // supporting PCM audio formats that use WAVEFORMATEX.
    //
    if  (   (Format->FormatSize >= sizeof(KSDATAFORMAT_WAVEFORMATEX))
        &&  IsEqualGUIDAligned(Format->MajorFormat,KSDATAFORMAT_TYPE_AUDIO)
        &&  IsEqualGUIDAligned(Format->SubFormat,KSDATAFORMAT_SUBTYPE_PCM)
        &&  IsEqualGUIDAligned(Format->Specifier,KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        &&  (waveFormat->wFormatTag == WAVE_FORMAT_PCM)
        &&  ((waveFormat->wBitsPerSample == 8) ||  (waveFormat->wBitsPerSample == 16))
        &&  ((waveFormat->nChannels == 1) ||  (waveFormat->nChannels == 2))
        &&  ((waveFormat->nSamplesPerSec >= 5000) &&  (waveFormat->nSamplesPerSec <= 48000))
        )
    {
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveSolo::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP
CMiniportWaveSolo::
NonDelegatingQueryInterface
(
    IN      REFIID  Interface,
    OUT     PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTWAVECYCLIC(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportWaveCyclic))
    {
        *Object = PVOID(PMINIPORTWAVECYCLIC(this));
    }
    else if (IsEqualGUIDAligned (Interface, IID_IPowerNotify))
    {
        *Object = (PVOID)(PPOWERNOTIFY)this;
    } 
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        //
        // We reference the interface for the caller.
        //
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*****************************************************************************
 * CMiniportWaveSolo::~CMiniportWaveSolo()
 *****************************************************************************
 * Destructor.
 */
CMiniportWaveSolo::
~CMiniportWaveSolo
(   void
)
{
    PAGED_CODE();

    if (Port)
    {
        Port->Release();
        Port = NULL;
    }
    if (DmaChannel8)
    {
        DmaChannel8->Release();
        DmaChannel8 = NULL;
    }
    if (DmaChannel16)
    {
        DmaChannel16->Release();
        DmaChannel16 = NULL;
    }
    if (ServiceGroup)
    {
        ServiceGroup->Release();
        ServiceGroup = NULL;
    }
    if (AdapterCommon)
    {
        AdapterCommon->Release();
        AdapterCommon = NULL;
    }

}

/*****************************************************************************
 * CMiniportWaveSolo::Init()
 *****************************************************************************
 * Initializes a the miniport.
 */
STDMETHODIMP
CMiniportWaveSolo::
Init
(
    IN      PUNKNOWN        UnknownAdapter,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTWAVECYCLIC Port_
)
{
    PAGED_CODE();

    ASSERT(UnknownAdapter);
    ASSERT(ResourceList);
    ASSERT(Port_);

    //
    // AddRef() is required because we are keeping this pointer.
    //
    Port = Port_;
    Port->AddRef();
    
    //
    // Initialize the member variables.
    //
    Running = 0;
    PowerState = PowerSystemWorking;

    //
    // We want the IAdapterCommon interface on the adapter common object,
    // which is given to us as a IUnknown.  The QueryInterface call gives us
    // an AddRefed pointer to the interface we want.
    //
    NTSTATUS ntStatus =
        UnknownAdapter->QueryInterface
        (
            IID_IAdapterCommon,
            (PVOID *) &AdapterCommon
        );

    //
    // We need a service group for notifications.  We will bind all the
    // streams that are created to this single service group.  All interrupt
    // notifications ask for service on this group, so all streams will get
    // serviced.  The PcNewServiceGroup() call returns an AddRefed pointer.
    // The adapter needs a copy of the service group since it is doing the
    // ISR.
    //
    if (NT_SUCCESS(ntStatus))
    {
        KeInitializeMutex(&SampleRateSync,1);
        ntStatus = PcNewServiceGroup(&ServiceGroup,NULL);
    }

    if (NT_SUCCESS(ntStatus))
    {
        AdapterCommon->SetWaveServiceGroup (ServiceGroup);
        ntStatus = ProcessResources(ResourceList);
    }
    
    if (!NT_SUCCESS(ntStatus))
    {
        if (ServiceGroup)
        {
            if (AdapterCommon) AdapterCommon->SetWaveServiceGroup (NULL);
            ServiceGroup->Release();
            ServiceGroup = NULL;
        }
        if (AdapterCommon)
        {
            AdapterCommon->Release();
            AdapterCommon = NULL;
        }
        Port->Release();
        Port = NULL;
    }

    //
    // In case of failure object gets destroyed and destructor cleans up.
    //

    return ntStatus;
}

/*****************************************************************************
 * PinDataRangesStream
 *****************************************************************************
 * Structures indicating range of valid format values for streaming pins.
 */
static
KSDATARANGE_AUDIO PinDataRangesStream[] =
{
    {
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        2,      // Max number of channels.
        8,      // Minimum number of bits per sample.
        16,     // Maximum number of bits per channel.
        5000,   // Minimum rate.
        48000   // Maximum rate.
    }
};

/*****************************************************************************
 * PinDataRangePointersStream
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for streaming pins.
 */
static
PKSDATARANGE PinDataRangePointersStream[] =
{
    PKSDATARANGE(&PinDataRangesStream[0])
};

/*****************************************************************************
 * PinDataRangesBridge
 *****************************************************************************
 * Structures indicating range of valid format values for bridge pins.
 */
static
KSDATARANGE _PinDataRangesBridge[] =
{
   {
      sizeof(KSDATARANGE),
      0,
      0,
      0,
      STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
      STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
   }
};

/*****************************************************************************
 * PinDataRangePointersBridge
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for bridge pins.
 */
static
PKSDATARANGE _PinDataRangePointersBridge[] =
{
    &_PinDataRangesBridge[0]
};

/*****************************************************************************
 * MiniportPins
 *****************************************************************************
 * List of pins.
 */
static
PCPIN_DESCRIPTOR 
_MiniportPins[] =
{
    // Wave In Streaming Pin (Capture)
    {
        1,1,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(PinDataRangePointersStream),
            PinDataRangePointersStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            (GUID *) &PINNAME_CAPTURE,
            &KSAUDFNAME_RECORDING_CONTROL,  // this name shows up as the recording panel name in SoundVol.
            0
        }
    },
    // Wave In Bridge Pin (Capture - From Topology)
    {
        0,0,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(_PinDataRangePointersBridge),
            _PinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            (GUID *) &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer)
    {
        1,1,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(PinDataRangePointersStream),
            PinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            (GUID *) &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Bridge Pin (Renderer)
    {
        0,0,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(_PinDataRangePointersBridge),
            _PinDataRangePointersBridge,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_NONE,
            (GUID *) &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    }
};

/*****************************************************************************
 * TopologyNodes
 *****************************************************************************
 * List of nodes.
 */
static
PCNODE_DESCRIPTOR MiniportNodes[] =
{
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_ADC,        // Type
        NULL                    // Name
    },
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_DAC,        // Type
        NULL                    // Name
    }
};

/*****************************************************************************
 * MiniportConnections
 *****************************************************************************
 * List of connections.
 */
static
PCCONNECTION_DESCRIPTOR _MiniportConnections[] =
{
    { PCFILTER_NODE,  1,  0,                1 },    // Bridge in to ADC.
    { 0,              0,  PCFILTER_NODE,    0 },    // ADC to stream pin (capture).
    { PCFILTER_NODE,  2,  1,                1 },    // Stream in to DAC.
    { 1,              0,  PCFILTER_NODE,    3 }     // DAC to Bridge.
};

/*****************************************************************************
 * MiniportFilterDescriptor
 *****************************************************************************
 * Complete miniport description.
 */
static
PCFILTER_DESCRIPTOR 
MiniportWaveSoloDescriptor =
{
    0,                                  // Version
    NULL,                               // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    SIZEOF_ARRAY(_MiniportPins),        // PinCount
    _MiniportPins,                      // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    SIZEOF_ARRAY(MiniportNodes),        // NodeCount
    MiniportNodes,                      // Nodes
    SIZEOF_ARRAY(_MiniportConnections), // ConnectionCount
    _MiniportConnections,               // Connections
    0,                                  // CategoryCount
    NULL                                // Categories  - use the default categories (audio, render, capture)
};

/*****************************************************************************
 * CMiniportWaveSolo::GetDescription()
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP
CMiniportWaveSolo::
GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    *OutFilterDescriptor = &MiniportWaveSoloDescriptor;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportWaveSolo::DataRangeIntersection()
 *****************************************************************************
 * Tests a data range intersection.
 */
STDMETHODIMP 
CMiniportWaveSolo::
DataRangeIntersection
(   
    IN      ULONG           PinId,
    IN      PKSDATARANGE    ClientDataRange,
    IN      PKSDATARANGE    MyDataRange,
    IN      ULONG           OutputBufferLength,
    OUT     PVOID           ResultantFormat,
    OUT     PULONG          ResultantFormatLength
)
{
    UNREFERENCED_PARAMETER(PinId);
    UNREFERENCED_PARAMETER(ClientDataRange);
    UNREFERENCED_PARAMETER(MyDataRange);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(ResultantFormat);
    UNREFERENCED_PARAMETER(ResultantFormatLength);

    return STATUS_NOT_IMPLEMENTED;
}

/*****************************************************************************
 * CMiniportWaveSolo::PowerChangeNotify()
 *****************************************************************************
 * Change power state for the device.
 */
STDMETHODIMP_(void)
CMiniportWaveSolo::
PowerChangeNotify
(
    IN      POWER_STATE     NewState
)
{
    PAGED_CODE();

    // Is this actually a state change?
    if( NewState.SystemState != PowerState )
    {
        // switch on new state
        switch( NewState.SystemState )
        {
            case PowerSystemWorking:
                if (AllocatedCapture)
                {
                    AdapterCommon->dspWrite(ESS_CMD_EXTSAMPLERATE);
                    AdapterCommon->dspWrite(SaveExtSampleRate);
                    AdapterCommon->dspWrite(ESS_CMD_FILTERDIV);
                    AdapterCommon->dspWrite(SaveFilterDiv);
                }
                if (AllocatedRender)
                {
                    AdapterCommon->dspWriteMixer(ESM_MIXER_AUDIO2_SR, SaveA2SampleRate);
                    AdapterCommon->dspWriteMixer(ESM_MIXER_AUDIO2_CLKRATE, SaveA2ClockRate);
                }
                break;

            case PowerSystemSleeping1:
            case PowerSystemSleeping2:
            case PowerSystemSleeping3:
                if (AllocatedCapture)
                {
                    AdapterCommon->dspWrite(ESS_CMD_READREG);
                    AdapterCommon->dspWrite(ESS_CMD_EXTSAMPLERATE);
                    SaveExtSampleRate = AdapterCommon->dspRead();
                    AdapterCommon->dspWrite(ESS_CMD_READREG);
                    AdapterCommon->dspWrite(ESS_CMD_FILTERDIV);
                    SaveFilterDiv = AdapterCommon->dspRead();
                }
                if (AllocatedRender)
                {
                    SaveA2SampleRate = AdapterCommon->dspReadMixer(ESM_MIXER_AUDIO2_SR);
                    SaveA2ClockRate = AdapterCommon->dspReadMixer(ESM_MIXER_AUDIO2_CLKRATE);
                }
                break;
    
            default:
                break;
        }
        
        PowerState = NewState.SystemState;
    }
}

/*****************************************************************************
 * CMiniportWaveSolo::NewStream()
 *****************************************************************************
 * Creates a new stream.  This function is called when a streaming pin is
 * created.
 */
STDMETHODIMP
CMiniportWaveSolo::
NewStream
(
    OUT     PMINIPORTWAVECYCLICSTREAM * OutStream,
    IN      PUNKNOWN                    OuterUnknown,
    IN      POOL_TYPE                   PoolType,
    IN      ULONG                       Channel,
    IN      BOOLEAN                     Capture,
    IN      PKSDATAFORMAT               DataFormat,
    OUT     PDMACHANNEL *               OutDmaChannel,
    OUT     PSERVICEGROUP *             OutServiceGroup
)
{
    PAGED_CODE();

    ASSERT(OutStream);
    ASSERT(DataFormat);
    ASSERT(OutDmaChannel);
    ASSERT(OutServiceGroup);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Make sure the hardware is not already in use.
    //
    if (Capture)
    {
        if (AllocatedCapture)
        {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    else
    {
        if (AllocatedRender)
        {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    //
    // Determine if the format is valid.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ValidateFormat(DataFormat);
    }

    PDMACHANNEL         dmaChannel = NULL;

    //
    // Get the required DMA channel if it's not already in use.
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (Capture)
        {
            dmaChannel = DmaChannel8;
        }
        else
        {
            dmaChannel = DmaChannel16;
        }
        
        //
        // Instantiate a stream.
        //
        CMiniportWaveStreamSolo *stream =
            new(PoolType) CMiniportWaveStreamSolo(OuterUnknown);

        if (stream)
        {
            stream->AddRef();

            ntStatus =
                stream->Init
                (
                    this,
                    Channel,
                    Capture,
                    DataFormat,
                    dmaChannel
                );

            if (NT_SUCCESS(ntStatus))
            {
                if (Capture)
                {
                    AllocatedCapture = TRUE;
                }
                else
                {
                    AllocatedRender = TRUE;
                }

                *OutStream = PMINIPORTWAVECYCLICSTREAM(stream);
                stream->AddRef();

                *OutDmaChannel = dmaChannel;
                dmaChannel->AddRef();

                *OutServiceGroup = ServiceGroup;
                ServiceGroup->AddRef();

                //
                // The stream, the DMA channel, and the service group have
                // references now for the caller.  The caller expects these
                // references to be there.
                //
            }

            //
            // This is our private reference to the stream.  The caller has
            // its own, so we can release in any case.
            //
            stream->Release();
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveStreamSolo::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP
CMiniportWaveStreamSolo::
NonDelegatingQueryInterface
(
    IN      REFIID  Interface,
    OUT     PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTWAVECYCLICSTREAM(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportWaveCyclicStream))
    {
        *Object = PVOID(PMINIPORTWAVECYCLICSTREAM(this));
    }
    else 
    if (IsEqualGUIDAligned (Interface, IID_IDrmAudioStream))
    {
        *Object = (PVOID)(PDRMAUDIOSTREAM(this));
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
 * CMiniportWaveStreamSolo::~CMiniportWaveStreamSolo()
 *****************************************************************************
 * Destructor.
 */
CMiniportWaveStreamSolo::
~CMiniportWaveStreamSolo
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveStreamSolo::~CMiniportWaveStreamSolo]"));

    if (DmaChannel)
    {
        DmaChannel->Release();
    }

    if (Miniport)
    {
        //
        // Clear allocation flags in the miniport.
        //
        if (Capture)
        {
            Miniport->AllocatedCapture = FALSE;
            Miniport->AdapterCommon->StartRecording(FALSE);
        }
        else
        {
            Miniport->AllocatedRender = FALSE;
            Miniport->AdapterCommon->DRM_SetFlags(FALSE, FALSE);
        }

        Miniport->Release();
        Miniport = NULL;
    }
}

/*****************************************************************************
 * CMiniportWaveStreamSolo::Init()
 *****************************************************************************
 * Initializes a stream.
 */
NTSTATUS
CMiniportWaveStreamSolo::
Init
(
    IN      CMiniportWaveSolo*          Miniport_,
    IN      ULONG                       Channel_,
    IN      BOOLEAN                     Capture_,
    IN      PKSDATAFORMAT               DataFormat,
    IN      PDMACHANNEL                 DmaChannel_
)
{
    PAGED_CODE();

    ASSERT(Miniport_);
    ASSERT(DataFormat);
    ASSERT(NT_SUCCESS(Miniport_->ValidateFormat(DataFormat)));
    ASSERT(DmaChannel_);

    PWAVEFORMATEX waveFormat = PWAVEFORMATEX(DataFormat + 1);

    //
    // We must add references because the caller will not do it for us.
    //
    Miniport = Miniport_;
    Miniport->AddRef();

    DmaChannel = DmaChannel_;
    DmaChannel->AddRef();

    Channel         = Channel_;
    Capture         = Capture_;
    FormatStereo    = (waveFormat->nChannels == 2);
    Format16Bit     = (waveFormat->wBitsPerSample == 16);
    State           = KSSTATE_STOP;

    KeWaitForSingleObject
    (
        &Miniport->SampleRateSync,
        Executive,
        KernelMode,
        FALSE,
        NULL
    );
    Miniport->SamplingFrequency = waveFormat->nSamplesPerSec;
    KeReleaseMutex(&Miniport->SampleRateSync,FALSE);
    
    return SetFormat( DataFormat );
}

/*****************************************************************************
 * CMiniportWaveStreamSolo::SetNotificationFreq()
 *****************************************************************************
 * Sets the notification frequency.
 */
STDMETHODIMP_(ULONG)
CMiniportWaveStreamSolo::
SetNotificationFreq
(
    IN      ULONG   Interval,
    OUT     PULONG  FramingSize    
)
{
    PAGED_CODE();

    Miniport->NotificationInterval = Interval;
    //
    //  This value needs to be sample block aligned for DMA to work correctly.
    //
    *FramingSize = 
        (1 << (FormatStereo + Format16Bit)) * 
            (Miniport->SamplingFrequency * Interval / 1000);

    return Miniport->NotificationInterval;
}

unsigned char divisor_tab[56] =
{
  0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x88, 0x00, 
  0x00, 0x00, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 
  0x00, 0x00, 0x03, 0x05, 0x05, 0x05, 0x06, 0x06, 
  0x06, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 
  0x08, 0x09, 0x09, 0x09, 0x00, 0x00, 0x03, 0x05, 
  0x05, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x08, 
  0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x0A, 0x0A,
};

/*****************************************************************************
 * CMiniportWaveStreamSolo::SetFormat()
 *****************************************************************************
 * Sets the wave format.
 */
STDMETHODIMP
CMiniportWaveStreamSolo::
SetFormat
(
    IN      PKSDATAFORMAT   Format
)
{
    PAGED_CODE();

    ASSERT(Format);

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    ntStatus = Miniport->ValidateFormat(Format);

    PWAVEFORMATEX waveFormat = PWAVEFORMATEX(Format + 1);

    if (NT_SUCCESS(ntStatus))
    {
        BYTE FilterDiv, SampleRate, Divisor, Divisor2;
        ULONG Freq, Freq2;
        
        _DbgPrintF(DEBUGLVL_VERBOSE,("Set Fmt SR:%d BPS:%d CH:%d ",
            waveFormat->nSamplesPerSec,
            waveFormat->wBitsPerSample,
            waveFormat->nChannels) );

        Miniport->SamplingFrequency = waveFormat->nSamplesPerSec;

        Miniport->AdapterCommon->dspWriteMixer(ESM_MIXER_AUDIO2_MODE, 
            Miniport->AdapterCommon->dspReadMixer(ESM_MIXER_AUDIO2_MODE) | 0x20);
        
        Divisor = (BYTE)(768000 / waveFormat->nSamplesPerSec);
        if (768000 % waveFormat->nSamplesPerSec > waveFormat->nSamplesPerSec / 2)
            Divisor++;
        Freq = 768000 / Divisor;
        if ( 768000 % Divisor > Divisor / 2 ) Freq++;
        if (Freq <= waveFormat->nSamplesPerSec)
            Freq = waveFormat->nSamplesPerSec - Freq;
        else
            Freq -= waveFormat->nSamplesPerSec;
        SampleRate = -Divisor | 0x80;

        Divisor2 = (BYTE)(793800 / waveFormat->nSamplesPerSec);
        if (793800 % waveFormat->nSamplesPerSec > waveFormat->nSamplesPerSec / 2)
            Divisor2++;
        Freq2 = 793800 / Divisor2;
        if ( 793800 % Divisor2 > Divisor2 / 2 ) Freq2++;
        if (Freq2 <= waveFormat->nSamplesPerSec)
            Freq2 = waveFormat->nSamplesPerSec - Freq2;
        else
            Freq2 -= waveFormat->nSamplesPerSec;
        if ( Freq > Freq2 )
            SampleRate = -Divisor2 & (~0x80);

        if (Capture)
        {
            Miniport->AdapterCommon->dspWrite(ESS_CMD_EXTSAMPLERATE);
            Miniport->AdapterCommon->dspWrite((BYTE)SampleRate);
            if (waveFormat->nSamplesPerSec <= 22050 || SampleRate < 18 || SampleRate > 35)
                FilterDiv = (BYTE)(199582 / waveFormat->nSamplesPerSec);
            else
                FilterDiv = divisor_tab[SampleRate + (Capture?20:0) ];
            FilterDiv = -FilterDiv;
            
            Miniport->AdapterCommon->dspWrite(ESS_CMD_FILTERDIV);
            Miniport->AdapterCommon->dspWrite((BYTE)FilterDiv);
        }
        else
        {
            Miniport->AdapterCommon->dspWriteMixer(ESM_MIXER_AUDIO2_SR, (BYTE)SampleRate);
            if (waveFormat->nSamplesPerSec == 44100 || waveFormat->nSamplesPerSec == 48000)
                FilterDiv = 0xFD;
            else
            {
                FilterDiv = (BYTE)(199582 / waveFormat->nSamplesPerSec);
                FilterDiv = -FilterDiv;
            }
            Miniport->AdapterCommon->dspWriteMixer(ESM_MIXER_AUDIO2_CLKRATE, (BYTE)FilterDiv);
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveStreamSolo::SetState()
 *****************************************************************************
 * Sets the state of the channel.
 */
STDMETHODIMP
CMiniportWaveStreamSolo::
SetState
(
    IN      KSSTATE     NewState
)
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // The acquire state is not distinguishable from the pause state for our
    // purposes.
    //
    if (NewState == KSSTATE_ACQUIRE)
    {
        NewState = KSSTATE_PAUSE;
    }

    if (State != NewState)
    {
        switch (NewState)
        {
        case KSSTATE_PAUSE:
            if (State == KSSTATE_RUN)
            {
                Miniport->AdapterCommon->PauseDma(Capture);
                Miniport->Running--;
                if (!Miniport->Running)
                    Miniport->AdapterCommon->SetClkRunEnable(TRUE);
                
            }
            break;

        case KSSTATE_RUN:
            if (!Active) Active=TRUE;
            
            if (Capture)
            {
                Miniport->AdapterCommon->StartRecording(TRUE);
                PosStart = 1;
                DmaAddress = DmaChannel->SystemAddress();
            }

            //
            // Start DMA.
            //
            DmaBufferSize = DmaChannel->BufferSize();
            Miniport->AdapterCommon->StartDma(Capture, DmaChannel->PhysicalAddress().LowPart, 
                DmaBufferSize, Format16Bit, FormatStereo, 
                Miniport->NotificationInterval, Miniport->SamplingFrequency);
            Miniport->Running++;
            if (Miniport->Running) 
                Miniport->AdapterCommon->SetClkRunEnable(FALSE);
            break;

        case KSSTATE_STOP:
            if (Active == TRUE)
            {
                Miniport->AdapterCommon->StopDma(Capture);
                Active = FALSE;
            }
            break;
        }

        State = NewState;
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveStreamSolo::SetContentId
 *****************************************************************************
 * This routine gets called by drmk.sys to pass the content to the driver.
 * The driver has to enforce the rights passed.
 */
STDMETHODIMP_(NTSTATUS) 
CMiniportWaveStreamSolo::SetContentId
(
    IN  ULONG       contentId,
    IN  PCDRMRIGHTS drmRights
)
{
    UNREFERENCED_PARAMETER(contentId);

    PAGED_CODE ();

    Miniport->AdapterCommon->DRM_SetFlags(drmRights->CopyProtect, 
        drmRights->DigitalOutputDisable);
    
    return STATUS_SUCCESS;
}

#pragma code_seg()

/*****************************************************************************
 * CMiniportWaveStreamSolo::GetPosition()
 *****************************************************************************
 * Gets the current position.  May be called at dispatch level.
 */
STDMETHODIMP
CMiniportWaveStreamSolo::
GetPosition
(
    OUT     PULONG  Position
)
{
    ULONG Pos, i;
    PBYTE pDmaAddress;
    
    ASSERT(Position);

    if (State == KSSTATE_RUN)
    {
        Pos = DmaBufferSize - Miniport->AdapterCommon->GetPosition(Capture, DmaBufferSize) - 1;
        if (Capture)
        {
            pDmaAddress = (PBYTE)DmaAddress;
            i = PosStart;
            while (i != Pos)
            {
                if (i) pDmaAddress[i - 1] = pDmaAddress[i];
                else pDmaAddress[DmaBufferSize - 1] = *pDmaAddress;
                if (i != DmaBufferSize - 1) i++;
                else i=0;
            }
            PosStart = Pos;
            *Position = Pos?Pos-1:DmaBufferSize - 1;
        }
        else
        {
            *Position = Pos;
        }
    }
    else
    {
        *Position = 0;
    }

   return STATUS_SUCCESS;
}

STDMETHODIMP
CMiniportWaveStreamSolo::NormalizePhysicalPosition(
    IN OUT PLONGLONG PhysicalPosition
)

/*++

Routine Description:
    Given a physical position based on the actual number of bytes transferred,
    this function converts the position to a time-based value of 100ns units.

Arguments:
    IN OUT PLONGLONG PhysicalPosition -
        value to convert.

Return:
    STATUS_SUCCESS or an appropriate error code.

--*/

{                           
    *PhysicalPosition =
            (_100NS_UNITS_PER_SECOND / 
                (1 << (FormatStereo + Format16Bit)) * *PhysicalPosition) / 
                    Miniport->SamplingFrequency;
    return STATUS_SUCCESS;
}
    
/*****************************************************************************
 * CMiniportWaveStreamSolo::Silence()
 *****************************************************************************
 * Fills a buffer with silence.
 */
STDMETHODIMP_(void)
CMiniportWaveStreamSolo::
Silence
(
    IN      PVOID   Buffer,
    IN      ULONG   ByteCount
)
{
    RtlFillMemory(Buffer,ByteCount,Format16Bit ? 0 : 0x80);
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * CMiniportWaveStreamSolo::AllocateBuffer()
 *****************************************************************************
 * Allocate a buffer for this DMA channel.
 */
STDMETHODIMP
CMiniportWaveStreamSolo::AllocateBuffer
(   
    IN      ULONG               BufferSize,
    IN      PPHYSICAL_ADDRESS   PhysicalAddressConstraint   OPTIONAL
)
{
    PAGED_CODE();

    return DmaChannel->AllocateBuffer(BufferSize,PhysicalAddressConstraint);
}

/*****************************************************************************
 * CMiniportWaveStreamSolo::FreeBuffer()
 *****************************************************************************
 * Free the buffer for this DMA channel.
 */
STDMETHODIMP_(void)
CMiniportWaveStreamSolo::FreeBuffer(void)
{
    PAGED_CODE();

    DmaChannel->FreeBuffer();
}


#pragma code_seg()

/*****************************************************************************
 * CMiniportWaveStreamSolo::TransferCount()
 *****************************************************************************
 * Return the amount of data to be transfered via DMA.
 */
STDMETHODIMP_(ULONG) 
CMiniportWaveStreamSolo::TransferCount(void)
{
    return DmaChannel->TransferCount();
}


/*****************************************************************************
 * CMiniportWaveStreamSolo::MaximumBufferSize()
 *****************************************************************************
 * Return the maximum size that can be allocated to this DMA buffer.
 */
STDMETHODIMP_(ULONG) 
CMiniportWaveStreamSolo::MaximumBufferSize(void)
{
    return DmaChannel->MaximumBufferSize();
}


/*****************************************************************************
 * CMiniportWaveStreamSolo::AllocatedBufferSize()
 *****************************************************************************
 * Return the original size allocated to this DMA buffer -- the maximum value
 * that can be sent to SetBufferSize().
 */
STDMETHODIMP_(ULONG) 
CMiniportWaveStreamSolo::AllocatedBufferSize(void)
{
    return DmaChannel->AllocatedBufferSize();
}


/*****************************************************************************
 * CMiniportWaveStreamSolo::BufferSize()
 *****************************************************************************
 * Return the current size of the DMA buffer.
 */
STDMETHODIMP_(ULONG) 
CMiniportWaveStreamSolo::BufferSize(void)
{  
    return DmaChannel->BufferSize();
}


/*****************************************************************************
 * CMiniportWaveStreamSolo::SetBufferSize()
 *****************************************************************************
 * Change the size of the DMA buffer.  This cannot exceed the initial 
 * buffer size returned by AllocatedBufferSize().
 */
STDMETHODIMP_(void) 
CMiniportWaveStreamSolo::SetBufferSize(IN ULONG BufferSize)
{
    DmaChannel->SetBufferSize(BufferSize);
}

/*****************************************************************************
 * CMiniportWaveStreamSolo::SystemAddress()
 *****************************************************************************
 * Return the virtual address of this DMA buffer.
 */
STDMETHODIMP_(PVOID) 
CMiniportWaveStreamSolo::SystemAddress(void)
{
    return DmaChannel->SystemAddress();
}


/*****************************************************************************
 * CMiniportWaveStreamSolo::PhysicalAddress()
 *****************************************************************************
 * Return the actual physical address of this DMA buffer.
 */
STDMETHODIMP_(PHYSICAL_ADDRESS) 
CMiniportWaveStreamSolo::PhysicalAddress(void)
{
   return DmaChannel->PhysicalAddress();
}


/*****************************************************************************
 * CMiniportWaveStreamSolo::GetAdapterObject()
 *****************************************************************************
 * Return the DMA adapter object (defined in wdm.h).
 */
STDMETHODIMP_(PADAPTER_OBJECT) 
CMiniportWaveStreamSolo::GetAdapterObject(void)
{
   return DmaChannel->GetAdapterObject();
}


/*****************************************************************************
 * CMiniportWaveStreamSolo::CopyTo()
 *****************************************************************************
 * Copy data into the DMA buffer.  If you need to modify data on render
 * (playback), modify this routine to taste.
 */
STDMETHODIMP_(void)
CMiniportWaveStreamSolo::CopyTo
(   
    IN      PVOID   Destination,
    IN      PVOID   Source,
    IN      ULONG   ByteCount
)
{
   DmaChannel->CopyTo(Destination, Source, ByteCount);
}


/*****************************************************************************
 * CMiniportWaveStreamSolo::CopyFrom()
 *****************************************************************************
 * Copy data out of the DMA buffer.  If you need to modify data on capture 
 * (recording), modify this routine to taste.
 */
STDMETHODIMP_(void)
CMiniportWaveStreamSolo::CopyFrom
(
    IN      PVOID   Destination,
    IN      PVOID   Source,
    IN      ULONG   ByteCount
)
{
   DmaChannel->CopyFrom(Destination, Source, ByteCount);
}

