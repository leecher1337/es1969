/****************************************************************************
 *
 *   config.c
 *
 *   Reconstruction of ESS NT4 driver sourcecode
 *
 *   Copyright (c) leecher@dose.0wnz.at  2023. All rights reserved.
 *
 ***************************************************************************/

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <tchar.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <regstr.h>
#include <registry.h>
#include <drvlib.h>

#include "driver.h"




TCHAR STR_PRODUCTNAME[]    = TEXT("AUDDRIVE");

#define INVALID_DSP_VERSION ((DWORD)-1)

#define CharSizeOf(x)   (sizeof(x) / sizeof(*x))
#define ByteCountOf(x)  ((x) * sizeof(TCHAR))

/****************************************************************************

       typedefs

 ***************************************************************************/

 typedef struct {
     UINT  CardId;          // Card number
     DWORD Interrupt;       // Capture IRQ    
     DWORD LoadType;        // Normal or configuration
     DWORD DmaBufferSize;   // Size for DMA buffer
     DWORD SynthType;       // Opl3?
 } ES_CONFIG, *PES_CONFIG;

/************************

  Globals

*************************/
DWORD     CurrentCard;
DWORD     CurrentPort;
DWORD     NumberOfCards;
ES_CONFIG Configuration;
BOOL      FirstTime;

DWORD     IrqChoices[] = {2, 3, 5, 7, 9, 10, 11, 12, 15, 0, 0, 0, 0, 0, 0, 0};

// Config 
DWORD  SOUND_DEF_INT  = 11;
BOOL   SOUND_DEF_ENABLE_AUTO_INSTALL = TRUE;
BOOL   SOUND_DEF_MUTE_PHONE = TRUE;
BOOL   SOUND_DEF_NEW_LAPTOP_SYS = TRUE;
BOOL   SOUND_DEF_ENABLE_MPU401 = TRUE;

DWORD  SOUND_DEF_ES938 = 0;
DWORD  SOUND_DEF_DISABLE_MICGAIN = 0;
DWORD  SOUND_DEF_2WIRES_EXTHWVOL = 0;
DWORD  SOUND_DEF_USE_MONOIN = 0;
DWORD  SOUND_DEF_SINGLE_TX_DMA = 0;
DWORD  SOUND_DEF_3D_LIMIT = 0;
DWORD  SOUND_DEF_SPATIALIZER_ENABLE = 0;
DWORD  SOUND_DEF_DEFAULT_EXT_MIDI = 0;
DWORD  SOUND_DEF_DISABLE_AUXB = 0;
DWORD  SOUND_DEF_RECORD_MONITOR_MODE =0;
DWORD  SOUND_DEF_ENABLE_IIS = 0;
DWORD  SOUND_DEF_SOFTEX_APM = 0;

DWORD  LoadDriverDuringBoot = 0;




WNDPROC OldButtonProc;
WNDPROC OldComboProc;
HFONT   g_hDlgFont;
TCHAR   g_szAppFontName[] = TEXT("MS Shell Dlg");

RECT    g_OrgRect[MAX_IDDS];
RECT    g_OrgRectDlg;


/*****************************************************************************

    internal function prototypes

 ****************************************************************************/

int     Configure(HWND hDlg);
BOOL    CenterPopup(HWND hWnd, HWND hParentWnd);
VOID    GetDriverConfig(UINT CardId, PES_CONFIG Config);
BOOL    SetDriverConfig(PES_CONFIG Config);
void    ReadConfigini(BOOL ParseOptions);
void
 GetRealString(
     PREG_ACCESS RegAccess,
     UINT   DeviceNumber,
     BYTE   StringId,
     LPTSTR Value);
VOID AddEssAPMEntry();
LONG DrvSetOptions(
     PREG_ACCESS RegAccess,
     UINT   DeviceNumber,
     LPTSTR ValueName,
     LPTSTR Value);
LONG DrvSetBinaryValue(
     PREG_ACCESS RegAccess,
     UINT   DeviceNumber,
     LPTSTR ValueName,
     LPBYTE Value,
     DWORD  ValueSize);
     
BOOL ESConfigEqual(IN  PES_CONFIG Config1,
                   IN  PES_CONFIG Config2);


LRESULT CALLBACK
ButtonSubClassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );


LRESULT CALLBACK
ComboBoxSubClassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
Config_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    );

void
Config_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    );

void
PrintHelpText(
    HWND hwnd
    );

void
ResizeDialog(
    HWND hwnd
    );

void
SetDialogTitle(
    HWND hwnd
    );

void
GetOriginalControlPositions(
    HWND hwnd
    );


/*
**  Set the configuration to invalid - this will stop SetDriverConfig writing
**  things we didn't set
*/
VOID InitConfiguration(PES_CONFIG Config)
{
    Config->Interrupt     = (DWORD)-1;
    Config->SynthType     = (DWORD)-1;
    Config->LoadType      = SOUND_LOADTYPE_NORMAL;
}


/*****************************************************************************

    ConfigRemove()

*****************************************************************************/
LRESULT ConfigRemove(HWND hDlg)
{
    BOOL    Unloaded;
    BOOL    Deleted;

    //
    // Is the driver currently loaded
    //
    if (!DrvIsDriverLoaded(&RegAccess)) {
        DrvDeleteServicesNode(&RegAccess);    // Just in case
        return DRVCNF_OK;
    }

    //
    // Try to unload the driver
    //
    Unloaded = DrvUnloadKernelDriver(&RegAccess);

    //
    // Remove the driver entry from the registry
    //
    Deleted = DrvDeleteServicesNode(&RegAccess);

    if (Unloaded && Deleted) {
        return DRVCNF_RESTART;
    } else {
        if (Deleted) {
            return DRVCNF_OK;
        } else {
            /*
             *  Tell the user there's a problem
             */
            ConfigErrorMsgBox( hDlg, IDS_FAILREMOVE );

            return DRVCNF_CANCEL;
        }
    }
}

void Setupmidimapper()
{
    DrvQueryDeviceIdParameter(
        &RegAccess, 
        Configuration.CardId, 
        SOUND_REG_SYNTH_TYPE, 
        &Configuration.SynthType);
        
    if ( Configuration.SynthType != (DWORD)-1 )
        DrvSetMapperName(ESS_MAPPER_ESS_ESFM);
}

