/*****************************************************************************
 * mintopo.cpp - ESS topology miniport implementation
 *****************************************************************************
 * Copyright (c) 2023 leecher@dose.0wnz.at  All rights reserved.
 */

#include "limits.h"
#include "mintopo.h"
#include "tables.h"

#define STR_MODULENAME "Topo: "

#define CHAN_LEFT       0
#define CHAN_RIGHT      1
#define CHAN_MASTER     (-1)

#define CONFIG_VAL_SIZE     256

BYTE 	MasterVolumeMap[CONFIG_VAL_SIZE],
		MainRecordVolumeMap[CONFIG_VAL_SIZE],
		WaveVolumeInMap[CONFIG_VAL_SIZE],
		LineInVolumeInMap[CONFIG_VAL_SIZE],
		MicVolumeInMap[CONFIG_VAL_SIZE],
		CDAudioVolumeInMap[CONFIG_VAL_SIZE],
		SynthVolumeInMap[CONFIG_VAL_SIZE],
		AuxBVolumeInMap[CONFIG_VAL_SIZE],
		IISVolumeInMap[CONFIG_VAL_SIZE],
		LineInVolumeOutMap[CONFIG_VAL_SIZE],
		WaveVolumeOutMap[CONFIG_VAL_SIZE],
		MicVolumeOutMap[CONFIG_VAL_SIZE],
		CDAudioVolumeOutMap[CONFIG_VAL_SIZE],
		SynthVolumeOutMap[CONFIG_VAL_SIZE],
		AuxBVolumeOutMap[CONFIG_VAL_SIZE],
		IISVolumeOutMap[CONFIG_VAL_SIZE],
        PhoneVolumeOutMap[CONFIG_VAL_SIZE];
		
typedef struct 
{
	PBYTE Data;
	PWCHAR KeyName;
} REGKEY_TBL;

#define RK(s) {s, L#s}
static REGKEY_TBL m_RegKeys[] = {
    RK(MasterVolumeMap),
    RK(MainRecordVolumeMap),
    RK(WaveVolumeInMap),
    RK(LineInVolumeInMap),
    RK(MicVolumeInMap),
    RK(CDAudioVolumeInMap),
    RK(SynthVolumeInMap),
    RK(AuxBVolumeInMap),
    RK(IISVolumeInMap),
    RK(LineInVolumeOutMap),
    RK(WaveVolumeOutMap),
    RK(MicVolumeOutMap),
    RK(CDAudioVolumeOutMap),
    RK(SynthVolumeOutMap),
    RK(AuxBVolumeOutMap),
    RK(IISVolumeOutMap),
    RK(PhoneVolumeOutMap)
};

typedef struct
{
  WCHAR *Channel[2];
  WCHAR *Hide;
} MIXERREGISTRYSETTING;

MIXERREGISTRYSETTING MixerRegSettings[] =
{
  { L"LeftWaveOutVolume", L"RightWaveOutVolume", L"HideWaveOutVolume" },
  { L"WaveOutMute", NULL, L"HideWaveOutMute" },
  { L"LeftSynthVolume", L"RightSynthVolume", L"HideSynthVolume" },
  { L"SynthMute", NULL, L"HideSynthMute" },
  { L"LeftCDVolume", L"RightCDVolume", L"HideCDVolume" },
  { L"CDMute", NULL, L"HideCDMute" },
  { L"LeftLineInVolume", L"RightLineInVolume", L"HideLineInVolume" },
  { L"LineInMute", NULL, L"HideLineInMute" },
  { L"LeftMicVolume", L"RightMicVolume", L"HideMicVolume" },
  { L"MicMute", NULL, L"HideMicMute" },
  { L"LeftAuxBVolume", L"RightAuxBVolume", L"HideAuxBVolume" },
  { L"AuxBMute", NULL, L"HideAuxBMute" },
  { L"LeftIISVolume", L"RightIISVolume", L"HideIISVolume" },
  { L"IISMute", NULL, L"HideIISMute" },
  { L"LeftPCSpeakerVolume", L"RightPCSpeakerVolume", L"HidePCSpeakerVolume" },
  { L"PCSpeakerMute", NULL, L"HidePCSpeakerMute" },
  { L"LeftPhoneVolume", L"RightPhoneVolume", L"HidePhoneVolume" },
  { L"PhoneMute", NULL, L"HidePhoneMute" },
  { L"LeftSynthInVolume", L"RightSynthInVolume", L"HideSynthInVolume" },
  { L"LeftCDInVolume", L"RightCDInVolume", L"HideCDInVolume" },
  { L"LeftLineInInVolume", L"RightLineInInVolume", L"HideLineInInVolume" },
  { L"LeftMicInVolume", L"RightMicInVolume", L"HideMicInVolume" },
  { NULL, NULL, NULL },
  { L"LeftAuxBInVolume", L"RightAuxBInVolume", L"HideAuxBInVolume" },
  { L"LeftIISInVolume", L"RightIISInVolume", L"HideIISInVolume" },
  { L"LeftMixerInVolume", L"RightMixerInVolume", L"HideMixerInVolume" },
  { NULL, NULL, NULL },
  { L"LeftLineoutVol", L"RightLineoutVol", NULL },
  { L"LineoutMute", NULL, NULL },
  { L"Lineout3dEffect", NULL, L"HideLineout3dEffect" },
  { NULL, NULL, NULL },
  { L"WaveInRecmon", NULL, L"HideWaveInRecmon" }
};

unsigned int VolLut[] =
{
    0x80000000, 0xFFCFDEB4, 0xFFD5E3F4, 0xFFD96987, 0xFFDBE934,
    0xFFDDD960, 0xFFDF6EC7, 0xFFE0C58A, 0xFFE1EE74, 0xFFE2F459,
    0xFFE3DEA0, 0xFFE4B28D, 0xFFE57407, 0xFFE62601, 0xFFE6CACA,
    0xFFE76433, 0xFFE7F3B4, 0xFFE87A81, 0xFFE8F999, 0xFFE971D2,
    0xFFE9E3E0, 0xFFEA505D, 0xFFEAB7CD, 0xFFEB1AA4, 0xFFEB7947,
    0xFFEBD40C, 0xFFEC2B41, 0xFFEC7F2C, 0xFFECD00A, 0xFFED1E11,
    0xFFED6973, 0xFFEDB25C, 0xFFEDF8F4, 0xFFEE3D60, 0xFFEE7FC1,
    0xFFEEC036, 0xFFEEFED9, 0xFFEF3BC6, 0xFFEF7712, 0xFFEFB0D4,
    0xFFEFE920, 0xFFF02008, 0xFFF0559D, 0xFFF089EF, 0xFFF0BD0D,
    0xFFF0EF05, 0xFFF11FE4, 0xFFF14FB6, 0xFFF17E87, 0xFFF1AC60,
    0xFFF1D94C, 0xFFF20554, 0xFFF23081, 0xFFF25ADC, 0xFFF2846C,
    0xFFF2AD39, 0xFFF2D54A, 0xFFF2FCA5, 0xFFF32351, 0xFFF34954,
    0xFFF36EB3, 0xFFF39374, 0xFFF3B79C, 0xFFF3DB2F, 0xFFF3FE34,
    0xFFF420AD, 0xFFF442A0, 0xFFF46410, 0xFFF48501, 0xFFF4A577,
    0xFFF4C576, 0xFFF4E500, 0xFFF50419, 0xFFF522C5, 0xFFF54106,
    0xFFF55EDF, 0xFFF57C52, 0xFFF59963, 0xFFF5B614, 0xFFF5D268,
    0xFFF5EE60, 0xFFF609FF, 0xFFF62548, 0xFFF6403B, 0xFFF65ADD,
    0xFFF6752D, 0xFFF68F2F, 0xFFF6A8E4, 0xFFF6C24D, 0xFFF6DB6D,
    0xFFF6F445, 0xFFF70CD7, 0xFFF72524, 0xFFF73D2E, 0xFFF754F6,
    0xFFF76C7E, 0xFFF783C7, 0xFFF79AD1, 0xFFF7B1A0, 0xFFF7C833,
    0xFFF7DE8C, 0xFFF7F4AC, 0xFFF80A94, 0xFFF82046, 0xFFF835C1,
    0xFFF84B09, 0xFFF8601C, 0xFFF874FD, 0xFFF889AC, 0xFFF89E2B,
    0xFFF8B279, 0xFFF8C699, 0xFFF8DA8A, 0xFFF8EE4E, 0xFFF901E5,
    0xFFF91550, 0xFFF92891, 0xFFF93BA7, 0xFFF94E94, 0xFFF96157,
    0xFFF973F3, 0xFFF98667, 0xFFF998B4, 0xFFF9AADA, 0xFFF9BCDC,
    0xFFF9CEB8, 0xFFF9E06F, 0xFFF9F203, 0xFFFA0374, 0xFFFA14C2,
    0xFFFA25ED, 0xFFFA36F7, 0xFFFA47E0, 0xFFFA58A8, 0xFFFA6950,
    0xFFFA79D8, 0xFFFA8A41, 0xFFFA9A8B, 0xFFFAAAB7, 0xFFFABAC5,
    0xFFFACAB6, 0xFFFADA89, 0xFFFAEA40, 0xFFFAF9DB, 0xFFFB095A,
    0xFFFB18BD, 0xFFFB2805, 0xFFFB3733, 0xFFFB4646, 0xFFFB553F,
    0xFFFB641F, 0xFFFB72E5, 0xFFFB8192, 0xFFFB9027, 0xFFFB9EA3,
    0xFFFBAD08, 0xFFFBBB54, 0xFFFBC989, 0xFFFBD7A8, 0xFFFBE5AF,
    0xFFFBF3A0, 0xFFFC017A, 0xFFFC0F3F, 0xFFFC1CEE, 0xFFFC2A88,
    0xFFFC380C, 0xFFFC457B, 0xFFFC52D6, 0xFFFC601D, 0xFFFC6D4F,
    0xFFFC7A6D, 0xFFFC8778, 0xFFFC946F, 0xFFFCA153, 0xFFFCAE24,
    0xFFFCBAE2, 0xFFFCC78D, 0xFFFCD426, 0xFFFCE0AD, 0xFFFCED22,
    0xFFFCF985, 0xFFFD05D7, 0xFFFD1217, 0xFFFD1E46, 0xFFFD2A65,
    0xFFFD3672, 0xFFFD426E, 0xFFFD4E5B, 0xFFFD5A36, 0xFFFD6602,
    0xFFFD71BE, 0xFFFD7D6A, 0xFFFD8907, 0xFFFD9494, 0xFFFDA012,
    0xFFFDAB80, 0xFFFDB6E0, 0xFFFDC231, 0xFFFDCD73, 0xFFFDD8A7,
    0xFFFDE3CC, 0xFFFDEEE3, 0xFFFDF9EC, 0xFFFE04E7, 0xFFFE0FD4,
    0xFFFE1AB4, 0xFFFE2586, 0xFFFE304A, 0xFFFE3B01, 0xFFFE45AC,
    0xFFFE5049, 0xFFFE5AD9, 0xFFFE655C, 0xFFFE6FD3, 0xFFFE7A3D,
    0xFFFE849B, 0xFFFE8EEC, 0xFFFE9932, 0xFFFEA36B, 0xFFFEAD98,
    0xFFFEB7B9, 0xFFFEC1CF, 0xFFFECBD9, 0xFFFED5D7, 0xFFFEDFCA,
    0xFFFEE9B1, 0xFFFEF38E, 0xFFFEFD5F, 0xFFFF0725, 0xFFFF10E0,
    0xFFFF1A90, 0xFFFF2436, 0xFFFF2DD1, 0xFFFF3761, 0xFFFF40E7,
    0xFFFF4A62, 0xFFFF53D4, 0xFFFF5D3A, 0xFFFF6697, 0xFFFF6FEA,
    0xFFFF7933, 0xFFFF8272, 0xFFFF8BA7, 0xFFFF94D2, 0xFFFF9DF4,
    0xFFFFA70C, 0xFFFFB01A, 0xFFFFB920, 0xFFFFC21C, 0xFFFFCB0E,
    0xFFFFD3F8, 0xFFFFDCD8, 0xFFFFE5AF, 0xFFFFEE7E, 0xFFFFF743
};
#undef RK

