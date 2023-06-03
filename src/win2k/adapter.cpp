/*****************************************************************************
 * adapter.cpp - Reconstruction of ESS adapter driver implementation.
 *****************************************************************************
 * Copyright (c) 2023 leecher@dose.0wnz.at  All rights reserved.
 *
 * This files does setup and resource allocation/verification for the ESS
 * card. It controls which miniports are started and which resources are
 * given to each miniport. It also deals with interrupt sharing between
 * miniports by hooking the interrupt and calling the correct DPC.
 */

//
// All the GUIDS for all the miniports end up in this object.
//
#define PUT_GUIDS_HERE

#define STR_MODULENAME "es1969Adapter: "

#include "common.h"
#include "minwave.h"
#include "minfm.h"
#include "minuart.h"


/*****************************************************************************
 * Defines
 */
#define MAX_MINIPORTS 5

#if (DBG)
#define SUCCEEDS(s) ASSERT(NT_SUCCESS(s))
#else
#define SUCCEEDS(s) (s)
#endif

DEFINE_GUID(CLSID_MiniportDriverESFMSynth,
0x7EAEB839, 0xFF5F, 0x11D0, 0xA5, 0x10, 0x00, 0x60, 0x97, 0xC7, 0x9D, 0x21);

DEFINE_GUID(CLSID_MiniportDriverUartESS,
0xF6A7ED80, 0xE68E, 0x11D1, 0x9B, 0x4B, 0x00, 0xC0, 0x9F, 0x00, 0x3C, 0x24);


struct DEVICE_CONTEXT
{
  DEVICE_RELATIONS DeviceRelation;
  ULONG InterruptVector;
  DWORD IOPort;
  CMiniportWaveSolo *MiniportWaveCyclic;
  CMiniportMidiFM *MiniportMidiESFM;
  CMiniportMidiUart *MiniportMidiUartESS;
  struct _DEVICE_CONTEXT *ParentContext;
};
typedef struct DEVICE_CONTEXT *PDEVICE_CONTEXT;

typedef struct DEVICE_EXTENSION
{
  ULONG_PTR gap1[4];
  DWORD Cookie1;
  DWORD Cookie2;
  PDEVICE_OBJECT PhysicalDeviceObject;
  PVOID DeviceContext;
  DWORD DeviceNumber;
  ULONG_PTR gap2[2];
  DWORD JoystickPort;
  DWORD JoystickIRQ;
  ULONG_PTR gap3[11];
  PDEVICE_OBJECT DeviceObject;
  POWER_STATE SystemState;
  POWER_STATE DeviceState;
} *PDEVICE_EXTENSION;


/*****************************************************************************
 * Globals
 */
WORD gwPDO_Count = 0;
BOOL NoJoy = FALSE;
PDEVICE_CONTEXT ParentDeviceContext = NULL;
static GUID GUID_Solo1Bus = {0x7E274041, 0x0C960, 0x11D1, 0xA6, 0x2F, 0x00, 0xC0, 0x9F, 0x00, 0x2B, 0x8F};


/*****************************************************************************
 * Externals
 */
NTSTATUS
CreateMiniportWaveSolo
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);
NTSTATUS
CreateMiniportTopologyESS
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);





/*****************************************************************************
 * Referenced forward
 */
extern "C"
NTSTATUS
AddDevice
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
);

extern "C"
VOID
Unload
(
    IN PDRIVER_OBJECT   DriverObject
);

NTSTATUS
StartDevice
(
    IN  PDEVICE_OBJECT  DeviceObject,   // Device object.
    IN  PIRP            Irp,            // IO request packet.
    IN  PRESOURCELIST   ResourceList    // List of hardware resources.
);

NTSTATUS
AssignResources
(
    IN  PRESOURCELIST   ResourceList,           // All resources.
    OUT PRESOURCELIST * ResourceListTopology,   // Topology resources.
    OUT PRESOURCELIST * ResourceListWave,       // Wave  resources.
    OUT PRESOURCELIST * ResourceListFmSynth,    // FM synth resources.
    OUT PRESOURCELIST * ResourceListUart,       // UART resources.
    OUT PRESOURCELIST * ResourceListAdapter     // a copy needed by the adapter
);

extern "C"
NTSTATUS
AdapterGlobalIrpDispatch
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
);

extern "C"
NTSTATUS
AdapterDispatchPnp
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
);

extern "C"
NTSTATUS
ChildPdoPower
(
    IN      PDEVICE_OBJECT     pDeviceObject,
    IN      PIRP               pIrp,
    IN      PIO_STACK_LOCATION pIrpStack,
    IN      PDEVICE_EXTENSION  pDeviceExtension
);

extern "C"
NTSTATUS
ChildPdoPnp
(
    IN      PDEVICE_OBJECT     pDeviceObject,
    IN      PIRP               pIrp,
    IN      PIO_STACK_LOCATION pIrpStack,
    IN      PDEVICE_EXTENSION  pDeviceExtension
);

DWORD DeterminePlatform(PPORTTOPOLOGY Port);



#define ESSM_COOKIE         0x33E811D3
#define COOKIE_SOUNDCARD    0xA067AA5C
#define COOKIE_JOYSTICK     0xAE350760

#pragma code_seg("PAGE")


/*****************************************************************************
 * RemoveParentDeviceContext()
 *****************************************************************************
 * Free DeviceContext from DeviceObject 
 */
VOID RemoveParentDeviceContext(PDEVICE_OBJECT pDeviceObject)
{
    PDEVICE_EXTENSION pDeviceExtension;
    
    PAGED_CODE();
    
    pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
    if ( pDeviceExtension->DeviceContext )
    {
        ExFreePool(pDeviceExtension->DeviceContext);
        pDeviceExtension->DeviceContext = NULL;
    }
}

