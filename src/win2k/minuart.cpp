/*****************************************************************************
 * minuart.cpp - ESS UART miniport implementation
 *****************************************************************************
 * Copyright (c) 2023 leecher@dose.0wnz.at  All rights reserved.
 */

#include "minuart.h"

#define STR_MODULENAME "UartMini: "

#define kMaxNumCaptureStreams       1
#define kMaxNumRenderStreams        1


#define UartFifoOkForWrite(status)  ((status & MPU401_DRR) == 0)
#define UartFifoOkForRead(status)   ((status & MPU401_DSR) == 0)

typedef struct
{
    CMiniportMidiUart  *Miniport;
    PUCHAR              PortBase;
    PVOID               BufferAddress;
    ULONG               Length;
    PULONG              BytesRead;
}
SYNCWRITECONTEXT, *PSYNCWRITECONTEXT;

typedef struct
{
    PVOID               BufferAddress;
    ULONG               Length;
    PULONG              BytesRead;
    PULONG              pMPUInputBufferHead;
    ULONG               MPUInputBufferTail;
    PUCHAR              MPUInputBuffer;
}
DEFERREDREADCONTEXT, *PDEFERREDREADCONTEXT;

NTSTATUS DeferredLegacyRead(IN PINTERRUPTSYNC InterruptSync,IN PVOID DynamicContext);
BOOLEAN  TryLegacyMPU(IN PUCHAR PortBase);
NTSTATUS WriteLegacyMPU(IN PUCHAR PortBase,IN BOOLEAN IsCommand,IN UCHAR Value);


#pragma code_seg("PAGE")

/*****************************************************************************
 * CreateMiniportMidiUartESS()
 *****************************************************************************
 * Creates a MIDI UART miniport object for the ESS  adapter.  This uses a
 * macro from STDUNK.H to do all the work.
 */
NTSTATUS
CreateMiniportMidiUartESS
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY_(CMiniportMidiUart,Unknown,UnknownOuter,PoolType,PMINIPORTMIDI);
}

#pragma code_seg("PAGE")
//
// We initialize the UART with interrupts suppressed so we don't
// try to service the chip prematurely.
//
NTSTATUS CMiniportMidiUart::InitializeHardware(PINTERRUPTSYNC interruptSync,PUCHAR portBase)
{
    PAGED_CODE();

    NTSTATUS ntStatus;
    
    ntStatus = interruptSync->CallSynchronizedRoutine(InitLegacyMPU,PVOID(portBase));

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Start the UART (this should trigger an interrupt).
        //
        ntStatus = ResetMPUHardware(portBase);
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE,("*** InitMPU returned with ntStatus 0x%08x ***",ntStatus));
    }

    m_fMPUInitialized = NT_SUCCESS(ntStatus);

    return ntStatus;
}


#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportMidiUart::ProcessResources()
 *****************************************************************************
 * Processes the resource list, setting up helper objects accordingly.
 */
NTSTATUS
CMiniportMidiUart::
ProcessResources
(
    IN      PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_BLAB,("ProcessResources"));
    ASSERT(ResourceList);
    if (!ResourceList)
    {
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    //
    // Get counts for the types of resources.
    //
    ULONG   countIO     = ResourceList->NumberOfPorts();
    ULONG   countIRQ    = ResourceList->NumberOfInterrupts();
    ULONG   countDMA    = ResourceList->NumberOfDmas();

#if (DBG)
    _DbgPrintF(DEBUGLVL_VERBOSE,("Starting MPU401 Port 0x%X",
        ResourceList->FindTranslatedPort(0)->u.Port.Start.LowPart) );
#endif

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Make sure we have the expected number of resources.
    //
    if  (   (countIO != 1)
        ||  (countIRQ  != 1)
        ||  (countDMA != 0)
        )
    {
        _DbgPrintF(DEBUGLVL_TERSE,("Unknown configuraton"));
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Get the port address.
        //
        m_pPortBase =
            PUCHAR(ResourceList->FindTranslatedPort(0)->u.Port.Start.QuadPart);

        ntStatus = InitializeHardware(m_pInterruptSync,m_pPortBase);
    }

    return ntStatus;
}


