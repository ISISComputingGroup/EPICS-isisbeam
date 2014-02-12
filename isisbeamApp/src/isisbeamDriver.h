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
	int P_DmodRunTS2; // double
	int P_DmodRunLimTS2; // double
	int P_BeamDmodTS2; // double
	int P_DmodAnnLowTS2; // double
	int P_OnTS1; //string
	int P_OffTS1; //string
	int P_OnTS2; //string
	int P_OffTS2; //string
//Additions for story #266
	int P_N1Shut; //string
	int P_N2Shut; //string
	int P_N3Shut; //string
	int P_N4Shut; //string
	int P_N5Shut; //string
	int P_N6Shut; //string
	int P_N7Shut; //string
	int P_N8Shut; //string
	int P_N9Shut; //string
	int P_S1Shut; //string
	int P_S2Shut; //string
	int P_S3Shut; //string
	int P_S4Shut; //string
	int P_S5Shut; //string
	int P_S6Shut; //string
	int P_S7Shut; //string
	int P_S8Shut; //string
	int P_S9Shut; //string
	int P_E1Shut; //string
	int P_E2Shut; //string
	int P_E3Shut; //string
	int P_E4Shut; //string
	int P_E5Shut; //string
	int P_E6Shut; //string
	int P_E7Shut; //string
	int P_E8Shut; //string
	int P_E9Shut; //string
	int P_W1Shut; //string
	int P_W2Shut; //string
	int P_W3Shut; //string
	int P_W4Shut; //string
	int P_W5Shut; //string
	int P_W6Shut; //string
	int P_W7Shut; //string
	int P_W8Shut; //string
	int P_W9Shut; //string
	int P_E1VAT; //string
	int P_E2VAT; //string
	int P_E3VAT; //string
	int P_E4VAT; //string
	int P_E5VAT; //string
	int P_E6VAT; //string
	int P_E7VAT; //string
	int P_E8VAT; //string
	int P_E9VAT; //string
	int P_W1VAT; //string
	int P_W2VAT; //string
	int P_W3VAT; //string
	int P_W4VAT; //string
	int P_W5VAT; //string
	int P_W6VAT; //string
	int P_W7VAT; //string
	int P_W8VAT; //string
	int P_W9VAT; //string
//End Additions for story #266
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
#define P_DmodRunTS2String "DRUNTS2"
#define P_DmodRunLimTS2String "DRLIMTS2"
#define P_BeamDmodTS2String "BEAMDMODTS2"
#define P_DmodAnnLowTS2String "DMODANNLOWTS2"
#define P_OnTS1String "ONTS1"
#define P_OffTS1String "OFFTS1"
#define P_OnTS2String "ONTS2"
#define P_OffTS2String "OFFTS2"
//Additions for story #266
#define P_N1ShutString "N1SHUT"
#define P_N2ShutString "N2SHUT"
#define P_N3ShutString "N3SHUT"
#define P_N4ShutString "N4SHUT"
#define P_N5ShutString "N5SHUT"
#define P_N6ShutString "N6SHUT"
#define P_N7ShutString "N7SHUT"
#define P_N8ShutString "N8SHUT"
#define P_N9ShutString "N9SHUT"
#define P_S1ShutString "S1SHUT"
#define P_S2ShutString "S2SHUT"
#define P_S3ShutString "S3SHUT"
#define P_S4ShutString "S4SHUT"
#define P_S5ShutString "S5SHUT"
#define P_S6ShutString "S6SHUT"
#define P_S7ShutString "S7SHUT"
#define P_S8ShutString "S8SHUT"
#define P_S9ShutString "S9SHUT"
#define P_E1ShutString "E1SHUT"
#define P_E2ShutString "E2SHUT"
#define P_E3ShutString "E3SHUT"
#define P_E4ShutString "E4SHUT"
#define P_E5ShutString "E5SHUT"
#define P_E6ShutString "E6SHUT"
#define P_E7ShutString "E7SHUT"
#define P_E8ShutString "E8SHUT"
#define P_E9ShutString "E9SHUT"
#define P_W1ShutString "W1SHUT"
#define P_W2ShutString "W2SHUT"
#define P_W3ShutString "W3SHUT"
#define P_W4ShutString "W4SHUT"
#define P_W5ShutString "W5SHUT"
#define P_W6ShutString "W6SHUT"
#define P_W7ShutString "W7SHUT"
#define P_W8ShutString "W8SHUT"
#define P_W9ShutString "W9SHUT"
#define P_E1VATString "E1VAT"
#define P_E2VATString "E2VAT"
#define P_E3VATString "E3VAT"
#define P_E4VATString "E4VAT"
#define P_E5VATString "E5VAT"
#define P_E6VATString "E6VAT"
#define P_E7VATString "E7VAT"
#define P_E8VATString "E8VAT"
#define P_E9VATString "E9VAT"
#define P_W1VATString "W1VAT"
#define P_W2VATString "W2VAT"
#define P_W3VATString "W3VAT"
#define P_W4VATString "W4VAT"
#define P_W5VATString "W5VAT"
#define P_W6VATString "W6VAT"
#define P_W7VATString "W7VAT"
#define P_W8VATString "W8VAT"
#define P_W9VATString "W9VAT"
//End Additions for story #266

#endif /* ISISBEAMDRIVER_H */