void  SetupSoundMapperName
(
    PREG_ACCESS RegAccess, 
    int DeviceNumber, 
    BYTE StringIdPlayback, 
    BYTE StringIdRecord
)
{
    HKEY hKey;
    TCHAR String[128];
    LONG ReturnCode;
    
    if (RegOpenKeyEx(
           HKEY_USERS,
           TEXT(".DEFAULT\\Software\\Microsoft\\Multimedia\\Sound Mapper"),
           0,
           KEY_ALL_ACCESS,
           &hKey
        ) != ERROR_SUCCESS) return;

    GetRealString(RegAccess, DeviceNumber, StringIdPlayback, String);
    RegSetValueEx(hKey,  TEXT("Playback"), 0, REG_SZ, (PBYTE)String, ByteCountOf(lstrlen(String)+1));
    GetRealString(RegAccess, DeviceNumber, StringIdRecord, String);
    ReturnCode = RegSetValueEx(hKey, TEXT("Record"), 0, REG_SZ, (PBYTE)String, ByteCountOf(lstrlen(String)+1));
    RegCloseKey(hKey);
    
    if (ReturnCode == ERROR_SUCCESS && 
        RegOpenKeyEx(
            HKEY_CURRENT_USER, 
            TEXT("Software\\Microsoft\\Multimedia\\Sound Mapper"), 
            0, 
            KEY_ALL_ACCESS, 
            &hKey
        ) == ERROR_SUCCESS)
    {
        GetRealString(RegAccess, DeviceNumber, StringIdPlayback, String);
        RegSetValueEx(hKey, TEXT("Playback"), 0, REG_SZ, (PBYTE)String, ByteCountOf(lstrlen(String)+1));
        GetRealString(RegAccess, DeviceNumber, StringIdRecord, String);
        RegSetValueExW(hKey, TEXT("Record"), 0, REG_SZ, (PBYTE)String, ByteCountOf(lstrlen(String)+1));
        RegCloseKey(hKey);
    }
}

void SetupDefaultRecordFormat()
{
    HKEY hKey;
    LONG ReturnCode;
    TCHAR DefaultFormat[24] = TEXT("Default Record Quality");
    BYTE DefaultQuality[36] = {
        1, 0, 1, 0, 34, 86, 0, 0, 68, 172, 0, 0, 
        2, 0, 16, 0, 32, 128, 77, 115, 120, 98, 51, 48, 
        51, 50, 46, 100, 0, 0, 0 ,0, 12, 1, 118, 0};

    if (RegOpenKeyEx(
            HKEY_CURRENT_USER, 
            TEXT("Software\\Microsoft\\Multimedia\\Audio"), 
            0, 
            KEY_ALL_ACCESS, 
            &hKey
        ) == ERROR_SUCCESS)
    {
        ReturnCode = RegSetValueEx(hKey, TEXT("DefaultFormat"), 0, REG_SZ, (PBYTE)DefaultFormat, ByteCountOf(lstrlen(DefaultFormat)+1));
        RegCloseKey(hKey);
        if (ReturnCode == ERROR_SUCCESS && 
            RegOpenKeyEx(
                HKEY_CURRENT_USER, 
                TEXT("Software\\Microsoft\\Multimedia\\Audio\\WaveFormats"), 
                0, 
                KEY_ALL_ACCESS, 
                &hKey
            ) == ERROR_SUCCESS)
        {
            RegSetValueEx(hKey, TEXT("Default Record Quality"), 0, REG_BINARY, DefaultQuality, sizeof(DefaultQuality));
            RegCloseKey(hKey);
        }
    }
}




/****************************************************************************
 * @doc INTERNAL
 *
 * @api int | Config | This puts up the configuration dialog box.
 *
 * @parm HWND | hWnd | Our Window handle.
 *
 * @parm HANDLE | hInstance | Our instance handle.
 *
 * @rdesc Returns whatever was returned from the dialog box procedure.
 ***************************************************************************/
int Config(HWND hWnd, HANDLE hInstance)
{
    BOOL      ReturnCode;
    BOOL      DriverWasLoaded;
    ES_CONFIG InitConfig;
    BOOL      AutoInstall;

    CurrentPort = (DWORD)-1;
    AutoInstall = FALSE;
    FirstTime   = TRUE;

    /*
    **  Find out what stuff is in use!
    */

    DriverWasLoaded = DrvIsDriverLoaded(&RegAccess);

    CurrentCard   = 0;

    DrvNumberOfDevices(&RegAccess, &NumberOfCards);

    if (!bInstall) {
	// Save configuration in case Dialog is cancelled
        GetDriverConfig(CurrentCard, &InitConfig);
    }


    if (NumberOfCards == 0) {
        HKEY hKey;
        hKey = DrvCreateDeviceKey(RegAccess.DriverName);
        RegCloseKey(hKey);

        DrvNumberOfDevices(&RegAccess, &NumberOfCards);

        if (NumberOfCards == 0) {
            ConfigErrorMsgBox(hWnd, IDS_ERROR_UNKNOWN);
            return DRVCNF_CANCEL;
        }
    }
    
    if (NumberOfCards == 1)
        ReadConfigini(TRUE);
    
    if (SOUND_DEF_ENABLE_AUTO_INSTALL)
    {
        BOOL Success;
        DWORD AutoInstall;
        
        if (!DriverWasLoaded)
            DrvSetDeviceIdParameter(&RegAccess, CurrentCard, SOUND_REG_AUTO_INSTALL, TRUE);
        Success = DrvConfigureDriver(&RegAccess, STR_DRIVERNAME, SoundDriverTypeNormal, NULL, NULL);
        DrvQueryDeviceIdParameter(&RegAccess, CurrentCard, SOUND_REG_AUTO_INSTALL, &AutoInstall);
        if (DriverWasLoaded && AutoInstall || Success)
        {
            ReadConfigini(TRUE);
            Setupmidimapper();
            DrvSetMapperName(ESS_MAPPER_ESS);
            SetupSoundMapperName(&RegAccess, CurrentCard, 104, 103);
            SetupDefaultRecordFormat();
            return DRVCNF_RESTART;
        }
        if (AutoInstall)
            DrvDeleteServicesNode(&RegAccess);
    }

    DrvSetDeviceIdParameter(&RegAccess, CurrentCard, SOUND_REG_AUTO_INSTALL, FALSE);

    if ( NumberOfCards == 1 )
        ReadConfigini(FALSE);

    ReturnCode = DialogBox( hInstance,
                            MAKEINTRESOURCE(DLG_ESSCONFIG),
                            hWnd,
                            (DLGPROC)ConfigDlgProc );

    if (ReturnCode == DRVCNF_CANCEL) {
        if (bInstall) {
            DrvRemoveDriver(&RegAccess);
        } else {
            //
            // only reload the driver if we know that the settings
            // changed.
            //
            if (!ESConfigEqual(&InitConfig, &Configuration))
            {
                SetDriverConfig(&InitConfig);
                if (DriverWasLoaded)
                {
                    DrvConfigureDriver(&RegAccess,
                                       STR_DRIVERNAME,
                                       SoundDriverTypeNormal,
                                       NULL,
                                       NULL);
                }
            }
        }
    }

    return ReturnCode;
}

