//*****************************************************************************
//*		windowtab_alpacalist.cpp		(c) 2021 by Mark Sproul
//*
//*	Description:	C++ Client to talk to Alpaca devices
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
//*	Jan 13,	2021	<MLS> Created windowtab_alpacalist.cpp
//*	Jan 21,	2021	<MLS> Added sortable columns
//*	May 26,	2021	<MLS> Added support for FilterWheel controller window
//*****************************************************************************

#include	<stdlib.h>

#define _ENABLE_CONSOLE_DEBUG_
#include	"ConsoleDebug.h"


#include	"alpaca_defs.h"
#include	"discoverythread.h"
#include	"sendrequest_lib.h"

#include	"windowtab.h"
#include	"windowtab_alpacalist.h"

#include	"controller.h"
#include	"controller_cam_normal.h"
#include	"controller_covercalib.h"
#include	"controller_dome.h"
#include	"controller_filterwheel.h"
#include	"controller_focus.h"
#include	"controller_switch.h"
#include	"controller_telescope.h"
#include	"controller_skytravel.h"

//**************************************************************************************
WindowTabAlpacaList::WindowTabAlpacaList(	const int	xSize,
											const int	ySize,
											CvScalar	backGrndColor,
											const char	*windowName)
										:WindowTab(xSize, ySize, backGrndColor, windowName)
{
//	CONSOLE_DEBUG(__FUNCTION__);

	cAlpacaDevCnt		=	0;
	cPrevAlpacaDevCnt	=	0;
	cIncludeManagment	=	false;
	cSortColumn			=	-1;

	ClearRemoteDeviceList();

	SetupWindowControls();
}

//**************************************************************************************
// Destructor
//**************************************************************************************
WindowTabAlpacaList::~WindowTabAlpacaList(void)
{
	CONSOLE_DEBUG(__FUNCTION__);
}

