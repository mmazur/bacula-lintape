/*

			 S M A R T A L L O C
			Smart Memory Allocator

	Evolved   over	 several  years,  starting  with  the  initial
	SMARTALLOC code for AutoSketch in 1986, guided	by  the  Blind
	Watchbreaker,  John  Walker.  Isolated in this general-purpose
	form in  September  of	1989.	Updated  with  be  more  POSIX
	compliant  and	to  include Web-friendly HTML documentation in
	October  of  1998  by  the  same  culprit.    For   additional
	information and the current version visit the Web page:

		  http://www.fourmilab.ch/smartall/

  
	 Version $Id$

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

#include "bacula.h"
/* Use the real routines here */
#undef realloc
#undef calloc
#undef malloc
#undef free
      
/*LINTLIBRARY*/


#ifdef SMARTALLOC

extern char my_name[];		      /* daemon name */

typedef unsigned short sm_ushort;

#define EOS      '\0'              /* End of string sentinel */
#define sm_min(a, b) ((a) < (b) ? (a) : (b))

/*  Queue data structures  */

/*  Memory allocation control structures and storage.  */

struct abufhead {
	struct b_queue abq;	     /* Links on allocated queue */
	unsigned ablen; 	   /* Buffer length in bytes */
	char *abfname;		   /* File name pointer */
	sm_ushort ablineno;	   /* Line number of allocation */ 
};

static struct b_queue abqueue = {    /* Allocated buffer queue */
	&abqueue, &abqueue
};

static Boolean bufimode = False;   /* Buffers not tracked when True */

#define HEAD_SIZE BALIGN(sizeof(struct abufhead))


/*  SMALLOC  --  Allocate buffer, enqueing on the orphaned buffer
		 tracking list.  */

static void *smalloc(char *fname, int lineno, unsigned int nbytes)
{
	char *buf;

	/* Note:  Unix	MALLOC	actually  permits  a zero length to be
	   passed and allocates a valid block with  zero  user	bytes.
	   Such  a  block  can	later  be expanded with realloc().  We
           disallow this based on the belief that it's better to  make
	   a  special case and allocate one byte in the rare case this
	   is desired than to miss all the erroneous occurrences where
	   buffer length calculation code results in a zero.  */

	ASSERT(nbytes > 0);

	nbytes += HEAD_SIZE + 1;
	if ((buf = (char *) malloc(nbytes)) != NULL) {
	   /* Enqueue buffer on allocated list */
	   qinsert(&abqueue, (struct b_queue *) buf);
	   ((struct abufhead *) buf)->ablen = nbytes;
	   ((struct abufhead *) buf)->abfname = bufimode ? NULL : fname;
	   ((struct abufhead *) buf)->ablineno = (sm_ushort) lineno;
	   /* Emplace end-clobber detector at end of buffer */
	   buf[nbytes - 1] = (((long) buf) & 0xFF) ^ 0xC5;
	   buf += HEAD_SIZE;  /* Increment to user data start */
	}
	sm_check(fname, lineno, True);
        Dmsg4(1150, "smalloc %d at %x from %s:%d\n", nbytes, buf, fname, lineno);
	return (void *) buf;
}

/*  SM_NEW_OWNER -- Update the File and line number for a buffer
		    This is to accomodate mem_pool. */

void sm_new_owner(char *fname, int lineno, char *buf)
{
	buf -= HEAD_SIZE;  /* Decrement to header */
	((struct abufhead *) buf)->abfname = bufimode ? NULL : fname;
	((struct abufhead *) buf)->ablineno = (sm_ushort) lineno;
	return;
}

/*  SM_FREE  --  Update free pool availability.  FREE is never called
		 except  through  this interface or by actuallyfree().
		 free(x)  is  defined  to  generate  a	call  to  this
		 routine.  */