/*****************************************************************************
 * CMiniportMidiUart::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP
CMiniportMidiUart::
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
        *Object = PVOID(PUNKNOWN(PMINIPORTMIDI(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportMidi))
    {
        *Object = PVOID(PMINIPORTMIDI(this));
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
 * CMiniportMidiUart::~CMiniportMidiUart()
 *****************************************************************************
 * Destructor.
 */
CMiniportMidiUart::
~CMiniportMidiUart
(   void
)
{
    PAGED_CODE();

    if (m_pServiceGroup)
    {
        m_pServiceGroup->Release();
        m_pServiceGroup = NULL;
    }
    if (m_pPort)
    {
        m_pPort->Release();
        m_pPort = NULL;
    }
    if (m_pAdapterCommon)
    {
        m_pAdapterCommon->Release();
        m_pAdapterCommon = NULL;
    }
    bMPU401IntEnable = FALSE;
}


/*****************************************************************************
 * CMiniportMidiUart::Init()
 *****************************************************************************
 * Initializes a the miniport.
 */
STDMETHODIMP
CMiniportMidiUart::
Init
(
    IN      PUNKNOWN        UnknownAdapter,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTMIDI       Port_,
    OUT     PSERVICEGROUP * ServiceGroup
)
{
    NTSTATUS ntStatus;
    
    PAGED_CODE();

    ASSERT(ResourceList);
    ASSERT(Port_);
    ASSERT(ServiceGroup);

    //
    // AddRef() is required because we are keeping this pointer.
    //
    m_pPort = Port_;
    m_pPort->AddRef();
    
    //
    // Initialize the member variables.
    //
    m_pPortBase = 0;
    m_fMPUInitialized = FALSE;
    m_PowerState  = PowerSystemUnspecified;
    
    RtlZeroMemory(m_MPUInputBuffer, sizeof(m_MPUInputBuffer));
    m_MPUInputBufferHead = 0;
    m_MPUInputBufferTail = 0;
    m_KSStateInput = KSSTATE_STOP;
    
    m_NumRenderStreams = 0;
    m_NumCaptureStreams = 0;
    _DbgPrintF(DEBUGLVL_VERBOSE,("Init: resetting m_NumRenderStreams and m_NumCaptureStreams"));

    if (UnknownAdapter)
    {
        //
        // We want the IAdapterCommon interface on the adapter common object,
        // which is given to us as a IUnknown.  The QueryInterface call gives us
        // an AddRefed pointer to the interface we want.
        //
        ntStatus =
            UnknownAdapter->QueryInterface
            (
                IID_IAdapterCommon,
                (PVOID *) &m_pAdapterCommon
            );
            
        if (NT_SUCCESS(ntStatus))
        {
            m_pInterruptSync  = m_pAdapterCommon->GetInterruptSync();
            if (m_pInterruptSync)
            {
                m_pInterruptSync->AddRef();
                ntStatus = m_pInterruptSync->
                    RegisterServiceRoutine(MPUInterruptServiceRoutine,PVOID(this),TRUE);
            }
        }
    }
    else
    {   // create our own interruptsync mechanism.
        ntStatus = 
            PcNewInterruptSync
            (
                &m_pInterruptSync,
                NULL,
                ResourceList,
                0,                          // Resource Index
                InterruptSyncModeNormal     // Run ISRs once until we get SUCCESS
            );
        if (NT_SUCCESS(ntStatus) && m_pInterruptSync)
        {                                                                       //  run this ISR first
            ntStatus = m_pInterruptSync->RegisterServiceRoutine(MPUInterruptServiceRoutine,PVOID(this),TRUE);
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
    }

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
        ntStatus = PcNewServiceGroup(&m_pServiceGroup,NULL);
        if (NT_SUCCESS(ntStatus))
        {
            *ServiceGroup = m_pServiceGroup;
            m_pServiceGroup->AddRef();
            m_pPort->RegisterServiceGroup(m_pServiceGroup);
            ntStatus = ProcessResources(ResourceList);
            bMPU401IntEnable = TRUE;
            if (NT_SUCCESS(ntStatus))
                return STATUS_SUCCESS;
        }
    }

    //
    // In case of failure object gets destroyed and destructor cleans up.
    //

    if ( m_pServiceGroup )
    {
        m_pServiceGroup->Release();
        m_pServiceGroup = NULL;
    }

    if ( m_pInterruptSync )
    {
        m_pInterruptSync->Disconnect();
        m_pInterruptSync->Release();
        m_pInterruptSync = NULL;
    }
    
    if ( m_pAdapterCommon )
    {
        m_pAdapterCommon->Release();
        m_pAdapterCommon = NULL;       
    }
    
    m_pPort->Release();
    m_pPort = NULL;

    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * PinDataRangesStream
 *****************************************************************************
 * Structures indicating range of valid format values for streaming pins.
 */
static
KSDATARANGE_MUSIC PinDataRangesStream[] =
{
    {
        {
            sizeof(KSDATARANGE_MUSIC),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
        },
        STATICGUIDOF(KSMUSIC_TECHNOLOGY_PORT),
        0,
        0,
        0xFFFF
    }
};

/*****************************************************************************
 * PinDataRangePointersStream
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for live pins.
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
KSDATARANGE PinDataRangesBridge[] =
{
   {
      sizeof(KSDATARANGE),
      0,
      0,
      0,
      STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC),
      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI_BUS),
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
PKSDATARANGE PinDataRangePointersBridge[] =
{
    &PinDataRangesBridge[0]
};

#define kMaxNumCaptureStreams       1
#define kMaxNumRenderStreams        1

/*****************************************************************************
 * MiniportPins
 *****************************************************************************
 * List of pins.
 */
static
PCPIN_DESCRIPTOR MiniportPins[] =
{
    {
        kMaxNumRenderStreams,kMaxNumRenderStreams,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersStream),   // DataRangesCount
            PinDataRangePointersStream,                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            &KSAUDFNAME_MIDI,                           // Name
            0                                           // Reserved
        }
    },
    {
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },
    {
        kMaxNumCaptureStreams,kMaxNumCaptureStreams,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersStream),   // DataRangesCount
            PinDataRangePointersStream,                 // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            &KSAUDFNAME_MIDI,                           // Name
            0                                           // Reserved
        }
    },
    {
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
};

