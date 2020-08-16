// tide library for station name IndianRiverDelaware
#ifndef tides_h
#define tides_h

#include "RTClib.h"

class TideCalc {
 public:
	TideCalc();
		float currentTide(DateTime now);
		char* returnStationID(void);
		long returnStationIDnumber(void);
};
#endif
