/*****************************************************************************
 * tables.h - ESS topology miniport tables
 *****************************************************************************
 * Copyright (c) 2023 leecher@dose.0wnz.at  All rights reserved.
 */

#ifndef _SB16TOPO_TABLES_H_
#define _SB16TOPO_TABLES_H_

#include "DRIVER.H"

/*****************************************************************************
 * The topology
 *****************************************************************************
 *
 *  wave>-------VOL---------------------+
 *                                      |
 * synth>-------VOL--+------------------+
 *                   |                  |
 *                   +--SWITCH_2X2--+   |
 *                                  |   |
 *    cd>-------VOL--+--SWITCH----------+
 *                   |              |   |
 *                   +--SWITCH_2X2--+   |
 *                                  |   |
 *   aux>-------VOL--+--SWITCH----------+
 *                   |              |   |
 *                   +--SWITCH_2X2--+   |
 *                                  |   |
 *   mic>--AGC--VOL--+--SWITCH----------+--VOL--BASS--TREBLE--GAIN-->lineout
 *                   |              |
 *                   +--SWITCH_1X2--+-------------------------GAIN-->wavein
 *
 */
 
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
      STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
      STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
   }
};

/*****************************************************************************
 * PinDataRangePointersBridge
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for audio bridge pins.
 */
static
PKSDATARANGE PinDataRangePointersBridge[] =
{
    &PinDataRangesBridge[0]
};

#define STATIC_KSAUDFNAME_IISVOLUME\
    0x4AABCD47L, 0xFB00, 0x11D1, 0x9B, 0x43, 0x00, 0x60, 0x97, 0xC7, 0x9D, 0x21
DEFINE_GUIDSTRUCT("4AABCD47-FB00-11D1-9B43-006097C79D21", KSAUDFNAME_IISVOLUME);
#define KSAUDFNAME_IISVOLUME DEFINE_GUIDNAMED(KSAUDFNAME_IISVOLUME)

#define STATIC_KSAUDFNAME_IISMUTE\
    0x4AABCD48L, 0xFB00, 0x11D1, 0x9B, 0x43, 0x00, 0x60, 0x97, 0xC7, 0x9D, 0x21
DEFINE_GUIDSTRUCT("4AABCD48-FB00-11D1-9B43-006097C79D21", KSAUDFNAME_IISMUTE);
#define KSAUDFNAME_IISMUTE DEFINE_GUIDNAMED(KSAUDFNAME_IISMUTE)

#define STATIC_KSAUDFNAME_IISPIN\
    0x4AABCD41L, 0xFB00, 0x11D1, 0x9B, 0x43, 0x00, 0x60, 0x97, 0xC7, 0x9D, 0x21
DEFINE_GUIDSTRUCT("4AABCD41-FB00-11D1-9B43-006097C79D21", KSAUDFNAME_IISPIN);
#define KSAUDFNAME_IISPIN DEFINE_GUIDNAMED(KSAUDFNAME_IISPIN)

#define STATIC_KSAUDFNAME_MIXERPIN\
    0xD6C86C69L, 0x51D8, 0x11D1, 0xA4, 0x82, 0x00, 0x60, 0x97, 0xC7, 0x9D, 0x21
DEFINE_GUIDSTRUCT("D6C86C69-51D8-11D1-A482-006097C79D21", KSAUDFNAME_MIXERPIN);
#define KSAUDFNAME_MIXERPIN DEFINE_GUIDNAMED(KSAUDFNAME_MIXERPIN)

#define STATIC_KSAUDFNAME_MIXERVOLUME\
    0xD6C86C70L, 0x51D8, 0x11D1, 0xA4, 0x82, 0x00, 0x60, 0x97, 0xC7, 0x9D, 0x21
DEFINE_GUIDSTRUCT("D6C86C70-51D8-11D1-A482-006097C79D21", KSAUDFNAME_MIXERVOLUME);
#define KSAUDFNAME_MIXERVOLUME DEFINE_GUIDNAMED(KSAUDFNAME_MIXERVOLUME)

