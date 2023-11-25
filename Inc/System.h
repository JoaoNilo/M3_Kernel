//==============================================================================
/** @file System.h
 *  @brief EDROS Framework System class.
 *  - This class provides the basic functionalities for the framework.
 *  @version 1.0.0
 *  @author Joao Nilo Rodrigues - nilo@pobox.com
 *
 *------------------------------------------------------------------------------
 *
 * <h2><center>&copy; Copyright (c) 2020 Joao Nilo Rodrigues
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by "Joao Nilo Rodrigues" under BSD 3-Clause
 * license, the "License".
 * You may not use this file except in compliance with the License.
 *               You may obtain a copy of the License at:
 *                 opensource.org/licenses/BSD-3-Clause
 */
//------------------------------------------------------------------------------
#ifndef SYSTEM_H
    #define SYSTEM_H

    #include "NMessagePipe.h"

//------------------------------------------------------------------------------
#define __SYS_STANDARD_MESSAGES 	((uint32_t) 4)
#define __SYS_PRIORITY_MESSAGES 	((uint32_t) 4)
#define __SYS_STANDARD_CALLBACKS 	((uint32_t) 4)
#define __SYS_TICK_RATE 			((uint32_t) 1)
#define __SYS_SCAN_RATE 			((uint32_t) 10)
#define __SYS_UPDATE_RATE 			((uint32_t) 20)
#define __SYS_PRIORITY_BORDERLINE 	((uint32_t) 0xFFFF0000)
#define __SYS_INDEX_INVALID 		((uint32_t) 0xFFFFFFFF)

//------------------------------------------------------------------------------
/** @brief EDROS System class.
 * @warning This class must be used exclusively by the system kernel.
 */
class System{
	public:
		//----------------------------------------------------------------------
		/**
		 * @struct SIGNAL
		 * Signal parameters definition.
		 */
		struct SIGNAL{
			public:
				void* flag;		//!< pointer to a boolean flag to receive the state changes
				int counter;	//!< internal counter
				SIGNAL(){ flag = NULL;  counter = -1;}
		};

    private:
	  	bool halt;
		bool sleep;
        uint32_t time;

        uint16_t fclk;
        uint16_t ticks_timers;
        uint16_t ticks_inputs;
        uint16_t ticks_outputs;
        uint32_t ksc0, ksc1, ksc2;

        uint32_t UpdateTimeouts();
		void UpdatePowerdown();

        HANDLE sysVectors[__SYS_MAX_VECTORS];
        SIGNAL sysSignals[__SYS_MAX_SIGNALS];

    public:
    NMessagePipe* queue;
	NFifo* CallbackQueue;

	public:
		/**
		 * @brief Standard constructor for this class.
		 */
        System();

        /**
         * @brief Standard destructor for this component.
         */
        ~System();

        /**
         * @brief This method halts the message dispatcher by disabling the SysTick timer.
         * @warning
         * - This method MUST not be called by the application.
         */
		void Halt();

        /**
         * @brief This method resumes the message dispatching by enabling the SysTick timer.
         * @warning
         * - This method MUST not be called by the application.
         */
		void Resume();

        /**
         * @brief This method checks the status of the message dispatcher.
         * @return true if the dispatcher is stopped (halt).
         */
		bool IsHalt();

        /**
         * @brief This method is used to initialize all the system internal variables.
         * It is also used to allocate some system buffers.
         * For some unknown reason this could not be done from the constructor.
         * @warning
         * - This method MUST not be called by the application.
         */
        void Initialize();

        /**
         * @brief This method starts the execution of the "system thread"
         * @warning
         * - This method MUST not be called by the application.
         */
        void Execute();

        /**
         * @brief This method prompts the system to enter "power-saving" mode.
         * @arg Stat:
         * - true: the low-power mode (sleep) is activated as soon as the system dispatcher
         * finishes the current round of messages dispatch.
         * Sleep mode will remain until the next hardware interrupt "awakes" it.
         * - false: the low-power mode (sleep) is deactivated.
         *
         * @note As the SysTick timer is not disabled, the execution of the
         * system messages dispatcher is not compromised, with the sleep mode being
         * activated between each dispatch round.
         */
        void Sleep(bool Stat);

        /**
         * @brief This method check the status of the "power-saving" mode flag.
         * @return true if the "power-saving mode" (sleep) flag is ON.
         */
		bool IsSleeping();

        /**
         * @brief This method controls the system internal timers.
         * - Many kernel process are associated with these internal timers.
         * @warning
         * - This method MUST not be called by the application.
         */
        void Heartbeat();

        /**
         * @brief This method is used to request the inclusion of a particular component in the system notification table.
         * @arg iComp:
         * The component handle to be included.
         * @return
         * - true: the component was successfully included.
         * - false: component not included. Probably @ref __SYS_MAX_OBJECTS was reached.
         * @note
         * - iComp MUST be descendant of NCompenent!
         */
        bool IncludeComponent(HANDLE iComp);