/****************************************************************************
 *
 *  GetDriverConfig
 *
 *  Parameters:
 *
 *     UINT       CardId
 *
 *     PAS_CONFIG * Config
 *
 *  Returns:
 *
 *     Config copied into Config (unobtainable values set to (DWORD)-1)
 *
 ****************************************************************************/
VOID GetDriverConfig(UINT CardId, PES_CONFIG Config)
{
    Config->CardId         = CardId;

    /*
    **  Set up the defaults in case we get nothing from the registry
    */

    InitConfiguration(Config);

    DrvQueryDeviceIdParameter(&RegAccess,
                              CardId,
                              SOUND_REG_INTERRUPT,
                              &Config->Interrupt);

    if (Config->Interrupt == 9) {
        Config->Interrupt = 2;
    }

    DrvQueryDeviceIdParameter(&RegAccess,
                              CardId,
                              SOUND_REG_DMABUFFERSIZE,
                              &Config->DmaBufferSize);

    DrvQueryDeviceIdParameter(&RegAccess,
                              CardId,
                              SOUND_REG_SYNTH_TYPE,
                              &Config->SynthType);

}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api BOOL | SetDriverConfig | Callback to set config info in the registry
 *         does not write uninitialized values (-1)
 *
 * @parm PVOID | Context | Our context.
 *
 * @rdesc Returns TRUE if success, FALSE otherwise.
 ***************************************************************************/
BOOL SetDriverConfig(PES_CONFIG Config)
{
    if (Config->Interrupt != (DWORD)-1)
    {
        if (DrvSetDeviceIdParameter(
            &RegAccess,
            Config->CardId,
            SOUND_REG_INTERRUPT,
            Config->Interrupt == 2 ? 9 : Config->Interrupt) != ERROR_SUCCESS)
            return FALSE;
    }
    if (DrvSetDeviceIdParameter(
            &RegAccess,
            Config->CardId,
            SOUND_REG_DMABUFFERSIZE,
            Config->DmaBufferSize) != ERROR_SUCCESS ||
        DrvSetDeviceIdParameter(
            &RegAccess,
            Config->CardId,
            SOUND_REG_LOADTYPE,
            Config->LoadType) != ERROR_SUCCESS) {

        return FALSE;
    } else {
        return TRUE;
    }
}

VOID SetItem(HWND hDlg, UINT Combo, DWORD Value, DWORD Current)
{
    LRESULT Index;
    TCHAR   String[20];
    HWND    hwndCombo;

    hwndCombo = GetDlgItem(hDlg, Combo);

    if (hwndCombo == NULL) {
        //
        //  This can happen since we share some code between dialogs
        return;
    }
    if (Value == (DWORD)-1) {
        LoadString(hDriverModule1, IDS_DISABLED, String, sizeof(String) / sizeof(String[0]));
    } else {
        wsprintf(String, Combo == IDD_IRQCB ? TEXT("%d") : TEXT("%d"), Value);
    }
    Index = ComboBox_AddString(hwndCombo, String);
    if (Value == Current || Index == 0) {
        ComboBox_SetCurSel(hwndCombo, Index);
    }
    ComboBox_SetItemData(hwndCombo, Index, Value);
}

DWORD GetCurrentValue(HWND hDlg, UINT Combo)
{
    HWND    hwndCombo;

    hwndCombo = GetDlgItem(hDlg, Combo);
    if (hwndCombo == NULL) {
        /*
        **  -1 means nothing there
        */
        return (DWORD)-1;
    }

    return ComboBox_GetItemData(hwndCombo, ComboBox_GetCurSel(hwndCombo));
}

/*
**  Get the configuration the user has entered and put it in the
**  registry
*/

BOOL
SetCurrentConfig(HWND hDlg, UINT CardId, PES_CONFIG Config)
{
    Config->CardId = CardId;

    /*
    **  Get the new configuration which the user entered
    */

    Config->LoadType     = SOUND_LOADTYPE_NORMAL;
    Config->CardId       = CardId;
    Config->Interrupt    = GetCurrentValue(hDlg, IDD_IRQCB);

    /*
    **  Write it to the registry for the driver
    */

    SetDriverConfig(Config);

    return TRUE;
}

/*
**  Switch to a new card
*/

BOOL SetupDialog(HWND hDlg, UINT CardId )
{
    DWORD i;
    
    GetDriverConfig(CardId, &Configuration);


    /*
    ** First reset the combo boxes contents
    */
    ComboBox_ResetContent( GetDlgItem(hDlg, IDD_IOADDRESSCB));
    ComboBox_ResetContent( GetDlgItem(hDlg, IDD_IRQCB));

    /*
    **  Fill our combo boxes
    */
    if (Configuration.Interrupt == (DWORD)-1) {
        Configuration.Interrupt = SOUND_DEF_INT;  //7
    }
    
    for (i = 0; i < sizeof(IrqChoices) / sizeof(IrqChoices[0]); i++)
        SetItem(hDlg, IDD_IRQCB, IrqChoices[i], Configuration.Interrupt);

    
    {
        TCHAR OKString[30];

        LoadString(hDriverModule1,
                   IDS_OK,
                   OKString,
                   sizeof(OKString) / sizeof(OKString[0]));

        SetWindowText(GetDlgItem(hDlg, IDOK), OKString);
    }
    return TRUE;
}

//------------------------------------------------------------------------
//  int AdvancedDlgProc
//
//  Description:
//     Dialog procedure for the configuration dialog box.
//
//  Parameters:
//     HWND hDlg
//        handle to configuration dialog box
//
//     UINT uMsg
//        message
//
//     WPARAM wParam
//        message dependent parameter
//
//     LPARAM lParam
//        message dependent parameter
//
//  Return Value:
//     DRV_OK on success, otherwise DRV_CANCEL
//
//------------------------------------------------------------------------