#define STATIC_KSAUDFNAME_3DEFFECT\
    0xD6C86C67L, 0x51D8, 0x11D1, 0xA4, 0x82, 0x00, 0x60, 0x97, 0xC7, 0x9D, 0x21
DEFINE_GUIDSTRUCT("D6C86C67-51D8-11D1-A482-006097C79D21", KSAUDFNAME_3DEFFECT);
#define KSAUDFNAME_3DEFFECT DEFINE_GUIDNAMED(KSAUDFNAME_3DEFFECT)

#define STATIC_KSAUDFNAME_RECMON\
    0xD6C86C68L, 0x51D8, 0x11D1, 0xA4, 0x82, 0x00, 0x60, 0x97, 0xC7, 0x9D, 0x21
DEFINE_GUIDSTRUCT("D6C86C68-51D8-11D1-A482-006097C79D21", KSAUDFNAME_RECMON);
#define KSAUDFNAME_RECMON DEFINE_GUIDNAMED(KSAUDFNAME_RECMON)

#define STATIC_KSAUDFNAME_PHONEPIN\
    0x7DCEDC60L, 0x0F43, 0x11D1, 0x90, 0x5C, 0xA4, 0x84, 0x1B, 0x26, 0x29, 0x22
DEFINE_GUIDSTRUCT("7DCEDC60-0F43-11D2-905C-A4841B262922", KSAUDFNAME_PHONEPIN);
#define KSAUDFNAME_PHONEPIN DEFINE_GUIDNAMED(KSAUDFNAME_PHONEPIN)

#define STATIC_KSAUDFNAME_PHONEVOLUME\
    0x7DCEDC62L, 0x0F43, 0x11D1, 0x90, 0x5C, 0xA4, 0x84, 0x1B, 0x26, 0x29, 0x21
DEFINE_GUIDSTRUCT("7DCEDC62-0F43-11D2-905C-A4841B262922", KSAUDFNAME_PHONEVOLUME);
#define KSAUDFNAME_PHONEVOLUME DEFINE_GUIDNAMED(KSAUDFNAME_PHONEVOLUME)

#define STATIC_KSAUDFNAME_PHONEMUTE\
    0x7DCEDC61L, 0x0F43, 0x11D1, 0x90, 0x5C, 0xA4, 0x84, 0x1B, 0x26, 0x29, 0x21
DEFINE_GUIDSTRUCT("7DCEDC61-0F43-11D2-905C-A4841B262922", KSAUDFNAME_PHONEMUTE);
#define KSAUDFNAME_PHONEMUTE DEFINE_GUIDNAMED(KSAUDFNAME_PHONEMUTE)

#define STATIC_KSAUDFNAME_20DB\
    0x9DBBDD21L, 0x5CF6, 0x11D1, 0x88, 0xB4, 0x00, 0xC0, 0x9F, 0x00, 0x2B, 0x8F
DEFINE_GUIDSTRUCT("9DBBDD21-5CF6-11d1-88B4-00c09F002B8F", KSAUDFNAME_20DB);
#define KSAUDFNAME_20DB DEFINE_GUIDNAMED(KSAUDFNAME_20DB)



/*****************************************************************************
 * MiniportPins
 *****************************************************************************
 * List of pins.
 */