#pragma code_seg("PAGE")


/*****************************************************************************
 * CreateMiniportTopologyESS()
 *****************************************************************************
 * Creates a topology miniport object for the ESS adapter.  This uses a
 * macro from STDUNK.H to do all the work.
 */
NTSTATUS
CreateMiniportTopologyESS
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY_(CMiniportTopologyESS,Unknown,UnknownOuter,PoolType,PMINIPORTTOPOLOGY);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportTopologyESS::ProcessResources()
 *****************************************************************************
 * Processes the resource list, setting up helper objects accordingly.
 */
NTSTATUS
CMiniportTopologyESS::
ProcessResources
(
    IN      PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_BLAB,("ProcessResources"));
    ASSERT(ResourceList);

    //
    // Get counts for the types of resources.
    //
    ULONG   countIO     = ResourceList->NumberOfPorts();
    ULONG   countIRQ    = ResourceList->NumberOfInterrupts();
    ULONG   countDMA    = ResourceList->NumberOfDmas();

#if (DBG)
    _DbgPrintF(DEBUGLVL_VERBOSE,("Starting ESS Port 0x%X",
        ResourceList->FindTranslatedPort(0)->u.Port.Start.LowPart) );
#endif

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Make sure we have the expected number of resources.
    //
    if  (   (countIO != 1)
        ||  (countIRQ  != 0)
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
        m_BaseAddress0 =
            PUCHAR(ResourceList->FindTranslatedPort(0)->u.Port.Start.QuadPart);
    }

    return ntStatus;
}


/*****************************************************************************
 * CMiniportTopologyESS::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP
CMiniportTopologyESS::
NonDelegatingQueryInterface
(
    IN      REFIID  Interface,
    OUT     PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportTopologyESS::NonDelegatingQueryInterface]"));

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTTOPOLOGY(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportTopology))
    {
        *Object = PVOID(PMINIPORTTOPOLOGY(this));
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
 * CMiniportTopologyESS::~CMiniportTopologyESS()
 *****************************************************************************
 * Destructor.
 */
CMiniportTopologyESS::
~CMiniportTopologyESS
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportTopologyESS::~CMiniportTopologyESS]"));

    if (m_PortEvents)
    {
        m_AdapterCommon->SetPortEventsDest(NULL);
        m_PortEvents->Release ();
        m_PortEvents = NULL;
    }

    if (m_Port)
    {
        m_Port->Release();
    }

    if (m_AdapterCommon)
    {
        m_AdapterCommon->Release();
    }
}

/*****************************************************************************
 * CMiniportTopologyESS::InitSpatializer()
 *****************************************************************************
 * Initializes Spatializer.
 */
VOID
CMiniportTopologyESS::
InitSpatializer
(   void
)
{
    PAGED_CODE();

    if ( m_BoardId == 0x1869 || m_BoardId == 0x1879 )
    {
        m_AdapterCommon->dspWriteMixer(ESM_MIXER_SPATIALIZER_EN, 0);
        m_AdapterCommon->dspWriteMixer(ESM_MIXER_SPATIALIZER_EN, 4);
        m_AdapterCommon->dspWriteMixer(ESM_MIXER_SPATIALIZER_LV, m_SpatializerLevel);
    }
}

/*****************************************************************************
 * CMiniportTopologyESS::InitNodeValues()
 *****************************************************************************
 * Initializes Spatializer.
 */
VOID
CMiniportTopologyESS::
InitNodeValues
(   void
)
{
    int i;
    
    for (i=0; i<NODENUM; i++)
    {
        if (m_VolumeRegs[i]) SetNodeValue(i);
    }
}

/*****************************************************************************
 * CMiniportTopologyESS::Init()
 *****************************************************************************
 * Initializes a the miniport.
 */
