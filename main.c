/* ---------- ---------- ---------- ---------- ---------- ---------- ----------
OMX2GPX - A simple Geonaute OnMove 220 GPS watch OMH/OMD to GPX converter.
Code: Copyright (C) 2016 Clement CORDE - c1702@yahoo.com
Project started on 2016/10/08.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
---------- ---------- ---------- ---------- ---------- ---------- ---------- */


#define	VER_MAJOR	1
#define	VER_MINOR	3

#define	ELEVATION	1		// Comment to remove all elevation stuff.
#define	GOOGLE_MAPS_API_KEY	"***************************************"		// *YOUR* API key.


// Includes.
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ctypes.h"
#include "format.h"


#define	TRUNC(x) ((x) >= 0 ? (x) + 0.5 : (x) - 0.5)

struct SOptions
{
	u8	nOptions;

	// Zero point data, if provided.
	struct SDataRecordGPS	sZeroGPS;
	struct SHalfCurve		sZeroCurve;
};
struct SOptions	gOptions;

enum
{
	e_OPT_HeaderOnly		= 1,		// Display header only.
	e_OPT_GPXFile			= 1 << 1,	// Generate GPX file.
	e_OPT_GPSDataDisplay	= 1 << 2,	// Display GPS data to screen.
	e_OPT_Elevation			= 1 << 3,	// Retrieve elevation.
	e_OPT_ZeroPt			= 1 << 4,	// 1 = Zero point information provided.

	e_OPT_DefaultOptions	= (e_OPT_GPSDataDisplay | e_OPT_GPXFile)
};


// Load file.
// Out: malloc and load file into ppBuf, return file size through pnSz.
void FileLoad(char *pFilename, u8 **ppBuf, u32 *pnSz)
{
	FILE	*pFile = NULL;
	*ppBuf = NULL;

	// Open file.
	pFile = fopen(pFilename, "rb");
	if (pFile == NULL)
	{
		fprintf(stderr, "FileLoad(): Error opening file '%s'.\n", pFilename);
		goto _err1;
	}
	// Retrieve file size.
	fseek(pFile, 0, SEEK_END);
	*pnSz = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	// Malloc.
	if ((*ppBuf = (u8 *)malloc(*pnSz)) == NULL)
	{
		fprintf(stderr, "FileLoad(): Error allocating memory.\n");
		goto _err1;
	}
	// Loading.
	u32	nSz;
	nSz = fread(*ppBuf, 1, *pnSz, pFile);
	fclose(pFile); pFile = NULL;
	// Size check.
	if (nSz != *pnSz)
	{
		fprintf(stderr, "FileLoad(): Read error! %d bytes expected, %d bytes read.\n", *pnSz, nSz);
		goto _err1;
	}

	return;

_err1:
	if (pFile != NULL) fclose(pFile);
	if (*ppBuf != NULL) free(*ppBuf);
	exit(1);
}

// Show help.
void Help_Display(void)
{
	printf("Usage: OMX2GPX [options] <ACT file name, without extension>\n\n");
	printf("Options:\n");
	printf("\t-h: Display help.\n");
	printf("\t-r: Display header only.\n");
	printf("\t-d: Do not display GPS data.\n");
	printf("\t-g: Do not generate GPX file.\n");
#ifdef ELEVATION
	printf("\t-e: Retrieve elevation (Google Maps Elevation API).\n");
#endif
	printf("\t-z:lat:lon[:spd[:hr]] Provide zero point information.\n");
	printf("\t    lat & lon: up to 6 decimal places. spd: up to 2 decimal places.\n");
	printf("\n");

	exit(1);
}