//**************************************************************************************
void	WindowTabAlpacaList::SetupWindowControls(void)
{
int		xLoc;
int		yLoc;
int		textBoxHt;
int		textBoxWd;
int		widgetWidth;
int		iii;
short	tabArray[kMaxTabStops]	=	{200, 400, 600, 1000, 1199, 0};
int		clmnHdr_xLoc;
int		clmnHdrWidth;

//	CONSOLE_DEBUG(__FUNCTION__);

	//------------------------------------------
	yLoc			=	cTabVertOffset;

	//------------------------------------------
	SetWidget(kAlpacaList_Title,		0,			yLoc,		cWidth,		cTitleHeight);
	SetWidgetText(kAlpacaList_Title, "AlpacaPi Device List (double click on colored entries)");
	SetBGcolorFromWindowName(kAlpacaList_Title);
	yLoc			+=	cTitleHeight;
	yLoc			+=	2;

	clmnHdr_xLoc		=	1;
	iii	=	kAlpacaList_ClmTitle1;
	while(iii <= kAlpacaList_ClmTitle5)
	{
		clmnHdrWidth	=	tabArray[iii - kAlpacaList_ClmTitle1] - clmnHdr_xLoc;

//		CONSOLE_DEBUG_W_NUM("clmnHdr_xLoc\t=",	clmnHdr_xLoc);
//		CONSOLE_DEBUG_W_NUM("clmnHdrWidth\t=",	clmnHdrWidth);

		SetWidget(				iii,	clmnHdr_xLoc,			yLoc,		clmnHdrWidth,		cRadioBtnHt);
		SetWidgetType(			iii,	kWidgetType_Button);
		SetWidgetFont(			iii,	kFont_RadioBtn);
		SetWidgetBGColor(		iii,	CV_RGB(128,	128,	128));
		//	SetWidgetTextColor(		iii,	CV_RGB(255,	255,	255));
		SetWidgetTextColor(		iii,	CV_RGB(0,	0,	0));

		clmnHdr_xLoc	=	tabArray[iii - kAlpacaList_ClmTitle1];;
		clmnHdr_xLoc	+=	2;


		iii++;
	}
	SetWidgetText(		kAlpacaList_ClmTitle1,	"ip-address : port");
	SetWidgetText(		kAlpacaList_ClmTitle2,	"/etc/hosts");
	SetWidgetText(		kAlpacaList_ClmTitle3,	"type");
	SetWidgetText(		kAlpacaList_ClmTitle4,	"name");
	SetWidgetText(		kAlpacaList_ClmTitle5,	"InterfaceVersion");
	yLoc			+=	cRadioBtnHt;
	yLoc			+=	2;


	//=======================================================
	xLoc		=	10;
	textBoxHt	=	14;
	textBoxWd	=	cWidth - (xLoc + 3);
	for (iii=kAlpacaList_AlpacaDev_01; iii<=kAlpacaList_AlpacaDev_Last; iii++)
	{
		SetWidget(				iii,	xLoc,			yLoc,		textBoxWd,		textBoxHt);
		SetWidgetJustification(	iii,	kJustification_Left);
		SetWidgetFont(			iii,	kFont_TextList);
		SetWidgetTextColor(		iii,	CV_RGB(255,	255,	255));
		SetWidgetBorder(		iii,	false);
		SetWidgetTabStops(		iii,	tabArray);

		yLoc			+=	textBoxHt;
		yLoc			+=	3;
	}

	//---------------------------------------------------------------------
	xLoc		=	0;
	widgetWidth	=	cWidth / 4;
	SetWidget(				kAlpacaList_AlpacaDev_Total,	xLoc,	yLoc,	widgetWidth,	cSmallBtnHt);
	SetWidgetFont(			kAlpacaList_AlpacaDev_Total,	kFont_Medium);
	SetWidgetText(			kAlpacaList_AlpacaDev_Total,	"Total units =?");
	SetWidgetJustification(	kAlpacaList_AlpacaDev_Total,	kJustification_Left);
	SetWidgetTextColor(		kAlpacaList_AlpacaDev_Total,	CV_RGB(255,	255,	255));

	xLoc		+=	widgetWidth;
	xLoc		+=	2;

	//---------------------------------------------------------------------
	widgetWidth	=	cWidth / 5;
	SetWidget(			kAlpacaList_ChkBx_IncManagment,	xLoc,	yLoc,		widgetWidth,	cSmallBtnHt);
	SetWidgetFont(		kAlpacaList_ChkBx_IncManagment,	kFont_Medium);
	SetWidgetType(		kAlpacaList_ChkBx_IncManagment,	kWidgetType_CheckBox);
	SetWidgetText(		kAlpacaList_ChkBx_IncManagment,	"Include Management");
	SetWidgetChecked(	kAlpacaList_ChkBx_IncManagment, cIncludeManagment);

	xLoc		+=	widgetWidth;
	xLoc		+=	2;

	//---------------------------------------------------------------------
	widgetWidth	=	cWidth / 8;
	SetWidget(			kAlpacaList_Btn_Refresh,	xLoc,	yLoc,		widgetWidth,		cSmallBtnHt);
	SetWidgetFont(		kAlpacaList_Btn_Refresh,	kFont_Medium);
	SetWidgetType(		kAlpacaList_Btn_Refresh,	kWidgetType_Button);
	SetWidgetBGColor(	kAlpacaList_Btn_Refresh,	CV_RGB(255,	255,	255));
	SetWidgetText(		kAlpacaList_Btn_Refresh,	"Refresh");

	xLoc		+=	widgetWidth;
	xLoc		+=	2;


	//---------------------------------------------------------------------
	SetWidget(			kAlpacaList_Btn_CloseAll,	xLoc,	yLoc,		widgetWidth,		cSmallBtnHt);
	SetWidgetFont(		kAlpacaList_Btn_CloseAll,	kFont_Medium);
	SetWidgetType(		kAlpacaList_Btn_CloseAll,	kWidgetType_Button);
	SetWidgetBGColor(	kAlpacaList_Btn_CloseAll,	CV_RGB(255,	255,	255));
	SetWidgetText(		kAlpacaList_Btn_CloseAll,	"Close All");

	xLoc		+=	widgetWidth;
	xLoc		+=	2;

	yLoc			+=	cTitleHeight;
	yLoc			+=	2;

	SetAlpacaLogoBottomCorner(kAlpacaList_AlpacaLogo);

	//=======================================================
	//*	IP address
//	SetIPaddressBoxes(kAlpacaList_IPaddr, kAlpacaList_Readall, kAlpacaList_AlpacaDrvrVersion, -1);
//	SetIPaddressBoxes(kAlpacaList_IPaddr, kAlpacaList_Readall, -1, -1);
}