void sm_free(char *file, int line, void *fp)
{
	char *cp = (char *) fp;
	struct b_queue *qp;

	sm_check(__FILE__, __LINE__, True);
	if (cp == NULL) {
           Emsg2(M_ABORT, 0, "Attempt to free NULL called from %s:%d\n", file, line);
	}

	cp -= HEAD_SIZE;
	qp = (struct b_queue *) cp;

        Dmsg4(1150, "sm_free %d at %x from %s:%d\n", 
	      ((struct abufhead *)cp)->ablen, fp, 
	      ((struct abufhead *)cp)->abfname, ((struct abufhead *)cp)->ablineno);

	/* The following assertions will catch virtually every release
           of an address which isn't an allocated buffer. */
	if (qp->qnext->qprev != qp) {
           Emsg2(M_ABORT, 0, "qp->qnext->qprev != qp called from %s:%d\n", file, line);
	}
	if (qp->qprev->qnext != qp) {
           Emsg2(M_ABORT, 0, "qp->qprev->qnext != qp called from %s:%d\n", file, line);
	}

	/* The following assertion detects storing off the  end  of  the
	   allocated  space in the buffer by comparing the end of buffer
	   checksum with the address of the buffer.  */

	if (((unsigned char *) cp)[((struct abufhead *) cp)->ablen - 1] !=
		 ((((long) cp) & 0xFF) ^ 0xC5)) {
           Emsg2(M_ABORT, 0, "Buffer overrun called from %s:%d\n", file, line);
	}


	qdchain(qp);

	/* Now we wipe the contents of	the  just-released  buffer  with
           "designer  garbage"  (Duff  Kurland's  phrase) of alternating
	   bits.  This is intended to ruin the day for any miscreant who
           attempts to access data through a pointer into storage that's
	   been previously released. */

	memset(cp, 0xAA, (int) ((struct abufhead *) cp)->ablen);

	free(cp);
}

/*  SM_MALLOC  --  Allocate buffer.  NULL is returned if no memory
		   was available.  */

void *sm_malloc(char *fname, int lineno, unsigned int nbytes)
{
	void *buf;

	if ((buf = smalloc(fname, lineno, nbytes)) != NULL) {

	   /* To catch sloppy code that assumes  buffers  obtained  from
	      malloc()	are  zeroed,  we  preset  the buffer contents to
              "designer garbage" consisting of alternating bits.  */

	   memset(buf, 0x55, (int) nbytes);
	}
	return buf;
}

/*  SM_CALLOC  --  Allocate an array and clear it to zero.  */

void *sm_calloc(char *fname, int lineno,
		unsigned int nelem, unsigned int elsize)
{
	void *buf;

	if ((buf = smalloc(fname, lineno, nelem * elsize)) != NULL) {
	   memset(buf, 0, (int) (nelem * elsize));
	}
	return buf;
}

/*  SM_REALLOC	--  Adjust the size of a  previously  allocated  buffer.
                    Note  that  the trick of "resurrecting" a previously
		    freed buffer with realloc() is NOT supported by this
		    function.	Further, because of the need to maintain
		    our control storage, SM_REALLOC must always allocate
		    a  new  block  and	copy  the data in the old block.
		    This may result in programs which make heavy use  of
		    realloc() running much slower than normally.  */

