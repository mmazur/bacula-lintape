/*
 * Bacula Catalog Database Get record interface routines
 *  Note, these routines generally get a record by id or
 *	  by name.  If more logic is involved, the routine
 *	  should be in find.c 
 *
 *    Kern Sibbald, March 2000
 *
 *    Version $Id$
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

/* *****FIXME**** fix fixed length of select_cmd[] and insert_cmd[] */

/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C 		      /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"

#if    HAVE_MYSQL || HAVE_SQLITE

/* -----------------------------------------------------------------------
 *
 *   Generic Routines (or almost generic)
 *
 * -----------------------------------------------------------------------
 */

/* Forward referenced functions */
static int db_get_file_record(B_DB *mdb, FILE_DBR *fdbr);
static int db_get_filename_record(B_DB *mdb, char *fname);
static int db_get_path_record(B_DB *mdb, char *path);


/* Imported subroutines */
extern void print_result(B_DB *mdb);
extern int QueryDB(char *file, int line, B_DB *db, char *select_cmd);


/*
 * Given a full filename (with path), look up the File record
 * (with attributes) in the database.
 *
 *  Returns: 0 on failure
 *	     1 on success with the File record in FILE_DBR
 */
int db_get_file_attributes_record(B_DB *mdb, char *fname, FILE_DBR *fdbr)
{
   int fnl, pnl;
   char *l, *p;
   int stat;
   char file[MAXSTRING];
   char spath[MAXSTRING];
   char buf[MAXSTRING];
   Dmsg1(20, "Enter get_file_from_catalog fname=%s \n", fname);

   /* Find path without the filename.  
    * I.e. everything after the last / is a "filename".
    * OK, maybe it is a directory name, but we treat it like
    * a filename. If we don't find a / then the whole name
    * must be a path name (e.g. c:).
    */
   for (p=l=fname; *p; p++) {
      if (*p == '/') {
	 l = p;
      }
   }
   if (*l == '/') {                   /* did we find a slash? */
      l++;			      /* yes, point to filename */
   } else {			      /* no, whole thing must be path name */
      l = p;
   }

   /* If filename doesn't exist (i.e. root directory), we
    * simply create a blank name consisting of a single 
    * space. This makes handling zero length filenames
    * easier.
    */
   fnl = p - l;
   if (fnl > 255) {
      Jmsg1(mdb->jcr, M_WARNING, 0, _("Filename truncated to 255 chars: %s\n"), l);
      fnl = 255;
   }
   if (fnl > 0) {
      strncpy(file, l, fnl);	      /* copy filename */
      file[fnl] = 0;
   } else {
      file[0] = ' ';                  /* blank filename */
      file[1] = 0;
      fnl = 1;
   }

   pnl = l - fname;    
   if (pnl > 255) {
      Jmsg1(mdb->jcr, M_WARNING, 0, _("Path name truncated to 255 chars: %s\n"), fname);
      pnl = 255;
   }
   strncpy(spath, fname, pnl);
   spath[pnl] = 0;

   if (pnl == 0) {
      Mmsg1(&mdb->errmsg, _("Path length is zero. File=%s\n"), fname);
      Jmsg(mdb->jcr, M_ERROR, 0, "%s", mdb->errmsg);
      spath[0] = ' ';
      spath[1] = 0;
      pnl = 1;
   }

   Dmsg1(400, "spath=%s\n", spath);
   Dmsg1(400, "file=%s\n", file);

   db_escape_string(buf, file, fnl);
   fdbr->FilenameId = db_get_filename_record(mdb, buf);
   Dmsg2(400, "db_get_filename_record FilenameId=%u file=%s\n", fdbr->FilenameId, buf);

   db_escape_string(buf, spath, pnl);
   fdbr->PathId = db_get_path_record(mdb, buf);
   Dmsg2(400, "db_get_path_record PathId=%u path=%s\n", fdbr->PathId, buf);

   stat = db_get_file_record(mdb, fdbr);

   return stat;
}

 
/*
 * Get a File record   
 * Returns: 0 on failure
 *	    1 on success
 *
 *  DO NOT use Jmsg in this routine.
 *
 *  Note in this routine, we do not use Jmsg because it may be
 *    called to get attributes of a non-existent file, which is
 *    "normal" if a new file is found during Verify.
 */