// Command line arguments check.
void CommandLine_Check(int nArgc, char **ppArgv, char **ppFilename)
{
	u32	nCur = 1;
	char	*pStr;

	while (nCur < (u32)nArgc)
	{
		pStr = *(ppArgv + nCur);	// Ptr sur l'arg en cours.

		if (*pStr == '-')
		{
			// Argument starts with '-'.
			pStr++;		// Skips the '-'.
			switch(*pStr)
			{
			// -h: Display help.
			case 'h':
			case 'H':
			case '?':
				Help_Display();
				break;

			// -r: Display header only.
			case 'r':
			case 'R':
				gOptions.nOptions |= e_OPT_HeaderOnly;
				break;

			// -g: Do not generate GPX file.
			case 'g':
			case 'G':
				gOptions.nOptions &= ~e_OPT_GPXFile;
				break;

			// -d: Do not display GPS data.
			case 'd':
			case 'D':
				gOptions.nOptions &= ~e_OPT_GPSDataDisplay;
				break;

#ifdef ELEVATION
			// -e: Retrieve elevation (Google Maps Elevation API).
			case 'e':
			case 'E':
				gOptions.nOptions |= e_OPT_Elevation;
				break;
#endif

			// -z:lat:lon[:spd[:hr]] Provide zero point information.
			case 'z':
			case 'Z':
				{
					double	nLat = 0.0, nLon = 0.0, nSpd = 0.0;
					u32	nHr = 0;
					u32	n;
					n = sscanf(pStr, "%*c:%lf:%lf:%lf:%d", &nLat, &nLon, &nSpd, &nHr);
					if (n < 2)
					{
						fprintf(stderr, "CommandLine_Check(): z: Could not read parameters.\n");
						exit(1);
					}
					// lat: -90.0 <= value <= 90.0
					if (nLat < -90.0 || nLat > 90.0)
					{
						fprintf(stderr, "CommandLine_Check(): z: Latitude should be in the [-90.0;90.0] interval.\n");
						exit(1);
					}
					// lon: -180.0 <= value < 180.0
					if (nLon < -180.0 || nLon > 180.0)
					{
						fprintf(stderr, "CommandLine_Check(): z: Longitude should be in the [-180.0;180.0] interval.\n");
						exit(1);
					}

					// Save info.
					gOptions.sZeroGPS.nLatitude = (s32)TRUNC(nLat * 1000000);
					gOptions.sZeroGPS.nLongitude = (s32)TRUNC(nLon * 1000000);
					if (n >= 3) gOptions.sZeroCurve.nSpeed = (u32)TRUNC(nSpd * 100);
					if (n >= 4) gOptions.sZeroCurve.nHeartRate = nHr;
					gOptions.nOptions |= e_OPT_ZeroPt;
				}
				break;

			default:
				fprintf(stderr, "CommandLine_Check(): Unknown parameter: '%s'.\n", *(ppArgv + nCur));
				break;
			}
		}
		else
		{
			// Argument doesn't start with '-', it's a file name.
			*ppFilename = pStr;
		}

		nCur++;		// Next arg.
	}

}

