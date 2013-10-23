#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <exception>
#include <iostream>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <iocsh.h>

#include "isisbeamDriver.h"

#include <macLib.h>
#include <epicsGuard.h>

#include <epicsExport.h>

#include "1st_nd_post.h"

static epicsThreadOnceId onceId = EPICS_THREAD_ONCE_INIT;

static void initCOM(void*)
{
//	CoInitializeEx(NULL, COINIT_MULTITHREADED);
}

static const char *driverName="isisbeamDriver";


/// Constructor for the lvDCOMDriver class.
/// Calls constructor for the asynPortDriver base class.
/// \param[in] dcomint DCOM interface pointer created by lvDCOMConfigure()
/// \param[in] portName @copydoc initArg0
isisbeamDriver::isisbeamDriver(const char *portName) 
   : asynPortDriver(portName, 
                    0, /* maxAddr */ 
                    NUM_ISISBEAM_PARAMS,
                    asynInt32Mask | asynFloat64Mask | asynOctetMask | asynDrvUserMask, /* Interface mask */
                    asynInt32Mask | asynFloat64Mask | asynOctetMask,  /* Interrupt mask */
                    ASYN_CANBLOCK, /* asynFlags.  This driver can block but it is not multi-device */
                    1, /* Autoconnect */
                    0, /* Default priority */
                    0)	/* Default stack size*/
{
	epicsThreadOnce(&onceId, initCOM, NULL);

    const char *functionName = "isisbeamDriver";
	
	createParam(P_BeamTS1String, asynParamFloat64, &P_BeamTS1);
	createParam(P_BeamTS2String, asynParamFloat64, &P_BeamTS2);
	createParam(P_BeamEPB1String, asynParamFloat64, &P_BeamEPB1);
	createParam(P_MethaneTS1String, asynParamFloat64, &P_MethaneTS1);
	createParam(P_HydrogenTS1String, asynParamFloat64, &P_HydrogenTS1);
	createParam(P_BeamSynchString, asynParamFloat64, &P_BeamSynch);
	createParam(P_FreqSynchString, asynParamFloat64, &P_FreqSynch);
	createParam(P_TotalTS1String, asynParamFloat64, &P_TotalTS1);
	createParam(P_FreqTS2String, asynParamFloat64, &P_FreqTS2);
	createParam(P_TotalTS2String, asynParamFloat64, &P_TotalTS2);
	createParam(P_DeMethaneTS2String, asynParamFloat64, &P_DeMethaneTS2);
	createParam(P_MethaneTS2String, asynParamFloat64, &P_MethaneTS2);
	createParam(P_HydrogenTS2String, asynParamFloat64, &P_HydrogenTS2);
	createParam(P_MuonKickString, asynParamFloat64, &P_MuonKick);
	createParam(P_DmodRunTS2String, asynParamFloat64, &P_DmodRunTS2);
	createParam(P_DmodRunLimTS2String, asynParamFloat64, &P_DmodRunLimTS2);
	createParam(P_BeamDmodTS2String, asynParamFloat64, &P_BeamDmodTS2);
	createParam(P_DmodAnnLowTS2String, asynParamFloat64, &P_DmodAnnLowTS2);
	createParam(P_OnTS1String, asynParamOctet, &P_OnTS1);
	createParam(P_OffTS1String, asynParamOctet, &P_OffTS1);
	createParam(P_OnTS2String, asynParamOctet, &P_OnTS2);
	createParam(P_OffTS2String, asynParamOctet, &P_OffTS2);
	
    // Create the thread for background tasks (not used at present, could be used for I/O intr scanning) 
    if (epicsThreadCreate("isisbeamPoller",
                          epicsThreadPriorityMedium,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          (EPICSTHREADFUNC)pollerThreadC, this) == 0)
    {
        printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
        return;
    }
}

asynStatus isisbeamDriver::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
//    pasynUser->timeStamp = m_timestamp;
    return asynPortDriver::readFloat64(pasynUser, value);
}

void isisbeamDriver::pollerThreadC(void* arg)
{ 
    isisbeamDriver* driver = (isisbeamDriver*)arg; 
	driver->pollerThread();
}

#define LEN_BUFFER 10024

