/*
 *
 *  Copyright (c) 2013
 *  name : mhogo mchungu
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "auto_mount.h"
#include <QDebug>

#include <QString>
#include <QObject>
#include <QThread>
#include <QProcess>
#include <QStringList>
#include <QDir>

#include "bin_path.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "auto_mount_helper.h"
/*
http://linux.die.net/man/7/inotify
http://darkeside.blogspot.com/2007/12/linux-inotify-example.html
 */

#define stringPrefixMatch( x,y,z ) strncmp( x,y,z ) == 0
#define stringEqual( x,y ) strcmp( x,y ) == 0
#define stringHasComponent( x,y ) strstr( x,y ) != 0

auto_mount::auto_mount( QObject * parent )
{
	m_baba = static_cast< QThread * >( this ) ;
	m_main = this ;
	m_babu = parent ;
}

auto_mount::~auto_mount()
{
	if( m_fdDir != -1 ){
		close( m_fdDir ) ;
	}
}

void auto_mount::stop()
{
	if( m_threadIsRunning ){
		m_mtoto->terminate() ;
	}else{
		this->threadStopped() ;
	}
}

void auto_mount::threadStopped()
{
	emit stopped() ;
	m_threadIsRunning = false ;
}

bool auto_mount::ignoreDevice( const char * device )
{
	/*
	 * dont care about these devices.
	 * /dev/sgX seem to be created when a usb device is plugged in
	 * /dev/dm-X are dm devices we dont care about since we will be dealing with them differently
	 */
	return stringPrefixMatch( device,"sg",2 )      ||
	       stringPrefixMatch( device,"dm-",3 )     ||
	       stringHasComponent( device,"dev/tmp" )  ||
	       stringHasComponent( device,"dev-tmp" )  ||
	       stringHasComponent( device,".tmp.md." ) ||
	       stringHasComponent( device,"md/md-device-map" ) ;
}

void auto_mount::run()
{
	/*
	 * Not exactly sure what i am doing here but this kind of thing seem to be necessary to prevent
	 * an occassional crash on exit with an error that reads something like "object deleted while thread is still running"
	 */
	m_mtoto = static_cast< QThread * >( this ) ;
	connect( m_mtoto,SIGNAL( terminated() ),m_main,SLOT( threadStopped() ) ) ;
	connect( m_mtoto,SIGNAL( terminated() ),m_mtoto,SLOT( deleteLater() ) ) ;
	connect( m_mtoto,SIGNAL( terminated() ),this,SLOT( deleteLater() ) ) ;

	m_fdDir = inotify_init() ;
	if( m_fdDir == -1 ){
		qDebug() << "inotify_init() failed to start,automounting is turned off" ;
		m_threadIsRunning = false ;
		return ;
	}else{
		m_threadIsRunning = true ;
	}

	int dev    = inotify_add_watch( m_fdDir,"/dev",IN_CREATE|IN_DELETE ) ;
	int mapper = inotify_add_watch( m_fdDir,"/dev/mapper",IN_CREATE|IN_DELETE ) ;
	int md = -1 ;

	QDir d( QString( "/dev/dm" ) ) ;
	if( d.exists() ){
		md = inotify_add_watch( m_fdDir,"/dev/md",IN_DELETE ) ;
	}

	const struct inotify_event * pevent ;
	const char * device ;

	const char * f ;
	const char * z ;
	int data_read ;
	int baseSize = sizeof( struct inotify_event ) ;

	#define BUFF_SIZE 4096
	char buffer[ BUFF_SIZE ];

	while( 1 ){

		data_read = read( m_fdDir,buffer,BUFF_SIZE ) ;

		z = buffer + data_read ;
		f = buffer ;

		while( f < z ){

			pevent = reinterpret_cast< const struct inotify_event * >( f ) ;

			if( pevent ){
				device = f + baseSize ;
				f = f + baseSize + pevent->len ;
			}else{
				f = f + baseSize ;
				continue ;
			}

			if( this->ignoreDevice( device ) ){
				;
			}else{
				if( pevent->wd == dev && pevent->mask & IN_CREATE ){
					/*
					 * /dev/md path seem to be deleted when the last entry in it is removed and
					 * created before the first entry is added.To account for this,monitor for the
					 * folder created to start monitoring its contents if it get created after we have started
					 */
					if( stringEqual( "md",device ) ){
						md = inotify_add_watch( m_fdDir,"/dev/md",IN_DELETE ) ;
						continue ;
					}
				}

				if( pevent->wd == dev && pevent->mask & IN_DELETE ){
					if( stringEqual( "md",device ) ){
						inotify_rm_watch( md,dev ) ;
						continue ;
					}
				}

				m_thread_helper = new auto_mount_helper( m_babu ) ;

				if( pevent->wd == dev ){
					m_thread_helper->start( device,auto_mount_helper::dev,pevent->mask ) ;
				}else if( pevent->wd == mapper ){
					m_thread_helper->start( device,auto_mount_helper::dev_mapper,pevent->mask ) ;
				}else if( pevent->wd == md ){
					m_thread_helper->start( device,auto_mount_helper::dev_md,pevent->mask ) ;
				}else{
					;
				}
			}
		}
	}
}
