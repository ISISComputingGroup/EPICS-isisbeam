#ifndef ISISBEAMDRIVER_H
#define ISISBEAMDRIVER_H
 
#include "asynPortDriver.h"

template <typename T>
struct asynItems
{
    enum ParamType { value = -1 };
};

template <>
struct asynItems<double>
{
    enum ParamType { value = asynParamFloat64 };
};

template <>
struct asynItems<int>
{
    enum ParamType { value = asynParamInt32 };
};


class isisbeamDriver : public asynPortDriver 
{
public:
    isisbeamDriver(const char* portName);
	static void pollerThreadC(void* arg);
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
                 
private:
    int P_BeamTS1; // double
    int P_BeamTS2; // double
	int P_MethaneTS1; // double
	int P_HydrogenTS1; // double
	int P_BeamSynch; // double
	int P_FreqSynch; // double
	int P_TotalTS1; // double
	int P_FreqTS2; // double
	int P_TotalTS2; // double
	int P_DeMethaneTS2; // double
	int P_MethaneTS2; // double
	int P_HydrogenTS2; // double
	int P_MuonKick; // double
	//int P_DmodRunTS2; // double
	//int P_DmodRunLimTS2; // double
	//int P_BeamDmodTS2; // double
	int P_OnTS1; //string
    int P_BeamEPB1; // double
	
	
	#define FIRST_ISISBEAM_PARAM P_BeamTS1
	#define LAST_ISISBEAM_PARAM P_BeamEPB1
	
	void pollerThread();
	epicsTimeStamp m_timestamp;
};

#define NUM_ISISBEAM_PARAMS (&LAST_ISISBEAM_PARAM - &FIRST_ISISBEAM_PARAM + 1)
 
#define P_BeamTS1String "BEAMTS1"
#define P_BeamTS2String "BEAMTS2"
#define P_MethaneTS1String "METHTS1"
#define P_HydrogenTS1String "HDGNTS1"
#define P_BeamEPB1String "BEAMEPB1"
#define P_BeamSynchString "BEAMSYNCH"
#define P_FreqSynchString "FREQSYNCH"
#define P_TotalTS1String "TOTALTS1"
#define P_FreqTS2String "FREQTS2"
#define P_TotalTS2String "TOTALTS2"
#define P_DeMethaneTS2String "DEMETHTS2"
#define P_MethaneTS2String "METHTS2"
#define P_HydrogenTS2String "HDGNTS2"
#define P_MuonKickString "MUKICK"
//#define P_DmodRunTS2String "DRUNTS2"
//#define P_DmodRunLimTS2String "DRLIMTS2"
//#define P_BeamDmodTS2String "BEAMDMODTS2"
#define P_OnTS1String "ONTS1"

#endif /* ISISBEAMDRIVER_H */
