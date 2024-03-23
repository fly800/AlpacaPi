//*****************************************************************************
//*		controller_image.cpp		(c) 2020 by Mark Sproul
//*
//*
//*
//*****************************************************************************
//*	Edit History
//*****************************************************************************
//*	Dec 27,	2020	<MLS> Created controller_image.cpp
//*	Apr  2,	2021	<MLS> Added SetLiveWindowImage()
//*	Apr  2,	2021	<MLS> Added UpdateLiveWindowImage()
//*	Apr  3,	2021	<MLS> Added UpdateLiveWindowInfo()
//*	Apr  8,	2021	<MLS> Added CopyImageToLiveImage()
//*	Feb 25,	2022	<MLS> SetLiveWindowImage() working under C++ opencv
//*	Mar 12,	2022	<MLS> Added SetImageWindowInfo()
//*	Mar 12,	2022	<MLS> Added TYPE_BinaryImageHdr struct to Image controller
//*	Nov 15,	2022	<MLS> Added ControllerImage(filePath);
//*	Nov 15,	2022	<MLS> Added InitClassVariables()
//*	Feb 25,	2023	<MLS> Added SetDownloadInfo()
//*	Feb 26,	2023	<MLS> Added histogram to image window
//*	Feb 26,	2023	<MLS> Added CalculateHistogramArray()
//*	Feb 28,	2023	<MLS> Added SaveHistogram()
//*	Mar 15,	2024	<MLS> Changed handling of long file paths for name display
//*	Mar 16,	2024	<MLS> Added ProcessFitsHeader()
//*	Mar 17,	2024	<MLS> Added Title Block Info
//*	Mar 18,	2024	<MLS> Added LoadImage()
//*	Mar 18,	2024	<MLS> Added DrawTitleBlock()
//*	Mar 19,	2024	<MLS> Added DrawTextString2()
//*	Mar 19,	2024	<MLS> Added SaveImage()
//*	Mar 21,	2024	<MLS> Added fits header window tab
//*	Mar 23,	2024	<MLS> Added SetImageWindowTitles()
//*	Mar 23,	2024	<MLS> Working on reading NASA PDS images (Planetary Data Systems)
//*	Mar 23,	2024	<MLS> PDS Header data displaying properly
//*	Mar 23,	2024	<MLS> Added IsFilePDS()
//*****************************************************************************

#ifdef _ENABLE_CTRL_IMAGE_

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>

#include	<fitsio.h>

#include	"discovery_lib.h"
#include	"sendrequest_lib.h"
#include	"helper_functions.h"

#include	"opencv_utils.h"
#include	"file_utils.h"
#include	"file_utils.c"
#include	"fits_opencv.h"

#define _ENABLE_CONSOLE_DEBUG_
//#define	_DEBUG_TIMING_
#include	"ConsoleDebug.h"


#define	kWindowWidth	1100
#define	kWindowHeight	800

#include	"alpaca_defs.h"
#include	"windowtab_image.h"
#include	"windowtab_about.h"

#include	"fits_opencv.h"
#include	"fits_helper.h"
#include	"fits_helper.cpp"


#include	"controller.h"
#include	"controller_image.h"
#include	"windowtab_imageinfo.h"

#ifdef _ENABLE_NASA_PDS_
	#include	"PDS_ReadNASAfiles.h"
#endif // _ENABLE_NASA_PDS_

//**************************************************************************************
enum
{
	kTab_Image	=	1,
	kTab_ImageInfo,
	kTab_FitsHeader,
	kTab_About,

	kTab_Count

};


#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
//**************************************************************************************
ControllerImage::ControllerImage(	const char			*argWindowName,
									cv::Mat				*downloadedImage,
									TYPE_BinaryImageHdr	*binaryImageHdr)
			:Controller(	argWindowName,
							kWindowWidth,
							kWindowHeight)
#else
//**************************************************************************************
ControllerImage::ControllerImage(	const char			*argWindowName,
									IplImage			*downloadedImage,
									TYPE_BinaryImageHdr	*binaryImageHdr)
			:Controller(	argWindowName,
							kWindowWidth,
							kWindowHeight)
#endif // _USE_OPENCV_CPP_
{

	CONSOLE_DEBUG(__FUNCTION__);

	InitClassVariables();

	//*	deal with the binary image header if it was supplied
	if (binaryImageHdr != NULL)
	{
	char	elementTypeStr[32];
	char	transmitTypeStr[32];

		CONSOLE_DEBUG("Setting binary image header data");
		cBinaryImageHdr	=	*binaryImageHdr;

		GetBinaryElementTypeString(cBinaryImageHdr.ImageElementType,		elementTypeStr);
		GetBinaryElementTypeString(cBinaryImageHdr.TransmissionElementType,	transmitTypeStr);

//		CONSOLE_DEBUG_W_NUM("MetadataVersion        \t=",	cBinaryImageHdr.MetadataVersion);
//		CONSOLE_DEBUG_W_NUM("ErrorNumber            \t=",	cBinaryImageHdr.ErrorNumber);
//		CONSOLE_DEBUG_W_NUM("ClientTransactionID    \t=",	cBinaryImageHdr.ClientTransactionID);
//		CONSOLE_DEBUG_W_NUM("ServerTransactionID    \t=",	cBinaryImageHdr.ServerTransactionID);
//		CONSOLE_DEBUG_W_NUM("DataStart              \t=",	cBinaryImageHdr.DataStart);
//		CONSOLE_DEBUG_W_STR("ImageElementType       \t=",	elementTypeStr);
//		CONSOLE_DEBUG_W_STR("TransmissionElementType\t=",	transmitTypeStr);
//		CONSOLE_DEBUG_W_NUM("Rank                   \t=",	cBinaryImageHdr.Rank);
//		CONSOLE_DEBUG_W_NUM("Dimension1             \t=",	cBinaryImageHdr.Dimension1);
//		CONSOLE_DEBUG_W_NUM("Dimension2             \t=",	cBinaryImageHdr.Dimension2);
//		CONSOLE_DEBUG_W_NUM("Dimension3             \t=",	cBinaryImageHdr.Dimension3);

		SetWidgetNumber(kTab_ImageInfo, kImageInfo_MetadataVersionVal,			cBinaryImageHdr.MetadataVersion);
		SetWidgetNumber(kTab_ImageInfo, kImageInfo_ErrorNumberVal,				cBinaryImageHdr.ErrorNumber);
		SetWidgetNumber(kTab_ImageInfo, kImageInfo_ClientTransactionIDVal,		(int32_t)cBinaryImageHdr.ClientTransactionID);
		SetWidgetNumber(kTab_ImageInfo, kImageInfo_ServerTransactionIDVal,		(int32_t)cBinaryImageHdr.ServerTransactionID);
		SetWidgetNumber(kTab_ImageInfo, kImageInfo_DataStartVal,				cBinaryImageHdr.DataStart);
		SetWidgetText(	kTab_ImageInfo, kImageInfo_ImageElementTypeVal,			elementTypeStr);
		SetWidgetText(	kTab_ImageInfo, kImageInfo_TransmissionElementTypeVal,	transmitTypeStr);
		SetWidgetNumber(kTab_ImageInfo, kImageInfo_RankVal,						cBinaryImageHdr.Rank);
		SetWidgetNumber(kTab_ImageInfo, kImageInfo_Dimension1Val,				cBinaryImageHdr.Dimension1);
		SetWidgetNumber(kTab_ImageInfo, kImageInfo_Dimension2Val,				cBinaryImageHdr.Dimension2);
		SetWidgetNumber(kTab_ImageInfo, kImageInfo_Dimension3Val,				cBinaryImageHdr.Dimension3);


		SetWidgetText(		kTab_Image, kImageDisplay_LiveOrDownLoad,	"Downloaded image");
		SetWidgetBGColor(	kTab_Image, kImageDisplay_LiveOrDownLoad,	CV_RGB(0,	200, 0));
	}
	else
	{
		CONSOLE_DEBUG("Setting Live image");
		SetWidgetText(		kTab_Image, kImageDisplay_LiveOrDownLoad,	"LIVE image!!!!!!!");
		SetWidgetBGColor(	kTab_Image, kImageDisplay_LiveOrDownLoad,	CV_RGB(255,	255, 77));
	}


	//*	the downloaded image needs to be copied and/or resized to the displayed image
	if (downloadedImage != NULL)
	{
		SetLiveWindowImage(downloadedImage);
	}
	else
	{
		CONSOLE_DEBUG("No image was specfified");
	}
}


//**************************************************************************************
//*	this version reads a file from disk into the display window
//**************************************************************************************
ControllerImage::ControllerImage(	const char	*argWindowName,
									const char *imageFilePath)
			:Controller(	argWindowName,
							kWindowWidth,
							kWindowHeight)
{
//int		imagePathLen;
//#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
//	cv::Mat		*imageFromDisk;
//#else
//	IplImage	*imageFromDisk;
//#endif // _USE_OPENCV_CPP_

//	CONSOLE_DEBUG(__FUNCTION__);
//	CONSOLE_DEBUG_W_STR("Setting image from disk", imageFilePath);
	InitClassVariables();

	LoadImage(imageFilePath);
//	CONSOLE_DEBUG_W_STR(__FUNCTION__, "EXIT");
}