static BOOL ReturnDupString
(
    IN      PIRP               pIrp,
    IN      PWCHAR             String
)
{
    PVOID Text;
    size_t Length;
    
    Length = (wcslen(String) + 2) * sizeof(WCHAR);
    if ((Text = ExAllocatePool(PagedPool, Length)) == NULL)
        return FALSE;
    RtlZeroMemory(Text, Length);
    RtlCopyMemory(Text, String, Length - sizeof(WCHAR));
    pIrp->IoStatus.Information = (ULONG_PTR)Text;
    return TRUE;
}



/*****************************************************************************
 * AdapterDispatchPnp()
 *****************************************************************************
 * Supplying your PnP resource filtering needs.
 */
extern "C"
NTSTATUS
AdapterDispatchPnp
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    PDEVICE_CONTEXT pDeviceContext;
    PDEVICE_OBJECT pDeviceObjectNew;
    CM_PARTIAL_RESOURCE_LIST *pPartialResourceList;
    unsigned int i;

    PIO_STACK_LOCATION pIrpStack =
        IoGetCurrentIrpStackLocation(pIrp);

    switch( pIrpStack->MinorFunction )
    {
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
            pDeviceContext = (PDEVICE_CONTEXT)((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->DeviceContext;
            if ( pDeviceContext )
            {
                if ( pDeviceContext->MiniportWaveCyclic )
                {
                    pDeviceContext->MiniportWaveCyclic->Release();
                    pDeviceContext->MiniportWaveCyclic = NULL;
                }
                if ( pDeviceContext->MiniportMidiESFM )
                {
                    pDeviceContext->MiniportMidiESFM->Release();
                    pDeviceContext->MiniportMidiESFM = NULL;
                }
                if ( pDeviceContext->MiniportMidiUartESS )
                {
                    pDeviceContext->MiniportMidiUartESS->Release();
                    pDeviceContext->MiniportMidiUartESS = NULL;
                }
                
                if ( pIrpStack->MinorFunction == IRP_MN_REMOVE_DEVICE )
                {
                    for ( i = 0; i < gwPDO_Count; i++ )
                    {
                        if ( pDeviceContext->DeviceRelation.Objects[i] && i < pDeviceContext->DeviceRelation.Count)
                        {
                            ObDereferenceObject(pDeviceContext->DeviceRelation.Objects[i]);
                            ZwMakeTemporaryObject(pDeviceContext->DeviceRelation.Objects[i]);
                            ZwClose(pDeviceContext->DeviceRelation.Objects[i]);
                            IoDeleteDevice(pDeviceContext->DeviceRelation.Objects[i]);
                            pDeviceContext->DeviceRelation.Objects[i] = NULL;
                        }
                    }
                    RemoveParentDeviceContext(pDeviceObject);
                }
            }
            break;
        
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if ( pIrpStack->Parameters.QueryDeviceRelations.Type == BusRelations )
            {
                DWORD RelationsSize;
                PDEVICE_RELATIONS pDeviceRelations;
                PDEVICE_EXTENSION pDeviceExtension;
                
                gwPDO_Count = NoJoy == FALSE;
                pDeviceContext = (PDEVICE_CONTEXT)((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->DeviceContext;
                
                if ( pDeviceContext ) 
                {
                    RelationsSize = sizeof(DEVICE_RELATIONS) + (gwPDO_Count * sizeof(PDEVICE_OBJECT));
                    pDeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, RelationsSize);
                    
                    if (!pDeviceRelations)
                    {
                        pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    RtlZeroMemory(pDeviceRelations, RelationsSize);
                    for ( i = 0; i < gwPDO_Count; i++ )
                    {
                        if ( i || !NoJoy )
                        {
                            // NB: i < pDeviceContext->DeviceRelation.Count not in orig code, added for safety
                            if ( pDeviceContext->DeviceRelation.Objects[i] && i < pDeviceContext->DeviceRelation.Count)
                            {
                                pDeviceRelations->Objects[pDeviceRelations->Count++] = pDeviceContext->DeviceRelation.Objects[i];
                                ObReferenceObject( pDeviceContext->DeviceRelation.Objects[i] );
                                pDeviceExtension = (PDEVICE_EXTENSION)pDeviceContext->DeviceRelation.Objects[i]->DeviceExtension;
                                pDeviceExtension->DeviceNumber = i;
                                pDeviceExtension->DeviceObject = pDeviceObject;
                            }
                            else if ( NT_SUCCESS(IoCreateDevice(pDeviceObject->DriverObject, sizeof(DEVICE_EXTENSION), 
                                      NULL, FILE_DEVICE_UNKNOWN, FILE_AUTOGENERATED_DEVICE_NAME, FALSE, &pDeviceObjectNew)) )
                            {
                                pDeviceRelations->Objects[pDeviceRelations->Count++] = pDeviceObjectNew;
                                ObReferenceObject(pDeviceObjectNew);
                                pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObjectNew->DeviceExtension;
                                pDeviceExtension->DeviceContext = NULL;
                                pDeviceExtension->PhysicalDeviceObject = pDeviceObjectNew;
                                pDeviceExtension->Cookie1 = ESSM_COOKIE;;
                                pDeviceExtension->Cookie2 = COOKIE_JOYSTICK;
                                pDeviceExtension->DeviceNumber = i;
                                pDeviceExtension->DeviceObject = pDeviceObject;
                                pDeviceObjectNew->Flags |= DO_POWER_PAGABLE | DO_DIRECT_IO;
                            }
                        }
                    }
                    if ( pIrp->IoStatus.Information )
                        ExFreePool((PVOID)pIrp->IoStatus.Information);
                    pIrp->IoStatus.Status = STATUS_SUCCESS;
                    pIrp->IoStatus.Information = (ULONG_PTR)pDeviceRelations;
                }
            }
            break;
        case IRP_MN_START_DEVICE:
            pDeviceContext = (PDEVICE_CONTEXT)((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->DeviceContext;
            if ( !pDeviceContext )
            {
                pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return STATUS_INSUFFICIENT_RESOURCES;               
            }
            ParentDeviceContext = pDeviceContext;
            pDeviceContext->IOPort = 0;
            pDeviceObjectNew = NULL;
            pPartialResourceList = &pIrpStack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList;
            for ( i = 0; i < pPartialResourceList->Count; i++ )
            {
                switch ( pPartialResourceList->PartialDescriptors[i].Type )
                {
                    case CmResourceTypePort:
                        if ( !pDeviceContext->IOPort )
                            pDeviceContext->IOPort = pIrpStack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i].u.Port.Start.LowPart;
                        break;
                    case CmResourceTypeInterrupt:
                        pDeviceContext->InterruptVector = pPartialResourceList->PartialDescriptors[i].u.Interrupt.Vector;
                        break;
                }
            }
            break;
    }

    //
    // Pass the IRPs on to PortCls
    //
    return PcDispatchIrp( pDeviceObject,
                              pIrp );
}


/*****************************************************************************
 * ChildPdoPnp()
 *****************************************************************************
 * PNP Dispatcher
 */
extern "C"
NTSTATUS
ChildPdoPnp
(
    IN      PDEVICE_OBJECT     pDeviceObject,
    IN      PIRP               pIrp,
    IN      PIO_STACK_LOCATION pIrpStack,
    IN      PDEVICE_EXTENSION  pDeviceExtension
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);
    ASSERT(pIrpStack);

    NTSTATUS Status = pIrp->IoStatus.Status;
    PCM_PARTIAL_RESOURCE_LIST pPartialResourceList;
    PDEVICE_CAPABILITIES Capabilities;
    PIO_RESOURCE_REQUIREMENTS_LIST NewRequirements;
    PPNP_BUS_INFORMATION BusInformation;
    DWORD PortData, PortData2;
    PDEVICE_RELATIONS pDeviceRelations;
    unsigned int i;
    
    _DbgPrintF(DEBUGLVL_TERSE, ("ChildPdoPnp() %x", pIrpStack->MinorFunction));
    
    switch( pIrpStack->MinorFunction )
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if ( pIrpStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation )
            {
                pDeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(NonPagedPool, sizeof(DEVICE_RELATIONS) + sizeof(PDEVICE_OBJECT) /* why? */);
                if ( !pDeviceRelations )
                  return STATUS_INSUFFICIENT_RESOURCES;
                pDeviceRelations->Count = 1;
                pDeviceRelations->Objects[0] = pDeviceObject;
                ObfReferenceObject(pDeviceObject);
                if ( pIrp->IoStatus.Information )
                    ExFreePool((PVOID)pIrp->IoStatus.Information);
                pIrp->IoStatus.Information = (ULONG_PTR)pDeviceRelations;
                Status = STATUS_SUCCESS;
            }
            break;
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_STOP_DEVICE:
            if ( pDeviceExtension->DeviceNumber == 0 )
            {
                AccessConfigSpace(pDeviceExtension->DeviceObject, FALSE, &pDeviceExtension->JoystickIRQ, 0x20, sizeof(pDeviceExtension->JoystickIRQ));
                Status = STATUS_SUCCESS;
            }
            break;
        case IRP_MN_START_DEVICE:
            pDeviceExtension->DeviceState.SystemState = PowerSystemWorking;
            if ( pDeviceExtension->DeviceNumber == 0 )
            {
                pPartialResourceList = &pIrpStack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList;
                for ( i = 0; i < pIrpStack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList.Count; i++ )
                {
                    switch ( pIrpStack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList.PartialDescriptors->Type )
                    {
                        case CmResourceTypePort:
                            pDeviceExtension->JoystickPort = pPartialResourceList->PartialDescriptors[i].u.Port.Start.LowPart;
                            break;
                    }
                }
            }
            if ( pDeviceExtension->JoystickPort == 0x201 ) // Joystick port
            {
ConfigJoyPort:
                AccessConfigSpace(pDeviceExtension->DeviceObject, TRUE,  &PortData, ESM_LEGACY_AUDIO_CONTROL, sizeof(PortData));
                PortData |= 4;
                AccessConfigSpace(pDeviceExtension->DeviceObject, FALSE, &PortData, ESM_LEGACY_AUDIO_CONTROL, sizeof(PortData));
                AccessConfigSpace(pDeviceExtension->DeviceObject, TRUE,  &PortData, ESM_GAMEPORT, sizeof(PortData));
                pDeviceExtension->JoystickIRQ = PortData;
                PortData = 0x201;
                AccessConfigSpace(pDeviceExtension->DeviceObject, FALSE, &PortData, ESM_GAMEPORT, sizeof(PortData));
            }
            Status = STATUS_SUCCESS;
            break;
            
        case IRP_MN_QUERY_CAPABILITIES:
            Capabilities = pIrpStack->Parameters.DeviceCapabilities.Capabilities;
            Capabilities->LockSupported = FALSE;
            Capabilities->EjectSupported = FALSE;
            Capabilities->Removable = FALSE;
            Capabilities->DockDevice = FALSE;
            Capabilities->UniqueID = FALSE;
            Capabilities->SilentInstall = FALSE;
            Capabilities->RawDeviceOK = FALSE;
            Capabilities->SurpriseRemovalOK = FALSE;
            Capabilities->DeviceState[0] = PowerDeviceD3;
            Capabilities->DeviceState[1] = PowerDeviceD0;
            Capabilities->DeviceState[2] = PowerDeviceD1;
            Capabilities->DeviceState[3] = PowerDeviceD2;
            Capabilities->DeviceState[4] = PowerDeviceD3;
            Capabilities->DeviceState[5] = PowerDeviceD3;
            Capabilities->DeviceState[6] = PowerDeviceD3;
            Capabilities->SystemWake = PowerSystemUnspecified;
            Capabilities->DeviceWake = PowerDeviceUnspecified;
            Capabilities->D1Latency = 0;
            Capabilities->D2Latency = 0;
            Capabilities->D3Latency = 0;
            Status = STATUS_SUCCESS;
            break;
            
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            if ( pDeviceExtension->DeviceNumber == 0 )
            {
                NewRequirements = (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePool(PagedPool, sizeof(IO_RESOURCE_REQUIREMENTS_LIST));
                if ( NewRequirements )
                {
                    RtlZeroMemory(NewRequirements, sizeof(IO_RESOURCE_REQUIREMENTS_LIST));
                    NewRequirements->ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);
                    NewRequirements->AlternativeLists = 1;
                    NewRequirements->List[0].Descriptors[0].Option = 0;
                    NewRequirements->List[0].Descriptors[0].ShareDisposition = CmResourceShareUndetermined;
                    NewRequirements->List[0].Descriptors[0].u.Port.MinimumAddress.HighPart = 0;
                    NewRequirements->List[0].Descriptors[0].u.Port.MaximumAddress.HighPart = 0;
                    NewRequirements->List[0].Descriptors[0].u.Port.MinimumAddress.LowPart = 0x201;
                    NewRequirements->List[0].Descriptors[0].u.Port.MaximumAddress.LowPart = 0x201;
                    NewRequirements->List[0].Version = 1;
                    NewRequirements->List[0].Revision = 1;
                    NewRequirements->List[0].Count = 1;
                    NewRequirements->List[0].Descriptors[0].Type = CmResourceTypePort;
                    NewRequirements->List[0].Descriptors[0].Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                    NewRequirements->List[0].Descriptors[0].u.Port.Length = 1;
                    NewRequirements->List[0].Descriptors[0].u.Port.Alignment = 1;
                    pDeviceExtension->JoystickPort = 0x201;
                    pIrp->IoStatus.Information = (ULONG_PTR)NewRequirements;
                    goto ConfigJoyPort;
                }
            }
            break;
        case IRP_MN_QUERY_DEVICE_TEXT:
            if ( pIrpStack->Parameters.Read.Length || pDeviceExtension->DeviceNumber > 0 || 
                 !ReturnDupString(pIrp,  L"ESS ES1969 Gameport Joystick") ) break;
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_QUERY_ID:
            switch ( pIrpStack->Parameters.QueryId.IdType )
            {
                case BusQueryCompatibleIDs:
                    if ( pDeviceExtension->DeviceNumber > 0 || 
                         !ReturnDupString(pIrp,  L"*PNPB02F") ) break;
                    Status = STATUS_SUCCESS;
                    break;
                case BusQueryInstanceID:
                    if ( !ReturnDupString(pIrp,  L"00000000") ) break;
                    Status = STATUS_SUCCESS;
                    break;
                case BusQueryHardwareIDs:
                    if ( pDeviceExtension->DeviceNumber > 0 || 
                         !ReturnDupString(pIrp,  L"ES1969_GAMEPORT") ) break;
                    Status = STATUS_SUCCESS;
                    break;
                case BusQueryDeviceID:
                    if ( pDeviceExtension->DeviceNumber > 0 || 
                         !ReturnDupString(pIrp,  L"ESS_ES1969\\CHILD0000") ) break;
                    Status = STATUS_SUCCESS;
                    break;
            }
            break;
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            AccessConfigSpace(pDeviceExtension->DeviceObject, TRUE,  &PortData,  ESM_LEGACY_AUDIO_CONTROL, sizeof(PortData));
            PortData2 = PortData & (~0x8000);
            AccessConfigSpace(pDeviceExtension->DeviceObject, FALSE, &PortData2, ESM_LEGACY_AUDIO_CONTROL, sizeof(PortData2));
            pIrp->IoStatus.Information = READ_PORT_UCHAR((PUCHAR)((ULONG_PTR)pDeviceExtension->JoystickPort)) == 0xFF ? PNP_DEVICE_FAILED : 0;
            AccessConfigSpace(pDeviceExtension->DeviceObject, FALSE, &PortData,  ESM_LEGACY_AUDIO_CONTROL, sizeof(PortData));
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_QUERY_BUS_INFORMATION:
            if ((BusInformation = (PPNP_BUS_INFORMATION)ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION))) == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            BusInformation->BusTypeGuid = GUID_Solo1Bus;
            BusInformation->BusNumber = 0;
            BusInformation->LegacyBusType = PNPBus;
            pIrp->IoStatus.Information = (ULONG_PTR)BusInformation;
            Status = STATUS_SUCCESS;
            break;
    }

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}

/*****************************************************************************
 * AdapterGlobalIrpDispatch()
 *****************************************************************************
 * Main dispatch routine 
 */
extern "C"
NTSTATUS
AdapterGlobalIrpDispatch
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    PIO_STACK_LOCATION pIrpStack =
        IoGetCurrentIrpStackLocation(pIrp);
    PDEVICE_EXTENSION pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

    if (pDeviceExtension->Cookie1 == ESSM_COOKIE && pDeviceExtension->Cookie2 == COOKIE_JOYSTICK)
    {
        switch ( pIrpStack->MajorFunction )
        {
            case IRP_MJ_POWER:
                return ChildPdoPower(pDeviceObject, pIrp, pIrpStack, pDeviceExtension);

            default:
                pIrp->IoStatus.Status = STATUS_SUCCESS;

            case IRP_MJ_SYSTEM_CONTROL:
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return pIrp->IoStatus.Status;
                
            case IRP_MJ_PNP:
                return ChildPdoPnp(pDeviceObject, pIrp, pIrpStack, pDeviceExtension);
        }
    }
    else if (pDeviceExtension->Cookie1 == ESSM_COOKIE && 
             pDeviceExtension->Cookie2 == COOKIE_SOUNDCARD && 
             pIrpStack->MajorFunction == IRP_MJ_PNP)
    {
        return AdapterDispatchPnp(pDeviceObject, pIrp);
    }

    //
    // Pass the IRPs on to PortCls
    //
    return PcDispatchIrp( pDeviceObject,
                              pIrp );
}


