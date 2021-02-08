//*****************************************************************************
//#include	"controller_telescope.h"


#ifndef	_ALPACA_DEFS_H_
	#include	"alpaca_defs.h"
#endif



#include	"controller.h"

#include	"windowtab_telescope.h"

#ifndef	_WINDOWTAB_ABOUT_H_
	#include	"windowtab_about.h"
#endif // _WINDOWTAB_ABOUT_H_



//*****************************************************************************
class ControllerTelescope: public Controller
{
	public:
		//
		// Construction
		//
				ControllerTelescope(const char			*argWindowName,
									struct sockaddr_in	*deviceAddress,
									const int			port,
									const int			deviceNum);


		virtual	~ControllerTelescope(void);


		virtual	void	SetupWindowControls(void);
//		virtual	void	ProcessButtonClick(const int buttonIdx);
		virtual	void	RunBackgroundTasks(void);

		virtual	bool	AlpacaGetStartupData(void);
		virtual	void	AlpacaProcessSupportedActions(	const char	*deviceType,
														const int	deviveNum,
														const char	*valueString);

		virtual	void	AlpacaProcessReadAll(			const char	*deviceType,
														const int	deviveNum,
														const char	*keywordString,
														const char	*valueString);
		virtual	void	AlpacaDisplayErrorMessage(const char *errorMsgString);

				void	AlpacaProcessSupportedActions_Telescope(	const int	deviveNum,
																	const char	*valueString);
				void	AlpacaProcessReadAll_Telescope(	const int	deviceNum,
														const char	*keywordString,
														const char *valueString);
				bool	AlpacaGetStatus(void);
				bool	AlpacaGetStatus_TelescopeOneAAT(void);	//*	One At A Time
				bool	AlpacaGetStartupData_TelescopeOneAAT(void);	//*	One At A Time
			//*	tab information
				WindowTabTelescope	*cTelescopeTabObjPtr;
				WindowTabAbout		*cAboutBoxTabObjPtr;


				//---------------------------------------------------
				void			Update_TelescopeRtAscension(void);
				void			Update_TelescopeDeclination(void);

				//---------------------------------------------------
				//*	telescope driver variables
				TYPE_TelescopeProperties	cTelescopeProp;


};