//*****************************************************************************
void	ToLowerStr(char *theString)
{
int		ii;

	ii	=	0;
	while (theString[ii] > 0)
	{
		theString[ii]	=	tolower(theString[ii]);
		ii++;
	}
}

//*****************************************************************************
void	GetInterfaceVersion(TYPE_REMOTE_DEV *remoteDevice)
{
SJP_Parser_t	jsonParser;
bool			validData;
int				jjj;
char			getFunctionString[256];
int				alpacaErrorCode;

//	CONSOLE_DEBUG(__FUNCTION__);

	SJP_Init(&jsonParser);
	//	http://wo102:6800/api/v1/filterwheel/0/interfaceversion
	sprintf(getFunctionString, "/api/v1/%s/%d/interfaceversion",	remoteDevice->deviceTypeStr,
																	remoteDevice->alpacaDeviceNum);

	ToLowerStr(getFunctionString);
//	CONSOLE_DEBUG(getFunctionString);
	validData	=	GetJsonResponse(	&remoteDevice->deviceAddress,
										remoteDevice->port,
										getFunctionString,
										NULL,
										&jsonParser);
	if (validData)
	{
//		CONSOLE_DEBUG_W_NUM("tokenCount_Data", jsonParser.tokenCount_Data);
		for (jjj=0; jjj<jsonParser.tokenCount_Data; jjj++)
		{
			if (strcasecmp(jsonParser.dataList[jjj].keyword, "Value") == 0)
			{
				remoteDevice->interfaceVersion	=	atoi(jsonParser.dataList[jjj].valueString);
			}
			else if (strcasecmp(jsonParser.dataList[jjj].keyword, "ErrorNumber") == 0)
			{
				alpacaErrorCode	=	atoi(jsonParser.dataList[jjj].valueString);
				if (alpacaErrorCode != 0)
				{
					CONSOLE_DEBUG_W_NUM("Alpaca error code", alpacaErrorCode);
					remoteDevice->interfaceVersion	=	-1;
				}
			}
			else
			{
			//	CONSOLE_DEBUG(jsonParser.dataList[jjj].keyword);
			//	CONSOLE_DEBUG(jsonParser.dataList[jjj].valueString);
			}
		}
	}
	else
	{
		CONSOLE_DEBUG("Data not valid");
	}
}


//*****************************************************************************
void	WindowTabAlpacaList::RunBackgroundTasks(void)
{
int		iii;


	for (iii=0; iii<cAlpacaDevCnt; iii++)
	{
		if (cRemoteDeviceList[iii].validEntry)
		{
			if (cRemoteDeviceList[iii].interfaceVersion == 0)
			{
				GetInterfaceVersion(&cRemoteDeviceList[iii]);
				break;
			}
		}
	}
}

//*****************************************************************************
void	WindowTabAlpacaList::ProcessButtonClick(const int buttonIdx)
{
//	CONSOLE_DEBUG(__FUNCTION__);

	switch(buttonIdx)
	{
		case kAlpacaList_ClmTitle1:
		case kAlpacaList_ClmTitle2:
		case kAlpacaList_ClmTitle3:
		case kAlpacaList_ClmTitle4:
		case kAlpacaList_ClmTitle5:
			cSortColumn	=	buttonIdx - kAlpacaList_ClmTitle1;
			UpdateSortOrder();
			ForceUpdate();
			break;

		case kAlpacaList_ChkBx_IncManagment:
			cIncludeManagment	=	!cIncludeManagment;
			SetWidgetChecked(kAlpacaList_ChkBx_IncManagment, cIncludeManagment);
			//*	fall through to force refresh

		case kAlpacaList_Btn_Refresh:
			ClearRemoteDeviceList();
			ForceUpdate();
			break;

		case kAlpacaList_Btn_CloseAll:
			CloseAllExceptFirst();
			break;
	}
}