/*****************************************************************************
 * Unload()
 *****************************************************************************
 * This function is called by the operating system when the device is unloaded.
 */
extern "C"
VOID
Unload
(
    IN PDRIVER_OBJECT   DriverObject
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);
}


/*****************************************************************************
 * DriverEntry()
 *****************************************************************************
 * This function is called by the operating system when the driver is loaded.
 * All adapter drivers can use this code without change.
 */
extern "C"
NTSTATUS
DriverEntry
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPathName
)
{
    UNREFERENCED_PARAMETER(RegistryPathName);

    PAGED_CODE();

    int i;
    
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = AdapterGlobalIrpDispatch;
    
    DriverObject->DriverExtension->AddDevice = AddDevice;
    DriverObject->DriverUnload = Unload;
    
    //return STATUS_UNSUCCESSFUL;
    return STATUS_SUCCESS;    
}


/*****************************************************************************
 * ChildPdoPower()
 *****************************************************************************
 * PDO Power
 */
extern "C"
NTSTATUS
ChildPdoPower
(
    IN      PDEVICE_OBJECT     pDeviceObject,
    IN      PIRP               pIrp,
    IN      PIO_STACK_LOCATION pIrpStack,
    IN      PDEVICE_EXTENSION  pDeviceExtension
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);
    ASSERT(pIrpStack);

    NTSTATUS Status = pIrp->IoStatus.Status;
    
    switch( pIrpStack->MinorFunction )
    {
        case IRP_MN_REMOVE_DEVICE:
        {
            POWER_STATE DeviceState, SystemState;
            
            DeviceState.SystemState = pIrpStack->Parameters.Power.State.SystemState;
            if ( pIrpStack->Parameters.Power.Type )
            {
                if ( pIrpStack->Parameters.Power.Type != DevicePowerState ) break;
                PoSetPowerState(pDeviceObject, DevicePowerState, DeviceState);
                pDeviceExtension->DeviceState = DeviceState;
            }
            else
            {
                SystemState.SystemState = PowerSystemWorking;
                if ( DeviceState.SystemState != PowerSystemWorking )
                    SystemState.SystemState = PowerSystemSleeping3;
                PoRequestPowerIrp(pDeviceObject, IRP_MN_REMOVE_DEVICE, SystemState, NULL, NULL, NULL);
                pDeviceExtension->SystemState = SystemState;
            }
        }
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = STATUS_SUCCESS;
            break;
    }
    PoStartNextPowerIrp(pIrp);
    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}