static
int db_get_file_record(B_DB *mdb, FILE_DBR *fdbr)
{
   SQL_ROW row;
   int stat = 0;

   db_lock(mdb);
   Mmsg(&mdb->cmd, 
"SELECT FileId, LStat, MD5 from File where File.JobId=%u and File.PathId=%u and \
File.FilenameId=%u", fdbr->JobId, fdbr->PathId, fdbr->FilenameId);

   Dmsg3(050, "Get_file_record JobId=%u FilenameId=%u PathId=%u\n",
      fdbr->JobId, fdbr->FilenameId, fdbr->PathId);
      
   Dmsg1(100, "Query=%s\n", mdb->cmd);

   if (QUERY_DB(mdb, mdb->cmd)) {

      mdb->num_rows = sql_num_rows(mdb);
      Dmsg1(050, "get_file_record num_rows=%d\n", (int)mdb->num_rows);

      if (mdb->num_rows > 1) {
         Mmsg1(&mdb->errmsg, _("get_file_record want 1 got rows=%d\n"),
	    mdb->num_rows);
      }
      if (mdb->num_rows >= 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("Error fetching row: %s\n"), sql_strerror(mdb));
	 } else {
	    fdbr->FileId = (FileId_t)strtod(row[0], NULL);
	    strncpy(fdbr->LStat, row[1], sizeof(fdbr->LStat));
	    fdbr->LStat[sizeof(fdbr->LStat)] = 0;
	    strncpy(fdbr->MD5, row[2], sizeof(fdbr->MD5));
	    fdbr->MD5[sizeof(fdbr->MD5)] = 0;
	    stat = 1;
	 }
      } else {
         Mmsg2(&mdb->errmsg, _("File record not found for PathId=%u FilenameId=%u\n"),
	    fdbr->PathId, fdbr->FilenameId);
      }
      sql_free_result(mdb);
   }
   db_unlock(mdb);
   return stat;

}

/* Get Filename record	 
 * Returns: 0 on failure
 *	    FilenameId on success
 *
 *   DO NOT use Jmsg in this routine (see notes for get_file_record)
 */
static int db_get_filename_record(B_DB *mdb, char *fname) 
{
   SQL_ROW row;
   int FilenameId = 0;

   if (*fname == 0) {
      Mmsg0(&mdb->errmsg, _("Null name given to db_get_filename_record\n"));
      Emsg0(M_ABORT, 0, mdb->errmsg);
   }

   db_lock(mdb);
   Mmsg(&mdb->cmd, "SELECT FilenameId FROM Filename WHERE Name='%s'", fname);
   if (QUERY_DB(mdb, mdb->cmd)) {

      mdb->num_rows = sql_num_rows(mdb);

      if (mdb->num_rows > 1) {
         Mmsg1(&mdb->errmsg, _("More than one Filename!: %d\n"), (int)(mdb->num_rows));
      }
      if (mdb->num_rows >= 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
	 } else {
	    FilenameId = atoi(row[0]);
	    if (FilenameId <= 0) {
               Mmsg2(&mdb->errmsg, _("Get DB Filename record %s found bad record: %d\n"),
		  mdb->cmd, FilenameId); 
	       FilenameId = 0;
	    }
	 }
      } else {
         Mmsg1(&mdb->errmsg, _("Filename record: %s not found.\n"), fname);
      }
      sql_free_result(mdb);
   }
   db_unlock(mdb);
   return FilenameId;
}

/* Get path record   
 * Returns: 0 on failure
 *	    PathId on success
 *
 *   DO NOT use Jmsg in this routine (see notes for get_file_record)
 */