//*****************************************************************************
void	WindowTabAlpacaList::ProcessDoubleClick(	const int	widgetIdx,
													const int	event,
													const int	xxx,
													const int	yyy,
													const int	flags)
{
int		deviceIdx;
char	windowName[64];
char	myHostName[64];
bool	windowExists;
//	CONSOLE_DEBUG(__FUNCTION__);
//	CONSOLE_DEBUG_W_NUM("widgetIdx\t=", widgetIdx);

	if ((widgetIdx >= kAlpacaList_AlpacaDev_01) && (widgetIdx <= kAlpacaList_AlpacaDev_Last))
	{
		deviceIdx	=	widgetIdx - kAlpacaList_AlpacaDev_01;
		if (deviceIdx >= 0)
		{
			if (strlen(cRemoteDeviceList[deviceIdx].hostName) > 0)
			{
				strcpy(myHostName, cRemoteDeviceList[deviceIdx].hostName);
			}
			else
			{
				inet_ntop(AF_INET, &(cRemoteDeviceList[deviceIdx].deviceAddress.sin_addr), myHostName, INET_ADDRSTRLEN);
			}
			//*	generate the window name
			sprintf(windowName, "%s-%s-%d",	cRemoteDeviceList[deviceIdx].deviceTypeStr,
											myHostName,
											cRemoteDeviceList[deviceIdx].alpacaDeviceNum);

			switch(cRemoteDeviceList[deviceIdx].deviceTypeEnum)
			{

			case kDeviceType_Camera:
				windowExists	=	CheckForOpenWindowByName(windowName);
				if (windowExists)
				{
					CONSOLE_DEBUG_W_STR("Window already open:", windowName);
				}
				else
				{
					new ControllerCamNormal(windowName, &cRemoteDeviceList[deviceIdx]);
				}
				break;

			case kDeviceType_CoverCalibrator:
				windowExists	=	CheckForOpenWindowByName(windowName);
				if (windowExists)
				{
					CONSOLE_DEBUG_W_STR("Window already open:", windowName);
				}
				else
				{
					new ControllerCoverCalib(	windowName,
												&cRemoteDeviceList[deviceIdx].deviceAddress,
												cRemoteDeviceList[deviceIdx].port,
												cRemoteDeviceList[deviceIdx].alpacaDeviceNum);
				}
				break;

			case kDeviceType_Dome:
				windowExists	=	CheckForOpenWindowByName(windowName);
				if (windowExists)
				{
					CONSOLE_DEBUG_W_STR("Window already open:", windowName);
				}
				else
				{
					new ControllerDome(windowName, &cRemoteDeviceList[deviceIdx]);
				}
				break;

			case kDeviceType_Filterwheel:
				windowExists	=	CheckForOpenWindowByName(windowName);
				if (windowExists)
				{
					CONSOLE_DEBUG_W_STR("Window already open:", windowName);
				}
				else
				{
					new ControllerFilterWheel(windowName, &cRemoteDeviceList[deviceIdx]);
				}
				break;

			case kDeviceType_Focuser:
				GenerateFocuserWindowName(&cRemoteDeviceList[deviceIdx], 1, windowName);
				windowExists	=	CheckForOpenWindowByName(windowName);
				if (windowExists)
				{
					CONSOLE_DEBUG_W_STR("Window already open:", windowName);
				}
				else
				{
					CheckForFocuser(&cRemoteDeviceList[deviceIdx]);
				}
				break;

			case kDeviceType_Management:
			case kDeviceType_Observingconditions:
			case kDeviceType_Rotator:
				break;

			case kDeviceType_Telescope:
				windowExists	=	CheckForOpenWindowByName(windowName);
				if (windowExists)
				{
					CONSOLE_DEBUG_W_STR("Window already open:", windowName);
				}
				else
				{
					new ControllerTelescope(	windowName,
												&cRemoteDeviceList[deviceIdx].deviceAddress,
												cRemoteDeviceList[deviceIdx].port,
												cRemoteDeviceList[deviceIdx].alpacaDeviceNum);
				}
				break;

			case kDeviceType_SafetyMonitor:
				break;

			case kDeviceType_Switch:
				windowExists	=	CheckForOpenWindowByName(windowName);
				if (windowExists)
				{
					CONSOLE_DEBUG_W_STR("Window already open:", windowName);
				}
				else
				{
					new ControllerSwitch(	windowName,
											&cRemoteDeviceList[deviceIdx].deviceAddress,
											cRemoteDeviceList[deviceIdx].port,
											cRemoteDeviceList[deviceIdx].alpacaDeviceNum);
				}
				break;

			//*	extras defined by MLS
			case kDeviceType_Multicam:
			case kDeviceType_Shutter:
			case kDeviceType_SlitTracker:
			case kDeviceType_undefined:
			case kDeviceType_last:
				break;
			}
		}
		else
		{
			CONSOLE_DEBUG_W_NUM("widgetIdx\t=", widgetIdx);
			CONSOLE_DEBUG_W_NUM("deviceIdx\t=", deviceIdx);
			CONSOLE_ABORT(__FUNCTION__);
		}
	}
}

