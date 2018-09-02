#include "target.h"

Target::Target(PCHAR Format){
	for(UINT i = 0; i < 4; i++)
		Rand[i] = FALSE;
	CHAR FormatC[64];
	strncpy(FormatC, Format, sizeof(FormatC));
	if(FormatC[0] == 'R'){
		Random = atoi(&FormatC[1]);
	}else{
		Random = 0;
	}
	PCHAR Token = strtok(FormatC, ".");
	for(UINT i = 0; i < 4; i++, Min[i] = 0, Max[i] = 255);
	UINT i = 0;
	while(Token && i < 4){
		if(Token[0] == 'r'){
			Rand[i] = TRUE;
			Min[i] = 0;
			Max[i] = atoi(&Token[1]);
		}else
		if(strcmp(Token, "*") == 0){
			Min[i] = 0;
			Max[i] = 255;
		}else
		if(strstr(Token, "-")){
			Max[i] = atoi(strchr(Token, '-') + 1);
			PCHAR Null = strchr(Token, '-');
			*(Null) = NULL;
			Min[i] = atoi(Token);
		}else{
			Min[i] = Max[i] = atoi(Token);
		}
		Token = strtok(NULL, ".");
		i++;
	}
	for(UINT i = 0; i < 4; i++){
		if(Min[i] > Max[i])
			Min[i] = Max[i];
		Pos[i] = Min[i];
	}
	if(Min[3] == 0)
		Min[3] = 1;
	if(Max[3] == 255)
		Max[3] = 254;
	Octet = 0;
	Final = FALSE;
}

ULONG Target::GetNext(VOID){
	ULONG Addr = NULL;
	BOOL Done = TRUE;
	if(Random){
		UCHAR Rand[4];
		for(UINT i = 0; i < 4; i++)
			Rand[i] = rand_r(1, 255);
		while(Rand[0] == 127)
			Rand[0] = rand_r(1, 255);
		Addr = MAKELONG(MAKEWORD(Rand[0], Rand[1]), MAKEWORD(Rand[2], Rand[3]));
		Random--;
		if(!Random)
			Final = TRUE;
		return Addr;
	}
	for(INT i = 3; i >= 0; i--){
		PCHAR Array = (PCHAR)&Addr;
		if(Rand[i]){
			Array[i] = rand_r(1, 255);
		while(Array[0] == 127)
			Array[0] = rand_r(1, 255);
		}else{
			Array[i] = Pos[i];
		}
		if(Pos[i] < Max[i] && Min[i] != Max[i] && Done != FALSE){
			if(Octet != (3 - i)){
				Octet = 3 - i;
				for(INT f = 3; f > 3 - Octet; f--){
					Pos[f] = Min[f];
				}
			}
			Pos[i]++;
			Done = FALSE;
		}
	}
	if(Final)
		return NULL;
	if(Done)
		Final = TRUE;
	return Addr;
}