BOOL CALLBACK AdvancedDlgProc
(
    HWND            hDlg,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam
)
{
   BOOL fWasted ;
   int  nDMABufferSize ;

#define MAX_DMA_BUFFER_SIZE 64

   switch ( uMsg )
   {
      case WM_INITDIALOG:
      {

         /*
         **  Center the Dialog Box
         */

         CenterPopup( hDlg, GetParent(hDlg) );
         SetDlgItemInt( hDlg,
                        IDD_DMABUFFEREC,
                        Configuration.DmaBufferSize / 0x400,
                        FALSE ) ;
      }
      break ;

      case WM_CLOSE:
         EndDialog( hDlg, DRV_CANCEL ) ;
         break ;

      case WM_COMMAND:
         switch ( wParam )
         {
#if 0
            case IDD_HELPADV:
               WinHelp( hDlg, gszHelpFile, HELP_CONTEXT, 6000 ) ;
               break ;
#endif

            case IDOK:
            {
               nDMABufferSize =
                  (DWORD) GetDlgItemInt( hDlg, IDD_DMABUFFEREC,
                                         &fWasted, TRUE );

               if ((nDMABufferSize < 4) || (nDMABufferSize > MAX_DMA_BUFFER_SIZE))
               {
                  ConfigErrorMsgBox( hDlg,
                                     IDS_BADDMABUFFERSIZE,
                                     MAX_DMA_BUFFER_SIZE ) ;
                  return ( FALSE ) ;
               }

               Configuration.DmaBufferSize = nDMABufferSize * 0x400;
               EndDialog( hDlg, TRUE ) ;
            }
            break ;

            case IDCANCEL:
               EndDialog(hDlg, FALSE ) ;
               break ;

            default:
               break ;
         }
         break ;
         
      case WM_VSCROLL:
         nDMABufferSize =
            (DWORD) GetDlgItemInt( hDlg, IDD_DMABUFFEREC,
                                   &fWasted, TRUE );
         
         switch ( wParam )
         {
            case SB_LINEDOWN:
            case SB_PAGEDOWN:
            {
               if (nDMABufferSize <= 4) nDMABufferSize = 4;
               else nDMABufferSize--;
               break;
            }
            break ;

            case SB_PAGEUP:
               if (nDMABufferSize < MAX_DMA_BUFFER_SIZE) nDMABufferSize++;
               break ;

            default:
               break ;
         }
         
         SetDlgItemInt( hDlg, IDD_DMABUFFEREC, nDMABufferSize, FALSE);
         break;

      default:
        return FALSE ;
    }

    return TRUE ;

} // end of AdvancedDlgProc()


/****************************************************************************
 * @doc INTERNAL
 *
 * @api int | ConfigDlgProc | Dialog proc for the configuration dialog box.
 *
 * @parm HWND | hDlg | Handle to the configuration dialog box.
 *
 * @parm WORD | msg | Message sent to the dialog box.
 *
 * @parm WORD | wParam | Message dependent parameter.
 *
 * @parm LONG | lParam | Message dependent parameter.
 *
 * @rdesc Returns DRV_RESTART if the user has changed settings, which will
 *     cause the drivers applet which launched this to give the user a
 *     message about having to restart Windows for the changes to take
 *     effect.  If the user clicks on "Cancel" or if no settings have changed,
 *     DRV_CANCEL is returned.
 ***************************************************************************/
int ConfigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {

    HANDLE_MSG( hwnd, WM_INITDIALOG, Config_OnInitDialog );
    HANDLE_MSG( hwnd, WM_COMMAND, Config_OnCommand );

    default:
        return FALSE;
    }

    return TRUE;
}


/*****************************Private*Routine******************************\
* Config_OnInitDialog
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
Config_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    )
{
    LOGFONT lf;
    int     iLogPelsY;
    HDC     hdc;

    D3(("ConfigDlgProc() - WM_INITDIALOG"));

    hdc = GetDC( hwnd );
    iLogPelsY = GetDeviceCaps( hdc, LOGPIXELSY );
    ReleaseDC( hwnd, hdc );

    ZeroMemory( &lf, sizeof(lf) );

#ifdef JAPAN
//fix kksuzuka: #2562
    lf.lfHeight = (-10 * iLogPelsY) / 72;    /* 10pt                        */
    lf.lfWeight = 400;                      /* normal                       */
    lf.lfCharSet = SHIFTJIS_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    lstrcpy( lf.lfFaceName, TEXT("System") );
#else
    lf.lfHeight = (-8 * iLogPelsY) / 72;    /* 8pt                          */
    lf.lfWeight = 400;                      /* normal                       */
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    lstrcpy( lf.lfFaceName, g_szAppFontName );
#endif
    g_hDlgFont = CreateFontIndirect(&lf);

    if (g_hDlgFont) {

        SendDlgItemMessage( hwnd, IDD_HELPTEXT, WM_SETFONT,
                            (WPARAM)g_hDlgFont, 0L );

        SendDlgItemMessage( hwnd, IDD_IRQCB, WM_SETFONT,
                            (WPARAM)g_hDlgFont, 0L );

    }


    /*
    ** Subclass the buttons
    */
    {
        HWND    hwndButton;

        hwndButton = GetDlgItem( hwnd, IDOK );
        if (hwndButton) {
            OldButtonProc = SubclassWindow( hwndButton,
                                            ButtonSubClassProc );
        }

        hwndButton = GetDlgItem( hwnd, IDCANCEL );
        if (hwndButton) {
            OldButtonProc = SubclassWindow( hwndButton,
                                            ButtonSubClassProc );
        }

        hwndButton = GetDlgItem( hwnd, IDD_ADVANCEDBTN );
        if (hwndButton) {
            OldButtonProc = SubclassWindow( hwndButton,
                                            ButtonSubClassProc );
        }
    }


    /*
    ** Subclass the combo boxes
    */
    {
        HWND    hwndCombo;

        hwndCombo = GetDlgItem( hwnd, IDD_IRQCB );
        if (hwndCombo) {
            OldComboProc = SubclassWindow( hwndCombo,
                                           ComboBoxSubClassProc );
        }
    }

    /*
    **  Display card data and centre the Dialog Box
    */
    GetOriginalControlPositions( hwnd );
    ResizeDialog( hwnd );
    SetDialogTitle( hwnd );
    SetupDialog(hwnd, CurrentCard);
    CenterPopup( hwnd, GetParent(hwnd) );

    /*
    **  Set the focus
    */

    SetFocus(GetDlgItem(hwnd, IDOK));

    return FALSE;  // FALSE means WE set the focus
}


