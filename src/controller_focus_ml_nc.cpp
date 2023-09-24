//*****************************************************************************
//*		controller_focus_ml_nc.cpp		(c) 2020 by Mark Sproul
//*
//*
//*
//*****************************************************************************
//*	AlpacaPi is an open source project written in C/C++
//*
//*	Use of this source code for private or individual use is granted
//*	Use of this source code, in whole or in part for commercial purpose requires
//*	written agreement in advance.
//*
//*	You may use or modify this source code in any way you find useful, provided
//*	that you agree that the author(s) have no warranty, obligations or liability.  You
//*	must determine the suitability of this source code for your use.
//*
//*	Redistributions of this source code must retain this copyright notice.
//*****************************************************************************
//*	Edit History
//*****************************************************************************
//*	Feb 22,	2020	<MLS> Sending focus and rotate commands working properly
//*	Feb 22,	2020	<MLS> Rotator compass working
//*	Feb 23,	2020	<MLS> Added steps per revolution and setting based on model
//*	Feb 23,	2020	<MLS> Added WindowTabFocuser
//*	Feb 24,	2020	<MLS> Added WindowTabGraph, WindowTabAuxMotor, WindowTabConfig
//*	Feb 24,	2020	<MLS> Window tabs fully working
//*	Feb 25,	2020	<MLS> USB control fully working
//*	Feb 25,	2020	<MLS> Added SendMoveAuxMotorCommand()
//*	Feb 25,	2020	<MLS> Aux motor control working in usb mode
//*	Feb 27,	2020	<MLS> Slowed down query rate
//*	Feb 27,	2020	<MLS> Added SendStopMotorsCommand()
//*	Feb 29,	2020	<MLS> Added support for NiteCrawler switches via alpaca
//*	Mar 16,	2020	<MLS> Added cHas_readall flag
//*	Mar 16,	2020	<MLS> Changed AlpacaGetStatus() to AlpacaGetStatus_ReadAll()
//*	Apr 19,	2020	<MLS> Fixed bug when trying to stop motors in USB mode
//*	Apr 19,	2020	<MLS> Added CloseUSBport()
//*	Apr 19,	2020	<MLS> Finished support for closing and reopening USB port
//*	Apr 21,	2020	<MLS> Added about box to focuser controller
//*	Apr 24,	2020	<MLS> Created controller_ml_nc.cpp
//*	Apr 24,	2020	<MLS> This will be a subclass from the controller_focuser class
//*	Apr 24,	2020	<MLS> Moonlite nitecrawler focuser working as subclass
//*	Nov 28,	2022	<MLS> Changed name to controller_focus_ml_nc.cpp
//*	Jun 19,	2023	<MLS> Updated constructor to use TYPE_REMOTE_DEV
//*	Jul  1,	2023	<MLS> Added DeviceState window to NiteCrawler controller
//*	Jul  8,	2023	<MLS> Simplified Update routines
//*	Jul 14,	2023	<MLS> Added UpdateOnlineStatus() to NiteCrawler Controller
//*****************************************************************************
//*	From the Nitecrawler web site
//*	Rotating drawtube .001 degree resolution
//*	WR25 374,920 steps per revolution
//*	WR30 444,080 steps per revolution
//*	WR35 505,960 steps per revolution
//*	Approx. .9  travel, 94,580 steps per inch resolution
//*	.0000105 inch per step or .2667 Microns per step
//*****************************************************************************

#ifdef _ENABLE_CTRL_FOCUSERS_

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>

#include	"discovery_lib.h"
#include	"sendrequest_lib.h"
#include	"helper_functions.h"


#define _ENABLE_CONSOLE_DEBUG_
#include	"ConsoleDebug.h"

#include	"windowtab_nitecrawler.h"
#include	"windowtab_auxmotor.h"
#include	"windowtab_DeviceState.h"
#include	"windowtab_graphs.h"
#include	"windowtab_config.h"


#include	"controller.h"
#include	"controller_focus.h"
#include	"controller_focus_ml_nc.h"
#include	"focuser_common.h"




#define	_INCLUDE_GRAPHS_