/*****************************************************************************
 * MiniportConnections
 *****************************************************************************
 * List of connections.
 */
static
PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{
    { PCFILTER_NODE,  0,  PCFILTER_NODE,    1 },
    { PCFILTER_NODE,  3,  PCFILTER_NODE,    2 }    
};

/*****************************************************************************
 * MiniportFilterDescriptor
 *****************************************************************************
 * Complete miniport filter description.
 */
static
PCFILTER_DESCRIPTOR MiniportFilterDescriptor =
{
    0,                                  // Version
    NULL,                               // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    SIZEOF_ARRAY(MiniportPins),         // PinCount
    MiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    0,                                  // NodeCount
    NULL,                               // Nodes
    SIZEOF_ARRAY(MiniportConnections),  // ConnectionCount
    MiniportConnections,                // Connections
    0,                                  // CategoryCount
    NULL                                // Categories
};

/*****************************************************************************
 * CMiniportMidiUart::GetDescription()
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP
CMiniportMidiUart::
GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);
    
    _DbgPrintF(DEBUGLVL_VERBOSE,("GetDescription"));

    *OutFilterDescriptor = &MiniportFilterDescriptor;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportMidiUart::NewStream()
 *****************************************************************************
 * Creates a new stream.  This function is called when a streaming pin is
 * created.
 */
