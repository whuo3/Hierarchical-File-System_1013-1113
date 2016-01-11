/* lfilesys.h - ib2sect, ib2disp */

/************************************************************************/
/*									*/
/*               Local File System Data Structures			*/
/*									*/
/*   A local file system uses a random-access disk composed of 512-byte	*/
/* sectors numbered 0 through N-1.  We assume disk hardware can read or	*/
/* write any sector at random, but must transfer an entire sector.	*/
/* Thus, to write a few bytes, the file system must read the sector,	*/
/* replace the bytes, and then write the sector back to disk.  Xinu's	*/
/* local file system divides the disk as follows: sector 0 is a 	*/
/* directory, the next K sectors constitute an index area, and the	*/
/* remaining sectors comprise a data area. The data area is easiest to	*/
/* understand: each sector holds one data block (d-block) that stores	*/
/* contents from one of the files (or is on a free list of unused data	*/
/* blocks).  We think of the index area as holding an array of index	*/
/* blocks (i-blocks) numbered l0 through I-1.  A given sector in the	*/
/* index area holds 7 of the index blocks, which are each 72 bytes	*/
/* long.  Given an i-block number, the file system must calculate the	*/
/* disk sector in which the i-block is located and the byte offset	*/
/* within the sector at which the i-block resides.  Internally, a file	*/
/* is known by the i-block index of the first i-block for the file.	*/
/* The directory contains a list of file names and the	i-block number	*/
/* of the first i-block for the file.  The directory also holds the	*/
/* i-block number for a list of free i-blocks and a data block number	*/
/* of the first data block on a list of free data blocks.		*/
/*									*/
/************************************************************************/

#ifndef	Nlfl
#define	Nlfl	1
#endif

/* Use the remote disk device if no disk is defined (file system  */
/*  *assumes* the underlying disk has a block size of 512 bytes)  */

#ifndef	LF_DISK_DEV
#define	LF_DISK_DEV	RDISK
#endif

#define	LF_MODE_R	F_MODE_R	/* mode bit for "read"		*/
#define	LF_MODE_W	F_MODE_W	/* mode bit for "write"		*/
#define	LF_MODE_RW	F_MODE_RW	/* mode bits for "read or write"*/
#define	LF_MODE_O	F_MODE_O	/* mode bit for "old"		*/
#define	LF_MODE_N	F_MODE_N	/* mode bit for "new"		*/

#define	LF_BLKSIZ	512		/* assumes 512-byte disk blocks	*/
#define	LF_NAME_LEN	16		/* length of name plus null	*/
#define LF_NUM_ROOT_LDENT 16    /* num. of ldentry in a datablock in root */
#define	LF_NUM_DIR_ENT	20		/* num. of files in a directory	*/
#define	LF_NUM_DAT_ENT	20		/* num. of files in a directory	*/

#define	LF_FREE		0		/* slave device is available	*/
#define	LF_USED		1		/* slave device is in use	*/

#define	LF_INULL	(ibid32) -1	/* index block null pointer	*/
#define	LF_DNULL	(dbid32) -1	/* data block null pointer	*/
#define	LF_IBLEN	16		/* data block ptrs per i-block	*/
#define	LF_IDATA	8192		/* bytes of data indexed by a	*/
					/*  single index block		*/

#define	LF_IMASK	0x00001fff	/* mask for the data indexed by	*/
					/*  a single index block (i.e.,	*/
					/*  bytes 0 through 8191).	*/
#define	LF_DMASK	0x000001ff	/* mask for the data in a data	*/
					/*  block  (0 through 511)	*/

#define	LF_AREA_IB	1		/* first sector of i-blocks	*/
#define	LF_AREA_DIR	0		/* first sector of directory	*/


/* Structure of an index block on disk */

struct	lfiblk		{		/* format of index block  ------80 bytes	*/
	uint32		ld_size;
	ibid32		ib_next;	/* address of next index block	*/
	uint32		ib_offset;	/* first data byte of the file	*/
							/*  indexed by this i-block	*/
	dbid32		ib_dba[LF_IBLEN];/* ptrs to data blocks indexed	*/
	bool8		dir_ent;
};

/* Conversion functions below assume 6 index blocks per disk block */

/* Conversion between index block number and disk sector number */

#define	ib2sect(ib)	(((ib)/6)+LF_AREA_IB)