STDMETHODIMP
CMiniportTopologyESS::
Init
(
    IN      PUNKNOWN        UnknownAdapter,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTTOPOLOGY   Port_
)
{
    PAGED_CODE();

    ASSERT(UnknownAdapter);
    ASSERT(ResourceList);
    ASSERT(Port_);

    //
    // AddRef() is required because we are keeping this pointer.
    //
    m_Port = Port_;
    m_Port->AddRef();
    
    //
    // Get the port event interface.
    //
    m_Port->QueryInterface (IID_IPortEvents, (PVOID *)&m_PortEvents);

    NTSTATUS ntStatus = 
        UnknownAdapter->QueryInterface
        (
            IID_IAdapterCommon,
            (PVOID *) &m_AdapterCommon
        );

    if (NT_SUCCESS(ntStatus))
    {
        ProcessResources(ResourceList);
        
        m_BoardId = m_AdapterCommon->BoardId();
        
        InitTables();
        InitNodeValues();
        InitSpatializer();
        m_AdapterCommon->SetMixerTables((PUCHAR)m_VolumesOut, (PUCHAR)m_VolumesIn);
        m_AdapterCommon->SetRecordingSource(&m_MuxPin);
        m_AdapterCommon->dspWriteMixer(ESM_MIXER_AUDIO2_RECVOL, 0);
        m_AdapterCommon->dspWriteMixer(ESM_MIXER_MIC_PREAMP, 
            m_AdapterCommon->dspReadMixer(ESM_MIXER_MIC_PREAMP) & 8 | m_MonoOutSource);
        m_AdapterCommon->SetPortEventsDest(m_PortEvents);
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportTopologyESS::InitTables()
 *****************************************************************************
 * Initializes the class-internal tables.
 */
VOID
CMiniportTopologyESS::
InitTables
(   void
)
{

    m_LineVolumes[AUX_LINEOUT_VOLUME].Channel[CHAN_LEFT]     = 0xFFFA0B69;
    m_LineVolumes[AUX_LINEOUT_VOLUME].Channel[CHAN_RIGHT]    = 0xFFFA0B69;
    m_LineVolumes[SYNTH_VOLUME].Channel[CHAN_LEFT]           = 0xFFFA0B69;
    m_LineVolumes[SYNTH_VOLUME].Channel[CHAN_RIGHT]          = 0xFFFA0B69;
    m_LineVolumes[CD_VOLUME].Channel[CHAN_LEFT]              = 0xFFFA0B69;
    m_LineVolumes[CD_VOLUME].Channel[CHAN_RIGHT]             = 0xFFFA0B69;
    m_LineVolumes[LINEIN_VOLUME].Channel[CHAN_LEFT]          = 0xFFFA0B69;
    m_LineVolumes[LINEIN_VOLUME].Channel[CHAN_RIGHT]         = 0xFFFA0B69;
    m_LineVolumes[MIC_VOLUME].Channel[CHAN_LEFT]             = 0xFFFA0B69;
    m_LineVolumes[MIC_VOLUME].Channel[CHAN_RIGHT]            = 0xFFFA0B69;
    m_LineVolumes[PC_SPEAKER_VOLUME].Channel[CHAN_LEFT]      = 0xFFFA0B69;
    m_LineVolumes[PC_SPEAKER_VOLUME].Channel[CHAN_RIGHT]     = 0xFFFA0B69;
    m_LineVolumes[PHONE_VOLUME].Channel[CHAN_LEFT]           = 0xFFFA0B69;
    m_LineVolumes[WAVEOUT_VOLUME].Channel[CHAN_LEFT]         = 0xFFFA0B69;
    m_LineVolumes[WAVEOUT_VOLUME].Channel[CHAN_RIGHT]        = 0xFFFA0B69;
    m_LineVolumes[MIDI_IN_VOLUME].Channel[CHAN_LEFT]         = 0xFFFA0B69;
    m_LineVolumes[MIDI_IN_VOLUME].Channel[CHAN_RIGHT]        = 0xFFFA0B69;
    m_LineVolumes[CD_IN_VOLUME].Channel[CHAN_LEFT]           = 0xFFFA0B69;
    m_LineVolumes[CD_IN_VOLUME].Channel[CHAN_RIGHT]          = 0xFFFA0B69;
    m_LineVolumes[LINE_IN_VOLUME].Channel[CHAN_LEFT]         = 0xFFFA0B69;
    m_LineVolumes[LINE_IN_VOLUME].Channel[CHAN_RIGHT]        = 0xFFFA0B69;
    m_LineVolumes[MIC_IN_VOLUME].Channel[CHAN_LEFT]          = 0xFFFA0B69;
    m_LineVolumes[MIC_IN_VOLUME].Channel[CHAN_RIGHT]         = 0xFFFA0B69;
    m_LineVolumes[AUX_IN_VOLUME].Channel[CHAN_LEFT]          = 0xFFFA0B69;
    m_LineVolumes[AUX_IN_VOLUME].Channel[CHAN_RIGHT]         = 0xFFFA0B69;
    m_LineVolumes[IIS_IN_VOLUME].Channel[CHAN_LEFT]          = 0xFFFA0B69;
    m_LineVolumes[IIS_IN_VOLUME].Channel[CHAN_RIGHT]         = 0xFFFA0B69;
    m_LineVolumes[MIXER_VOLUME].Channel[CHAN_LEFT]           = 0xFFFA0B69;
    m_LineVolumes[MIXER_VOLUME].Channel[CHAN_RIGHT]          = 0xFFFA0B69;
    
    m_VolumeRegs[SYNTH_VOLUME]          = ESM_MIXER_FM_VOL;
    m_VolumeRegs[SYNTH_WAVEIN_MUTE]     = ESM_MIXER_FM_VOL;
    m_VolumeRegs[CD_VOLUME]             = ESM_MIXER_AUXA_VOL;
    m_VolumeRegs[CD_LINEOUT_MUTE]       = ESM_MIXER_AUXA_VOL;
    m_VolumeRegs[LINEIN_VOLUME]         = ESM_MIXER_LINE_VOL;
    m_VolumeRegs[LINEIN_LINEOUT_MUTE]   = ESM_MIXER_LINE_VOL;
    m_VolumeRegs[MIC_VOLUME]            = ESM_MIXER_MICMIX_VOL;
    m_VolumeRegs[MIC_LINEOUT_MUTE]      = ESM_MIXER_MICMIX_VOL;
    m_VolumeRegs[AUX_LINEOUT_VOLUME]    = ESM_MIXER_AUXB_VOL;
    m_VolumeRegs[AUX_LINEOUT_MUTE]      = ESM_MIXER_AUXB_VOL;
    m_VolumeRegs[PC_SPEAKER_VOLUME]     = ESM_MIXER_PCSPKR_VOL;
    m_VolumeRegs[PC_SPEAKER_MUTE]       = ESM_MIXER_PCSPKR_VOL;
    m_VolumeRegs[LINEOUT_MIX]           = 0;
    m_VolumeRegs[RECORDING_SOURCE]      = 0;
    m_VolumeRegs[GAIN_3DEFFECT]         = ESM_MIXER_SPATIALIZER_EN;
    m_VolumeRegs[WAVEOUT_VOLUME]        = ESM_MIXER_AUDIO2_VOL;
    m_VolumeRegs[WAVEOUT_MUTE]          = ESM_MIXER_AUDIO2_VOL;
    m_VolumeRegs[LINEOUT_VOL]           = ESM_MIXER_LEFT_MASTER_VOL;
    m_VolumeRegs[LINEOUT_MUTE]          = ESM_MIXER_LEFT_MASTER_VOL;
    m_VolumeRegs[RECMON]                = ESS_CMD_ANALOGCONTROL;
    m_VolumeRegs[IIS_IN_VOLUME]         = ESS_CMD_RECLEVEL;
    m_VolumeRegs[MIXER_VOLUME]          = ESS_CMD_RECLEVEL;
    m_VolumeRegs[MIDI_IN_VOLUME]        = ESM_MIXER_DAC_RECVOL;
    m_VolumeRegs[CD_IN_VOLUME]          = ESM_MIXER_AUXA_RECVOL;
    m_VolumeRegs[LINE_IN_VOLUME]        = ESM_MIXER_LINE_RECVOL;
    m_VolumeRegs[AUX_IN_VOLUME]         = ESM_MIXER_AUXB_RECVOL;
    m_VolumeRegs[MIC_IN_VOLUME]         = ESM_MIXER_MIC_RECVOL;
    m_VolumeRegs[GAIN_20DB]             = ESM_MIXER_MIC_PREAMP;
    m_VolumeRegs[PHONE_VOLUME]          = ESM_MIXER_MONOIN_PLAYMIX;
    
    m_LineVolumes[MIC_LINEOUT_MUTE].Channel[CHAN_LEFT]  = 1;
    m_LineVolumes[PHONE_MUTE].Channel[CHAN_LEFT]        = 0;
    m_LineVolumes[MIDI_IN_VOLUME].Pin                   = 0;
    m_LineVolumes[CD_IN_VOLUME].Pin                     = 0;
    m_LineVolumes[LINE_IN_VOLUME].Pin                   = 0;
    m_LineVolumes[MIC_IN_VOLUME].Pin                    = 1;
    m_LineVolumes[AUX_IN_VOLUME].Pin                    = 0;
    m_LineVolumes[IIS_IN_VOLUME].Pin                    = 0;
    m_LineVolumes[MIXER_VOLUME].Pin                     = 0;
    m_LineVolumes[RECMON].Channel[CHAN_LEFT]            = 0;
    m_LineVolumes[GAIN_3DEFFECT].Channel[CHAN_LEFT]     = 1;
    m_LineVolumes[LINEOUT_VOL].Channel[CHAN_LEFT]       = 0xFFFA1CA0;
    m_LineVolumes[LINEOUT_VOL].Channel[CHAN_RIGHT]      = 0xFFFA1CA0;
    if ( m_BoardId == 6249 )
    {
        m_VolumeRegs[IIS_LINEOUT_VOLUME]    = ESM_MIXER_FM_VOL;
        m_VolumeRegs[IIS_MUTE]              = ESM_MIXER_FM_VOL;
        m_VolumeRegs[IIS_IN_VOLUME]         = ESM_MIXER_DAC_RECVOL;
    }
    else
    {
        m_VolumeRegs[IIS_LINEOUT_VOLUME]    = 0;
        m_VolumeRegs[IIS_MUTE]              = 0;
        m_VolumeRegs[IIS_IN_VOLUME]         = 0;
    }
    
    GetRegistrySettings();
    
    m_VolumesOut[0].Register = m_VolumeRegs[WAVEOUT_VOLUME];
    m_VolumesOut[1].Register = m_VolumeRegs[SYNTH_VOLUME];
    m_VolumesOut[2].Register = m_VolumeRegs[CD_VOLUME];
    m_VolumesOut[3].Register = m_VolumeRegs[LINEIN_VOLUME];
    m_VolumesOut[4].Register = m_VolumeRegs[MIC_VOLUME];
    m_VolumesOut[5].Register = m_VolumeRegs[AUX_LINEOUT_VOLUME];
    m_VolumesOut[6].Register = m_VolumeRegs[IIS_LINEOUT_VOLUME];
    
    m_VolumesIn[0].Register = m_VolumeRegs[MIDI_IN_VOLUME];
    m_VolumesIn[1].Register = m_VolumeRegs[CD_IN_VOLUME];
    m_VolumesIn[2].Register = m_VolumeRegs[LINE_IN_VOLUME];
    m_VolumesIn[3].Register = m_VolumeRegs[MIC_IN_VOLUME];
    m_VolumesIn[4].Register = m_VolumeRegs[AUX_IN_VOLUME];
    m_VolumesIn[5].Register = m_VolumeRegs[IIS_IN_VOLUME];
    m_VolumesIn[6].Register = m_VolumeRegs[MIXER_VOLUME];
    m_VolumesIn[7].Register = m_VolumeRegs[RECMON];
}

/*****************************************************************************
 * CMiniportTopologyESS::GetRegistrySettings
 *****************************************************************************
 * Restores the settings based on settings stored in the registry.
 */
VOID
CMiniportTopologyESS::
GetRegistrySettings
(   void
)
{
    PREGISTRYKEY    DriverKey;
    NTSTATUS        ntStatus;
    DWORD           dwVolume;

   
    // open the driver registry key
    ntStatus = m_Port->NewRegistryKey( &DriverKey,               // IRegistryKey
                                        NULL,                     // OuterUnknown
                                        DriverRegistryKey,        // Registry key type
                                        KEY_ALL_ACCESS,           // Access flags
                                        NULL,                     // ObjectAttributes
                                        0,                        // Create options
                                        NULL );                   // Disposition
    if(NT_SUCCESS(ntStatus))
    {
        UNICODE_STRING  KeyName;
        ULONG           ResultLength;
        UINT            i, j;
        DWORD           Value;
        
        // allocate data to hold key info
        PVOID KeyInfo = ExAllocatePool(PagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + CONFIG_VAL_SIZE);
        if(NULL != KeyInfo)
        {
            // loop through all mixer settings
            for(i = 0; i < SIZEOF_ARRAY(m_RegKeys); i++)
            {
                // init key name
                RtlInitUnicodeString( &KeyName, m_RegKeys[i].KeyName );
                ResultLength = 0;

                // query the value key
                ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                       KeyValuePartialInformation,
                                                       KeyInfo,
                                                       sizeof(KEY_VALUE_PARTIAL_INFORMATION) + CONFIG_VAL_SIZE,
                                                       &ResultLength );
                if(NT_SUCCESS(ntStatus))
                {
                    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                    for (j=0; j<CONFIG_VAL_SIZE; j++)
                        m_RegKeys[i].Data[j] = PartialInfo->Data[j];
                } else
                {
                    // if key access failed, set to default
                    
                    // BUGBUG: In original driver, this is a strcmp and therefore can never match??
                    if (wcscmp(m_RegKeys[i].KeyName, L"MasterVolumeMap") == 0)
                    {
                        for (j=0; j<CONFIG_VAL_SIZE; j++)
                            m_RegKeys[i].Data[j] = (BYTE)j >> 2;
                    }
                    else
                    {
                        for (j=0; j<CONFIG_VAL_SIZE; j++)
                            m_RegKeys[i].Data[j] = (BYTE)j >> 4;
                    }
                }
            }
            
            RtlInitUnicodeString( &KeyName, L"SpatializerLevel" );
            ResultLength = 0;
            
            ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                   KeyValuePartialInformation,
                                                   KeyInfo,
                                                   sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                   &ResultLength );

            if(NT_SUCCESS(ntStatus))
            {
                PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);
                

                Value = *(PDWORD(PartialInfo->Data));
                if (Value > 63) Value = 63;
                m_SpatializerLevel = (UCHAR)Value;
            } else
            {
                m_SpatializerLevel = 63;
            }
            
            RtlInitUnicodeString( &KeyName, L"MuxPin" );
            ResultLength = 0;
            
            ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                   KeyValuePartialInformation,
                                                   KeyInfo,
                                                   sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                   &ResultLength );

            if(NT_SUCCESS(ntStatus))
            {
                PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                m_MuxPin = *(PDWORD(PartialInfo->Data));
            } else
            {
                m_MuxPin = 4;
            }
            
            RtlInitUnicodeString( &KeyName, L"MonoOutSource" );
            ResultLength = 0;
            
            ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                   KeyValuePartialInformation,
                                                   KeyInfo,
                                                   sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                   &ResultLength );

            if(NT_SUCCESS(ntStatus))
            {
                PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                Value = *(PDWORD(PartialInfo->Data));
                if (Value > 3) Value = 0; else Value *= 2;
                m_MonoOutSource = (UCHAR)Value;
            } else
            {
                m_MonoOutSource = 0;
            }

            RtlInitUnicodeString( &KeyName, L"Mic20DBBoost" );
            ResultLength = 0;
            
            ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                   KeyValuePartialInformation,
                                                   KeyInfo,
                                                   sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                   &ResultLength );

            if(NT_SUCCESS(ntStatus))
            {
                PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                m_LineVolumes[GAIN_20DB].Channel[CHAN_LEFT] = *(PDWORD(PartialInfo->Data)) != 0;
            } 

            RtlInitUnicodeString( &KeyName, L"HideMic20DBBoost" );
            ResultLength = 0;
            
            ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                   KeyValuePartialInformation,
                                                   KeyInfo,
                                                   sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                   &ResultLength );

            if(NT_SUCCESS(ntStatus))
            {
                m_HideVolumes[GAIN_20DB] = 1;
            } 
            else
            {
                m_HideVolumes[GAIN_20DB] = 0;
            }
            
            // loop through all mixer settings
            for(i = 0; i < SIZEOF_ARRAY(MixerRegSettings); i++)
            {
                if (MixerRegSettings[i].Channel[CHAN_LEFT])
                {
                    // init key name
                    RtlInitUnicodeString( &KeyName, MixerRegSettings[i].Channel[CHAN_LEFT] );
                    ResultLength = 0;

                    // query the value key
                    ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                           KeyValuePartialInformation,
                                                           KeyInfo,
                                                           sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                           &ResultLength );
                    if(NT_SUCCESS(ntStatus))
                    {
                        PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                        dwVolume = *(PDWORD(PartialInfo->Data));
                        if (MixerRegSettings[i].Channel[CHAN_RIGHT])
                        {
                            // BUGBUG: In original driver, this is a strcmp and therefore can never match??
                            if (wcscmp(MixerRegSettings[i].Channel[CHAN_RIGHT], L"LeftLineoutVol") == 0)
                            {
                                if (dwVolume > 63)
                                    dwVolume = 31;
                                dwVolume = VolLut[dwVolume * 4];
                            }
                            else
                            {
                                if (dwVolume > 15)
                                    dwVolume = 7;
                                dwVolume = VolLut[dwVolume * 16];
                            }
                        }
                    } else
                    {
                        dwVolume = MixerRegSettings[i].Channel[CHAN_RIGHT] ? 0xFFFA0B69 : 0;
                    }
                    m_LineVolumes[i].Channel[CHAN_LEFT] = dwVolume;
                }
                
                if (MixerRegSettings[i].Channel[CHAN_RIGHT])
                {
                    // init key name
                    RtlInitUnicodeString( &KeyName, MixerRegSettings[i].Channel[CHAN_RIGHT] );
                    ResultLength = 0;

                    // query the value key
                    ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                           KeyValuePartialInformation,
                                                           KeyInfo,
                                                           sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                           &ResultLength );
                    if(NT_SUCCESS(ntStatus))
                    {
                        PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                        dwVolume = *(PDWORD(PartialInfo->Data));
                        
                        // BUGBUG: In original driver, this is a strcmp and therefore can never match??
                        if (wcscmp(MixerRegSettings[i].Channel[CHAN_RIGHT], L"RightLineoutVol") == 0)
                        {
                            if (dwVolume > 63)
                                dwVolume = 31;
                            dwVolume = VolLut[dwVolume * 4];
                        }
                        else
                        {
                            if (dwVolume > 15)
                                dwVolume = 7;
                            dwVolume = VolLut[dwVolume * 16];
                        }
                        m_LineVolumes[i].Channel[CHAN_RIGHT] = dwVolume;
                    } else
                    {
                        m_LineVolumes[i].Channel[CHAN_RIGHT] = 0xFFFA0B69;
                    }
                }

                if (MixerRegSettings[i].Hide)
                {
                    // init key name
                    RtlInitUnicodeString( &KeyName, MixerRegSettings[i].Hide );
                    ResultLength = 0;

                    // query the value key
                    ntStatus = DriverKey->QueryValueKey( &KeyName,
                                                           KeyValuePartialInformation,
                                                           KeyInfo,
                                                           sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                           &ResultLength );
                    if(NT_SUCCESS(ntStatus))
                    {
                        PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                        if (*(PDWORD(PartialInfo->Data)))
                            m_HideVolumes[i] = 1;
                    } else
                    {
                        m_HideVolumes[i] = 0;
                    }
                }
            }


            // free the key info
            ExFreePool(KeyInfo);
        }
        // release the driver key
        DriverKey->Release();
    }
}

