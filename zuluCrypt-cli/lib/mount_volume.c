/*
 * 
 *  Copyright (c) 2011
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

#include "includes.h"

#include <sys/mount.h>
#include <mntent.h>
#include <blkid/blkid.h>
#include <stdlib.h>
#include <unistd.h>

#include "../libmount_header.h"

int mount_mapper( const char * mapper,const char * m_point,const char * mode,uid_t id, const char * fs, string_t * options )
{
	unsigned long mountflags = 0 ;
	int h ;
	char uid_s[ 5 ] ;
	char * uid = StringIntToString( uid_s,5,id ) ;	
	string_t opt ;
	
	if( strcmp( mode,"ro" ) == 0 )
		mountflags = MS_RDONLY ;
	
	if( strcmp( fs,"vfat" ) == 0 ){
		opt = String( "dmask=077,uid=" ) ;
		StringAppend( opt,uid ) ;
		StringAppend( opt,",gid=" ) ;
		StringAppend( opt,uid );
		h = mount( mapper,m_point,fs,mountflags,StringContent( opt ) ) ;	
		StringPrepend( opt,"," ) ;
		StringPrepend( opt,mode ) ;
	}else{		
		opt = String( mode ) ;
		h = mount( mapper,m_point,fs,mountflags,NULL ) ;	
		if( h == 0 && mountflags != MS_RDONLY ){			
			chmod( m_point,S_IRWXU ) ;
			chown( m_point,id,id ) ;
		}
	}
	*options = opt ;
	return h ;
}

int mount_volume( const char * mapper,const char * m_point,const char * mode,uid_t id )
{
	struct mntent mt  ;
	blkid_probe blkid ;
	char * path ;
	int h ;
	const char * cf ;	
	FILE * f ;
	mnt_lock * m_lock ;
	string_t options = NULL ;
	string_t fs = NULL ;
	
	blkid = blkid_new_probe_from_filename( mapper ) ;
	blkid_do_probe( blkid );
	
	if( blkid_probe_lookup_value( blkid,"TYPE",&cf,NULL ) != 0 ){
		/*
		 * Attempt to read volume file system has failed because either an attempt to open a plain based volumes with
		 * a wrong password was made or the volume has no file system.
		 */		
		blkid_free_probe( blkid );
		return 4 ;		
	}
	fs = String( cf ) ;
	
	blkid_free_probe( blkid );
	
	path = realpath( "/etc/mtab",NULL ) ;
	
	if( strncmp( path,"/proc",5 ) == 0 )
		h = mount_mapper( mapper,m_point,mode,id,StringContent( fs ),&options ) ;
	else{
		/* "/etc/mtab" is not a symbolic link to /proc/mounts, manually,add an entry to it since 
		 * mount command does not
		 */		
		m_lock = mnt_new_lock( "/etc/mtab~",getpid() ) ;
		if( mnt_lock_file( m_lock ) != 0 ){
			h = 12 ;
		}else{		
			h = mount_mapper( mapper,m_point,mode,id,StringContent( fs ),&options ) ;
			if( h == 0 ){
				f = setmntent( "/etc/mtab","a" ) ;	
				mt.mnt_fsname = ( char * ) mapper ;
				mt.mnt_dir    = ( char * ) m_point ;
				mt.mnt_type   = ( char * ) StringContent( fs ) ;	
				mt.mnt_opts   = ( char * ) StringContent( options ) ;
				mt.mnt_freq   = 0 ;
				mt.mnt_passno = 0 ;
				addmntent( f,&mt ) ;	
				endmntent( f ) ;
			}
			mnt_unlock_file( m_lock ) ;
		}	
		mnt_free_lock( m_lock ) ;
	}
	free( path ) ;
	StringDelete( &fs ) ;
	StringDelete( &options ) ;
	return h ;
}

