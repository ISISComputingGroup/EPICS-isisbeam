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


//Additions for Story #266, KVLB
//Copied from relay_server.c to format the shutter status info
static char* ts2_shutter_status(int stat)
{
    switch( stat & 0xff )
    {
    	case 0:
    	    /* return "DEACTIVE"; */
    	    return "DEACT";
    	    break;
    	case 1:
    	    return "OPEN";
    	    break;
    	case 2:
    	    return "CLOSED";
    	    break;
    	case 3:
    	    return "MOVING";
    	    break;
    	case 4:
    	    return "FAULT";
    	    break;
    	default:
    	    return "INVALID";
    	    break;
    }	    
    return "INVALID"; /*NOTREACHED*/
}

static char* ts2_shutter_mode(int stat)
{
    switch( (stat >> 8) & 0xff )
    {
                case 0:
                    return "DEACT";; // De-Activated
                    break;
                case 1:
                    return "MANUAL"; // Manual (Control Room HMI)
                    break;
                case 2:
                    return "REM-MANUAL"; // Remote Manual (used for shutter scanning)
                    break;
                case 3:
                    return "STC"; // Shield Top Control (used for maintenance)
                    break;
                case 4:
                    return "OPENING-BLR"; // Opening (beam line request)
                    break;
                case 5:
                    return "CLOSING-BLR"; // Closing (beam line request)
                    break;
                case 6:
                    return "POS-CORR"; // Position Correction
                    break;
                case 7:
                    return "OPENING-CR"; // Opening (control Room HMI request)
                    break;
                case 8:
                    return "CLOSING-CR"; // Closing (control Room HMI request)
                    break;
                case 9:
                    return "EMG-CLOSE"; // Emergency Close (control Room HMI request)
                    break;
                case 98:
                    return "BLC"; // Beam Line Control (no auto correction)
                    break;
                case 99:
                    return "BLC"; // Beam Line Control (with auto correction)
                    break;
                default:
                    return "INVALID";
                    break;

    }	    
    return "INVALID"; /*NOTREACHED*/
}

static char* ts2_vat_status(int stat)
{
    switch(stat)
    {
    	case 0:
    	    /* return "DEACTIVE"; */
    	    return "DEACT";
    	    break;
    	case 1:
    	    return "OPEN";
    	    break;
    	case 2:
    	    return "MOVING";
    	    break;
    	case 3:
    	    return "CLOSED";
    	    break;
    	case 4:
    	    return "FAULT";
    	    break;
    	default:
    	    return "INVALID";
    	    break;
    }	    
    return "INVALID"; /*NOTREACHED*/
}

