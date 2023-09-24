//************************************************************************
//*	Edit History
//************************************************************************
//*	Dec 31,	2020	<MLS> Migrating NGCcatalog.c for using in AlpacaPi
//************************************************************************
//*	https://heasarc.gsfc.nasa.gov/W3Browse/all/ngc2000.html
//************************************************************************

#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<string.h>
#include	<stdlib.h>
#include	<stdio.h>

//*	MLS Libraries
#define _ENABLE_CONSOLE_DEBUG_
#include	"ConsoleDebug.h"


#include	"SkyStruc.h"
#include	"SkyTravelConstants.h"
#include	"helper_functions.h"
#include	"controller_startup.h"

#include	"StarCatalogHelper.h"
#include	"NGCcatalog.h"



//	 1492  Gx  3 58.1  -35 27 r  Eri              vF, vS, R
//	I1492  Gx 23 30.7  -03 02 m  Psc   0.8  14. p eF, S, R, sp of 2


//************************************************************************
//*	http://cdsarc.u-strasbg.fr/ftp/cats/VII/118/ReadMe
//************************************************************************
//*	Byte-by-byte Description of file: ngc2000.dat
//*	--------------------------------------------------------------------------------
//*	   Bytes Format  Units   Label    Explanations
//*	--------------------------------------------------------------------------------
//*	   1-  5  A5     ---     Name     NGC or IC designation (preceded by I)
//*	   7-  9  A3     ---     Type    *Object classification
//*	  11- 12  I2     h       RAh      Right Ascension 2000 (hours)
//*	  14- 17  F4.1   min     RAm      Right Ascension 2000 (minutes)
//*	      20  A1     ---     DE-      Declination 2000 (sign)
//*	  21- 22  I2     deg     DEd      Declination 2000 (degrees)
//*	  24- 25  I2     arcmin  DEm      Declination 2000 (minutes)
//*	      27  A1     ---     Source  *Source of entry
//*	  30- 32  A3     ---     Const    Constellation
//*	      33  A1     ---     l_size   [<] Limit on Size
//*	  34- 38  F5.1   arcmin  size     ? Largest dimension
//*	  41- 44  F4.1   mag     mag      ? Integrated magnitude, visual or photographic
//*	                                      (see n_mag)
//*	      45  A1     ---     n_mag    [p] 'p' if mag is photographic (blue)
//*	  47- 96  A50    ---     Desc    *Description of the object
//*	--------------------------------------------------------------------------------
//*	Note on Type: the field is coded as follows:
//*	     Gx    Galaxy
//*	     OC    Open star cluster
//*	     Gb    Globular star cluster, usually in the Milky Way Galaxy
//*	     Nb    Bright emission or reflection nebula
//*	     Pl    Planetary nebula
//*	     C+N   Cluster associated with nebulosity
//*	     Ast   Asterism or group of a few stars
//*	     Kt    Knot  or  nebulous  region  in  an  external galaxy
//*	     ***   Triple star
//*	     D*    Double star
//*	     *     Single star
//*	     ?     Uncertain type or may not exist
//*	     blank Unidentified at the place given, or type unknown
//*	     -     Object called nonexistent in the RNGC (Sulentic and Tifft 1973)
//*	     PD    Photographic plate defect
//*
//*
//*	sample
//*			  1	        2         3         4
//*	123456789 123456789 123456789 123456789 123456789 123456789 123456789
//*	I5370  Gx  0 00.1  +32 45 m  And   0.7  15. p pB, S, R, stell N
//*
//************************************************************************

static	int	gICcount	=	0;

