//*****************************************************************************
//#include	"GaiaSQL.h"

#ifndef _SKY_STRUCTS_H_
	#include	"SkyStruc.h"
#endif

#define	kPolarDeclinationLimit	(84.0)

#define	kSQLserverConfigFile	"sqlserver.txt"

#ifdef __cplusplus
	extern "C" {
#endif

bool	GaiaSQLinit(void);		//*	returns true if valid config file
int		StartGaiaSQLthread(void);

//*	returns 1 if new request was started, 0 if not
int		UpdateSkyTravelView(double ra_Degrees, double dec_Degrees, double viewAngle_Degrees);
void	ClearAllSQLdata(void);
double	CalcRA_DEC_Distance_Deg(const double	ra1_Deg,
								const double	dec1_Deg,
								const double	ra2_Deg,
								const double	dec2Deg);
bool	GetSQLdataFromIDnumber(const char *gaiaIDnumberStr, TYPE_CelestData *gaiaData, char *rtnErrorMessage);

int		UpdateDataBaseListFromServer(char *errorString);

//*****************************************************************************
typedef struct
{
	bool			validData;
	int				serverReqCount;		//*	the number of times we have requested this block
	double			centerRA_deg;
	double			centerDEC_deg;
	int				block_RA_deg;		//*	the degree value of the 1x1 degree block
	int				block_DEC_deg;
	TYPE_CelestData	*gaiaData;
	int				gaiaDataCnt;
	unsigned int	elapsedMilliSecs;
	int				sequenceNum;
	struct timeval	timeStamp;
	double			distanceCtrScrn;	//*	the distance to the center of the screen
} TYPE_GAIA_REMOTE_DATA;


#define	kMaxGaiaDataSets	30
#define	JD2016				(2457388.5)

extern TYPE_GAIA_REMOTE_DATA	gGaiaDataList[];
extern bool						gEnableSQLlogging;


//*****************************************************************************
typedef struct
{
	char			Name[32];
} TYPE_DATABASE_NAME;

#define	kMaxDataBaseNames	10

extern TYPE_DATABASE_NAME	gDataBaseNames[];
extern int					gDataBaseNameCnt;

#ifdef __cplusplus
}
#endif

