//**************************************************************************
//*	Name:			domedriver_ror.h
//*
//*	Author:			Mark Sproul (C) 2020
//*
//*	Description:	C++ Driver for Alpaca protocol
//*
//*	Limitations:
//*
//*	Usage notes:
//*
//*	References:
//*		https://ascom-standards.org/api/#/Dome%20Specific%20Methods/get_dome__device_number__athome
//*		https://github.com/OpenPHDGuiding/phd2/tree/master/cameras/zwolibs
//*****************************************************************************
//*	Edit History
//*****************************************************************************
//*	<MLS>	=	Mark L Sproul
//*****************************************************************************
//*	Dec  2,	2020	<MLS> Created domedriver_ror_rpi.h
//*****************************************************************************
//#include	"domedriver_ror_rpi.h"

#ifndef _STDBOOL_H
	#include	<stdbool.h>
#endif

#ifndef _ALPACA_DRIVER_H_
	#include	"alpacadriver.h"
#endif

#ifndef _DOME_DRIVER_H_
	#include	"domedriver.h"
#endif

void	CreateDomeObjectsROR(void);


//**************************************************************************************
class DomeDriverROR: public DomeDriver
{
	public:

		//
		// Construction
		//
						DomeDriverROR(const int argDevNum);
		virtual			~DomeDriverROR(void);
		virtual	void	OutputHTML_Part2(TYPE_GetPutRequestData *reqData);
		virtual	void	Init_Hardware(void);
		virtual	int32_t	RunStateMachine_ROR(void);

//	protected:


//		virtual	bool	BumpDomeSpeed(const int howMuch);
//		virtual	void	CheckDomeButtons(void);
//		virtual	void	CheckSensors(void);
//		virtual	void 	ProcessButtonPressed(const int pressedButton);
//		virtual	void	StartDomeMoving(const int direction);
		virtual	void	StopDomeMoving(bool rightNow);

		virtual	TYPE_ASCOM_STATUS	SetPower(bool onOffFlag);
		virtual	TYPE_ASCOM_STATUS	GetPower(bool *onOffFlag);
		virtual	TYPE_ASCOM_STATUS	SetAuxiliary(bool onOffFlag);
		virtual	TYPE_ASCOM_STATUS	GetAuxiliary(bool *onOffFla);

		virtual	TYPE_ASCOM_STATUS 	OpenShutter(char *alpacaErrMsg);
		virtual	TYPE_ASCOM_STATUS 	CloseShutter(char *alpacaErrMsg);

		int			cRelayCount;
		int32_t		cTimeOfLastOpenClose;
		bool		cRORisOpening;
		bool		cRORisClosing;

		//-----------------------------------------------------------------
		//*	background thread operations
		bool						cCmdRcvd_OpenRoof;
		bool						cCmdRcvd_CloseRoof;

	protected:
		virtual	void				RunThread_Startup(void);
		virtual	void				RunThread_Loop(void);

};