STDMETHODIMP
CMiniportMidiUart::
NewStream
(
    OUT     PMINIPORTMIDISTREAM *       OutStream,
    IN      PUNKNOWN                    OuterUnknown,
    IN      POOL_TYPE                   PoolType,
    IN      ULONG                       PinID,
    IN      BOOLEAN                     Capture,
    IN      PKSDATAFORMAT               DataFormat,
    OUT     PSERVICEGROUP *             OutServiceGroup
)
{
    UNREFERENCED_PARAMETER(DataFormat);
    UNREFERENCED_PARAMETER(PinID);

    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("NewStream"));
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // if we don't have any streams already open, get the hardware ready.
    if ((!m_NumCaptureStreams) && (!m_NumRenderStreams))
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("NewStream: m_NumRenderStreams and m_NumCaptureStreams are both 0, so ResetMPUHardware"));

        ntStatus = ResetMPUHardware(m_pPortBase);
        if (!NT_SUCCESS(ntStatus))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("CMiniportMidiUart::NewStream ResetHardware failed"));
            return ntStatus;
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("NewStream: m_NumRenderStreams %d, m_NumCaptureStreams %d, no ResetMPUHardware",
                                     m_NumRenderStreams,m_NumCaptureStreams));
    }

    if  (   (   (m_NumCaptureStreams < kMaxNumCaptureStreams)
            &&  (Capture)  )
        ||  (   (m_NumRenderStreams < kMaxNumRenderStreams) 
            &&  (!Capture) )
        )
    {
        CMiniportMidiStreamUart *pStream =
            new(PoolType) CMiniportMidiStreamUart(OuterUnknown);

        if (pStream)
        {
            pStream->AddRef();

            ntStatus = 
                pStream->Init(this,m_pPortBase,Capture);

            if (NT_SUCCESS(ntStatus))
            {
                *OutStream = PMINIPORTMIDISTREAM(pStream);
                (*OutStream)->AddRef();

                if (Capture)
                {
                    m_NumCaptureStreams=1;
                    *OutServiceGroup = m_pServiceGroup;
                    (*OutServiceGroup)->AddRef();
                }
                else
                {
                    m_NumRenderStreams=1;
                    *OutServiceGroup = NULL;
                }
                _DbgPrintF(DEBUGLVL_VERBOSE,("NewStream: succeeded, m_NumRenderStreams %d, m_NumCaptureStreams %d",
                                              m_NumRenderStreams,m_NumCaptureStreams));
            }

            pStream->Release();
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        if (Capture)
        {
            _DbgPrintF(DEBUGLVL_TERSE,("NewStream failed, too many capture streams"));
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("NewStream failed, too many render streams"));
        }
    }

    if (NT_SUCCESS(ntStatus) && !Capture)
        m_pAdapterCommon->SetDacToMidi(TRUE);

    return ntStatus;
}

/*****************************************************************************
 * CMiniportMidiUart::PowerChangeNotify()
 *****************************************************************************
 * Handle power state change for the miniport.
 */
STDMETHODIMP_(void)
CMiniportMidiUart::
PowerChangeNotify
(
    IN      POWER_STATE             PowerState
)
{
    NTSTATUS ntStatus;
    
    PAGED_CODE();
    
    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportMidiUart::PoweChangeNotify D%d", PowerState.SystemState));
       
    if ( PowerState.SystemState == PowerSystemWorking && m_PowerState != PowerSystemWorking )
    {
        ntStatus = m_pInterruptSync->CallSynchronizedRoutine(InitLegacyMPU, m_pPortBase);
        
        if (NT_SUCCESS(ntStatus))
        {
            //
            // Start the UART (this should trigger an interrupt).
            //
            ResetMPUHardware(m_pPortBase);
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("*** InitLegacyMPU returned with ntStatus 0x%08x ***",ntStatus));
        }
        
        m_fMPUInitialized = NT_SUCCESS(ntStatus);
    }
        
    m_PowerState = PowerState.SystemState;
} // PowerChangeNotify


