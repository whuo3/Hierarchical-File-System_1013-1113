/* lfsControl.c - lfsControl */

#include <xinu.h>

/*------------------------------------------------------------------------
 * lfsControl - Provide control functions for a local file pseudo-device
 *------------------------------------------------------------------------
 */
devcall	lfsControl (
	 struct dentry	*devptr,	/* entry in device switch table	*/
	 int32	func,			/* a control function		*/
	 int32	arg1,			/* argument #1			*/
	 int32	arg2			/* argument #2			*/
	)
{
	struct	lflcblk	*lfptr;		/* ptr to open file table entry	*/
	int32	retval;			/* return value from func. call	*/
	char*   name;
	/* Obtain exclusive use of the file */

	name  = (char*)arg1;
	lfptr = &lfltab[devptr->dvminor];
	wait(lfptr->lfmutex);

	/* If file is not open, return an error */

	switch (func) {

	/* Truncate a file */

	case LF_CTL_MKDIR:
		wait(Lf_data.lf_mutex);
		bool8 found;
		found = FALSE;
		char namBuf[LF_NAME_LEN];
		dbid32 cur_dnum;
		struct  ldirU buf;
		char* namPtr = name+1;
		char* bufPtr = namBuf;
		did32 diskdev = Lf_data.lf_dskdev;
		struct lfiblk curIblock;
		struct lfiblk* Iptr = &curIblock;
		lfibget(diskdev, 0, &curIblock);
		while(*namPtr != NULLCH){
			*bufPtr = *namPtr;
			bufPtr++;
			namPtr++;
			if(*namPtr == '/'){
				*bufPtr = NULLCH;
				int32 y;
				bool8 Dir_found = FALSE;
				for(y=0; y< LF_IBLEN; y++){
					dbid32 dnum = Iptr->ib_dba[y];
					if(dnum == LF_DNULL) continue;
					read(diskdev, (char *)&buf, dnum);
					cur_dnum = dnum;
					int32 z;
					for(z = 0; z<LF_NUM_ROOT_LDENT; z++){
						if(buf.arr[z].Used == 0) continue;
						char* nam = namBuf;
						char* cmp = buf.arr[z].ld_name;
						while(*nam != NULLCH){
							if(*nam != *cmp) break;
							nam++; cmp++;
						}
						if(*nam == NULLCH && *cmp == NULLCH){
							//kprintf("Match\r\n");
							Dir_found = TRUE;
							break;
						}
					}
					if(Dir_found == TRUE){
						//kprintf("**Match**\r\n");
						lfibget(diskdev, buf.arr[z].ld_ilist, &curIblock);
						break;
					}
				}
				//if Directory not found, then create the dictory.
				if(Dir_found == FALSE){
					signal(Lf_data.lf_mutex);
					signal(lfptr->lfmutex);
					return SYSERR;
				}
				else
					Dir_found = FALSE;
				namPtr++;
				bufPtr = namBuf;
			}
			if(*namPtr == NULLCH)				*bufPtr = NULLCH;
		}
		//kprintf("%s \r\n", namBuf);
		int32 y;
		for(y=0; y< LF_IBLEN; y++){
			dbid32 dnum = Iptr->ib_dba[y];
			if(dnum == LF_DNULL) continue;
			read(diskdev, (char *)&buf, dnum);
			cur_dnum = dnum;
			int32 z;
			bool8 check = FALSE;
			for(z = 0; z < LF_NUM_ROOT_LDENT; z++){
				if(buf.arr[z].Used == 0){ 
				//if condition matched, then there is available space to store the new ldentry in data block
					ibid32 newDir = lfiballoc();
					//lfibget(diskdev, newDir, Iptr);
					lfibclear(Iptr, 0);
					Iptr->dir_ent = TRUE;
					Iptr->ld_size = 0;
					int32 q;
					for(q=0; q<LF_IBLEN ;q++){
						struct ldirU Tbuf;
						dbid32 dnum = lfdballoc((struct lfdbfree *)&Tbuf);
						int32 p;
						for(p = 0; p < LF_NUM_ROOT_LDENT; p++){
							Tbuf.arr[p].Used = 0;
						}
						write(diskdev, (char*)&Tbuf ,dnum);
						Iptr->ib_dba[q] = dnum;
					}
					lfibput(diskdev, newDir, Iptr);

					//ldp->ld_size = 0;
					buf.arr[z].ld_ilist 	 = newDir;
					buf.arr[z].dir_ent = TRUE;
					buf.arr[z].Used 	 = 1;
					char* np = namBuf;
					char* ln = buf.arr[z].ld_name;
					while(*np != NULLCH){
						*ln = *np;
						ln++; np++;
					}
					*ln = NULLCH;
					//kprintf("\r\nFILE %s Created\r\n", namBuf);
					write(diskdev, (char *)&buf ,cur_dnum);
					check = TRUE;
					break;
				}
			}
			if(check == TRUE)
				break;
		}
		signal(Lf_data.lf_mutex);
		signal(lfptr->lfmutex);
		return retval;	

	default:
		kprintf("lfControl: function %d not valid\n\r", func);
		signal(lfptr->lfmutex);
		return SYSERR;
	}
}
