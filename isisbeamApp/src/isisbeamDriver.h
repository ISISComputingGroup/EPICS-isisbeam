#ifndef ISISBEAMDRIVER_H
#define ISISBEAMDRIVER_H
 
#include "asynPortDriver.h"

class isisbeamDriver : public asynPortDriver 
{
public:
    isisbeamDriver(const char* portName);
	static void pollerThreadC(void* arg);
                 
private:
    int P_BeamTS1; // double
    int P_BeamTS2; // double
    int P_BeamEPB1; // double
	#define FIRST_ISISBEAM_PARAM P_BeamTS1
	#define LAST_ISISBEAM_PARAM P_BeamEPB1
	
	void pollerThread();
};

#define NUM_ISISBEAM_PARAMS (&LAST_ISISBEAM_PARAM - &FIRST_ISISBEAM_PARAM + 1)
 
#define P_BeamTS1String "BEAMTS1"
#define P_BeamTS2String "BEAMTS2"
#define P_BeamEPB1String "BEAMEPB1"

#endif /* ISISBEAMDRIVER_H */
