#include <xinu.h>
#include <stdio.h>

int lflistdirh(did32 diskdev, char *directorypath){
	wait(Lf_data.lf_mutex);
	bool8 found;
	found = FALSE;
	char namBuf[LF_NAME_LEN];
	dbid32 cur_dnum;
	struct ldirU buf;
	char* namPtr = directorypath+1;
	char* bufPtr = namBuf;
	struct lfiblk curIblock;
	struct lfiblk* Iptr = &curIblock;
	lfibget(Lf_data.lf_dskdev, 0, &curIblock);
	//kprintf("Struck....1111\r\n");
	while(TRUE){
		*bufPtr = *namPtr;
		//kprintf("*****%c ", *namPtr);
		bufPtr++;
		namPtr++;
		if(*namPtr == '/'||*namPtr == NULLCH){
			//kprintf("\r\n");
			*bufPtr = NULLCH;
			int32 y;
			bool8 Dir_found = FALSE;
			for(y=0; y< LF_IBLEN; y++){
				dbid32 dnum = Iptr->ib_dba[y];
				if(dnum == LF_DNULL) continue;
				if(Iptr->dir_ent == FALSE) return SYSERR;
				read(Lf_data.lf_dskdev, (char*)&buf, dnum);
				cur_dnum = dnum;
				int32 z;
				for(z = 0; z<LF_NUM_ROOT_LDENT; z++){
					if(buf.arr[z].Used == 0) continue;
					char* nam = namBuf;
					char* cmp = buf.arr[z].ld_name;
					//kprintf("%s\r\n%s\r\n", nam, cmp);
					while(*nam != NULLCH){
						if(*nam != *cmp) break;
						nam++; cmp++;
					}
					if(*nam == NULLCH && *cmp == NULLCH){
						//kprintf("%s\r\n", namBuf);
						//kprintf("%s\r\n\r\n", buf.arr[z].ld_name);
						Dir_found = TRUE;
						break;
					}
				}
				if(Dir_found == TRUE){
					lfibget(Lf_data.lf_dskdev, buf.arr[z].ld_ilist, &curIblock);
					break;
				}
			}
			if(*namPtr == NULLCH) break;
			if(Dir_found == FALSE){
				signal(Lf_data.lf_mutex);
				return SYSERR;
			}
			else
				Dir_found = FALSE;
			if(*namPtr == '/')
				namPtr++;
			bufPtr = namBuf;
		}
	}
	int32 y;
	for(y=0; y< LF_IBLEN; y++){
		dbid32 dnum = Iptr->ib_dba[y];
		if(dnum == LF_DNULL) continue;
		if(read(Lf_data.lf_dskdev, (char*)&buf, dnum) == SYSERR){
			signal(Lf_data.lf_mutex);
			return SYSERR;
		} 
		//kprintf("%d ",Iptr->ib_dba[y]);
		cur_dnum = dnum;
		//kprintf("%s\r\n", dbuff);
		int32 z;
		//kprintf("Struck....33333\r\n");
		for(z = 0; z < LF_NUM_ROOT_LDENT; z++){
			//kprintf("%d ",buf.arr[z].Used);
			if(buf.arr[z].Used == 1){ 
				kprintf("%s ", buf.arr[z].ld_name);
			}
		}
	}
	kprintf("\r\n");
	//kprintf("Struck....44444\r\n");
	signal(Lf_data.lf_mutex);
	return OK;
}