static int db_get_path_record(B_DB *mdb, char *path)
{
   SQL_ROW row;
   uint32_t PathId = 0;

   if (*path == 0) {
      Emsg0(M_ABORT, 0, _("Null path given to db_get_path_record\n"));
   }

   db_lock(mdb);

   if (mdb->cached_path_id != 0 && strcmp(mdb->cached_path, path) == 0) {
      db_unlock(mdb);
      return mdb->cached_path_id;
   }	      

   Mmsg(&mdb->cmd, "SELECT PathId FROM Path WHERE Path='%s'", path);

   if (QUERY_DB(mdb, mdb->cmd)) {
      char ed1[30];
      mdb->num_rows = sql_num_rows(mdb);

      if (mdb->num_rows > 1) {
         Mmsg1(&mdb->errmsg, _("More than one Path!: %s\n"), 
	    edit_uint64(mdb->num_rows, ed1));
         Jmsg(mdb->jcr, M_ERROR, 0, "%s", mdb->errmsg);
      } else if (mdb->num_rows == 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
	 } else {
	    PathId = atoi(row[0]);
	    if (PathId <= 0) {
               Mmsg2(&mdb->errmsg, _("Get DB path record %s found bad record: %u\n"),
		  mdb->cmd, PathId); 
	       PathId = 0;
	    } else {
	       /* Cache path */
	       if (PathId != mdb->cached_path_id) {
		  mdb->cached_path_id = PathId;
		  mdb->cached_path = check_pool_memory_size(mdb->cached_path,
		     strlen(path)+1);
		  strcpy(mdb->cached_path, path);
	       }
	    }
	 }
      } else {	
         Mmsg1(&mdb->errmsg, _("Path record: %s not found.\n"), path);
      }
      sql_free_result(mdb);
   }
   db_unlock(mdb);
   return PathId;
}


/* 
 * Get Job record for given JobId or Job name
 * Returns: 0 on failure
 *	    1 on success
 */
int db_get_job_record(B_DB *mdb, JOB_DBR *jr)
{
   SQL_ROW row;

   db_lock(mdb);
   if (jr->JobId == 0) {
      Mmsg(&mdb->cmd, "SELECT VolSessionId,VolSessionTime,\
PoolId,StartTime,EndTime,JobFiles,JobBytes,JobTDate,Job,JobStatus,\
Type,Level \
FROM Job WHERE Job='%s'", jr->Job);
    } else {
      Mmsg(&mdb->cmd, "SELECT VolSessionId,VolSessionTime,\
PoolId,StartTime,EndTime,JobFiles,JobBytes,JobTDate,Job,JobStatus,\
Type,Level \
FROM Job WHERE JobId=%u", jr->JobId);
    }

   if (!QUERY_DB(mdb, mdb->cmd)) {
      db_unlock(mdb);
      return 0; 		      /* failed */
   }
   if ((row = sql_fetch_row(mdb)) == NULL) {
      Mmsg1(&mdb->errmsg, _("No Job found for JobId %u\n"), jr->JobId);
      sql_free_result(mdb);
      db_unlock(mdb);
      return 0; 		      /* failed */
   }

   jr->VolSessionId = atol(row[0]);
   jr->VolSessionTime = atol(row[1]);
   jr->PoolId = atoi(row[2]);
   bstrncpy(jr->cStartTime, row[3]!=NULL?row[3]:"", sizeof(jr->cStartTime));
   bstrncpy(jr->cEndTime, row[4]!=NULL?row[4]:"", sizeof(jr->cEndTime));
   jr->JobFiles = atol(row[5]);
   jr->JobBytes = (uint64_t)strtod(row[6], NULL);
   jr->JobTDate = (utime_t)strtod(row[7], NULL);
   bstrncpy(jr->Job, row[8]!=NULL?row[8]:"", sizeof(jr->Job));
   jr->JobStatus = (int)*row[9];
   jr->Type = (int)*row[10];
   jr->Level = (int)*row[11];
   sql_free_result(mdb);

   db_unlock(mdb);
   return 1;
}

/*
 * Find VolumeNames for a give JobId
 *  Returns: 0 on error or no Volumes found
 *	     number of volumes on success
 *		Volumes are concatenated in VolumeNames
 *		separated by a vertical bar (|).
 *
 *  Returns: number of volumes on success
 */