/*****************************************************************************
 * CMiniportTopologyESS::InitNodeValues()
 *****************************************************************************
 * Initializes Spatializer.
 */
BOOLEAN
CMiniportTopologyESS::
MixerSetMux
(
    DWORD pin
)
{
    unsigned int i;
    
    if (m_MuxPin != pin)
    {
        m_MuxPin = pin;
        switch (pin)
        {
        case 1:
            pin = 18;
            break;
        case 2:
            pin = 19;
            break;
        case 3:
            pin = 20;
            break;
        case 4:
            pin = 21;
            break;
        case 5:
            pin = 23;
            break;
        case 6:
            pin = 24;
            break;
        case 7:
            pin = 25;
            break;
        }
        
        for (i=MIDI_IN_VOLUME; i<=MIXER_VOLUME; i++)
        {
            m_LineVolumes[i].Pin = i == pin;
            SetNodeValue(i);
        }
        
        if (m_MuxPin > 0 && m_MuxPin <= 6)
        {
            m_AdapterCommon->dspWrite(ESS_CMD_ENABLEEXT);
            m_AdapterCommon->dspWrite(ESS_CMD_RECLEVEL);
            m_AdapterCommon->dspWrite(0xFF);
            SetNodeValue(RECMON);
            m_AdapterCommon->dspWriteMixer(ESM_MIXER_EXTENDEDRECSRC, 5); // Record mixer
        }
        else if (m_MuxPin == 7)
        {
            BYTE Reg;
            
            m_AdapterCommon->dspWrite(ESS_CMD_ENABLEEXT);
            m_AdapterCommon->dspWrite(ESS_CMD_READREG);
            m_AdapterCommon->dspWrite(0xA8);
            Reg = m_AdapterCommon->dspRead() & 0xF7;
            m_AdapterCommon->dspWrite(0xA8);
            m_AdapterCommon->dspWrite(Reg);
            m_AdapterCommon->dspWriteMixer(ESM_MIXER_EXTENDEDRECSRC, 7); // Master volume inputs
        }
    }
    return TRUE;
}


/*****************************************************************************
 * CMiniportTopologyESS::GetDescription()
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP
CMiniportTopologyESS::
GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportTopologyESS::GetDescription]"));

    *OutFilterDescriptor = &MiniportFilterDescriptor;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportTopologyESS::SetNodeValue
 *****************************************************************************
 * This is the generic event handler.
 */
