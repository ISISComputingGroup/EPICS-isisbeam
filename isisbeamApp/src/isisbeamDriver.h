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

#endif /* ISISBEAMDRIVER_H */