//**************************************************************************************
enum
{
	kTab_Focuser	=	1,
	kTab_AuxMotor,
#ifdef _ENABLE_CONFIG_TAB_
	kTab_Config,
#endif // _ENABLE_CONFIG_TAB_
	kTab_Graphs,
	kTab_DeviceState,
	kTab_DriverInfo,
	kTab_About,

	kTab_Count
};

#include	"nitecrawler_colors.h"

//******************************************************************************
//*	<C 20# >00
//*	<C 01# >ffff
//*	<C 02# >0000
//*	<C 03# >39e7
//*	<C 04# >f800
//*	<C 05# >f800
//*	<C 06# >8000
//*	<C 07# >fe00
//*	<C 08# >07ff
//*	<C 12# >39e7
//*	<C 14# >ffff
//*	<C 11# >000f
//*	<C 13# >f800
//******************************************************************************
//******************************************************************************

TYPE_NC_COLORS	gNiteCrawlerColors[]	=
{
	{	"Foreground",		1,	0xffff,		0			},
	{	"Background",		2,	0x0000,		0			},
	{	"Box outline",		3,	0x39e7,		0			},
	{	"Focuser",			4,	0xf800,		0			},
	{	"Rotator",			5,	0xf800,		0			},
	{	"Aux",				6,	0x8000,		0			},
	{	"Temperature",		7,	0xfe00,		0			},
	{	"Voltage",			8,	0x07ff,		0			},
	{	"Button Text OFF",	11,	0x000F,		0			},
	{	"Button Text ON",	12,	0x39E7,		0			},
	{	"Button OFF",		13,	0xF800,		0			},
	{	"Button ON",		14,	0xffff,		0			},
	{	"---",				0,	0xffff,		0			},
	{	"---",				0,	0xffff,		0			},
	{	"---",				0,	0xffff,		0			},
	{	"---",				0,	0xffff,		0			},
	{	"---",				0,	0xffff,		0			},

};

#pragma mark -

//**************************************************************************************
ControllerNiteCrawler::ControllerNiteCrawler(	const char			*argWindowName,
												TYPE_REMOTE_DEV		*alpacaDevice)
					:ControllerFocus(		argWindowName,
											alpacaDevice,
											kFocuserType_NiteCrawler)
{

	CONSOLE_DEBUG(__FUNCTION__);

	//*	moved all init stuff to separate routine so we can have multiple constructors
	ControllerFocusInit(kComMode_Alpaca, kFocuserType_NiteCrawler);

	CreateWindowTabs();
}

//**************************************************************************************
ControllerNiteCrawler::ControllerNiteCrawler(	const char			*argWindowName,
												const char			*usbPortPath)
	:ControllerFocus(argWindowName, usbPortPath,  kFocuserType_NiteCrawler)
{
	CONSOLE_DEBUG(__FUNCTION__);

	cDriverInfoTabNum	=	kTab_DriverInfo;
	cValidIPaddr		=	false;

	//*	moved all init stuff to separate routine so we can have multiple constructors
	ControllerFocusInit(kComMode_USB, kFocuserType_NiteCrawler);

	if (usbPortPath != NULL)
	{
		strcpy(cUSBpath, usbPortPath);
		cValidUSB	=	true;
	}
	CreateWindowTabs();

	SetWindowIPaddrInfo(cUSBpath, true);

}


//**************************************************************************************
// Destructor
//**************************************************************************************
ControllerNiteCrawler::~ControllerNiteCrawler(void)
{
	CONSOLE_DEBUG_W_STR(__FUNCTION__, cWindowName);
	//*	delete the windowtab objects
	DELETE_OBJ_IF_VALID(cNiteCrawlerTabObjPtr);
	DELETE_OBJ_IF_VALID(cAuxTabObjPtr);
#ifdef _ENABLE_CONFIG_TAB_
	DELETE_OBJ_IF_VALID(cConfigTabObjPtr);
#endif // _ENABLE_CONFIG_TAB_
	DELETE_OBJ_IF_VALID(cGraphTabObjPtr);
	DELETE_OBJ_IF_VALID(cDeviceStateTabObjPtr);
	DELETE_OBJ_IF_VALID(cDriverInfoTabObjPtr);
	DELETE_OBJ_IF_VALID(cAboutBoxTabObjPtr);
}