/*****************************Private*Routine******************************\
* Config_OnCommand
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
Config_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    )
{
    DWORD ConfigReturn;

    switch ( id) {

    case IDOK:

        /*
        **  If we successfully configure then finish
        */

        ConfigReturn = Configure(hwnd);

        if ( DRVCNF_CONTINUE != ConfigReturn) {
            EndDialog( hwnd, ConfigReturn );
        } else {
            ResizeDialog( hwnd );
            SetDialogTitle( hwnd );

            /*
            **  Update the help for the button we've pressed
            */
            PrintHelpText( GetDlgItem(hwnd, IDOK ) );
            SetupDialog(hwnd, CurrentCard);
        }
        break;


    case IDCANCEL:
        EndDialog( hwnd, DRVCNF_CANCEL );
        break;


    case IDD_ABOUT:
        // About( hwnd );
        break;


    case IDD_ADVANCEDBTN:
        DialogBox(hInstance,
                  MAKEINTRESOURCE(DLG_ADVANCED),
                  hwnd,
                  (DLGPROC)AdvancedDlgProc);
        break;
    }
}





/**************************************************************************
 *
 * Function : Configure
 *
 * Arguments :
 *
 *     hDlg - Dialog window handle
 *
 **************************************************************************/
int Configure(HWND hDlg)
{
    BOOL      Success;

    /*
    **  Make sure the current settings are in the registry
    */

    SetCurrentConfig( hDlg, CurrentCard, &Configuration );

    Success = DrvConfigureDriver(&RegAccess,
                                 STR_DRIVERNAME,
                                 SoundDriverTypeNormal,
                                 NULL,
                                 NULL);

    /*
    **  Get the DriverEntry() load Status from the Kernel driver
    **  by reading the registry
    */

    /*
    ** See if we succeeded
    */
    if ( !DrvIsDriverLoaded(&RegAccess) ) {
        /*
        **  Search for what went wrong
        */

        UINT  CardId;
        DWORD DriverLoadStatus;
        BOOL  ErrorFound;
        DWORD ErrorStringId;


        /*
        **  Make sure we know how many devices we have
        */

        DrvNumberOfDevices(&RegAccess, &NumberOfCards);

        for (CardId = 0,
             DriverLoadStatus = SOUND_CONFIG_OK,
             ErrorFound = FALSE;

             CardId < NumberOfCards;

             CardId++) {
            if ( DrvQueryDeviceIdParameter(
                    &RegAccess,
                    CardId,
                    SOUND_REG_CONFIGERROR,
                    &DriverLoadStatus ) == ERROR_SUCCESS &&
                 DriverLoadStatus != SOUND_CONFIG_OK) {
                ErrorFound = TRUE;
                break;
            }
        }
        /*
        **  Point to failing card
        */

        if (ErrorFound) {
            CurrentCard = CardId;
        } else {
            return DRVCNF_CONTINUE;
        }

        /*
        **  This is a private interface to the Kernel driver
        **  Read the status and put up a dialog message
        */

#define CONFIGERR(_x_)                      \
            case _x_:                       \
                ErrorStringId = IDS_##_x_;  \
                break;


        switch (DriverLoadStatus) {
            CONFIGERR(SOUND_CONFIG_BADPORT)
            CONFIGERR(SOUND_CONFIG_BADDMA)
            CONFIGERR(SOUND_CONFIG_BADINT)
            CONFIGERR(SOUND_CONFIG_BAD_MPU401_PORT)
            CONFIGERR(SOUND_CONFIG_DMA_INUSE)
            CONFIGERR(SOUND_CONFIG_ERROR)
            CONFIGERR(SOUND_CONFIG_INT_INUSE)
            CONFIGERR(SOUND_CONFIG_MPU401_PORT_INUSE)
            CONFIGERR(SOUND_CONFIG_PORT_INUSE)
            CONFIGERR(SOUND_CONFIG_RESOURCE)
            default:
                ErrorStringId = IDS_SOUND_CONFIG_ERROR;
                break;
        }

        ConfigErrorMsgBox(hDlg, ErrorStringId);

        return DRVCNF_CONTINUE;
    } else {

        DWORD NewBufferSize;

        /*
        **  The driver may be loaded but it's possible we failed to unload it
        */
        if (!Success) {
            ConfigErrorMsgBox(hDlg, IDS_BUSY);
            return DRVCNF_CONTINUE;
        }

        if (bInstall) {
            /*
            **  Set up midi mapper
            */
            Setupmidimapper();

            /*
            **  Reset the Install Flag
            */

            bInstall = FALSE;
        }

        return DRVCNF_RESTART;
    }
}



/****************************************************************************
 * @doc INTERNAL
 *
 * @api int | About | This puts up the About dialog box.
 *
 * @parm HWND | hWnd | Our Window handle.
 *
 * @parm HANDLE | hInstance | Our instance handle.
 *
 * @rdesc Returns whatever was returned from the dialog box procedure.
 ***************************************************************************/
int About( HWND hWnd )
{
    return TRUE;
#if 0
    return DialogBox(hDriverModule1,
                     MAKEINTRESOURCE(DLG_ABOUT),
                     hWnd,
                     (DLGPROC) AboutDlgProc );
#endif
}



/*****************************************************************************
* CenterPopup( hWnd, hParentWnd )                                            *
*                                                                            *
*    hWnd              window handle                                         *
*    hParentWnd        parent window handle                                  *
*                                                                            *
* This routine centers the popup window in the screen or display             *
* using the window handles provided.  The window is centered over            *
* the parent if the parent window is valid.  Special provision               *
* is made for the case when the popup would be centered outside              *
* the screen - in this case it is positioned at the appropriate              *
* border.                                                                    *
*                                                                            *
*****************************************************************************/

BOOL FAR PASCAL CenterPopup( HWND   hWnd,
                             HWND   hParentWnd )

{
    int      xPopup;
    int      yPopup;
    int      cxPopup;
    int      cyPopup;
    int      cxScreen;
    int      cyScreen;
    int      cxParent;
    int      cyParent;
    RECT     rcWindow;

    /* retrieve main display dimensions */
    cxScreen = GetSystemMetrics( SM_CXSCREEN );
    cyScreen = GetSystemMetrics( SM_CYSCREEN );

    /* retrieve popup rectangle  */
    GetWindowRect( hWnd, (LPRECT)&rcWindow );

    /* calculate popup extents */
    cxPopup = rcWindow.right - rcWindow.left;
    cyPopup = rcWindow.bottom - rcWindow.top;

    /* calculate bounding rectangle */
    if ( hParentWnd ) {

       /* retrieve parent rectangle */
       GetWindowRect( hParentWnd, (LPRECT)&rcWindow );

       /* calculate parent extents */
       cxParent = rcWindow.right - rcWindow.left;
       cyParent = rcWindow.bottom - rcWindow.top;

       /* center within parent window */
       xPopup = rcWindow.left + ((cxParent - cxPopup)/2);
       yPopup = rcWindow.top + ((cyParent - cyPopup)/2);

       /* adjust popup x-location for screen size */
       if ( xPopup+cxPopup > cxScreen ) {
           xPopup = cxScreen - cxPopup;
       }

       /* adjust popup y-location for screen size */
       if ( yPopup+cyPopup > cyScreen ) {
           yPopup = cyScreen - cyPopup;
       }
    } else {
       /* center within entire screen */
       xPopup = (cxScreen - cxPopup) / 2;
       yPopup = (cyScreen - cyPopup) / 2;
    }

    /* move window to new location & display */

    MoveWindow( hWnd,
                xPopup > 0 ? xPopup : 0,
                yPopup > 0 ? yPopup : 0,
                cxPopup,
                cyPopup,
                TRUE );

    /* normal return */

    return( TRUE );

}