//**************************************************************************************
void	WindowTabAlpacaList::ClearRemoteDeviceList(void)
{
int		iii;

	for (iii=kAlpacaList_AlpacaDev_01; iii<=kAlpacaList_AlpacaDev_Last; iii++)
	{
		SetWidgetTextColor(		iii,	CV_RGB(255,	255,	255));
		SetWidgetText(			iii,	"");
	}

	for (iii=0; iii<kMaxDeviceCnt; iii++)
	{
		memset(&cRemoteDeviceList[iii], 0, sizeof(TYPE_REMOTE_DEV));
	}
	cAlpacaDevCnt		=	0;
	cPrevAlpacaDevCnt	=	-1;
	cSortColumn			=	-1;

	SetWidgetBGColor(kAlpacaList_ClmTitle1,	((cSortColumn == 0) ? CV_RGB(255,	255,	255) : CV_RGB(128,	128,	128)));
	SetWidgetBGColor(kAlpacaList_ClmTitle2,	((cSortColumn == 1) ? CV_RGB(255,	255,	255) : CV_RGB(128,	128,	128)));
	SetWidgetBGColor(kAlpacaList_ClmTitle3,	((cSortColumn == 2) ? CV_RGB(255,	255,	255) : CV_RGB(128,	128,	128)));
	SetWidgetBGColor(kAlpacaList_ClmTitle4,	((cSortColumn == 3) ? CV_RGB(255,	255,	255) : CV_RGB(128,	128,	128)));
	SetWidgetBGColor(kAlpacaList_ClmTitle5,	((cSortColumn == 4) ? CV_RGB(255,	255,	255) : CV_RGB(128,	128,	128)));

}

//**************************************************************************************
void	WindowTabAlpacaList::UpdateRemoteDeviceList(void)
{
int		iii;
bool	includeDevice;
int		foundIndex;

//	CONSOLE_DEBUG(__FUNCTION__);
//	CONSOLE_DEBUG_W_NUM("gRemoteCnt\t=", gRemoteCnt);

	for (iii=0; iii<gRemoteCnt; iii++)
	{
		//*	first, see if this device is already in our list.
		foundIndex		=	FindDeviceInList(&gRemoteList[iii], cRemoteDeviceList, kMaxDeviceCnt);
		if (foundIndex >= 0)
		{
			//*	its already in the list
		}
		else
		{
			includeDevice	=	true;

			//		CONSOLE_DEBUG_W_STR("deviceTypeStr\t=", gRemoteList[iii].deviceTypeStr);
			if ((cIncludeManagment == false) && (strcasecmp(gRemoteList[iii].deviceTypeStr, "management") == 0))
			{
				includeDevice	=	false;
			}
			if (includeDevice)
			{
				if (cAlpacaDevCnt < kMaxDeviceCnt)
				{
					cRemoteDeviceList[cAlpacaDevCnt]			=	gRemoteList[iii];
					cRemoteDeviceList[cAlpacaDevCnt].validEntry	=	true;

					cAlpacaDevCnt++;
				}
			}
		}
	}

	//*	now update the widget text
	UpdateOnScreenWidgetList();

	if (cAlpacaDevCnt != cPrevAlpacaDevCnt)
	{
		ForceUpdate();
		cPrevAlpacaDevCnt	=	cAlpacaDevCnt;
	}
}

