/* lfsOpen.c  -  lfsOpen */

#include <xinu.h>
/*------------------------------------------------------------------------
 * lfsOpen - open a file and allocate a local file pseudo-device
 *------------------------------------------------------------------------
 */
devcall	lfsOpen (
	 struct	dentry	*devptr,	/* entry in device switch table	*/
	 char	*name,			/* name of file to open		*/
	 char	*mode			/* mode chars: 'r' 'w' 'o' 'n'	*/
	)
{
	//kprintf("Opening File...\r\n");
	struct	lfdir	*dirptr;	/* ptr to in-memory directory	*/
	char		*from, *to;	/* ptrs used during copy	*/
	char		*nam, *cmp;	/* ptrs used during comparison	*/
	int32		i;		/* general loop index		*/
	did32		lfnext;		/* minor number of an unused	*/
					/*    file pseudo-device	*/
	ibid32	cur_file;		/* ptr to an entry in directory	*/
	struct	lflcblk	*lfptr;		/* ptr to open file table entry	*/
	bool8		found;		/* was the name found?		*/
	int32	retval;			/* value returned from function	*/
	int32	mbits;			/* mode bits			*/

	/* Check length of name file (leaving space for NULLCH */

	from = name;
	for (i=0; i< LF_NAME_LEN; i++) {
		if (*from++ == NULLCH) {
			break;
		}
	}
	/*if (i >= LF_NAME_LEN) {		// name is too long 
		kprintf("Name is too long..\r\n");
		return SYSERR;
	}*/

	/* Parse mode argument and convert to binary */

	mbits = lfgetmode(mode);
	if (mbits == SYSERR) {
		//kprintf("Parse mode fail..\r\n");
		return SYSERR;
	}

	/* If named file is already open, return SYSERR */

	lfnext = SYSERR;
	for (i=0; i<Nlfl; i++) {	/* search file pseudo-devices	*/
		lfptr = &lfltab[i];
		if (lfptr->lfstate == LF_FREE) {
			if (lfnext == SYSERR) {
				lfnext = i; /* record index */
			}
			continue;
		}

		/* Compare requested name to name of open file */

		nam = name;
		cmp = lfptr->lfname;
		while(*nam != NULLCH) {
			if (*nam != *cmp) {
				break;
			}
			nam++;
			cmp++;
		}

		/* See if comparison succeeded */

		if ( (*nam==NULLCH) && (*cmp == NULLCH) ) {
			//kprintf("Same file name fail...\r\n");
			return SYSERR;
		}
	}
	if (lfnext == SYSERR) {	/* no slave file devices are available	*/
		//kprintf("No slave file devices are available\r\n");
		return SYSERR;
	}

	/* Obtain copy of directory if not already present in memory	*/

	dirptr = &Lf_data.lf_dir;
	wait(Lf_data.lf_mutex);
	if (! Lf_data.lf_dirpresent) {
	    retval = read(Lf_data.lf_dskdev, (char *)dirptr,LF_AREA_DIR);
	    if (retval == SYSERR ) {
		signal(Lf_data.lf_mutex);
		return SYSERR;
	    }
	    Lf_data.lf_dirpresent = TRUE;
	}



	//***************** Changes start from here *********************

	/* Search directory to see if file exists */
	//Find the specific directory Path
	found = FALSE;
	char namBuf[LF_NAME_LEN];
	dbid32 cur_dnum;
	struct ldirU buf;
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
				read(diskdev, (char*)&buf, dnum);
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
						//kprintf("Directory %s founded\r\n", buf.arr[z].ld_name);
						Dir_found = TRUE;
						break;
					}
				}
				if(Dir_found == TRUE){
					lfibget(diskdev, buf.arr[z].ld_ilist, &curIblock);
					break;
				}
			}
			//if Directory not found, then create the dictory.
			if(Dir_found == FALSE){
				control(LFILESYS,LF_CTL_MKDIR,(int)namBuf, 0);
			}
			else
				Dir_found = FALSE;
			namPtr++;
			bufPtr = namBuf;
		}
		if(*namPtr == NULLCH)
			*bufPtr = NULLCH;
	}
	//kprintf("Looking for File %s ...\r\n", namBuf);
	*bufPtr = NULLCH;
	// Here are the steps to find the exact file under the specific directory
	if(*namBuf == NULLCH){
		signal(Lf_data.lf_mutex);
		return SYSERR;
	}
	else{
		int32 x;
		for(x = 0; x < LF_IBLEN; x++){
			dbid32 dnum = Iptr->ib_dba[x];
			if(dnum == LF_DNULL) continue;
			read(diskdev, (char*)&buf, dnum);
			cur_dnum = dnum;
			int32 z;
			for(z = 0; z<LF_NUM_ROOT_LDENT; z++){
				if(buf.arr[z].Used == 0) continue;
				if(buf.arr[z].dir_ent == TRUE) continue;
				char* nam = namBuf;
				char* cmp = buf.arr[z].ld_name;
				while(*nam != NULLCH){
					if(*nam != *cmp) break;
					nam++; cmp++;
				}
				if(*nam == NULLCH && *cmp == NULLCH){
					found = TRUE;
					cur_file = buf.arr[z].ld_ilist;
					break;
				}
			}
			if(found == TRUE)
				//kprintf("File %s Founded...\r\n", namBuf);
				break;
		}
	}


	/* Case #1 - file is not in directory (i.e., does not exist)	*/

	if (! found) {
		if (mbits & LF_MODE_O) {	/* file *must* exist	*/
			signal(Lf_data.lf_mutex);
			return SYSERR;
		}
		//kprintf("Creating File %s ...\r\n", namBuf);

		int32 y;
		for(y=0; y< LF_IBLEN; y++){
			dbid32 dnum = Iptr->ib_dba[y];
			if(dnum == LF_DNULL) continue;
			read(diskdev, (char*)&buf, dnum);
			cur_dnum = dnum;
			int32 z;
			bool8 check = FALSE;
			for(z = 0; z < LF_NUM_ROOT_LDENT; z++){
				if(buf.arr[z].Used == 0){ 
				//if condition matched, then there is available space to store the new ldentry in data block
					ibid32 newFil = lfiballoc();
					lfibclear(Iptr, 0);
					//lfibget(diskdev, newFil, &curIblock);
					Iptr->ib_next = LF_INULL;
					Iptr->ld_size = 0;
					Iptr->ib_offset = 0;
					Iptr->dir_ent = FALSE;
					lfibget(diskdev, newFil, &curIblock);
					//ldp->ld_size = 0;
					buf.arr[z].ld_ilist = newFil;
					buf.arr[z].Used = 1;
					buf.arr[z].dir_ent = FALSE;
					char* np = namBuf;
					char* ln = buf.arr[z].ld_name;
					while(*np != NULLCH){
						*ln = *np;
						ln++; np++;
					}
					*ln = *np;
					write(diskdev, (char*)&buf ,cur_dnum);
					cur_file = newFil;
					dirptr->lfd_nfiles++;
					check = TRUE;
					break;
				}
			}
			if(check ==TRUE)
					break;
		}

	/* Case #2 - file is in directory (i.e., already exists)	*/

	} else if (mbits & LF_MODE_N) {		/* file must not exist	*/
			signal(Lf_data.lf_mutex);
			return SYSERR;
	}

	/* Initialize the local file pseudo-device */

	lfptr = &lfltab[lfnext];
	lfptr->lfstate = LF_USED;
	lfptr->lffilptr = cur_file;	/* point to directory entry	*/
	lfptr->lfmode = mbits & LF_MODE_RW;

	/* File starts at position 0 */

	lfptr->lfpos     = 0;

	to = lfptr->lfname;
	from = name;
	while ( (*to = *from++) != NULLCH ) {
		;
	}

	//lfinum is assigned with the first iblock of the iblock
	lfptr->lfinum    = cur_file;
	lfptr->lfdnum    = LF_DNULL;
	lfibget(diskdev, cur_file, &curIblock );
	lfptr->ld_size 	 = curIblock.ld_size;
	/* Initialize byte pointer to address beyond the end of the	*/
	/*	buffer (i.e., invalid pointer triggers setup)		*/

	lfptr->lfbyte = &lfptr->lfdblock[LF_BLKSIZ];
	lfptr->lfibdirty = FALSE;
	lfptr->lfdbdirty = FALSE;

	signal(Lf_data.lf_mutex);

	return lfptr->lfdev;
}
