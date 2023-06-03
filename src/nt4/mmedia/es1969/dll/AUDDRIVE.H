/*++ BUILD Version: 0001    // Increment this if a change has global effects


Copyright (c) 2023 leecher@dose.0wnz.at

Module Name:

    ess.h

Abstract:

    This include file defines constants and types for
    the ESS card.

	This header file is shared between the low level driver and the
	kernel driver.

Revision History:

--*/

/*
**  Registry value names
*/

#define SOUND_REG_ES938                      (L"ES938")
#define SOUND_REG_ENABLE_AUTO_INSTALL        (L"EnableAutoInstall")
#define SOUND_REG_AUTO_INSTALL               (L"AutoInstall")
#define SOUND_REG_DISABLE_MICGAIN            (L"DisableMicGain")
#define SOUND_REG_MUTE_PHONE                 (L"MutePhone")
#define SOUND_REG_2WIRES_EXTHWVOL            (L"2WireExtHwVol")
#define SOUND_REG_SINGLE_TX_DMA              (L"SingleTransferDMA")
#define SOUND_REG_3D_LIMIT                   (L"3D-Limit")
#define SOUND_REG_SPATIALIZER_ENABLE         (L"SpatializerEnable")
#define SOUND_REG_NEW_LAPTOP_SYS             (L"New_Laptop.Sys")
#define SOUND_REG_DEFAULT_EXT_MIDI           (L"defaultExtMidi")
#define SOUND_REG_DISABLE_AUXB               (L"DisableAuxB")
#define SOUND_REG_RECORD_MONITOR_MODE        (L"RecordMonitorMode")
#define SOUND_REG_ENABLE_IIS                 (L"EnableIIS")
#define SOUND_REG_ENABLE_MPU401              (L"enableMPU401")
#define SOUND_REG_SOFTEXAPM                  (L"SoftexAPM")
#define SOUND_REG_DEFAULT_MASTER_VOL         (L"DefaultMasterVol")
#define SOUND_REG_DEFAULT_RECORDMIC_VOL      (L"DefaultRecordMicVol")
#define SOUND_REG_POLLINGMASTER_VOL          (L"PollingMasterVol")
#define SOUND_REG_REDUCEDDECBOUNCEEXTHW_VOL  (L"ReducedDebounceExtHwVol")
#define SOUND_REG_DEFAULT_LINEIN_MUTE        (L"DefaultLineInMute")
#define SOUND_REG_DEFAULT_AUXB_MUTE          (L"DefaultAuxBMute")
#define SOUND_REG_DEFAULT_PCSPKR_MUTE        (L"DefaultPCSpkrMute")
#define SOUND_REG_VERBOSE                    (L"Verbose")
#define SOUND_REG_IRQCHOICES                 (L"irqchoices")


/*
**  ESS midi mappings
*/

#define ESS_MAPPER_ESS      TEXT("ESS")
#define ESS_MAPPER_ESS_ESFM TEXT("ESFM")


/*
**  Registry 'return' codes
*/

#define SOUND_CONFIG_MPU401_PORT_INUSE   0x80000002
#define SOUND_CONFIG_BAD_MPU401_PORT     0x80000003

