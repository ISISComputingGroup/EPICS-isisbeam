#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <exception>
#include <iostream>
#include <map>
#include <string>

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

/// @file isisbeamDriver.cpp Driver for ISISBEAM

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

static void initCOM(void*)
{
//	CoInitializeEx(NULL, COINIT_MULTITHREADED);
}

static const char *driverName="isisbeamDriver";

#define MAX_ASYN_BL_PARAMS	300  ///< needs to be large enough to cover beamline parameters created dynamically in  isisbeamDriver()

/// Constructor for the isisbeamDriver class.
/// Calls constructor for the asynPortDriver base class.
/// \param[in] portName @copydoc initArg0
isisbeamDriver::isisbeamDriver(const char *portName) 
   : asynPortDriver(portName, 
                    0, /* maxAddr */ 
                    MAX_ASYN_BL_PARAMS,
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
	createParam(P_InstTS1String, asynParamOctet, &P_InstTS1);
	createParam(P_InstTS2String, asynParamOctet, &P_InstTS2);
	createParam(P_OsirisCryomagString, asynParamInt32, &P_OsirisCryomag);
	createParam(P_UpdateTimeString, asynParamOctet, &P_UpdateTime);
	createParam(P_UpdateTimeTString, asynParamInt32, &P_UpdateTimeT);
	// create beamline specific parameters (shutter status, shutter mode, VAT valve)
	// assigns a default "unavailable" value to each as not all are available on every beamline
	// make sure MAX_ASYN_BL_PARAMS is big enough
	const char* axes = "NSWE";
	char buff[32];
	int id;
	for(int i=1; i<=9; ++i)
	{
	    for(int j=0; j<4; ++j)
		{
			sprintf(buff, "SHUT_%c%d", axes[j], i);
			createParam(buff, asynParamOctet, &id);
			setStringParam(id, "N/A");
			m_blparams[buff] = id;
			sprintf(buff, "VAT_%c%d", axes[j], i);
			createParam(buff, asynParamOctet, &id);
			setStringParam(id, "N/A");
			m_blparams[buff] = id;
			sprintf(buff, "SMODE_%c%d", axes[j], i);
			createParam(buff, asynParamOctet, &id);
			setStringParam(id, "N/A");
			m_blparams[buff] = id;
		}
	}
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
				strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%S+0100", pstm);
				tmp = xml_parse(buffer, "DMOD_RUNTIME"); dmodrunts2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "DMOD_RUNTIME_LIM"); dmodrunlimts2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "DMOD_UABEAM"); beamdmodts2 = atof(tmp); free(tmp);
				tmp = xml_parse(buffer, "DMOD_ANNLOW1"); dmodannlowts2 = atof(tmp); free(tmp);
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
				lock();
				setDoubleParam(P_DmodRunTS2, dmodrunts2);
				setDoubleParam(P_DmodRunLimTS2, dmodrunlimts2);
				setDoubleParam(P_BeamDmodTS2, beamdmodts2);
				setDoubleParam(P_DmodAnnLowTS2, dmodannlowts2);
				setStringParam(m_blparams["VAT_E1"], e1);
				setStringParam(m_blparams["VAT_E2"], e2);
				setStringParam(m_blparams["VAT_E3"], e3);
				setStringParam(m_blparams["VAT_E4"], e4);
				setStringParam(m_blparams["VAT_E5"], e5);
				setStringParam(m_blparams["VAT_E6"], e6);
				setStringParam(m_blparams["VAT_E7"], e7);
				setStringParam(m_blparams["VAT_E8"], e8);
				setStringParam(m_blparams["VAT_E9"], e9);
				setStringParam(m_blparams["VAT_W1"], w1);
				setStringParam(m_blparams["VAT_W2"], w2);
				setStringParam(m_blparams["VAT_W3"], w3);
				setStringParam(m_blparams["VAT_W4"], w4);
				setStringParam(m_blparams["VAT_W5"], w5);
				setStringParam(m_blparams["VAT_W6"], w6);
				setStringParam(m_blparams["VAT_W7"], w7);
				setStringParam(m_blparams["VAT_W8"], w8);
				setStringParam(m_blparams["VAT_W9"], w9);
				setStringParam(P_UpdateTime, time_buffer);
				setIntegerParam(P_UpdateTimeT, timer);
				setStringParam(P_InstTS1, "ALF,ARGUS,CHRONUS,CRISP,EMMA,EMU,ENGINX,EVS,GEM,HIFI,HRPD,INES,IRIS,LOQ,MAPS,MARI,MERLIN,MUSR,OSIRIS,PEARL,POLARIS,"
				                          "PRISMA,ROTAX,SANDALS,SURF,SXD,TOSCA,VESUVIO");

				setStringParam(P_InstTS2, "CHIPIR,IMAT,INTER,LARMOR,LET,NIMROD,OFFSPEC,POLREF,SANS2D,WISH,ZOOM");
				setIntegerParam(P_OsirisCryomag, 0);
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
				strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%S+0100", pstm);
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
				setStringParam(m_blparams["SHUT_N1"], n1);
				setStringParam(m_blparams["SHUT_N2"], n2);
				setStringParam(m_blparams["SHUT_N3"], n3);
				setStringParam(m_blparams["SHUT_N4"], n4);
				setStringParam(m_blparams["SHUT_N5"], n5);
				setStringParam(m_blparams["SHUT_N6"], n6);
				setStringParam(m_blparams["SHUT_N7"], n7);
				setStringParam(m_blparams["SHUT_N8"], n8);
				setStringParam(m_blparams["SHUT_N9"], n9);
				setStringParam(m_blparams["SHUT_S1"], s1);
				setStringParam(m_blparams["SHUT_S2"], s2);
				setStringParam(m_blparams["SHUT_S3"], s3);
				setStringParam(m_blparams["SHUT_S4"], s4);
				setStringParam(m_blparams["SHUT_S5"], s5);
				setStringParam(m_blparams["SHUT_S6"], s6);
				setStringParam(m_blparams["SHUT_S7"], s7);
				setStringParam(m_blparams["SHUT_S8"], s8);
				setStringParam(m_blparams["SHUT_S9"], s9);
				setStringParam(m_blparams["SHUT_E1"], e1);
				setStringParam(m_blparams["SHUT_E2"], e2);
				setStringParam(m_blparams["SHUT_E3"], e3);
				setStringParam(m_blparams["SHUT_E4"], e4);
				setStringParam(m_blparams["SHUT_E5"], e5);
				setStringParam(m_blparams["SHUT_E6"], e6);
				setStringParam(m_blparams["SHUT_E7"], e7);
				setStringParam(m_blparams["SHUT_E8"], e8);
				setStringParam(m_blparams["SHUT_E9"], e9);
				setStringParam(m_blparams["SHUT_W1"], w1);
				setStringParam(m_blparams["SHUT_W2"], w2);
				setStringParam(m_blparams["SHUT_W3"], w3);
				setStringParam(m_blparams["SHUT_W4"], w4);
				setStringParam(m_blparams["SHUT_W5"], w5);
				setStringParam(m_blparams["SHUT_W6"], w6);
				setStringParam(m_blparams["SHUT_W7"], w7);
				setStringParam(m_blparams["SHUT_W8"], w8);
				setStringParam(m_blparams["SHUT_W9"], w9);
				setStringParam(m_blparams["SMODE_E1"], em1);
				setStringParam(m_blparams["SMODE_E2"], em2);
				setStringParam(m_blparams["SMODE_E3"], em3);
				setStringParam(m_blparams["SMODE_E4"], em4);
				setStringParam(m_blparams["SMODE_E5"], em5);
				setStringParam(m_blparams["SMODE_E6"], em6);
				setStringParam(m_blparams["SMODE_E7"], em7);
				setStringParam(m_blparams["SMODE_E8"], em8);
				setStringParam(m_blparams["SMODE_E9"], em9);
				setStringParam(m_blparams["SMODE_W1"], wm1);
				setStringParam(m_blparams["SMODE_W2"], wm2);
				setStringParam(m_blparams["SMODE_W3"], wm3);
				setStringParam(m_blparams["SMODE_W4"], wm4);
				setStringParam(m_blparams["SMODE_W5"], wm5);
				setStringParam(m_blparams["SMODE_W6"], wm6);
				setStringParam(m_blparams["SMODE_W7"], wm7);
				setStringParam(m_blparams["SMODE_W8"], wm8);
				setStringParam(m_blparams["SMODE_W9"], wm9);
				setStringParam(P_UpdateTime, time_buffer);
				setIntegerParam(P_UpdateTimeT, timer);
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

/// EPICS iocsh callable function to call constructor of isisbeamDriver().
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