//**************************************************************************************
void	WindowTabAlpacaList::UpdateOnScreenWidgetList(void)
{
int		boxId;
int		iii;
char	textString[128];
char	ipAddrStr[32];
int		myDevCount;

//	CONSOLE_DEBUG(__FUNCTION__);

	iii			=	0;
	myDevCount	=	0;
	boxId		=	0;
	while ((iii < kMaxDeviceCnt))
	{
		boxId	=	iii + kAlpacaList_AlpacaDev_01;
		if ((boxId <= kAlpacaList_AlpacaDev_Last) && (cRemoteDeviceList[iii].validEntry))
		{
			inet_ntop(AF_INET, &(cRemoteDeviceList[iii].deviceAddress.sin_addr), ipAddrStr, INET_ADDRSTRLEN);

			sprintf(textString, "%s:%d\t%s\t%s\t%s\t%d",
									ipAddrStr,
									cRemoteDeviceList[iii].port,
									cRemoteDeviceList[iii].hostName,
									cRemoteDeviceList[iii].deviceTypeStr,
									cRemoteDeviceList[iii].deviceNameStr,
									cRemoteDeviceList[iii].interfaceVersion);


			SetWidgetText(boxId, textString);


			switch(cRemoteDeviceList[iii].deviceTypeEnum)
			{
				case kDeviceType_Camera:
					SetWidgetTextColor(		boxId,	CV_RGB(255,	255,	0));		//*	yellow
					break;

				case kDeviceType_CoverCalibrator:
					SetWidgetTextColor(		boxId,	CV_RGB(255,	0,		0));		//*	Red
					break;

				case kDeviceType_Dome:
					SetWidgetTextColor(		boxId,	CV_RGB(255,	0,		255));		//*	magenta
					break;

				case kDeviceType_Filterwheel:
					SetWidgetTextColor(		boxId,	CV_RGB(255,	127,	0));		//*	orage
					break;

				case kDeviceType_Focuser:
					SetWidgetTextColor(		boxId,	CV_RGB(0,	255,	0));		//*	green
					break;

				case kDeviceType_Switch:
					SetWidgetTextColor(		boxId,	CV_RGB(0,	255,	255));		//*	cyan
					break;

				case kDeviceType_Telescope:
					SetWidgetTextColor(		boxId,	CV_RGB(100,	100,	255));		//*	blue
					break;

				default:
					SetWidgetTextColor(		boxId,	CV_RGB(255,	255,	255));
					break;
			}

			myDevCount++;
		}
		else if (boxId <= kAlpacaList_AlpacaDev_Last)
		{
			SetWidgetText(boxId, "---");
		//	SetWidgetNumber(boxId, iii);
		}
		else
		{
		//	CONSOLE_DEBUG_W_NUM("iii\t=", iii);
			break;
		}
		iii++;
	}


	sprintf(textString, "Total Alpaca Devices found=%d", myDevCount);
	SetWidgetText(kAlpacaList_AlpacaDev_Total, textString);

	SetWidgetBGColor(kAlpacaList_ClmTitle1,	((cSortColumn == 0) ? CV_RGB(255,	255,	255) : CV_RGB(128,	128,	128)));
	SetWidgetBGColor(kAlpacaList_ClmTitle2,	((cSortColumn == 1) ? CV_RGB(255,	255,	255) : CV_RGB(128,	128,	128)));
	SetWidgetBGColor(kAlpacaList_ClmTitle3,	((cSortColumn == 2) ? CV_RGB(255,	255,	255) : CV_RGB(128,	128,	128)));
	SetWidgetBGColor(kAlpacaList_ClmTitle4,	((cSortColumn == 3) ? CV_RGB(255,	255,	255) : CV_RGB(128,	128,	128)));
	SetWidgetBGColor(kAlpacaList_ClmTitle5,	((cSortColumn == 4) ? CV_RGB(255,	255,	255) : CV_RGB(128,	128,	128)));

}