int db_get_job_volume_names(B_DB *mdb, uint32_t JobId, POOLMEM **VolumeNames)
{
   SQL_ROW row;
   int stat = 0;
   int i;

   db_lock(mdb);
   Mmsg(&mdb->cmd, 
"SELECT VolumeName FROM JobMedia,Media WHERE JobMedia.JobId=%u \
AND JobMedia.MediaId=Media.MediaId", JobId);

   Dmsg1(130, "VolNam=%s\n", mdb->cmd);
   *VolumeNames[0] = 0;
   if (QUERY_DB(mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      Dmsg1(130, "Num rows=%d\n", mdb->num_rows);
      if (mdb->num_rows <= 0) {
         Mmsg1(&mdb->errmsg, _("No volumes found for JobId=%d\n"), JobId);
	 stat = 0;
      } else {
	 stat = mdb->num_rows;
	 for (i=0; i < stat; i++) {
	    if ((row = sql_fetch_row(mdb)) == NULL) {
               Mmsg2(&mdb->errmsg, _("Error fetching row %d: ERR=%s\n"), i, sql_strerror(mdb));
               Jmsg(mdb->jcr, M_ERROR, 0, "%s", mdb->errmsg);
	       stat = 0;
	       break;
	    } else {
	       if (*VolumeNames[0] != 0) {
                  pm_strcat(VolumeNames, "|");
	       }
	       pm_strcat(VolumeNames, row[0]);
	    }
	 }
      }
      sql_free_result(mdb);
   }
   db_unlock(mdb);
   return stat;
}


/* 
 * Get the number of pool records
 *
 * Returns: -1 on failure
 *	    number on success
 */
int db_get_num_pool_records(B_DB *mdb)
{
   int stat = 0;

   db_lock(mdb);
   Mmsg(&mdb->cmd, "SELECT count(*) from Pool");
   stat = get_sql_record_max(mdb);
   db_unlock(mdb);
   return stat;
}

/*
 * This function returns a list of all the Pool record ids.
 *  The caller must free ids if non-NULL.
 *
 *  Returns 0: on failure
 *	    1: on success
 */
int db_get_pool_ids(B_DB *mdb, int *num_ids, uint32_t *ids[])
{
   SQL_ROW row;
   int stat = 0;
   int i = 0;
   uint32_t *id;

   db_lock(mdb);
   *ids = NULL;
   Mmsg(&mdb->cmd, "SELECT PoolId FROM Pool");
   if (QUERY_DB(mdb, mdb->cmd)) {
      *num_ids = sql_num_rows(mdb);
      if (*num_ids > 0) {
	 id = (uint32_t *)malloc(*num_ids * sizeof(uint32_t));
	 while ((row = sql_fetch_row(mdb)) != NULL) {
	    id[i++] = (uint32_t)atoi(row[0]);
	 }
	 *ids = id;
      }
      sql_free_result(mdb);
      stat = 1;
   } else {
      Mmsg(&mdb->errmsg, _("Pool id select failed: ERR=%s\n"), sql_strerror(mdb));
      Jmsg(mdb->jcr, M_ERROR, 0, "%s", mdb->errmsg);
      stat = 0;
   }
   db_unlock(mdb);
   return stat;
}


/* Get Pool Record   
 * If the PoolId is non-zero, we get its record,
 *  otherwise, we search on the PoolName
 *
 * Returns: 0 on failure
 *	    id on success 
 */
int db_get_pool_record(B_DB *mdb, POOL_DBR *pdbr)
{
   SQL_ROW row;
   int stat = 0;

   db_lock(mdb);
   if (pdbr->PoolId != 0) {		  /* find by id */
      Mmsg(&mdb->cmd, 
"SELECT PoolId,Name,NumVols,MaxVols,UseOnce,UseCatalog,AcceptAnyVolume,\
AutoPrune,Recycle,VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,\
PoolType,LabelFormat FROM Pool WHERE Pool.PoolId=%d", pdbr->PoolId);
   } else {			      /* find by name */
      Mmsg(&mdb->cmd, 
"SELECT PoolId,Name,NumVols,MaxVols,UseOnce,UseCatalog,AcceptAnyVolume,\
AutoPrune,Recycle,VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,\
PoolType,LabelFormat FROM Pool WHERE Pool.Name='%s'", pdbr->Name);
   }  

   if (QUERY_DB(mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
	 char ed1[30];
         Mmsg1(&mdb->errmsg, _("More than one Pool!: %s\n"), 
	    edit_uint64(mdb->num_rows, ed1));
         Jmsg(mdb->jcr, M_ERROR, 0, "%s", mdb->errmsg);
      } else if (mdb->num_rows == 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
            Jmsg(mdb->jcr, M_ERROR, 0, "%s", mdb->errmsg);
	 } else {
	    pdbr->PoolId = atoi(row[0]);
            bstrncpy(pdbr->Name, row[1]!=NULL?row[1]:"", sizeof(pdbr->Name));
	    pdbr->NumVols = atoi(row[2]);
	    pdbr->MaxVols = atoi(row[3]);
	    pdbr->UseOnce = atoi(row[4]);
	    pdbr->UseCatalog = atoi(row[5]);
	    pdbr->AcceptAnyVolume = atoi(row[6]);
	    pdbr->AutoPrune = atoi(row[7]);
	    pdbr->Recycle = atoi(row[8]);
	    pdbr->VolRetention = (utime_t)strtod(row[9], NULL);
	    pdbr->VolUseDuration = (utime_t)strtod(row[10], NULL);
	    pdbr->MaxVolJobs = atoi(row[11]);
	    pdbr->MaxVolFiles = atoi(row[12]);
            bstrncpy(pdbr->PoolType, row[13]!=NULL?row[13]:"", sizeof(pdbr->PoolType));
            bstrncpy(pdbr->LabelFormat, row[14]!=NULL?row[14]:"", sizeof(pdbr->LabelFormat));
	    stat = pdbr->PoolId;
	 }
      }
      sql_free_result(mdb);
   }
   db_unlock(mdb);
   return stat;
}