VOID
CMiniportTopologyESS::SetNodeValue
(
    IN      ULONG      Node
)
{
    BYTE Volume, PreAmpFlag;
    int VolIdx;
    
    switch(Node)
    {
        case WAVEOUT_VOLUME:
        case SYNTH_VOLUME:
        case CD_VOLUME:
        case LINEIN_VOLUME:
        case MIC_VOLUME:
        case AUX_LINEOUT_VOLUME:
            Volume = (BYTE)(m_LineVolumes[Node].Channel[CHAN_LEFT]?0:GetHwVol(Node));
            for (VolIdx=0; VolIdx<sizeof(m_VolumesOut)/sizeof(m_VolumesOut[0]); VolIdx++)
            {
                if (m_VolumesOut[VolIdx].Register == m_VolumeRegs[Node])
                {
                    m_VolumesOut[VolIdx].Value = Volume;
                    break;
                }
            }
            if ( (m_BoardId < 0x6880 || m_BoardId >= 0x6888)
                && m_BoardId < 0x6888
                && m_BoardId != 0x1878
                && m_BoardId != 0x1868
                || !m_AdapterCommon->IsRecording())
            {
                if ( m_BoardId != 0x1869 || Node != SYNTH_VOLUME || m_AdapterCommon->IsMidiActive())
                    m_AdapterCommon->dspWriteMixer(m_VolumeRegs[Node], Volume);
            }
            return;
        case WAVEOUT_MUTE:
        case SYNTH_WAVEIN_MUTE:
        case CD_LINEOUT_MUTE:
        case LINEIN_LINEOUT_MUTE:
        case MIC_LINEOUT_MUTE:
        case AUX_LINEOUT_MUTE:
            Volume = (BYTE)(m_LineVolumes[Node].Channel[CHAN_LEFT]?0:GetHwVol(Node - 1));
            for (VolIdx=0; VolIdx<sizeof(m_VolumesOut)/sizeof(m_VolumesOut[0]); VolIdx++)
            {
                if (m_VolumesOut[VolIdx].Register == m_VolumeRegs[Node])
                {
                    m_VolumesOut[VolIdx].Value = Volume;
                    break;
                }
            }
            if ( (m_BoardId < 0x6880 || m_BoardId >= 0x6888)
                && m_BoardId < 0x6888
                && m_BoardId != 0x1878
                && m_BoardId != 0x1868
                || !m_AdapterCommon->IsRecording())
            {
                if ( m_BoardId != 0x1869 || Node != SYNTH_WAVEIN_MUTE || m_AdapterCommon->IsMidiActive())
                    m_AdapterCommon->dspWriteMixer(m_VolumeRegs[Node], Volume);
            }
            return;
        case IIS_LINEOUT_VOLUME:
        case IIS_MUTE:
            if ( m_BoardId == 0x1878 || m_BoardId == 0x1869 || m_BoardId == 0x1879 )
            {
                Volume = (BYTE)(m_LineVolumes[IIS_MUTE].Channel[CHAN_LEFT]?0:GetHwVol(IIS_LINEOUT_VOLUME));
                m_VolumesOut[6].Value = Volume;
                if ( m_BoardId != 0x1878 || !m_AdapterCommon->IsRecording() )
                {
                    if ( m_BoardId != 0x1869 || m_AdapterCommon->IsMidiActive())
                        m_AdapterCommon->dspWriteMixer(m_VolumeRegs[Node], Volume);
                }
            }
            return;
        case PC_SPEAKER_VOLUME:
        case PC_SPEAKER_MUTE:
            if ( m_BoardId < 0x6880 || m_BoardId >= 0x6888 )
            {
                Volume = m_LineVolumes[PC_SPEAKER_MUTE].Channel[CHAN_LEFT]?0:(BYTE)(GetHwVol(PC_SPEAKER_VOLUME) / 32);
                return m_AdapterCommon->dspWriteMixer(m_VolumeRegs[Node], Volume);
            }
            return;
        case PHONE_VOLUME:
            if ( m_LineVolumes[PHONE_MUTE].Channel[CHAN_LEFT] == 0 )
            {
                Volume = (BYTE)GetHwVol(PHONE_VOLUME);
                m_AdapterCommon->dspWriteMixer(ESM_MIXER_MONOIN_RECVOL, Volume);
                m_AdapterCommon->dspWriteMixer(ESM_MIXER_MONOIN_PLAYMIX, Volume);
                
            }
            return;
        case PHONE_MUTE:
            PreAmpFlag = m_AdapterCommon->dspReadMixer(ESM_MIXER_MIC_PREAMP);
            if ( m_LineVolumes[PHONE_MUTE].Channel[CHAN_LEFT] == 0 )
            {
                Volume = 0;
            }
            else
            {
                m_MicPreampGain = (PreAmpFlag & 8) != 0;
                Volume = (BYTE)GetHwVol(PHONE_VOLUME);
            }
            m_AdapterCommon->dspWriteMixer(ESM_MIXER_MONOIN_RECVOL, Volume);
            m_AdapterCommon->dspWriteMixer(ESM_MIXER_MONOIN_PLAYMIX, Volume);
            m_AdapterCommon->dspWriteMixer(ESM_MIXER_MIC_PREAMP, Volume);
            return;
        case MIDI_IN_VOLUME:
        case CD_IN_VOLUME:
        case LINE_IN_VOLUME:
        case MIC_IN_VOLUME:
        case AUX_IN_VOLUME:
            Volume = m_LineVolumes[Node].Pin?(BYTE)GetHwVol(Node):0;
            for (VolIdx=0; VolIdx<sizeof(m_VolumesIn)/sizeof(m_VolumesIn[0]); VolIdx++)
            {
                if (m_VolumesIn[VolIdx].Register == m_VolumeRegs[Node])
                {
                    m_VolumesIn[VolIdx].Value = Volume;
                    break;
                }
            }           
            if ( m_BoardId != 0x1869 || Node != MIDI_IN_VOLUME || m_AdapterCommon->IsMidiActive())
                m_AdapterCommon->dspWriteMixer(m_VolumeRegs[Node], Volume);
            return;
        case GAIN_20DB:
            PreAmpFlag = m_AdapterCommon->dspReadMixer(ESM_MIXER_MIC_PREAMP);
            if ( m_LineVolumes[GAIN_20DB].Channel[CHAN_LEFT] == 0 )
                PreAmpFlag &= ~8;
            else
                PreAmpFlag |= 8;
            m_AdapterCommon->dspWriteMixer(ESM_MIXER_MIC_PREAMP, PreAmpFlag);
            return;
        case IIS_IN_VOLUME:
            if ( m_BoardId == 0x1878 || m_BoardId == 0x1869 || m_BoardId == 0x1879 )
            {
                Volume = m_LineVolumes[IIS_IN_VOLUME].Pin?(BYTE)GetHwVol(IIS_IN_VOLUME):0;
                m_VolumesIn[5].Value = Volume;
                if ( m_AdapterCommon->IsRecording() && (m_BoardId != 0x1869 || !m_AdapterCommon->IsMidiActive()) )
                        m_AdapterCommon->dspWriteMixer(m_VolumeRegs[Node], Volume);
            }
            return;
        case MIXER_VOLUME:
            if ( m_MuxPin == 7 )
            {
                if ( m_LineVolumes[MIXER_VOLUME].Pin )
                {
                    Volume = (BYTE)GetHwVol(MIXER_VOLUME);
                    Volume = (Volume<<4) | (Volume>>4);
                }
                else Volume = 0;
                m_VolumesIn[6].Value = Volume;
                if (m_AdapterCommon->IsRecording())
                {
                    m_AdapterCommon->dspWrite(ESS_CMD_RECLEVEL);
                    m_AdapterCommon->dspWrite(Volume);
                }
            }
            return;
        case LINEOUT_VOL:
            if ( m_LineVolumes[LINEOUT_MUTE].Channel[CHAN_LEFT] == 0 )
            {
                if ( (m_BoardId < 0x6880 || m_BoardId >= 0x6888) && m_BoardId < 0x6888)
                {
                    m_AdapterCommon->dspWriteMixer(m_VolumeRegs[LINEOUT_VOL] + 2, 
                        GetMasterHwVol(m_LineVolumes[LINEOUT_VOL].Channel[CHAN_RIGHT]));
                    Volume = GetMasterHwVol(m_LineVolumes[LINEOUT_VOL].Channel[CHAN_LEFT]);
                }
                else Volume = (BYTE)GetHwVol(LINEOUT_VOL);
                m_AdapterCommon->dspWriteMixer(m_VolumeRegs[LINEOUT_VOL], Volume);
            }
            return;
        case LINEOUT_MUTE:
            if ( (m_BoardId < 0x6880 || m_BoardId >= 0x6888) && m_BoardId < 0x6888)
            {
                Volume = GetMasterHwVol(m_LineVolumes[LINEOUT_VOL].Channel[CHAN_RIGHT]);
                if (m_LineVolumes[LINEOUT_MUTE].Channel[CHAN_LEFT])
                {
                    m_AdapterCommon->dspWriteMixer(m_VolumeRegs[LINEOUT_MUTE] + 2, Volume | 0x40);
                    Volume = GetMasterHwVol(m_LineVolumes[LINEOUT_VOL].Channel[CHAN_LEFT]) | 0x40;
                }
                else
                {
                    m_AdapterCommon->dspWriteMixer(m_VolumeRegs[LINEOUT_MUTE] + 2, Volume);
                    Volume = GetMasterHwVol(m_LineVolumes[LINEOUT_VOL].Channel[CHAN_LEFT]);
                }
            }
            else
            {
                Volume = m_LineVolumes[LINEOUT_MUTE].Channel[CHAN_LEFT]?0:(BYTE)GetHwVol(LINEOUT_VOL);
            }
            m_AdapterCommon->SetMute(m_LineVolumes[LINEOUT_MUTE].Channel[CHAN_LEFT] != 0);
            m_AdapterCommon->dspWriteMixer(m_VolumeRegs[Node], Volume);
            return;
        case GAIN_3DEFFECT:
            if ( m_BoardId == 0x1869 || m_BoardId == 0x1879 )
            {
                Volume = m_LineVolumes[GAIN_3DEFFECT].Channel[CHAN_LEFT]?12:4;
                m_AdapterCommon->dspWriteMixer(m_VolumeRegs[Node], Volume);
            }
            return;
        case RECMON:
            if (m_AdapterCommon->IsRecording())
            {
                if (m_MuxPin == 7) return;
                m_AdapterCommon->dspWrite(ESS_CMD_READREG);
                m_AdapterCommon->dspWrite(ESS_CMD_ANALOGCONTROL);
                PreAmpFlag = m_AdapterCommon->dspRead();
                if ( m_LineVolumes[RECMON].Channel[CHAN_LEFT] )
                    PreAmpFlag |= 8;
                else
                    PreAmpFlag &= ~8;
                m_AdapterCommon->dspWrite(ESS_CMD_ANALOGCONTROL);
                m_AdapterCommon->dspWrite(PreAmpFlag);
            }
            m_VolumesIn[LINEIN_LINEOUT_MUTE].Value = m_LineVolumes[RECMON].Channel[CHAN_LEFT] != 0;
            return;
    }
}

