INT pop_item_rn(PCHAR buffer, INT len, PCHAR item){

	for(INT i=0;i<(len);i++){
		item[i] = buffer[i];
		if(buffer[i] == '\r' && buffer[i+1] == '\n'){
			item[i] = NULL;
			item[i+i] = NULL;

			strcpy(buffer,buffer+i+2);
			return (i+2);
		}
	}
	return 0;
}

BOOL PopItem(PCHAR RecvBuf, INT Length, PCHAR PoppedItem, PCHAR Delimiter, INT DSize){
	for(INT i=0; i<Length; i++){
		PoppedItem[i] = RecvBuf[i];
		for(INT i2=0; i2<=DSize; i2++){
			if(RecvBuf[i+i2] != Delimiter[i2]){
				i2 = DSize+1;
				break;
			}
			if(i2 == (DSize-1)){
				memset(PoppedItem+i,0,DSize);
				memcpy(RecvBuf,RecvBuf+i+DSize,1024);
				return (i+DSize);
			}
		}
	}
	return 0;
}