/* Get FileSet Record	
 * If the FileSetId is non-zero, we get its record,
 *  otherwise, we search on the name
 *
 * Returns: 0 on failure
 *	    id on success 
 */
int db_get_fileset_record(B_DB *mdb, FILESET_DBR *fsr)
{
   SQL_ROW row;
   int stat = 0;

   db_lock(mdb);
   if (fsr->FileSetId != 0) {		    /* find by id */
      Mmsg(&mdb->cmd, 
           "SELECT FileSetId, FileSet, MD5 FROM FileSet "
           "WHERE FileSetId=%u", fsr->FileSetId);
   } else {			      /* find by name */
      Mmsg(&mdb->cmd, 
           "SELECT FileSetId, FileSet, MD5 FROM FileSet "
           "WHERE FileSet='%s'", fsr->FileSet);
   }  

   if (QUERY_DB(mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
	 char ed1[30];
         Mmsg1(&mdb->errmsg, _("Error got %s FileSets but expected only one!\n"), 
	    edit_uint64(mdb->num_rows, ed1));
	 sql_data_seek(mdb, mdb->num_rows-1);
      }
      if ((row = sql_fetch_row(mdb)) == NULL) {
         Mmsg1(&mdb->errmsg, _("Error: FileSet record \"%s\" not found\n"), fsr->FileSet);
      } else {
	 fsr->FileSetId = atoi(row[0]);
         bstrncpy(fsr->FileSet, row[1]!=NULL?row[1]:"", sizeof(fsr->FileSet));
         bstrncpy(fsr->MD5, row[2]!=NULL?row[2]:"", sizeof(fsr->MD5));
	 stat = fsr->FileSetId;
      }
      sql_free_result(mdb);
   }
   db_unlock(mdb);
   return stat;
}


/* 
 * Get the number of Media records
 *
 * Returns: -1 on failure
 *	    number on success
 */
int db_get_num_media_records(B_DB *mdb)
{
   int stat = 0;

   db_lock(mdb);
   Mmsg(&mdb->cmd, "SELECT count(*) from Media");
   stat = get_sql_record_max(mdb);
   db_unlock(mdb);
   return stat;
}


/*
 * This function returns a list of all the Media record ids.
 *  The caller must free ids if non-NULL.
 *
 *  Returns 0: on failure
 *	    1: on success
 */
int db_get_media_ids(B_DB *mdb, int *num_ids, uint32_t *ids[])
{
   SQL_ROW row;
   int stat = 0;
   int i = 0;
   uint32_t *id;

   db_lock(mdb);
   *ids = NULL;
   Mmsg(&mdb->cmd, "SELECT MediaId FROM Media");
   if (QUERY_DB(mdb, mdb->cmd)) {
      *num_ids = sql_num_rows(mdb);
      if (*num_ids > 0) {
	 id = (uint32_t *)malloc(*num_ids * sizeof(uint32_t));
	 while ((row = sql_fetch_row(mdb)) != NULL) {
	    id[i++] = (uint32_t)atoi(row[0]);
	 }
	 *ids = id;
      }
      sql_free_result(mdb);
      stat = 1;
   } else {
      Mmsg(&mdb->errmsg, _("Media id select failed: ERR=%s\n"), sql_strerror(mdb));
      Jmsg(mdb->jcr, M_ERROR, 0, "%s", mdb->errmsg);
      stat = 0;
   }
   db_unlock(mdb);
   return stat;
}


