/*
 *  Copyright ( c ) 2012
 *  name : mhogo mchungu
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  ( at your option ) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "walletconfig.h"
#include "ui_walletconfig.h"

#include <QCloseEvent>

#include "dialogmsg.h"
#include "openvolume.h"
#include "tablewidget.h"
#include "walletconfiginput.h"
#include "utility.h"
#include "task.h"

#include <Qt>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

walletconfig::walletconfig( QWidget * parent ) : QDialog( parent ),m_ui( new Ui::walletconfig )
{
	m_ui->setupUi( this ) ;

	this->setFixedSize( this->size() ) ;
	this->setFont( parent->font() ) ;

	m_ui->tableWidget->setColumnWidth( 0,386 ) ;
	m_ui->tableWidget->setColumnWidth( 1,237 ) ;
	m_ui->tableWidget->hideColumn( 2 ) ;

	connect( m_ui->pbAdd,SIGNAL( clicked() ),this,SLOT( pbAdd() ) ) ;
	connect( m_ui->pbClose,SIGNAL( clicked() ),this,SLOT( pbClose() ) ) ;
	connect( m_ui->pbDelete,SIGNAL( clicked() ),this,SLOT( pbDelete() ) ) ;
	connect( m_ui->tableWidget,SIGNAL( currentItemChanged( QTableWidgetItem *,QTableWidgetItem * ) ),
		 this,SLOT(currentItemChanged( QTableWidgetItem *,QTableWidgetItem * ) ) ) ;
	connect( m_ui->tableWidget,SIGNAL( itemClicked( QTableWidgetItem * ) ),
		 this,SLOT( itemClicked( QTableWidgetItem * ) ) ) ;
}

void walletconfig::currentItemChanged( QTableWidgetItem * current,QTableWidgetItem * previous )
{
	tablewidget::selectTableRow( current,previous ) ;
}

void walletconfig::itemClicked( QTableWidgetItem * item )
{
	this->disableAll() ;

	DialogMsg msg( this ) ;

	m_row = item->row() ;

	QString volumeID = m_ui->tableWidget->item( m_row,0 )->text() ;

	int r = msg.ShowUIYesNo( tr( "warning" ),tr( "are you sure you want to delete a volume with an id of \"%1\"?" ).arg( volumeID ) ) ;

	if( r == QMessageBox::Yes ){
		m_action = int( Task::deleteKey ) ;

		Task * t = new Task( m_wallet,volumeID ) ;
		connect( t,SIGNAL( finished() ),this,SLOT( TaskFinished() ) ) ;
		t->start( Task::deleteKey ) ;
	}else{
		this->enableAll() ;
		m_ui->tableWidget->setFocus() ;
	}
}

void walletconfig::pbDelete()
{
	this->itemClicked( m_ui->tableWidget->currentItem() ) ;
}

void walletconfig::pbClose()
{
	this->HideUI() ;
}

void walletconfig::add( QString volumeID,QString comment,QString key )
{
	m_comment  = comment ;
	m_volumeID = volumeID ;

	m_action = int( Task::addKey ) ;

	Task * t = new Task( m_wallet,m_volumeID,key,m_comment ) ;
	connect( t,SIGNAL( finished() ),this,SLOT( TaskFinished() ) ) ;
	t->start( Task::addKey ) ;
}

const QByteArray& walletconfig::getAccInfo( const QString& acc )
{
	int j = m_keys.size() ;

	for( int i = 0 ; i < j ; i++ ){
		if( m_keys.at( i ).getKey() == acc ){
			return m_keys.at( i ).getValue() ;
		}
	}

	/*
	 * we are not supposed to get here
	 */
	return m_bogusEntry ;
}