// Display GPS point info.
void Point_Display(u32 nGPSPtNo, time_t nTimeStamp, struct SDataRecordGPS *pGPS, struct SHalfCurve *pCurve, double *pElev, FILE *pGpxFile)
{
	if (pGPS->nTime != pCurve->nTime) printf("ERROR! GPS time: %d - Curve time: %d\n", pGPS->nTime, pCurve->nTime);

	nTimeStamp += pGPS->nTime;				// Add GPS' point time to starting time.

	struct tm sTime2;
	tzset();
	localtime_r(&nTimeStamp, &sTime2);		// Retrieve date and time from current timestamp.

	// Display info.
	if (gOptions.nOptions & e_OPT_GPSDataDisplay)
	{
		printf("%d - Lat: %f - Long: %f - ",
			nGPSPtNo,
			(double)pGPS->nLatitude / 1000000, (double)pGPS->nLongitude / 1000000);
#ifdef ELEVATION
		if (gOptions.nOptions & e_OPT_Elevation)
			printf("Ele: %f - ", *pElev);
#endif
		printf("Dist: %dm - Time: %02d:%02d'%02d\" (%ds) - Speed: %.02fkm/h - KCal: %d - Heart: ",
			pGPS->nDistance,
			sTime2.tm_hour, sTime2.tm_min, sTime2.tm_sec, pGPS->nTime,
			(double)pCurve->nSpeed / 100,
			pCurve->nKCal
			);
		if (pCurve->nHeartRate) printf("%d", pCurve->nHeartRate); else printf("N/A");
		printf("\n");
	}

	// Save info into GPX.
	if (gOptions.nOptions & e_OPT_GPXFile)
	{
		// Base values: lat, lon, time and speed.
		fprintf(pGpxFile, "\t\t<trkpt lat=\"%f\" lon=\"%f\">\n",
			(double)pGPS->nLatitude / 1000000, (double)pGPS->nLongitude / 1000000);
#ifdef ELEVATION
		if (gOptions.nOptions & e_OPT_Elevation)
			fprintf(pGpxFile, "\t\t\t<ele>%f</ele>\n", *pElev);
#endif
		fprintf(pGpxFile, "\t\t\t<time>%04d-%02d-%02dT%02d:%02d:%02dZ</time>\n",
			sTime2.tm_year + 1900, sTime2.tm_mon + 1, sTime2.tm_mday, sTime2.tm_hour, sTime2.tm_min, sTime2.tm_sec);
		fprintf(pGpxFile, "\t\t\t<speed>%.02f</speed>\n", (double)pCurve->nSpeed / 100);

		// Double tests, in case other info has to be included in the <extensions> tag.
		if (pCurve->nHeartRate != 0)
		{
			fprintf(pGpxFile, "\t\t\t<extensions>\n\t\t\t\t<gpxtpx:TrackPointExtension>\n");
			if (pCurve->nHeartRate != 0)
				fprintf(pGpxFile, "\t\t\t\t\t<gpxtpx:hr>%d</gpxtpx:hr>\n", pCurve->nHeartRate);
			fprintf(pGpxFile, "\t\t\t\t</gpxtpx:TrackPointExtension>\n\t\t\t</extensions>\n");
		}
		fprintf(pGpxFile, "\t\t</trkpt>\n");
	}

}

#ifdef ELEVATION

#define	GOOGLE_MAPS_API_SZ_MAX	(2048)	// Possibly 8192? (Google API limit).

#define	HTTP_REQ_PART1			"wget -qO- \"https://maps.googleapis.com/maps/api/elevation/xml?locations="
#define	HTTP_REQ_PART2			"&key="
#define	HTTP_REQ_PART3			"\""
#define	HTTP_REQ_PART4			" | awk 'tolower($0) ~ /^ *<elevation>.*<\\/elevation> *$/ { sub(/^ *<elevation>/, \"\", $0); sub(/<\\/elevation> *$/, \"\", $0); print $0; }'"

#define	REQ_LOCATIONS_MAX_LN	(GOOGLE_MAPS_API_SZ_MAX - (sizeof(HTTP_REQ_PART1) + sizeof(HTTP_REQ_PART2) + sizeof(GOOGLE_MAPS_API_KEY) + sizeof(HTTP_REQ_PART3)))	// HTTP request, GPS records max len.
#define	REQ_LOCATIONS_MAX_NB	(512)		// Max number of coordinates per request (Google API limit).

#define	HTTP_REQ_MAX_LN			(sizeof(HTTP_REQ_PART1) + REQ_LOCATIONS_MAX_LN + sizeof(HTTP_REQ_PART2) + sizeof(GOOGLE_MAPS_API_KEY) + sizeof(HTTP_REQ_PART3) + sizeof(HTTP_REQ_PART4) + 1)