//**************************************************************************************
// Destructor
//**************************************************************************************
ControllerImage::~ControllerImage(void)
{
	CONSOLE_DEBUG_W_STR(__FUNCTION__, cWindowName);
	SetWidgetImage(kTab_Image, kImageDisplay_ImageDisplay, NULL);
#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
//++	#warning "OpenCV++ not tested"
	//*	free up the image memory
	if (cDownLoadedImage != NULL)
	{
		CONSOLE_DEBUG("Deleting cDownLoadedImage!!!!!!!!!!!!!!!!!!!!!!!!");
		delete cDownLoadedImage;
		cDownLoadedImage	=	NULL;
	}
	if (cDisplayedImage != NULL)
	{
		CONSOLE_DEBUG("Deleting cDisplayedImage!!!!!!!!!!!!!!!!!!!!!!!!");
		delete cDisplayedImage;
		cDisplayedImage	=	NULL;
	}
#else
	//--------------------------------------------
	//*	free up the image memory
	if (cDownLoadedImage != NULL)
	{
//		CONSOLE_DEBUG_W_LHEX("Release cDownLoadedImage", (unsigned long)cDownLoadedImage);
		cvReleaseImage(&cDownLoadedImage);
		cDownLoadedImage	=	NULL;
	}
	if (cDisplayedImage != NULL)
	{
//		CONSOLE_DEBUG_W_LHEX("Release cDisplayedImage", (unsigned long)cDisplayedImage);
		cvReleaseImage(&cDisplayedImage);
		cDisplayedImage	=	NULL;
	}
#endif // _USE_OPENCV_CPP_

	//--------------------------------------------
	//*	delete the window tab objects
	DELETE_OBJ_IF_VALID(cImageTabObjPtr);
	DELETE_OBJ_IF_VALID(cImageInfoTabObjcPtr);
	DELETE_OBJ_IF_VALID(cFitsHeaderTabObjPtr);
	DELETE_OBJ_IF_VALID(cAboutBoxTabObjPtr);
}

//**************************************************************************************
void	ControllerImage::InitClassVariables(void)
{
//	CONSOLE_DEBUG(__FUNCTION__);
	memset((void *)&cBinaryImageHdr, 0, sizeof(TYPE_BinaryImageHdr));
	memset((void *)&cFitsHeaderData, 0, sizeof(TYPE_FitsHeaderData));
	strcpy(cFitsHeaderData.Location, "Shohola, PA");

	memset((void *)&cFitsHeaderText, 0, (sizeof(TYPE_FitsHdrLine) * kMaxFitsHdrLines));
	cFitsHdrCnt				=	0;


	cDownLoadedImage		=	NULL;
	cDisplayedImage			=	NULL;
	cColorImage				=	NULL;
	cImageFromDisk			=	false;
	cImageIsFITS			=	false;
	cImageIsPDS				=	false;

	cImageTabObjPtr			=	NULL;
	cImageInfoTabObjcPtr	=	NULL;
	cFitsHeaderTabObjPtr	=	NULL;
	cAboutBoxTabObjPtr		=	NULL;

	strcpy(cImageFileName,		"unkownimage");
	strcpy(cImageFileNameRoot,	"unkownimage");


//	CONSOLE_DEBUG_W_SIZE("sizeof(TYPE_BinaryImageHdr)", sizeof(TYPE_BinaryImageHdr));

	SetupWindowControls();
//	CONSOLE_DEBUG_W_STR(__FUNCTION__, "EXIT");
}

//**************************************************************************************
void	ControllerImage::SetupWindowControls(void)
{

//	CONSOLE_DEBUG(__FUNCTION__);
//	CONSOLE_DEBUG_W_NUM("cWidth        \t=",	cWidth);

	SetTabCount(kTab_Count);

	//---------------------------------------------------------------------
	SetTabText(kTab_Image,		"Image");
	cImageTabObjPtr		=	new WindowTabImage(	cWidth, cHeight, cBackGrndColor, cWindowName);
	if (cImageTabObjPtr != NULL)
	{
		SetTabWindow(kTab_Image,	cImageTabObjPtr);
		cImageTabObjPtr->SetParentObjectPtr(this);
	}

	//---------------------------------------------------------------------
	SetTabText(kTab_ImageInfo,		"Image Info");
	cImageInfoTabObjcPtr		=	new WindowTabImageInfo(	cWidth, cHeight, cBackGrndColor, cWindowName, &cBinaryImageHdr);
	if (cImageInfoTabObjcPtr != NULL)
	{
		SetTabWindow(kTab_ImageInfo,	cImageInfoTabObjcPtr);
		cImageInfoTabObjcPtr->SetParentObjectPtr(this);
	}
	//---------------------------------------------------------------------
	SetTabText(kTab_FitsHeader,		"Fits/PDS Header");
	cFitsHeaderTabObjPtr		=	new WindowTabFITSheader(	cWidth, cHeight, cBackGrndColor, cWindowName);
	if (cFitsHeaderTabObjPtr != NULL)
	{
		SetTabWindow(kTab_FitsHeader,	cFitsHeaderTabObjPtr);
		cFitsHeaderTabObjPtr->SetParentObjectPtr(this);
	}

	//---------------------------------------------------------------------
	SetTabText(kTab_About,		"About");
	cAboutBoxTabObjPtr		=	new WindowTabAbout(	cWidth, cHeight, cBackGrndColor, cWindowName);
	if (cAboutBoxTabObjPtr != NULL)
	{
		SetTabWindow(kTab_About,	cAboutBoxTabObjPtr);
		cAboutBoxTabObjPtr->SetParentObjectPtr(this);
	}
//	CONSOLE_DEBUG_W_STR(__FUNCTION__, "EXIT");
}

//**************************************************************************************
static bool	IsFilePDS(const char *extension)
{
bool	isPDS	=	false;

	if ((strcasecmp(extension, ".img") == 0) ||
		(strcasecmp(extension, ".imq") == 0) ||
		(strcasecmp(extension, ".lbl") == 0) ||
		(strcasecmp(extension, ".red") == 0) ||
		(strcasecmp(extension, ".grn") == 0) ||
		(strcasecmp(extension, ".sgr") == 0) ||
		(strcasecmp(extension, ".vio") == 0))
	{
		isPDS	=	true;
	}
	return(isPDS);
}

//**************************************************************************************
void	ControllerImage::SetImageWindowTitles(const char *imageFilePath)
{
int		imagePathLen;
char	newFileName[1024];
char	*slashPtr;
int		sLen;
int		ccc;

	CONSOLE_DEBUG_W_STR("imageFilePath    \t=",	imageFilePath);
	//*	isolate the file name from the path
	//*	find the last "/" char and use that
	strcpy(newFileName, "");
	slashPtr	=	strchr((char *)imageFilePath, '/');
	while (slashPtr != NULL)
	{
		slashPtr++;
//			CONSOLE_DEBUG(slashPtr);
		strcpy(newFileName, slashPtr);
		slashPtr	=	strchr(slashPtr, '/');
	}
	strcpy(cImageFileName,		newFileName);
	strcpy(cImageFileNameRoot,	newFileName);
	//*	now remove the extension
	sLen	=	strlen(cImageFileNameRoot);
	ccc		=	sLen -1;
	while ((cImageFileNameRoot[ccc] != '.') && (ccc > 0))
	{
		ccc--;
	}
	cImageFileNameRoot[ccc]	=	0;
//	CONSOLE_DEBUG_W_STR("cImageFileName    \t=",	cImageFileName);
//	CONSOLE_DEBUG_W_STR("cImageFileNameRoot\t=",	cImageFileNameRoot);

	cImageTabObjPtr->SetImageFilePath(imageFilePath);

	cImageFromDisk			=	true;
	ExtractFileExtension(imageFilePath, cFileExtension);
	//*	check for fits file
	if ((strcasecmp(cFileExtension, ".fits") == 0) || (strcasecmp(cFileExtension, ".fit") == 0))
	{
		cImageIsFITS	=	true;
	}
	else
	{
		cImageIsPDS	=	IsFilePDS(cFileExtension);
	}

	CONSOLE_DEBUG_W_STR("cFileExtension\t=",	cFileExtension);
	CONSOLE_DEBUG_W_BOOL("cImageIsFITS \t=",	cImageIsFITS);
	CONSOLE_DEBUG_W_BOOL("cImageIsPDS  \t=",	cImageIsPDS);

	imagePathLen	=	strlen(imageFilePath);
	if (imagePathLen < 110)
	{
		SetWidgetText(	kTab_Image,			kImageDisplay_LiveOrDownLoad,	imageFilePath);
		SetWidgetText(	kTab_FitsHeader,	kFitsHeader_FileName,			imageFilePath);
	}
	else
	{
	char	newFileName[1024];
	char	*slashPtr;
		//*	find the last "/" char and use that
		slashPtr	=	strchr((char *)imageFilePath, '/');
		while (slashPtr != NULL)
		{
			strcpy(newFileName, "...");
			strcat(newFileName, slashPtr);
			slashPtr++;
			slashPtr	=	strchr(slashPtr, '/');
		}
		SetWidgetText(	kTab_Image,			kImageDisplay_LiveOrDownLoad,	newFileName);
		SetWidgetText(	kTab_FitsHeader,	kFitsHeader_FileName,			newFileName);
	}
}