/*****************************************************************************
 * CMiniportTopologyESS::GetHwVol
 *****************************************************************************
 * Get Hardware Volume
 */
USHORT
CMiniportTopologyESS::
GetHwVol
(
    IN      ULONG      Node
)
{
    int Volume;
    BYTE resultLeft, resultRight;
    
    Volume = (int)m_LineVolumes[Node].Channel[CHAN_LEFT];
    if ( Volume > (int)VolLut[0] )
    {
        if ( Volume < 0 )
        {
            for (resultLeft = 0; Volume > (int)VolLut[resultLeft]; resultLeft++)
            {
                if (resultLeft >= 256)
                {
                    resultLeft = (BYTE)Node;
                    break;
                }
            }
        }
        else resultLeft = (BYTE)-1;
    } else resultLeft = 0;
    
    Volume = (int)m_LineVolumes[Node].Channel[CHAN_RIGHT];
    if ( Volume > (int)VolLut[0] )
    {
        if ( Volume < 0 )
        {
            for (resultRight = 0; Volume > (int)VolLut[resultRight]; resultRight++)
            {
                if (resultRight >= 256)
                {
                    resultRight = (BYTE)Node;
                    break;
                }
            }
        }
        else resultRight = (BYTE)-1;
    } else resultRight = 0;

    switch (Node)
    {
        case MIDI_IN_VOLUME:
            resultLeft = SynthVolumeInMap[resultLeft];
            resultRight = SynthVolumeInMap[resultRight];
            break;
        case CD_IN_VOLUME:
            resultLeft = CDAudioVolumeInMap[resultLeft];
            resultRight = CDAudioVolumeInMap[resultRight];
            break;
        case LINE_IN_VOLUME:
            resultLeft = LineInVolumeInMap[resultLeft];
            resultRight = LineInVolumeInMap[resultRight];
            break;
        case MIC_IN_VOLUME:
            resultLeft = MicVolumeInMap[resultLeft];
            resultRight = MicVolumeInMap[resultRight];
            break;
        case AUX_IN_VOLUME:
            resultLeft = AuxBVolumeInMap[resultLeft];
            resultRight = AuxBVolumeInMap[resultRight];
            break;
        case IIS_IN_VOLUME:
            resultLeft = IISVolumeInMap[resultLeft];
            resultRight = IISVolumeInMap[resultRight];
            break;
        case MIXER_VOLUME:
            resultLeft = MainRecordVolumeMap[resultLeft];
            resultRight = MainRecordVolumeMap[resultRight];
            break;
        case PHONE_VOLUME:
            resultLeft = PhoneVolumeOutMap[resultLeft];
            resultRight = PhoneVolumeOutMap[resultRight];
            break;
        case SYNTH_VOLUME:
            resultLeft = SynthVolumeOutMap[resultLeft];
            resultRight = SynthVolumeOutMap[resultRight];
            break;
        case CD_VOLUME:
            resultLeft = CDAudioVolumeOutMap[resultLeft];
            resultRight = CDAudioVolumeOutMap[resultRight];
            break;
        case LINEIN_VOLUME:
            resultLeft = LineInVolumeOutMap[resultLeft];
            resultRight = LineInVolumeOutMap[resultRight];
            break;
        case MIC_VOLUME:
            resultLeft = MicVolumeOutMap[resultLeft];
            resultRight = MicVolumeOutMap[resultRight];
            break;
        case AUX_LINEOUT_VOLUME:
            resultLeft = AuxBVolumeOutMap[resultLeft];
            resultRight = AuxBVolumeOutMap[resultRight];
            break;
        case IIS_LINEOUT_VOLUME:
            resultLeft = IISVolumeOutMap[resultLeft];
            resultRight = IISVolumeOutMap[resultRight];
            break;
        case WAVEOUT_VOLUME:
            resultLeft = WaveVolumeOutMap[resultLeft];
            resultRight = WaveVolumeOutMap[resultRight];
            break;
    }
    
    return (USHORT)resultRight | ((USHORT)resultLeft << 4);
}

/*****************************************************************************
 * CMiniportTopologyESS::GetMasterHwVol
 *****************************************************************************
 * Get Master Hardware Volume
 */
BYTE
CMiniportTopologyESS::
GetMasterHwVol
(
    IN      int      Volume
)
{
    int i;
    
    if ( Volume > (int)VolLut[0])
    {
        if ( Volume > 255 ) return (BYTE)-1;
        for (i = 0; Volume > (int)VolLut[i]; i++)
        {
            if (i > 255)
                return MasterVolumeMap[HIBYTE(Volume)];
        }
        return MasterVolumeMap[i];
    }
    return MasterVolumeMap[0];
}

/*****************************************************************************
 * CMiniportTopologyESS::GetMuteFromHardware
 *****************************************************************************
 * Get Mute status to m_LineVolumes
 */
VOID
CMiniportTopologyESS::
GetMuteFromHardware
(
    IN      ULONG      Node
)
{
    if ( Node == LINEOUT_MUTE )
        m_LineVolumes[LINEOUT_MUTE].Channel[CHAN_LEFT] = (m_AdapterCommon->dspReadMixer(m_VolumeRegs[LINEOUT_VOL]) >> 6) & 1;
}

/*****************************************************************************
 * CMiniportTopologyESS::GetVolumeFromHardware
 *****************************************************************************
 * Get Lineout Volume from Hardware 
 */
VOID
CMiniportTopologyESS::
GetVolumeFromHardware
(
    IN      ULONG      Node
)
{
    BYTE MasterVolume;
    int i;
    
    if ( Node == LINEOUT_VOL )
    {
        MasterVolume = m_AdapterCommon->dspReadMixer(m_VolumeRegs[LINEOUT_VOL]) & (~0x40);
        if (MasterVolume)
        {
            for (i=255; MasterVolume < MasterVolumeMap[i]; i--)
                if (i<0) break;
            if (i>=0) m_LineVolumes[LINEOUT_VOL].Channel[CHAN_LEFT] = VolLut[i];
        }
        else
            m_LineVolumes[LINEOUT_VOL].Channel[CHAN_LEFT] = VolLut[0];
        
        MasterVolume = m_AdapterCommon->dspReadMixer(m_VolumeRegs[LINEOUT_VOL] + 2) & (~0x40);
        if (MasterVolume)
        {
            for (i=255; MasterVolume < MasterVolumeMap[i]; i--)
                if (i<0) break;
            if (i>=0) m_LineVolumes[LINEOUT_VOL].Channel[CHAN_RIGHT] = VolLut[i];
        }        
        else
            m_LineVolumes[LINEOUT_VOL].Channel[CHAN_RIGHT] = VolLut[0];
    }
}

/*****************************************************************************
 * CMiniportTopologyESS::PowerChangeNotify()
 *****************************************************************************
 * Handle power state change for the miniport.
 */
STDMETHODIMP_(void)
CMiniportTopologyESS::
PowerChangeNotify
(
    IN      POWER_STATE             NewState
)
{
    int i;
    
    PAGED_CODE();
    
    if ( m_PowerState != NewState.SystemState )
    {
        if ( NewState.SystemState == PowerSystemWorking )
        {
            for ( i = 0; i < NODENUM; ++i )
                SetNodeValue(i);
            m_PowerState = PowerSystemWorking;
            return;
        }
    }
    m_PowerState = NewState.SystemState;
}

/*****************************************************************************
 * CMiniportTopologyESS::SaveRegistrySettings
 *****************************************************************************
 * Save the MuxPin Setting to the driver's registry key
 */
