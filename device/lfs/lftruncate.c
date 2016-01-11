/* lftruncate.c  -  lftruncate */

#include <xinu.h>

/*------------------------------------------------------------------------
 * lftruncate  -  truncate a file by freeing its index and data blocks
 *			(assumes directory mutex held)
 *------------------------------------------------------------------------
 */
status	lftruncate (
	  struct lflcblk *lfptr		/* ptr to file's cntl blk entry	*/
	)
{
	kprintf("Closing File %s...\r\n", lfptr->lfname);
	//struct	ldentry	*ldptr;		/* pointer to file's dir. entry	*/
	struct	lfiblk	iblock;		/* buffer for one index block	*/
	ibid32	ifree;			/* start of index blk free list	*/
	ibid32	firstib;		/* first index blk of the file	*/
	ibid32	nextib;			/* walks down list of the	*/
					/*   file's index blocks	*/
	dbid32	nextdb;			/* next data block to free	*/
	int32	i;			/* moves through data blocks in	*/
					/*   a given index block	*/

	//ldptr = lfptr->lfdirptr;	/* Get pointer to dir. entry	*/
	if (lfptr->ld_size == 0) {	/* file is already empty */
		return OK;
	}

	/* Clean up the open local file first */

	if ( (lfptr->lfibdirty) || (lfptr->lfdbdirty) ) {
		lfflush(lfptr);
	}
	lfptr->lfpos = 0;
	lfptr->lfinum = LF_INULL;
	lfptr->lfdnum = LF_DNULL;
	lfptr->lfbyte = &lfptr->lfdblock[LF_BLKSIZ];

	/* Obtain ID of first index block on free list */

	ifree = Lf_data.lf_dir.lfd_ifree;

	/* Record file's first i-block and clear directory entry */

	firstib = lfptr->lffilptr;
	bool8 found = FALSE;
	char namBuf[LF_NAME_LEN];
	dbid32 cur_dnum;
	struct ldirU buf;
	char* namPtr = lfptr->lfname+1;
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
					if(buf.arr[z].dir_ent == FALSE) continue;
					char* nam = namBuf;
					char* cmp = buf.arr[z].ld_name;
					while(*nam != NULLCH){
						if(*nam != *cmp) break;
						nam++; cmp++;
					}
					if(*nam == NULLCH && *cmp == NULLCH){
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
				return SYSERR;
			}
			else
				Dir_found = FALSE;
			namPtr++;
			bufPtr = namBuf;
		}
		if(*namPtr == NULLCH)
			*bufPtr = NULLCH;
	}

	// Here are the steps to find the exact file under the specific directory
	ibid32	cur_file;		/* ptr to an entry in directory	*/
	*namPtr = NULLCH;
	if(*namBuf == NULLCH){
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
				char* nam = namBuf;
				char* cmp = buf.arr[z].ld_name;
				while(*nam != NULLCH){
					if(*nam != *cmp) break;
					nam++; cmp++;
				}
				if(*nam == NULLCH && *cmp == NULLCH){
					found = TRUE;
					//the specific location in data block for the specific file get free.
					buf.arr[z].Used = 0;
					cur_file = buf.arr[z].ld_ilist;
					break;
				}
			}
			if(found == TRUE){
				break;
			}
		}
	}
	if(found == FALSE)
		return SYSERR;
	Lf_data.lf_dirdirty = TRUE;

	/* Walk along index block list, disposing of each data block	*/
	/*  and clearing the corresponding pointer.  A note on loop	*/
	/*  termination: last pointer is set to ifree below.		*/

	for (nextib=firstib; nextib!=ifree; nextib=iblock.ib_next) {

		/* Obtain a copy of current index block from disk	*/

		lfibget(Lf_data.lf_dskdev, nextib, &iblock);

		/* Free each data block in the index block		*/

		for (i=0; i<LF_IBLEN; i++) {	/* for each d-block	*/

			/* Free the data block */

			nextdb = iblock.ib_dba[i];
			if (nextdb != LF_DNULL) {
				lfdbfree(Lf_data.lf_dskdev, nextdb);
			}

			/* Clear entry in i-block for this d-block	*/

				iblock.ib_dba[i] = LF_DNULL;
		}

		/* Clear offset (just to make debugging easier)		*/

		iblock.ib_offset = 0;

		/* For the last index block on the list, make it point	*/
		/*	to the current free list			*/

		if (iblock.ib_next == LF_INULL) {
			iblock.ib_next = ifree;
		}

		/* Write cleared i-block back to disk */

		lfibput(Lf_data.lf_dskdev, nextib, &iblock);
	}

	/* Last index block on the file list now points to first node	*/
	/*   on the current free list.  Once we make the free list	*/
	/*   point to the first index block on the file list, the	*/
	/*   entire set of index blocks will be on the free list	*/

	Lf_data.lf_dir.lfd_ifree = firstib;

	/* Indicate that directory has changed and return */

	Lf_data.lf_dirdirty = TRUE;
	
	return OK;
}
