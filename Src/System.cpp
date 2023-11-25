//============================================================================//
// Title: EDROS - Event-driven Real Time Operating System					  //
// Version: 1.0.0                                                             //
// Author: edros.master@pobox.com											  //
// Date: 19/10/2019                                                           //
// Last Update: 19/10/2019                                                    //
//----------------------------------------------------------------------------//
// Development Notes:                                                         //
//============================================================================//

//==============================================================================
//
// NOTE: 	CSTACK:  400h (1 Kbytes)
//          HEAP:    9000h(36Kbytes)
//==============================================================================

//------------------------------------------------------------------------------
// NOTE: The line below to enable/disable "Deep Sleep"
//#define DEEP_SLEEP_MODE
//------------------------------------------------------------------------------

//-------------------------------- Includes ------------------------------------
#include "System.h"
#include "DRV_CPU.h"
#include "Priorities.h"
#include "Interrupts.h"

//------------------------------------------------------------------------------
System*     SYS;

//------------------------------------------------------------------------------
void __attribute__((weak)) ApplicationException(uint32_t e);
void __attribute__((weak)) ApplicationCreate();

extern void RelocateVectors();

//------------------------------------------------------------------------------
// Disable SysTick IRQ and SysTick Timer
void System::Halt(){
  halt = true;
  SysTick->CTRL &= ~( SysTick_CTRL_CLKSOURCE_Msk |
                   	  SysTick_CTRL_TICKINT_Msk   |
                   	  SysTick_CTRL_ENABLE_Msk);
}

//------------------------------------------------------------------------------
// Enable SysTick IRQ and SysTick Timer
void System::Resume(){
  halt = false;
  SysTick->CTRL |=  ( SysTick_CTRL_CLKSOURCE_Msk |
                   	  SysTick_CTRL_TICKINT_Msk   |
                   	  SysTick_CTRL_ENABLE_Msk);
}

//------------------------------------------------------------------------------
// Check if SysTick is running
bool System::IsHalt(){ return(halt);}

//------------------------------------------------------------------------------
// "power saving mode"
void System::UpdatePowerdown(){
	//FLASH->ACR |= FLASH_ACR_SLEEP_PD; ///TDO
	if(sleep == true){
		__DSB();
		__WFI();
	}
}

//------------------------------------------------------------------------------
// Enter "power saving mode" (wake-up on interrupts)
void System::Sleep(bool s){
	if(s){
		// sets the "sleep on exit" to wait for the least prioritized interrupt to finish
		__DSB();
		SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;
	}
	sleep = s;
}

//------------------------------------------------------------------------------
// Check if in "power saving mode"
bool System::IsSleeping(){ return(sleep);}

//------------------------------------------------------------------------------
System::System(){}

//------------------------------------------------------------------------------
void System::Initialize(){

    //---------------------------------------
  	halt = false;
	sleep = false;
    time = 0L;
    ticks_timers = 1L;
    ticks_inputs = 2L;
    ticks_outputs = 3L;
	ksc0 = ksc1 = ksc2 = 1L;

    //---------------------------------------
    for(uint32_t i = 0; i<__SYS_MAX_VECTORS; i++) sysVectors[i] = 0;

	__disable_irq();
	
    //---------------------------------------
    queue = new NMessagePipe();
	
    //---------------------------------------
    CallbackQueue = new NFifo(sizeof(NMESSAGE), __SYS_STANDARD_CALLBACKS);
	
	__enable_irq();

	#ifdef DEEP_SLEEP_MODE
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; /* Enable deep sleep feature */
	#endif
}

//------------------------------------------------------------------------------
void System::Heartbeat(){
    NMESSAGE    Msg1;

    time++;
    UpdateTimeouts();

    //----------------------------------------
    if(__SYS_TICK_RATE > 0){
        if(++ticks_timers > __SYS_TICK_RATE){
            Msg1.message = NM_TIMETICK;
            Msg1.data1 = 0L;
            Msg1.data2 = time;
            queue->Insert(&Msg1);
            ticks_timers = 1L;
        }
    }

    //----------------------------------------
    if(__SYS_SCAN_RATE > 0){
        if(++ticks_inputs > __SYS_SCAN_RATE){
            Msg1.message = NM_KEYSCAN;
            Msg1.data1 = 0L; Msg1.data2 = 0L;
            queue->Insert(&Msg1);
            ticks_inputs = 1L;
        }
    }

    //----------------------------------------
    if(__SYS_UPDATE_RATE > 0){
        if(++ticks_outputs > __SYS_UPDATE_RATE){
            Msg1.message = NM_REPAINT;
            Msg1.data1 = 0L; Msg1.data2 = 0L;
            SYS->queue->Insert(&Msg1);
            ticks_outputs = 1L;
        }
    }
    return;
}

