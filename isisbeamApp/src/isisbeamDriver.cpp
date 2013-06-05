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
	double beamts1, beamts2, beamepb1;
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
				continue; // ignore ISISBEAM2 etc packets
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
			tmp = xml_parse(buffer, "BEAMT"); beamts1 = atof(tmp); free(tmp);
			tmp = xml_parse(buffer, "BEAMT2"); beamts2 = atof(tmp); free(tmp);
			tmp = xml_parse(buffer, "BEAME1"); beamepb1 = atof(tmp); free(tmp);
			lock();
			setDoubleParam(P_BeamTS1, beamts1);
			setDoubleParam(P_BeamTS2, beamts2);
			setDoubleParam(P_BeamEPB1, beamepb1);
			callParamCallbacks();
			unlock();
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