void *sm_realloc(char *fname, int lineno, void *ptr, unsigned int size)
{
	unsigned osize;
	void *buf;
	char *cp = (char *) ptr;

	sm_check(fname, lineno, True);
	if (size <= 0) {
           e_msg(fname, lineno, M_ABORT, 0, "sm_realloc size: %d\n", size);
	}

	/*  If	the  old  block  pointer  is  NULL, treat realloc() as a
	   malloc().  SVID is silent  on  this,  but  many  C  libraries
	   permit this.  */

	if (ptr == NULL)
	   return sm_malloc(fname, lineno, size);

	/* If the old and new sizes are the same, be a nice guy and just
	   return the buffer passed in.  */

	cp -= HEAD_SIZE;
	osize = ((struct abufhead *) cp)->ablen -
		(HEAD_SIZE + 1);
	if (size == osize) {
	   return ptr;
	}

	/* Sizes differ.  Allocate a new buffer of the	requested  size.
           If  we  can't  obtain  such a buffer, act as defined in SVID:
	   return NULL from  realloc()	and  leave  the  buffer  in  PTR
	   intact.  */

	if ((buf = smalloc(fname, lineno, size)) != NULL) {
	   memcpy(buf, ptr, (int) sm_min(size, osize));
	   /* If the new buffer is larger than the old, fill the balance
              of it with "designer garbage". */
	   if (size > osize) {
	      memset(((char *) buf) + osize, 0x55, (int) (size - osize));
	   }

	   /* All done.  Free and dechain the original buffer. */

	   sm_free(__FILE__, __LINE__, ptr);
	}
        Dmsg4(150, "sm_realloc %d at %x from %s:%d\n", size, buf, fname, lineno);
	sm_check(fname, lineno, True);
	return buf;
}

/*  ACTUALLYMALLOC  --	Call the system malloc() function to obtain
			storage which will eventually be released
			by system or library routines not compiled
			using SMARTALLOC.  */

void *actuallymalloc(unsigned int size)
{
	return malloc(size);
}

/*  ACTUALLYCALLOC  --	Call the system calloc() function to obtain
			storage which will eventually be released
			by system or library routines not compiled
			using SMARTALLOC.  */

void *actuallycalloc(unsigned int nelem, unsigned int elsize)
{
	return calloc(nelem, elsize);
}

/*  ACTUALLYREALLOC  --  Call the system realloc() function to obtain
			 storage which will eventually be released
			 by system or library routines not compiled
			 using SMARTALLOC.  */

void *actuallyrealloc(void *ptr, unsigned int size)
{
	return realloc(ptr, size);
}

/*  ACTUALLYFREE  --  Interface to system free() function to release
		      buffers allocated by low-level routines. */

void actuallyfree(void *cp)
{
	free(cp);
}

/*  SM_DUMP  --  Print orphaned buffers (and dump them if BUFDUMP is
 *		 True).
 *  N.B. DO NOT USE any Bacula print routines (Dmsg, Jmsg, Emsg, ...)
 *    as they have all been shut down at this point.
 */
void sm_dump(Boolean bufdump)
{
	struct abufhead *ap = (struct abufhead *) abqueue.qnext;

	while (ap != (struct abufhead *) &abqueue) {

	   if ((ap == NULL) ||
	       (ap->abq.qnext->qprev != (struct b_queue *) ap) || 
	       (ap->abq.qprev->qnext != (struct b_queue *) ap)) {
	      fprintf(stderr,
                 "\nOrphaned buffers exist.  Dump terminated following\n");
	      fprintf(stderr,
                 "  discovery of bad links in chain of orphaned buffers.\n");
	      fprintf(stderr,
                 "  Buffer address with bad links: %lx\n", (long) ap);
	      break;
	   }

	   if (ap->abfname != NULL) {
	      unsigned memsize = ap->ablen - (HEAD_SIZE + 1);
	      char errmsg[80];

	      sprintf(errmsg,
                "Orphaned buffer:  %6u bytes allocated at line %d of %s %s\n",
		 memsize, ap->ablineno, my_name, ap->abfname
	      );
              fprintf(stderr, "%s", errmsg);
	      if (bufdump) {
		 unsigned llen = 0;
		 char *cp = ((char *) ap) + HEAD_SIZE;

		 errmsg[0] = EOS;
		 while (memsize) {
		    if (llen >= 16) {
                       strcat(errmsg, "\n");
		       llen = 0;
                       fprintf(stderr, "%s", errmsg);
		       errmsg[0] = EOS;
		    }
                    sprintf(errmsg + strlen(errmsg), " %02X",
		       (*cp++) & 0xFF);
		    llen++;
		    memsize--;
		 }
                 fprintf(stderr, "%s\n", errmsg);
	      }
	   }
	   ap = (struct abufhead *) ap->abq.qnext;
	}
}

