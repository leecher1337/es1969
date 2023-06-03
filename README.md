About
=====
For unknown reason, there are no 64bit drivers available for the old 
[ESS](https://en.wikipedia.org/wiki/ESS_Technology) PCI Soundcards like 
the ES1969 (Solo1), ES1946, Allegro PCI etc.
Therefore they don't work on newer Windows versions like Windows 10 x64
(at least not in native ESFM mode).
However these cards feature a native MIDI Synthesizer called ESFM that 
[some](https://github.com/Wohlstand/OPL3BankEditor/issues/164) 
[people](https://github.com/leecher1337/ntvdmx64/issues/203) may want
to use.
This project is an attempted reconstruction of the ES1969.SYS soundcard
driver shipped with 32bit Windows Versions, source code should be a pretty
accurate reconstruction of the original sourcecode including naming schemes.

Additionally, this project also provides a generic 64bit Gameport driver for 
the MPU-401 port, in case some people are missing it, as Windows since XP
doesn't support it officially anymore and I didn't find a 64bit gameport 
driver.

Installing
==========

Preparation
-----------
Download the appropriate ES1969 driver package for your machine from the 
"Releases".
Optionally, you can also download the gameport-driver, if you want to try
and need it, but it's not required.

The driver is self-signed, as I don't have a driver signature certificate which 
has insane requirements (register as a company, pay yearly fees for the 
certificate, let Microsoft sign the driver, ...), therefore until someone 
offers to sign  the driver you unfortunately only can run the driver in test 
mode:

Open up command prompt as administrator and run the following command:
```
bcdedit /set testsigning on
```
If you get the "The value is protected by Secure Boot policy" error, you need 
to disable Secure Boot in BIOS before running the command.
Alternatively refer to 
[this guide](https://www.thewindowsclub.com/disable-driver-signature-enforcement-windows)
on how to enable test signing using the boot menu.

Next, install the certificate shipped with the driver as trusted root 
certificate by running the script `install_cert.cmd` as administrator 
in the distribution directory.
If it doesn't work, double click the es1969.cer file and import it by
adding it to the store of "Trusted publishers".

Then reboot to enable test mode in order to be able to install unsigned drivers.

Installation
------------
Just install the driver like any usual driver. Go to "Device manager" and
search your audio device (which has an exclamation mark, as driver cannot
be found), double click and "Update driver...".
Then tell the wizard to locate the driver on your computer.

Then browse to the directory where you put the es1969 drivers and hit Next.
ES 1969 driver should be found and installed.

If you need the gameport, be aware that I was unable to test it, as 
for me it ended up in a resource conflict error, possibly because I have 
an onboard soundcard too which doesn't let me use the MPU-401 Port, as it
is occupying the same IO-port range, but I cannot disable it (BIOS locks up 
my machine when disabling onboard sound, seems to be a bug), so it's just
a guess about the reason.
Use the Issue tracker if you really need the Gameport for some reason.

You find a device like i.e. "Game port for Thrustmaster ACM" in Device 
manager under  "Audio, Video and Gamecontroller". Double click it and 
Update driver. As before, tell the wizard to locate the driver on your computer.
On next dialog, do not browse to the gameport driver path, but choose
"Select from a list" on your computer. The go to "Have disk.." and locate
the directory of the downloaded gameport driver and then select it from 
the list. This way, it will overwrite the default driver that disables 
gameport.

Configuration
=============
The driver should work out of the box. However, there is an important 
enhancement to the original driver by including the ability to load different
MIDI patch sets. The rest of the parameters are the same as with the original
ESS driver (no idea if anyone ever documented them, but usually you don't
want to change them).

Setting different Midi Patches
------------------------------
There were 2 different Sets of MIDI-Patches, one in the NT4 driver
and the default one in the Win 9x/ME/Win 2k+ driver.
The default patchset of the appropriate driver has been linked into the
drivers. With this driver however, you can change the patch set via a registry
key.
The patch sets can be obtained from 
[ESSplaymid repository](https://github.com/pachuco/ESSPlayMid/tree/master/bin).

To use a patchset other than the linked-in patchset (i.e. the NT4 patchset),
first locate the registry key containing the driver settings. 
This is by opening the device properties in the device manager and reading
the property "Driver key", which gives you the correct path in the registry 
under: `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class`.

For example, my settings are located under: 
`HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e96c-e325-11ce-bfc1-08002be10318}\0002`

Create a new reigstry key named `Patches` with type `REG_SZ` there which 
points to the soundbank to use.
Ensure that you use a NT-Path, otherwise it won't work. If you are using
a standard DOS-Path, say, `C:\Temp\bnk_NT4.bin`, then you have to prepend
`\??\` in front of the filename, so the filename to use would be 
`\??\C:\Temp\bnk_NT4.bin`.

Don't forget to disable and re-eanble audio driver in device manager to apply
the settings (or simply reboot).

Playing MIDI files
==================
When playing .mid files i.e. with Windows Media Player, Widnows uses the 
Windwos built-in Microsoft GS Wavetable Synthesizer and outputs it to the 
sound card as wave-sound, thus, not using the card's own Synthesizer.
Therefore you need a dedicated MIDI player, like for instance 
[TMIDI: (Tom's MIDI Player)](https://www.grandgent.com/tom/projects/tmidi/).
On "MIDI out", select the `ESS ESFM Synthesizer` and it will play via the 
ESS native Synthesizer using this driver.

Compiling
=========
The repository contains 2 driver sources. One for Windows NT4, one for 
Windows 2000 and above (WDM). 

The NT4 driver
--------------
The NT4 driver consists of a usermode part (AUDDRIVE.DLL) and a kernel mode 
part (AUDDRIVE.SYS). The kernel mode part does the communication with the
audio card whereas the usermode part just does i.e. the handling for the 
WINMM Multimedia System (i.e. MIDI handling) or displaying the configuration
dialog for the card's parameters (IRQ/DMA).

Currently, only the AUDDRIVE.DLL usermode part has been reconstructed,
the kernel mode driver could be reconstructed, but I don't see any need 
for it, its mainly useful for reference purposes.

In order to compile the NT4 driver, copy the directory structure under
[src\NT4\mmedia](NT4/) to the src\mmedia path of the 
[NT4DDK](https://winworldpc.com/product/windows-sdk-ddk/nt-40).
Then open NT4 build shell and:

1) Issue `build` in `NT4DDK\src\mmedia\drvlib` directory to compile the 
prereq. library 
2) Issue `build` in `NT4DDK\src\mmedia\es1969\synthlib` 
3) Issue `build` in `NT4DDK\src\mmedia\es1969\dll`

As a result, you will have a `AUDDRIVE.DLL` i.e. in `NT4DDK\lib\I386\free`
(or whatever build type you choose).

The Windows 2000 (and above) WDM driver
---------------------------------------
This is the driver you most probably want to use. It is based on the WDM and
therefore is compatible with Windows 2000 and above.

You can build it with 
[Windows 2000/XP DDK](https://winworldpc.com/product/windows-sdk-ddk/2003-nt-52)
by just entering the [win2k](src/win2k/) directory and issueing `build` there, 
but it won't generate the .inf file and it won't sign the driver file which 
is recommended since Windows XP and required since Windows Vista.
It just contains a generated .inf file for x64 builds (as there is a x86 
driver by the publisher anyway, so you won't need this driver on x86).

Therefore it may be more convenient to compile it with 
[Visual Studio 2019](https://visualstudio.microsoft.com/vs/older-downloads/).
For this, open the [es1969.sln](src/win2k/vs2019/es1969.sln) file in the 
win2k/vs2019 forder with Visual Studio 2019 and compile your preferred 
version.
If you have the ability to properly sign a device driver, I would be delighted
if you can do so and provide a signed driver to me for easier redistribution.

Documentation
=============
[ESS Solo-1 chip documentation](https://www.alsa-project.org/files/pub/manuals/ess/DsSolo1.pdf)

[ESFM documentation](https://github.com/jwt27/esfm)

[ESSPlayMid native ASM reference implementation](https://github.com/pachuco/ESSPlayMid)

Driver is based on SB16 NTDDK driver sample and may contain some leftovers 
from it.

Tested configurations
=====================
Currently the driver has been tested on Windows 10 x64, but it should work on
any Windows version that supports the WDM driver model (if in doubt, recompile
it for target platform).
The driver has been tested with the ESS 1969 (Solo-1) soundcard.

Bug reports
===========
Please use the project's [Issue tracker](https://github.com/leecher1337/es1969/issues).

Author
======
All hard reverse-engineering and programming work for the win2k driver has been 
done by leecher1337 (leecher@dose.0wnz.at).
Project homepage is https://github.com/leecher1337/es1969 

Special thanks to [pachuco](https://github.com/pachuco/ESSPlayMid) for ASM-port 
as Native ESFM that helped debugging issues with the reconstructed driver by 
allowing to switch between ASM "reference" implementation and reconstructed C 
code in usermode.
