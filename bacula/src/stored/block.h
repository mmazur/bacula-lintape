/*
 * Block definitions for Bacula media data format.
 *
 *    Kern Sibbald
 *
 *   Version $Id$
 *
 */
/*
   Copyright (C) 2000, 2001, 2002 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */


#ifndef __BLOCK_H
#define __BLOCK_H 1

#define MAX_BLOCK_LENGTH    500001	/* this is a sort of sanity check */
#define DEFAULT_BLOCK_SIZE (512 * 126)	/* 64,512 N.B. do not use 65,636 here */

/* Block Header definitions. */
#define BLKHDR_ID        "BB01"
#define BLKHDR_ID_LENGTH  4
#define BLKHDR_CS_LENGTH  4		/* checksum length */
#define BLKHDR_LENGTH	 16		/* Total length */

/*
 * This is the Media structure for a block header
 *  Note, when written, it is serialized.
 */
typedef struct s_block_hdr {
   uint32_t CheckSum;
   uint32_t block_size;
   uint32_t BlockNumber;
   char     Id[BLKHDR_ID_LENGTH+1];
} BLOCK_HDR;

/*
 * DEV_BLOCK for reading and writing blocks.
 * This is the basic unit that is written to the device, and
 * it contains a Block Header followd by Records.  Note,
 * at times (when reading a file), this block may contain
 * multiple blocks.
 *
 *  This is the memory structure for a device block.
 */
typedef struct s_dev_block {
   struct s_dev_block *next;	      /* pointer to next one */
   /* binbuf is the number of bytes remaining
    * in the buffer. For writes, it is bytes not yet written.
    * For reads, it is remaining bytes not yet read.
    */
   uint32_t binbuf;		      /* bytes in buffer */
   uint32_t block_len;		      /* length of current block read */
   uint32_t buf_len;		      /* max/default block length */
   uint32_t BlockNumber;	      /* sequential block number */
   uint32_t read_len;		      /* bytes read into buffer */  
   int failed_write;		      /* set if write failed */
   char *bufp;			      /* pointer into buffer */
   char *buf;			      /* actual data buffer. This is a 
				       * Pool buffer!	
				       */
} DEV_BLOCK;

#endif