/* Get Media Record   
 *
 * Returns: 0 on failure
 *	    id on success 
 */
int db_get_media_record(B_DB *mdb, MEDIA_DBR *mr)
{
   SQL_ROW row;
   int stat = 0;

   db_lock(mdb);
   if (mr->MediaId == 0 && mr->VolumeName[0] == 0) {
      Mmsg(&mdb->cmd, "SELECT count(*) from Media");
      mr->MediaId = get_sql_record_max(mdb);
      db_unlock(mdb);
      return 1;
   }
   if (mr->MediaId != 0) {		 /* find by id */
      Mmsg(&mdb->cmd, "SELECT MediaId,VolumeName,VolJobs,VolFiles,VolBlocks,\
VolBytes,VolMounts,VolErrors,VolWrites,MaxVolBytes,VolCapacityBytes,\
MediaType,VolStatus,PoolId,VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,\
Recycle,Slot, FirstWritten \
FROM Media WHERE MediaId=%d", mr->MediaId);
   } else {			      /* find by name */
      Mmsg(&mdb->cmd, "SELECT MediaId,VolumeName,VolJobs,VolFiles,VolBlocks,\
VolBytes,VolMounts,VolErrors,VolWrites,MaxVolBytes,VolCapacityBytes,\
MediaType,VolStatus,PoolId,VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,\
Recycle,Slot,FirstWritten \
FROM Media WHERE VolumeName='%s'", mr->VolumeName);
   }  

   if (QUERY_DB(mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
	 char ed1[30];
         Mmsg1(&mdb->errmsg, _("More than one Volume!: %s\n"), 
	    edit_uint64(mdb->num_rows, ed1));
         Jmsg(mdb->jcr, M_ERROR, 0, "%s", mdb->errmsg);
      } else if (mdb->num_rows == 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
            Jmsg(mdb->jcr, M_ERROR, 0, "%s", mdb->errmsg);
	 } else {
	    /* return values */
	    mr->MediaId = atoi(row[0]);
            bstrncpy(mr->VolumeName, row[1]!=NULL?row[1]:"", sizeof(mr->VolumeName));
	    mr->VolJobs = atoi(row[2]);
	    mr->VolFiles = atoi(row[3]);
	    mr->VolBlocks = atoi(row[4]);
	    mr->VolBytes = (uint64_t)strtod(row[5], NULL);
	    mr->VolMounts = atoi(row[6]);
	    mr->VolErrors = atoi(row[7]);
	    mr->VolWrites = atoi(row[8]);
	    mr->MaxVolBytes = (uint64_t)strtod(row[9], NULL);
	    mr->VolCapacityBytes = (uint64_t)strtod(row[10], NULL);
            bstrncpy(mr->MediaType, row[11]!=NULL?row[11]:"", sizeof(mr->MediaType));
            bstrncpy(mr->VolStatus, row[12]!=NULL?row[12]:"", sizeof(mr->VolStatus));
	    mr->PoolId = atoi(row[13]);
	    mr->VolRetention = (utime_t)strtod(row[14], NULL);
	    mr->VolUseDuration = (utime_t)strtod(row[15], NULL);
	    mr->MaxVolJobs = atoi(row[16]);
	    mr->MaxVolFiles = atoi(row[17]);
	    mr->Recycle = atoi(row[18]);
	    mr->Slot = atoi(row[19]);
            bstrncpy(mr->cFirstWritten, row[20]!=NULL?row[20]:"", sizeof(mr->cFirstWritten));
	    stat = mr->MediaId;
	 }
      } else {
         Mmsg0(&mdb->errmsg, _("Media record not found.\n"));
      }
      sql_free_result(mdb);
   }
   db_unlock(mdb);
   return stat;
}


#endif /* HAVE_MYSQL || HAVE_SQLITE */