/*****************************************************************************

    ConfigErrorMsgBox()

*****************************************************************************/
void cdecl ConfigErrorMsgBox(HWND hDlg, UINT StringId, ...)
{

    TCHAR   szErrorBuffer[256];    /* buffer for error messages */
    TCHAR   szErrorString[256];
    va_list va;

    LoadString( hDriverModule1,
                StringId,
                szErrorString,
                sizeof(szErrorString) / sizeof(TCHAR));

    va_start(va, StringId);
    wvsprintf(szErrorBuffer, szErrorString, va);
    va_end(va);

    MessageBox( hDlg,
                szErrorBuffer,
                STR_PRODUCTNAME,
                MB_OK | MB_ICONEXCLAMATION);

}



/******************************Public*Routine******************************\
* ButtonSubClassProc
*
* If the button is receiving the focus display a suitable help
* message in the help text box.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
ButtonSubClassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if ( uMsg == WM_SETFOCUS) {
        PrintHelpText( hwnd );
    }

    return CallWindowProc(OldButtonProc, hwnd, uMsg, wParam, lParam);
}


/******************************Public*Routine******************************\
* ComboBoxSubClassProc
*
* If the combo box is receiving the focus display a suitable help
* message in the help text box.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
ComboBoxSubClassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    if ( uMsg == WM_SETFOCUS) {
        PrintHelpText( hwnd );
    }

    return CallWindowProc(OldComboProc, hwnd, uMsg, wParam, lParam);
}



/*****************************Private*Routine******************************\
* PrintHelpText
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
PrintHelpText(
    HWND hwnd
    )
{
}


void
MyShowWindow(HWND hwnd, int nCmdShow)
{
    ShowWindow(hwnd, nCmdShow);
    EnableWindow(hwnd, nCmdShow == SW_SHOW);
}

/******************************Public*Routine******************************\
* ResizeDialog
*
* Here we resize the dialog box according to the type of Sound Blaster
* that is being configured.  Also, any unecessary controls get hidden here.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
ResizeDialog(
    HWND hwnd
    )
{
    POINT   ptNewPos;
    HDWP    hdwp;
    LONG    lDecrement;


    /*
    ** Resize the dialog.  Start by calculating the relative adjustment
    ** for the main dialog and then decrement it from the help text frame
    ** and help text controls.  Also while were calculating the size
    ** adjustments show/hide the necessary controls.
    */

    ptNewPos.x = g_OrgRectDlg.right - g_OrgRectDlg.left;
    ptNewPos.y = g_OrgRectDlg.bottom - g_OrgRectDlg.top;

    MyShowWindow( GetDlgItem( hwnd, IDD_IRQCB ), SW_SHOW );
    MyShowWindow( GetDlgItem( hwnd, IDD_IRQCB_T ), SW_SHOW );
    MyShowWindow( GetDlgItem( hwnd, IDD_ADVANCEDBTN ), SW_HIDE );

    /*
    ** Resize the dialog box.
    */
    SetWindowPos( hwnd, HWND_TOP, 0, 0, ptNewPos.x, ptNewPos.y -
                  g_OrgRect[(IDD_DLG_BASE - IDD_HELPTEXTFRAME)].bottom +
                  g_OrgRect[(IDD_DLG_BASE - IDD_HELPTEXTFRAME)].top - 40,
                  SWP_NOMOVE | SWP_NOZORDER );

}



/******************************Public*Routine******************************\
* SetDialogTitle
*
* Adjusts the dialog box title to match the type of sound balster card
* being configured.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
SetDialogTitle(
    HWND hwnd
    )
{
    TCHAR   szTitle[80];

    LoadString( hDriverModule1, IDS_ESSCONFIG + IDS_DIALOG_BASE, szTitle, 80 );
    SetWindowText( hwnd, szTitle );
}



/*****************************Private*Routine******************************\
* GetOriginalControlPositions
*
* Get the original positions/sizes of the dialog box and all the controls
* this information is converted into dialog box co-ordinates and used in
* the ResizeDialog function above.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
GetOriginalControlPositions(
    HWND hwnd
    )
{
    int     i;

    GetWindowRect( hwnd, &g_OrgRectDlg );

    for ( i = 0; i < MAX_IDDS; i++ ) {

        GetWindowRect( GetDlgItem( hwnd, IDD_DLG_BASE + i ), &g_OrgRect[i] );
        MapWindowRect( HWND_DESKTOP, hwnd, &g_OrgRect[i] );
    }

}

 
 
/* Get line from config file */
static int getline(int fd, char *pszBuffer, int len)
{
    int i;
    BOOL fComment = FALSE;
    char c = 0;
    
    for (i = 0, len -= 2; i < len; i++)
    {
        if (!_lread(fd, &c, 1) || c == '\n')
            break;
        if (c == ';') fComment=TRUE;
        if (!fComment) pszBuffer[i++] = c;
    }
    if (c == '\n')
    {
        if (fComment) pszBuffer[i++] = '\r';
        pszBuffer[i++] = '\n';
    }
    pszBuffer[i] = 0;
    return i;
}

#define CfgIsKey(Name) (lstrcmpiA(Token,Name) == 0 && (Token = Token + sizeof(Name) - 1))
#define CfgSetDeviceParameter(Name,Value) \
    DrvSetDeviceIdParameter(&RegAccess, CurrentCard, Name, Value)

#define CfgSetDeviceParamInt(Destination,Name) \
    CfgSetDeviceParameter(Name,(Destination=atoi(Token)))
#define CfgSetDeviceParamHex(Destination,Name) \
    CfgSetDeviceParameter(Name,(Destination=strtol(Token,&endptr,16)))


