/* lfibput.c - lfibput */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  lfibput  --  write an index block to disk given its ID (assumes
 *			mutex is held)
 *------------------------------------------------------------------------
 */
status	lfibput(
	  did32		diskdev,	/* ID of disk device		*/
	  ibid32	inum,		/* ID of index block to write	*/
	  struct lfiblk	*ibuff		/* buffer holding the index blk	*/
	)
{
	dbid32	diskblock;		/* ID of disk sector (block)	*/
	char	*from, *to;		/* pointers used in copying	*/
	int32	i;			/* loop index used during copy	*/
	char	dbuff[LF_BLKSIZ];	/* temp. buffer to hold d-block	*/

	/* Compute disk block number and offset of index block */

	diskblock = ib2sect(inum);
	to = dbuff + ib2disp(inum);
	from = (char *)ibuff;
	//kprintf("put: %d\r\n   %d\r\n \r\n", ib2sect(inum), ib2disp(inum));
	/* Read disk block */

	if (read(diskdev, dbuff, diskblock) == SYSERR) {
		return SYSERR;
	}

	/* Copy index block into place */

	for (i=0 ; i<sizeof(struct lfiblk) ; i++) {
		*to++ = *from++;
	}

	/* Write the block back to disk */

	write(diskdev, dbuff, diskblock);
	return OK;
}
