/*++

Module Name:

    devcaps.c

Abstract:

    This module contains code for the device capabilities functions.

Author:

    leecher1337

Environment:

    Kernel mode

Revision History:

--*/

#include "sound.h"
#include <string.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SoundWaveOutGetCaps)
#pragma alloc_text(PAGE, SoundWaveInGetCaps)
#pragma alloc_text(PAGE, SoundMidiOutGetCaps)
#pragma alloc_text(PAGE, SoundMidiInGetCaps)
#pragma alloc_text(PAGE, SoundMidiOutGetSynthCaps)
#pragma alloc_text(PAGE, SoundAuxGetCaps)
#pragma alloc_text(PAGE, SoundQueryFormat)
#endif

// non-localized strings version is wrong !!!
WCHAR STR_MIDIOUT_PNAME[] = L"ESS AudioDrive MIDI port Output";
WCHAR STR_MIDIIN_PNAME[] = L"ESS AudioDrive MIDI port Input";


NTSTATUS
SoundWaveOutGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for wave output device.
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    WAVEOUTCAPSW        wc;
    NTSTATUS            status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;

    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(wc),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    //

    wc.wMid = MM_ESS;
    wc.vDriverVersion = DRIVER_VERSION;

    wc.wPid = MM_ESS_AMWAVEOUT;
    wc.dwFormats = WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 |
                   WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16 |
                   WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 |
                   WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 |
                   WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08 |
                   WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16;
    wc.wChannels = 2;
    wc.dwSupport = WAVECAPS_VOLUME | WAVECAPS_LRVOLUME | WAVECAPS_SAMPLEACCURATE;


    //
    // Copy across the product name - we just provide the string id
    //

    *(PULONG)wc.szPname = IDS_WAVEOUT_PNAME;

    RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &wc,
                  (int)pIrp->IoStatus.Information);

    return status;
}


NTSTATUS
SoundWaveInGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for wave input device.
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    WAVEINCAPSW wc;
    NTSTATUS status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;


    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(wc),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    //

    wc.wMid = MM_ESS;
    wc.vDriverVersion = DRIVER_VERSION;

    wc.wPid = MM_ESS_AMWAVEIN;
    wc.dwFormats = WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 |
                   WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16 |
                   WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 |
                   WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 |
                   WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08 |
                   WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16;
    wc.wChannels = 2;

    //
    // Copy across the product name - we just provide the string id
    //

    *(PULONG)wc.szPname = IDS_WAVEIN_PNAME;

    RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &wc,
                  (int)pIrp->IoStatus.Information);

    return status;
}


NTSTATUS
SoundMidiOutGetSynthCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for midi output device.
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    MIDIOUTCAPSW        mc;
    NTSTATUS            status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = CONTAINING_RECORD(pLDI->pGlobalInfo, GLOBAL_DEVICE_INFO, Synth);

    ASSERT(pGDI->Key == GDI_KEY);

    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(mc),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    //

    mc.wMid = MM_ESS;
    mc.wPid = MM_ESS_AMSYNTH;
    mc.wTechnology = MOD_FMSYNTH;
    mc.wVoices = 128;
    mc.wNotes = 18;
    mc.wChannelMask = 0xffff;                       // all channels
    mc.vDriverVersion = DRIVER_VERSION;
    mc.dwSupport = MIDICAPS_VOLUME | MIDICAPS_LRVOLUME;

    //
    // Copy across the product name - we just provide the string id
    //

    *(PULONG)mc.szPname = IDS_SYNTH_PNAME;

    RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &mc,
                  (int)pIrp->IoStatus.Information);

    return status;
}


NTSTATUS
SoundMidiOutGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for midi output device.
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    MIDIOUTCAPSW    mc;
    NTSTATUS        status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;

    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(mc),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    //

    mc.wMid = MM_ESS;
    mc.wPid = MM_ESS_AMMIDIOOUT;
    mc.vDriverVersion = DRIVER_VERSION;
    mc.wTechnology = MOD_MIDIPORT;
    mc.wVoices = 0;                   // not used for ports
    mc.wNotes = 0;                    // not used for ports
    mc.wChannelMask = 0xFFFF;         // all channels
    mc.dwSupport = 0L;

    RtlCopyMemory(mc.szPname, STR_MIDIOUT_PNAME, sizeof(STR_MIDIOUT_PNAME));

    RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &mc,
                  (int)pIrp->IoStatus.Information);

    return status;
}



NTSTATUS
SoundMidiInGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for midi input device.
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    MIDIINCAPSW mc;
    NTSTATUS    status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;


    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(mc),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    //

    mc.wMid = MM_ESS;
    mc.wPid = MM_ESS_AMMIDIIN;
    mc.vDriverVersion = DRIVER_VERSION;

    RtlCopyMemory(mc.szPname, STR_MIDIIN_PNAME, sizeof(STR_MIDIIN_PNAME));

    RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &mc,
                  (int)pIrp->IoStatus.Information);

    return status;
}


NTSTATUS
SoundAuxGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for axu devices
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    AUXCAPSW auxCaps;
    NTSTATUS status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;

    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(auxCaps),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    //

    auxCaps.wMid = MM_ESS;
    auxCaps.wPid = MM_ESS_AMAUX;
    auxCaps.vDriverVersion = DRIVER_VERSION;
    auxCaps.wTechnology = AUXCAPS_AUXIN;
    auxCaps.dwSupport = AUXCAPS_LRVOLUME | AUXCAPS_VOLUME;

    RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &auxCaps,
                  (int)pIrp->IoStatus.Information);

    return status;
}



NTSTATUS SoundQueryFormat(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN    PPCMWAVEFORMAT pFormat,
    IN    ULONG IoControlCode
)
/*++

Routine Description:

    Tell the caller whether the wave format specified (input or
    output) is supported

Arguments:

    pLDI - pointer to local device info
    pFormat - format being queried

Return Value:

    STATUS_SUCCESS - format is supported
    STATUS_NOT_SUPPORTED - format not supported

--*/
{
    PGLOBAL_DEVICE_INFO pGDI;
    DWORD nMaxSamplesPerSec = 44100;

    pGDI = pLDI->pGlobalInfo;

    if (pFormat->wf.wFormatTag != WAVE_FORMAT_PCM ||
        pFormat->wf.nChannels != 1 && pFormat->wf.nChannels != 2) {
        return STATUS_NOT_SUPPORTED;
    }

    if (pLDI->pGlobalInfo->Hw.DSPVersion == 6249 || pLDI->pGlobalInfo->Hw.DSPVersion == 6265)
        nMaxSamplesPerSec = 44100;

    if (pFormat->wBitsPerSample != 8 && pFormat->wBitsPerSample != 16 ||
        pFormat->wf.nSamplesPerSec < 4000 ||
        pFormat->wf.nSamplesPerSec > nMaxSamplesPerSec ||
        ((pLDI->pGlobalInfo->Hw.DSPVersion == 6248 || pLDI->pGlobalInfo->Hw.DSPVersion == 6264) && 
         IoControlCode == IOCTL_WDMAUD_REMOVE_DEVNODE && !FdxFormatCheck(pLDI, pFormat)) {
        return STATUS_NOT_SUPPORTED;
    } 
    
    return STATUS_SUCCESS;
}