void ReadConfigini(BOOL ParseOptions)
{
    static BOOL HasBeenHere = FALSE;
    BOOLEAN SaveIrqChoices = FALSE, Verbose = FALSE;
    char *endptr, SystemPath[MAX_PATH], Line[130], *Token;
    DWORD IrqIdx = 0;
    int fd, Len, LangId;
    long val;
    
    if (HasBeenHere) return;
    HasBeenHere = TRUE;
    
    GetSystemDirectoryA(SystemPath, sizeof(SystemPath));
    lstrcatA(SystemPath, "\\config.ini");
    
	fd  = _lopen(SystemPath, OF_READ);
	if (fd != -1)
    {
        while (Len = getline(fd, Line, sizeof(Line)))
        {
            if (Token = strtok(Line, "="))
            {
                LangId = atoi(Token);
                if (LangId < 100 || LangId > 221)
                {
                    if (ParseOptions)
                    {
                        if (CfgIsKey("es938installed"))
                            CfgSetDeviceParameter(TEXT("ES938"), (SOUND_DEF_ES938 = atoi(Token)));
                        else if (CfgIsKey("EnableAutoInstall"))
                            SOUND_DEF_ENABLE_AUTO_INSTALL = atoi(Token);
                        else if (CfgIsKey("DisableMicGain"))
                            CfgSetDeviceParamInt(SOUND_DEF_DISABLE_MICGAIN, SOUND_REG_DISABLE_MICGAIN);
                        else if (CfgIsKey("MutePhone"))
                            CfgSetDeviceParamInt(SOUND_DEF_MUTE_PHONE, SOUND_REG_MUTE_PHONE);
                        else if (CfgIsKey("2WireExtHwVol"))
                            CfgSetDeviceParamInt(SOUND_DEF_2WIRES_EXTHWVOL, SOUND_REG_2WIRES_EXTHWVOL);
                        else if (CfgIsKey("SingleTransferDMA"))
                            CfgSetDeviceParamInt(SOUND_DEF_SINGLE_TX_DMA, SOUND_REG_SINGLE_TX_DMA);
                        else if (CfgIsKey("3D-Limit"))
                            CfgSetDeviceParamInt(SOUND_DEF_3D_LIMIT, SOUND_REG_3D_LIMIT);
                        else if (CfgIsKey("SpatializerEnable"))
                            CfgSetDeviceParamInt(SOUND_DEF_SPATIALIZER_ENABLE, SOUND_REG_SPATIALIZER_ENABLE);
                        else if (CfgIsKey("New_Laptop.Sys"))
                             CfgSetDeviceParamInt(SOUND_DEF_NEW_LAPTOP_SYS, SOUND_REG_NEW_LAPTOP_SYS);
                        else if (CfgIsKey("defaultExtMidi"))
                             CfgSetDeviceParamInt(SOUND_DEF_DEFAULT_EXT_MIDI, SOUND_REG_DEFAULT_EXT_MIDI);
                        else if (CfgIsKey("DisableAuxB"))
                             CfgSetDeviceParamInt(SOUND_DEF_DISABLE_AUXB, SOUND_REG_DISABLE_AUXB);
                        else if (CfgIsKey("RecordMonitorMode"))
                            CfgSetDeviceParamInt(SOUND_DEF_RECORD_MONITOR_MODE, SOUND_REG_RECORD_MONITOR_MODE);
                        else if (CfgIsKey("EnableIIS"))
                            CfgSetDeviceParamInt(SOUND_DEF_ENABLE_IIS, SOUND_REG_ENABLE_IIS);
                        else if (CfgIsKey("enableMPU401"))
                            CfgSetDeviceParamInt(SOUND_DEF_ENABLE_MPU401, SOUND_REG_ENABLE_MPU401);
                        else if (CfgIsKey("SoftexAPM"))
                        {
                            CfgSetDeviceParameter(SOUND_REG_SOFTEXAPM,(val=atoi(Token)));
                            if (val) AddEssAPMEntry();
                        }
                        else if (CfgIsKey("DefaultMasterVol"))
                            CfgSetDeviceParamHex(val, SOUND_REG_DEFAULT_MASTER_VOL);
                        else if (CfgIsKey("DefaultRecordMicVol"))
                            CfgSetDeviceParamHex(val, SOUND_REG_DEFAULT_RECORDMIC_VOL);
                        else if (CfgIsKey("PollingMasterVol"))
                            CfgSetDeviceParamHex(val, SOUND_REG_POLLINGMASTER_VOL);
                        else if (CfgIsKey("ReducedDebounceExtHwVol"))
                            CfgSetDeviceParamHex(val, SOUND_REG_REDUCEDDECBOUNCEEXTHW_VOL);
                        else if (CfgIsKey("DefaultLineInMute"))
                            CfgSetDeviceParamHex(val, SOUND_REG_DEFAULT_LINEIN_MUTE);
                        else if (CfgIsKey("DefaultAuxBMute"))
                            CfgSetDeviceParamHex(val, SOUND_REG_DEFAULT_AUXB_MUTE);
                        else if (CfgIsKey("DefaultPCSpkrMute"))
                            CfgSetDeviceParamHex(val, SOUND_REG_DEFAULT_PCSPKR_MUTE);
                        else if (CfgIsKey("LoadDriverDuringBoot"))
                            LoadDriverDuringBoot = atoi(Token);
                    }
                    else if (lstrcmpiA(Token, "irqchoices") == 0)
                    {
                        memset(&IrqChoices, 0, sizeof(IrqChoices));
                        for (Token = strtok(NULL, ","); Token; Token = strtok(NULL, ","))
                            IrqChoices[IrqIdx++] = atoi(Token);
                        SaveIrqChoices = TRUE;
                    }
                    else if (CfgIsKey("defaultirq"))
                        SOUND_DEF_INT = atoi(Token);
                }
                else
                {
                    TCHAR tszKey[40], tszValue[40];
                    unsigned int i;
                    
                    // NB: Even though this is prone to  buffer overflows, inefficient, 
                    // buggy, that's how it  it is done in the original code...
                    for (i = 0; i < strlen(Token); i++)
                        tszKey[i] = Token[i];
                    tszKey[i] = 0;
                    tszKey[i + 1] = 0;
                    
                    Token += 4;
                    
                    for (i = 0; i < strlen(Token); i++)
                        tszValue[i] = Token[i];
                    tszValue[i] = 0;
                    tszValue[i + 1] = 0;
                    
                    DrvSetOptions(&RegAccess, CurrentCard, tszKey, tszValue);
                }
            }
        }
	    _lclose(fd);
        DrvSetDeviceIdParameter(&RegAccess, CurrentCard, SOUND_REG_VERBOSE, Verbose);
        if ( !ParseOptions )
        {
          if ( SaveIrqChoices )
            DrvSetBinaryValue(&RegAccess, CurrentCard, SOUND_REG_IRQCHOICES, (LPBYTE)IrqChoices, IrqIdx);
        }
        DrvSetDeviceIdParameter(&RegAccess, CurrentCard, SOUND_REG_ENABLE_AUTO_INSTALL, SOUND_DEF_ENABLE_AUTO_INSTALL);
	}
}

