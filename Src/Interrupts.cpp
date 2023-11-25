//============================================================================//
// Title: IRQ Handlers                                 					  	  //
// Version: 1.0.0                                                             //
// Author: J. Nilo Rodrigues                                                  //
// Date: 18/08/2022                                                           //
//============================================================================//
//#include "SysInts.h"
#include "DRV_CPU.h"
#include "DRV_SSR.h"
#include "System.h"
#include "DRV_IO.h"
#include "Interrupts.h"

#define optional __attribute__((unused))

void EDROS_HardFault_Handler(){ while(1);}
void EDROS_MemManage_Handler(){ while(1);}
void EDROS_BusFault_Handler(){ while(1);}
void EDROS_UsageFault_Handler(){ while(1);}
void EDROS_DebugMon_Handler(){ while(1);}

//------------------------------------------------------------------------------
extern "C" {
#ifdef __GNUC__	
void __attribute__((naked)) EDROS_SVC_Handler(void){
	asm volatile(
		"mrs r0, msp\t\n"
		"ldr r1, =EDROS_SVCall_Handler \t\n"
		"bx r1"
	);
}
#elif defined(__ICCARM__)
void EDROS_SVC_Handler(void){
    asm("MRS R0, MSP");
    asm("B EDROS_SVCall_Handler");
}
#endif
}