//**************************************************************************************
void	ControllerNiteCrawler::CreateWindowTabs(void)
{
	CONSOLE_DEBUG(__FUNCTION__);

	cNiteCrawlerTabObjPtr	=	NULL;
	cAuxTabObjPtr			=	NULL;
#ifdef _ENABLE_CONFIG_TAB_
	cConfigTabObjPtr		=	NULL;
#endif // _ENABLE_CONFIG_TAB_
	cGraphTabObjPtr			=	NULL;
	cAboutBoxTabObjPtr		=	NULL;


	SetTabCount(kTab_Count);
	SetTabText(kTab_Focuser,		"Focuser");
	SetTabText(kTab_AuxMotor,		"Aux Motor");
#ifdef _ENABLE_CONFIG_TAB_
	SetTabText(kTab_Config,			"Config");
#endif // _ENABLE_CONFIG_TAB_
	SetTabText(kTab_Graphs,			"Graphs");
	SetTabText(kTab_DeviceState,	"Dev State");
	SetTabText(kTab_DriverInfo,		"Driver Info");
	SetTabText(kTab_About,			"About");

	//================================================================
	cNiteCrawlerTabObjPtr	=	new WindowTabNitecrawler(	cWidth,
															cHeight,
															cBackGrndColor,
															cCommMode,
															cWindowName);
	//*	tell the objects who there daddy is
	if (cNiteCrawlerTabObjPtr != NULL)
	{
		//*	set the ptrs for the tab objects
		SetTabWindow(kTab_Focuser,	cNiteCrawlerTabObjPtr);
		cNiteCrawlerTabObjPtr->SetParentObjectPtr(this);
	}

	//================================================================
	cAuxTabObjPtr			=	new WindowTabAuxMotor(	cWidth,
														cHeight,
														cBackGrndColor);
	if (cAuxTabObjPtr != NULL)
	{
		SetTabWindow(kTab_AuxMotor,	cAuxTabObjPtr);
		cAuxTabObjPtr->SetParentObjectPtr(this);
		if (cCommMode == kComMode_Alpaca)
		{
			cAuxTabObjPtr->DisableAuxMotorButons();
		}
	}



#ifdef _ENABLE_CONFIG_TAB_
	//================================================================
	cConfigTabObjPtr	=	new WindowTabConfig(	cWidth, cHeight, cBackGrndColor);
	if (cConfigTabObjPtr != NULL)
	{
		SetTabWindow(kTab_Config,	cConfigTabObjPtr);
		cConfigTabObjPtr->SetParentObjectPtr(this);
	}
#endif // _ENABLE_CONFIG_TAB_


	//================================================================
	cGraphTabObjPtr		=	new WindowTabGraph(cWidth, cHeight, cBackGrndColor);
	if (cGraphTabObjPtr != NULL)
	{
		SetTabWindow(kTab_Graphs,	cGraphTabObjPtr);
		cGraphTabObjPtr->SetParentObjectPtr(this);
	}

	//================================================================
	cDeviceStateTabObjPtr		=	new WindowTabDeviceState(	cWidth, cHeight, cBackGrndColor, cWindowName);
	if (cDeviceStateTabObjPtr != NULL)
	{
		SetTabWindow(kTab_DeviceState,	cDeviceStateTabObjPtr);
		cDeviceStateTabObjPtr->SetParentObjectPtr(this);
		SetDeviceStateTabInfo(kTab_DeviceState, kDeviceState_FirstBoxName, kDeviceState_FirstBoxValue, kDeviceState_Stats);
	}

	//================================================================
	cDriverInfoTabObjPtr		=	new WindowTabDriverInfo(	cWidth, cHeight, cBackGrndColor, cWindowName);
	if (cDriverInfoTabObjPtr != NULL)
	{
		SetTabWindow(kTab_DriverInfo,	cDriverInfoTabObjPtr);
		cDriverInfoTabObjPtr->SetParentObjectPtr(this);
	}

	//================================================================
	cAboutBoxTabObjPtr		=	new WindowTabAbout(	cWidth, cHeight, cBackGrndColor, cWindowName);
	if (cAboutBoxTabObjPtr != NULL)
	{
		SetTabWindow(kTab_About,	cAboutBoxTabObjPtr);
		cAboutBoxTabObjPtr->SetParentObjectPtr(this);
	}

	//*	display the IPaddres/port
	if (cValidIPaddr)
	{
	char	ipString[32];
	char	lineBuff[64];

		PrintIPaddressToString(cDeviceAddress.sin_addr.s_addr, ipString);
		sprintf(lineBuff, "%s:%d/%d", ipString, cPort, cAlpacaDevNum);
		SetWindowIPaddrInfo(lineBuff, true);
	}
}

