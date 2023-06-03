/*****************************************************************************
 * mintopo.h - ESS topology miniport private definitions
 *****************************************************************************
 * Copyright (c) 2023 leecher@dose.0wnz.at  All rights reserved.
 */

#ifndef _ESSTOPO_PRIVATE_H_
#define _ESSTOPO_PRIVATE_H_

#include "common.h"

#define NODENUM     39  // Not sure why not 32 

typedef struct
{
    BYTE Reg;
    BYTE Val;
} MIXER_TABLE;

typedef struct 
{
  DWORD Channel[2];
  DWORD Pin;
} LINEOUT_VOLUME;

/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * CMiniportTopologyESS
 *****************************************************************************
 * ESS topology miniport.  This object is associated with the device and is
 * created when the device is started.  The class inherits IMiniportTopology
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CMiniportTopologyESS 
:   public IMiniportTopology, 
    public IPowerNotify,
    public CUnknown
{
private:
    PPORTTOPOLOGY       m_Port;
    PPORTEVENTS         m_PortEvents;
    
    PUCHAR              m_BaseAddress0;
    DWORD               m_BoardId;
    BYTE                m_VolumeRegs[NODENUM];
    LINEOUT_VOLUME      m_LineVolumes[NODENUM];
    SETTING             m_VolumesIn[10];
    SETTING             m_VolumesOut[10];
    BOOLEAN             m_HideVolumes[NODENUM];
    SYSTEM_POWER_STATE  m_PowerState;
    UCHAR               m_SpatializerLevel;
    UCHAR               m_MonoOutSource;
    BOOLEAN             m_MicPreampGain;
    DWORD               m_MuxPin;

    /*************************************************************************
     * CMiniportTopologyESS methods
     *
     * These are private member functions used internally by the object.  See
     * MINIPORT.CPP for specific descriptions.
     */
    NTSTATUS ProcessResources
    (
        IN      PRESOURCELIST   ResourceList
    );
    VOID InitSpatializer
    (   void
    );
    VOID InitNodeValues
    (   void
    );
    VOID SetNodeValue
    (
        IN      ULONG      Node
    );
    VOID InitTables
    (   void
    );
    VOID GetRegistrySettings
    (   void
    );
    VOID SaveRegistrySettings
    (   void
    );
    BOOLEAN MixerSetMux
    (
        DWORD pin
    );
    USHORT GetHwVol
    (
        IN      ULONG      Node
    );
    BYTE GetMasterHwVol
    (
        IN      int      Volume
    );
    VOID GetMuteFromHardware
    (
        IN      ULONG      Node
    );
    VOID GetVolumeFromHardware
    (
        IN      ULONG      Node
    );

public:
    PADAPTERCOMMON      m_AdapterCommon;      // Adapter common object.
    
    /*************************************************************************
     * The following two macros are from STDUNK.H.  DECLARE_STD_UNKNOWN()
     * defines inline IUnknown implementations that use CUnknown's aggregation
     * support.  NonDelegatingQueryInterface() is declared, but it cannot be
     * implemented generically.  Its definition appears in MINIPORT.CPP.
     * DEFINE_STD_CONSTRUCTOR() defines inline a constructor which accepts
     * only the outer unknown, which is used for aggregation.  The standard
     * create macro (in MINIPORT.CPP) uses this constructor.
     */
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportTopologyESS);

    ~CMiniportTopologyESS();

    /*************************************************************************
     * IMiniport methods
     */
    STDMETHODIMP 
    GetDescription
    (   OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
    );
    STDMETHODIMP 
    DataRangeIntersection
    (   IN      ULONG           PinId
    ,   IN      PKSDATARANGE    DataRange
    ,   IN      PKSDATARANGE    MatchingDataRange
    ,   IN      ULONG           OutputBufferLength
    ,   OUT     PVOID           ResultantFormat     OPTIONAL
    ,   OUT     PULONG          ResultantFormatLength
    )
    {
        UNREFERENCED_PARAMETER(PinId);
        UNREFERENCED_PARAMETER(DataRange);
        UNREFERENCED_PARAMETER(MatchingDataRange);
        UNREFERENCED_PARAMETER(OutputBufferLength);
        UNREFERENCED_PARAMETER(ResultantFormat);
        UNREFERENCED_PARAMETER(ResultantFormatLength);

        return STATUS_NOT_IMPLEMENTED;
    }

    /*************************************************************************
     * IMiniportTopology methods
     */
    STDMETHODIMP Init
    (
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTTOPOLOGY   Port
    );

    /*************************************************************************
     * IPowerNotify methods
     */
    STDMETHODIMP_(void) PowerChangeNotify(
        IN  POWER_STATE     PowerState
    );
    
    /*************************************************************************
     * Friends
     */
    friend
    static
    NTSTATUS
    PropertyHandler_OnOff
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    friend
    static
    NTSTATUS
    PropertyHandlerBasicSupportVolume
    (
        IN      PPCPROPERTY_REQUEST   PropertyRequest
    );
    friend
    static
    NTSTATUS
    PropertyHandler_Level
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    friend
    static
    NTSTATUS
    PropertyHandler_MuxSource
    (
        IN      PPCPROPERTY_REQUEST   PropertyRequest
    );
    friend
    static
    NTSTATUS
    PropertyHandler_CpuResources
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    static
    NTSTATUS
    EventHandler
    (
        IN      PPCEVENT_REQUEST      EventRequest
    );
};

#endif