#undef sm_check
/*  SM_CHECK --  Check the buffers and dump if any damage exists. */
void sm_check(char *fname, int lineno, Boolean bufdump)
{
	if (!sm_check_rtn(fname, lineno, bufdump)) {
           Emsg2(M_ABORT, 0, "Damaged buffer found. Called from %s:%d\n",
	      fname, lineno);
	}
}

#undef sm_check_rtn
/*  SM_CHECK_RTN -- Check the buffers and return 1 if OK otherwise 0 */
int sm_check_rtn(char *fname, int lineno, Boolean bufdump)
{
	struct abufhead *ap = (struct abufhead *) abqueue.qnext;
	int bad, badbuf = 0;

	while (ap != (struct abufhead *) &abqueue) {
	   bad = 0;
	   if ((ap == NULL) ||
	       (ap->abq.qnext->qprev != (struct b_queue *) ap)) {
	      bad = 0x1;
	   }
	   if (ap->abq.qprev->qnext != (struct b_queue *) ap) { 
	      bad |= 0x2;
	   }
	   if (((unsigned char *) ap)[((struct abufhead *) ap)->ablen - 1] !=
		((((long) ap) & 0xFF) ^ 0xC5)) {
	      bad |= 0x4;
	   }
	   badbuf |= bad;
	   if (bad) {
	      Emsg2(M_FATAL, 0, 
                 "\nDamaged buffers found at %s:%d\n", fname, lineno);
	      if (bad & 0x1) {
                 Emsg0(M_FATAL, 0, "  discovery of bad prev link.\n");
	      }
	      if (bad & 0x2) {
                 Emsg0(M_FATAL, 0, "  discovery of bad next link.\n");
	      }
	      if (bad & 0x4) {
                 Emsg0(M_FATAL, 0, "  discovery of data overrun.\n");
	      }

              Emsg1(M_FATAL, 0, "  Buffer address: %lx\n", (long) ap);

	      if (ap->abfname != NULL) {
		 unsigned memsize = ap->ablen - (HEAD_SIZE + 1);
		 char errmsg[80];

		 sprintf(errmsg,
                   "Damaged buffer:  %6u bytes allocated at line %d of %s %s\n",
		    memsize, ap->ablineno, my_name, ap->abfname
		 );
                 Emsg1(M_FATAL, 0, "%s", errmsg);
		 if (bufdump) {
		    unsigned llen = 0;
		    char *cp = ((char *) ap) + HEAD_SIZE;

		    errmsg[0] = EOS;
		    while (memsize) {
		       if (llen >= 16) {
                          strcat(errmsg, "\n");
			  llen = 0;
                          fprintf(stderr, "%s", errmsg);
			  errmsg[0] = EOS;
		       }
		       if (*cp < 0x20) {
                          sprintf(errmsg + strlen(errmsg), " %02X",
			     (*cp++) & 0xFF);
		       } else {
                          sprintf(errmsg + strlen(errmsg), " %c ",
			     (*cp++) & 0xFF);
		       }
		       llen++;
		       memsize--;
		    }
                    Emsg1(M_FATAL, 0, "%s\n", errmsg);
		 }
	      }
	   }
	   ap = (struct abufhead *) ap->abq.qnext;
	}
	return badbuf ? 0 : 1;
}


/*  SM_STATIC  --  Orphaned buffer detection can be disabled  (for  such
		   items  as buffers allocated during initialisation) by
		   calling   sm_static(1).    Normal   orphaned   buffer
		   detection  can be re-enabled with sm_static(0).  Note
		   that all the other safeguards still apply to  buffers
		   allocated  when  sm_static(1)  mode is in effect.  */

void sm_static(int mode)
{
	bufimode = (Boolean) (mode != 0);
}

#endif