//------------------------------------------------------------------------------
bool System::IncludeComponent(HANDLE newcomp){
    return(queue->IncludeComponent(newcomp));
}

//------------------------------------------------------------------------------
bool System::ExcludeComponent(HANDLE comp){
    return(queue->ExcludeComponent(comp));
}

//------------------------------------------------------------------------------
bool System::FindComponent(HANDLE comp){
	bool result = true;
	if(queue->FindComponent(comp) == __SYS_INDEX_INVALID){ result = false;}
	return(result);
}

//------------------------------------------------------------------------------
// register new component
// hcomp: component�s handle; NV_ID vector index
// retorno: se for diferente de hcomp deve ser armazenado no componente p/ chamadas
HANDLE System::InstallCallback(HANDLE hcomp, NV_ID vct_index){
    HANDLE hprev;
    hprev = sysVectors[vct_index];
    sysVectors[vct_index] = hcomp;
    return(hprev);
}

//------------------------------------------------------------------------------
// return component�s Handler, if registered
// hcomp: component�s handle; NV_ID vector index
HANDLE System::GetCallback(NV_ID vector_id){
    return(sysVectors[vector_id]);
}

//------------------------------------------------------------------------------
// insert message in the Callback notifications queue
void System::CallbackSchedule(NMESSAGE* Msg){
	CallbackQueue->Put((uint8_t*)Msg, sizeof(NMESSAGE));
	// calls the callback service
	*((uint32_t volatile*) 0xE000ED04) |= 0x10000000;
}

//------------------------------------------------------------------------------
// remove message from Callback notifications queue
bool System::CallbackAttend(NMESSAGE* Msg){
	bool result = (bool)CallbackQueue->Counter();
	if(result){ CallbackQueue->Get((uint8_t*)Msg);}
	return result;
}

//------------------------------------------------------------------------------
// finish a Callback notification
void System::CallbackAttended(){
	// when leaving, assure non-occurrency of "late arrival"
	*((uint32_t volatile*) 0xE000ED04) &= ~0x10000000;
}

///-----------------------------------------------------------------------------
// dispatch notification based on "Priority"
//------------------------------------------------------------------------------
// Formato da mensagem passada ao dispatcher
// Msg1 resultante: message = NM_DMA_OK, NM_DMA_MOK, NM_DMA_ERR ou NM_NULL
//                  data1  = vector id (NV_ID) para "busca do sender"
//                  data2  = n�mero do canal de DMA
//                  tag     = n�o usado
// ATEN��O: Componentes com DMA "DEVEM" implementar m�todo "InterrupCallBack"
//-----------------------------------------------------------------------------*/
void System::Dispatch(NMESSAGE* M){

	NComponent* Owner = NULL;
	if(M->message == (uint32_t)NULL){ return;}

    Owner = (NComponent*) GetCallback(M->data1);
    if((HANDLE)Owner != NULL){
		switch(Owner->Priority){
			case nTimeCritical:	Owner->InterruptCallBack(M); break;
			case nNormal:
				if(!sleep){ CallbackSchedule(M);}
				else { Owner->InterruptCallBack(M);}
				break;
			default: queue->Insert(M); break;
		}
	}
}

//------------------------------------------------------------------------------
// register a "timeout-event" in the system ( 1ms to 10 seconds)
// NOTE: DO NOT call this from within "classes" or "components"
bool System::InstallTimeout(void* flag, uint32_t tempo){
    bool result = false;
    for(int c = 0; c < __SYS_MAX_SIGNALS; c++){
        if(sysSignals[c].flag==NULL){
            sysSignals[c].flag = flag;
            if(tempo == 0){ tempo = 1;}
            else if(tempo > 10000){ tempo = 10000;}
            sysSignals[c].counter = tempo;
            result = true; break;
        }
    }
    return(result);
}

//------------------------------------------------------------------------------
// update timeout-events in the system
uint32_t System::UpdateTimeouts(){
    uint32_t result = 0;
    for(int c = 0; c < __SYS_MAX_SIGNALS; c++){
        if(sysSignals[c].flag != NULL){
            if(--sysSignals[c].counter == 0){
                *(bool*)(sysSignals[c].flag) = true;
                sysSignals[c].counter = -1;
                sysSignals[c].flag = NULL;
                result++;
            }
        }
    }
    return(result);
}

//------------------------------------------------------------------------------
uint32_t System::GetSystemTime(){ return(this->time);}