static
PCPIN_DESCRIPTOR 
MiniportPins[] =
{
    // WAVEIN_DEST
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
            &KSCATEGORY_AUDIO,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // SYNTH_SOURCE
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
            &KSNODETYPE_SYNTHESIZER,                    // Category
            &KSAUDFNAME_MIDI,                           // Name
            0                                           // Reserved
        }
    },

    // CD_SOURCE
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
            &KSNODETYPE_CD_PLAYER,                      // Category
            &KSAUDFNAME_CD_AUDIO,                       // Name
            0                                           // Reserved
        }
    },

    // LINEIN_SOURCE
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
            &KSNODETYPE_LINE_CONNECTOR,                 // Category
            &KSAUDFNAME_LINE_IN,                        // Name
            0                                           // Reserved
        }
    },

    // MIC_SOURCE
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
            &KSNODETYPE_MICROPHONE,                     // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // AUX_SOURCE
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
            &KSNODETYPE_ANALOG_CONNECTOR,               // Category
            &KSAUDFNAME_AUX,                            // Name
            0                                           // Reserved
        }
    },

    // IIS_SOURCE
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
            &KSNODETYPE_ANALOG_CONNECTOR,               // Category
            &KSAUDFNAME_IISPIN,                         // Name
            0                                           // Reserved
        }
    },

    // MIXER_SOURCE
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
            &KSNODETYPE_ANALOG_CONNECTOR,               // Category
            &KSAUDFNAME_MIXERPIN,                       // Name
            0                                           // Reserved
        }
    },

    // PCSPEAKER_SOURCE
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
            &KSNODETYPE_ANALOG_CONNECTOR,               // Category
            &KSAUDFNAME_PC_SPEAKER,                     // Name
            0                                           // Reserved
        }
    },

    // PHONE_SOURCE
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
            &KSNODETYPE_ANALOG_CONNECTOR,               // Category
            &KSAUDFNAME_PHONEPIN,                       // Name
            0                                           // Reserved
        }
    },

    // LINEOUT_DEST
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
            &KSNODETYPE_SPEAKER,                        // Category
            &KSAUDFNAME_VOLUME_CONTROL,                 // Name (this name shows up as
                                                        // the playback panel name in SoundVol)
            0                                           // Reserved
        }
    },
    
    // WAVEIN_PIN  Wave In Streaming Pin (Capture)
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
            &KSCATEGORY_AUDIO,                          // Category
            &KSAUDFNAME_RECORDING_CONTROL,              // Name (this name shows up as
                                                        // the playback panel name in SoundVol)
            0                                           // Reserved
        }
    }
};

/*****************************************************************************
 * PropertiesVolume
 *****************************************************************************
 * Properties for volume controls.
 */
static
PCPROPERTY_ITEM PropertiesVolume[] =
{
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_VOLUMELEVEL,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Level
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET,
        PropertyHandler_CpuResources
    }
};

/*****************************************************************************
 * AutomationVolume
 *****************************************************************************
 * Automation table for volume controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationVolume,PropertiesVolume);

/*****************************************************************************
 * PropertiesAgc
 *****************************************************************************
 * Properties for AGC controls.
 */
static
PCPROPERTY_ITEM PropertiesAgc[] =
{
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_AGC,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET,
        PropertyHandler_OnOff
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET,
        PropertyHandler_CpuResources
    }
};

/*****************************************************************************
 * AutomationAgc
 *****************************************************************************
 * Automation table for Agc controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationAgc,PropertiesAgc);

/*****************************************************************************
 * PropertiesMute
 *****************************************************************************
 * Properties for mute controls.
 */
static
PCPROPERTY_ITEM PropertiesMute[] =
{
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_MUTE,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET,
        PropertyHandler_OnOff
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET,
        PropertyHandler_CpuResources
    }
};

/*****************************************************************************
 * AutomationMute
 *****************************************************************************
 * Automation table for mute controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMute,PropertiesMute);

/*****************************************************************************
 * PropertiesMute
 *****************************************************************************
 * Properties for mute controls.
 */
static
PCPROPERTY_ITEM PropertiesMuxSource[] =
{
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_MUTE,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET,
        PropertyHandler_MuxSource
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET,
        PropertyHandler_CpuResources
    }
};

/*****************************************************************************
 * AutomationMuxSource
 *****************************************************************************
 * Automation table for mute controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMuxSource,PropertiesMuxSource);


/*****************************************************************************
 * The Event for the Master Volume (or other nodes)
 *****************************************************************************
 * Generic event for nodes.
 */
static PCEVENT_ITEM NodeEvent[] =
{
    // This is a generic event for nearly every node property.
    {
        &KSEVENTSETID_AudioControlChange,   // Something changed!
        KSEVENT_CONTROL_CHANGE,             // The only event-property defined.
        KSEVENT_TYPE_BASICSUPPORT | KSEVENT_TYPE_ENABLE,
        CMiniportTopologyESS::EventHandler
    }
};