static int	gSortColumn;

//**************************************************************************************
static  int RemoteObjectQsortProc(const void *e1, const void *e2)
{
	TYPE_REMOTE_DEV	*obj1, *obj2;
	int				returnValue;
	uint32_t		ipAddr1;
	uint32_t		ipAddr2;


	obj1		=	(TYPE_REMOTE_DEV *)e1;
	obj2		=	(TYPE_REMOTE_DEV *)e2;

	returnValue	=	0;
	switch(gSortColumn)
	{
		case 0:
			returnValue	=	0;	//*	let the default code below take care of this
			break;

		case 1:
			returnValue	=	strcasecmp(obj1->hostName, obj2->hostName);
			break;

		case 2:
			returnValue	=	strcasecmp(obj1->deviceTypeStr, obj2->deviceTypeStr);
			if (returnValue == 0)
			{
				returnValue	=	strcasecmp(obj1->deviceNameStr, obj2->deviceNameStr);
			}
			break;

		case 3:
			returnValue	=	strcasecmp(obj1->deviceNameStr, obj2->deviceNameStr);
			break;

		case 4:
			returnValue	=	obj1->interfaceVersion -obj2->interfaceVersion;
			if (returnValue == 0)
			{
				returnValue	=	strcasecmp(obj1->deviceTypeStr, obj2->deviceTypeStr);
				if (returnValue == 0)
				{
					returnValue	=	strcasecmp(obj1->deviceNameStr, obj2->deviceNameStr);
				}
			}
			break;
	}
	//*	if they are the same, sort by address
	if (returnValue == 0)
	{
		ipAddr1	=	(obj1->deviceAddress.sin_addr.s_addr & 0x000000ff) << 24;
		ipAddr1	+=	(obj1->deviceAddress.sin_addr.s_addr & 0x0000ff00) << 8;
		ipAddr1	+=	(obj1->deviceAddress.sin_addr.s_addr & 0x00ff0000) >> 8;
		ipAddr1	+=	(obj1->deviceAddress.sin_addr.s_addr & 0xff000000) >> 24;

		ipAddr2	=	(obj2->deviceAddress.sin_addr.s_addr & 0x000000ff) << 24;
		ipAddr2	+=	(obj2->deviceAddress.sin_addr.s_addr & 0x0000ff00) << 8;
		ipAddr2	+=	(obj2->deviceAddress.sin_addr.s_addr & 0x00ff0000) >> 8;
		ipAddr2	+=	(obj2->deviceAddress.sin_addr.s_addr & 0xff000000) >> 24;

		if (ipAddr1 < ipAddr2)
		{
			returnValue	=	-1;
		}
		else if (ipAddr1 > ipAddr2)
		{
			returnValue	=	1;
		}
	}

//	CONSOLE_DEBUG_W_NUM("returnValue\t=", returnValue);

	return(returnValue);

}


//**************************************************************************************
void	WindowTabAlpacaList::UpdateSortOrder(void)
{
//	CONSOLE_DEBUG(__FUNCTION__);
//	CONSOLE_DEBUG_W_NUM("cAlpacaDevCnt\t=", cAlpacaDevCnt);

	if (cSortColumn >= 0)
	{
		gSortColumn	=	cSortColumn;
		qsort(cRemoteDeviceList, cAlpacaDevCnt, sizeof(TYPE_REMOTE_DEV), RemoteObjectQsortProc);

		UpdateOnScreenWidgetList();
	}

}