//************************************************************************
static bool	ParseOneLineOfNGCStarCatalog(char *lineBuff, TYPE_CelestData *starRec)
{
bool		validObject;
long		raHour;
double		raMin;
double		raHours;
double		raDegrees;

long		deDeg;
long		deMin;
double		declDegrees;
double		raRadians;
double		declRadians;
short		deSign;
char		icSourceDesignator;
double		magnitude;

	memset(starRec, 0, sizeof(TYPE_CelestData));

	validObject					=	true;
	icSourceDesignator			=	lineBuff[0];
	starRec->id					=	ParseLongFromString(lineBuff,		 2-1, 4);
	starRec->NGCtype			=	ParseCharValueFromString(lineBuff,	 7-1, 3);
	raHour						=	ParseLongFromString(lineBuff,		11-1, 2);
	raMin						=	ParseFloatFromString(lineBuff,		14-1, 4);
	magnitude					=	ParseFloatFromString(lineBuff,		41-1, 4);

	deSign						=	lineBuff[20-1];
	deDeg						=	ParseLongFromString(lineBuff,		21-1, 2);
	deMin						=	ParseLongFromString(lineBuff,		24-1, 2);

//	starRec->maxSizeArcMinutes	=	ParseFloatFromString(lineBuff,		34-1, 5);
	if (isdigit(lineBuff[40]) || isdigit(lineBuff[41]))
	{
		starRec->realMagnitude	=	ParseFloatFromString(lineBuff,		41-1, 4);
	}
	else
	{
		starRec->realMagnitude	=	16.0;
	}
	raHours			=	raHour + (raMin / 60.0) ;
	raDegrees		=	raHours * 15;
	declDegrees		=	deDeg + (deMin / 60.0);

	raRadians		=	(raDegrees * PI) / 180.0;
	declRadians		=	(declDegrees * PI) / 180.0;

	if (deSign == '-')
	{
		declRadians	=	-declRadians;
	}

	if (magnitude == 0.0)
	{
		magnitude	=	17.77;
	}

	starRec->ra				=	raRadians;
	starRec->decl			=	declRadians;
	starRec->org_ra			=	raRadians;
	starRec->org_decl		=	declRadians;
	starRec->realMagnitude	=	magnitude;

//	CONSOLE_DEBUG_W_DBL("magnitude\t=", magnitude);

	if (icSourceDesignator == 'I')
	{
		gICcount++;
		starRec->dataSrc	=	kDataSrc_NGC2000IC;
		sprintf(starRec->longName, "IC%ld", starRec->id);
	}
	else
	{
		starRec->dataSrc	=	kDataSrc_NGC2000;
		sprintf(starRec->longName, "NGC%ld", starRec->id);
	}
//	CONSOLE_DEBUG_W_STR("longName\t=", starRec->longName);
	if (starRec->NGCtype == '  PD')
	{
		validObject	=	false;
	}
	return(validObject);
}


//************************************************************************
TYPE_CelestData	*ReadNGCStarCatalog(long *starCount)
{
FILE			*filePointer;
long			ngcLineCount;
long			recordCount;
TYPE_CelestData	*ngcStarData;
TYPE_CelestData	starObject;
bool			validObject;
char			lineBuff[512];
char			filePath[128];
//size_t			bufferSize;

//	CONSOLE_DEBUG(__FUNCTION__);

	ngcStarData	=	NULL;

	strcpy(filePath, kSkyTravelDataDirectory);
	strcat(filePath, "/ngc2000.dat");

	filePointer	=	fopen(filePath, "r");
	if (filePointer != NULL)
	{
		ngcLineCount	=	13300;

//		bufferSize	=	(ngcLineCount + 2) * sizeof(TYPE_CelestData);
		ngcStarData	=	(TYPE_CelestData *)calloc((ngcLineCount + 2), sizeof(TYPE_CelestData));
		if (ngcStarData != NULL)
		{

			recordCount	=	0;
			while (fgets(lineBuff, 500, filePointer) && (recordCount < ngcLineCount))
			{
				validObject	=	ParseOneLineOfNGCStarCatalog(lineBuff, &starObject);
				if (validObject)
				{
					ngcStarData[recordCount]	=	starObject;
					recordCount++;
				}
			}

			*starCount	=	recordCount;
		}
		fclose(filePointer);
		SetStartupText("NGC catalog:\tOK");
	}

	return(ngcStarData);
}



#pragma mark -

//************************************************************************
//*	Byte-by-byte Description of file: names.dat
//*	--------------------------------------------------------------------------------
//*	   Bytes Format Units   Label     Explanations
//*	--------------------------------------------------------------------------------
//*	   1- 35  A35   ---     Object    Common name (including Messier numbers)
//*	  37- 41  A5    ---     Name     *NGC or IC name, as in ngc2000.dat
//*	  43- 70  A28   ---     Comment   Text of comment, if any
//*	--------------------------------------------------------------------------------
//*	Note on Name: this field may be blank for Messier objects without
//*		NGC or IC counterparts.
//*		when one object corresponds to several entries in ngc2000,
//*		the Object is repeated (e.g. Copeland's Septet appears 7 times)
//************************************************************************