/*****************************************************************************
 * AddDevice()
 *****************************************************************************
 * This function is called by the operating system when the device is added.
 */
extern "C"
NTSTATUS
AddDevice
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
)
{
    PAGED_CODE();

    //
    // Tell the class driver to add the device.
    //
    NTSTATUS ntStatus = PcAddAdapterDevice( DriverObject,
                                            PhysicalDeviceObject,
                                            PCPFNSTARTDEVICE( StartDevice ),
                                            MAX_MINIPORTS,
                                            0 );

    if(NT_SUCCESS(ntStatus))
    {
        PDEVICE_OBJECT pDeviceObject;
        PDEVICE_EXTENSION pDeviceExtension;
        PDEVICE_CONTEXT DeviceContext = (PDEVICE_CONTEXT)ExAllocatePool(PagedPool, sizeof(DEVICE_CONTEXT));
        

        if (!DeviceContext) return STATUS_INSUFFICIENT_RESOURCES;
        RtlZeroMemory(DeviceContext, sizeof(DeviceContext));
        
        pDeviceObject = IoGetAttachedDeviceReference(PhysicalDeviceObject);
        pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
        pDeviceExtension->Cookie1 = ESSM_COOKIE;
        pDeviceExtension->Cookie2 = COOKIE_SOUNDCARD;
        pDeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
        pDeviceExtension->DeviceContext = DeviceContext;
        ObfDereferenceObject(pDeviceObject);
    }

    return ntStatus;   
}