/*****************************************************************************
 * AutomationVolumeWithEvent
 *****************************************************************************
 * This is the automation table for Volume events.
 * You can create Automation tables with event support for any type of nodes
 * (e.g. mutes) with just adding the generic event above. The automation table
 * then gets added to every node that should have event support.
 */
DEFINE_PCAUTOMATION_TABLE_PROP_EVENT (AutomationVolumeWithEvent, PropertiesVolume, NodeEvent);
DEFINE_PCAUTOMATION_TABLE_PROP_EVENT (AutomationMuteWithEvent, PropertiesMute, NodeEvent);


/*****************************************************************************
 * TopologyNodes
 *****************************************************************************
 * List of node identifiers.
 */
static
PCNODE_DESCRIPTOR TopologyNodes[] =
{
    // WAVEOUT_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_WAVE_VOLUME // Name
    },

    // WAVEOUT_MUTE
    {
        0,                      // Flags
        &AutomationMute,        // AutomationTable
        &KSNODETYPE_MUTE,       // Type
        &KSAUDFNAME_WAVE_MUTE   // Name
    },

    // SYNTH_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_MIDI_VOLUME // Name
    },

    // SYNTH_WAVEIN_MUTE
    {
        0,                      // Flags
        &AutomationMute,        // AutomationTable
        &KSNODETYPE_MUTE,       // Type
        &KSAUDFNAME_MIDI_MUTE   // Name
    },

    // CD_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_CD_VOLUME   // Name
    },

    // CD_LINEOUT_MUTE
    {
        0,                      // Flags
        &AutomationMute,        // AutomationTable
        &KSNODETYPE_MUTE,       // Type
        &KSAUDFNAME_CD_MUTE     // Name
    },

    // LINEIN_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_LINE_VOLUME // Name
    },

    // LINEIN_LINEOUT_MUTE
    {
        0,                      // Flags
        &AutomationMute,        // AutomationTable
        &KSNODETYPE_MUTE,      // Type
        &KSAUDFNAME_LINE_MUTE   // Name
    },

    // MIC_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_MIC_VOLUME  // Name
    },

    // MIC_LINEOUT_MUTE
    {
        0,                      // Flags
        &AutomationMute,        // AutomationTable
        &KSNODETYPE_MUTE,       // Type
        &KSAUDFNAME_MIC_MUTE    // Name
    },

    // AUX_LINEOUT_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_AUX_VOLUME  // Name
    },

    // AUX_LINEOUT_MUTE
    {
        0,                      // Flags
        &AutomationMute,        // AutomationTable
        &KSNODETYPE_MUTE,       // Type
        &KSAUDFNAME_AUX_MUTE    // Name
    },

    // IIS_LINEOUT_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_IISVOLUME   // Name
    },

    // IIS_MUTE
    {
        0,                      // Flags
        &AutomationMute,        // AutomationTable
        &KSNODETYPE_MUTE,       // Type
        &KSAUDFNAME_IISMUTE     // Name
    },

    // PC_SPEAKER_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_PC_SPEAKER_VOLUME   // Name
    },

    // PC_SPEAKER_MUTE
    {
        0,                      // Flags
        &AutomationMute,        // AutomationTable
        &KSNODETYPE_MUTE,       // Type
        &KSAUDFNAME_PC_SPEAKER_MUTE     // Name
    },

    // PHONE_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_PHONEVOLUME // Name
    },

    // PHONE_MUTE
    {
        0,                      // Flags
        &AutomationMute,        // AutomationTable
        &KSNODETYPE_MUTE,       // Type
        &KSAUDFNAME_PHONEMUTE   // Name
    },

    // MIDI_IN_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_MIDI_IN_VOLUME // Name
    },

    // CD_IN_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_CD_IN_VOLUME // Name
    },

    // LINE_IN_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_LINE_IN_VOLUME // Name
    },

    // MIC_IN_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_MIC_IN_VOLUME // Name
    },

    // GAIN_20DB
    {
        0,                      // Flags
        &AutomationAgc,         // AutomationTable
        &KSNODETYPE_AGC,        // Type
        &KSAUDFNAME_20DB	    // Name
    },

    // AUX_IN_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_AUX_VOLUME  // Name
    },

    // IIS_IN_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_IISVOLUME  // Name
    },

    // MIXER_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_MIXERVOLUME // Name
    },

    // LINEOUT_MIX
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_SUM,        // Type
        NULL                    // Name
    },

    // LINEOUT_VOL
    {
        0,                      // Flags
        &AutomationVolumeWithEvent, // AutomationTable with event support
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_MASTER_VOLUME // Name
    },

    // LINEOUT_MUTE
    {
        0,                      // Flags
        &AutomationMuteWithEvent, // AutomationTable with event support
        &KSNODETYPE_MUTE,       // Type
        &KSAUDFNAME_MASTER_MUTE // Name
    },

	// GAIN_3DEFFECT
    {
        0,                      // Flags
        &AutomationAgc,         // AutomationTable
        &KSNODETYPE_AGC,        // Type
        &KSAUDFNAME_3DEFFECT    // Name
    },

	// RECORDING_SOURCE
    {
        0,                      // Flags
        &AutomationMuxSource,   // AutomationTable
        &KSNODETYPE_MUX,        // Type
        &KSAUDFNAME_RECORDING_SOURCE    // Name
    },

	// RECMON
    {
        0,                      // Flags
        &AutomationAgc,         // AutomationTable
        &KSNODETYPE_AGC,        // Type
        &KSAUDFNAME_RECMON      // Name
    }
};