VOID
CMiniportTopologyESS::
SaveRegistrySettings
(   void
)
{
    PREGISTRYKEY    DriverKey;
    NTSTATUS        ntStatus;
   
    // open the driver registry key
    ntStatus = m_Port->NewRegistryKey( &DriverKey,               // IRegistryKey
                                        NULL,                     // OuterUnknown
                                        DriverRegistryKey,        // Registry key type
                                        KEY_ALL_ACCESS,           // Access flags
                                        NULL,                     // ObjectAttributes
                                        0,                        // Create options
                                        NULL );                   // Disposition
    if(NT_SUCCESS(ntStatus))
    {
        UNICODE_STRING  KeyName;
        
        RtlInitUnicodeString( &KeyName, L"MuxPin" );
        
        // query the value key
        DriverKey->SetValueKey ( &KeyName,
                                   REG_DWORD,
                                   &m_MuxPin,
                                   sizeof (DWORD));
        
        DriverKey->Release();
    }
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportTopologyESS::EventHandler
 *****************************************************************************
 * This is the generic event handler.
 */
NTSTATUS CMiniportTopologyESS::EventHandler
(
    IN      PPCEVENT_REQUEST      EventRequest
)
{
    ASSERT(EventRequest);

    _DbgPrintF (DEBUGLVL_VERBOSE, ("CMiniportTopologyESS::EventHandler"));

    // The major target is the object pointer to the topology miniport.
    CMiniportTopologyESS *that =
        (CMiniportTopologyESS *)(PMINIPORTTOPOLOGY(EventRequest->MajorTarget));

    ASSERT (that);

    // What is to do?
    switch (EventRequest->Verb)
    {
        // Do we support event handling?!?
        case PCEVENT_VERB_SUPPORT:
            _DbgPrintF (DEBUGLVL_VERBOSE, ("BasicSupport Query for Event."));
            return STATUS_SUCCESS;

        // We should add the event now!
        case PCEVENT_VERB_ADD:
            _DbgPrintF (DEBUGLVL_VERBOSE, ("Adding Event."));

            // If we have the interface and EventEntry is defined ...
            if ((EventRequest->EventEntry) && (that->m_PortEvents))
            {
                that->m_PortEvents->AddEventToEventList (EventRequest->EventEntry);
                return STATUS_SUCCESS;
            }
            break;

        case PCEVENT_VERB_REMOVE:
            // We cannot remove the event but we can stop generating the
            // events. However, it also doesn't hurt to always generate them ...
            _DbgPrintF (DEBUGLVL_VERBOSE, ("Removing Event."));
            break;
    }

    return STATUS_UNSUCCESSFUL;
}


/*****************************************************************************
 * PropertyHandler_OnOff()
 *****************************************************************************
 * Accesses a KSAUDIO_ONOFF value property.
 */
static
NTSTATUS
PropertyHandler_OnOff
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PropertyHandler_OnOff]"));

    CMiniportTopologyESS *that =
        (CMiniportTopologyESS *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    LONG            channel;

    // validate node
    if (PropertyRequest->Node != ULONG(-1) && !that->m_HideVolumes[PropertyRequest->Node])
    {
        if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            // get the instance channel parameter
            if(PropertyRequest->InstanceSize >= sizeof(LONG))
            {
                channel = *(PLONG(PropertyRequest->Instance));

                // validate and get the output parameter
                if (PropertyRequest->ValueSize >= sizeof(BOOL))
                {
                    PBOOL OnOff = PBOOL(PropertyRequest->Value);
    
                    // switch on node id
                    switch(PropertyRequest->Node)
                    {
                        case PC_SPEAKER_MUTE:
                            if ( that->m_BoardId >= 0x6880 && that->m_BoardId < 0x6888 )
                                return STATUS_INVALID_PARAMETER;
                        case MIC_LINEOUT_MUTE:
                        case PHONE_MUTE:                             
                            // check if AGC property request on mono/left channel
                            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE ) &&
                                ( channel == CHAN_LEFT ) )
                            {
                                *OnOff = that->m_LineVolumes[PropertyRequest->Node].Channel[CHAN_LEFT] != 0;
                                PropertyRequest->ValueSize = sizeof(BOOL);
                                ntStatus = STATUS_SUCCESS;
                            }
                            break;
                        case GAIN_3DEFFECT:
                            if ( that->m_BoardId != 0x1869 && that->m_BoardId != 0x1879 )
                                return STATUS_INVALID_PARAMETER;
                        case GAIN_20DB:
                        case RECMON:
                            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_AGC ) &&
                                ( channel == CHAN_LEFT ) )
                            {
                                *OnOff = that->m_LineVolumes[PropertyRequest->Node].Channel[CHAN_LEFT] != 0;
                                PropertyRequest->ValueSize = sizeof(BOOL);
                                ntStatus = STATUS_SUCCESS;
                            }
                            break;   
                        case WAVEOUT_MUTE:
                        case SYNTH_WAVEIN_MUTE:
                        case CD_LINEOUT_MUTE:
                        case LINEIN_LINEOUT_MUTE:
                        case AUX_LINEOUT_MUTE:
                        case IIS_MUTE:
                        case LINEOUT_MUTE:
                            // check if MUTE property request on mono/left channel
                            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE ) )
                            {
                                if ( PropertyRequest->Node == LINEOUT_MUTE )
                                {
                                    that->m_AdapterCommon->SetHardwareVolumeFlag(1);
                                    that->GetMuteFromHardware(PropertyRequest->Node);
                                    that->m_AdapterCommon->SetMute(that->m_LineVolumes[PropertyRequest->Node].Channel[CHAN_LEFT] != 0);
                                }

                                ntStatus = STATUS_SUCCESS;
                                *OnOff = that->m_LineVolumes[PropertyRequest->Node].Channel[CHAN_LEFT] != 0;
                                PropertyRequest->ValueSize = sizeof(BOOL);
                            }
                            break;
                    }
                }
            }

        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            // get the instance channel parameter
            if(PropertyRequest->InstanceSize >= sizeof(LONG))
            {
                channel = *(PLONG(PropertyRequest->Instance));
                
                // validate and get the input parameter
                if (PropertyRequest->ValueSize == sizeof(BOOL))
                {
                    // switch on the node id
                    switch(PropertyRequest->Node)
                    {
                        case GAIN_3DEFFECT:
                            if ( that->m_BoardId != 0x1869 && that->m_BoardId != 0x1879 )
                                break;
                        case GAIN_20DB:
                        case RECMON:
                            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_AGC ) &&
                                ( channel == CHAN_LEFT ) )
                            {
                                that->m_LineVolumes[PropertyRequest->Node].Channel[CHAN_LEFT] = *(PBOOL(PropertyRequest->Value));
                                that->SetNodeValue(PropertyRequest->Node);
                                ntStatus = STATUS_SUCCESS;
                            }
                            break;
                        case PC_SPEAKER_MUTE:
                            if ( that->m_BoardId >= 0x6880 && that->m_BoardId < 0x6888 )
                                break;
                        case MIC_LINEOUT_MUTE:
                        case PHONE_MUTE:
                            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE ) &&
                                ( channel == CHAN_LEFT ) )
                            {
                                that->m_LineVolumes[PropertyRequest->Node].Channel[CHAN_LEFT] = *(PBOOL(PropertyRequest->Value));
                                that->SetNodeValue(PropertyRequest->Node);
                                ntStatus = STATUS_SUCCESS;
                            }
                            break;
                        case WAVEOUT_MUTE:
                        case SYNTH_WAVEIN_MUTE:
                        case CD_LINEOUT_MUTE:
                        case LINEIN_LINEOUT_MUTE:
                        case AUX_LINEOUT_MUTE:
                        case IIS_MUTE:
                        case LINEOUT_MUTE:
                            // check if MUTE property request on mono/left channel
                            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE ) )
                            {
                                that->m_LineVolumes[PropertyRequest->Node].Channel[CHAN_LEFT] = *(PBOOL(PropertyRequest->Value));
                                that->SetNodeValue(PropertyRequest->Node);
                                ntStatus = STATUS_SUCCESS;
                            }
                            break;
                    }
                }
            }
        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            if ( ( (PropertyRequest->Node == RECMON) && (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_AGC) && 
                   (that->m_BoardId < 0x6880 || that->m_BoardId >= 0x6888)) ||
                 ( (PropertyRequest->Node == GAIN_3DEFFECT) && (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_AGC) &&
                    (that->m_BoardId == 0x1869 || that->m_BoardId == 0x1879))  ||
                 ( (PropertyRequest->Node == PC_SPEAKER_MUTE || PropertyRequest->Node == WAVEOUT_MUTE || 
                    PropertyRequest->Node == SYNTH_WAVEIN_MUTE || PropertyRequest->Node == CD_LINEOUT_MUTE ||
                    PropertyRequest->Node == LINEIN_LINEOUT_MUTE || PropertyRequest->Node == MIC_LINEOUT_MUTE ||
                    PropertyRequest->Node == IIS_MUTE
                   ) && (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE) )                    
               )
            {
                if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
                {
                    // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
                    PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

                    PropDesc->AccessFlags       = KSPROPERTY_TYPE_BASICSUPPORT |
                                                  KSPROPERTY_TYPE_GET |
                                                  KSPROPERTY_TYPE_SET;
                    PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION);
                    PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
                    PropDesc->PropTypeSet.Id    = VT_BOOL;
                    PropDesc->PropTypeSet.Flags = 0;
                    PropDesc->MembersListCount  = 0;
                    PropDesc->Reserved          = 0;

                    // set the return value size
                    PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
                    ntStatus = STATUS_SUCCESS;
                } else if(PropertyRequest->ValueSize >= sizeof(ULONG))
                {
                    // if return buffer can hold a ULONG, return the access flags
                    PULONG AccessFlags = PULONG(PropertyRequest->Value);
            
                    *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                   KSPROPERTY_TYPE_GET |
                                   KSPROPERTY_TYPE_SET;
            
                    // set the return value size
                    PropertyRequest->ValueSize = sizeof(ULONG);
                    ntStatus = STATUS_SUCCESS;                    
                }               
            }
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * BasicSupportHandler()
 *****************************************************************************
 * Assists in BASICSUPPORT accesses on level properties
 */