/* Conversion between index block number and the relative offset within	*/
/*	a disk sector							*/

#define	ib2disp(ib)	(((ib)%6)*sizeof(struct lfiblk))


/* Structure used in each directory entry for the local file system */

struct	ldentry	{			/* description of entry for one  ------32 bytes	*/
					/*  file in the directory	*/
	//uint32	ld_size;		/* curr. size of file in bytes	*/
	ibid32	ld_ilist;		// if it's file ldentry: it is the iblock of the file
							// if it's directory ldentry: it is the iblock of the directory
	char	ld_name[LF_NAME_LEN];	/* null-terminated file name	*/
	int32	Used;			// To identify if the current ldentry is available or not;
	bool8	dir_ent;
	char 	padding[4];
};

/* Structure of a data block when on the free list on disk */

struct	lfdbfree {   									 //  ------512 bytes
	dbid32	lf_nextdb;		/* next data block on the list	*/
	char	lf_unused[LF_BLKSIZ - sizeof(dbid32)];
};

/* Format of the file system directory, either on disk or in memory */

#pragma pack(2)
struct	lfdir	{			/* entire directory on disk  ------512 bytes	*/
	dbid32	lfd_dfree;		/* list of free d-blocks on disk*/
	ibid32	lfd_ifree;		/* list of free i-blocks on disk*/
	int32	lfd_nfiles;		/* current number of files	*/
	//struct	ldentry lfd_files[LF_NUM_DIR_ENT]; /* set of files	*/
	//char	padding[20];		/* unused chars in directory blk*/

	ibid32	root;			/* pointed to the first iblock*/
	char 	padding[496];
};
#pragma pack()

/* Global data used by local file system */

struct	lfdata	{			/* local file system data	*/
	did32	lf_dskdev;		/* device ID of disk to use	*/
	sid32	lf_mutex;		/* mutex for the directory and	*/
					/*  index/data free lists	*/
	struct	lfdir	lf_dir;		/* In-memory copy of directory	*/
	bool8	lf_dirpresent;		/* True when directory is in	*/
					/*  memory (first file is open)	*/
	bool8	lf_dirdirty;		/* Has the directory changed?	*/
};

/* Control block for local file pseudo-device */

struct	lflcblk	{			/* Local file control block	*/
					/*  (one for each open file)	*/
	byte	lfstate;		/* Is entry free or used	*/
	did32	lfdev;			/* device ID of this device	*/
	sid32	lfmutex;		/* Mutex for this file		*/
	ibid32	lffilptr;	    /* file block that always contains the first iblock of the file	*/
	int32	lfmode;			/* mode (read/write/both)	*/
	uint32	lfpos;			/* Byte position of next byte	*/
					/*   to read or write		*/
	uint32		ld_size;	/* Record the size of the file */
	char	lfname[LF_NAME_LEN];	/* Name of the file		*/
	ibid32	lfinum;			/* ID of current index block in	*/
					/*   lfiblock or LF_INULL	*/
	struct	lfiblk	lfiblock;	/* In-mem copy of current index	*/
					/*   block			*/
	dbid32	lfdnum;			/* Number of current data block	*/
					/*   in lfdblock or LF_DNULL	*/
	char	lfdblock[LF_BLKSIZ];	/* in-mem copy of current data	*/
					/*   block			*/
	char	*lfbyte;		/* Ptr to byte in lfdblock or	*/
					/*  address one beyond lfdblock	*/
					/*   if current file pos lies	*/
					/*   outside lfdblock		*/
	bool8	lfibdirty;		/* Has lfiblock changed?	*/
	bool8	lfdbdirty;		/* Has lfdblock changed?	*/
};

struct  ldirU {
	struct ldentry arr[LF_NUM_ROOT_LDENT];
};

extern	struct	lfdata	Lf_data;
extern	struct	lflcblk	lfltab[];

/* Control functions */

#define	LF_CTL_DEL	F_CTL_DEL	/* Delete a file		*/
#define	LF_CTL_TRUNC	F_CTL_TRUNC	/* Truncate a file		*/
#define LF_CTL_SIZE	F_CTL_SIZE	/* Obtain the size of a file	*/
#define LF_CTL_MKDIR F_CTL_MKDIR /* List the file under specific dir*/
