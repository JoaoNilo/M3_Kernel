//==============================================================================
/**
 * @file NMessagePipe.h
 * @brief EDROS system messages manager\n
 * This class provides the message queues for the kernel.\n
 * @version 1.0.0
 * @author Joao Nilo Rodrigues - nilo@pobox.com
 * @warning This class must be used exclusively by the system kernel.
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
#ifndef NMESSAGEPIPE_H
    #define NMESSAGEPIPE_H

    #include "NFifo.h"
    #include "NComponent.h"

    //------------------------------------------------
	/** @brief EDROS system messages manager.
	 * @warning This class must be used exclusively by the system kernel.
 	 */
    class NMessagePipe{
        private:
            NFifo* fila;
            NFifo* filah;
            NComponent* comp;

            uint32_t objects_number;

            HANDLE sysObjects[__SYS_MAX_OBJECTS];

            NMESSAGE    Message;
            NMESSAGE    BkMessage;

    public:
            //-------------------------------------------
            // METHODS
            /**
             * @brief Standard constructor for this class.
             */
            NMessagePipe();

            /**
             * @brief Standard destructor for this component.
             */
            ~NMessagePipe();

            /**
             * @brief This method is used to insert a message in the system queue.
             * @arg Msg
             * - pointer to the @ref NMESSAGE structure containing the message to be inserted.
             */
            void Insert(NMESSAGE* Msg);

            /**
             * @brief This method is called by the system kernel to notify the registered components
             * of queued messages.
             * @return
             * - number of messages dispatched (for each component)
             */
            uint32_t Dispatch();

            /**
             * @brief This method is used "by the kernel" to register a given component in the notification table.
             * @arg inComp
             * - the handle of the component to be registered.
             * @return
             * - true if the component was successfully registered.
             */
            bool IncludeComponent(HANDLE inComp);

            /**
             * @brief This method is used "by the kernel" to check if a particular component is registered in the notification table.
             * @arg fComp
             * - the handle of the component to be searched.
             * @return
             * - the component index, if the component is registered.
             * - __SYS_INDEX_INVALID, otherwise.
             */
			uint32_t FindComponent(HANDLE fComp);

            /**
             * @brief This method is used "by the kernel" to exclude a registered component from the notification table.
             * @arg inComp
             * - the handle of the component to be excluded.
             * @return
             * - true if the component was successfully excluded.
             */
			bool ExcludeComponent(HANDLE);
    };

#endif

//==============================================================================