static char* ts1_shutter_status(int stat, int beamline)
{
    switch( stat & (1 << (beamline - 1)) )
    {
    	case 0:
    	    return "CLOSED";
    	    break;
    	default: 
    	    return "OPEN";
    	    break;
    }	    
    return "INVALID"; /*NOTREACHED*/
}
//End Additions for Story #266

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
	//Additional Params for story #266
	createParam(P_N1ShutString, asynParamOctet, &P_N1Shut);
	createParam(P_N2ShutString, asynParamOctet, &P_N2Shut);
	createParam(P_N3ShutString, asynParamOctet, &P_N3Shut);
	createParam(P_N4ShutString, asynParamOctet, &P_N4Shut);
	createParam(P_N5ShutString, asynParamOctet, &P_N5Shut);
	createParam(P_N6ShutString, asynParamOctet, &P_N6Shut);
	createParam(P_N7ShutString, asynParamOctet, &P_N7Shut);
	createParam(P_N8ShutString, asynParamOctet, &P_N8Shut);
	createParam(P_N9ShutString, asynParamOctet, &P_N9Shut);
	createParam(P_S1ShutString, asynParamOctet, &P_S1Shut);
	createParam(P_S2ShutString, asynParamOctet, &P_S2Shut);
	createParam(P_S3ShutString, asynParamOctet, &P_S3Shut);
	createParam(P_S4ShutString, asynParamOctet, &P_S4Shut);
	createParam(P_S5ShutString, asynParamOctet, &P_S5Shut);
	createParam(P_S6ShutString, asynParamOctet, &P_S6Shut);
	createParam(P_S7ShutString, asynParamOctet, &P_S7Shut);
	createParam(P_S8ShutString, asynParamOctet, &P_S8Shut);
	createParam(P_S9ShutString, asynParamOctet, &P_S9Shut);
	createParam(P_E1ShutString, asynParamOctet, &P_E1Shut);
	createParam(P_E2ShutString, asynParamOctet, &P_E2Shut);
	createParam(P_E3ShutString, asynParamOctet, &P_E3Shut);
	createParam(P_E4ShutString, asynParamOctet, &P_E4Shut);
	createParam(P_E5ShutString, asynParamOctet, &P_E5Shut);
	createParam(P_E6ShutString, asynParamOctet, &P_E6Shut);
	createParam(P_E7ShutString, asynParamOctet, &P_E7Shut);
	createParam(P_E8ShutString, asynParamOctet, &P_E8Shut);
	createParam(P_E9ShutString, asynParamOctet, &P_E9Shut);
	createParam(P_W1ShutString, asynParamOctet, &P_W1Shut);
	createParam(P_W2ShutString, asynParamOctet, &P_W2Shut);
	createParam(P_W3ShutString, asynParamOctet, &P_W3Shut);
	createParam(P_W4ShutString, asynParamOctet, &P_W4Shut);
	createParam(P_W5ShutString, asynParamOctet, &P_W5Shut);
	createParam(P_W6ShutString, asynParamOctet, &P_W6Shut);
	createParam(P_W7ShutString, asynParamOctet, &P_W7Shut);
	createParam(P_W8ShutString, asynParamOctet, &P_W8Shut);
	createParam(P_W9ShutString, asynParamOctet, &P_W9Shut);
	createParam(P_E1VATString, asynParamOctet, &P_E1VAT);
	createParam(P_E2VATString, asynParamOctet, &P_E2VAT);
	createParam(P_E3VATString, asynParamOctet, &P_E3VAT);
	createParam(P_E4VATString, asynParamOctet, &P_E4VAT);
	createParam(P_E5VATString, asynParamOctet, &P_E5VAT);
	createParam(P_E6VATString, asynParamOctet, &P_E6VAT);
	createParam(P_E7VATString, asynParamOctet, &P_E7VAT);
	createParam(P_E8VATString, asynParamOctet, &P_E8VAT);
	createParam(P_E9VATString, asynParamOctet, &P_E9VAT);
	createParam(P_W1VATString, asynParamOctet, &P_W1VAT);
	createParam(P_W2VATString, asynParamOctet, &P_W2VAT);
	createParam(P_W3VATString, asynParamOctet, &P_W3VAT);
	createParam(P_W4VATString, asynParamOctet, &P_W4VAT);
	createParam(P_W5VATString, asynParamOctet, &P_W5VAT);
	createParam(P_W6VATString, asynParamOctet, &P_W6VAT);
	createParam(P_W7VATString, asynParamOctet, &P_W7VAT);
	createParam(P_W8VATString, asynParamOctet, &P_W8VAT);
	createParam(P_W9VATString, asynParamOctet, &P_W9VAT);
	createParam(P_E1SModeString, asynParamOctet, &P_E1SMode);
	createParam(P_E2SModeString, asynParamOctet, &P_E2SMode);
	createParam(P_E3SModeString, asynParamOctet, &P_E3SMode);
	createParam(P_E4SModeString, asynParamOctet, &P_E4SMode);
	createParam(P_E5SModeString, asynParamOctet, &P_E5SMode);
	createParam(P_E6SModeString, asynParamOctet, &P_E6SMode);
	createParam(P_E7SModeString, asynParamOctet, &P_E7SMode);
	createParam(P_E8SModeString, asynParamOctet, &P_E8SMode);
	createParam(P_E9SModeString, asynParamOctet, &P_E9SMode);
	createParam(P_W1SModeString, asynParamOctet, &P_W1SMode);
	createParam(P_W2SModeString, asynParamOctet, &P_W2SMode);
	createParam(P_W3SModeString, asynParamOctet, &P_W3SMode);
	createParam(P_W4SModeString, asynParamOctet, &P_W4SMode);
	createParam(P_W5SModeString, asynParamOctet, &P_W5SMode);
	createParam(P_W6SModeString, asynParamOctet, &P_W6SMode);
	createParam(P_W7SModeString, asynParamOctet, &P_W7SMode);
	createParam(P_W8SModeString, asynParamOctet, &P_W8SMode);
	createParam(P_W9SModeString, asynParamOctet, &P_W9SMode);
	//End Additional Params for story #266
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
	int n,v;
	char* tmp;
	struct tm* pstm;
	time_t timer;
	double beamts1, beamts2, beamepb1, mtempts1, htempts1, beamsynch, freqsynch, totalts1, freqts2, totalts2, demethanets2, methanets2, hydrogents2, dmodrunts2, dmodrunlimts2, beamdmodts2, muonkick, dmodannlowts2;
	char *onts1, *offts1, *onts2, *offts2, *e1, *e2, *e3, *e4, *e5, *e6, *e7, *e8, *e9, *w1, *w2, *w3, *w4, *w5, *w6, *w7, *w8, *w9, *n1, *n2, *n3, *n4, *n5, *n6, *n7, *n8, *n9, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9, *em1, *em2, *em3, *em4, *em5, *em6, *em7, *em8, *em9, *wm1, *wm2, *wm3, *wm4, *wm5, *wm6, *wm7, *wm8, *wm9;
	static char time_buffer[128];
	int  c1, c2, c3, c4, c5, c6, c7, c8, c9;
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
				//Addtions story #266
				tmp = xml_parse(buffer, "VATE");
				sscanf(tmp, "%lu %lu %lu %lu %lu %lu %lu %lu %lu", &c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9);
				e1 = ts2_vat_status(c1);
				e2 = ts2_vat_status(c2);
				e3 = ts2_vat_status(c3);
				e4 = ts2_vat_status(c4);
				e5 = ts2_vat_status(c5);
				e6 = ts2_vat_status(c6);
				e7 = ts2_vat_status(c7);
				e8 = ts2_vat_status(c8);
				e9 = ts2_vat_status(c9);
				free(tmp);
				tmp = xml_parse(buffer, "VATW");
				sscanf(tmp, "%lu %lu %lu %lu %lu %lu %lu %lu %lu", &c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9);
				w1 = ts2_vat_status(c1);
				w2 = ts2_vat_status(c2);
				w3 = ts2_vat_status(c3);
				w4 = ts2_vat_status(c4);
				w5 = ts2_vat_status(c5);
				w6 = ts2_vat_status(c6);
				w7 = ts2_vat_status(c7);
				w8 = ts2_vat_status(c8);
				w9 = ts2_vat_status(c9);
				free(tmp);
				//End Addtions story #266
				lock();
				setDoubleParam(P_DmodRunTS2, dmodrunts2);
				setDoubleParam(P_DmodRunLimTS2, dmodrunlimts2);
				setDoubleParam(P_BeamDmodTS2, beamdmodts2);
				setDoubleParam(P_DmodAnnLowTS2, dmodannlowts2);
				//Addtions story #266
				setStringParam(P_E1VAT, e1);
				setStringParam(P_E2VAT, e2);
				setStringParam(P_E3VAT, e3);
				setStringParam(P_E4VAT, e4);
				setStringParam(P_E5VAT, e5);
				setStringParam(P_E6VAT, e6);
				setStringParam(P_E7VAT, e7);
				setStringParam(P_E8VAT, e8);
				setStringParam(P_E9VAT, e9);
				setStringParam(P_W1VAT, w1);
				setStringParam(P_W2VAT, w2);
				setStringParam(P_W3VAT, w3);
				setStringParam(P_W4VAT, w4);
				setStringParam(P_W5VAT, w5);
				setStringParam(P_W6VAT, w6);
				setStringParam(P_W7VAT, w7);
				setStringParam(P_W8VAT, w8);
				setStringParam(P_W9VAT, w9);
				//End Addtions story #266
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
				//Addtions story #266
				tmp = xml_parse(buffer, "SHUTE");
				sscanf(tmp, "%lu %lu %lu %lu %lu %lu %lu %lu %lu", &c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9);
				e1 = ts2_shutter_status(c1);
				e2 = ts2_shutter_status(c2);
				e3 = ts2_shutter_status(c3);
				e4 = ts2_shutter_status(c4);
				e5 = ts2_shutter_status(c5);
				e6 = ts2_shutter_status(c6);
				e7 = ts2_shutter_status(c7);
				e8 = ts2_shutter_status(c8);
				e9 = ts2_shutter_status(c9);
				em1 = ts2_shutter_mode(c1);
				em2 = ts2_shutter_mode(c2);
				em3 = ts2_shutter_mode(c3);
				em4 = ts2_shutter_mode(c4);
				em5 = ts2_shutter_mode(c5);
				em6 = ts2_shutter_mode(c6);
				em7 = ts2_shutter_mode(c7);
				em8 = ts2_shutter_mode(c8);
				em9 = ts2_shutter_mode(c9);
				free(tmp);
				tmp = xml_parse(buffer, "SHUTW");
				sscanf(tmp, "%lu %lu %lu %lu %lu %lu %lu %lu %lu", &c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9);
				w1 = ts2_shutter_status(c1);
				w2 = ts2_shutter_status(c2);
				w3 = ts2_shutter_status(c3);
				w4 = ts2_shutter_status(c4);
				w5 = ts2_shutter_status(c5);
				w6 = ts2_shutter_status(c6);
				w7 = ts2_shutter_status(c7);
				w8 = ts2_shutter_status(c8);
				w9 = ts2_shutter_status(c9);
				wm1 = ts2_shutter_mode(c1);
				wm2 = ts2_shutter_mode(c2);
				wm3 = ts2_shutter_mode(c3);
				wm4 = ts2_shutter_mode(c4);
				wm5 = ts2_shutter_mode(c5);
				wm6 = ts2_shutter_mode(c6);
				wm7 = ts2_shutter_mode(c7);
				wm8 = ts2_shutter_mode(c8);
				wm9 = ts2_shutter_mode(c9);
				free(tmp);
				tmp = xml_parse(buffer, "SHUTN");
				v = atoi((const char *)tmp);
				n1 = ts1_shutter_status(v, 1);
				n2 = ts1_shutter_status(v, 2);
				n3 = ts1_shutter_status(v, 3);
				n4 = ts1_shutter_status(v, 4);
				n5 = ts1_shutter_status(v, 5);
				n6 = ts1_shutter_status(v, 6);
				n7 = ts1_shutter_status(v, 7);
				n8 = ts1_shutter_status(v, 8);
				n9 = ts1_shutter_status(v, 9);				
				free(tmp);
				tmp = xml_parse(buffer, "SHUTS");
				v = atoi((const char *)tmp);
				s1 = ts1_shutter_status(v, 1);
				s2 = ts1_shutter_status(v, 2);
				s3 = ts1_shutter_status(v, 3);
				s4 = ts1_shutter_status(v, 4);
				s5 = ts1_shutter_status(v, 5);
				s6 = ts1_shutter_status(v, 6);
				s7 = ts1_shutter_status(v, 7);
				s8 = ts1_shutter_status(v, 8);
				s9 = ts1_shutter_status(v, 9);				
				free(tmp);
				//End Addtions story #266
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
				//Addtions story #266
				setStringParam(P_E1Shut, e1);
				setStringParam(P_E2Shut, e2);
				setStringParam(P_E3Shut, e3);
				setStringParam(P_E4Shut, e4);
				setStringParam(P_E5Shut, e5);
				setStringParam(P_E6Shut, e6);
				setStringParam(P_E7Shut, e7);
				setStringParam(P_E8Shut, e8);
				setStringParam(P_E9Shut, e9);
				setStringParam(P_W1Shut, w1);
				setStringParam(P_W2Shut, w2);
				setStringParam(P_W3Shut, w3);
				setStringParam(P_W4Shut, w4);
				setStringParam(P_W5Shut, w5);
				setStringParam(P_W6Shut, w6);
				setStringParam(P_W7Shut, w7);
				setStringParam(P_W8Shut, w8);
				setStringParam(P_W9Shut, w9);
				setStringParam(P_E1SMode, em1);
				setStringParam(P_E2SMode, em2);
				setStringParam(P_E3SMode, em3);
				setStringParam(P_E4SMode, em4);
				setStringParam(P_E5SMode, em5);
				setStringParam(P_E6SMode, em6);
				setStringParam(P_E7SMode, em7);
				setStringParam(P_E8SMode, em8);
				setStringParam(P_E9SMode, em9);
				setStringParam(P_W1SMode, wm1);
				setStringParam(P_W2SMode, wm2);
				setStringParam(P_W3SMode, wm3);
				setStringParam(P_W4SMode, wm4);
				setStringParam(P_W5SMode, wm5);
				setStringParam(P_W6SMode, wm6);
				setStringParam(P_W7SMode, wm7);
				setStringParam(P_W8SMode, wm8);
				setStringParam(P_W9SMode, wm9);
				setStringParam(P_N1Shut, n1);
				setStringParam(P_N2Shut, n2);
				setStringParam(P_N3Shut, n3);
				setStringParam(P_N4Shut, n4);
				setStringParam(P_N5Shut, n5);
				setStringParam(P_N6Shut, n6);
				setStringParam(P_N7Shut, n7);
				setStringParam(P_N8Shut, n8);
				setStringParam(P_N9Shut, n9);
				setStringParam(P_S1Shut, s1);
				setStringParam(P_S2Shut, s2);
				setStringParam(P_S3Shut, s3);
				setStringParam(P_S4Shut, s4);
				setStringParam(P_S5Shut, s5);
				setStringParam(P_S6Shut, s6);
				setStringParam(P_S7Shut, s7);
				setStringParam(P_S8Shut, s8);
				setStringParam(P_S9Shut, s9);
				//End Addtions story #266
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