static	TYPE_ObjectInfo	*gNGCobjectInfo		=	NULL;
static	long			gNGCobjectInfoCount	=	0;


//************************************************************************
static bool	ParseOneLineOfNGCnameFile(char *lineBuff, TYPE_ObjectInfo *objectInfo)
{
char	icSourceDesignator;

//	CONSOLE_DEBUG(__FUNCTION__);

	strncpyZero(objectInfo->name, &lineBuff[0], 35);
	icSourceDesignator				=	lineBuff[37-1];
	objectInfo->id					=	ParseLongFromString(lineBuff,	38-1, 4);
	strncpyZero(objectInfo->miscInfo, &lineBuff[0], 28);
	if (icSourceDesignator == 'I')
	{
		objectInfo->dataSrc	=	kDataSrc_NGC2000IC;
	}
	else
	{
		objectInfo->dataSrc	=	kDataSrc_NGC2000;
	}

	//*	truncate any trailing spaces
	StripTrailingSpaces(objectInfo->name);
	StripTrailingSpaces(objectInfo->miscInfo);

	return(true);
}


//************************************************************************
static TYPE_ObjectInfo	*ReadNGCnameFile(long *objectCount)
{
FILE			*filePointer;
long			ngcLineCount;
long			recordCount;
TYPE_ObjectInfo	*ngcNameData;
TYPE_ObjectInfo	nameObject;
bool			validObject;
char			lineBuff[512];
char			filePath[128];
//size_t			bufferSize;

//	CONSOLE_DEBUG(__FUNCTION__);

	ngcNameData	=	NULL;

	strcpy(filePath, kSkyTravelDataDirectory);
	strcat(filePath, "/NGCnames.dat");

	filePointer	=	fopen(filePath, "r");
	if (filePointer != NULL)
	{
		ngcLineCount	=	230;

//		bufferSize	=	ngcLineCount * sizeof(TYPE_ObjectInfo);
		ngcNameData	=	(TYPE_ObjectInfo *)calloc((ngcLineCount + 2), sizeof(TYPE_ObjectInfo));
		if (ngcNameData != NULL)
		{
			recordCount	=	0;
			while (fgets(lineBuff, 500, filePointer) && (recordCount < ngcLineCount))
			{
				validObject	=	ParseOneLineOfNGCnameFile(lineBuff, &nameObject);
				if (validObject)
				{
					ngcNameData[recordCount]	=	nameObject;
					recordCount++;
				}

				*objectCount	=	recordCount;
			}
		}
	}
	else
	{
		CONSOLE_DEBUG_W_STR("Failed to read", filePath);
//		CONSOLE_ABORT(__FUNCTION__);
	}
	return(ngcNameData);
}

//************************************************************************
bool	GetObjectDescription(TYPE_CelestData *objectPtr, char *returnText, short maxTexLen)
{
long	ii;
bool	objectFound;

//	CONSOLE_DEBUG(__FUNCTION__);

	objectFound	=	false;
	if ((objectPtr != NULL) && (returnText != NULL))
	{
		returnText[0]	=	0;

		switch(objectPtr->dataSrc)
		{
			case kDataSrc_NGC2000:
			case kDataSrc_NGC2000IC:
			//	CONSOLE_DEBUG("kDataSrc_NGC2000");
				if (gNGCobjectInfo == NULL)
				{
					gNGCobjectInfo	=	ReadNGCnameFile(&gNGCobjectInfoCount);
				}
				if (gNGCobjectInfo != NULL)
				{
					for (ii=0; ii<gNGCobjectInfoCount; ii++)
					{
						if (objectPtr->id == gNGCobjectInfo[ii].id)
						{
							objectFound	=	true;

							strcat(returnText, gNGCobjectInfo[ii].name);
							if (strcmp(gNGCobjectInfo[ii].name, gNGCobjectInfo[ii].miscInfo) != 0)
							{
								strcat(returnText, " ");
								strcat(returnText, gNGCobjectInfo[ii].miscInfo);
							}
							strcat(returnText, " ");
						}
					//	CONSOLE_DEBUG(returnText);
						//*	go all the way thru, some items are be in the list twice
					}
				}
				else
				{
					CONSOLE_DEBUG("Object info not present");
					CONSOLE_ABORT(__FUNCTION__);
				}
				break;

			case kDataSrc_Orginal:
			case kDataSrc_YaleBrightStar:
				break;
		}
	}
	return(objectFound);
}
