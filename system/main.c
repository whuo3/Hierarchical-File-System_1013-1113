#include <xinu.h>

int main(int argc, char **argv)
{
	char wbuff[] = {"While disks provide a convenient means of permanent storage, they support only a simple and limited naming mechanism of the data they hold (the block number). File systems are used to overcoming this limitation by providing a more elaborate and useful naming mechanism for the data blocks on a disk. This original Xinu File System is a mini-file system with an elegant design that has a limited numbers of files, short file names, and no hierarchical directory structure; however, it allows files to grow dynamically. The objective of this lab is to enhance the existing file system for Xinu. You will extend the functionality of the Xinu File System on top of the remote disk abstraction. You are going to transform the current flat file system into a new file system with hierarchical directory structure.While disks provide a convenient means of permanent storage, they support only a simple and limited naming mechanism of the data they hold (the block number). File systems are used to overcoming this limitation by providing a more elaborate and useful naming mechanism for the data blocks on a disk. This original Xinu File System is a mini-file system with an elegant design that has a limited numbers of files, short file names, and no hierarchical directory structure; however, it allows files to grow dynamically. \r\nThe objective of this lab is to enhance the existing file system for Xinu. You will extend the functionality of the Xinu File System on top of the remote disk abstraction. You are going to transform the current flat file system into a new file system with hierarchical directory structure. \r\n"};
	int32 wbufflen = sizeof( wbuff );
	char rbuff[ wbufflen * 10 ];
	int32 rbufflen = sizeof( rbuff );
	int32 retval;
	int32 fd[5];
	int32 i;
	int32 j;

	kprintf("Open remote disk\r\n");
	retval = open(RDISK,"XinuDisk","rw");
	if (retval == SYSERR){
		panic("Error: could not open the remote disk, check the server\n\r");
	}

	kprintf("Initializing file system\r\n");
	retval = lfscreate ( RDISK, 1000, 1024000);
	if( retval == SYSERR )
		panic("Initialize file system failed");
	
	kprintf("\r\nList root directory:\r\n");
	retval = lflistdirh( RDISK, "/" ); //list root dir
	if( retval == SYSERR )
		kprintf("Listing failed\r\n");
	
	// Create 5 directories
	char dirname [] = {"/DIR_1"};
	kprintf("\r\n");
	for( i=0; i<5; i++){
		dirname[5] = '1'+i;
		kprintf("Creating directory %s", dirname );
		retval = control(LFILESYS, LF_CTL_MKDIR, (int)dirname, 0);
		//kprintf("\r\n************\r\n");
		if( retval == SYSERR )
			kprintf("....failed");
		kprintf("\r\n");
	}
	
	char dirname2 [] = {"/DIR_1/DIRSHIT"};
	control(LFILESYS, LF_CTL_MKDIR, (int)dirname2, 0);
	char dirname21 [] = {"/DIR_1/DIRSHIT1"};
	control(LFILESYS, LF_CTL_MKDIR, (int)dirname21, 0);
	char dirname22 [] = {"/DIR_1/DIRSHIT2"};
	control(LFILESYS, LF_CTL_MKDIR, (int)dirname22, 0);
	char dirname23 [] = {"/DIR_1/DIRSHIT3"};
	control(LFILESYS, LF_CTL_MKDIR, (int)dirname23, 0);
	char dirname3 [] = {"/DIR_1/DIRSHIT/DIRGOOD1"};
	control(LFILESYS, LF_CTL_MKDIR, (int)dirname3, 0);
	char dirname4 [] = {"/DIR_1/DIRSHIT/DIRGOOD2"};
	control(LFILESYS, LF_CTL_MKDIR, (int)dirname4, 0);
	char dirname5 [] = {"/DIR_1/DIRSHIT/DIRGOOD3"};
	control(LFILESYS, LF_CTL_MKDIR, (int)dirname5, 0);
	char dirname6 [] = {"/DIR_1/DIRSHIT/DIRGOOD4"};
	control(LFILESYS, LF_CTL_MKDIR, (int)dirname6, 0);
	char dirname7 [] = {"/DIR_1/DIRSHIT/DIRGOOD5"};
	control(LFILESYS, LF_CTL_MKDIR, (int)dirname7, 0);
	
	char file [] = {"/DIR_1/DIRSHIT/DIRGOOD1/1"};
	for(i =0; i<5; i++){
		file[24] = '1'+i;
		//kprintf("%s\r\n",file);
		close(open(LFILESYS, file, "rwn"));
	}

	//Open a file in each directory
	char filename [] = {"/DIR_1/file"};
	kprintf("\r\n");
	for( i=0; i<5; i++)
	{ 
		filename[5] = '1'+i;
		kprintf("Opening file at %s\r\n", filename);
		fd[i] = open(LFILESYS, filename, "rwn");
		kprintf("  Got the sudo device : %d \r\n", fd[i]);
	}
	
	//Testing for the extra credit part..
	kprintf("\r\n\r\nDir layer:\r\n");
	lflistdirh( RDISK, "/");
	lflistdirh( RDISK, "/DIR_1");
	lflistdirh( RDISK, "/DIR_1/DIRSHIT");
	lflistdirh( RDISK, "/DIR_1/DIRSHIT/DIRGOOD1");

	// Write to first file
	retval = write( fd[0], wbuff, wbufflen );
	//kprintf("%d***\r\n",wbufflen);
	if( retval == SYSERR )
		kprintf("Write failed \r\b");
	else
		kprintf("Write returned %d \r\n", retval );
	
	// Write to last file
	kprintf("\r\nWriting %d characters to /DIR_5/file \r\n", wbufflen*10 );
	int total = 0;
	for( i=0; i<10; i++ ){
		retval = write( fd[4], wbuff, wbufflen );
		if( retval == SYSERR )
			kprintf("Write failed \r\b");
		else {
			kprintf("Write returned %d \r\n", retval );
			total += retval;
		}
	}
	if( total != 0 )
		kprintf("Wrote a total of %d characters\r\n", total );
	
	//Go to the beginning of the file at /DIR_5/file
	seek(fd[4],0);
	
	//Read
	int wrong = 0;
	kprintf("\r\nReading %d characters.... ", rbufflen);
	retval = read( fd[4], rbuff, rbufflen );
	if( retval == SYSERR )
		kprintf("Read failed \r\n");
	else{ 
		kprintf("Read returned %d\r\n", retval );

		//check the words read
		for (j=0,i=0; j<retval; j++,i++)
		{
			i = i % wbufflen;
			if(rbuff[j]!=wbuff[i]){
				wrong ++; 
				kprintf("read wrong at i: %d (expect %c, got %c)\r\n", j, wbuff[i], rbuff[j]);
			}
			if( wrong >= 20 ) break; //stops printing after 20 characters wrong
		} 
		if( wrong == 0)
			kprintf("Congrats. Characters read matches characters wrote.\r\n");
	}
	
	
	//close all files
	kprintf("\r\nClosing all files \r\n");
	for( i=0; i<5; i++ ){
		retval = close( fd[i] );
		if( retval == SYSERR )
			kprintf("Close file /DIR_%d/file failed \r\n", i+1);
	}

	//list files
	kprintf("\r\nList root directory:\r\n");
	retval = lflistdirh( LFILESYS, "/" );
	if( SYSERR == retval ) 
		kprintf("  Listing failed\r\n");
	for( i=0; i<5; i++ ){
		dirname[5] = '1'+i;
		kprintf("\r\nList directory %s:\r\n", dirname);
		retval = lflistdirh( LFILESYS, dirname );
		if( SYSERR == retval ) 
			kprintf("  Listing failed\r\n");
	}

	return OK;
}