#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportMidiStreamUart::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamUart::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("Stream::NonDelegatingQueryInterface"));
    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportMidiStream))
    {
        *Object = PVOID(PMINIPORTMIDISTREAM(this));
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

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportMidiStreamUart::SetFormat()
 *****************************************************************************
 * Sets the format.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamUart::
SetFormat
(
    IN      PKSDATAFORMAT   Format
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Format);
    
    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiStreamUart::SetFormat"));

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportMidiStreamUart::~CMiniportMidiStreamUart()
 *****************************************************************************
 * Destructs a stream.
 */
CMiniportMidiStreamUart::~CMiniportMidiStreamUart(void)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("~CMiniportMidiStreamUart"));

    if (m_pMiniport)
    {
        if (m_fCapture)
        {
            m_pMiniport->m_NumCaptureStreams = 0;
        }
        else
        {
            m_pMiniport->m_NumRenderStreams = 0;
            m_pMiniport->m_pAdapterCommon->SetDacToMidi(FALSE);
        }

        m_pMiniport->Release();
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportMidiStreamUart::Init()
 *****************************************************************************
 * Initializes a stream.
 */
STDMETHODIMP_(NTSTATUS) 
CMiniportMidiStreamUart::
Init
(
    IN      CMiniportMidiUart * pMiniport,
    IN      PUCHAR              pPortBase,
    IN      BOOLEAN             fCapture
)
{
    PAGED_CODE();

    ASSERT(pMiniport);
    ASSERT(pPortBase);

    m_pMiniport = pMiniport;
    m_pMiniport->AddRef();

    m_pPortBase = pPortBase;
    m_fCapture = fCapture;
    
    m_NumFailedMPUTries = 0;
    // m_gap1 = 4

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportMidiStreamUart::SetState()
 *****************************************************************************
 * Sets the state of the channel.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamUart::
SetState
(
    IN      KSSTATE     NewState
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("SetState %d",NewState));

    if (NewState == KSSTATE_RUN)
    {
        if (!m_pMiniport->m_fMPUInitialized)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("CMiniportMidiStreamUart::SetState KSSTATE_RUN failed due to uninitialized MPU"));
            return STATUS_INVALID_DEVICE_STATE;
        }
    }

    m_pMiniport->m_KSStateInput = NewState;
    
    if (NewState == KSSTATE_STOP)   // STOPping
    {
        m_pMiniport->m_MPUInputBufferHead = 0;   // Previously read bytes are discarded.
        m_pMiniport->m_MPUInputBufferTail = 0;   // The entire FIFO is available.
    }

    return STATUS_SUCCESS;
}



