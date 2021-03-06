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

#include "erasetask.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <QProcess>
#include <QThreadPool>
#include <QFile>

#include "utility.h"
#include "../zuluCrypt-cli/constants.h"
#include "filetask.h"
#include "socketsendkey.h"

#include <blkid/blkid.h>
#include <string.h>
#include <unistd.h>

EraseTask::EraseTask( QString path )
{
	m_path = path ;
	m_status = 0 ;
}

void EraseTask::start()
{
	QThreadPool::globalInstance()->start( this ) ;
}

int EraseTask::writeJunk()
{
	const int ZSIZE = 512 ;
	QByteArray arraypath = m_path.toLatin1() ;
	int f = open( arraypath.constData(),O_RDONLY ) ;
	if( f == -1 ){
		return 8;
	}
	qint64 size = ( qint64 ) blkid_get_dev_size( f )   ;
	close( f ) ;

	QFile file( m_path ) ;
	if( !file.open( QIODevice::WriteOnly ) ){
		return 8 ;
	}
	char buffer[ SIZE ] ;

	QFile random( QString( "/dev/urandom" ) ) ;
	random.open( QIODevice::ReadOnly ) ;

	qint64 size_written = 0 ;

	int j = 0 ;
	int k ;
	do{
		if( m_status == 5 ){
			break ;
		}
		random.read( buffer,ZSIZE ) ;
		file.write( buffer,ZSIZE ) ;
		file.flush() ;

		size_written += ZSIZE ;

		k = ( int ) ( size_written * 100 / size ) ;

		if( k > j ){
			emit progress( k ) ;
		}
		j = k ;
	}while( size_written < size ) ;

	file.close() ;
	random.close() ;

	return 0 ;
}

void EraseTask::run()
{
	/*
	 * RANDOM_SOURCE is set at createfilethread.h
	 */
	if( RANDOM_SOURCE == 1 ){
		/*
		 * write random data using data from reading "/dev/urandom", slow
		 * but dependable
		 */
		m_status = this->writeJunk() ;
	}else{
		/*
		 * write raandom data using cryptsetup,much faster but
		 * hangs on some kernels when the data to be written is large enough
		 */
		m_status = this->openMapper() ;

		if( m_status == 0 ){
			this->writeJunkThroughMapper() ;
			this->closeMapper() ;
		}
	}
}

void EraseTask::writeJunkThroughMapper()
{
	QString path = utility::mapperPath( m_path ) ;
	QFile fd( path ) ;
	if( !fd.open( QIODevice::WriteOnly ) ){
		return ;
	}
	const int ZSIZE = 1024 ;

	qint64 size_written = 0 ;

	char buffer[ ZSIZE ];

	memset( buffer,0,ZSIZE ) ;

	int i ;
	int j = -1 ;

	QByteArray arraypath = path.toLatin1() ;
	int f = open( arraypath.constData(),O_RDONLY ) ;
	if( f == -1 ){
		return ;
	}
	qint64 dev_size = ( qint64 ) blkid_get_dev_size( f )  ;
	close( f ) ;

	while( fd.write( buffer,ZSIZE ) > 0 ){

		if( m_status == 5 ){
			break ;
		}
		fd.flush() ;

		size_written += ZSIZE ;

		i = ( int ) ( size_written * 100 / dev_size ) ;

		if( i > j ){
			emit progress( i ) ;
			j = i ;
		}
	}

	fd.close() ;
}

void EraseTask::closeMapper()
{
	QString exe = QString( "%1 -q -d \"%2\"" ).arg( ZULUCRYPTzuluCrypt ).arg( m_path ) ;

	QProcess p ;
	p.start( exe ) ;
	p.waitForFinished() ;
	p.close() ;
}

int EraseTask::openMapper()
{
	QString exe = QString( "%1 -k -J -d \"%2\"" ).arg( ZULUCRYPTzuluCrypt ).arg( m_path ) ;

	QProcess p ;
	p.start( exe ) ;
	p.waitForFinished() ;
	int st = p.exitCode() ;
	p.close() ;

	return st ;
}

void EraseTask::cancel()
{
	m_status = 5 ;
}

EraseTask::~EraseTask()
{
	emit exitStatus( m_status ) ;
}
