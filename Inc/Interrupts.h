//==============================================================================
#if !defined(INTERRUPTS_H)
    #define INTERRUPTS_H


//------------------------------------------------------------------------------
extern "C"{

void RelocateVectors();

#if defined(STM32F10X_CL)
	#define EDROS_CoreIrqs    	16
	#define EDROS_F1xxIrqs      68
	#define EDROS_TotalIrqs		(EDROS_CoreIrqs + EDROS_F1xxIrqs)
#else
	#define EDROS_CoreIrqs      16
	#define EDROS_F1xxIrqs      67
	#define EDROS_TotalIrqs		(EDROS_CoreIrqs + EDROS_F1xxIrqs)
#endif
}

#endif
//==============================================================================
