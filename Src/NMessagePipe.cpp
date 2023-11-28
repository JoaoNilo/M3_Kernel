//==============================================================================
#include "System.h"

//------------------------------------------------------------------------------
NMessagePipe::NMessagePipe(){
    fila = new NFifo(sizeof(NMESSAGE), __SYS_STANDARD_MESSAGES);
    filah = new NFifo(sizeof(NMESSAGE), __SYS_PRIORITY_MESSAGES);

    //---------------------------------------
    objects_number = 0L; comp = NULL;

    //---------------------------------------
    for(int i=0L; i<__SYS_MAX_OBJECTS; i++) sysObjects[i] = 0L;
}

//------------------------------------------------------------------------------
NMessagePipe::~NMessagePipe(){
    delete(fila);
    delete(filah);
}

//------------------------------------------------------------------------------
void NMessagePipe::Insert(NMESSAGE* Msg){
    if(Msg->message > __SYS_PRIORITY_BORDERLINE) filah->Put((uint8_t*)Msg, sizeof(NMESSAGE));
    else fila->Put((uint8_t*)Msg, sizeof(NMESSAGE));
}

//------------------------------------------------------------------------------
uint32_t NMessagePipe::Dispatch(){
    uint32_t index, n=0L;

    //-----------------------------------------
    while(filah->Counter() > 0){
        filah->Get((uint8_t*)&Message);
        for(index=0L; index< objects_number; index++){
            BkMessage = Message;
            comp = (NComponent*)(sysObjects[index]);
            comp->Notify(&BkMessage);
            if(BkMessage.message != NM_NULL){
                Insert(&BkMessage);
            }
        }
        n++;
    }

    //-----------------------------------------

    while(fila->Counter()>0){
        fila->Get((uint8_t*)&Message);
        for(index=0L; index<objects_number; index++){
            BkMessage = Message;
            comp = (NComponent*)(sysObjects[index]);
            comp->Notify(&BkMessage);
            if(BkMessage.message != NM_NULL){
                if(BkMessage.message != NM_EXTINGUISH){
                    Insert(&BkMessage);
                } else index = objects_number;
            }
        }
        n++;
    }
    return(n);
    //-------------------------------------
}

//------------------------------------------------------------------------------
bool NMessagePipe::IncludeComponent(HANDLE newcomp){
  	bool result = false;
    if(newcomp != NULL){
        //------------------------------------
	  	if(objects_number<__SYS_MAX_OBJECTS){
        	sysObjects[objects_number++] = newcomp; result = true;
		}
        //------------------------------------
    }
    return(result);
}

//------------------------------------------------------------------------------
uint32_t NMessagePipe::FindComponent(HANDLE fcomp){
  	uint32_t result = __SYS_INDEX_INVALID; uint32_t c= 0;
    if(fcomp != NULL){
        //------------------------------------
	  	while((result == __SYS_INDEX_INVALID)&&(c < objects_number)){
        	if(sysObjects[c] == fcomp){ result = c;} else { c++;}
		}
        //------------------------------------
    }
    return(result);
}

//------------------------------------------------------------------------------
bool NMessagePipe::ExcludeComponent(HANDLE xcomp){
  	bool result = false;
	uint32_t index = FindComponent(xcomp);

   	if(index < objects_number){
        //------------------------------------
		if(objects_number > 1){
			objects_number--;
			if(index == objects_number){ sysObjects[index]=NULL;}
			else{
				for(uint32_t c = index; c < objects_number; c++){
					sysObjects[c] = sysObjects[c+1];
				}
				sysObjects[objects_number]=NULL;
			}
		} else { sysObjects[0]=NULL; objects_number--;}
		result = true;
        //------------------------------------
    }
    return(result);
}

//==============================================================================
