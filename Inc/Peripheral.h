//===============================================================================
#ifndef PERIPHERALS_H
    #define PERIPHERALS_H

    //--------------------------------------
	// IO peripheral drivers
    #include "NTinyOutput.h"
    #include "NOutput.h"
    #include "NInput.h"

    //--------------------------------------
	// Time/frequency peripherals drivers
	#include "NHardwareTimer.h"
	#include "NLowPowerTimer.h"
	#include "NFrequencyMeter.h"
	
    //--------------------------------------
	// Analog peripherals drivers
	#include "NAdc.h"
	#include "NDac.h"
	#include "NPwm.h"

    //--------------------------------------
	// Communication peripherals drivers
	#include "NSerial.h"
	#include "NSpi.h"
	#include "NIic.h"

#endif
//==============================================================================
