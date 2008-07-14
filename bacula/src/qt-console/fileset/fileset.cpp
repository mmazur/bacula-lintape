/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
 
/*
 *   Version $Id$
 *
 *  FileSet Class
 *
 *   Dirk Bartley, March 2007
 *
 */ 

#include <QAbstractEventDispatcher>
#include <QMenu>
#include "bat.h"
#include "fileset/fileset.h"
#include "util/fmtwidgetitem.h"

FileSet::FileSet()
{
   setupUi(this);
   m_name = tr("FileSets");
   pgInitialize();
   QTreeWidgetItem* thisitem = mainWin->getFromHash(this);
   thisitem->setIcon(0,QIcon(QString::fromUtf8(":images/system-file-manager.png")));

   /* tableWidget, FileSet Tree Tree Widget inherited from ui_fileset.h */
   m_populated = false;
   m_checkcurwidget = true;
   m_closeable = false;
   readSettings();
   /* add context sensitive menu items specific to this classto the page
    * selector tree. m_contextActions is QList of QActions */
   m_contextActions.append(actionRefreshFileSet);
   dockPage();
}

FileSet::~FileSet()
{
   writeSettings();
}

/*
 * The main meat of the class!!  The function that querries the director and 
 * creates the widgets with appropriate values.
 */
void FileSet::populateTable()
{
   QBrush blackBrush(Qt::black);

   if (!m_console->preventInUseConnect())
       return;

   m_checkcurwidget = false;
   tableWidget->clear();
   m_checkcurwidget = true;

   QStringList headerlist = (QStringList() << tr("FileSet Name") << tr("FileSet Id")
       << tr("Create Time"));

   tableWidget->setColumnCount(headerlist.count());
   tableWidget->setHorizontalHeaderLabels(headerlist);
   tableWidget->horizontalHeader()->setHighlightSections(false);
   tableWidget->setRowCount(m_console->fileset_list.count());
   tableWidget->verticalHeader()->hide();
   tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
   tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
   tableWidget->setSortingEnabled(false); /* rows move on insert if sorting enabled */
   int row = 0;

   foreach(QString filesetName, m_console->fileset_list) {

      /* Set up query QString and header QStringList */
      QString query("");
      query += "SELECT FileSet AS Name, FileSetId AS Id, CreateTime"
           " FROM FileSet"
           " WHERE ";
      query += " FileSet='" + filesetName + "'";
      query += " ORDER BY CreateTime DESC LIMIT 1";

      QStringList results;
      if (mainWin->m_sqlDebug) {
         Pmsg1(000, "FileSet query cmd : %s\n",query.toUtf8().data());
      }
      if (m_console->sql_cmd(query, results)) {
         int resultCount = results.count();
         if (resultCount) {
            /* only use the last one */
            QString resultline = results[resultCount - 1];
            QStringList fieldlist = resultline.split("\t");

	    TableItemFormatter item(*tableWidget, row);
  
	    /* Iterate through fields in the record */
	    QStringListIterator fld(fieldlist);
	    int col = 0;

	    /* name */
	    item.setTextFld(col++, fld.next());

	    /* id */
	    item.setNumericFld(col++, fld.next());

	    /* creation time */
	    item.setTextFld(col++, fld.next());

         }
      }
      row++;
   }
   /* set default sorting */
   tableWidget->sortByColumn(headerlist.indexOf(tr("Create Time")), Qt::DescendingOrder);
   tableWidget->setSortingEnabled(true);
   
   /* Resize rows and columns */
   tableWidget->resizeColumnsToContents();
   tableWidget->resizeRowsToContents();
}

/*
 * When the treeWidgetItem in the page selector tree is singleclicked, Make sure
 * The tree has been populated.
 */
void FileSet::PgSeltreeWidgetClicked()
{
   if (!m_populated) {
      populateTable();
      createContextMenu();
      m_populated = true;
   }
}

/*
 * Added to set the context menu policy based on currently active treeWidgetItem
 * signaled by currentItemChanged
 */
void FileSet::tableItemChanged(QTableWidgetItem *currentwidgetitem, QTableWidgetItem *previouswidgetitem)
{
   /* m_checkcurwidget checks to see if this is during a refresh, which will segfault */
   if (m_checkcurwidget) {
      int currentRow = currentwidgetitem->row();
      QTableWidgetItem *currentrowzeroitem = tableWidget->item(currentRow, 0);
      m_currentlyselected = currentrowzeroitem->text();

      /* The Previous item */
      if (previouswidgetitem) { /* avoid a segfault if first time */
         tableWidget->removeAction(actionStatusFileSetInConsole);
         tableWidget->removeAction(actionShowJobs);
      }

      if (m_currentlyselected.length() != 0) {
         /* set a hold variable to the fileset name in case the context sensitive
          * menu is used */
         tableWidget->addAction(actionStatusFileSetInConsole);
         tableWidget->addAction(actionShowJobs);
      }
   }
}

/* 
 * Setup a context menu 
 * Made separate from populate so that it would not create context menu over and
 * over as the tree is repopulated.
 */
void FileSet::createContextMenu()
{
   tableWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
   tableWidget->addAction(actionRefreshFileSet);
   connect(tableWidget, SIGNAL(
           currentItemChanged(QTableWidgetItem *, QTableWidgetItem *)),
           this, SLOT(tableItemChanged(QTableWidgetItem *, QTableWidgetItem *)));
   /* connect to the action specific to this pages class */
   connect(actionRefreshFileSet, SIGNAL(triggered()), this,
                SLOT(populateTable()));
   connect(actionStatusFileSetInConsole, SIGNAL(triggered()), this,
                SLOT(consoleStatusFileSet()));
   connect(actionShowJobs, SIGNAL(triggered()), this,
                SLOT(showJobs()));
}

/*
 * Function responding to actionListJobsofFileSet which calls mainwin function
 * to create a window of a list of jobs of this fileset.
 */
void FileSet::consoleStatusFileSet()
{
   QString cmd("status fileset=");
   cmd += m_currentlyselected;
   consoleCommand(cmd);
}

/*
 * Virtual function which is called when this page is visible on the stack
 */
void FileSet::currentStackItem()
{
   if(!m_populated) {
      populateTable();
      /* Create the context menu for the fileset table */
      createContextMenu();
      m_populated=true;
   }
}

/*
 * Save user settings associated with this page
 */
void FileSet::writeSettings()
{
   QSettings settings(m_console->m_dir->name(), "bat");
   settings.beginGroup("FileSet");
   settings.setValue("geometry", saveGeometry());
   settings.endGroup();
}

/*
 * Read and restore user settings associated with this page
 */
void FileSet::readSettings()
{ 
   QSettings settings(m_console->m_dir->name(), "bat");
   settings.beginGroup("FileSet");
   restoreGeometry(settings.value("geometry").toByteArray());
   settings.endGroup();
}

/*
 * Create a JobList object pre-populating a fileset
 */
void FileSet::showJobs()
{
   QTreeWidgetItem *parentItem = mainWin->getFromHash(this);
   mainWin->createPageJobList("", "", "", m_currentlyselected, parentItem);
}
