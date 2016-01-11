/* lfscreate.c  -  lfscreate */

#include <xinu.h>

/*------------------------------------------------------------------------
 * lfscreate  --  Create an initially-empty file system on a disk
 *------------------------------------------------------------------------
 */
status	lfscreate (
	  did32		disk,		/* ID of an open disk device	*/
	  ibid32	lfiblks,	/* num. of index blocks on disk	*/
	  uint32	dsiz		/* total size of disk in bytes	*/
	)
{
	uint32	sectors;		/* number of sectors to use	*/
	uint32	ibsectors;		/* number of sectors of i-blocks*/
	uint32	ibpersector;		/* number of i-blocks per sector*/
	struct	lfdir	dir;		/* Buffer to hold the directory	*/
	uint32	dblks;			/* total free data blocks	*/
	struct	lfiblk	iblock;		/* space for one i-block	*/
	struct	lfdbfree dblock;	/* data block on the free list	*/
	dbid32	dbindex;		/* index for data blocks	*/
	int32	retval;			/* return value from func call	*/
	int32	i;			/* loop index			*/

	/* Compute total sectors on disk */

	sectors = dsiz	/ LF_BLKSIZ;	/* truncate to full sector */

	/* Compute number of sectors comprising i-blocks */

	ibpersector = LF_BLKSIZ / sizeof(struct lfiblk);
	ibsectors = (lfiblks+(ibpersector-1)) / ibpersector; /* round up*/
	lfiblks = ibsectors * ibpersector;
	if (ibsectors > sectors/2) {	/* invalid arguments */
		return SYSERR;
	}

	/* Create an initial directory */

	memset((char *)&dir, NULLCH, sizeof(struct lfdir));
	Lf_data.lf_dskdev = disk;
	dir.lfd_nfiles = 0;
	dbindex= (dbid32)(ibsectors + 1);
	dir.lfd_dfree = dbindex;
	dir.lfd_ifree = 1;
	dblks = sectors - ibsectors - 1;
	retval = write(disk,(char *)&dir, LF_AREA_DIR);
	if (retval == SYSERR) {
		return SYSERR;
	}

	/* Create list of free i-blocks on disk */

	lfibclear(&iblock, 0);
	for (i=1; i<lfiblks-1; i++) {
		iblock.ib_next = (ibid32)(i + 1);
		lfibput(disk, i, &iblock);
	}
	iblock.ib_next = LF_INULL;
	lfibput(disk, i, &iblock);

	/* Create list of free data blocks on disk */

	memset((char*)&dblock, NULLCH, LF_BLKSIZ);
	for (i=0; i<dblks-1; i++) {
		dblock.lf_nextdb = dbindex + 1;
		write(disk, (char *)&dblock, dbindex);
		dbindex++;
	}
	dblock.lf_nextdb = LF_DNULL;
	write(disk, (char *)&dblock, dbindex);

	/* Initialize the root */
	lfibclear(&iblock,0);
	iblock.ib_next = LF_INULL;
	iblock.ld_size   = 0;
	iblock.dir_ent 	 = TRUE;
	close(disk);
	//every dir, I allocate 16 data block to it when it's initialized;
	if (! Lf_data.lf_dirpresent) {
	    read(Lf_data.lf_dskdev, (char *)&Lf_data.lf_dir,LF_AREA_DIR);
	    Lf_data.lf_dirpresent = TRUE;
	}
	int32 q;
	int32 p;
	struct  ldirU buf;
	for(q=0; q<LF_IBLEN ;q++){
		dbid32 dnum = lfdballoc(&dblock);
		iblock.ib_dba[q] = dnum;
		//kprintf("%d ",iblock.ib_dba[q]);
		for(p = 0; p < LF_NUM_ROOT_LDENT; p++){
			buf.arr[p].Used = 0;
			//kprintf("\r\n%d llll\r\n",lpt->Used);
			write(disk, (char *)&buf ,dnum);
		}
	}
	//kprintf("\r\n");
	lfibput(disk, 0, &iblock);
	return OK;
}