/*****************************************************************************
 * InstallSubdevice()
 *****************************************************************************
 * This function creates and registers a subdevice consisting of a port
 * driver, a minport driver and a set of resources bound together.  It will
 * also optionally place a pointer to an interface on the port driver in a
 * specified location before initializing the port driver.  This is done so
 * that a common ISR can have access to the port driver during initialization,
 * when the ISR might fire.
 */
NTSTATUS
InstallSubdevice
(
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIRP                Irp,
    IN      PWCHAR              Name,
    IN      REFGUID             PortClassId,
    IN      REFGUID             MiniportClassId,
    IN      PFNCREATEINSTANCE   MiniportCreate      OPTIONAL,
    IN      PUNKNOWN            UnknownAdapter      OPTIONAL,
    IN      PRESOURCELIST       ResourceList,
    IN      REFGUID             PortInterfaceId,
    OUT     PUNKNOWN *          OutPortInterface    OPTIONAL,
    OUT     PUNKNOWN *          OutPortUnknown      OPTIONAL,
    OUT     PUNKNOWN *          OutMiniportUnknown  OPTIONAL
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Name);
    ASSERT(ResourceList);

    //
    // Create the port driver object
    //
    PPORT       port;
    NTSTATUS    ntStatus = PcNewPort(&port,PortClassId);

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Deposit the port somewhere if it's needed.
        //
        if (OutPortInterface)
        {
            ntStatus = port->QueryInterface
            (
                PortInterfaceId,
                (PVOID *) OutPortInterface
            );
        }

        if (NT_SUCCESS(ntStatus))
        {

            PUNKNOWN miniport;
            //
            // Create the miniport object
            //
            if (MiniportCreate)
            {
                ntStatus = MiniportCreate
                (
                    &miniport,
                    MiniportClassId,
                    NULL,
                    NonPagedPool
                );
            }
            else
            {
                ntStatus = PcNewMiniport((PMINIPORT*) &miniport,MiniportClassId);
            }

            if (NT_SUCCESS(ntStatus))
            {              
                //
                // Deposit the port as an unknown if it's needed.
                //
                if (OutMiniportUnknown)
                {
                    ntStatus = miniport->QueryInterface
                    (
                        IID_IUnknown,
                        (PVOID *) OutMiniportUnknown
                    );
                }

                if (NT_SUCCESS(ntStatus))
                {              
                    //
                    // Init the port driver and miniport in one go.
                    //
                    ntStatus = port->Init( DeviceObject,
                                           Irp,
                                           miniport,
                                           UnknownAdapter,
                                           ResourceList );

                    if (NT_SUCCESS(ntStatus))
                    {
                        //
                        // Register the subdevice (port/miniport combination).
                        //
                        ntStatus = PcRegisterSubdevice( DeviceObject,
                                                        Name,
                                                        port );
                        if (!(NT_SUCCESS(ntStatus)))
                        {
                            _DbgPrintF(DEBUGLVL_TERSE, ("StartDevice: PcRegisterSubdevice failed"));
                        }
                    }
                    else
                    {
                        _DbgPrintF(DEBUGLVL_TERSE, ("InstallSubdevice: port->Init failed"));
                    }
                }
                
                //
                // We don't need the miniport any more.  Either the port has it,
                // or we've failed, and it should be deleted.
                //
                miniport->Release();
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE, ("InstallSubdevice: PcNewMiniport failed"));
            }

            if (NT_SUCCESS(ntStatus))
            {
                //
                // Deposit the port as an unknown if it's needed.
                //
                if (OutPortUnknown)
                {
                    ntStatus = port->QueryInterface
                    (
                        IID_IUnknown,
                        (PVOID *) OutPortUnknown
                    );
                }
            }
            
            port->Release();
            
            return ntStatus;
        }
        
        //
        // Retract previously delivered port interface.
        //
        if (OutPortInterface && (*OutPortInterface))
        {
            (*OutPortInterface)->Release();
            *OutPortInterface = NULL;
        }

        //
        // Release the reference which existed when PcNewPort() gave us the
        // pointer in the first place.  This is the right thing to do
        // regardless of the outcome.
        //
        port->Release();
    }

    return ntStatus;
}