//**************************************************************************************
void	ControllerImage::LoadImage(const char *imageFilePath)
{
int		iii;

#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
	cv::Mat		*imageFromDisk;
#else
	IplImage	*imageFromDisk;
#endif // _USE_OPENCV_CPP_

#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
	if (imageFilePath != NULL)
	{
		if (strlen(imageFilePath) > 5)
		{
			//*	free up the image memory
			if (cDownLoadedImage != NULL)
			{
				CONSOLE_DEBUG("Deleting cDownLoadedImage!!!!!!!!!!!!!!!!!!!!!!!!");
				delete cDownLoadedImage;
				cDownLoadedImage	=	NULL;
			}
			if (cDisplayedImage != NULL)
			{
				CONSOLE_DEBUG("Deleting cDisplayedImage!!!!!!!!!!!!!!!!!!!!!!!!");
				delete cDisplayedImage;
				cDisplayedImage	=	NULL;
			}
			if (cColorImage != NULL)
			{
				CONSOLE_DEBUG("Deleting cColorImage!!!!!!!!!!!!!!!!!!!!!!!!");
				delete cColorImage;
				cColorImage	=	NULL;
			}

		#else
			#error "No longer supported"
		#endif	//
			cImageFromDisk		=	false;
			cImageIsFITS		=	false;
			cImageIsPDS			=	false;

			//--------------------------------------------------------
			SetImageWindowTitles(imageFilePath);

			if (cImageIsFITS)
			{
				imageFromDisk	=	ReadImageIntoOpenCVimage(imageFilePath);
				if (imageFromDisk != NULL)
				{
					SetLiveWindowImage(imageFromDisk);

					SetWidgetBGColor(	kTab_Image, kImageDisplay_LiveOrDownLoad,	CV_RGB(0,	200, 0));
				}
				else
				{
					CONSOLE_DEBUG_W_STR("Failed to read image from disk:", imageFilePath);
				}
				ProcessFitsHeader(imageFilePath);
			}
		#ifdef _ENABLE_NASA_PDS_
			else if (cImageIsPDS)
			{
			bool			readOK;
			PDS_header_data	pdsHeader;

				imageFromDisk	=	ReadPDSimageIntoOpenCVimage(imageFilePath);
				if (imageFromDisk != NULL)
				{
					SetLiveWindowImage(imageFromDisk);

					SetWidgetBGColor(	kTab_Image, kImageDisplay_LiveOrDownLoad,	CV_RGB(0,	200, 0));
				}
				else
				{
					CONSOLE_DEBUG_W_STR("Failed to read image from disk:", imageFilePath);
				}

				readOK	=	PDS_ReadImage(imageFilePath, &pdsHeader);
				for (iii=0; iii<pdsHeader.HeaderLineCnt; iii++)
				{
					CONSOLE_DEBUG(pdsHeader.HeaderData[iii].headerLine);
					strcpy(cFitsHeaderText[iii].fitsLine, pdsHeader.HeaderData[iii].headerLine);
				}
				cFitsHdrCnt		=	pdsHeader.HeaderLineCnt;
				cFitsHeaderTabObjPtr->SetFitsHeaderData(cFitsHeaderText, cFitsHdrCnt);
				CONSOLE_DEBUG_W_NUM("cFitsHdrCnt  \t=",	cFitsHdrCnt);
//				CONSOLE_ABORT(__FUNCTION__);
			}
		#endif // _ENABLE_NASA_PDS_
		}
		else
		{
			CONSOLE_DEBUG("Invalid file path");
		}
	}
	else
	{
		CONSOLE_DEBUG("Setting Live image");
		SetWidgetText(		kTab_Image, kImageDisplay_LiveOrDownLoad,	"LIVE image!!!!!!!");
		SetWidgetBGColor(	kTab_Image, kImageDisplay_LiveOrDownLoad,	CV_RGB(255,	255, 77));
	}
}

//**************************************************************************************
void	ControllerImage::RunBackgroundTasks(const char *callingFunction, bool enableDebug)
{
uint32_t	currentMillis;
uint32_t	deltaSeconds;
bool		needToUpdate;

//	CONSOLE_DEBUG_W_STR(__FUNCTION__, cWindowName);
//	CONSOLE_DEBUG_W_STR("Called from:", callingFunction);
	if (cReadStartup)
	{
//		AlpacaGetStartupData();
		cReadStartup	=	false;
	}

	needToUpdate	=	false;
	currentMillis	=	millis();
	deltaSeconds	=	(currentMillis - cLastUpdate_milliSecs) / 1000;


	if (cForceAlpacaUpdate)
	{
		needToUpdate		=	true;
		cForceAlpacaUpdate	=	false;
	}
	else if (deltaSeconds >= 1)
	{
		needToUpdate	=	true;
	}

	if (needToUpdate)
	{
		cLastUpdate_milliSecs	=	millis();
	}

	if (cImageTabObjPtr != NULL)
	{
		cImageTabObjPtr->RunWindowBackgroundTasks();
	}
}

//**************************************************************************************
void	ControllerImage::RefreshWindow(void)
{
	HandleWindowUpdate();
	cv::waitKey(100);
}

//**************************************************************************************
void	ControllerImage::DrawWidgetImage(TYPE_WIDGET *theWidget)
{
//	CONSOLE_DEBUG_W_STR(__FUNCTION__, cWindowName);
//	CONSOLE_DEBUG_W_NUM("theWidget->left\t=",	theWidget->left);
//	CONSOLE_DEBUG_W_NUM("theWidget->top\t=",	theWidget->top);
//
//	//*	this is hard coded to debug an image display problem
//	if (theWidget->left == 272)
//	{
//		DumpWidget(theWidget);
//	}
//
//	CONSOLE_DEBUG_W_NUM("theWidget->width\t=",	theWidget->width);
//	CONSOLE_DEBUG_W_NUM("theWidget->height\t=",	theWidget->height);
	if (cImageTabObjPtr != NULL)
	{
		if (cImageTabObjPtr->cImageZoomState)
		{
//			CONSOLE_DEBUG(__FUNCTION__);
//			CONSOLE_DEBUG("Zoomed");
			cImageTabObjPtr->DrawFullScaleIamge();
			Controller::DrawWidgetImage(theWidget, cImageTabObjPtr->cOpenCVdisplayedImage);
		}
		else
		{
//			CONSOLE_DEBUG("Normal");
			Controller::DrawWidgetImage(theWidget);
		}
	}
	else
	{
		CONSOLE_DEBUG("cImageTabObjPtr is NULL");
	}
}

