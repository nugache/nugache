#include "wildcard.h"

BOOL WildcardCompare(PCHAR Wildcard, PCHAR String){
	PCHAR CP, MP;
	while((*String) && (*Wildcard != '*')){
		if((*Wildcard != *String) && (*Wildcard != '?'))
			return FALSE;
		Wildcard++;
		String++;
	}
	while(*String){
		if(*Wildcard == '*'){
			if(!*++Wildcard)
				return TRUE;
			MP = Wildcard;
			CP = String + 1;
		}else
		if((*Wildcard == *String) || (*Wildcard == '?')){
			Wildcard++;
			String++;
		}else{
			Wildcard = MP;
			String = CP++;
        }
    }
	while(*Wildcard == '*')
		Wildcard++;
    return !*Wildcard;
}