void walletconfig::TaskFinished()
{
	Task::action r = Task::action( m_action ) ;

	QStringList entry ;

	if( r == Task::addKey ){

		entry.append( m_volumeID ) ;
		entry.append( m_comment ) ;
		entry.append( tr( "<redacted>" ) ) ;

		tablewidget::addRowToTable( m_ui->tableWidget,entry ) ;

	}else if( r == Task::deleteKey ){

		tablewidget::deleteRowFromTable( m_ui->tableWidget,m_row ) ;

	}else if( r == Task::getAllKeys ){
		QTableWidget * table = m_ui->tableWidget ;

		if( m_keys.empty() ){
			;
		}else{
			/*
			 * each volume gets two entries in wallet:
			 * First one in the form of  : entry         -> entry password
			 * Second one in the form of : entry-COMMENT -> comment
			 *
			 * This allows to store a a volume volume,a comment about the volume and the passphrase.
			 *
			 * COMMENT is defined in task.h
			 */

			QStringList s ;
			int j = m_keys.size() ;
			for( int i = 0 ; i < j ; i++ ){
				const QString& acc = m_keys.at( i ).getKey() ;
				if( acc.endsWith( COMMENT ) ){
					;
				}else{
					s.clear() ;
					s.append( acc ) ;
					s.append( this->getAccInfo( acc + QString( COMMENT ) ) ) ;
					s.append( tr( "<redacted>" ) ) ;
					tablewidget::addRowToTable( table,s ) ;
				}
			}
		}
	}else{
		/*
		 * we dont get here
		 */
	}

	this->enableAll() ;
	m_ui->tableWidget->setFocus() ;
}

void walletconfig::cancel()
{
	this->enableAll() ;
}

void walletconfig::pbAdd()
{
	this->disableAll() ;
	walletconfiginput * w = new walletconfiginput( this ) ;
	connect( w,SIGNAL( add( QString,QString,QString ) ),this,SLOT( add( QString,QString,QString ) ) ) ;
	connect( w,SIGNAL( cancel() ),this,SLOT( cancel() ) ) ;
	w->ShowUI() ;
}

void walletconfig::ShowUI( LxQt::Wallet::walletBackEnd backEnd )
{
	this->disableAll() ;
	this->show() ;
	m_wallet = LxQt::Wallet::getWalletBackend( backEnd ) ;
	m_wallet->setInterfaceObject( this ) ;
	if( backEnd == LxQt::Wallet::kwalletBackEnd ){
		m_wallet->open( m_wallet->localDefaultWalletName(),utility::applicationName() ) ;
	}else{
		m_wallet->open( utility::walletName(),utility::applicationName() ) ;
	}
}

void walletconfig::walletIsOpen( bool opened )
{
	if( opened ){
		m_action = int( Task::getAllKeys ) ;

		Task * t = new Task( m_wallet,&m_keys ) ;
		connect( t,SIGNAL( finished() ),this,SLOT( TaskFinished() ) ) ;
		t->start( Task::getAllKeys ) ;
	}else{
		emit couldNotOpenWallet() ;
		this->HideUI() ;
	}
}

void walletconfig::enableAll()
{
	m_ui->groupBox->setEnabled( true ) ;
	m_ui->pbAdd->setEnabled( true ) ;
	m_ui->pbClose->setEnabled( true ) ;
	m_ui->pbDelete->setEnabled( true ) ;
	m_ui->tableWidget->setEnabled( true ) ;
}

void walletconfig::disableAll()
{
	m_ui->groupBox->setEnabled( false ) ;
	m_ui->pbAdd->setEnabled( false ) ;
	m_ui->pbClose->setEnabled( false ) ;
	m_ui->pbDelete->setEnabled( false ) ;
	m_ui->tableWidget->setEnabled( false ) ;
}

void walletconfig::HideUI()
{
	this->hide() ;
	this->deleteLater() ;
}

void walletconfig::closeEvent( QCloseEvent * e )
{
	e->ignore() ;
	this->HideUI() ;
}

walletconfig::~walletconfig()
{
	m_wallet->deleteLater() ;
	delete m_ui ;
}