#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
//**************************************************************************************
void	ControllerImage::SetLiveWindowImage(cv::Mat *newOpenCVImage)
{
int		smallDispalyWidth;
int		smallDisplayHeight;
int		reduceFactor;
int		newImgWidth;
int		newImgHeight;
int		newImgBytesPerPixel;
//int		openCVerr;
bool	validImg;

//	CONSOLE_DEBUG(__FUNCTION__);

	if (cDownLoadedImage != NULL)
	{
		CONSOLE_DEBUG("Deleting cDownLoadedImage!!!!!!!!!!!!!!!!!!!!!!!!");
	//	cvReleaseImage(&cDownLoadedImage);
		delete cDownLoadedImage;
		cDownLoadedImage	=	NULL;
	}
	if (cDisplayedImage != NULL)
	{
		CONSOLE_DEBUG("Deleting cDisplayedImage!!!!!!!!!!!!!!!!!!!!!!!!");
	//	cvReleaseImage(&cDisplayedImage);
		delete cDisplayedImage;
		cDisplayedImage	=	NULL;
	}

	if (newOpenCVImage != NULL)
	{
		//*	ok, now its time to CREATE our own image, we are going to make it the same as the
		//*	supplied image
		//	https://docs.opencv.org/3.4/d3/d63/classcv_1_1Mat.html
		newImgWidth			=	newOpenCVImage->cols;
		newImgHeight		=	newOpenCVImage->rows;
		newImgBytesPerPixel	=	newOpenCVImage->step[1];
//		CONSOLE_DEBUG_W_NUM("newImgBytesPerPixel\t=", newImgBytesPerPixel);
		validImg			=	true;
		if ((newImgWidth < 100) || (newImgWidth > 10000))
		{
			validImg		=	false;
		}
		if ((newImgHeight < 100) || (newImgHeight > 10000))
		{
			validImg		=	false;
		}
//		if ((newImgBytesPerPixel != 1) && (newImgBytesPerPixel != 3))
//		{
//			validImg		=	false;
//		}

		//--------------------------------------------------------------
		if (validImg)
		{
//			CONSOLE_DEBUG_W_NUM("newImgBytesPerPixel\t=", newImgBytesPerPixel);
			cDownLoadedImage	=	ConvertImageToRGB(newOpenCVImage);

			//*	the downloaded image needs to be copied and/or resized to the displayed image
			if (cDownLoadedImage != NULL)
			{
//				CONSOLE_DEBUG_W_LONG("cDownLoadedImage->step[0]\t=",	cDownLoadedImage->step[0]);
//				CONSOLE_DEBUG_W_LONG("cDownLoadedImage->step[1]\t=",	cDownLoadedImage->step[1]);

//				CONSOLE_DEBUG("Creating small image");
				reduceFactor		=	1;
				smallDispalyWidth	=	cDownLoadedImage->cols;
				smallDisplayHeight	=	cDownLoadedImage->rows;

//				CONSOLE_DEBUG_W_NUM("cDownLoadedImage->cols\t=",	cDownLoadedImage->cols);
//				CONSOLE_DEBUG_W_NUM("cDownLoadedImage->rows\t=",	cDownLoadedImage->rows);


				int		maxWindowWidth	=	800;
				int		maxWindowHeight	=	700;

//				CONSOLE_DEBUG_W_NUM("smallDisplayHeight\t=", smallDisplayHeight);
				while ((smallDispalyWidth > maxWindowWidth) || (smallDisplayHeight > (maxWindowHeight - 50)))
				{
					reduceFactor++;
					smallDispalyWidth	=	cDownLoadedImage->cols / reduceFactor;
					smallDisplayHeight	=	cDownLoadedImage->rows / reduceFactor;
//					CONSOLE_DEBUG_W_NUM("smallDisplayHeight\t=", smallDisplayHeight);
				}
//				CONSOLE_DEBUG_W_NUM("reduceFactor\t=", reduceFactor);
//				CONSOLE_DEBUG_W_NUM("smallDispalyWidth \t=",	smallDispalyWidth);
//				CONSOLE_DEBUG_W_NUM("smallDisplayHeight\t=",	smallDisplayHeight);

				cDisplayedImage	=	new cv::Mat(cv::Size(	smallDispalyWidth,
															smallDisplayHeight),
															CV_8UC3);

				if (cDisplayedImage != NULL)
				{
//					CONSOLE_DEBUG_W_LONG("cDisplayedImage->step[0]\t=",	cDisplayedImage->step[0]);
//					CONSOLE_DEBUG_W_LONG("cDisplayedImage->step[1]\t=",	cDisplayedImage->step[1]);
//					CONSOLE_DEBUG("Resizing image");

//					CONSOLE_DEBUG("Original is 8 bit color (3 channels)");
					cv::resize(	*cDownLoadedImage,
								*cDisplayedImage,
								cDisplayedImage->size(),
								0,
								0,
								cv::INTER_LINEAR);

//					openCVerr	=	cv::imwrite("displayed-resized.png", *cDisplayedImage);
//					if (openCVerr != 0)
//					{
//						CONSOLE_DEBUG_W_NUM("openCVerr               \t=",	openCVerr);
//						CONSOLE_DEBUG_W_NUM("cDisplayedImage->cols   \t=",	cDisplayedImage->cols);
//						CONSOLE_DEBUG_W_NUM("cDisplayedImage->rows   \t=",	cDisplayedImage->rows);
//						CONSOLE_DEBUG_W_LONG("cDisplayedImage->step[0]\t=",	cDisplayedImage->step[0]);
//						CONSOLE_DEBUG_W_LONG("cDisplayedImage->step[1]\t=",	cDisplayedImage->step[1]);
//					}

					SetWidgetImage(kTab_Image, kImageDisplay_ImageDisplay, cDisplayedImage);
				}
				else
				{
					CONSOLE_DEBUG("Failed to create new image");
				}

				if (cImageTabObjPtr != NULL)
				{
					cImageTabObjPtr->SetImagePtrs(cDownLoadedImage,	cDisplayedImage);
				}
			}
			else
			{
				CONSOLE_DEBUG("Error creating image (new cv::Mat)");
			}
			//*	Update the image size on the screen
			SetImageWindowInfo();
		}
		else
		{
			CONSOLE_DEBUG("Invalid image (size to big or to small)");
		}
	}
	else
	{
		CONSOLE_DEBUG("Image parameters invalid !!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
//		CONSOLE_ABORT(__FUNCTION__);
	}
	CalculateHistogramArray();
}

#else
//**************************************************************************************
void	ControllerImage::SetLiveWindowImage(IplImage *newOpenCVImage)
{
int		smallDispalyWidth;
int		smallDisplayHeight;
int		reduceFactor;
int		newImgWidth;
int		newImgHeight;
int		newImgChannels;
bool	validImg;
size_t	byteCount;

	CONSOLE_DEBUG(__FUNCTION__);
	if (cDownLoadedImage != NULL)
	{
		cvReleaseImage(&cDownLoadedImage);
		cDownLoadedImage	=	NULL;
	}
	if (cDisplayedImage != NULL)
	{
		cvReleaseImage(&cDisplayedImage);
		cDisplayedImage	=	NULL;
	}

	if (newOpenCVImage != NULL)
	{
		//*	ok, now its time to CREATE our own image, we are going to make it the same as the
		//*	supplied image
		newImgWidth			=	newOpenCVImage->width;
		newImgHeight		=	newOpenCVImage->height;
		newImgChannels		=	newOpenCVImage->nChannels;

		validImg			=	true;
		if ((newImgWidth < 100) || (newImgWidth > 10000))
		{
			validImg		=	false;
		}
		if ((newImgHeight < 100) || (newImgHeight > 10000))
		{
			validImg		=	false;
		}
		if ((newImgChannels != 1) && (newImgChannels != 3))
		{
			validImg		=	false;
		}
		if (validImg)
		{
			CONSOLE_DEBUG_W_NUM("newImgChannels\t=", newImgChannels);
			newImgChannels		=	3;
			cDownLoadedImage	=	cvCreateImage(cvSize(	newImgWidth,
															newImgHeight),
															IPL_DEPTH_8U,
															newImgChannels);
		}

		//*	the downloaded image needs to be copied and/or resized to the displayed image
		if (cDownLoadedImage != NULL)
		{
			//*	copy the image data to OUR image
			byteCount	=	newOpenCVImage->height * newOpenCVImage->widthStep;
			memcpy(cDownLoadedImage->imageData, newOpenCVImage->imageData, byteCount);


//			CONSOLE_DEBUG("Creating small image");
			reduceFactor		=	1;
			smallDispalyWidth	=	cDownLoadedImage->width;
			smallDisplayHeight	=	cDownLoadedImage->height;

			CONSOLE_DEBUG_W_NUM("cDownLoadedImage->width\t=",	cDownLoadedImage->width);
			CONSOLE_DEBUG_W_NUM("cDownLoadedImage->height\t=",	cDownLoadedImage->height);


			int		maxWindowWidth	=	800;
			int		maxWindowHeight	=	700;

			while ((smallDispalyWidth > maxWindowWidth) || (smallDisplayHeight > (maxWindowHeight - 50)))
			{
				CONSOLE_DEBUG_W_NUM("smallDisplayHeight\t=", smallDisplayHeight);
				reduceFactor++;
				smallDispalyWidth	=	cDownLoadedImage->width / reduceFactor;
				smallDisplayHeight	=	cDownLoadedImage->height / reduceFactor;
			}
			CONSOLE_DEBUG_W_NUM("reduceFactor\t=", reduceFactor);
			CONSOLE_DEBUG_W_NUM("smallDispalyWidth\t=", smallDispalyWidth);
			CONSOLE_DEBUG_W_NUM("smallDisplayHeight\t=", smallDisplayHeight);

			cDisplayedImage	=	cvCreateImage(cvSize(	smallDispalyWidth,
														smallDisplayHeight),
														IPL_DEPTH_8U,
														3);
			if (cDisplayedImage != NULL)
			{
				CONSOLE_DEBUG("Resizing image");

				//*	Check to see if the original is color
				if ((cDownLoadedImage->nChannels == 3) && (cDownLoadedImage->depth == 8))
				{
					CONSOLE_DEBUG("Original is 8 bit color (3 channels)");
					cvResize(cDownLoadedImage, cDisplayedImage, CV_INTER_LINEAR);
				}
				else if ((cDownLoadedImage->nChannels == 1) && (cDownLoadedImage->depth == 8))
				{
					CONSOLE_DEBUG("Original is 8 bit B/W");
					cvCvtColor(cDownLoadedImage, cDisplayedImage, CV_GRAY2RGB);
				}

				SetWidgetImage(kTab_Image, kImageDisplay_ImageDisplay, cDisplayedImage);
			}
			else
			{
				CONSOLE_DEBUG("Failed to create new image");
			}

			if (cImageTabObjPtr != NULL)
			{
				cImageTabObjPtr->SetImagePtrs(cDownLoadedImage,	cDisplayedImage);
			}
		}
		else
		{

		}
		//*	Update the image size on the screen
		SetImageWindowInfo();
	}
}
#endif // _USE_OPENCV_CPP_

//**************************************************************************************
void	ControllerImage::SetImageWindowInfo(void)
{
char	textString[64];
int		imgWidth;
int		imgHeight;
int		imgChannels;

#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
	imgWidth	=	cDownLoadedImage->cols;
	imgHeight	=	cDownLoadedImage->rows;
	imgChannels	=	cDownLoadedImage->step[1];

#else
	imgWidth	=	cDownLoadedImage->width;
	imgHeight	=	cDownLoadedImage->height;
	imgChannels	=	cDownLoadedImage->nChannels;
#endif // _USE_OPENCV_CPP_

	sprintf(textString, "%4d x %4d", imgWidth, imgHeight);
	SetWidgetText(kTab_Image, kImageDisplay_FrameCnt, textString);

	sprintf(textString, "ch = %d", imgChannels);
	SetWidgetText(kTab_Image, kImageDisplay_Exposure, textString);
}

//**************************************************************************************
void	ControllerImage::SetDownloadInfo(double download_MBytes, double download_seconds)
{
double	download_MB_per_sec;

//	CONSOLE_DEBUG(__FUNCTION__);
	download_MB_per_sec	=	0.0;
	if (download_seconds > 0.0)
	{
		download_MB_per_sec	=	download_MBytes / download_seconds;
	}
	SetWidgetNumber(kTab_ImageInfo,	kImageInfo_DownLoadMBytesVal,	download_MBytes);
	SetWidgetNumber(kTab_ImageInfo,	kImageInfo_DownLoadSecondsVal,	download_seconds);
	SetWidgetNumber(kTab_ImageInfo,	kImageInfo_DownLoadSpeedVal,	download_MB_per_sec);

}

#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
int		gImgNumber	=	1;
char	gImageFileName[64];

//**************************************************************************************
void	ControllerImage::CopyImageToLiveImage(cv::Mat *newOpenCVImage)
{
size_t	byteCount_src;
size_t	byteCount_old;
//int		newImgWidth;
int		newImgHeight;
int		newImgRowStepSize;
//int		newImgChannels;

int		oldImgHeight;
int		oldImgRowStepSize;

//*	displayed image info
int		dspImgHeight;
int		dspImgRowStepSize;
size_t	byteCount_dsp;

//	sprintf(gImageFileName, "debugimg/A-newOpenCVImage%02d.png", gImgNumber);
//	cv::imwrite(gImageFileName, *newOpenCVImage);


	CONSOLE_DEBUG("OpenCV++ not finished!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	CONSOLE_DEBUG("++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
	CONSOLE_DEBUG("+++++           OpenCV++ not finished              +++++");
	CONSOLE_DEBUG("+++++           Finish this NEXT!!!!!!             +++++");
	CONSOLE_DEBUG("++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
	CONSOLE_DEBUG(__FUNCTION__);
	//*	this is just an extra check, it was crashing on testing
	if ((cDownLoadedImage != NULL) && (newOpenCVImage != NULL))
	{
//		DumpCVMatStruct(__FUNCTION__, newOpenCVImage,	"newOpenCVImage");
//		DumpCVMatStruct(__FUNCTION__, cDownLoadedImage,	"cDownLoadedImage");
//		newImgWidth			=	newOpenCVImage->cols;
		newImgHeight		=	newOpenCVImage->rows;
		newImgRowStepSize	=	newOpenCVImage->step[0];
//		newImgChannels		=	newOpenCVImage->step[1];

		byteCount_src		=	newImgHeight * newImgRowStepSize;

		oldImgHeight		=	cDownLoadedImage->rows;
		oldImgRowStepSize	=	cDownLoadedImage->step[0];

		byteCount_old		=	oldImgHeight * oldImgRowStepSize;
//		CONSOLE_DEBUG_W_LONG("byteCount_src\t=",	byteCount_src);
//		CONSOLE_DEBUG_W_LONG("byteCount_old\t=",	byteCount_old);

		if (byteCount_src == byteCount_old)
		{
			CONSOLE_DEBUG("memcpy to cDownLoadedImage");
			//*	copy FROM newOpenCVImage to cDownLoadedImage
			//*	cDownLoadedImage  <<= newOpenCVImage
			memcpy(cDownLoadedImage->data, newOpenCVImage->data, byteCount_src);
		}
		else
		{
			CONSOLE_DEBUG("byteCount_src != byteCount_old");
			CONSOLE_DEBUG("Calling cv::resize");
			cv::resize(	*newOpenCVImage,
						*cDownLoadedImage,
						cDownLoadedImage->size(),
						0,
						0,
						cv::INTER_LINEAR);
		}
//		sprintf(gImageFileName, "debugimg/B-cDownLoadedImage%02d.png", gImgNumber);
//		cv::imwrite(gImageFileName, *cDownLoadedImage);

		//*	double check the displayed image
		if (cDisplayedImage != NULL)
		{
//			DumpCVMatStruct(__FUNCTION__, cDisplayedImage, "cDisplayedImage");
			dspImgHeight		=	cDisplayedImage->rows;
			dspImgRowStepSize	=	cDisplayedImage->step[0];
			byteCount_dsp		=	dspImgHeight * dspImgRowStepSize;

			if (byteCount_dsp == byteCount_old)
			{
				CONSOLE_DEBUG("memcpy to cDisplayedImage");
				//*	cDisplayedImage <<= cDownLoadedImage
				memcpy(cDisplayedImage->data, cDownLoadedImage->data, byteCount_dsp);
			}
			else
			{
				//*	now make a small copy that will fit on the screen.
				CONSOLE_DEBUG("byteCount_dsp != byteCount_old");
				CONSOLE_DEBUG_W_SIZE("byteCount_dsp\t=",	byteCount_dsp);
				CONSOLE_DEBUG_W_SIZE("byteCount_old\t=",	byteCount_old);
				cv::resize(	*newOpenCVImage,
							*cDisplayedImage,
							cDisplayedImage->size(),
							0,
							0,
							cv::INTER_LINEAR);
			}
//			sprintf(gImageFileName, "debugimg/C-cDisplayedImage%02d.png", gImgNumber);
//			cv::imwrite(gImageFileName, *cDisplayedImage);
			gImgNumber++;
		}
		else
		{
			CONSOLE_DEBUG("cDisplayedImage is NULL");
		}
	}
	else
	{
		CONSOLE_DEBUG("((cDownLoadedImage != NULL) && (newOpenCVImage != NULL))");
		CONSOLE_ABORT(__FUNCTION__);
	}
	CalculateHistogramArray();
}
#else
//**************************************************************************************
void	ControllerImage::CopyImageToLiveImage(IplImage *newOpenCVImage)
{
size_t				byteCount_src;
size_t				byteCount_dst;

//	CONSOLE_DEBUG(__FUNCTION__);
	//*	this is just an extra check, it was crashing on testing
	if ((cDownLoadedImage != NULL) && (newOpenCVImage != NULL))
	{
		byteCount_src	=	newOpenCVImage->height * newOpenCVImage->widthStep;
		byteCount_dst	=	cDownLoadedImage->height * cDownLoadedImage->widthStep;

		if (byteCount_src == byteCount_dst)
		{
			//*	copy the new data to the existing buffer
//			CONSOLE_DEBUG_W_NUM("Copying image:byteCount\t=",	byteCount_src);
			if ((cDownLoadedImage->imageData != NULL) && (newOpenCVImage->imageData != NULL))
			{
				memcpy(cDownLoadedImage->imageData, newOpenCVImage->imageData, byteCount_src);

				//*	double check the displayed image
				if (cDisplayedImage != NULL)
				{
					//*	now make a small copy that will fit on the screen.

//					CONSOLE_DEBUG("Resizing image");
					//*	Check to see if the original is color
					if ((cDownLoadedImage->nChannels == 3) && (cDownLoadedImage->depth == 8))
					{
//						CONSOLE_DEBUG("Original is 8 bit color (3 channels)");
						cvResize(cDownLoadedImage, cDisplayedImage, CV_INTER_LINEAR);
					}
					else if ((cDownLoadedImage->nChannels == 1) && (cDownLoadedImage->depth == 8))
					{
//						CONSOLE_DEBUG("Original is 8 bit B/W");
						cvCvtColor(cDownLoadedImage, cDisplayedImage, CV_GRAY2RGB);
					}
				}
				else
				{
					CONSOLE_DEBUG("cDisplayedImage is NULL");
				}
			}
			else
			{
				CONSOLE_DEBUG("Data ptr is NULL!!!!!!!!!!!!!!!!!!!!!!!!");
			//	CONSOLE_ABORT(__FUNCTION__);
			}
		}
		else
		{
			CONSOLE_DEBUG("Byte counts dont match");
		}
	}
}
#endif // _USE_OPENCV_CPP_



#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
//**************************************************************************************
//*	this routine updates the existing image by copying the new image to the old image buffer
//*	it checks to make sure they are compatible
//**************************************************************************************
void	ControllerImage::UpdateLiveWindowImage(cv::Mat *newOpenCVImage, const char *imageFileName)
#else
void	ControllerImage::UpdateLiveWindowImage(IplImage *newOpenCVImage, const char *imageFileName)
#endif // _USE_OPENCV_CPP_
{
bool			imagesAreTheSame;
#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
	unsigned int	rowStepSize;
	unsigned int	bytesPerPixel;
#endif

	CONSOLE_DEBUG("-------------------Start");
	CONSOLE_DEBUG(__FUNCTION__);
	if (newOpenCVImage  != NULL)
	{
		CONSOLE_DEBUG("newOpenCVImage is OK!!!!");
		if ((cDownLoadedImage == NULL) || (cDisplayedImage == NULL))
		{
			CONSOLE_DEBUG("Setting image");
			SetLiveWindowImage(newOpenCVImage);
		}
		else
		{
//			CONSOLE_DEBUG("Updating image");
			imagesAreTheSame	=	true;
	#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
			//*	check if width are the same
			if (newOpenCVImage->cols != cDownLoadedImage->cols)
			{
				imagesAreTheSame	=	false;
				CONSOLE_DEBUG("Failed on width");
				CONSOLE_DEBUG_W_NUM("newOpenCVImage->cols  \t=",	newOpenCVImage->cols);
				CONSOLE_DEBUG_W_NUM("cDownLoadedImage->cols\t=",	cDownLoadedImage->cols);
			}
			//*	check if height are the same
			if (newOpenCVImage->rows != cDownLoadedImage->rows)
			{
				imagesAreTheSame	=	false;
				CONSOLE_DEBUG("Failed on height");
				CONSOLE_DEBUG_W_NUM("newOpenCVImage->rows  \t=",	newOpenCVImage->rows);
				CONSOLE_DEBUG_W_NUM("cDownLoadedImage->rows\t=",	cDownLoadedImage->rows);
			}
			rowStepSize	=	newOpenCVImage->step[0];
			bytesPerPixel	=	newOpenCVImage->step[1];
			if (rowStepSize != cDownLoadedImage->step[0])
			{
				imagesAreTheSame	=	false;
				CONSOLE_DEBUG("Failed on rowStepSize");
				CONSOLE_DEBUG_W_NUM("rowStepSize              \t=",		rowStepSize);
				CONSOLE_DEBUG_W_LONG("cDownLoadedImage->step[0]\t=",	cDownLoadedImage->step[0]);
			}

			if (bytesPerPixel != cDownLoadedImage->step[1])
			{
				imagesAreTheSame	=	false;
				CONSOLE_DEBUG("Failed on bytesPerPixel");
				CONSOLE_DEBUG_W_NUM("bytesPerPixel            \t=",		bytesPerPixel);
				CONSOLE_DEBUG_W_LONG("cDownLoadedImage->step[1]\t=",	cDownLoadedImage->step[1]);
			}
	#else
			//*	check if width are the same
			if (newOpenCVImage->width != cDownLoadedImage->width)
			{
				imagesAreTheSame	=	false;
				CONSOLE_DEBUG("Failed on width");
				CONSOLE_DEBUG_W_NUM("newOpenCVImage->width  \t=",	newOpenCVImage->width);
				CONSOLE_DEBUG_W_NUM("cDownLoadedImage->width\t=",	cDownLoadedImage->width);

			}
			//*	check if height are the same
			if (newOpenCVImage->height != cDownLoadedImage->height)
			{
				imagesAreTheSame	=	false;
				CONSOLE_DEBUG("Failed on height");
				CONSOLE_DEBUG_W_NUM("newOpenCVImage->height  \t=",	newOpenCVImage->height);
				CONSOLE_DEBUG_W_NUM("cDownLoadedImage->height\t=",	cDownLoadedImage->height);
			}
			//*	check if nChannels are the same
			if (newOpenCVImage->nChannels != cDownLoadedImage->nChannels)
			{
				imagesAreTheSame	=	false;
				CONSOLE_DEBUG("Failed on nChannels");
				CONSOLE_DEBUG_W_NUM("newOpenCVImage->nChannels  \t=",	newOpenCVImage->nChannels);
				CONSOLE_DEBUG_W_NUM("cDownLoadedImage->nChannels\t=",	cDownLoadedImage->nChannels);
			}
			//*	check if depth are the same
			if (newOpenCVImage->depth != cDownLoadedImage->depth)
			{
				imagesAreTheSame	=	false;
				CONSOLE_DEBUG("Failed on depth");
				CONSOLE_DEBUG_W_NUM("newOpenCVImage->depth  \t=",	newOpenCVImage->depth);
				CONSOLE_DEBUG_W_NUM("cDownLoadedImage->depth\t=",	cDownLoadedImage->depth);
			}
			//*	check if widthStep are the same
			if (newOpenCVImage->widthStep != cDownLoadedImage->widthStep)
			{
				imagesAreTheSame	=	false;
				CONSOLE_DEBUG("Failed on widthStep");
				CONSOLE_DEBUG_W_NUM("newOpenCVImage->widthStep  \t=",	newOpenCVImage->widthStep);
				CONSOLE_DEBUG_W_NUM("cDownLoadedImage->widthStep\t=",	cDownLoadedImage->widthStep);
			}
	#endif // _USE_OPENCV_CPP_

			if (imagesAreTheSame)
			{
				CONSOLE_DEBUG("imagesAreTheSame");
				CopyImageToLiveImage(newOpenCVImage);
			}
			else
			{
				CONSOLE_DEBUG("images are different !!!!!!!!!!!!!!!!!!!!!!!!!!!!");
				//*	check to see if our temporary image exists
				if (cColorImage == NULL)
				{
				int		newImgWidth;
				int		newImgHeight;

			#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
					newImgWidth		=	newOpenCVImage->cols;
					newImgHeight	=	newOpenCVImage->rows;
					cColorImage		=	new cv::Mat(cv::Size(	newImgWidth,
																newImgHeight),
																CV_8UC3);
			#else
					newImgWidth		=	newOpenCVImage->width;
					newImgHeight	=	newOpenCVImage->height;
					cColorImage		=	cvCreateImage(cvSize(	newImgWidth,
																newImgHeight),
																IPL_DEPTH_8U,
																3);
			#endif // _USE_OPENCV_CPP_
				}
				if (cColorImage != NULL)
				{
					CONSOLE_DEBUG("Converting to color cv::cvtColor/cvCvtColor");
				#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
					//*	convert gray scale to color
					cv::cvtColor(*newOpenCVImage, *cColorImage, cv::COLOR_GRAY2BGR);
//					cv::imwrite("newColorImg.png", *newOpenCVImage);
//					cv::imwrite("cColorImage.png", *cColorImage);
				#else
					cvCvtColor(newOpenCVImage, cColorImage, CV_GRAY2RGB);
				#endif // _USE_OPENCV_CPP_
					CopyImageToLiveImage(cColorImage);
				}
				else
				{
					CONSOLE_DEBUG("Failed to create new color image");
				}
			}
			cUpdateWindow	=	true;
		}
	}
	else
	{
		CONSOLE_DEBUG("newOpenCVImage is NULL");
	}

	//*	was a file name supplied
	if (imageFileName != NULL)
	{
		strcpy(cImageFileName, imageFileName);
		SetWidgetText(kTab_Image, kImageDisplay_Title, imageFileName);
	}


	CONSOLE_DEBUG_W_STR(__FUNCTION__, "-------------------exit");
}

//**************************************************************************************
void	ControllerImage::UpdateLiveWindowInfo(	TYPE_CameraProperties	*cameraProp,
												const int				framesRead,
												const double			exposure_Secs,
												const char				*filterName,
												const char				*objectName
												)
{
char	lineBuff[64];

	CONSOLE_DEBUG(__FUNCTION__);

	SetWidgetNumber(kTab_Image, kImageDisplay_Gain,			cameraProp->Gain);

	sprintf(lineBuff, "%1.1f F",			(cameraProp->CCDtemperature * 9.0/5.0) +32.0);
	SetWidgetText(	kTab_Image, kImageDisplay_CameraTemp,	lineBuff);


	SetWidgetNumber(kTab_Image, kImageDisplay_FrameCnt,		framesRead);
	SetWidgetNumber(kTab_Image, kImageDisplay_Exposure,		exposure_Secs);
	SetWidgetText(	kTab_Image, kImageDisplay_Object,		objectName);

	if (filterName != NULL)
	{
		SetWidgetText(	kTab_Image, kImageDisplay_Filter,	filterName);
	}

	SetWidgetNumber(kTab_Image, kImageDisplay_FramesSaved,	cameraProp->SavedImageCnt);

	cUpdateWindow	=	true;
}


//*****************************************************************************
void	ControllerImage::CalculateHistogramArray(void)
{
int32_t			imageDataLen;
int32_t			currPixValue;
int32_t			iii;
int32_t			ccc;
//uint16_t		*imageDataPtr16bit;
uint8_t			*imageDataPtr8bit;
int32_t			redValue;
int32_t			grnValue;
int32_t			bluValue;
int32_t			lumValue;		//*	luminance value
int32_t			peakPixelIdx;
int32_t			peakPixelCount;
bool			lookingForMin;
int				imgWidth;
int				imgHeight;
int				imgChannels;
//int				imgRowStepSize;
uint8_t			*imgDataPtr;
char			textString[32];

//	SETUP_TIMING();
//
//	CONSOLE_DEBUG(__FUNCTION__);
//	START_TIMING();

	cPeakHistogramValue	=	0;
	cMaxHistogramValue	=	0;
	cMaxHistogramPixCnt	=	0;
	cMaxRedValue		=	0;
	cMaxGrnValue		=	0;
	cMaxBluValue		=	0;
	cMaxGryValue		=	0;

	//*	clear out the histogram data array
	memset(cHistogramLum,	0,	sizeof(cHistogramLum));
	memset(cHistogramRed,	0,	sizeof(cHistogramRed));
	memset(cHistogramGrn,	0,	sizeof(cHistogramGrn));
	memset(cHistogramBlu,	0,	sizeof(cHistogramBlu));

	if (cDownLoadedImage != NULL)
	{

	#if defined(_USE_OPENCV_CPP_) || (CV_MAJOR_VERSION >= 4)
		imgWidth		=	cDownLoadedImage->cols;
		imgHeight		=	cDownLoadedImage->rows;
//		imgRowStepSize	=	cDownLoadedImage->step[0];
		imgChannels		=	cDownLoadedImage->step[1];
		imgDataPtr		=	cDownLoadedImage->data;
//		CONSOLE_DEBUG_W_LONG("cDownLoadedImage->step[0]\t=",	cDownLoadedImage->step[0]);
//		CONSOLE_DEBUG_W_LONG("cDownLoadedImage->step[1]\t=",	cDownLoadedImage->step[1]);

	#else
		imgWidth		=	cDownLoadedImage->width;
		imgHeight		=	cDownLoadedImage->height;
		imgChannels		=	cDownLoadedImage->nChannels;
		imgDataPtr		=	NULL;
	#endif // _USE_OPENCV_CPP_
		//*	calculate the size of the image
		imageDataLen	=	imgWidth * imgHeight;

//		CONSOLE_DEBUG_W_NUM("imgWidth      \t=",	imgWidth);
//		CONSOLE_DEBUG_W_NUM("imgHeight     \t=",	imgHeight);
//		CONSOLE_DEBUG_W_NUM("imgChannels   \t=",	imgChannels);
//		CONSOLE_DEBUG_W_NUM("imgRowStepSize\t=",	imgRowStepSize);
//		CONSOLE_DEBUG_W_NUM("imageDataLen  \t=",	imageDataLen);

		if (imgDataPtr != NULL)
		{
			//*	figure out what type of image it is
			switch(imgChannels)
			{
				case 1:
					imageDataPtr8bit	=	(uint8_t *)imgDataPtr;
					for (iii=0; iii<imageDataLen; iii++)
					{
						currPixValue	=	imageDataPtr8bit[iii] & 0x00ff;

						cHistogramLum[currPixValue]++;

						if (currPixValue > cMaxGryValue)
						{
							cMaxGryValue	=	currPixValue;
						}
					}
					break;

//				case kImageType_RAW16:
//					imageDataPtr16bit	=	(uint16_t *)cCameraDataBuffer;
//					for (iii=0; iii<imageDataLen; iii++)
//					{
//						//*	for 16 bit data, we shift it right 8 bits
//						currPixValue	=	imageDataPtr16bit[iii] & 0x00ffff;
//						currPixValue	=	(currPixValue >> 8) & 0x00ff;
//						cHistogramLum[currPixValue]++;
//					}
//					break;


				case 3:
//					CONSOLE_DEBUG("Processing histogram for RGB image");
					imageDataPtr8bit	=	(uint8_t *)imgDataPtr;
					ccc					=	0;
					for (iii=0; iii<imageDataLen; iii++)
					{
						//*	openCV uses BGR instead of RGB
						//*	https://docs.opencv.org/master/df/d24/tutorial_js_image_display.html
						bluValue	=	imageDataPtr8bit[ccc + 0] & 0x00ff;
						grnValue	=	imageDataPtr8bit[ccc + 1] & 0x00ff;
						redValue	=	imageDataPtr8bit[ccc + 2] & 0x00ff;
						cHistogramRed[redValue]++;
						cHistogramGrn[grnValue]++;
						cHistogramBlu[bluValue]++;

						if (redValue > cMaxRedValue)
						{
							cMaxRedValue	=	redValue;
						}
						if (grnValue > cMaxGrnValue)
						{
							cMaxGrnValue	=	grnValue;
						}
						if (bluValue > cMaxBluValue)
						{
							cMaxBluValue	=	bluValue;
						}
						lumValue	=	(redValue + grnValue + bluValue) / 3;
						if (lumValue > cMaxGryValue)
						{
							cMaxGryValue	=	lumValue;
						}
						ccc	+=	3;
					}
					//*	now calculate the luminance
					for (iii=0; iii<256; iii++)
					{
						lumValue	=	cHistogramRed[iii];
						lumValue	+=	cHistogramGrn[iii];
						lumValue	+=	cHistogramBlu[iii];
						lumValue	=	(lumValue / 3);

						cHistogramLum[iii]	=	lumValue;
					}

					break;

				default:
					break;

			}
		}
		//*	now go through the array and find the peak value and max value
		peakPixelIdx		=	-1;
		peakPixelCount		=	0;
		lookingForMin		=	true;
		for (iii=0; iii<256; iii++)
		{
			//*	find the minimum value
			if (lookingForMin && (cHistogramLum[iii] > 0))
			{
				cMinHistogramValue	=	iii;
				lookingForMin		=	false;
			}
			//*	find the maximum value
			if (cHistogramLum[iii] > 0)
			{
				cMaxHistogramValue	=	iii;
			}
			//*	find the peak value
			if (cHistogramLum[iii] > peakPixelCount)
			{
				peakPixelIdx	=	iii;
				peakPixelCount	=	cHistogramLum[iii];
			}
		}
		cPeakHistogramValue	=	peakPixelIdx;

		//*	look for maximum pixel counts
		cMaxHistogramPixCnt	=	0;

		//*	purposely skip the first and last
//		for (iii=1; iii<255; iii++)
//		for (iii=5; iii<250; iii++)
		for (iii=0; iii<256; iii++)
		{
			if (cHistogramLum[iii] > cMaxHistogramPixCnt)
			{
				cMaxHistogramPixCnt	=	cHistogramLum[iii];
			}
			if (cHistogramRed[iii] > cMaxHistogramPixCnt)
			{
				cMaxHistogramPixCnt	=	cHistogramRed[iii];
			}
			if (cHistogramGrn[iii] > cMaxHistogramPixCnt)
			{
				cMaxHistogramPixCnt	=	cHistogramGrn[iii];
			}
			if (cHistogramBlu[iii] > cMaxHistogramPixCnt)
			{
				cMaxHistogramPixCnt	=	cHistogramBlu[iii];
			}
		}

//		DEBUG_TIMING("Time to calculate histogram data (milliseconds)\t=");

//		CONSOLE_DEBUG_W_NUM("cMinHistogramValue\t=",	cMinHistogramValue);
//		CONSOLE_DEBUG_W_NUM("cMaxHistogramValue\t=",	cMaxHistogramValue);
//		CONSOLE_DEBUG_W_NUM("cPeakHistogramValue\t=",	cPeakHistogramValue);
//		CONSOLE_DEBUG_W_NUM("cMaxHistogramPixCnt\t=",	cMaxHistogramPixCnt);
//
//		CONSOLE_DEBUG_W_NUM("cMaxRedValue\t=",	cMaxRedValue);
//		CONSOLE_DEBUG_W_NUM("cMaxGrnValue\t=",	cMaxGrnValue);
//		CONSOLE_DEBUG_W_NUM("cMaxBluValue\t=",	cMaxBluValue);
//		CONSOLE_DEBUG_W_NUM("cMaxGryValue\t=",	cMaxGryValue);

		sprintf(textString,	"R=%3.0f%%", ((100.0 * cMaxRedValue) / 255));
		SetWidgetText(		kTab_Image, kImageDisplay_HistRedPerct, textString);

		sprintf(textString,	"G=%3.0f%%", ((100.0 * cMaxGrnValue) / 255));
		SetWidgetText(		kTab_Image, kImageDisplay_HistGrnPerct, textString);

		sprintf(textString,	"B=%3.0f%%", ((100.0 * cMaxBluValue) / 255));
		SetWidgetText(		kTab_Image, kImageDisplay_HistBluPerct, textString);

		SetWidgetProgress(	kTab_Image, kImageDisplay_LumBar, cMaxGryValue, 255);
		SetWidgetProgress(	kTab_Image, kImageDisplay_RedBar, cMaxRedValue, 255);
		SetWidgetProgress(	kTab_Image, kImageDisplay_GrnBar, cMaxGrnValue, 255);
		SetWidgetProgress(	kTab_Image, kImageDisplay_BluBar, cMaxBluValue, 255);
	}
	else
	{
		CONSOLE_DEBUG("cDownLoadedImage is NULL");
	}
}

//*****************************************************************************
void	ControllerImage::SaveHistogram(void)
{
	CONSOLE_DEBUG(__FUNCTION__);
	CONSOLE_DEBUG("Not finished!!!!!!!!!!!!!!!!!!!");

}

//*****************************************************************************
//0123456789 123456789
//OBJECT  = 'Moon    '           / Observation title
//TELESCOP= 'Boller & Chivens-4-inch-Finder' / Telescope
//*****************************************************************************
static void	GetDataFromFitsLine(char *cardData, char *valueString)
{
int		jjj;
int		ccc;

//	CONSOLE_DEBUG_W_STR("cardData\t=",	cardData);
	jjj		=	10;
	ccc		=	0;
	if (cardData[jjj] == '\'')
	{
		jjj++;
		while ((cardData[jjj] != '\'') && (ccc < 64))
		{
			valueString[ccc]	=	cardData[jjj];
			ccc++;
			jjj++;
		}
		valueString[ccc]	=	0;
	}
	else
	{
		while (cardData[jjj] == 0x20)
		{
			jjj++;
		}
		while ((cardData[jjj] > 0x20) && (ccc < 64) && (cardData[jjj] != '\''))
		{
			valueString[ccc]	=	cardData[jjj];
			ccc++;
			jjj++;
		}
	}
	valueString[ccc]	=	0;
	StripTrailingSpaces(valueString);
}

//*****************************************************************************
void	ControllerImage::ProcessFitsHeader(const char *imageFilePath)
{
fitsfile	*fptr;
char		card[FLEN_CARD];
char		valueString[FLEN_CARD];
int			status;
int			nkeys;
int			iii;
int			fitsKeyWordEnum;

	CONSOLE_DEBUG(__FUNCTION__);
	cFitsHdrCnt	=	0;
	status	=	0;	///* MUST initialize status
	fits_open_file(&fptr, imageFilePath, READONLY, &status);
	if (status == 0)
	{
		fits_get_hdrspace(fptr, &nkeys, NULL, &status);

		for (iii = 1; iii <= nkeys; iii++)
		{
			status			=	0;
			fits_read_record(fptr, iii, card, &status);	//* read fits record
			card[80]		=	0;

			//*	save the fits header text for display
			if (cFitsHdrCnt < kMaxFitsHdrLines)
			{
				strcpy(cFitsHeaderText[cFitsHdrCnt].fitsLine, card);
				cFitsHdrCnt++;
			}

			fitsKeyWordEnum	=	FITS_FindKeyWordEnum(card);
			switch(fitsKeyWordEnum)
			{
				case kFitsKeyword_ApertureDiam:
					GetDataFromFitsLine(card, valueString);
					cFitsHeaderData.ApertureDiam	=	atof(valueString);
//					CONSOLE_DEBUG_W_STR("kFitsKeyword_ApertureDiam   \t=", valueString);
//					CONSOLE_DEBUG_W_DBL("cFitsHeaderData.ApertureDiam\t=", cFitsHeaderData.ApertureDiam);
//					CONSOLE_DEBUG_W_DBL("cFitsHeaderData.FocalLength \t=", cFitsHeaderData.FocalLength);
					if ((cFitsHeaderData.FocalRatio < 0.1) && (cFitsHeaderData.ApertureDiam > 0.0))
					{
						cFitsHeaderData.FocalRatio	=	cFitsHeaderData.FocalLength	 / cFitsHeaderData.ApertureDiam;
						CONSOLE_DEBUG_W_DBL("cFitsHeaderData.FocalRatio\t=", cFitsHeaderData.FocalRatio);
					}
					break;

				case kFitsKeyword_Camera:
					GetDataFromFitsLine(card, valueString);
					strcpy(cFitsHeaderData.Camera, valueString);
					break;

				case kFitsKeyword_CCDTEMP:
					GetDataFromFitsLine(card, valueString);
					strcat(valueString, " deg C");
					SetWidgetText(	kTab_Image, kImageDisplay_CameraTemp, valueString);
					break;

				case kFitsKeyword_Date:
					break;

				case kFitsKeyword_EXPTIME:
					GetDataFromFitsLine(card, valueString);
					SetWidgetText(	kTab_Image, kImageDisplay_Exposure, valueString);
					break;

				case kFitsKeyword_Filter:
					GetDataFromFitsLine(card, valueString);
					SetWidgetText(	kTab_Image, kImageDisplay_Filter, valueString);
					if (strcasecmp(valueString, "HA") == 0)
					{
						strcpy(cFitsHeaderData.Filter, "Hydrogen Alpha");
					}
					else if (strcasecmp(valueString, "Sii") == 0)
					{
						strcpy(cFitsHeaderData.Filter, "Sulphur 2");
					}
					else if (strcasecmp(valueString, "Oiii") == 0)
					{
						strcpy(cFitsHeaderData.Filter, "Oxygen 3");
					}
					else
					{
						strcpy(cFitsHeaderData.Filter, valueString);
					}
					break;

				case kFitsKeyword_FRatio:
					GetDataFromFitsLine(card, valueString);
					cFitsHeaderData.FocalRatio	=	atof(valueString);
					break;

				case kFitsKeyword_FocalLength:
					GetDataFromFitsLine(card, valueString);
					cFitsHeaderData.FocalLength	=	atof(valueString);
					break;

				case kFitsKeyword_Gain:
					GetDataFromFitsLine(card, valueString);
					SetWidgetText(	kTab_Image, kImageDisplay_Gain, valueString);
					break;

				case kFitsKeyword_Location:
					GetDataFromFitsLine(card, valueString);
					strcpy(cFitsHeaderData.Location, valueString);
					break;

				case kFitsKeyword_MoonPhase:
					GetDataFromFitsLine(card, valueString);
					strcpy(cFitsHeaderData.MoonPhase, valueString);
					break;

				case kFitsKeyword_Object:
					GetDataFromFitsLine(card, valueString);
					SetWidgetText(	kTab_Image, kImageDisplay_Object, valueString);
					strcpy(cFitsHeaderData.Object, valueString);
					break;

				case kFitsKeyword_Observer:
					GetDataFromFitsLine(card, valueString);
					strcpy(cFitsHeaderData.Observer, valueString);
					break;

				case kFitsKeyword_Observatory:
					GetDataFromFitsLine(card, valueString);
					strcpy(cFitsHeaderData.Observatory, valueString);
					break;

				case kFitsKeyword_Telescope:
					GetDataFromFitsLine(card, valueString);
					strcpy(cFitsHeaderData.Telescope, valueString);
					if (strncasecmp(valueString, "Mt Wilson", 9) == 0)
					{
						strcpy(cFitsHeaderData.Location, "Mt Wilson, CA");
					}
					break;

				case kFitsKeyword_TimeUTC:
					GetDataFromFitsLine(card, valueString);
					strcpy(cFitsHeaderData.Time_UTC, valueString);
					break;

				case kFitsKeyword_TimeLocal:
					GetDataFromFitsLine(card, valueString);
					strcpy(cFitsHeaderData.Time_Local, valueString);
					break;

				case kFitsKeyword_WebSite:
					GetDataFromFitsLine(card, valueString);
					strcpy(cFitsHeaderData.Website, valueString);
					break;

				case -1:
					break;

				default:
					CONSOLE_DEBUG_W_NUM("fitsKeyWordEnum\t=", fitsKeyWordEnum);
					break;

			}
		}
		strcpy(cFitsHeaderText[cFitsHdrCnt].fitsLine, "END");
		cFitsHdrCnt++;
		cFitsHeaderTabObjPtr->SetFitsHeaderData(cFitsHeaderText, cFitsHdrCnt);
	}
	else
	{
		CONSOLE_DEBUG_W_STR("Failed to open", imageFilePath);
	}
	status	=	0;	///* MUST initialize status
	fits_close_file(fptr, &status);

	if (status)				//* print any error messages
	{
		fits_report_error(stderr, status);
	}
}

//*****************************************************************************
void	ControllerImage::DrawTextString2(	const int	xxLoc1,
											const int	xxLoc2,
											const int	yyLoc,
											const char	*textString1,
											const char	*textString2)
{
cv::Point	textLoc;
cv::Scalar	fontColor	=	CV_RGB(255,255,255);
double		fontScale	=	3.0;

	textLoc.x	=	xxLoc1;
	textLoc.y	=	yyLoc;
	cv::putText(	*cDownLoadedImage,
					textString1,
					textLoc,
					cv::FONT_HERSHEY_SIMPLEX,		//	gFontInfo[curFontNum].fontID,
					fontScale,						//	gFontInfo[curFontNum].scale,
					fontColor,
					6								//	gFontInfo[curFontNum].thickness
					);


	textLoc.x	=	xxLoc2;
	textLoc.y	=	yyLoc;
	cv::putText(	*cDownLoadedImage,
					textString2,
					textLoc,
					cv::FONT_HERSHEY_SIMPLEX,		//	gFontInfo[curFontNum].fontID,
					fontScale,						//	gFontInfo[curFontNum].scale,
					fontColor,
					6								//	gFontInfo[curFontNum].thickness
					);

//	cv::putText(	*cDisplayedImage,
//					textString,
//					textLoc,
//					cv::FONT_HERSHEY_PLAIN,		//	gFontInfo[curFontNum].fontID,
//					fontScale,						//	gFontInfo[curFontNum].scale,
//					fontColor,
//					2							//	gFontInfo[curFontNum].thickness
//					);
}


//*****************************************************************************
void	ControllerImage::DrawTitleBlock(void)
{
int			xxLoc1;
int			xxLoc2;
int			yyLoc;
char		theString[128];
int			deltaY	=	100;
//cv::Scalar	fontColor;

	CONSOLE_DEBUG(__FUNCTION__);

	xxLoc1	=	75;
	xxLoc2	=	700;
	yyLoc	=	100;
	if (strlen(cFitsHeaderData.Object) > 0)
	{
		DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"Object:",	cFitsHeaderData.Object);
		yyLoc	+=	deltaY;

		//*	check to see if the object is the Moon
		if (strcasecmp(cFitsHeaderData.Object, "Moon") == 0)
		{
			if (strlen(cFitsHeaderData.MoonPhase) > 0)
			{
				DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"MoonPhase:",	cFitsHeaderData.MoonPhase);
				yyLoc	+=	deltaY;
			}
		}
	}
	if (strlen(cFitsHeaderData.Time_UTC) > 0)
	{
		DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"Date/Time:",	cFitsHeaderData.Time_UTC);
		yyLoc	+=	deltaY;
	}
	if (strlen(cFitsHeaderData.Camera) > 0)
	{
		DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"Camera:",	cFitsHeaderData.Camera);
		yyLoc	+=	deltaY;
	}
	if (strlen(cFitsHeaderData.Filter) > 0)
	{
		DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"Filter:",	cFitsHeaderData.Filter);
		yyLoc	+=	deltaY;
	}
	if (strlen(cFitsHeaderData.Telescope) > 0)
	{
		DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"Telescope:",	cFitsHeaderData.Telescope);
		yyLoc	+=	deltaY;
	}
	if (cFitsHeaderData.FocalRatio > 1.0)
	{
		sprintf(theString, "%3.2f", cFitsHeaderData.FocalRatio);
		DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"F-Ratio:",	theString);
		yyLoc	+=	deltaY;
	}
	if (strlen(cFitsHeaderData.Observer) > 0)
	{
		DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"Observer:",	cFitsHeaderData.Observer);
		yyLoc	+=	deltaY;
	}
	if (strlen(cFitsHeaderData.Observatory) > 0)
	{
		DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"Observatory:",	cFitsHeaderData.Observatory);
		yyLoc	+=	deltaY;
	}
	if (strlen(cFitsHeaderData.Location) > 0)
	{
		DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"Location:",	cFitsHeaderData.Location);
		yyLoc	+=	deltaY;
	}
	if (strlen(cFitsHeaderData.Website) > 0)
	{
		DrawTextString2(xxLoc1,	xxLoc2, yyLoc,	"Website:",	cFitsHeaderData.Website);
		yyLoc	+=	deltaY;
	}
}