#pragma code_seg()
/*****************************************************************************
 * SynchronizedMPUWrite()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
NTSTATUS
SynchronizedMPUWrite
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           syncWriteContext
)
{
    PSYNCWRITECONTEXT context;
    context = (PSYNCWRITECONTEXT)syncWriteContext;
    ASSERT(context->Miniport);
    ASSERT(context->PortBase);
    ASSERT(context->BufferAddress);
    ASSERT(context->Length);
    ASSERT(context->BytesRead);

    PUCHAR  pChar = PUCHAR(context->BufferAddress);
    NTSTATUS ntStatus,readStatus;
    ntStatus = STATUS_SUCCESS;
    //
    // while we're not there yet, and
    // while we don't have to wait on an aligned byte (including 0)
    // (we never wait on an aligned byte.  Better to come back later)
//    if (context->Miniport->m_NumCaptureStreams)
    {
        readStatus = MPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
    }
    while (  (*(context->BytesRead) < context->Length)
          && (TryLegacyMPU(context->PortBase) 
             || (*(context->BytesRead)%4)
          )  )
    {
        ntStatus = WriteLegacyMPU(context->PortBase,DATA,*pChar);
        if (NT_SUCCESS(ntStatus))
        {
            pChar++;
            *(context->BytesRead) = *(context->BytesRead) + 1;
            readStatus = MPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("SynchronizedMPUWrite failed (0x%08x)",ntStatus));
            break;
        }
    }
//    if (context->Miniport->m_NumCaptureStreams)
    {
            readStatus = MPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
    }
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportMidiStreamUart::Write()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamUart::
Write
(
    IN      PVOID       BufferAddress,
    IN      ULONG       Length,
    OUT     PULONG      BytesWritten
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("Write"));
    ASSERT(BytesWritten);
    if (!BufferAddress)
    {
        Length = 0;
    }

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!m_fCapture)
    {
        PUCHAR  pMidiData;
        ULONG   count;

        count = 0;
        pMidiData = PUCHAR(BufferAddress);

        if (Length)
        {
            SYNCWRITECONTEXT context;
            context.Miniport        = (m_pMiniport);
            context.PortBase        = m_pPortBase;
            context.BufferAddress   = pMidiData;
            context.Length          = Length;
            context.BytesRead       = &count;

            ntStatus = m_pMiniport->m_pInterruptSync->
                            CallSynchronizedRoutine(SynchronizedMPUWrite,PVOID(&context));

        }           //  if we have data at all
        *BytesWritten = count;
    }
    else    //  called write on the read stream
    {
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    return ntStatus;
}


#define kMPUPollTimeout 1

#pragma code_seg()
/*****************************************************************************
 * TryLegacyMPU()
 *****************************************************************************
 * See if the MPU401 is free.
 */
BOOLEAN
TryLegacyMPU
(
    IN      PUCHAR      PortBase
)
{
    BOOLEAN success;
    USHORT  numPolls;
    UCHAR   status;

    _DbgPrintF(DEBUGLVL_BLAB, ("TryLegacyMPU"));
    numPolls = 0;

    while (numPolls < kMPUPollTimeout)
    {
        status = READ_PORT_UCHAR(PortBase + MPU401_REG_STATUS);
                                       
        if (UartFifoOkForWrite(status)) // Is this a good time to write data?
        {
            break;
        }
        numPolls++;
    }
    if (numPolls >= kMPUPollTimeout)
    {
        success = FALSE;
        _DbgPrintF(DEBUGLVL_BLAB, ("TryLegacyMPU failed"));
    }
    else
    {
        success = TRUE;
    }

    return success;
}

#pragma code_seg()
/*****************************************************************************
 * WriteLegacyMPU()
 *****************************************************************************
 * Write a byte out to the MPU401.
 */
NTSTATUS
WriteLegacyMPU
(
    IN      PUCHAR      PortBase,
    IN      BOOLEAN     IsCommand,
    IN      UCHAR       Value
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("WriteLegacyMPU"));
    NTSTATUS ntStatus = STATUS_IO_DEVICE_ERROR;

    if (!PortBase)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("O: PortBase is zero\n"));
        return ntStatus;
    }
    PUCHAR deviceAddr = PortBase + MPU401_REG_DATA;

    if (IsCommand)
    {
        deviceAddr = PortBase + MPU401_REG_COMMAND;
    }

    ULONGLONG startTime = PcGetTimeInterval(0);
    
    while (PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50))
    {
        UCHAR status
        = READ_PORT_UCHAR(PortBase + MPU401_REG_STATUS);

        if (UartFifoOkForWrite(status)) // Is this a good time to write data?
        {                               // yep (Jon comment)
            WRITE_PORT_UCHAR(deviceAddr,Value);
            _DbgPrintF(DEBUGLVL_BLAB, ("WriteLegacyMPU emitted 0x%02x",Value));
            ntStatus = STATUS_SUCCESS;
            break;
        }
    }
    return ntStatus;
}

#pragma code_seg("PAGE")
//  make sure we're in UART mode
NTSTATUS ResetMPUHardware(PUCHAR portBase)
{
    PAGED_CODE();

    return (WriteLegacyMPU(portBase,COMMAND,MPU401_CMD_UART));
}