/*****************************************************************************
 * ConnectionTable
 *****************************************************************************
 * Table of topology unit connections.
 *
 * Pin numbering is technically arbitrary, but the convention established here
 * is to number a solitary output pin 0 (looks like an 'o') and a solitary
 * input pin 1 (looks like an 'i').  Even destinations, which have no output,
 * have an input pin numbered 1 and no pin 0.
 *
 * Nodes are more likely to have multiple ins than multiple outs, so the more
 * general rule would be that inputs are numbered >=1.  If a node has multiple
 * outs, none of these conventions apply.
 *
 * Nodes have at most one control value.  Mixers are therefore simple summing
 * nodes with no per-pin levels.  Rather than assigning a unique pin to each
 * input to a mixer, all inputs are connected to pin 1.  This is acceptable
 * because there is no functional distinction between the inputs.
 *
 * There are no multiplexers in this topology, so there is no opportunity to
 * give an example of a multiplexer.  A multiplexer should have a single
 * output pin (0) and multiple input pins (1..n).  Its control value is an
 * integer in the range 1..n indicating which input is connected to the
 * output.
 *
 * In the case of connections to pins, as opposed to connections to nodes, the
 * node is identified as PCFILTER_NODE and the pin number identifies the
 * particular filter pin.
 */
static
PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{   //  FromNode,               FromPin,          ToNode,                 ToPin
    {   PCFILTER_NODE,          WAVEIN_DEST,      WAVEOUT_VOLUME,         1             },
    {   WAVEOUT_VOLUME,         0,                WAVEOUT_MUTE,           1             },
	{   WAVEOUT_MUTE,           0,                LINEOUT_MIX,            1             },

    {   PCFILTER_NODE,          SYNTH_SOURCE,     SYNTH_VOLUME,           1             },
    {   SYNTH_VOLUME,           0,                SYNTH_WAVEIN_MUTE,      1             },
    {   SYNTH_WAVEIN_MUTE,      0,                LINEOUT_MIX,            2             },

    {   PCFILTER_NODE,          CD_SOURCE,        CD_VOLUME,              1             },
    {   CD_VOLUME,              0,                CD_LINEOUT_MUTE,        1             },
    {   CD_LINEOUT_MUTE,        0,                LINEOUT_MIX,            3             },

    {   PCFILTER_NODE,          LINEIN_SOURCE,    LINEIN_VOLUME,          1             },
    {   LINEIN_VOLUME,          0,                LINEIN_LINEOUT_MUTE,    1             },
    {   LINEIN_LINEOUT_MUTE,    0,                LINEOUT_MIX,            4             },

    {   PCFILTER_NODE,          MIC_SOURCE,       MIC_VOLUME,             1             },
    {   MIC_VOLUME,             0,                MIC_LINEOUT_MUTE,       1             },
    {   MIC_LINEOUT_MUTE,       0,                LINEOUT_MIX,            5             },

    {   PCFILTER_NODE,          AUX_SOURCE,       AUX_LINEOUT_VOLUME,     1             },
    {   AUX_LINEOUT_VOLUME,     0,                AUX_LINEOUT_MUTE,       1             },
    {   AUX_LINEOUT_MUTE,       0,                LINEOUT_MIX,            6             },

    {   PCFILTER_NODE,          IIS_SOURCE,       IIS_LINEOUT_VOLUME,     1             },
    {   IIS_LINEOUT_VOLUME,     0,                IIS_MUTE,               1             },
    {   IIS_MUTE,               0,                LINEOUT_MIX,            7             },

    {   PCFILTER_NODE,          PCSPEAKER_SOURCE, PC_SPEAKER_VOLUME,      1             },
    {   PC_SPEAKER_VOLUME,      0,                PC_SPEAKER_MUTE,        1             },
    {   PC_SPEAKER_MUTE,        0,                LINEOUT_MIX,            8             },

    {   PCFILTER_NODE,          PHONE_SOURCE,     PHONE_VOLUME,           1             },
    {   PHONE_VOLUME,           0,                PHONE_MUTE,             1             },
    {   PHONE_MUTE,             0,                LINEOUT_MIX,            9             },


    {   PCFILTER_NODE,          SYNTH_SOURCE,     MIDI_IN_VOLUME,         1             },
    {   MIDI_IN_VOLUME,         0,                RECORDING_SOURCE,       1             },

    {   PCFILTER_NODE,          CD_SOURCE,        CD_IN_VOLUME,           1             },
    {   CD_IN_VOLUME,           0,                RECORDING_SOURCE,       2             },
	
    {   PCFILTER_NODE,          LINEIN_SOURCE,    LINE_IN_VOLUME,         1             },
    {   LINE_IN_VOLUME,         0,                RECORDING_SOURCE,       3             },

    {   PCFILTER_NODE,          MIC_SOURCE,       GAIN_20DB,              1             },
	{	GAIN_20DB, 				0,				  MIC_IN_VOLUME,		  1				},
    {   MIC_IN_VOLUME,          0,                RECORDING_SOURCE,       4             },

    {   PCFILTER_NODE,          AUX_SOURCE,       AUX_IN_VOLUME,          1             },
    {   AUX_IN_VOLUME,          0,                RECORDING_SOURCE,       5             },

    {   PCFILTER_NODE,          IIS_SOURCE,       IIS_IN_VOLUME,          1             },
    {   IIS_IN_VOLUME,          0,                RECORDING_SOURCE,       6             },

    {   PCFILTER_NODE,          MIXER_SOURCE,     RECMON,             	  1             },
    {   RECMON,                 0,                MIXER_VOLUME,       	  1             },
	{   MIXER_VOLUME,           0,                RECORDING_SOURCE,       7             },
	
	
    {   LINEOUT_MIX,            0,                LINEOUT_VOL,            1             },
    {   LINEOUT_VOL,            0,                LINEOUT_MUTE,           1             },
	{   LINEOUT_MUTE,           0,                GAIN_3DEFFECT,          1             },
    {   GAIN_3DEFFECT,          0,                PCFILTER_NODE,          LINEOUT_DEST  },
    {   RECORDING_SOURCE,       0,                PCFILTER_NODE,          WAVEIN_PIN    }
};

/*****************************************************************************
 * MiniportFilterDescription
 *****************************************************************************
 * Complete miniport description.
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
    SIZEOF_ARRAY(TopologyNodes),        // NodeCount
    TopologyNodes,                      // Nodes
    SIZEOF_ARRAY(MiniportConnections),  // ConnectionCount
    MiniportConnections,                // Connections
    0,                                  // CategoryCount
    NULL                                // Categories: NULL->use default (audio, render, capture)
};

#endif
