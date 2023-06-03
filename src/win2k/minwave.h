/*****************************************************************************
 * minwave.h - ESS wave miniport private definitions
 *****************************************************************************
 * Copyright (c) 2023 leecher@dose.0wnz.at  All rights reserved.
 */

#ifndef _MINWAVE_PRIVATE_H_
#define _MINWAVE_PRIVATE_H_

#include "common.h"




 
/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * CMiniportWaveSolo
 *****************************************************************************
 * ESS wave miniport.  This object is associated with the device and is
 * created when the device is started.  The class inherits IMiniportWaveCyclic
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CMiniportWaveSolo
:   public IMiniportWaveCyclic,
    public IPowerNotify,
    public CUnknown
{
private:
    PADAPTERCOMMON      AdapterCommon;              // Adapter common object.
    PPORTWAVECYCLIC     Port;                       // Callback interface.
    PPCFILTER_DESCRIPTOR FilterDescriptor;          // FilterDescriptor

    ULONG               NotificationInterval;       // In milliseconds.
    ULONG               SamplingFrequency;          // Frames per second.

    BOOLEAN             AllocatedCapture;           // Capture in use.
    BOOLEAN             AllocatedRender;            // Render in use.
    BOOLEAN             Allocated8Bit;              // 8-bit DMA in use.
    BOOLEAN             Allocated16Bit;             // 16-bit DMA in use.
    
    SYSTEM_POWER_STATE  PowerState;                 // Power state 
    BYTE                SaveExtSampleRate;
    BYTE                SaveFilterDiv;
    BYTE                SaveA2SampleRate;
    BYTE                SaveA2ClockRate;

    PDMACHANNEL         DmaChannel8;                // Abstracted channel.
    PDMACHANNEL         DmaChannel16;               // Abstracted channel.

    PSERVICEGROUP       ServiceGroup;               // For notification.
    KMUTEX              SampleRateSync;             // Sync for sample rate changes.
    DWORD               Running;                    // Instances running

    /*************************************************************************
     * CMiniportWaveSolo methods
     *
     * These are private member functions used internally by the object.  See
     * MINIPORT.CPP for specific descriptions.
     */
    NTSTATUS ProcessResources
    (
        IN      PRESOURCELIST   ResourceList
    );
    NTSTATUS ValidateFormat
    (
        IN      PKSDATAFORMAT   Format
    );

public:
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
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveSolo);

    ~CMiniportWaveSolo();

    /*************************************************************************
     * This macro is from PORTCLS.H.  It lists all the interface's functions.
     */
    IMP_IMiniportWaveCyclic;
    
    /*************************************************************************
     * IPowerNotify methods
     */
    IMP_IPowerNotify;

    /*************************************************************************
     * Friends
     *
     * The miniport stream class is a friend because it needs to access the
     * private member variables of this class.
     */
    friend class CMiniportWaveStreamSolo;
};

/*****************************************************************************
 * CMiniportWaveStreamSolo
 *****************************************************************************
 * ESS wave miniport stream.  This object is associated with a streaming pin
 * and is created when a pin is created on the filter.  The class inherits
 * IMiniportWaveCyclicStream so it can expose this interface and CUnknown so
 * it automatically gets reference counting and aggregation support.
 */
class CMiniportWaveStreamSolo
:   public IMiniportWaveCyclicStream,
    public IDmaChannel,
    public IDrmAudioStream,
    public CUnknown
{
private:
    CMiniportWaveSolo *         Miniport;       // Miniport that created us.
    ULONG                       Channel;        // Index into channel list.
    BOOLEAN                     Capture;        // Capture or render.
    BOOLEAN                     Format16Bit;    // 16- or 8-bit samples.
    BOOLEAN                     FormatStereo;   // Two or one channel.
    KSSTATE                     State;          // Stop, pause, run.
    PDMACHANNEL                 DmaChannel;     // DMA channel to use.
    PVOID                       DmaAddress;     // DMA address to use.
    DWORD                       PosStart;
    DWORD                       DmaBufferSize;
    BOOLEAN                     Active;

public:
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
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveStreamSolo);

    ~CMiniportWaveStreamSolo();
    
    NTSTATUS 
    Init
    (
        IN      CMiniportWaveSolo *         Miniport,
        IN      ULONG                       Channel,
        IN      BOOLEAN                     Capture,
        IN      PKSDATAFORMAT               DataFormat,
        OUT     PDMACHANNEL                 DmaChannel
    );

    /*************************************************************************
     * Include IMiniportWaveCyclicStream public/exported methods (portcls.h)
     */
    IMP_IMiniportWaveCyclicStream;

    /*************************************************************************
     * Include IDrmAudioStream public/exported methods (drmk.h)
     *************************************************************************
     */
    IMP_IDrmAudioStream;
    
    /*************************************************************************
     * Include IDmaChannel public/exported methods (portcls.h)
     *************************************************************************
     */
    IMP_IDmaChannel;
};

#endif