#pragma code_seg()
/*****************************************************************************
 * InitLegacyMPU()
 *****************************************************************************
 * Synchronized routine to initialize the MPU401.
 */
NTSTATUS
InitLegacyMPU
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    PUCHAR      portBase = PUCHAR(DynamicContext);
    UCHAR       status;
    ULONGLONG   startTime;
    BOOLEAN     success;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(InterruptSync);

    //
    // Reset the card (puts it into "smart mode")
    //
    ntStatus = WriteLegacyMPU(portBase,COMMAND,MPU401_CMD_RESET);

    // wait for the acknowledgement
    // NOTE: When the Ack arrives, it will trigger an interrupt.  
    //       Normally the DPC routine would read in the ack byte and we
    //       would never see it, however since we have the hardware locked (HwEnter),
    //       we can read the port before the DPC can and thus we receive the Ack.
    startTime = PcGetTimeInterval(0);
    success = FALSE;
    while(PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50))
    {
        status = READ_PORT_UCHAR(portBase + MPU401_REG_STATUS);
        
        if (UartFifoOkForRead(status))                      // Is data waiting?
        {
            READ_PORT_UCHAR(portBase + MPU401_REG_DATA);    // yep.. read ACK 
            success = TRUE;                                 // don't need to do more 
            break;
        }
        KeStallExecutionProcessor(25);  //  microseconds
    }
#if (DBG)
    if (!success)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("First attempt to reset the MPU didn't get ACKed.\n"));
    }
#endif  //  (DBG)

    // NOTE: We cannot check the ACK byte because if the card was already in
    // UART mode it will not send an ACK but it will reset.

    // reset the card again
    (void) WriteLegacyMPU(portBase,COMMAND,MPU401_CMD_RESET);

                                    // wait for ack (again)
    startTime = PcGetTimeInterval(0); // This might take a while
    BYTE dataByte = 0;
    success = FALSE;
    while (PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50))
    {
        status = READ_PORT_UCHAR(portBase + MPU401_REG_STATUS);
        if (UartFifoOkForRead(status))                                  // Is data waiting?
        {
            dataByte = READ_PORT_UCHAR(portBase + MPU401_REG_DATA);     // yep.. read ACK
            success = TRUE;                                             // don't need to do more
            break;
        }
        KeStallExecutionProcessor(25);
    }

    if ((0xFE != dataByte) || !success)   // Did we succeed? If no second ACK, something is hosed  
    {                       
        _DbgPrintF(DEBUGLVL_TERSE,("Second attempt to reset the MPU didn't get ACKed.\n"));
        _DbgPrintF(DEBUGLVL_TERSE,("Init Reset failure error. Ack = %X", ULONG(dataByte) ) );
        ntStatus = STATUS_IO_DEVICE_ERROR;
    }
    
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportMidiUart::Service()
 *****************************************************************************
 * DPC-mode service call from the port.
 */
STDMETHODIMP_(void) 
CMiniportMidiUart::
Service
(   void
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("Service"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("Service: m_NumRenderStreams %d, m_NumCaptureStreams %d",
                                  m_NumRenderStreams,m_NumCaptureStreams));
    if (!m_NumCaptureStreams)
    {
        //  we should never get here....
        //  if we do, we must have read some trash,
        //  so just reset the input FIFO
        m_MPUInputBufferTail = m_MPUInputBufferHead = 0;
    }
}