static
NTSTATUS
PropertyHandlerBasicSupportVolume
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
    {
        // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
        PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

        PropDesc->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                KSPROPERTY_TYPE_GET |
                                KSPROPERTY_TYPE_SET;
        PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) +
                                      sizeof(KSPROPERTY_MEMBERSHEADER) +
                                      sizeof(KSPROPERTY_STEPPING_LONG);
        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        PropDesc->PropTypeSet.Id    = VT_I4;
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 1;
        PropDesc->Reserved          = 0;

        // if return buffer cn also hold a range description, return it too
        if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION) +
                                      sizeof(KSPROPERTY_MEMBERSHEADER) +
                                      sizeof(KSPROPERTY_STEPPING_LONG)))
        {
            // fill in the members header
            PKSPROPERTY_MEMBERSHEADER Members = PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

            Members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            Members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            Members->MembersCount   = 1;
            Members->Flags          = 0;

            // fill in the stepped range
            PKSPROPERTY_STEPPING_LONG Range = PKSPROPERTY_STEPPING_LONG(Members + 1);

            switch(PropertyRequest->Node)
            {
                case WAVEOUT_VOLUME:
                case SYNTH_VOLUME:
                case CD_VOLUME:
                case LINEIN_VOLUME:
                case MIC_VOLUME:
                case AUX_LINEOUT_VOLUME:
                case IIS_LINEOUT_VOLUME:
                case PC_SPEAKER_VOLUME:
                case PHONE_VOLUME:
                case MIDI_IN_VOLUME:
                case CD_IN_VOLUME:
                case LINE_IN_VOLUME:
                case MIC_IN_VOLUME:
                case AUX_IN_VOLUME:
                case IIS_IN_VOLUME:
                case MIXER_VOLUME:
                case LINEOUT_VOL:
                    Range->Bounds.SignedMaximum = 0;
                    Range->Bounds.SignedMinimum = 0xFFA08000;
                    Range->SteppingDelta        = 0x18000;
                    break;
            }
            Range->Reserved         = 0;

            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION) +
                                         sizeof(KSPROPERTY_MEMBERSHEADER) +
                                         sizeof(KSPROPERTY_STEPPING_LONG);
        } else
        {
            // set the return value size
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
        }
        ntStatus = STATUS_SUCCESS;

    } else if(PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        PULONG AccessFlags = PULONG(PropertyRequest->Value);

        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                       KSPROPERTY_TYPE_GET |
                       KSPROPERTY_TYPE_SET;

        // set the return value size
        PropertyRequest->ValueSize = sizeof(ULONG);
        ntStatus = STATUS_SUCCESS;

    }

    return ntStatus;
}

/*****************************************************************************
 * PropertyHandler_Level()
 *****************************************************************************
 * Accesses a KSAUDIO_LEVEL property.
 */
static
NTSTATUS
PropertyHandler_Level
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    CMiniportTopologyESS *that =
        (CMiniportTopologyESS *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    LONG            channel;

    // validate node
    if(PropertyRequest->Node != ULONG(-1) && that->m_HideVolumes[PropertyRequest->Node] == 0)
    {
        if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            // get the instance channel parameter
            if(PropertyRequest->InstanceSize >= sizeof(LONG))
            {
                channel = *(PLONG(PropertyRequest->Instance));

                // only support get requests on either mono/left (0) or right (1) channels
                if ( (channel == CHAN_LEFT) || (channel == CHAN_RIGHT) )
                {
                    // validate and get the output parameter
                    if (PropertyRequest->ValueSize >= sizeof(LONG))
                    {
                        PLONG Level = (PLONG)PropertyRequest->Value;

                        // switch on node if
                        switch(PropertyRequest->Node)
                        {
                            case PC_SPEAKER_VOLUME:
                                if (that->m_BoardId >= 0x6880 && that->m_BoardId < 0x6888)
                                    break;
                            case MIC_VOLUME:
                            case PHONE_VOLUME:
                            case MIC_IN_VOLUME:
                                if (channel != CHAN_LEFT) break;
                            case WAVEOUT_VOLUME:
                            case SYNTH_VOLUME:
                            case CD_VOLUME:
                            case LINEIN_VOLUME:
                            case AUX_LINEOUT_VOLUME:
                            case IIS_LINEOUT_VOLUME:
                            case MIDI_IN_VOLUME:
                            case CD_IN_VOLUME:
                            case LINE_IN_VOLUME:
                            case AUX_IN_VOLUME:
                            case IIS_IN_VOLUME:
                            case MIXER_VOLUME:
                            case LINEOUT_VOL:
                                // check if volume property request
                                if(PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                                {
                                    if (that->m_AdapterCommon->GetHardwareVolumeFlag(2) && PropertyRequest->Node == LINEOUT_VOL)
                                    {
                                        that->m_AdapterCommon->SetHardwareVolumeFlag(2);
                                        that->GetVolumeFromHardware(LINEOUT_VOL);
                                    }
                                    *Level = that->m_LineVolumes[PropertyRequest->Node].Channel[channel];
                                    
                                    PropertyRequest->ValueSize = sizeof(LONG);
                                    ntStatus = STATUS_SUCCESS;
                                }
                                break;       
                        }
                    }
                }
            }

        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            // get the instance channel parameter
            if(PropertyRequest->InstanceSize >= sizeof(LONG))
            {
                channel = *(PLONG(PropertyRequest->Instance));

                // only support set requests on either mono/left (0), right (1) channels
                if ( (channel == CHAN_LEFT) || (channel == CHAN_RIGHT))
                {
                    // validate and get the input parameter
                    if (PropertyRequest->ValueSize == sizeof(LONG))
                    {
                        PLONG Level = (PLONG)PropertyRequest->Value;

                        // switch on the node id
                        switch(PropertyRequest->Node)
                        {
                            case PC_SPEAKER_VOLUME:
                                if (that->m_BoardId >= 0x6880 && that->m_BoardId < 0x6888)
                                    break;                           
                            case MIC_VOLUME:
                            case PHONE_VOLUME:
                            case MIC_IN_VOLUME:
                                if (channel != CHAN_LEFT) break;
                            case WAVEOUT_VOLUME:
                            case SYNTH_VOLUME:
                            case CD_VOLUME:
                            case LINEIN_VOLUME:
                            case AUX_LINEOUT_VOLUME:
                            case IIS_LINEOUT_VOLUME:
                            case MIDI_IN_VOLUME:
                            case CD_IN_VOLUME:
                            case LINE_IN_VOLUME:
                            case AUX_IN_VOLUME:
                            case IIS_IN_VOLUME:
                            case MIXER_VOLUME:
                            case LINEOUT_VOL:
                                if(PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                                {
                                    that->m_LineVolumes[PropertyRequest->Node].Channel[channel] = *Level;
                                    that->SetNodeValue(PropertyRequest->Node);
                                    ntStatus = STATUS_SUCCESS;
                                }
                                break;
                        }
                    }
                }
            }

        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            // service basic support request
            switch(PropertyRequest->Node)
            {
                case PC_SPEAKER_VOLUME:
                    if (that->m_BoardId >= 0x6880 && that->m_BoardId < 0x6888)
                        break;                           
                case WAVEOUT_VOLUME:
                case SYNTH_VOLUME:
                case CD_VOLUME:
                case LINEIN_VOLUME:
                case MIC_VOLUME:
                case AUX_LINEOUT_VOLUME:
                case IIS_LINEOUT_VOLUME:
                case PHONE_VOLUME:
                case MIDI_IN_VOLUME:
                case CD_IN_VOLUME:
                case LINE_IN_VOLUME:
                case MIC_IN_VOLUME:
                case AUX_IN_VOLUME:
                case IIS_IN_VOLUME:
                case MIXER_VOLUME:
                case LINEOUT_VOL:
                    if(PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                    {
                        ntStatus = PropertyHandlerBasicSupportVolume(PropertyRequest);
                    }
                    break;
            }
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * PropertyHandler_MuxSource()
 *****************************************************************************
 * Handles supermixer caps accesses
 */
static
NTSTATUS
PropertyHandler_MuxSource
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    CMiniportTopologyESS *that =
        (CMiniportTopologyESS *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;

    // validate node
    if(PropertyRequest->Node != ULONG(-1))
    {
        if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUX_SOURCE ) &&
                (  PropertyRequest->ValueSize >= sizeof(ULONG) ) )
            {
                *(PULONG)PropertyRequest->Value = that->m_MuxPin;
                ntStatus = STATUS_SUCCESS;
            }
        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_SET &&
                ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUX_SOURCE ) &&
                (  PropertyRequest->ValueSize == sizeof(ULONG) ) &&
                ( that->MixerSetMux(*(DWORD *)PropertyRequest->Value ) ) )
        {
                that->SaveRegistrySettings();
                ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * PropertyHandler_CpuResources()
 *****************************************************************************
 * Processes a KSPROPERTY_AUDIO_CPU_RESOURCES request
 */
static
NTSTATUS
PropertyHandler_CpuResources
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    // validate node
    if(PropertyRequest->Node != ULONG(-1))
    {
        if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            if(PropertyRequest->ValueSize >= sizeof(LONG))
            {
                *(PLONG(PropertyRequest->Value)) = KSAUDIO_CPU_RESOURCES_HOST_CPU;
                PropertyRequest->ValueSize = sizeof(LONG);
                ntStatus = STATUS_SUCCESS;
            } else
            {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }
        }
    }

    return ntStatus;
}

