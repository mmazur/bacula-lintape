/*******************************************************************************
 *   IBM_tape.h
 *
 *   (C) Copyright International Business Machines  Corp., 2001-2012
 *   This file is part of the IBM Linux Enhanced SCSI Tape and
 *   Medium Changer Device Driver
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License version 2.1
 *   as published by the Free Software Foundation.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 ******************************************************************************/

#ifndef IBM_TAPE_H
#define IBM_TAPE_H

/*******************************************************************************
* IOCTL Structures                                                             *
*******************************************************************************/
#ifndef unchar
#define unchar unsigned char
#endif

#ifndef boolean
#define boolean unchar
#endif

#ifndef BYTE
#define BYTE unchar
#endif

#define SIOC_REQSENSE                 _IOR ('C',0x02,struct request_sense)

struct request_sense {
	uint valid : 1,                 /* information field is valid         */
		err_code : 7;           /* error code                         */
	unchar segnum;                  /* segment number                     */
	uint fm : 1,                    /* file mark detected                 */
		eom : 1,                /* end of medium                      */
		ili : 1,                /* incorrect length indicator         */
		resvd1 : 1,             /* reserved                           */
		key : 4;                /* sense key                          */
	int info;                       /* information bytes                  */
	unchar addlen;                  /* additional sense length            */
	uint cmdinfo;                   /* command specific information       */
	unchar asc;                     /* additional sense code              */
	unchar ascq;                    /* additional sense code qualifier    */
	unchar fru;                     /* field replaceable unit code        */
	uint sksv : 1,                  /* sense key specific valid           */
		cd : 1,                 /* control/data                       */
		resvd2 : 2,             /* reserved                           */
		bpv : 1,                /* bit pointer valid                  */
		sim : 3;                /* system information message         */
	unchar field[2];                /* field pointer                      */
	unchar vendor[109];             /* vendor specific (padded to 127)    */
};

#endif