#pragma code_seg()
/*****************************************************************************
 * CMiniportMidiStreamUart::Read()
 *****************************************************************************
 * Reads incoming MIDI data.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamUart::
Read
(
    IN      PVOID   BufferAddress,
    IN      ULONG   Length,
    OUT     PULONG  BytesRead
)
{
    ASSERT(BufferAddress);
    ASSERT(BytesRead);

    if (m_fCapture)
    {
        DEFERREDREADCONTEXT context;
        context.BufferAddress   = BufferAddress;
        context.Length          = Length;
        context.BytesRead       = BytesRead;
        context.pMPUInputBufferHead = &(m_pMiniport->m_MPUInputBufferHead);
        context.MPUInputBufferTail = m_pMiniport->m_MPUInputBufferTail;
        context.MPUInputBuffer     = m_pMiniport->m_MPUInputBuffer;

        return (DeferredLegacyRead(m_pMiniport->m_pInterruptSync,PVOID(&context)));
    }
    else
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
}

/*****************************************************************************
 * DeferredLegacyRead()
 *****************************************************************************
 * Synchronized routine to read incoming MIDI data.
 * We have already read the bytes in, and now the Port wants them.
 */
NTSTATUS
DeferredLegacyRead
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    UNREFERENCED_PARAMETER(InterruptSync);
    ASSERT(DynamicContext);

    PDEFERREDREADCONTEXT context = PDEFERREDREADCONTEXT(DynamicContext);

    ASSERT(context->BufferAddress);
    ASSERT(context->BytesRead);


    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUCHAR  pDest = PUCHAR(context->BufferAddress);
    PULONG  pMPUInputBufferHead = context->pMPUInputBufferHead;
    ULONG   MPUInputBufferTail = context->MPUInputBufferTail;
    ULONG   bytesRead = 0;

    ASSERT(pMPUInputBufferHead);
    ASSERT(context->MPUInputBuffer);

    while  (    (*pMPUInputBufferHead != MPUInputBufferTail)
            &&  (bytesRead < context->Length) )
    {
        *pDest = context->MPUInputBuffer[*pMPUInputBufferHead];

        pDest++;
        bytesRead++;
        *pMPUInputBufferHead = *pMPUInputBufferHead + 1;
        //
        //  Wrap FIFO position when reaching the buffer size.
        //
        if (*pMPUInputBufferHead >= kMPUInputBufferSize)
        {
            *pMPUInputBufferHead = 0;
        }
    }
    *context->BytesRead = bytesRead;

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * MPUInterruptServiceRoutine()
 *****************************************************************************
 * ISR.
 */
NTSTATUS
MPUInterruptServiceRoutine
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("MPUInterruptServiceRoutine"));

    UNREFERENCED_PARAMETER(InterruptSync);
    ASSERT(DynamicContext);

    NTSTATUS            ntStatus;
    CMiniportMidiUart   *that;

    that = (CMiniportMidiUart *) DynamicContext;
    ntStatus = STATUS_UNSUCCESSFUL;

    UCHAR portStatus;

    //
    // Read the MPU status byte.
    //
    if (that->m_pPortBase)
    {
        portStatus =
            READ_PORT_UCHAR(that->m_pPortBase + MPU401_REG_STATUS);

        //
        // If there is outstanding work to do and there is a port-driver for
        // the MPU miniport...
        //
        if (UartFifoOkForRead(portStatus) && that->m_pPort)
        {
            while ( (UartFifoOkForRead(portStatus)) )
            {
                UCHAR uDest = READ_PORT_UCHAR(that->m_pPortBase + MPU401_REG_DATA);
                if (that->m_KSStateInput == KSSTATE_RUN)
                {
                    //  ...place the data in our FIFO...
                    that->m_MPUInputBuffer[that->m_MPUInputBufferTail] = uDest;
                    
                    that->m_MPUInputBufferTail++;
                    if (that->m_MPUInputBufferTail >= kMPUInputBufferSize)
                    {
                        that->m_MPUInputBufferTail = 0;
                    }
                }
                //
                // Look for more MIDI data.
                //
                portStatus =
                    READ_PORT_UCHAR(that->m_pPortBase + MPU401_REG_STATUS);
            }   //  either there's no data or we ran too long
            if (that->m_KSStateInput == KSSTATE_RUN)
            {
                //
                // ...notify the MPU port driver that we have bytes.
                //
                that->m_pPort->Notify(that->m_pServiceGroup);
            }
            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}