/*****************************************************************************
 * StartDevice()
 *****************************************************************************
 * This function is called by the operating system when the device is started.
 * It is responsible for starting the miniports.  This code is specific to
 * the adapter because it calls out miniports for functions that are specific
 * to the adapter.
 */
NTSTATUS
StartDevice
(
    IN  PDEVICE_OBJECT  DeviceObject,   // Device object.
    IN  PIRP            Irp,            // IO request packet.
    IN  PRESOURCELIST   ResourceList    // List of hardware resources.
)
{
    PAGED_CODE();


    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(ResourceList);

    //
    // These are the sub-lists of resources that will be handed to the
    // miniports.
    //
    PRESOURCELIST   resourceListTopology    = NULL;
    PRESOURCELIST   resourceListWave        = NULL;
    PRESOURCELIST   resourceListFmSynth     = NULL;
    PRESOURCELIST   resourceListUart        = NULL;
    PRESOURCELIST   resourceListAdapter     = NULL;

    //
    // These are the port driver pointers we are keeping around for registering
    // physical connections.
    //
    PUNKNOWN    unknownTopology        = NULL;
    PUNKNOWN    unknownWave            = NULL;
    PUNKNOWN    unknownMiniportWave    = NULL;
    PUNKNOWN    unknownFmSynth         = NULL;
    PUNKNOWN    unknownMiniportFmSynth = NULL;
    PUNKNOWN    unknownMiniportUart    = NULL;
    
    PDEVICE_CONTEXT pDeviceContext;

    //
    // Assign resources to individual miniports.  Each sub-list is a copy
    // of the resources from the master list. Each sublist must be released.
    //
    NTSTATUS ntStatus = AssignResources( ResourceList,
                                         &resourceListTopology,
                                         &resourceListWave,
                                         &resourceListFmSynth,
                                         &resourceListUart,
                                         &resourceListAdapter );

    //
    // if AssignResources succeeded...
    //
    if(NT_SUCCESS(ntStatus))
    {
        //
        // If the adapter has resources...
        //
        PADAPTERCOMMON pAdapterCommon = NULL;
        if (resourceListAdapter)
        {
            PUNKNOWN pUnknownCommon;

            // create a new adapter common object
            ntStatus = NewAdapterCommon( &pUnknownCommon,
                                         IID_IAdapterCommon,
                                         NULL,
                                         NonPagedPool );
            if (NT_SUCCESS(ntStatus))
            {
                ASSERT( pUnknownCommon );

                // query for the IAdapterCommon interface
                ntStatus = pUnknownCommon->QueryInterface( IID_IAdapterCommon,
                                                           (PVOID *)&pAdapterCommon );
                if (NT_SUCCESS(ntStatus))
                {
                    // Initialize the object
                    ntStatus = pAdapterCommon->Init( ResourceList,
                                                     DeviceObject );
                    if (NT_SUCCESS(ntStatus))
                    {
                        // register with PortCls for power-management services
                        ntStatus = PcRegisterAdapterPowerManagement( (PUNKNOWN)pAdapterCommon,
                                                                     DeviceObject );
                    }
                }

                // release the IUnknown on adapter common
                pUnknownCommon->Release();
            }

            // release the adapter common resource list
            resourceListAdapter->Release();
        }

        // Start the wave table miniport if it exists.
        if (resourceListWave)
        {
            ntStatus = InstallSubdevice( DeviceObject,
                                         Irp,
                                         L"Wave",
                                         CLSID_PortWaveCyclic,
                                         CLSID_PortWaveCyclic, // not used
                                         CreateMiniportWaveSolo,
                                         pAdapterCommon,
                                         resourceListWave,
                                         IID_IPortWaveCyclic,
                                         (PUNKNOWN*)pAdapterCommon->WavePortDriverDest(),
                                         &unknownWave,
                                         &unknownMiniportWave);

            
            // release the wavetable resource list
            resourceListWave->Release();
        }


        //
        // Start the topology miniport.
        //
        if (resourceListTopology)
        {
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = InstallSubdevice( DeviceObject,
                                             Irp,
                                             L"Topology",
                                             CLSID_PortTopology,
                                             CLSID_PortTopology, // not used
                                             CreateMiniportTopologyESS,
                                             pAdapterCommon,
                                             resourceListTopology,
                                             GUID_NULL,
                                             NULL,
                                             &unknownTopology,
                                             NULL);
            }

            // release the wave resource list
            resourceListTopology->Release();
        }

        //
        // Start the FM synth miniport if it exists.
        //
        if (resourceListFmSynth)
        {
            if (NT_SUCCESS(ntStatus))
            {
                //
                // Failure here is not fatal.
                //
                InstallSubdevice( DeviceObject,
                                  Irp,
                                  L"FMSynth",
                                  CLSID_PortMidi,
                                  CLSID_MiniportDriverESFMSynth,
                                  CreateMiniportMidiESFM,
                                  pAdapterCommon,
                                  resourceListFmSynth,
                                  GUID_NULL,
                                  NULL,
                                  &unknownFmSynth,
                                  &unknownMiniportFmSynth);
            }

            // release the FM synth resource list
            resourceListFmSynth->Release();
        }

        //
        // Start the UART miniport if it exists.
        //
        if (resourceListUart)
        {
            pAdapterCommon->Init689();
            
            if (NT_SUCCESS(ntStatus))
            {
                //
                // Failure here is not fatal.
                //
                InstallSubdevice( DeviceObject,
                                  Irp,
                                  L"Uart",
                                  CLSID_PortMidi,
                                  CLSID_MiniportDriverUartESS,
                                  CreateMiniportMidiUartESS,
                                  pAdapterCommon,
                                  resourceListUart,
                                  IID_IPortMidi,
                                  (PUNKNOWN*)pAdapterCommon->MidiPortDriverDest(),
                                  NULL,    //  not physically connected to anything
                                  &unknownMiniportUart);
            }

            resourceListUart->Release();
        }

        if (unknownTopology && unknownWave)
        {
            // register wave <=> topology connections
            PcRegisterPhysicalConnection( (PDEVICE_OBJECT)DeviceObject,
                                        unknownTopology,
                                        11,
                                        unknownWave,
                                        1 );
            PcRegisterPhysicalConnection( (PDEVICE_OBJECT)DeviceObject,
                                        unknownWave,
                                        3,
                                        unknownTopology,
                                        0 );
        }

        if ((pDeviceContext = (PDEVICE_CONTEXT)((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->DeviceContext) != NULL)
        {
            if (unknownMiniportWave)
            {
                unknownMiniportWave->QueryInterface( IID_IMiniportWaveCyclic,
                                                       (PVOID *)&pDeviceContext->MiniportWaveCyclic );
            }

            if (unknownMiniportFmSynth)
            {
                unknownMiniportFmSynth->QueryInterface( IID_IMiniportMidi,
                                                       (PVOID *)&pDeviceContext->MiniportMidiESFM );
            }

            if (unknownMiniportUart)
            {
                unknownMiniportUart->QueryInterface( IID_IMiniportMidi,
                                                       (PVOID *)&pDeviceContext->MiniportMidiUartESS );
            }
        }
        
        //
        // Release the adapter common object.  It either has other references,
        // or we need to delete it anyway.
        //
        if (pAdapterCommon)
        {
            pAdapterCommon->PostProcessing();
            pAdapterCommon->Release();
        }

        //
        // Release the unknowns.
        //
        if (unknownTopology)
        {
            unknownTopology->Release();
        }
        if (unknownWave)
        {
            unknownWave->Release();
        }
        if (unknownFmSynth)
        {
            unknownFmSynth->Release();
        }
        if (unknownMiniportWave)
        {
            unknownMiniportWave->Release();
        }
        if (unknownMiniportUart)
        {
            unknownMiniportUart->Release();
        }
        if (NT_SUCCESS(ntStatus))
            pAdapterCommon->Enable_Irq();
    }

    return ntStatus;
}


/*****************************************************************************
 * AssignResources()
 *****************************************************************************
 * This function assigns the list of resources to the various functions on
 * the card.  This code is specific to the adapter.  All the non-NULL resource
 * lists handed back must be released by the caller.
 */
NTSTATUS
AssignResources
(
    IN      PRESOURCELIST   ResourceList,           // All resources.
    OUT     PRESOURCELIST * ResourceListTopology,   // Topology resources.
    OUT     PRESOURCELIST * ResourceListWave,       // Wave resources.
    OUT     PRESOURCELIST * ResourceListFmSynth,    // FM synth resources.
    OUT     PRESOURCELIST * ResourceListUart,       // Uart resources.
    OUT     PRESOURCELIST * ResourceListAdapter     // For the adapter
)
{
    //
    // Get counts for the types of resources.
    //
    ULONG countIO  = ResourceList->NumberOfPorts();
    ULONG countIRQ = ResourceList->NumberOfInterrupts();
    ULONG countDMA = ResourceList->NumberOfDmas();
    ULONG i;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    
    if ( countIO != 5 || countIRQ != 1 || countDMA )
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;

    //
    // Build the resource list for the card's wave I/O.
    //
    *ResourceListTopology = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            PcNewResourceSublist
            (
                ResourceListTopology,
                NULL,
                PagedPool,
                ResourceList,
                1
            );

        if (NT_SUCCESS(ntStatus))
        {
            //
            // Add the base address
            //
            ASSERT(NT_SUCCESS((*ResourceListTopology)->AddPortFromParent(ResourceList,1)));
        }
    }

    //
    // Build list of resources for wave table.
    //
    *ResourceListWave = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            PcNewResourceSublist
            (
                ResourceListWave,
                NULL,
                PagedPool,
                ResourceList,
                2
            );

        if (NT_SUCCESS(ntStatus))
        {
            ASSERT(NT_SUCCESS((*ResourceListWave)->AddPortFromParent(ResourceList,1)));
            ASSERT(NT_SUCCESS((*ResourceListWave)->AddInterruptFromParent(ResourceList,0)));
        }
    }

    //
    // Build list of resources for UART.
    //
    *ResourceListUart = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            PcNewResourceSublist
            (
                ResourceListUart,
                NULL,
                PagedPool,
                ResourceList,
                2
            );

        if (NT_SUCCESS(ntStatus))
        {
            ASSERT(NT_SUCCESS((*ResourceListUart)->AddPortFromParent(ResourceList,3)));
            ASSERT(NT_SUCCESS((*ResourceListUart)->AddInterruptFromParent(ResourceList,0)));
        }
    }

    //
    // Build list of resources for FM synth.
    //
    *ResourceListFmSynth = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            PcNewResourceSublist
            (
                ResourceListFmSynth,
                NULL,
                PagedPool,
                ResourceList,
                1
            );

        if (NT_SUCCESS(ntStatus))
        {
            ASSERT(NT_SUCCESS((*ResourceListFmSynth)->AddPortFromParent(ResourceList,1)));
            (*ResourceListFmSynth)->FindTranslatedPort(0)->u.Generic.Length = 4;
        }
    }

    //
    // Build list of resources for the adapter.
    //
    *ResourceListAdapter = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            PcNewResourceSublist
            (
                ResourceListAdapter,
                NULL,
                PagedPool,
                ResourceList,
                6
            );

        if (NT_SUCCESS(ntStatus))
        {
            //
            // The interrupt to share.
            //
            ASSERT(NT_SUCCESS((*ResourceListAdapter)->AddInterruptFromParent(ResourceList,0)));

            //
            // The base IO port (to tell who's interrupt it is)
            //
            for ( i = 0; i < countIO; i++ )
            {
                ASSERT(NT_SUCCESS((*ResourceListAdapter)->AddPortFromParent(ResourceList,i)));
            }
        }
    }

    //
    // Clean up if failure occurred.
    //
    if (! NT_SUCCESS(ntStatus))
    {
        if (*ResourceListTopology)
        {
            (*ResourceListTopology)->Release();
            *ResourceListTopology = NULL;
        }
        if (*ResourceListWave)
        {
            (*ResourceListWave)->Release();
            *ResourceListWave = NULL;
        }
        if (*ResourceListUart)
        {
            (*ResourceListUart)->Release();
            *ResourceListUart = NULL;
        }
        if (*ResourceListFmSynth)
        {
            (*ResourceListFmSynth)->Release();
            *ResourceListFmSynth = NULL;
        }
        if(*ResourceListAdapter)
        {
            (*ResourceListAdapter)->Release();
            *ResourceListAdapter = NULL;
        }
    }


    return ntStatus;
}









#pragma code_seg()

/*****************************************************************************
 * _purecall()
 *****************************************************************************
 * The C++ compiler loves me.
 * TODO: Figure out how to put this into portcls.sys
 */
int __cdecl
_purecall( void )
{
    ASSERT( !"Pure virutal function called" );
    return 0;
}