//------------------------------------------------------------------------
//  BOOL ESConfigEqual
//
//  Description:
//      Compares ES_CONFIG structures
//
//  Parameters:
//      Config1 - First structure to compare to
//
//      Config2 - Second structure to compare to
//
//  Return Value:
//      TRUE if the configs are equal, otherwise FALSE.
//
//------------------------------------------------------------------------
BOOL
ESConfigEqual(
    IN  PES_CONFIG Config1,
    IN  PES_CONFIG Config2
    )
{

    if ((Config1->Interrupt      == Config2->Interrupt) &&    
        (Config1->DmaBufferSize  == Config2->DmaBufferSize))
        return TRUE;
    else
        return FALSE;
}


/***************************************************************************
 *
 *  Function :
 *      DrvSetOptions
 *
 *  Parameters :
 *      ServiceNodeKey       Handle to the device services node key
 *      ValueName            Name of value to set
 *      Value                REG_SZ value to set
 *
 *  Return code :
 *
 *      Standard error code (see winerror.h)
 *
 *  Description :
 *
 *      Add the value to the device parameters section under the
 *      services node.
 *      This section is created if it does not already exist.
 *
 ***************************************************************************/

 LONG
 DrvSetOptions(
     PREG_ACCESS RegAccess,
     UINT   DeviceNumber,
     LPTSTR ValueName,
     LPTSTR Value)
 {
     HKEY ParmsKey;
     LONG ReturnCode;

     //
     //  ALWAYS create a key 0 - that way old drivers work
     //
     if (DeviceNumber == 0) {
         ParmsKey = DrvOpenRegKey(RegAccess->DriverName, TEXT("Device0"));
     } else {
         ParmsKey = DrvOpenDeviceKey(RegAccess->DriverName, DeviceNumber);
     }

     if (ParmsKey == NULL) {
         return ERROR_FILE_NOT_FOUND;
     }

     //
     // Write the value
     //


     ReturnCode = RegSetValueEx(ParmsKey,             // Registry handle
                                ValueName,            // Name of item
                                0,                    // Reserved 0
                                REG_SZ,               // Data type
                                (LPBYTE)Value,        // The value
                                ByteCountOf(lstrlen(Value)+1));       // Data length

     //
     // Free the handles we created
     //

     RegCloseKey(ParmsKey);

     return ReturnCode;
 }
 
 /***************************************************************************
 *
 *  Function :
 *      DrvSetBinaryValue
 *
 *  Parameters :
 *      ServiceNodeKey       Handle to the device services node key
 *      ValueName            Name of value to set
 *      Value                REG_BINARY value to set
 *
 *  Return code :
 *
 *      Standard error code (see winerror.h)
 *
 *  Description :
 *
 *      Add the value to the device parameters section under the
 *      services node.
 *      This section is created if it does not already exist.
 *
 ***************************************************************************/

 LONG
 DrvSetBinaryValue(
     PREG_ACCESS RegAccess,
     UINT   DeviceNumber,
     LPTSTR ValueName,
     LPBYTE Value,
     DWORD  ValueSize)
 {
     HKEY ParmsKey;
     LONG ReturnCode;

     //
     //  ALWAYS create a key 0 - that way old drivers work
     //
     if (DeviceNumber == 0) {
         ParmsKey = DrvOpenRegKey(RegAccess->DriverName, TEXT("Device0"));
     } else {
         ParmsKey = DrvOpenDeviceKey(RegAccess->DriverName, DeviceNumber);
     }

     if (ParmsKey == NULL) {
         return ERROR_FILE_NOT_FOUND;
     }

     //
     // Write the value
     //


     ReturnCode = RegSetValueEx(ParmsKey,             // Registry handle
                                ValueName,            // Name of item
                                0,                    // Reserved 0
                                REG_BINARY,           // Data type
                                (LPBYTE)Value ,       // The value
                                ValueSize);           // Data length

     //
     // Free the handles we created
     //

     RegCloseKey(ParmsKey);

     return ReturnCode;
 }

 /***************************************************************************
 *
 *  Function :
 *      GetRealString
 *
 *  Parameters :
 *      ServiceNodeKey       Handle to the device services node key
 *      StringId             Identifier of string to load
 *      Value                Buffer of 128 bytes for resulting value
 *
 *  Description :
 *
 *      Reads a String from the device parameters section under the
 *      services node identified by supplied string identifier.
 *
 ***************************************************************************/

 void
 GetRealString(
     PREG_ACCESS RegAccess,
     UINT   DeviceNumber,
     BYTE   StringId,
     LPTSTR Value)
 {
     HKEY ParmsKey;
     LONG ReturnCode;
     TCHAR ValueName[4];
     DWORD lpType = REG_SZ, ValueSize;

     memset(Value, 0, 128);
     
     if (!StringId) return;
     wsprintf(ValueName, TEXT("%d"), StringId);
     
     //
     //  ALWAYS create a key 0 - that way old drivers work
     //
     if (DeviceNumber == 0) {
         ParmsKey = DrvOpenRegKey(RegAccess->DriverName, TEXT("Device0"));
     } else {
         ParmsKey = DrvOpenDeviceKey(RegAccess->DriverName, DeviceNumber);
     }

     if (ParmsKey == NULL) {
         return;
     }

     //
     // Read the value
     //

     ValueSize = 130;
     ReturnCode = RegQueryValueEx(ParmsKey,             // Registry handle
                                  ValueName,            // Name of item
                                  0,                    // Reserved 0
                                  &lpType,              // Data type
                                  (LPBYTE)Value ,       // The value
                                  &ValueSize);          // Data length

     //
     // Free the handles we created
     //

     RegCloseKey(ParmsKey);
 }

VOID AddEssAPMEntry()
{
    TCHAR szAPM[] = {TEXT("essapm.exe")};
    HKEY hKey;

    if ( RegCreateKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), &hKey)  == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, TEXT("essapm"), 0, REG_SZ, (PBYTE)szAPM, (lstrlen(szAPM) + 1) * sizeof(TCHAR));
        RegCloseKey(hKey);
    }
}
