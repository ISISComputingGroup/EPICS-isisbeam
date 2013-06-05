#ifndef ISISBEAMDRIVER_H
#define ISISBEAMDRIVER_H
 
#include "asynPortDriver.h"

class isisbeamDriver : public asynPortDriver 
{
public:
    isisbeamDriver(const char* portName);
	static void pollerThreadC(void* arg);
                 
private:
    int P_Beam; // double
	#define FIRST_ISISBEAM_PARAM P_Beam
	#define LAST_ISISBEAM_PARAM P_Beam
	
	void pollerThread();
};

#define NUM_ISISBEAM_PARAMS (&LAST_ISISBEAM_PARAM - &FIRST_ISISBEAM_PARAM + 1)
 
#define P_BeamString "BEAM"

#endif /* ISISBEAMDRIVER_H */