// HTTP API request.
int ApiRequest(char *pLocations, double *pElev, u32 nExpectedPoints)
{
	u32	nRetrievedPoints = 0;
	char	pHttpRequest[HTTP_REQ_MAX_LN];

	snprintf(pHttpRequest, HTTP_REQ_MAX_LN, "%s%s%s%s%s%s", HTTP_REQ_PART1, pLocations, HTTP_REQ_PART2, GOOGLE_MAPS_API_KEY, HTTP_REQ_PART3, HTTP_REQ_PART4);

#define	LN_SZ	(256)
	char	pLn[LN_SZ];
	FILE	*pFlux;

	if ((pFlux = popen(pHttpRequest, "r")) != NULL)
	{
		while (fgets(pLn, LN_SZ, pFlux) != NULL)
		{
			double	nEle;
			if (sscanf(pLn, "%lf", &nEle) != 1)
			{
				fprintf(stderr, "ApiRequest(): Problem while retrieving elevations. Elevations cancelled.\n");
				gOptions.nOptions &= ~e_OPT_Elevation;
				return (1);
			}
			*pElev++ = nEle;
			nRetrievedPoints++;
		}
		pclose(pFlux);
	}

	// Last check.
	if (nRetrievedPoints != nExpectedPoints)
	{
		fprintf(stderr, "ApiRequest(): Number of retrieved points (%d) != Number of expected points (%d). Elevations cancelled.\n", nRetrievedPoints, nExpectedPoints);
		gOptions.nOptions &= ~e_OPT_Elevation;
		return (1);
	}

	return (0);
}

// Get GPS' points via Google Maps API.
// (Grouped to limit the number of http requests (day limit is 2500)).
void AltitudeGet(struct SDataRecordType *pData, u32 nDataSz, double *pElev)
{
#define	CUR_MAX_LN	(11+1+11+1)		// Single GPS record max string len. (11: 1 optinal minus sign, up to 3 digits, 1 point, up to 6 decimal places).
	char	pReqCoords[REQ_LOCATIONS_MAX_LN];

	u32	nRemainingBlks = nDataSz / sizeof(struct SDataRecordType);
	u32	nReqNo = 0;
	pReqCoords[0] = 0;
	u32	nStoIdx = 1;			// Storage index. Default pos is 1 (no zero point).

	printf("Retrieving elevation from Google Maps:\n");

	// Zero point?
	if (gOptions.nOptions & e_OPT_ZeroPt)
	{
		snprintf(pReqCoords, sizeof(pReqCoords), "%f,%f",
			(double)gOptions.sZeroGPS.nLatitude / 1000000, (double)gOptions.sZeroGPS.nLongitude / 1000000);
		nReqNo++;
		nStoIdx = 0;			// Storage index. Zero point => begin storage at pos 0.
	}

	while (nRemainingBlks)
	{
		switch (pData->nType)
		{
		case e_OMD_GPS_RECORD:
			{
				char	pCur[CUR_MAX_LN];
				struct SDataRecordGPS	*pRecGPS = (struct SDataRecordGPS *)pData;
				snprintf(pCur, sizeof(pCur), "%f,%f", (double)pRecGPS->nLatitude / 1000000, (double)pRecGPS->nLongitude / 1000000);

				if (nReqNo) strcat(pReqCoords, "|");
				nReqNo++;
				strcat(pReqCoords, pCur);

				// Current cluster complete?
				if (strlen(pReqCoords) + CUR_MAX_LN >= REQ_LOCATIONS_MAX_LN || nReqNo >= REQ_LOCATIONS_MAX_NB)
				{
					printf("Sending request for points %d to %d.\n", nStoIdx, nStoIdx + nReqNo - 1);
					if (ApiRequest(pReqCoords, pElev + nStoIdx, nReqNo)) return;
					nStoIdx += nReqNo;
					nReqNo = 0;
					pReqCoords[0] = 0;
				}
			}
			break;

		default:
			break;
		}
		pData++;
		nRemainingBlks--;
	}

	// Last cluster.
	if (nReqNo)
	{
		printf("Sending request for points %d to %d.\n", nStoIdx, nStoIdx + nReqNo - 1);
		if (ApiRequest(pReqCoords, pElev + nStoIdx, nReqNo)) return;
		nStoIdx += nReqNo;
	}

	printf("Done!\n");
}
#endif