//------------------------------------------------------------------------------
extern "C" {
#ifdef __GNUC__	
void EDROS_SVCall_Handler(uint32_t* param){

    uint32_t R0 = param[0];    // 1st param of function "CallService(p1, p2, ...)"
    uint32_t R1 = param[1];    // 2nd param of function "CallService(p1, p2, ...)"
    uint32_t svc_number = ((char*) param[6])[-2]; // service number (SVC #)

    switch(svc_number){
        // Initialize
        case 0:
          RelocateVectors();
          break;
        case 1:
          SYS->IncludeComponent((HANDLE)R0);
          break;
        case 2:
          SYS->ExcludeComponent((HANDLE)R0);
          break;
        case 3:
          SYS->InstallCallback((HANDLE)R0, R1);
          break;
        case 4: break;
        case 5:
          param[0] = (uint32_t)SYS->GetCallback(R0);
          break;
        case 6:
          param[0] = (uint32_t)SYS->GetSystemTime();
          break;
        case 7:
          param[0] = (uint32_t)SYS->InstallTimeout((void*)R0, R1);
          break;
        case 8:
          param[0] = (uint32_t)SYS->GetKSCode();
          break;
        case 9:
          break;

        case SVC_THROW_MESSAGE:{
            NMESSAGE Msg1 = { param[0], param[1], param[2], param[3]};
            SYS->Dispatch(&Msg1);
        } break;

        case SVC_THROW_EXCEPTION:
          SYS->AppException(R0);
          break;

        case SVC_MICROSECONDS:
          SYS->Microseconds();
          break;

        case SVC_DELAY:
          SYS->Delay(R0);
          break;

        case SVC_MICRODELAY:
          SYS->MicroDelay(R0);
          break;

        default: break;
    }
}
#elif defined(__ICCARM__)
__irq void EDROS_SVCall_Handler(uint32_t* svc_args){
    volatile uint32_t regR0  = svc_args[0]; // stack
    volatile uint32_t regR1  = svc_args[1]; // parâmetro 1
    volatile uint32_t regR2  = svc_args[2]; // parâmetro 2
	volatile uint32_t regR3  = svc_args[3]; // parâmetro 3
    volatile uint32_t regR12 = svc_args[4]; // parâmetro 4
	char* svc_number_addr = (char*)svc_args[6];
	uint32_t svc_number = *(svc_number_addr-2);
	
	switch(svc_number){
        case SVC_RELOCATE_VECTORS:
            RelocateVectors();
            break;
		case SVC_INCLUDE_COMPONENT: 
            SYS->IncludeComponent((HANDLE)regR0);
            break;
		case SVC_EXCLUDE_COMPONENT:
            SYS->ExcludeComponent((HANDLE)regR0);
            break;
		case SVC_INSTALL_CALLBACK:	
            svc_args[0] = (uint32_t)SYS->InstallCallback((HANDLE)regR0, (NV_ID)regR1);
            break;
        case SVC_FIND_COMPONENT:
            svc_args[0] = (uint32_t)SYS->FindComponent((HANDLE)regR0);
            break;
		case SVC_GET_CALLBACK:
            svc_args[0] = (uint32_t)SYS->GetCallback((NV_ID) regR0);
            break;
		case SVC_GET_SYSTEM_TIME:
            svc_args[0] = SYS->GetSystemTime();
            break;
		case SVC_INSTALL_TIMEOUT:
            svc_args[0] = (uint32_t)SYS->InstallTimeout((void*)regR0, regR1);
          break;
        case SVC_GET_KS_CODE:
          svc_args[0] = (uint32_t)SYS->GetKSCode();
          break;
        case SVC_GET_CALLBACK_VECTOR:
          svc_args[0] = (uint32_t)SYS->GetCallbackVector((HANDLE)regR0);
          break;
        case SVC_THROW_MESSAGE:
          break;
        
        case SVC_THROW_EXCEPTION:
          break;
	}
    return;
}
#endif
}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_PendSV_Handler(void){
	NMESSAGE Msg1;
	NComponent* Owner = NULL;

	while(SYS->CallbackAttend(&Msg1)){
		if(Msg1.data1 <= NV_LAST){
			Owner = (NComponent*)SYS->GetCallback(Msg1.data1);
			if(Owner != NULL){
				Owner->InterruptCallBack(&Msg1);
				if(Msg1.message != NM_NULL){ SYS->Dispatch(&Msg1);}
			}
		}
	}
	SYS->CallbackAttended();
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_SysTick_Handler (void){
    SYS->Heartbeat();
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA1_Channel1_IRQHandler(void){
    NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    Msg1.data1 = NV_DMA1_CH1;
    Msg1.data2 = (uint32_t) DMA1_Channel1;

    if(DMA1->ISR & DMA_ISR_GIF1){
        if(DMA1->ISR & DMA_ISR_TCIF1){
            DMA1->IFCR =  DMA_IFCR_CTCIF1 | DMA_IFCR_CGIF1;
            Msg1.message = NM_DMA_OK;
        } else if(DMA1->ISR & DMA_ISR_HTIF1){
            DMA1->IFCR =  DMA_IFCR_CHTIF1 | DMA_IFCR_CGIF1;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA1->ISR & DMA_ISR_TEIF1){
            DMA1->IFCR =  DMA_IFCR_CTEIF1 | DMA_IFCR_CGIF1;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
 	return;	
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA1_Channel2_IRQHandler(void){
    NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    Msg1.data1 = NV_DMA1_CH2;
    Msg1.data2 = (uint32_t) DMA1_Channel2;

    if(DMA1->ISR & DMA_ISR_GIF2){
        if(DMA1->ISR & DMA_ISR_TCIF2){
            DMA1->IFCR =  DMA_IFCR_CTCIF2 | DMA_IFCR_CGIF2;
            Msg1.message = NM_DMA_OK;
        } else if(DMA1->ISR & DMA_ISR_HTIF2){
            DMA1->IFCR =  DMA_IFCR_CHTIF2 | DMA_IFCR_CGIF2;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA1->ISR & DMA_ISR_TEIF2){
            DMA1->IFCR =  DMA_IFCR_CTEIF2 | DMA_IFCR_CGIF2;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA1_Channel3_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

	Msg1.data1 = NV_DMA1_CH3;
    Msg1.data2 = (uint32_t) DMA1_Channel3;

    if(DMA1->ISR & DMA_ISR_GIF3){
        if(DMA1->ISR & DMA_ISR_TCIF3){
            DMA1->IFCR =  DMA_IFCR_CTCIF3 | DMA_IFCR_CGIF3;
            Msg1.message = NM_DMA_OK;
        } else if(DMA1->ISR & DMA_ISR_HTIF3){
            DMA1->IFCR =  DMA_IFCR_CHTIF3 | DMA_IFCR_CGIF3;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA1->ISR & DMA_ISR_TEIF3){
            DMA1->IFCR =  DMA_IFCR_CTEIF3 | DMA_IFCR_CGIF3;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA1_Channel4_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

	Msg1.data1 = NV_DMA1_CH4;
    Msg1.data2 = (uint32_t) DMA1_Channel4;

    if(DMA1->ISR & DMA_ISR_GIF4){
        if(DMA1->ISR & DMA_ISR_TCIF4){
            DMA1->IFCR =  DMA_IFCR_CTCIF4 | DMA_IFCR_CGIF4;
            Msg1.message = NM_DMA_OK;
        } else if(DMA1->ISR & DMA_ISR_HTIF4){
            DMA1->IFCR =  DMA_IFCR_CHTIF4 | DMA_IFCR_CGIF4;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA1->ISR & DMA_ISR_TEIF4){
            DMA1->IFCR =  DMA_IFCR_CTEIF4 | DMA_IFCR_CGIF4;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;	
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA1_Channel5_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    Msg1.data1 = NV_DMA1_CH5;
    Msg1.data2 = (uint32_t) DMA1_Channel5;

    if(DMA1->ISR & DMA_ISR_GIF5){
        if(DMA1->ISR & DMA_ISR_TCIF5){
            DMA1->IFCR =  DMA_IFCR_CTCIF5 | DMA_IFCR_CGIF5;
            Msg1.message = NM_DMA_OK;
        } else if(DMA1->ISR & DMA_ISR_HTIF5){
            DMA1->IFCR =  DMA_IFCR_CHTIF5 | DMA_IFCR_CGIF5;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA1->ISR & DMA_ISR_TEIF5){
            DMA1->IFCR =  DMA_IFCR_CTEIF5 | DMA_IFCR_CGIF5;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA1_Channel6_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    Msg1.data1 = NV_DMA1_CH6;
    Msg1.data2 = (uint32_t) DMA1_Channel6;

    if(DMA1->ISR & DMA_ISR_GIF6){
        if(DMA1->ISR & DMA_ISR_TCIF6){
            DMA1->IFCR =  DMA_IFCR_CTCIF6 | DMA_IFCR_CGIF6;
            Msg1.message = NM_DMA_OK;
        } else if(DMA1->ISR & DMA_ISR_HTIF6){
            DMA1->IFCR =  DMA_IFCR_CHTIF6 | DMA_IFCR_CGIF6;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA1->ISR & DMA_ISR_TEIF6){
            DMA1->IFCR =  DMA_IFCR_CTEIF6 | DMA_IFCR_CGIF6;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;	
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA1_Channel7_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

	Msg1.data1 = NV_DMA1_CH7;
    Msg1.data2 = (uint32_t) DMA1_Channel7;

    if(DMA1->ISR & DMA_ISR_GIF7){
        if(DMA1->ISR & DMA_ISR_TCIF7){
            DMA1->IFCR =  DMA_IFCR_CTCIF7 | DMA_IFCR_CGIF7;
            Msg1.message = NM_DMA_OK;
        } else if(DMA1->ISR & DMA_ISR_HTIF7){
            DMA1->IFCR =  DMA_IFCR_CHTIF7 | DMA_IFCR_CGIF7;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA1->ISR & DMA_ISR_TEIF7){
            DMA1->IFCR =  DMA_IFCR_CTEIF7 | DMA_IFCR_CGIF7;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;	
}}

#if defined(DMA2)
//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA2_Channel1_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    Msg1.data1 = NV_DMA2_CH1;
    Msg1.data2 = (uint32_t) DMA2_Channel1;

    if(DMA2->ISR & DMA_ISR_GIF1){
        if(DMA2->ISR & DMA_ISR_TCIF1){
            DMA2->IFCR =  DMA_IFCR_CTCIF1 | DMA_IFCR_CGIF1;
            Msg1.message = NM_DMA_OK;
        } else if(DMA2->ISR & DMA_ISR_HTIF1){
            DMA2->IFCR =  DMA_IFCR_CHTIF1 | DMA_IFCR_CGIF1;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA2->ISR & DMA_ISR_TEIF1){
            DMA2->IFCR =  DMA_IFCR_CTEIF1 | DMA_IFCR_CGIF1;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA2_Channel2_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    Msg1.data1 = NV_DMA2_CH2;
    Msg1.data2 = (uint32_t) DMA2_Channel2;

    if(DMA2->ISR & DMA_ISR_GIF2){
        if(DMA2->ISR & DMA_ISR_TCIF2){
            DMA2->IFCR =  DMA_IFCR_CTCIF2 | DMA_IFCR_CGIF2;
            Msg1.message = NM_DMA_OK;
        } else if(DMA2->ISR & DMA_ISR_HTIF2){
            DMA2->IFCR =  DMA_IFCR_CHTIF2 | DMA_IFCR_CGIF2;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA2->ISR & DMA_ISR_TEIF2){
            DMA2->IFCR =  DMA_IFCR_CTEIF2 | DMA_IFCR_CGIF2;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA2_Channel3_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    Msg1.data1 = NV_DMA2_CH3;
    Msg1.data2 = (uint32_t) DMA2_Channel3;

    if(DMA2->ISR & DMA_ISR_GIF3){
        if(DMA2->ISR & DMA_ISR_TCIF3){
            DMA2->IFCR =  DMA_IFCR_CTCIF3 | DMA_IFCR_CGIF3;
            Msg1.message = NM_DMA_OK;
        } else if(DMA2->ISR & DMA_ISR_HTIF3){
            DMA2->IFCR =  DMA_IFCR_CHTIF3 | DMA_IFCR_CGIF3;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA2->ISR & DMA_ISR_TEIF3){
            DMA2->IFCR =  DMA_IFCR_CTEIF3 | DMA_IFCR_CGIF3;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA2_Channel4_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    Msg1.data1 = NV_DMA2_CH4;
    Msg1.data2 = (uint32_t) DMA2_Channel4;

    if(DMA2->ISR & DMA_ISR_GIF4){
        if(DMA2->ISR & DMA_ISR_TCIF4){
            DMA2->IFCR =  DMA_IFCR_CTCIF4 | DMA_IFCR_CGIF4;
            Msg1.message = NM_DMA_OK;
        } else if(DMA2->ISR & DMA_ISR_HTIF4){
            DMA2->IFCR =  DMA_IFCR_CHTIF4 | DMA_IFCR_CGIF4;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA2->ISR & DMA_ISR_TEIF4){
            DMA2->IFCR =  DMA_IFCR_CTEIF4 | DMA_IFCR_CGIF4;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_DMA2_Channel5_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    Msg1.data1 = NV_DMA2_CH5;
    Msg1.data2 = (uint32_t) DMA2_Channel5;

    if(DMA2->ISR & DMA_ISR_GIF5){
        if(DMA2->ISR & DMA_ISR_TCIF5){
            DMA2->IFCR =  DMA_IFCR_CTCIF5 | DMA_IFCR_CGIF5;
            Msg1.message = NM_DMA_OK;
        } else if(DMA2->ISR & DMA_ISR_HTIF5){
            DMA2->IFCR =  DMA_IFCR_CHTIF5 | DMA_IFCR_CGIF5;
            Msg1.message = NM_DMA_MOK;
        } else if(DMA2->ISR & DMA_ISR_TEIF5){
            DMA2->IFCR =  DMA_IFCR_CTEIF5 | DMA_IFCR_CGIF5;
            Msg1.message = NM_DMA_ERR;
        } else {}
    }
	SYS->Dispatch(&Msg1);
	return;
}}
#endif

//------------------------------------------------------------------------------
extern "C" {
void EDROS_USART1_IRQHandler(void){
    NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    USART_TypeDef* USARTx = USART1;
    Msg1.data1 = NV_UART1;

    if((USARTx->CR1 & USART_CR1_RXNEIE)&&(USARTx->SR & USART_SR_RXNE)){
        Msg1.message = NM_UARTRX;
        Msg1.data2 = USARTx->DR;
    } else if((USARTx->CR1 & USART_CR1_TXEIE)&&(USARTx->SR & USART_SR_TXE)){
        USARTx->CR1 &= ~USART_CR1_TXEIE;
        Msg1.message = NM_UARTTX;
        Msg1.data2 = (unsigned int)USART_SR_TXE;
    } else if((USARTx->CR1 & USART_CR1_IDLEIE)&&(USARTx->SR & USART_SR_IDLE)){
        USARTx->SR &= ~USART_SR_IDLE;
        Msg1.data2 = USARTx->DR;
		Msg1.message = NM_UARTRXIDLE;
		Msg1.data2 = (unsigned int)USART_SR_IDLE;
    } else if((USARTx->CR1 & USART_CR1_TCIE)&&(USARTx->SR & USART_SR_TC)){
        USARTx->SR &= ~USART_SR_TC;
        USARTx->CR1 &= ~USART_CR1_TCIE;
        Msg1.message = NM_UARTTX;
        Msg1.data2 = (unsigned int)USART_SR_TC;
    } else if((USARTx->CR3 & USART_CR3_CTSIE)&&(USARTx->CR3 & USART_CR3_CTSE)&&
              (USARTx->SR &= USART_SR_CTS)){
        USARTx->SR &= ~USART_SR_CTS;                
        Msg1.message = NM_UARTCTS;
	} else {
        if((USARTx->CR3 & USART_CR3_EIE)&&(USARTx->SR & 0x0F)){
        	Msg1.message = NM_UARTERROR;
        	Msg1.data2 = USARTx->SR & 0x0F;
        }
		optional uint8_t dump = USARTx->SR; dump = USARTx->DR;
	}
	SYS->Dispatch(&Msg1);
    return;

}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_USART2_IRQHandler(void){
    NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    USART_TypeDef* USARTx = USART2;
    Msg1.data1 = NV_UART2;

    if((USARTx->CR1 & USART_CR1_RXNEIE)&&(USARTx->SR & USART_SR_RXNE)){
        Msg1.message = NM_UARTRX;
        Msg1.data2 = USARTx->DR;
    } else if((USARTx->CR1 & USART_CR1_TXEIE)&&(USARTx->SR & USART_SR_TXE)){
        USARTx->CR1 &= ~USART_CR1_TXEIE;
        Msg1.message = NM_UARTTX;
        Msg1.data2 = (unsigned int)USART_SR_TXE;
    } else if((USARTx->CR1 & USART_CR1_IDLEIE)&&(USARTx->SR & USART_SR_IDLE)){
        USARTx->SR &= ~USART_SR_IDLE;
        Msg1.data2 = USARTx->DR;
		Msg1.message = NM_UARTRXIDLE;
		Msg1.data2 = (unsigned int)USART_SR_IDLE;
    } else if((USARTx->CR1 & USART_CR1_TCIE)&&(USARTx->SR & USART_SR_TC)){
        USARTx->SR &= ~USART_SR_TC;
        USARTx->CR1 &= ~USART_CR1_TCIE;
        Msg1.message = NM_UARTTX;
        Msg1.data2 = (unsigned int)USART_SR_TC;
    } else if((USARTx->CR3 & USART_CR3_CTSIE)&&(USARTx->CR3 & USART_CR3_CTSE)&&
              (USARTx->SR &= USART_SR_CTS)){
        USARTx->SR &= ~USART_SR_CTS;                
        Msg1.message = NM_UARTCTS;
	} else {
        if((USARTx->CR3 & USART_CR3_EIE)&&(USARTx->SR & 0x0F)){
        	Msg1.message = NM_UARTERROR;
        	Msg1.data2 = USARTx->SR & 0x0F;
        }
		optional uint8_t dump = USARTx->SR; dump = USARTx->DR;
	}
	SYS->Dispatch(&Msg1);
    return;

}}

//------------------------------------------------------------------------------
#if defined(USART3)
extern "C" {
void EDROS_USART3_IRQHandler(void){
    NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    USART_TypeDef* USARTx = USART3;
    Msg1.data1 = NV_UART3;

    if((USARTx->CR1 & USART_CR1_RXNEIE)&&(USARTx->SR & USART_SR_RXNE)){
        Msg1.message = NM_UARTRX;
        Msg1.data2 = USARTx->DR;
    } else if((USARTx->CR1 & USART_CR1_TXEIE)&&(USARTx->SR & USART_SR_TXE)){
        USARTx->CR1 &= ~USART_CR1_TXEIE;
        Msg1.message = NM_UARTTX;
        Msg1.data2 = (unsigned int)USART_SR_TXE;
    } else if((USARTx->CR1 & USART_CR1_IDLEIE)&&(USARTx->SR & USART_SR_IDLE)){
        USARTx->SR &= ~USART_SR_IDLE;
        Msg1.data2 = USARTx->DR;
		Msg1.message = NM_UARTRXIDLE;
		Msg1.data2 = (unsigned int)USART_SR_IDLE;
    } else if((USARTx->CR1 & USART_CR1_TCIE)&&(USARTx->SR & USART_SR_TC)){
        USARTx->SR &= ~USART_SR_TC;
        USARTx->CR1 &= ~USART_CR1_TCIE;
        Msg1.message = NM_UARTTX;
        Msg1.data2 = (unsigned int)USART_SR_TC;
    } else if((USARTx->CR3 & USART_CR3_CTSIE)&&(USARTx->CR3 & USART_CR3_CTSE)&&
              (USARTx->SR &= USART_SR_CTS)){
        USARTx->SR &= ~USART_SR_CTS;                
        Msg1.message = NM_UARTCTS;
	} else {
        if((USARTx->CR3 & USART_CR3_EIE)&&(USARTx->SR & 0x0F)){
        	Msg1.message = NM_UARTERROR;
        	Msg1.data2 = USARTx->SR & 0x0F;
        }
		optional uint8_t dump = USARTx->SR; dump = USARTx->DR;
	}
	SYS->Dispatch(&Msg1);
    return;

}}
#endif
//------------------------------------------------------------------------------
#if defined(UART4)
extern "C" {
void EDROS_USART4_IRQHandler(void){
    NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    USART_TypeDef* USARTx = USART4;
    Msg1.data1 = NV_UART4;

    if((USARTx->CR1 & USART_CR1_RXNEIE)&&(USARTx->SR & USART_SR_RXNE)){
        Msg1.message = NM_UARTRX;
        Msg1.data2 = USARTx->DR;
    } else if((USARTx->CR1 & USART_CR1_TXEIE)&&(USARTx->SR & USART_SR_TXE)){
        USARTx->CR1 &= ~USART_CR1_TXEIE;
        Msg1.message = NM_UARTTX;
        Msg1.data2 = (unsigned int)USART_SR_TXE;
    } else if((USARTx->CR1 & USART_CR1_IDLEIE)&&(USARTx->SR & USART_SR_IDLE)){
        USARTx->SR &= ~USART_SR_IDLE;
        Msg1.data2 = USARTx->DR;
		Msg1.message = NM_UARTRXIDLE;
		Msg1.data2 = (unsigned int)USART_SR_IDLE;
    } else if((USARTx->CR1 & USART_CR1_TCIE)&&(USARTx->SR & USART_SR_TC)){
        USARTx->SR &= ~USART_SR_TC;
        USARTx->CR1 &= ~USART_CR1_TCIE;
        Msg1.message = NM_UARTTX;
        Msg1.data2 = (unsigned int)USART_SR_TC;
    } else if((USARTx->CR3 & USART_CR3_CTSIE)&&(USARTx->CR3 & USART_CR3_CTSE)&&
              (USARTx->SR &= USART_SR_CTS)){
        USARTx->SR &= ~USART_SR_CTS;                
        Msg1.message = NM_UARTCTS;
	} else {
        if((USARTx->CR3 & USART_CR3_EIE)&&(USARTx->SR & 0x0F)){
        	Msg1.message = NM_UARTERROR;
        	Msg1.data2 = USARTx->SR & 0x0F;
        }
		optional uint8_t dump = USARTx->SR; dump = USARTx->DR;
	}
	SYS->Dispatch(&Msg1);
    return;

}}
#endif

#if defined(UART5)
//------------------------------------------------------------------------------
extern "C" {
void EDROS_USART5_IRQHandler(void){
    NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    USART_TypeDef* USARTx = USART5;
    Msg1.data1 = NV_UART5;

    if((USARTx->CR1 & USART_CR1_RXNEIE)&&(USARTx->SR & USART_SR_RXNE)){
        Msg1.message = NM_UARTRX;
        Msg1.data2 = USARTx->DR;
    } else if((USARTx->CR1 & USART_CR1_TXEIE)&&(USARTx->SR & USART_SR_TXE)){
        USARTx->CR1 &= ~USART_CR1_TXEIE;
        Msg1.message = NM_UARTTX;
        Msg1.data2 = (unsigned int)USART_SR_TXE;
    } else if((USARTx->CR1 & USART_CR1_IDLEIE)&&(USARTx->SR & USART_SR_IDLE)){
        USARTx->SR &= ~USART_SR_IDLE;
        Msg1.data2 = USARTx->DR;
		Msg1.message = NM_UARTRXIDLE;
		Msg1.data2 = (unsigned int)USART_SR_IDLE;
    } else if((USARTx->CR1 & USART_CR1_TCIE)&&(USARTx->SR & USART_SR_TC)){
        USARTx->SR &= ~USART_SR_TC;
        USARTx->CR1 &= ~USART_CR1_TCIE;
        Msg1.message = NM_UARTTX;
        Msg1.data2 = (unsigned int)USART_SR_TC;
    } else if((USARTx->CR3 & USART_CR3_CTSIE)&&(USARTx->CR3 & USART_CR3_CTSE)&&
              (USARTx->SR &= USART_SR_CTS)){
        USARTx->SR &= ~USART_SR_CTS;                
        Msg1.message = NM_UARTCTS;
	} else {
        if((USARTx->CR3 & USART_CR3_EIE)&&(USARTx->SR & 0x0F)){
        	Msg1.message = NM_UARTERROR;
        	Msg1.data2 = USARTx->SR & 0x0F;
        }
		optional uint8_t dump = USARTx->SR; dump = USARTx->DR;
	}
	SYS->Dispatch(&Msg1);
    return;

}}
#endif

//------------------------------------------------------------------------------
extern "C" {
void EDROS_SPI1_IRQHandler(){
    NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    SPI_TypeDef* SPIx = SPI1;
    
	Msg1.data1 = NV_SPI1;
    Msg1.tag = (uint32_t) SPIx;

    if((SPIx->CR2 & SPI_CR2_RXNEIE)&&(SPIx->SR & SPI_SR_RXNE)){
        Msg1.data2 = SPIx->DR;
        Msg1.message = NM_SPIMRXNE;
    } else if((SPIx->CR2 & SPI_CR2_ERRIE)&&
       (SPIx->SR & (SPI_SR_OVR | SPI_SR_UDR | SPI_SR_MODF | SPI_SR_CRCERR))){
    	Msg1.data2 = SPIx->DR; Msg1.data2 = SPIx->SR;
        Msg1.message = NM_SPIERROR;
        if(SPIx->SR & SPI_SR_CRCERR){ SPIx->SR &= ~SPI_SR_CRCERR;}
    } else if((SPIx->CR2 & SPI_CR2_TXEIE)&&(SPIx->SR & SPI_SR_TXE)){
        Msg1.message = NM_SPIMTXE;
    }
	SYS->Dispatch(&Msg1);
    return;
}}

//------------------------------------------------------------------------------
#if defined(SPI2)
extern "C" {
void EDROS_SPI2_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    SPI_TypeDef* SPIx = SPI2;
    
	Msg1.data1 = NV_SPI2;
    Msg1.tag = (uint32_t) SPIx;

    if((SPIx->CR2 & SPI_CR2_RXNEIE)&&(SPIx->SR & SPI_SR_RXNE)){
        Msg1.data2 = SPIx->DR;
        Msg1.message = NM_SPIMRXNE;
    } else if((SPIx->CR2 & SPI_CR2_ERRIE)&&
       (SPIx->SR & (SPI_SR_OVR | SPI_SR_UDR | SPI_SR_MODF | SPI_SR_CRCERR))){
        Msg1.data2 = SPIx->DR;
        Msg1.message = NM_SPIERROR;
        if(SPIx->SR & SPI_SR_CRCERR){ SPIx->SR &= ~SPI_SR_CRCERR;}
    } else if((SPIx->CR2 & SPI_CR2_TXEIE)&&(SPIx->SR & SPI_SR_TXE)){
        Msg1.message = NM_SPIMTXE;
    }
	SYS->Dispatch(&Msg1);
    return;
}}
#endif
//------------------------------------------------------------------------------
#if defined (SPI3)
extern "C" {
void EDROS_SPI3_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    SPI_TypeDef* SPIx = SPI3;
    
	Msg1.data1 = NV_SPI3;
    Msg1.tag = (uint32_t) SPIx;

    if((SPIx->CR2 & SPI_CR2_RXNEIE)&&(SPIx->SR & SPI_SR_RXNE)){
        Msg1.data2 = SPIx->DR;
        Msg1.message = NM_SPIMRXNE;
    } else if((SPIx->CR2 & SPI_CR2_ERRIE)&&
       (SPIx->SR & (SPI_SR_OVR | SPI_SR_UDR | SPI_SR_MODF | SPI_SR_CRCERR))){
        Msg1.data2 = SPIx->DR;
        Msg1.message = NM_SPIERROR;
        if(SPIx->SR & SPI_SR_CRCERR){ SPIx->SR &= ~SPI_SR_CRCERR;}
    } else if((SPIx->CR2 & SPI_CR2_TXEIE)&&(SPIx->SR & SPI_SR_TXE)){
        Msg1.message = NM_SPIMTXE;
    }
	SYS->Dispatch(&Msg1);
    return;
}}
#endif

//------------------------------------------------------------------------------
extern "C" {
void EDROS_I2C1_EV_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    I2C_TypeDef* I2Cx = I2C1;
    Msg1.tag = (uint32_t) I2Cx;
	Msg1.data1 = NV_I2C1;
    
    uint32_t SR1 = I2Cx->SR1;
    optional uint32_t SR2 = I2Cx->SR2;

    if(I2Cx->CR2 & I2C_CR2_ITEVTEN){
        Msg1.message = NM_I2CEVENT;
        if(SR1 & I2C_SR1_SB){ Msg1.data2 = I2C_SR1_SB;}
        else if(SR1 & I2C_SR1_ADDR){ Msg1.data2 = I2C_SR1_ADDR;}
        else if(SR1 & I2C_SR1_ADDR){ Msg1.data2 = I2C_SR1_ADDR;}
        else if(SR1 & I2C_SR1_ADD10){ Msg1.data2 = I2C_SR1_ADD10;}
        else if(SR1 & I2C_SR1_STOPF){ Msg1.data2 = I2C_SR1_STOPF;}
        else if(SR1 & I2C_SR1_BTF){ Msg1.data2 = I2C_SR1_BTF;}
        else if(I2Cx->CR2 & I2C_CR2_ITBUFEN){
            if(SR1 & I2C_SR1_TXE){ Msg1.data2 = I2C_SR1_TXE;}
            else if(SR1 & I2C_SR1_RXNE){ Msg1.data2 = I2C_SR1_RXNE;}
        }
    }
	SYS->Dispatch(&Msg1);
	return;

}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_I2C1_ER_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    I2C_TypeDef* I2Cx = I2C1;
    Msg1.tag = (uint32_t) I2Cx;
	Msg1.data1 = NV_I2C1;
    
    uint32_t SR1 = I2Cx->SR1;
    optional uint32_t SR2 = I2Cx->SR2;
    
    if(I2Cx->CR2 & I2C_CR2_ITERREN){
        Msg1.message = NM_I2CERR;
        
        if(SR1 & I2C_SR1_BERR){ 
            I2Cx->SR1 &= ~I2C_SR1_BERR; Msg1.data2 = I2C_SR1_BERR;
        } else if(SR1 & I2C_SR1_ARLO){
            I2Cx->SR1 &= ~I2C_SR1_ARLO; Msg1.data2 = I2C_SR1_ARLO;
        } else if(SR1 & I2C_SR1_AF){ 
          I2Cx->SR1 &= ~I2C_SR1_AF; Msg1.data2 = I2C_SR1_AF;
        } else if(SR1 & I2C_SR1_OVR){
            I2Cx->SR1 &= ~I2C_SR1_OVR; Msg1.data2 = I2C_SR1_OVR;
        } else if(SR1 & I2C_SR1_PECERR){
          I2Cx->SR1 &= ~I2C_SR1_PECERR; Msg1.data2 = I2C_SR1_PECERR;
        } else if(SR1 & I2C_SR1_TIMEOUT){ 
            I2Cx->SR1 &= ~I2C_SR1_TIMEOUT; Msg1.data2 = I2C_SR1_TIMEOUT;
        } else if(SR1 & I2C_SR1_SMBALERT){ 
            I2Cx->SR1 &= ~I2C_SR1_SMBALERT; Msg1.data2 = I2C_SR1_SMBALERT;
        }
    }
	SYS->Dispatch(&Msg1);
    return;
}}

//------------------------------------------------------------------------------
#if defined(I2C2)
extern "C" {
void EDROS_I2C2_EV_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    I2C_TypeDef* I2Cx = I2C2;
    Msg1.tag = (uint32_t) I2Cx;
	Msg1.data1 = NV_I2C2;
    
    uint32_t SR1 = I2Cx->SR1;
    optional uint32_t SR2 = I2Cx->SR2;

    if(I2Cx->CR2 & I2C_CR2_ITEVTEN){
        Msg1.message = NM_I2CEVENT;
        if(SR1 & I2C_SR1_SB){ Msg1.data2 = I2C_SR1_SB;}
        else if(SR1 & I2C_SR1_ADDR){ Msg1.data2 = I2C_SR1_ADDR;}
        else if(SR1 & I2C_SR1_ADDR){ Msg1.data2 = I2C_SR1_ADDR;}
        else if(SR1 & I2C_SR1_ADD10){ Msg1.data2 = I2C_SR1_ADD10;}
        else if(SR1 & I2C_SR1_STOPF){ Msg1.data2 = I2C_SR1_STOPF;}
        else if(SR1 & I2C_SR1_BTF){ Msg1.data2 = I2C_SR1_BTF;}
        else if(I2Cx->CR2 & I2C_CR2_ITBUFEN){
            if(SR1 & I2C_SR1_TXE){ Msg1.data2 = I2C_SR1_TXE;}
            else if(SR1 & I2C_SR1_RXNE){ Msg1.data2 = I2C_SR1_RXNE;}
        }
    }
	SYS->Dispatch(&Msg1);
	return;

}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_I2C2_ER_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    I2C_TypeDef* I2Cx = I2C2;
    
    Msg1.tag = (uint32_t) I2Cx;
	Msg1.data1 = NV_I2C2;

    uint32_t SR1 = I2Cx->SR1;
    optional uint32_t SR2 = I2Cx->SR2;
    
    if(I2Cx->CR2 & I2C_CR2_ITERREN){
        Msg1.message = NM_I2CERR;
        
        if(SR1 & I2C_SR1_BERR){ 
            I2Cx->SR1 &= ~I2C_SR1_BERR; Msg1.data2 = I2C_SR1_BERR;
        } else if(SR1 & I2C_SR1_ARLO){
            I2Cx->SR1 &= ~I2C_SR1_ARLO; Msg1.data2 = I2C_SR1_ARLO;
        } else if(SR1 & I2C_SR1_AF){ 
          I2Cx->SR1 &= ~I2C_SR1_AF; Msg1.data2 = I2C_SR1_AF;
        } else if(SR1 & I2C_SR1_OVR){
            I2Cx->SR1 &= ~I2C_SR1_OVR; Msg1.data2 = I2C_SR1_OVR;
        } else if(SR1 & I2C_SR1_PECERR){
          I2Cx->SR1 &= ~I2C_SR1_PECERR; Msg1.data2 = I2C_SR1_PECERR;
        } else if(SR1 & I2C_SR1_TIMEOUT){ 
            I2Cx->SR1 &= ~I2C_SR1_TIMEOUT; Msg1.data2 = I2C_SR1_TIMEOUT;
        } else if(SR1 & I2C_SR1_SMBALERT){ 
            I2Cx->SR1 &= ~I2C_SR1_SMBALERT; Msg1.data2 = I2C_SR1_SMBALERT;
        }
    }
	SYS->Dispatch(&Msg1);
    return;
}}
#endif

//------------------------------------------------------------------------------
extern "C" {
void EDROS_CAN1_TX_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    CAN_TypeDef* CANx = CAN1;
    
    Msg1.message = NM_CANTX;
	Msg1.data1 = NV_CAN1;
    Msg1.tag = (unsigned int) CANx;

    if(CANx->TSR & CAN_TSR_RQCP0){
        CANx->TSR |= CAN_TSR_RQCP0; Msg1.data2 = CAN_TSR_RQCP0;
    } else if(CANx->TSR & CAN_TSR_RQCP1){
        CANx->TSR |= CAN_TSR_RQCP1; Msg1.data2 = CAN_TSR_RQCP1;
    } else if(CANx->TSR & CAN_TSR_RQCP2){
        CANx->TSR |= CAN_TSR_RQCP2; Msg1.data2 = CAN_TSR_RQCP2;
    } else { Msg1.data2 = 0;}
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_CAN1_RX0_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    CAN_TypeDef* CANx = CAN1;

    Msg1.message = NM_NULL;
	Msg1.data1 = NV_CAN1;
    Msg1.tag = (uint32_t)CANx;

    if((CANx->IER & CAN_IER_FOVIE0)&&(CANx->RF0R & CAN_RF0R_FOVR0)){
        Msg1.message = NM_CANRX_FAULT; Msg1.data2 = CAN_RF0R_FOVR0;
        CANx->RF0R |= CAN_RF0R_FOVR0;
    } else if((CANx->IER & CAN_IER_FFIE0)&&(CANx->RF0R & CAN_RF0R_FULL0)){
        Msg1.message = NM_CANRX_FULL; Msg1.data2 = CAN_RF0R_FULL0;
        CANx->RF0R |= CAN_RF0R_FULL0;
    } else if((CANx->IER & CAN_IER_FMPIE0)&&(CANx->RF0R & CAN_RF0R_FMP0)){
        Msg1.message = NM_CANRX; Msg1.data2 = CANx->RF0R & CAN_RF0R_FMP0;
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_CAN1_RX1_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    CAN_TypeDef* CANx = CAN1;

    Msg1.message = NM_NULL;
	Msg1.data1 = NV_CAN1;
    Msg1.tag = (uint32_t)CANx;

    if((CANx->IER & CAN_IER_FOVIE1)&&(CANx->RF1R & CAN_RF1R_FOVR1)){
        Msg1.message = NM_CANRX_FAULT; Msg1.data2 = CAN_RF1R_FOVR1;
        CANx->RF1R |= CAN_RF1R_FOVR1;
    } else if((CANx->IER & CAN_IER_FFIE1)&&(CANx->RF1R & CAN_RF1R_FULL1)){
        Msg1.message = NM_CANRX_FULL; Msg1.data2 = CAN_RF1R_FULL1;
        CANx->RF1R |= CAN_RF1R_FULL1;
    } else if((CANx->IER & CAN_IER_FMPIE1)&&(CANx->RF1R & CAN_RF1R_FMP1)){
        Msg1.message = NM_CANRX; Msg1.data2 = CANx->RF1R & CAN_RF1R_FMP1;
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_CAN1_SCE_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    CAN_TypeDef* CANx = CAN1;

    Msg1.message = NM_NULL;
	Msg1.data1 = NV_CAN1;
    Msg1.tag = (uint32_t)CANx;

    Msg1.data2 = CAN1->ESR;
    Msg1.message = NM_CANERROR;

    CANx->IER &= ~CAN_IER_ERRIE;
    if(CANx->MSR & CAN_MSR_WKUI){ CANx->IER &= CAN_IER_WKUIE;}
    if(CANx->MSR & CAN_MSR_SLAKI){ CANx->IER &= CAN_IER_SLKIE;}
	SYS->Dispatch(&Msg1);
	return;	
}}

//------------------------------------------------------------------------------
#if defined(CAN2)
extern "C" {
void EDROS_CAN2_TX_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    CAN_TypeDef* CANx = CAN2;
    
    Msg1.message = NM_CANTX;
	Msg1.data1 = NV_CAN2;
    Msg1.tag = (unsigned int) CANx;

    if(CANx->TSR & CAN_TSR_RQCP0){
        CANx->TSR |= CAN_TSR_RQCP0; Msg1.data2 = CAN_TSR_RQCP0;
    } else if(CANx->TSR & CAN_TSR_RQCP1){
        CANx->TSR |= CAN_TSR_RQCP1; Msg1.data2 = CAN_TSR_RQCP1;
    } else if(CANx->TSR & CAN_TSR_RQCP2){
        CANx->TSR |= CAN_TSR_RQCP2; Msg1.data2 = CAN_TSR_RQCP2;
    } else { Msg1.data2 = 0;}
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_CAN2_RX0_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    CAN_TypeDef* CANx = CAN2;

    Msg1.message = NM_NULL;
	Msg1.data1 = NV_CAN2;
    Msg1.tag = (uint32_t)CANx;

    if((CANx->IER & CAN_IER_FOVIE0)&&(CANx->RF0R & CAN_RF0R_FOVR0)){
        Msg1.message = NM_CANRX_FAULT; Msg1.data2 = CAN_RF0R_FOVR0;
        CANx->RF0R |= CAN_RF0R_FOVR0;
    } else if((CANx->IER & CAN_IER_FFIE0)&&(CANx->RF0R & CAN_RF0R_FULL0)){
        Msg1.message = NM_CANRX_FULL; Msg1.data2 = CAN_RF0R_FULL0;
        CANx->RF0R |= CAN_RF0R_FULL0;
    } else if((CANx->IER & CAN_IER_FMPIE0)&&(CANx->RF0R & CAN_RF0R_FMP0)){
        Msg1.message = NM_CANRX; Msg1.data2 = CANx->RF0R & CAN_RF0R_FMP0;
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_CAN2_RX1_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    CAN_TypeDef* CANx = CAN2;

    Msg1.message = NM_NULL;
	Msg1.data1 = NV_CAN2;
    Msg1.tag = (uint32_t)CANx;

    if((CANx->IER & CAN_IER_FOVIE1)&&(CANx->RF1R & CAN_RF1R_FOVR1)){
        Msg1.message = NM_CANRX_FAULT; Msg1.data2 = CAN_RF1R_FOVR1;
        CANx->RF1R |= CAN_RF1R_FOVR1;
    } else if((CANx->IER & CAN_IER_FFIE1)&&(CANx->RF1R & CAN_RF1R_FULL1)){
        Msg1.message = NM_CANRX_FULL; Msg1.data2 = CAN_RF1R_FULL1;
        CANx->RF1R |= CAN_RF1R_FULL1;
    } else if((CANx->IER & CAN_IER_FMPIE1)&&(CANx->RF1R & CAN_RF1R_FMP1)){
        Msg1.message = NM_CANRX; Msg1.data2 = CANx->RF1R & CAN_RF1R_FMP1;
    }
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_CAN2_SCE_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};
    CAN_TypeDef* CANx = CAN2;

    Msg1.message = NM_NULL;
	Msg1.data1 = NV_CAN2;
    Msg1.tag = (uint32_t)CANx;

    Msg1.data2 = CAN1->ESR;
    Msg1.message = NM_CANERROR;

    CANx->IER &= ~CAN_IER_ERRIE;
    if(CANx->MSR & CAN_MSR_WKUI){ CANx->IER &= CAN_IER_WKUIE;}
    if(CANx->MSR & CAN_MSR_SLAKI){ CANx->IER &= CAN_IER_SLKIE;}
	SYS->Dispatch(&Msg1);
	return;	
}}
#endif  // NCAN_H

//------------------------------------------------------------------------------
extern "C" {
void EDROS_EXTI0_IRQHandler(){
    NMESSAGE Msg1 = {NM_EXTINT, (uint32_t)NV_EXTINT0, 0, 0};
	
    EXTI->PR |= EXTI_PR_PR0;
    Msg1.data2 = IO_GetExtendedIT(0);
    Msg1.tag = NV_EXTINT0; // <<<<<<<< ATENÇÃO: SERÁ RETIRADO <<<<<<<
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_EXTI1_IRQHandler(){
    NMESSAGE Msg1 = {NM_EXTINT, (uint32_t)NV_EXTINT1, 0, 0};
	
    EXTI->PR |= EXTI_PR_PR1;
    Msg1.data2 = IO_GetExtendedIT(1);
	Msg1.tag = NV_EXTINT1; // <<<<<<<< ATENÇÃO: SERÁ RETIRADO <<<<<<<
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_EXTI2_IRQHandler(){
    NMESSAGE Msg1 = {NM_EXTINT, (uint32_t)NV_EXTINT2, 0, 0};
	
    EXTI->PR |= EXTI_PR_PR2;
    Msg1.data2 = IO_GetExtendedIT(2);
	Msg1.tag = NV_EXTINT2;	// <<<<<<<< ATENÇÃO: SERÁ RETIRADO <<<<<<<
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_EXTI3_IRQHandler(){
    NMESSAGE Msg1 = {NM_EXTINT, (uint32_t)NV_EXTINT3, 0, 0};
	
    EXTI->PR |= EXTI_PR_PR3;
    Msg1.data2 = IO_GetExtendedIT(3);
	Msg1.tag = NV_EXTINT3;	// <<<<<<<< ATENÇÃO: SERÁ RETIRADO <<<<<<<
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_EXTI4_IRQHandler(){
    NMESSAGE Msg1 = {NM_EXTINT, (uint32_t)NV_EXTINT4, 0, 0};
	
    EXTI->PR |= EXTI_PR_PR4;
    Msg1.data2 = IO_GetExtendedIT(4);
    Msg1.tag = Msg1.data1;	// <<<<<<<< ATENÇÃO: SERÁ RETIRADO <<<<<<<
	SYS->Dispatch(&Msg1);
	return;	
}}

//------------------------------------------------------------------------------
// NOVO FORMATO DE MENSAGEM A SER ADOTADO
//------------------------------------------------------------------------------
// external interrupt services 5, 6, 7, 8, 9
//    message = NM_EXTINT
//    data1  = vector id (NV_ID) allows "sender search"
//    data2  = pending register EXTI->PR
//    tag     = interrupt source port (GPIOx)
// ATENÇÃO: All the "hardware driver" components must implement "InterrupCallBack"
//------------------------------------------------------------------------------
extern "C" {
void EDROS_EXTI9_5_IRQHandler(){
	NMESSAGE Msg1 = {NM_EXTINT, 0, 0, 0};
	uint32_t pin = -1;
	
    if(EXTI->PR & EXTI_PR_PR5){
        EXTI->PR |= EXTI_PR_PR5; Msg1.data1 = NV_EXTINT5; pin=5;
    } else if(EXTI->PR & EXTI_PR_PR6){
        EXTI->PR |= EXTI_PR_PR6; Msg1.data1 = NV_EXTINT6; pin=6;
    } else if(EXTI->PR & EXTI_PR_PR7){
        EXTI->PR |= EXTI_PR_PR7; Msg1.data1 = NV_EXTINT7; pin=7;
    } else if(EXTI->PR & EXTI_PR_PR8){
        EXTI->PR |= EXTI_PR_PR8; Msg1.data1 = NV_EXTINT8; pin=8;
    } else if(EXTI->PR & EXTI_PR_PR9){
        EXTI->PR |= EXTI_PR_PR9; Msg1.data1 = NV_EXTINT9; pin=9;
    } else {
        //SSR_ThrowException(EXTI_INTERRUPT_UNKNOWN)
        return;
    }
    Msg1.message = NM_EXTINT;
    Msg1.data2 = IO_GetExtendedIT(pin);
    Msg1.tag = Msg1.data1;	// <<<<<<<< ATENÇÃO: SERÁ RETIRADO <<<<<<<
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_EXTI15_10_IRQHandler(){
    NMESSAGE Msg1 = {NM_EXTINT, 0, 0, 0};
    uint32_t pin = -1;

    if(EXTI->PR & EXTI_PR_PR10){
        EXTI->PR |= EXTI_PR_PR10; pin = 10; Msg1.data1 = NV_EXTINT10; pin=10;
    } else if(EXTI->PR & EXTI_PR_PR11){
        EXTI->PR |= EXTI_PR_PR11; pin = 11; Msg1.data1 = NV_EXTINT11; pin=11;
    } else if(EXTI->PR & EXTI_PR_PR12){
        EXTI->PR |= EXTI_PR_PR12; pin = 12; Msg1.data1 = NV_EXTINT12; pin=12;
    } else if(EXTI->PR & EXTI_PR_PR13){ 
        EXTI->PR |= EXTI_PR_PR13; pin = 13; Msg1.data1 = NV_EXTINT13; pin=13;
    } else if(EXTI->PR & EXTI_PR_PR14){ 
        EXTI->PR |= EXTI_PR_PR14; pin = 14; Msg1.data1 = NV_EXTINT14; pin=14;
    } else if(EXTI->PR & EXTI_PR_PR15){ 
        EXTI->PR |= EXTI_PR_PR15; pin = 15; Msg1.data1 = NV_EXTINT15; pin=15;
    } else { 
      //SSR_ThrowException(EXTI_INTERRUPT_UNKNOWN)
      return;
    }
	
    Msg1.message = NM_EXTINT;
    Msg1.data2 = IO_GetExtendedIT(pin);
    Msg1.tag = Msg1.data1; // <<<<<<<< ATENÇÃO: SERÁ RETIRADO <<<<<<<
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
void EDROS_WWDG_IRQHandler(){}
void EDROS_PVD_IRQHandler(){}
void EDROS_TAMPER_IRQHandler(){}
void EDROS_RTC_IRQHandler(){}
void EDROS_FLASH_IRQHandler(){}
void EDROS_RCC_IRQHandler(){}

void EDROS_RTCAlarm_IRQHandler(){}
void EDROS_OTG_FS_WKUP_IRQHandler(){}
void EDROS_ETH_IRQHandler(){}
void EDROS_ETH_WKUP_IRQHandler(){}
void EDROS_OTG_FS_IRQHandler(){}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_ADC1_2_IRQHandler(){
	NMESSAGE Msg1 = {NM_EXTINT, 0, 0, 0};

    if(ADC1->SR & ADC_SR_EOC){
        ADC1->SR &= ~ADC_SR_EOC; Msg1.message = NM_ADCEOC; Msg1.data1 = NV_ADC1;
    } else if(ADC2->SR & ADC_SR_EOC){
        ADC2->SR &= ~ADC_SR_EOC; Msg1.message = NM_ADCEOC; Msg1.data1 = NV_ADC2;
    } else if(ADC1->SR & ADC_SR_JEOC){
        ADC1->SR &= ~ADC_SR_JEOC; Msg1.message = NM_ADCJEOC; Msg1.data1 = NV_ADC1;
    } else if(ADC2->SR & ADC_SR_JEOC){
        ADC2->SR &= ~ADC_SR_JEOC; Msg1.message = NM_ADCJEOC; Msg1.data1 = NV_ADC2;
    } else if(ADC1->SR & ADC_SR_AWD){
        ADC1->SR &= ~ADC_SR_AWD; Msg1.message = NM_ADCAWD; Msg1.data1 = NV_ADC1;
    } else if(ADC2->SR & ADC_SR_AWD){
        ADC2->SR &= ~ADC_SR_AWD; Msg1.message = NM_ADCAWD; Msg1.data1 = NV_ADC2;
    }
	SYS->Dispatch(&Msg1);
    if(ADC1->SR & ADC_SR_STRT){ ADC1->SR &= ~ADC_SR_STRT;}
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_TIM1_UP_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    TIM1->SR &= ~TIM_SR_UIF;
	Msg1.message = NM_HARDTICK;
    Msg1.data2 = (uint32_t)TIM1;
	Msg1.data1 = NV_TIM1;
	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
void EDROS_TIM1_BRK_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    TIM1->SR &= ~TIM_SR_BIF;
    Msg1.message = NM_HDTIM_BRK;
    Msg1.data2 = (uint32_t)TIM1;
	Msg1.data1 = NV_TIM1;
	SYS->Dispatch(&Msg1);
	return;
}
//------------------------------------------------------------------------------
void EDROS_TIM1_TRG_COM_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    if(TIM1->SR & TIM_SR_TIF){
        TIM1->SR &= ~TIM_SR_TIF; Msg1.message = NM_HDTIM_TRG;
    } else if(TIM1->SR & TIM_SR_COMIF){
        TIM1->SR &= ~TIM_SR_COMIF; Msg1.message = NM_HDTIM_COM;
    }
    Msg1.message = NM_NULL;
    Msg1.data2 = (uint32_t)TIM1;
	Msg1.data1 = NV_TIM1;
	SYS->Dispatch(&Msg1);
	return;
}
//------------------------------------------------------------------------------
void EDROS_TIM1_CC_IRQHandler(){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    if(TIM1->SR & TIM_SR_CC1IF){ TIM1->SR &= ~TIM_SR_CC1IF; Msg1.data2 = 1;}
    else if(TIM1->SR & TIM_SR_CC2IF){ TIM1->SR &= ~TIM_SR_CC2IF; Msg1.data2 = 2;}
    else if(TIM1->SR & TIM_SR_CC3IF){ TIM1->SR &= ~TIM_SR_CC3IF; Msg1.data2 = 3;}
    else if(TIM1->SR & TIM_SR_CC4IF){ TIM1->SR &= ~TIM_SR_CC4IF; Msg1.data2 = 4;}
    else {
        Msg1.message = NM_HDTIM_CCO;
        if(TIM1->SR & TIM_SR_CC1OF){ TIM1->SR &= ~TIM_SR_CC1OF; Msg1.data2 = 1;}
        else if(TIM1->SR & TIM_SR_CC2OF){ TIM1->SR &= ~TIM_SR_CC2OF; Msg1.data2 = 2;}
        else if(TIM1->SR & TIM_SR_CC3OF){ TIM1->SR &= ~TIM_SR_CC3OF; Msg1.data2 = 3;}
        else if(TIM1->SR & TIM_SR_CC4OF){ TIM1->SR &= ~TIM_SR_CC4OF; Msg1.data2 = 4;}
        else { TIM1->SR = 0; Msg1.message = NM_NULL;}
    }
    Msg1.message = NM_HDTIM_CC;
	Msg1.data1 = NV_TIM1;
    Msg1.tag = (uint32_t)TIM1;
	SYS->Dispatch(&Msg1);
	//----------------------------------------
	return;
}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_TIM2_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

	Msg1.data1 = NV_TIM2;
    Msg1.tag = (uint32_t)TIM2;

    if(TIM2->SR & TIM_SR_UIF){
    	TIM2->SR &= ~TIM_SR_UIF;
        Msg1.message = NM_HARDTICK;
    } else {
        Msg1.message = NM_HDTIM_CC;
        if(TIM2->SR & TIM_SR_CC1IF){ TIM2->SR &= ~TIM_SR_CC1IF; Msg1.data2 = 1;}
        else if(TIM2->SR & TIM_SR_CC2IF){ TIM2->SR &= ~TIM_SR_CC2IF; Msg1.data2 = 2;}
        else if(TIM2->SR & TIM_SR_CC3IF){ TIM2->SR &= ~TIM_SR_CC3IF; Msg1.data2 = 3;}
        else if(TIM2->SR & TIM_SR_CC4IF){ TIM2->SR &= ~TIM_SR_CC4IF; Msg1.data2 = 4;}
        else {
            Msg1.message = NM_HDTIM_CCO;
            if(TIM2->SR & TIM_SR_CC1OF){ TIM2->SR &= ~TIM_SR_CC1OF; Msg1.data2 = 1;}
            else if(TIM2->SR & TIM_SR_CC2OF){ TIM2->SR &= ~TIM_SR_CC2OF; Msg1.data2 = 2;}
            else if(TIM2->SR & TIM_SR_CC3OF){ TIM2->SR &= ~TIM_SR_CC3OF; Msg1.data2 = 3;}
            else if(TIM2->SR & TIM_SR_CC4OF){ TIM2->SR &= ~TIM_SR_CC4OF; Msg1.data2 = 4;}
            else { TIM2->SR = 0; Msg1.message = NM_NULL;}
        }
    }

    SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
extern "C" {
void EDROS_TIM3_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    if(TIM3->SR & TIM_SR_UIF){ TIM3->SR &= ~TIM_SR_UIF;}
    else {
        Msg1.message = NM_HDTIM_CC;
        if(TIM3->SR & TIM_SR_CC1IF){ TIM3->SR &= ~TIM_SR_CC1IF; Msg1.data2 = 1;}
        else if(TIM3->SR & TIM_SR_CC2IF){ TIM3->SR &= ~TIM_SR_CC2IF; Msg1.data2 = 2;}
        else if(TIM3->SR & TIM_SR_CC3IF){ TIM3->SR &= ~TIM_SR_CC3IF; Msg1.data2 = 3;}
        else if(TIM3->SR & TIM_SR_CC4IF){ TIM3->SR &= ~TIM_SR_CC4IF; Msg1.data2 = 4;}
        else {
            Msg1.message = NM_HDTIM_CCO;
            if(TIM3->SR & TIM_SR_CC1OF){ TIM3->SR &= ~TIM_SR_CC1OF; Msg1.data2 = 1;}
            else if(TIM3->SR & TIM_SR_CC2OF){ TIM3->SR &= ~TIM_SR_CC2OF; Msg1.data2 = 2;}
            else if(TIM3->SR & TIM_SR_CC3OF){ TIM3->SR &= ~TIM_SR_CC3OF; Msg1.data2 = 3;}
            else if(TIM3->SR & TIM_SR_CC4OF){ TIM3->SR &= ~TIM_SR_CC4OF; Msg1.data2 = 4;}
            else { TIM3->SR = 0; Msg1.message = NM_NULL;}
        }
    }

    Msg1.message = NM_HARDTICK;
    Msg1.tag = (uint32_t)TIM3;
	Msg1.data1 = NV_TIM3;

	SYS->Dispatch(&Msg1);
	return;
}}

//------------------------------------------------------------------------------
#if defined(TIM4)
extern "C" {
void EDROS_TIM4_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    if(TIM4->SR & TIM_SR_UIF){ TIM4->SR &= ~TIM_SR_UIF;}
    else {
        Msg1.message = NM_HDTIM_CC;
        if(TIM4->SR & TIM_SR_CC1IF){ TIM4->SR &= ~TIM_SR_CC1IF; Msg1.data2 = 1;}
        else if(TIM4->SR & TIM_SR_CC2IF){ TIM4->SR &= ~TIM_SR_CC2IF; Msg1.data2 = 2;}
        else if(TIM4->SR & TIM_SR_CC3IF){ TIM4->SR &= ~TIM_SR_CC3IF; Msg1.data2 = 3;}
        else if(TIM4->SR & TIM_SR_CC4IF){ TIM4->SR &= ~TIM_SR_CC4IF; Msg1.data2 = 4;}
        else {
            Msg1.message = NM_HDTIM_CCO;
            if(TIM4->SR & TIM_SR_CC1OF){ TIM4->SR &= ~TIM_SR_CC1OF; Msg1.data2 = 1;}
            else if(TIM4->SR & TIM_SR_CC2OF){ TIM4->SR &= ~TIM_SR_CC2OF; Msg1.data2 = 2;}
            else if(TIM4->SR & TIM_SR_CC3OF){ TIM4->SR &= ~TIM_SR_CC3OF; Msg1.data2 = 3;}
            else if(TIM4->SR & TIM_SR_CC4OF){ TIM4->SR &= ~TIM_SR_CC4OF; Msg1.data2 = 4;}
            else { TIM4->SR = 0; Msg1.message = NM_NULL;}
        }
    }

    Msg1.message = NM_HARDTICK;
	Msg1.data1 = NV_TIM4;
    Msg1.tag = (uint32_t)TIM4;
	SYS->Dispatch(&Msg1);
	return;
}}
#endif
//------------------------------------------------------------------------------
#if defined(TIM5)
extern "C" {
void EDROS_TIM5_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    if(TIM5->SR & TIM_SR_UIF){ TIM5->SR &= ~TIM_SR_UIF;}
    else {
        Msg1.message = NM_HDTIM_CC;
        if(TIM5->SR & TIM_SR_CC1IF){ TIM5->SR &= ~TIM_SR_CC1IF; Msg1.data2 = 1;}
        else if(TIM5->SR & TIM_SR_CC2IF){ TIM5->SR &= ~TIM_SR_CC2IF; Msg1.data2 = 2;}
        else if(TIM5->SR & TIM_SR_CC3IF){ TIM5->SR &= ~TIM_SR_CC3IF; Msg1.data2 = 3;}
        else if(TIM5->SR & TIM_SR_CC4IF){ TIM5->SR &= ~TIM_SR_CC4IF; Msg1.data2 = 4;}
        else {
            Msg1.message = NM_HDTIM_CCO;
            if(TIM5->SR & TIM_SR_CC1OF){ TIM5->SR &= ~TIM_SR_CC1OF; Msg1.data2 = 1;}
            else if(TIM5->SR & TIM_SR_CC2OF){ TIM5->SR &= ~TIM_SR_CC2OF; Msg1.data2 = 2;}
            else if(TIM5->SR & TIM_SR_CC3OF){ TIM5->SR &= ~TIM_SR_CC3OF; Msg1.data2 = 3;}
            else if(TIM5->SR & TIM_SR_CC4OF){ TIM5->SR &= ~TIM_SR_CC4OF; Msg1.data2 = 4;}
            else { TIM5->SR = 0; Msg1.message = NM_NULL;}
        }
    }
    Msg1.message = NM_HARDTICK;
	Msg1.data1 = NV_TIM5;
    Msg1.tag = (uint32_t)TIM5;
	SYS->Dispatch(&Msg1);
	return;
}}
#endif

//------------------------------------------------------------------------------
#if defined(TIM6)
extern "C" {
void EDROS_TIM6_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

    TIM6->SR &= ~TIM_SR_UIF;
    Msg1.message = NM_HARDTICK;
    Msg1.tag = (uint32_t)TIM6;
	Msg1.data1 = NV_TIM6;
	SYS->Dispatch(&Msg1);
	return;	
}}
#endif

//------------------------------------------------------------------------------
#if defined(TIM7)
extern "C" {
void EDROS_TIM7_IRQHandler(void){
	NMESSAGE Msg1 = {NM_NULL, 0, 0, 0};

	TIM7->SR &= ~TIM_SR_UIF;
    Msg1.message = NM_HARDTICK;
    Msg1.tag = (uint32_t)TIM7;
	Msg1.data1 = NV_TIM7;
	SYS->Dispatch(&Msg1);
	return;	
}}
#endif

//------------------------------------------------------------------------------
void RelocateVectors(){

	__disable_irq();
	//--------------------------------------------------------
	// Core: vector table relocation
	SSR_Relocate(EDROS_TotalIrqs);
	SSR_Allocate((uint32_t)EDROS_HardFault_Handler,        0x03);     //0x0000_000C
    SSR_Allocate((uint32_t)EDROS_MemManage_Handler,        0x04);     //0x0000_0010
    SSR_Allocate((uint32_t)EDROS_BusFault_Handler,         0x05);     //0x0000_0014
    SSR_Allocate((uint32_t)EDROS_UsageFault_Handler,       0x06);     //0x0000_0018
	SSR_Allocate((uint32_t)EDROS_SVC_Handler,              0x0B);     //0x0000_002C
    SSR_Allocate((uint32_t)EDROS_DebugMon_Handler,         0x0C);     //0x0000_0030
	SSR_Allocate((uint32_t)EDROS_PendSV_Handler,           0x0E);     //0x0000_0038
	SSR_Allocate((uint32_t)EDROS_SysTick_Handler,          0x0F);     //0x0000_003C

	//--------------------------------------------------------
	// Peripherals: vector table relocation
    SSR_Allocate((uint32_t)EDROS_WWDG_IRQHandler,          0x10);     //0x0000_0040
    SSR_Allocate((uint32_t)EDROS_PVD_IRQHandler,           0x11);     //0x0000_0044
    SSR_Allocate((uint32_t)EDROS_TAMPER_IRQHandler,        0x12);     //0x0000_0048
    SSR_Allocate((uint32_t)EDROS_RTC_IRQHandler,           0x13);     //0x0000_004C
    SSR_Allocate((uint32_t)EDROS_FLASH_IRQHandler,         0x14);     //0x0000_0050
	SSR_Allocate((uint32_t)EDROS_RCC_IRQHandler,           0x15);     //0x0000_0054
	SSR_Allocate((uint32_t)EDROS_EXTI0_IRQHandler,         0x16);     //0x0000_0058
    SSR_Allocate((uint32_t)EDROS_EXTI1_IRQHandler,         0x17);     //0x0000_005C
    SSR_Allocate((uint32_t)EDROS_EXTI2_IRQHandler,         0x18);     //0x0000_0060
    SSR_Allocate((uint32_t)EDROS_EXTI3_IRQHandler,         0x19);     //0x0000_0064
    SSR_Allocate((uint32_t)EDROS_EXTI4_IRQHandler,         0x1A);     //0x0000_0068

	//--------------------------------------------------------
	SSR_Allocate((uint32_t)EDROS_DMA1_Channel1_IRQHandler, 0x1B);     //0x0000_006C
	SSR_Allocate((uint32_t)EDROS_DMA1_Channel2_IRQHandler, 0x1C);     //0x0000_0070
	SSR_Allocate((uint32_t)EDROS_DMA1_Channel3_IRQHandler, 0x1D);     //0x0000_0074
    SSR_Allocate((uint32_t)EDROS_DMA1_Channel4_IRQHandler, 0x1E);     //0x0000_0078
    SSR_Allocate((uint32_t)EDROS_DMA1_Channel5_IRQHandler, 0x1F);     //0x0000_007C
    SSR_Allocate((uint32_t)EDROS_DMA1_Channel6_IRQHandler, 0x20);     //0x0000_0080
    SSR_Allocate((uint32_t)EDROS_DMA1_Channel7_IRQHandler, 0x21);     //0x0000_0084
	SSR_Allocate((uint32_t)EDROS_ADC1_2_IRQHandler,        0x22);     //0x0000_0088
	SSR_Allocate((uint32_t)EDROS_CAN1_TX_IRQHandler,       0x23);     //0x0000_008C
    SSR_Allocate((uint32_t)EDROS_CAN1_RX0_IRQHandler,      0x24);     //0x0000_0090
    SSR_Allocate((uint32_t)EDROS_CAN1_RX1_IRQHandler,      0x25);     //0x0000_0094
    SSR_Allocate((uint32_t)EDROS_CAN1_SCE_IRQHandler,      0x26);     //0x0000_0098
	//--------------------------------------------------------
    
	SSR_Allocate((uint32_t)EDROS_EXTI9_5_IRQHandler,       0x27);     //0x0000_009C
    SSR_Allocate((uint32_t)EDROS_TIM1_BRK_IRQHandler,      0x28);     //0x0000_00A0
    SSR_Allocate((uint32_t)EDROS_TIM1_UP_IRQHandler,       0x29);     //0x0000_00A4
    SSR_Allocate((uint32_t)EDROS_TIM1_TRG_COM_IRQHandler,  0x2A);     //0x0000_00A8
    SSR_Allocate((uint32_t)EDROS_TIM1_CC_IRQHandler,       0x2B);     //0x0000_00AC
    SSR_Allocate((uint32_t)EDROS_TIM2_IRQHandler,          0x2C);     //0x0000_00B0
    SSR_Allocate((uint32_t)EDROS_TIM3_IRQHandler,          0x2D);     //0x0000_00B4
	#if defined(TIM4)
    SSR_Allocate((uint32_t)EDROS_TIM4_IRQHandler,          0x2E);     //0x0000_00B8
	#endif
    SSR_Allocate((uint32_t)EDROS_I2C1_EV_IRQHandler,       0x2F);     //0x0000_00BC
    SSR_Allocate((uint32_t)EDROS_I2C1_ER_IRQHandler,       0x30);     //0x0000_00C0
	#if defined(I2C2)
    SSR_Allocate((uint32_t)EDROS_I2C2_EV_IRQHandler,       0x31);     //0x0000_00C4
    SSR_Allocate((uint32_t)EDROS_I2C2_ER_IRQHandler,       0x32);     //0x0000_00C8
	#endif
    SSR_Allocate((uint32_t)EDROS_SPI1_IRQHandler,          0x33);     //0x0000_00CC
	#if defined(SPI2)
    SSR_Allocate((uint32_t)EDROS_SPI2_IRQHandler,          0x34);     //0x0000_00D0
	#endif
    SSR_Allocate((uint32_t)EDROS_USART1_IRQHandler,        0x35);     //0x0000_00D4
    SSR_Allocate((uint32_t)EDROS_USART2_IRQHandler,        0x36);     //0x0000_00D8
	#if defined(USART3)
    SSR_Allocate((uint32_t)EDROS_USART3_IRQHandler,        0x37);     //0x0000_00DC
	#endif
    SSR_Allocate((uint32_t)EDROS_EXTI15_10_IRQHandler,     0x38);     //0x0000_00E0
    SSR_Allocate((uint32_t)EDROS_RTCAlarm_IRQHandler,      0x39);     //0x0000_00E4
    SSR_Allocate((uint32_t)EDROS_OTG_FS_WKUP_IRQHandler,   0x3A);     //0x0000_00E8
    
    
	#if defined(TIM5)
    SSR_Allocate((uint32_t)EDROS_TIM5_IRQHandler,         0x42);      //0x0000_0108
	#endif
	#if defined(SPI3)
    SSR_Allocate((uint32_t)EDROS_SPI3_IRQHandler,         0x43);      //0x0000_010C
	#endif
	#if defined(UART4)
    SSR_Allocate((uint32_t)EDROS_UART4_IRQHandler,        0x44);      //0x0000_0110
	#endif
	#if defined(UART5)
    SSR_Allocate((uint32_t)EDROS_UART5_IRQHandler,        0x45);      //0x0000_0114
	#endif
	#if defined(TIM6)
    SSR_Allocate((uint32_t)EDROS_TIM6_IRQHandler,         0x46);      //0x0000_0118
	#endif
	#if defined(TIM7)
    SSR_Allocate((uint32_t)EDROS_TIM7_IRQHandler,         0x47);      //0x0000_011C
	#endif
	#if defined(DMA2)
    SSR_Allocate((uint32_t)EDROS_DMA2_Channel1_IRQHandler,0x48);      //0x0000_0120
    SSR_Allocate((uint32_t)EDROS_DMA2_Channel2_IRQHandler,0x49);      //0x0000_0124
    SSR_Allocate((uint32_t)EDROS_DMA2_Channel3_IRQHandler,0x4A);      //0x0000_0128
    SSR_Allocate((uint32_t)EDROS_DMA2_Channel4_IRQHandler,0x4B);      //0x0000_012C
    SSR_Allocate((uint32_t)EDROS_DMA2_Channel5_IRQHandler,0x4C);      //0x0000_0130
	#endif
    
	#if defined (STM32F105xC) || defined (STM32F107xC)
    SSR_Allocate((uint32_t)EDROS_ETH_IRQHandler,          0x4D);      //0x0000_0134
    SSR_Allocate((uint32_t)EDROS_ETH_WKUP_IRQHandler,     0x4E);      //0x0000_0138
    SSR_Allocate((uint32_t)EDROS_CAN2_TX_IRQHandler,      0x4F);      //0x0000_013C
    SSR_Allocate((uint32_t)EDROS_CAN2_RX0_IRQHandler,     0x50);      //0x0000_0140
    SSR_Allocate((uint32_t)EDROS_CAN2_RX1_IRQHandler,     0x51);      //0x0000_0144
    SSR_Allocate((uint32_t)EDROS_CAN2_SCE_IRQHandler,     0x52);      //0x0000_0148
    SSR_Allocate((uint32_t)EDROS_OTG_FS_IRQHandler,       0x53);      //0x0000_014C
    #endif
    
    __enable_irq();
}

//==============================================================================