void isisbeamDriver::pollerThread()
{
    static const char* functionName = "isisbeamPoller";
	static char buffer[LEN_BUFFER+1];
	SOCKET sd = setup_udp_socket(ND_BROADCAST_PORT1, 0);
	int n;
	char* tmp;
	struct tm* pstm;
	time_t timer;
	double beamts1, beamts2, beamepb1, mtempts1, htempts1, beamsynch, freqsynch, totalts1, freqts2, totalts2, demethanets2, methanets2, hydrogents2, dmodrunts2, dmodrunlimts2, beamdmodts2, muonkick, dmodannlowts2;
	char *onts1, *offts1, *onts2, *offts2;
	static char time_buffer[128];
	while(true)
	{
		n = receive_data_udp(sd, buffer, LEN_BUFFER);
		if (n > 0)
		{
			buffer[n] = '\0';
			buffer[LEN_BUFFER] = '\0';
			tmp = xml_parse(buffer, "ISISBEAM");
			if (tmp == NULL)
			{
				tmp = xml_parse(buffer, "ISISBEAM2");
				if (tmp == NULL)
				{
					continue; // ignore anything other than ISISBEAM and ISISBEAM2 packets for moment
				}
				free(tmp);
				tmp = xml_parse(buffer, "TIME");
				if (tmp == NULL)
				{	
					continue;
				}
				timer = atol(tmp);
				pstm = localtime(&timer);
				free(tmp);
				strftime(time_buffer, sizeof(time_buffer), "%d-%b-%Y %H:%M:%S", pstm);
				tmp = xml_parse(buffer, "DMOD_RUNTIME"); dmodrunts2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "DMOD_RUNTIME_LIM"); dmodrunlimts2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "DMOD_UABEAM"); beamdmodts2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "DMOD_ANNLOW1"); dmodannlowts2 = atof(tmp); free(tmp);
				lock();
				setDoubleParam(P_DmodRunTS2, dmodrunts2);
				setDoubleParam(P_DmodRunLimTS2, dmodrunlimts2);
				setDoubleParam(P_BeamDmodTS2, beamdmodts2);
				setDoubleParam(P_DmodAnnLowTS2, dmodannlowts2);
				callParamCallbacks();
				unlock();
			}
			else
			{
				free(tmp);
				tmp = xml_parse(buffer, "TIME");
				if (tmp == NULL)
				{	
					continue;
				}
				timer = atol(tmp);
				pstm = localtime(&timer);
				free(tmp);
				strftime(time_buffer, sizeof(time_buffer), "%d-%b-%Y %H:%M:%S", pstm);
				tmp = xml_parse(buffer, "BEAMT"); beamts1 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "BEAMT2"); beamts2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "BEAME1"); beamepb1 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "MTEMP"); mtempts1 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "HTEMP"); htempts1 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "BEAMS"); beamsynch = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "REPR"); freqsynch = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "TS1_TOTAL"); totalts1 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "REPR2"); freqts2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "TS2_TOTAL"); totalts2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "T2MTEMP1"); demethanets2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "T2MTEMP2"); methanets2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "T2HTEMP1"); hydrogents2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "MUONKICKER"); muonkick = atof(tmp); free(tmp);
				onts1 = xml_parse(buffer, "TS1ON"); 
				offts1 = xml_parse(buffer, "TS1OFF");
				onts2 = xml_parse(buffer, "TS2ON");
				offts2 = xml_parse(buffer, "TS2OFF");
				lock();
				epicsTimeFromTime_t(&m_timestamp, timer);
				setDoubleParam(P_BeamTS1, beamts1);
				setDoubleParam(P_BeamTS2, beamts2);
				setDoubleParam(P_BeamEPB1, beamepb1);
				setDoubleParam(P_MethaneTS1, mtempts1);
				setDoubleParam(P_HydrogenTS1, htempts1);
				setDoubleParam(P_BeamSynch, beamsynch);
				setDoubleParam(P_FreqSynch, freqsynch);
				setDoubleParam(P_TotalTS1, totalts1);
				setDoubleParam(P_FreqTS2, freqts2);
				setDoubleParam(P_TotalTS2, totalts2);
				setDoubleParam(P_DeMethaneTS2, demethanets2);
				setDoubleParam(P_MethaneTS2, methanets2);
				setDoubleParam(P_HydrogenTS2, hydrogents2);
				setDoubleParam(P_MuonKick, muonkick);
				setStringParam(P_OnTS1, onts1);
				setStringParam(P_OffTS1, offts1);
				setStringParam(P_OnTS2, onts2);
				setStringParam(P_OffTS2, offts2);
				free(onts1);
				free(offts1);
				free(onts2);
				free(offts2);
				callParamCallbacks();
				unlock();
			}
		}
		else
		{
			epicsThreadSleep(3.0);
		}
	}
}	

extern "C" {

/// EPICS iocsh callable function to call constructor of lvDCOMInterface().
/// \param[in] portName @copydoc initArg0
int isisbeamConfigure(const char *portName)
{
	try
	{
		new isisbeamDriver(portName);
		return(asynSuccess);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "isisbeamDriver failed: " << ex.what() << std::endl;
		return(asynError);
	}
}

// EPICS iocsh shell commands 

static const iocshArg initArg0 = { "portName", iocshArgString};			///< The name of the asyn driver port we will create

static const iocshArg * const initArgs[] = { &initArg0 };

static const iocshFuncDef initFuncDef = {"isisbeamConfigure", sizeof(initArgs) / sizeof(iocshArg*), initArgs};

static void initCallFunc(const iocshArgBuf *args)
{
    isisbeamConfigure(args[0].sval);
}

static void isisbeamRegister(void)
{
    iocshRegister(&initFuncDef, initCallFunc);
}

epicsExportRegistrar(isisbeamRegister);

}