        /**
         * @brief This method is used to request the exclusion of a particular component from the system notification table.
         * @arg xComp:
         * The component handle to be excluded.
         * @return
         * - true: the component was successfully excluded.
         * - false: component not excluded, probably not found.
         * @note
         * - iComp MUST be descendant of NCompenent!
         */
		bool ExcludeComponent(HANDLE xComp);

        /**
         * @brief This method is used to verify if a particular component is registered in the system notification table.
         * @arg fComp:
         * The component handle to be verified.
         * @return
         * - true: the component is registered in the table.
         * - false: component not found.
         * @note
         * - iComp MUST be descendant of NCompenent!
         */
		bool FindComponent(HANDLE fComp);

        /**
         * @brief Registers a given component in the system "hardware interrupt" notification table.
         * @arg hComp:
         * The component handle to be registered.
         * @arg vComp:
         * The component "vector index".
         * @return
         * - NULL, if no component was registered with this "vector index", or
         * - handle of the previously registered component.
         * @note
         * - hComp MUST be descendant of NCompenent!
         */
        HANDLE InstallCallback(HANDLE hComp, NV_ID vComp);

        /**
         * @brief Retrieves the component handle registered in the system "hardware interrupt" notification table
         * with a particular vector index.
         * @arg vComp:
         * The requested "vector index", which should be a valid @ref NV_ID.
         * @return
         * - NULL, if no component was registered with this "vector index", or
         * - handle of the registered component.
         */
        HANDLE GetCallback(NV_ID vComp);

        /**
         * @brief This method inserts a message in the system "callback notification queue" and
         * activates the scheduler to dispatch the message when the system go idle, i.e.
         * no interrupt service being executed.
         * @arg Msg:
         * The pointer to a @ref NMESSAGE data struct to be scheduled.
         *
         * <b> Message Rules </b>
         * - Field "data1" must contain the "vector index" of the component to be notified.
         */
		void CallbackSchedule(NMESSAGE* Msg);

        /**
         * @brief This method extracts a message from system "callback notification queue".
         * @arg Msg:
         * The pointer to a @ref NMESSAGE data struct to be receive the extracted message.
         *
         * @warning
         * - This method MUST not be called by the application.
         */
		bool CallbackAttend(NMESSAGE* Msg);

        /**
         * @brief This method releases the system "callback notification queue".
         *
         * @warning
         * - This method MUST not be called by the application.
         */
		void CallbackAttended();

        /**
         * @brief This method sends a message a specific component.
         * @arg Msg:
         * The pointer to a @ref NMESSAGE data struct to be sent.
         *
         * <b> Message Rules </b>
         * The message fields should be assigned as follows:
         * - message: NM_DMA_OK, NM_USART_RX, NM_ADC_OK, etc.
         * - data1: vector id (NV_ID). As seen before, every peripheral has its own @ref NV_ID.
         * - data2: context dependent (DMA channel number, status register, data register, etc.)
         * - tag: rarely used.
         *
         * @note
         * - The components are notified by the InterruptCallback method.
         */
		void Dispatch(NMESSAGE* Msg);
	
        /**
         * @brief This method creates a "blocking" local timer
         * @arg flag: pointer to the flag to receive the status changes
         * @arg delay: the time interval to wait, in milliseconds.
         * @return true if successfully created
         *
         * @note
         * - This method is deprecated. Consider using @ref Delay.
         */
        bool InstallTimeout(void* flag, uint32_t delay);

        /**
         * @brief This method is used to get the system ticks counter.
         * The ticks counter is incremented every milliseconds,
         * resulting in a 49 days capacity (before the counter overlaps).
         * @return current value of the ticks counter.
         */
		uint32_t GetSystemTime();

        /**
         * @brief This method returns a pseudo-random 8-bit number.
         * @return random 8-bit number
         */
		uint8_t GetRandomNumber();

        /**
         * @brief This method returns a pseudo-random code.
         * @return random number in the range of 1 to 254. (0x01 to 0xFE)
         */
		uint8_t GetKSCode();

        //uint32_t SetClockFactors();

        /**
         * @brief This method gets the number of microseconds since the system startup.
         * Due to hardware limitations, this method is not very accurate.
         * @return current number of microseconds.
         */
        uint32_t Microseconds();

        /**
         * @brief This method freezes the execution for the specified number of milliseconds.
         * This method should be avoided as much as possible, as it makes the application less efficient,
         * both in terms of responsiveness and energy consumption.
         * @arg Tms: time to wait, in milliseconds.
         */
        void Delay(uint32_t Tms);

        /**
         * @brief This method freezes the execution for the specified number of microseconds.
         * The excessive use of this method should be avoided as, as mentioned in @ref Delay,
         * significantly compromises efficiency.
         * @arg Tus: time to wait, in microseconds.
         */
        void MicroDelay(uint32_t Tus);
        
        void (*AppStart)(void);
        void (*AppException)(uint32_t);
};

//------------------------------------------------------------------------------
int main(void);

//------------------------------------------------------------------------------
extern volatile uint16_t error;
extern System* SYS;
extern void ApplicationCreate(void);
extern void ApplicationException(uint32_t);

#endif
//==============================================================================