//*****************************************************************************
void	ControllerImage::SaveImage(void)
{
char		outputFileName[256];
int			openCVerr;
cv::Mat		*smallImage;
int			smallDispalyWidth;
int			smallDisplayHeight;
int			reduceFactor;
int			maxWindowWidth	=	1000;
int			maxWindowHeight	=	800;

	strcpy(outputFileName, cImageFileNameRoot);
	strcat(outputFileName, "-si");
	strcat(outputFileName, ".jpg");
	openCVerr	=	cv::imwrite(outputFileName, *cDownLoadedImage);

	//-----------------------------------------------------------------------
	//*	create a small image

	//*	calculate the size for the small image
	reduceFactor		=	1;
	smallDispalyWidth	=	cDownLoadedImage->cols;
	smallDisplayHeight	=	cDownLoadedImage->rows;


	while ((smallDispalyWidth > maxWindowWidth) || (smallDisplayHeight > (maxWindowHeight - 50)))
	{
		reduceFactor++;
		smallDispalyWidth	=	cDownLoadedImage->cols / reduceFactor;
		smallDisplayHeight	=	cDownLoadedImage->rows / reduceFactor;
//		CONSOLE_DEBUG_W_NUM("smallDisplayHeight\t=", smallDisplayHeight);
	}

	smallImage	=	new cv::Mat(cv::Size(	smallDispalyWidth,
											smallDisplayHeight),
											CV_8UC3);
	if (smallImage != NULL)
	{
		cv::resize(	*cDownLoadedImage,
					*smallImage,
					smallImage->size(),
					0,
					0,
					cv::INTER_LINEAR);
		strcpy(outputFileName, cImageFileNameRoot);
		strcat(outputFileName, "-si-small");
		strcat(outputFileName, ".jpg");
		openCVerr	=	cv::imwrite(outputFileName, *smallImage);

		delete smallImage;
	}
}

#endif // _ENABLE_CTRL_IMAGE_