//------------------------------------------------------------------------------
uint8_t System::GetRandomNumber(){
	uint8_t result; this->ksc2 = SysTick->VAL;
	result = (uint8_t)(this->ksc0 * this->ksc1 * this->ksc2);
	this->ksc0= this->ksc1; this->ksc1 = this->ksc2;
	return(result);
}

//------------------------------------------------------------------------------
// pseudo-random in the range of 1 to 254
uint8_t System::GetKSCode(){
  	uint8_t result = 0; 
	while((result==0x00)||(result==0xFF)){result=GetRandomNumber();}
	return(result);
}

//------------------------------------------------------------------------------
void System::Execute(){

    while(1){
        CPU_KickWatchdog();
        queue->Dispatch();
		UpdatePowerdown();
    }
}

//------------------------------------------------------------------------------
/*uint32_t System::SetClockFactors(){
    SystemCoreClockUpdate();
    uint32_t tick_rate = SystemCoreClock/1000;
    SysTick_Config(tick_rate);	// Ticks a cada 1ms
    tick_rate /= 1000;

    while(tick_rate > 0){
        fclk++; tick_rate >>= 1;
    }
    return(fclk);
}*/

//------------------------------------------------------------------------------
uint32_t System::Microseconds(void){
  
    uint32_t microseconds = SysTick->VAL >> 3;
    return((time * 1000) + microseconds);
}

//------------------------------------------------------------------------------
void System::Delay(uint32_t ms){
    uint32_t t0 = time;
    uint32_t delta = 0;
    
    while(ms > delta){
        delta = time - t0;
    }
}

//------------------------------------------------------------------------------
void System::MicroDelay(uint32_t us){
    uint32_t t1 = 0;
    uint32_t delta = 0;
    uint32_t t0 = SysTick->VAL >> fclk;
    uint32_t t3 = SysTick->LOAD >> fclk;
    
    while(us > delta){
        t1 = SysTick->VAL >> fclk;
        if(t1 < t0){ delta = t0 - t1;}
        else { delta = t0 + (t3 - t1);}
    }
}

//------------------------------------------------------------------------------
int main(void){

	//------------------------------------------------
	// *** All of these functions tested with oscilloscope via MCO ***
	//CPU_StartHSI();	// OK
	//CPU_StartPLL(Pll_Hsi, Pll16MHz);	// OK
	//CPU_StartPLL(Pll_Hsi, Pll32MHz);	// OK
	//CPU_StartPLL(Pll_Hsi, Pll36MHz);	// OK
	//CPU_StartPLL(Pll_Hsi, Pll72MHz);	// OK (64MHz max. for HSI)

	//CPU_StartHSE();	// OK
	CPU_StartPLL(Pll_Hse, Pll16MHz);	// OK
	//CPU_StartPLL(Pll_Hse, Pll36MHz);	// OK
	//CPU_StartPLL(Pll_Hse, Pll72MHz);  // OK
	//------------------------------------------------

    //-----------------------------------------
	// framework priority group definition
	NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);

	uint32_t priority = NVIC_EncodePriority(NVIC_PriorityGroup_4, SYS_PRIORITY_LOWEST, 0);
    NVIC_SetPriority(SysTick_IRQn, priority);
    priority = NVIC_EncodePriority(NVIC_PriorityGroup_4, SYS_PRIORITY_LOW, 0);
    NVIC_SetPriority(PendSV_IRQn, priority);
    priority = NVIC_EncodePriority(NVIC_PriorityGroup_4, SYS_PRIORITY_NORMAL, 0);
    NVIC_SetPriority(SVCall_IRQn, priority);

    //-----------------------------------------
    SYS = new System();

    //-----------------------------------------
	// relocate IRQs vector table
	RelocateVectors();


    //-----------------------------------------
    SYS->AppStart = ApplicationCreate;

    //-----------------------------------------
    SYS->AppException = ApplicationException;
    
    //-----------------------------------------
    SYS->Initialize();

    //-----------------------------------------
    SysTick_Config(SystemCoreClock/1000);

	//-----------------------------------------
	// workaround to avoid faults in the "deploy version"
	NComponent* Bu = new NComponent();
	Bu->~NComponent();

    //-----------------------------------------
    if(SYS->AppStart != NULL){ SYS->AppStart();}

    //-----------------------------------------
    SYS->Execute();
    return(0);
}

//------------------------------------------------------------------------------
void __attribute__((weak)) ApplicationCreate(){}
void __attribute__((weak)) ApplicationException(uint32_t e){}
    
//==============================================================================