// Main program.
int main(int argc, char **argv)
{
	u8	*pHeaderBuf = NULL;
	u32	nHeaderSz;
	u8	*pDataBuf = NULL;
	u32	nDataSz;
	double	*pElev = NULL;
	FILE	*pGpxFile = NULL;

	char	*pFilename = NULL;	// ptr on filename in args.

	char pFnHeader[256];	// Header file name.
	char pFnData[256];		// Data file name.
	char pFnGpx[256];		// GPX file name (output).
	char pFnGpxext[256];		// GPX file name as date (output).


	// Tool name.
	printf("*** OMX2GPX v%d.%02d, Geonaute OnMove 220 OMH/OMD to GPX.\n", VER_MAJOR, VER_MINOR);
	printf("*** By Clement CORDE - c1702@yahoo.com\n\n");
	// Reset options.
	memset(&gOptions, 0, sizeof(struct SOptions));
	gOptions.nOptions = e_OPT_DefaultOptions;
	// Do we have arguments?
	if (argc < 2)
	{
		Help_Display();
		exit(1);
	}
	// Check command line's arguments.
	CommandLine_Check(argc, argv, &pFilename);

	// Do we have a filename?
	if (pFilename == NULL)
	{
		printf("*** FATAL: No filename given!\n\n");
		Help_Display();
		exit(1);
	}
	// Create filenames.
	strncpy(pFnHeader, pFilename, sizeof(pFnHeader) - 4);
	strncpy(pFnData, pFilename, sizeof(pFnData) - 4);
	strncpy(pFnGpx, pFilename, sizeof(pFnGpx) - 4);
	strcat(pFnHeader, ".OMH");
	strcat(pFnData, ".OMD");
	strcat(pFnGpx, ".gpx");


	// Display options.
	if (gOptions.nOptions != e_OPT_DefaultOptions)
	{
		if (gOptions.nOptions & e_OPT_HeaderOnly) printf("=> Display header only.\n");
		if ((gOptions.nOptions & e_OPT_GPXFile) == 0) printf("=> Do not generate GPX file.\n");
		if ((gOptions.nOptions & e_OPT_GPSDataDisplay) == 0) printf("=> Do not display GPS data.\n");
#ifdef ELEVATION
		if (gOptions.nOptions & e_OPT_Elevation) printf("=> Retrieve elevation (Google Maps Elevation API).\n");
#endif
		if (gOptions.nOptions & e_OPT_ZeroPt)
		{
			printf("=> Insert zero point (lat=%f,lon=%f",
				(double)gOptions.sZeroGPS.nLatitude / 1000000, (double)gOptions.sZeroGPS.nLongitude / 1000000);
			if (gOptions.sZeroCurve.nSpeed) printf(",spd=%.02f", (double)gOptions.sZeroCurve.nSpeed / 100);
			if (gOptions.sZeroCurve.nHeartRate) printf(",hr=%d", gOptions.sZeroCurve.nHeartRate);
			printf(").\n");
		}
		printf("\n");
	}


	// *** Header ***

	// Load header (OMH).
	FileLoad(pFnHeader, &pHeaderBuf, &nHeaderSz);
	// Check: Header size ok?
	if (nHeaderSz != sizeof(struct SHeader))
	{
		fprintf(stderr, "OMH: Error! Incorrect header size.\n");
		goto _err0;
	}

	struct SHeader	*pHeader = (struct SHeader *)pHeaderBuf;

	// Check: File numbers & magic numbers ok?
	if (!(pHeader->nFileNumber0 == pHeader->nFileNumber1 && 
		pHeader->nFileNumber1 == pHeader->nFileNumber2 &&
		pHeader->nMagicNumber1 == 0xFA && pHeader->nMagicNumber2 == 0xF0))
	{
		fprintf(stderr, "OMH: Wrong header!\n");
		goto _err0;
	}

	struct tm sTime;
	time_t	nTimeStamp;		// Starting time. I decided to use a timestamp, so I wouldn't have to worry about potential days changes and stuff.

	memset(&sTime, 0, sizeof(struct tm));
	sTime.tm_min = pHeader->nMinutes;
	sTime.tm_hour = pHeader->nHours;
	sTime.tm_mday = pHeader->nDay;
	sTime.tm_mon = pHeader->nMonth - 1;		// (range: 0 to 11).
	sTime.tm_year = (2000 + pHeader->nYear) - 1900;
	sTime.tm_isdst = -1;
	if ((nTimeStamp = mktime(&sTime)) == -1)
	{
		fprintf(stderr, "mktime() error.\n");
		goto _err0;
	}

	printf("*** Session statistics ***\n");
	printf("Session: %02d/%02d/20%02d - %02d:%02d\n", pHeader->nDay, pHeader->nMonth, pHeader->nYear, pHeader->nHours, pHeader->nMinutes);
	printf("Distance: %.02f km (%d m)\n", (double)pHeader->nDistance / 1000, pHeader->nDistance);
	u32	nHours, nMins, nSecs;
	nHours = pHeader->nTime / 3600;
	nMins = (pHeader->nTime / 60) % 60;
	nSecs = pHeader->nTime % 60;
	printf("Time: %02d:%02d'%02d\" (%d s)\n", nHours, nMins, nSecs, pHeader->nTime);
	printf("Avg speed: %.01f km/h (%d0 m/h)\n", (double)pHeader->nSpeedAvg / 100, pHeader->nSpeedAvg);
	printf("Max speed: %.01f km/h (%d0 m/h)\n", (double)pHeader->nSpeedMax / 100, pHeader->nSpeedMax);

	u32	t = (pHeader->nTime * 1000) / pHeader->nDistance;
	//nHours = t / 3600;
	nMins = (t / 60) % 60;
	nSecs = t % 60;
	printf("Avg pace: %02d'%02d\"/km\n", nMins, nSecs);

	printf("KCal: %0d\n", pHeader->nKCal);
	printf("Avg heart rate: ");  if (pHeader->nHeartRateAvg) printf("%d bpm\n", pHeader->nHeartRateAvg); else printf("N/A\n");
	printf("Max heart rate: "); if (pHeader->nHeartRateMax) printf("%d bpm\n", pHeader->nHeartRateMax); else printf("N/A\n");

	printf("GPS points in OMD: %d\n", pHeader->nGPSPoints);
	printf("GPS: %s\n", (pHeader->nGPSOff == 0 ? "On" : "Off"));
	printf("\n");

	// Target.
	if (pHeader->nTargetMode)
	{
		// Display mode, min and max limits.
		char	*pTargetModeTxt[e_OMH_TARGET_Max] = { NULL, "Speed", "Heart Rate", "Pace" };
		switch (pHeader->nTargetMode)
		{
		case e_OMH_TARGET_HR:
			printf("Target mode: %s\n", pTargetModeTxt[pHeader->nTargetMode]);
			printf("Min: %d bpm - Max: %d bpm\n", pHeader->nTargetHeartRateMin, pHeader->nTargetHeartRateMax);
			break;

		case e_OMH_TARGET_SPEED:
			printf("Target mode: %s\n", pTargetModeTxt[pHeader->nTargetMode]);
			printf("Min: %.01f km/h - Max: %.01f km/h\n",
				(double)pHeader->nTargetSpeedMin / 100, (double)pHeader->nTargetSpeedMax / 100);
			break;

		case e_OMH_TARGET_PACE:
			printf("Target mode: %s\n", pTargetModeTxt[pHeader->nTargetMode]);
			{
				u32	nMinMins, nMinSecs, nMaxMins, nMaxSecs;
				nMinMins = (pHeader->nTargetSpeedMin / 60) % 60;
				nMinSecs = pHeader->nTargetSpeedMin % 60;
				nMaxMins = (pHeader->nTargetSpeedMax / 60) % 60;
				nMaxSecs = pHeader->nTargetSpeedMax % 60;
				printf("Min: %02d'%02d\"/km - Max: %02d'%02d\"/km\n", nMinMins, nMinSecs, nMaxMins, nMaxSecs);
			}
			break;

		default:
			printf("Target: Unhandled target value: %d\n", pHeader->nTargetMode);
			break;
		}
		// Time percentages.
		printf("%.02f%% <min< %.02f%% <max< %.02f%%\n",
			(100.0 * (double)pHeader->nTargetTimeBelow) / (double)pHeader->nTime,
			(100.0 * (double)pHeader->nTargetTimeIn) / (double)pHeader->nTime,
			(100.0 * (double)pHeader->nTargetTimeAbove) / (double)pHeader->nTime);
		printf("\n");
	}

	if (gOptions.nOptions & e_OPT_HeaderOnly) goto _err0;


	// *** Data ***
	if (pHeader->nGPSOff == 0)
	{

	printf("*** GPS data ***\n");

	// Load data file (OMD).
	FileLoad(pFnData, &pDataBuf, &nDataSz);
	// Check: Data size ok?
	if ( ((nDataSz / sizeof(struct SDataRecordType)) * sizeof(struct SDataRecordType)) != nDataSz)
	{
		fprintf(stderr, "OMD: Error! Incorrect data size.\n");
		goto _err0;
	}
	u32	nTotalBlks = nDataSz / sizeof(struct SDataRecordType);

	// Check: Number of GPS points in OMD ok?
	struct SDataRecordType	*pData = (struct SDataRecordType *)pDataBuf;
	u32	nRemainingBlks = nTotalBlks;
	u32	nGPSPtNo = 0;
	//
	while (nRemainingBlks)
	{
		switch (pData->nType)
		{
		case e_OMD_GPS_RECORD:
			nGPSPtNo++;
			break;

		default:
			break;
		}
		pData++;
		nRemainingBlks--;
	}
	if (nGPSPtNo != pHeader->nGPSPoints)
	{
		fprintf(stderr, "OMD: Error! Incorrect GPS points number. Found:%d, expected:%d.\n", nGPSPtNo, pHeader->nGPSPoints);
		goto _err0;
	}

	// GPX stuff.
	if (gOptions.nOptions & e_OPT_GPXFile)
	{
		// Open output file.
		// ---- CUSTOM CHANGE ----
		// GPX output filename -> YMD-distance.gpx
		sprintf(pFnGpxext, "20%02d%02d%02d_%.02fk-%.8s.gpx", pHeader->nYear, pHeader->nMonth, pHeader->nDay, (double)pHeader->nDistance / 1000, pFnGpx);
		// ---- End of custom change ----
		if ((pGpxFile = fopen(pFnGpxext, "w")) == NULL)
		{
			fprintf(stderr, "GPX: Error creating '%s'. Aborted.\n", pFnGpxext);
			goto _err0;
		}
		// GPX header.
		fprintf(pGpxFile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		fprintf(pGpxFile, "<gpx version=\"1.1\" creator=\"OMX2GPX\" xmlns=\"http://www.topografix.com/GPX/1/1\">\n");
		fprintf(pGpxFile, "\t<metadata>\n\t\t<author>\n\t\t\t<name>OMX2GPX</name>\n\t\t</author>\n\t</metadata>\n");
		fprintf(pGpxFile, "\t<trk>\n\t\t<trkseg>\n");
	}

	pData = (struct SDataRecordType *)pDataBuf;
#ifdef ELEVATION
	if (gOptions.nOptions & e_OPT_Elevation)
	{
		// Memory allocation for elevations. (+1 for possible zero point, though there should be more than enough blocks with the F2 records).
		if ((pElev = (double *)malloc((nTotalBlks + 1) * sizeof(double))) == NULL)
		{
			fprintf(stderr, "Elevation: Error allocating memory.\n");
			goto _err0;
		}
		// Clear array.
		u32	i;
		for (i = 0; i < nTotalBlks + 1; i++) *(pElev + i) = 0.0;
		// Retrieve elevations.
		AltitudeGet(pData, nDataSz, pElev);
	}
#endif

	// Expected data: F1/F1/F2 / F1/F1/F2 / ... Last sequence can be either F1/F1/F2 or F1/F2.
	struct SDataRecordGPS	*pSeqGPS0 = NULL;
	struct SDataRecordGPS	*pSeqGPS1 = NULL;
	struct SDataRecordCurve	*pSeqCurve = NULL;
	u32	nSeqGPS = 0;
	u32	nSeqCurve = 0;

	nGPSPtNo = 0;		// GPS points counter.
	// Zero point?
	if (gOptions.nOptions & e_OPT_ZeroPt)
	{
		Point_Display(nGPSPtNo, nTimeStamp, &gOptions.sZeroGPS, &gOptions.sZeroCurve, pElev, pGpxFile);
	}
	nGPSPtNo++;			// Counter start at 0 if zero point is provided, 1 if not.

	u32	nAbsRecNo = 0;	// Record number in OMD (all types), absolute, for debugging.
	nRemainingBlks = nTotalBlks;
	while (nRemainingBlks)
	{
		switch (pData->nType)
		{
		case e_OMD_GPS_RECORD:
			if (nSeqGPS == 0)
			{
				pSeqGPS0 = (struct SDataRecordGPS *)pData;
			}
			else if (nSeqGPS == 1)
			{
				pSeqGPS1 = (struct SDataRecordGPS *)pData;
			}
			else
			{
				fprintf(stderr, "OMD_GPS_RECORD: Unexpected block order! Aborted.\n");
				fprintf(stderr, "\t(debug: r=%d g=%d c=%d)\n", nAbsRecNo, nSeqGPS, nSeqCurve);
				goto _err0;
			}
			nSeqGPS++;
			break;

		case e_OMD_CURVE_RECORD:
			if (nSeqCurve == 0)
			{
				pSeqCurve = (struct SDataRecordCurve *)pData;
			}
			else
			{
				fprintf(stderr, "OMD_CURVE_RECORD: Unexpected block order! Aborted.\n");
				fprintf(stderr, "\t(debug: r=%d g=%d c=%d)\n", nAbsRecNo, nSeqGPS, nSeqCurve);
				goto _err0;
			}
			nSeqCurve++;
			break;

		default:
			fprintf(stderr, "Unknown data block! (debug: Type: %02X, record no: %d)\n", pData->nType, nAbsRecNo);
			break;
		}
		pData++;
		nAbsRecNo++;
		nRemainingBlks--;

		// Des infos à traiter ? (=> F1/F1/F2)
		if ((nSeqGPS == 2 && nSeqCurve == 1) || nRemainingBlks == 0)
		{
			if (pSeqGPS0 == NULL || pSeqCurve == NULL)
			{
				fprintf(stderr, "Shit happened...\n");
				goto _err0;
			}

			// Point 0.
			Point_Display(nGPSPtNo, nTimeStamp, pSeqGPS0, &pSeqCurve->sHalf0, pElev + nGPSPtNo, pGpxFile);
			nGPSPtNo++;
			// Point 1 (if exists).
			if (pSeqGPS1 != NULL)
			{
				Point_Display(nGPSPtNo, nTimeStamp, pSeqGPS1, &pSeqCurve->sHalf1, pElev + nGPSPtNo, pGpxFile);
				nGPSPtNo++;
			}

			// Reset ptrs and stuff.
			pSeqGPS0 = NULL;
			pSeqGPS1 = NULL;
			pSeqCurve = NULL;
			nSeqGPS = 0;
			nSeqCurve = 0;
		}

	}

	// GPX stuff.
	if (gOptions.nOptions & e_OPT_GPXFile)
	{
		// GPX's final tag.
		fprintf(pGpxFile, "\t\t</trkseg>\n\t</trk>\n</gpx>\n");
		printf("\n'%s' generated.\n", pFnGpxext);
	}

	}	// endif GPS on


	// Leaving.
_err0:
	if (pHeaderBuf != NULL) free(pHeaderBuf);
	if (pDataBuf != NULL) free(pDataBuf);
#ifdef ELEVATION
	if (pElev != NULL) free(pElev);
#endif
	if (pGpxFile != NULL) fclose(pGpxFile);
	return (0);

}



