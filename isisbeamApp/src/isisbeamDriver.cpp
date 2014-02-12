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
	int n,k,v,l;
	char* tmp;
	struct tm* pstm;
	time_t timer;
	double beamts1, beamts2, beamepb1, mtempts1, htempts1, beamsynch, freqsynch, totalts1, freqts2, totalts2, demethanets2, methanets2, hydrogents2, dmodrunts2, dmodrunlimts2, beamdmodts2, muonkick, dmodannlowts2;
	char *onts1, *offts1, *onts2, *offts2, three_chars[3], one_char[1], *e1, *e2, *e3, *e4, *e5, *e6, *e7, *e8, *e9, *w1, *w2, *w3, *w4, *w5, *w6, *w7, *w8, *w9, *n1, *n2, *n3, *n4, *n5, *n6, *n7, *n8, *n9, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
	static char time_buffer[128];
	int portvals[9];
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
				for (k=0;k<9;k++)
				{
					memcpy(one_char,tmp+k,1);
					v = atoi((const char *)one_char);
					portvals[k]=v;
				}
				e1 = ts2_vat_status(portvals[0]);
				e2 = ts2_vat_status(portvals[1]);
				e3 = ts2_vat_status(portvals[2]);
				e4 = ts2_vat_status(portvals[3]);
				e5 = ts2_vat_status(portvals[4]);
				e6 = ts2_vat_status(portvals[5]);
				e7 = ts2_vat_status(portvals[6]);
				e8 = ts2_vat_status(portvals[7]);
				e9 = ts2_vat_status(portvals[8]);
				free(tmp);
				tmp = xml_parse(buffer, "VATW");
				for (k=0;k<9;k++)
				{
					memcpy(one_char,tmp+k,1);
					v = atoi((const char *)one_char);
					portvals[k]=v;
				}
				w1 = ts2_vat_status(portvals[0]);
				w2 = ts2_vat_status(portvals[1]);
				w3 = ts2_vat_status(portvals[2]);
				w4 = ts2_vat_status(portvals[3]);
				w5 = ts2_vat_status(portvals[4]);
				w6 = ts2_vat_status(portvals[5]);
				w7 = ts2_vat_status(portvals[6]);
				w8 = ts2_vat_status(portvals[7]);
				w9 = ts2_vat_status(portvals[8]);
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
				l=0;
				for (k=0;k<9;k++)
				{
					memcpy(one_char,tmp+k+l,1);
					v = atoi((const char *)one_char);
					if (v != 0)
					{
						memcpy(three_chars,tmp+k+l,3);
						l=l+2;
						v = atoi((const char *)three_chars);
					};
					portvals[k]=v;
				}
				e1 = ts2_shutter_status(portvals[0]);
				e2 = ts2_shutter_status(portvals[1]);
				e3 = ts2_shutter_status(portvals[2]);
				e4 = ts2_shutter_status(portvals[3]);
				e5 = ts2_shutter_status(portvals[4]);
				e6 = ts2_shutter_status(portvals[5]);
				e7 = ts2_shutter_status(portvals[6]);
				e8 = ts2_shutter_status(portvals[7]);
				e9 = ts2_shutter_status(portvals[8]);
				free(tmp);
				tmp = xml_parse(buffer, "SHUTW");
				l=0;
				for (k=0;k<9;k++)
				{
					memcpy(one_char,tmp+k+l,1);
					v = atoi((const char *)one_char);
					if (v != 0)
					{
						memcpy(three_chars,tmp+k+l,3);
						l=l+2;
						v = atoi((const char *)three_chars);
					};
					portvals[k]=v;
				}
				w1 = ts2_shutter_status(portvals[0]);
				w2 = ts2_shutter_status(portvals[1]);
				w3 = ts2_shutter_status(portvals[2]);
				w4 = ts2_shutter_status(portvals[3]);
				w5 = ts2_shutter_status(portvals[4]);
				w6 = ts2_shutter_status(portvals[5]);
				w7 = ts2_shutter_status(portvals[6]);
				w8 = ts2_shutter_status(portvals[7]);
				w9 = ts2_shutter_status(portvals[8]);
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