//*****************************************************************************
void	ControllerNiteCrawler::AlpacaDisplayErrorMessage(const char *errorMsgString)
{
	CONSOLE_DEBUG_W_STR("Alpaca error=", errorMsgString);
//	SetWidgetText(kTab_Focuser, kNiteCrawlerTab_AlpacaErrorMsg, errorMsgString);
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateCommonProperties(void)
{
	CONSOLE_DEBUG(__FUNCTION__);

	SetWidgetText(kTab_DriverInfo,		kDriverInfo_Name,				cCommonProp.Name);
	SetWidgetText(kTab_DriverInfo,		kDriverInfo_Description,		cCommonProp.Description);
	SetWidgetText(kTab_DriverInfo,		kDriverInfo_DriverInfo,			cCommonProp.DriverInfo);
	SetWidgetText(kTab_DriverInfo,		kDriverInfo_DriverVersion,		cCommonProp.DriverVersion);
	SetWidgetNumber(kTab_DriverInfo,	kDriverInfo_InterfaceVersion,	cCommonProp.InterfaceVersion);

	UpdateAboutBoxRemoteDevice(kTab_About, kAboutBox_CPUinfo);
}

//**************************************************************************************
void	ControllerNiteCrawler::UpdateOnlineStatus(void)
{
cv::Scalar	bgColor;
cv::Scalar	txtColor;

	bgColor		=	cOnLine ? CV_RGB(0,		0,	0)	: CV_RGB(255,	0,	0);
	txtColor	=	cOnLine ? CV_RGB(255,	0,	0)	: CV_RGB(0,		0,	0);

	SetWidgetBGColor(	kTab_Focuser,		kNiteCrawlerTab_IPaddr,	bgColor);
	SetWidgetTextColor(	kTab_Focuser,		kNiteCrawlerTab_IPaddr,	txtColor);

	SetWidgetBGColor(	kTab_AuxMotor,		kAuxMotorBox_IPaddr,	bgColor);
	SetWidgetTextColor(	kTab_AuxMotor,		kAuxMotorBox_IPaddr,	txtColor);

	SetWidgetBGColor(	kTab_Graphs,		kGraphBox_IPaddr,	bgColor);
	SetWidgetTextColor(	kTab_Graphs,		kGraphBox_IPaddr,	txtColor);

	SetWidgetBGColor(	kTab_DeviceState,	kDeviceState_IPaddr,	bgColor);
	SetWidgetTextColor(	kTab_DeviceState,	kDeviceState_IPaddr,	txtColor);

	SetWidgetBGColor(	kTab_DriverInfo,	kDriverInfo_IPaddr,	bgColor);
	SetWidgetTextColor(	kTab_DriverInfo,	kDriverInfo_IPaddr,	txtColor);
}

//**************************************************************************************
void	ControllerNiteCrawler::UpdateConnectedStatusIndicator(void)
{
}

//**************************************************************************************
void	ControllerNiteCrawler::UpdateStartupData(void)
{
char	lineBuff[128];

	UpdateAboutBoxRemoteDevice(kTab_About, kAboutBox_CPUinfo);
	if (strlen(cSerialNumber) > 0)
	{
		sprintf(lineBuff, "%s-%s", cModelName, cSerialNumber);
	}
	else
	{
		strcpy(lineBuff,		cModelName);
	}
	SetWidgetText(kTab_Focuser, kNiteCrawlerTab_Model, lineBuff);
}

//**************************************************************************************
void	ControllerNiteCrawler::UpdateStatusData(void)
{
char			lineBuff[128];

//	CONSOLE_DEBUG(__FUNCTION__);
//	CONSOLE_DEBUG_W_NUM("cFocuserProp.Position\t=",	cFocuserProp.Position);
//	CONSOLE_DEBUG_W_NUM("cFocuserDesiredPos   \t=",	cFocuserDesiredPos);

	UpdateConnectedIndicator(	kTab_Focuser,	kNiteCrawlerTab_Connected);
	SetWidgetNumber(			kTab_Focuser,	kNiteCrawlerTab_focValue,	cFocuserProp.Position);
	SetWidgetNumber(			kTab_Focuser,	kNiteCrawlerTab_focDesired, cFocuserDesiredPos);

	SetWidgetNumber(			kTab_Focuser,	kNiteCrawlerTab_rotValue,	cRotatorPosition);
	SetWidgetNumber(			kTab_Focuser,	kNiteCrawlerTab_rotDesired, cRotatorDesiredPos);

	SetWidgetNumber(			kTab_AuxMotor,	kAuxMotorBox_Position,		cAuxMotorPosition);
	SetWidgetNumber(			kTab_AuxMotor,	kAuxMotorBox_DesiredPos,	cAuxMotorDesiredPos);

	sprintf(lineBuff, "%1.1f C / %1.1f F", cFocuserProp.Temperature_DegC, ((cFocuserProp.Temperature_DegC * (9.0/5.0)) + 32));
	SetWidgetText(kTab_Focuser, kNiteCrawlerTab_Temperature, lineBuff);
	if (cGraphTabObjPtr != NULL)
	{
		cGraphTabObjPtr->LogTemperature(cFocuserProp.Temperature_DegC);
		if (cCurrentTabNum == kTab_Graphs)
		{
			cUpdateWindow	=	true;
		}
	}

	UpdateWindowTabs_Everything();
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateRotatorPosition(void)
{
//	CONSOLE_DEBUG_W_NUM(__FUNCTION__, newRotatorPosition);
	SetWidgetNumber(kTab_Focuser, kNiteCrawlerTab_rotValue, cRotatorPosition);

	//*	tell the window tabs the new information
	if (cNiteCrawlerTabObjPtr != NULL)
	{
		cNiteCrawlerTabObjPtr->cRotatorPosition		=	cRotatorPosition;
		cNiteCrawlerTabObjPtr->cRotatorDesiredPos	=	cRotatorDesiredPos;
	}
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateVoltage(const double newVoltage)
{
char			lineBuff[128];
//	CONSOLE_DEBUG(__FUNCTION__);

	cVoltage	=	newVoltage;
	sprintf(lineBuff, "%1.1f V", cVoltage);
	SetWidgetText(kTab_Focuser, kNiteCrawlerTab_Voltage, lineBuff);

	if (cGraphTabObjPtr != NULL)
	{
		cGraphTabObjPtr->LogVoltage(cVoltage);
		if (cCurrentTabNum == kTab_Graphs)
		{
			cUpdateWindow	=	true;
		}
	}
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateStepsPerRev(const int newStepsPerRev)
{
//	CONSOLE_DEBUG(__FUNCTION__);
	cStepsPerRev	=	newStepsPerRev;
	if (cNiteCrawlerTabObjPtr != NULL)
	{
		cNiteCrawlerTabObjPtr->cStepsPerRev	=	newStepsPerRev;
	}
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateWindowTabs_Everything(void)
{
//	CONSOLE_DEBUG(__FUNCTION__);
	SetWidgetNumber(kTab_Focuser,	kNiteCrawlerTab_focDesired,		cFocuserDesiredPos);
	SetWidgetNumber(kTab_Focuser,	kNiteCrawlerTab_rotDesired,		cRotatorDesiredPos);
	SetWidgetNumber(kTab_AuxMotor,	kAuxMotorBox_DesiredPos,		cAuxMotorDesiredPos);

	UpdateConnectedIndicator(kTab_Focuser,		kNiteCrawlerTab_Connected);
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateSupportedActions(void)
{
//	CONSOLE_DEBUG(__FUNCTION__);
	SetWidgetValid(kTab_Focuser,	kNiteCrawlerTab_Readall,		cHas_readall);
	SetWidgetValid(kTab_Focuser,	kNiteCrawlerTab_DeviceState,	cHas_DeviceState);

	SetWidgetValid(kTab_AuxMotor,	kAuxMotorBox_Readall,			cHas_readall);
	SetWidgetValid(kTab_AuxMotor,	kAuxMotorBox_DeviceState,		cHas_DeviceState);
#ifdef _ENABLE_CONFIG_TAB_
	SetWidgetValid(kTab_Config,		kCongfigBox_Readall,			cHas_readall);
	SetWidgetValid(kTab_Config,		kCongfigBox_DeviceState,		cHas_DeviceState);
#endif

	SetWidgetValid(kTab_Graphs,		kGraphBox_Readall,				cHas_readall);
	SetWidgetValid(kTab_Graphs,		kGraphBox_DeviceState,			cHas_DeviceState);

	SetWidgetValid(kTab_DeviceState,	kDeviceState_Readall,		cHas_readall);
	SetWidgetValid(kTab_DeviceState,	kDeviceState_DeviceState,	cHas_DeviceState);

	SetWidgetValid(kTab_DriverInfo,	kDriverInfo_Readall,			cHas_readall);
	SetWidgetValid(kTab_DriverInfo,	kDriverInfo_DeviceState,		cHas_DeviceState);

	if (cHas_DeviceState == false)
	{
		cDeviceStateTabObjPtr->SetDeviceStateNotSupported();
	}
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateWindowTabs_SwitchBits(unsigned char switchBits)
{
//	CONSOLE_DEBUG(__FUNCTION__);

//	bit 0 for Rotation home switch
//	bit 1 for Out limit switch
//	bit 2 for In limit switch

	//========================================
	if (switchBits & 0x01)
	{
		SetWidgetBGColor(kTab_Focuser, kNiteCrawlerTab_SwitchROT, CV_RGB(255, 0, 0));
	}
	else
	{
		SetWidgetBGColor(kTab_Focuser, kNiteCrawlerTab_SwitchROT, kSwitchColorOff);
	}
	//========================================
	if (switchBits & 0x02)
	{
		SetWidgetBGColor(kTab_Focuser, kNiteCrawlerTab_SwitchOUT, CV_RGB(255, 0, 0));
	}
	else
	{
		SetWidgetBGColor(kTab_Focuser, kNiteCrawlerTab_SwitchOUT, kSwitchColorOff);
	}
	//========================================
	if (switchBits & 0x04)
	{
		SetWidgetBGColor(kTab_Focuser, kNiteCrawlerTab_SwitchIN, CV_RGB(255, 0, 0));
	}
	else
	{
		SetWidgetBGColor(kTab_Focuser, kNiteCrawlerTab_SwitchIN, kSwitchColorOff);
	}
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateWindowTabs_AuxSwitchBits(unsigned char auxSwitchBits)
{
//	CONSOLE_DEBUG(__FUNCTION__);

	//*	update the switch indicators
	if (auxSwitchBits & 0x01)
	{
		SetWidgetBGColor(kTab_AuxMotor, kAuxMotorBox_Switch1, CV_RGB(255, 0, 0));
	}
	else
	{
		SetWidgetBGColor(kTab_AuxMotor, kAuxMotorBox_Switch1, kSwitchColorOff);
	}
	//========================================
	if (auxSwitchBits & 0x02)
	{
		SetWidgetBGColor(kTab_AuxMotor, kAuxMotorBox_Switch2, CV_RGB(255, 0, 0));
	}
	else
	{
		SetWidgetBGColor(kTab_AuxMotor, kAuxMotorBox_Switch2, kSwitchColorOff);
	}
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateWindowTabs_DesiredFocusPos(const int newDesiredPoistion)
{
//	CONSOLE_DEBUG(__FUNCTION__);
	SetWidgetNumber(kTab_Focuser, kNiteCrawlerTab_focDesired, cFocuserDesiredPos);
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateWindowTabs_DesiredRotatorPos(const int newDesiredPoistion)
{
//	CONSOLE_DEBUG(__FUNCTION__);
	SetWidgetNumber(kTab_Focuser, kNiteCrawlerTab_rotDesired, cRotatorDesiredPos);
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateWindowTabs_DesiredAuxPos(const int newDesiredPoistion)
{
//	CONSOLE_DEBUG(__FUNCTION__);
	SetWidgetNumber(kTab_AuxMotor, kAuxMotorBox_DesiredPos, cAuxMotorDesiredPos);
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateWindowTabs_ConnectState(bool connectedState)
{
char	lineBuff[128];

//	CONSOLE_DEBUG(__FUNCTION__);

	if (connectedState)
	{
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_Connect, "Disconnect");
		if (strlen(cSerialNumber) > 0)
		{
			sprintf(lineBuff, "%s-%s", cModelName, cSerialNumber);
		}
		else
		{
			strcpy(lineBuff,		cModelName);
		}
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_Model, lineBuff);
	}
	else
	{
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_Connect,		"Connect");
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_Model,			"-----");
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_focValue,		"-----");
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_rotValue,		"-----");
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_focDesired,		"-----");
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_rotDesired,		"-----");
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_Temperature,	"-----");
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_Voltage,		"-----");
	}
	CONSOLE_ABORT(__FUNCTION__);
}

//*****************************************************************************
void	ControllerNiteCrawler::UpdateWindowTabs_SwitchState(int switchId, bool onOffState)
{
cv::Scalar switchColor;

	if (onOffState)
	{
		switchColor	=	CV_RGB(255, 0, 0);
	}
	else
	{
		switchColor	=	kSwitchColorOff;
	}
	switch(switchId)
	{
		case kSwitch_Out:
			SetWidgetBGColor(kTab_Focuser, kNiteCrawlerTab_SwitchOUT, switchColor);
			break;

		case kSwitch_In:
			SetWidgetBGColor(kTab_Focuser, kNiteCrawlerTab_SwitchIN, switchColor);
			break;

		case kSwitch_Rot:
			SetWidgetBGColor(kTab_Focuser, kNiteCrawlerTab_SwitchROT, switchColor);
			break;

	}
}

//*****************************************************************************
bool	ControllerNiteCrawler::AlpacaGetStatus(void)
{
bool	validData;
char	lineBuff[128];

//	CONSOLE_DEBUG_W_STR(__FUNCTION__, cWindowName);
	cFocuserProp.IsMoving	=	false;
	validData	=	false;
	if (cHas_readall)
	{
		validData	=	AlpacaGetStatus_ReadAll("focuser", cAlpacaDevNum);
		if (cFirstDataRead)
		{
			UpdateStartupData();
		}
		sprintf(lineBuff, "%s-%s", cModelName, cSerialNumber);
		SetWidgetText(kTab_Focuser, kNiteCrawlerTab_Model, lineBuff);

		SetWindowIPaddrInfo(NULL, true);

		if (cFocuserProp.Position != cFocuserDesiredPos)
		{
			cFocuserProp.IsMoving	=	true;
		}
		if (cRotatorPosition != cRotatorDesiredPos)
		{
			cFocuserProp.IsMoving	=	true;
		}
		cLastUpdate_milliSecs	=	millis();
	}
	else
	{
//		validData	=	AlpacaGetStatus_OneAAT();	//*	One At A Time
	}

	if (validData)
	{
		//*	check to see if we were one line before
		if (cOnLine == false)
		{
			//*	if we go from offline back to online, re-do the startup info
			cReadStartup	=	true;
		}
		cOnLine	=	true;
	}
	else
	{
		CONSOLE_DEBUG_W_STR("Offline-", cWindowName);
		cOnLine	=	false;
	}
	SetWindowIPaddrInfo(NULL, cOnLine);


	cLastUpdate_milliSecs	=	millis();
	return(validData);
}


#endif	//	_ENABLE_CTRL_FOCUSERS_

