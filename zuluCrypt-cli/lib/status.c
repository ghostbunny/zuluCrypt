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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <blkid/blkid.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <stdio.h>
#include <libcryptsetup.h>   
#include <fcntl.h>
#include <unistd.h>

/*
 * 64 byte buffer is more than enough because the API that will produce the largest number is crypt_get_data_offset()
 * and it produces a uint64_t number and this type has a maximum digit count is 19. 
 */
#define SIZE 64


/*
 * zuluCryptLoopDeviceAddress() is moved to create_loop.c
 */

char * zuluCryptGetMountPointFromPath( const char * path ) ;

char * zuluCryptVolumeDeviceName( const char * mapper ) ;

static void zuluCryptFormatSize_1( char * buffer,int x,const char * z )
{
	buffer[ x ] = buffer[ x - 1 ] ; 
	buffer[ x - 1 ] = '.' ;
	strcpy( buffer + x + 1,z ) ; 
}

void zuluCryptFormatSize( char * buffer,const char * buff )
{
	strcpy( buffer,buff ) ;
	
	switch( strlen( buff ) ){
		case 0 : 
		case 1 :  
		case 2 : 
		case 3 : strcat( buffer," B" )          	 ; break ;
		case 4 : zuluCryptFormatSize_1( buffer,2," KB" ) ; break ; 
		case 5 : zuluCryptFormatSize_1( buffer,3," KB" ) ; break ;
		case 6 : zuluCryptFormatSize_1( buffer,4," KB" ) ; break ;
		case 7 : zuluCryptFormatSize_1( buffer,2," MB" ) ; break ; 
		case 8 : zuluCryptFormatSize_1( buffer,3," MB" ) ; break ;
		case 9 : zuluCryptFormatSize_1( buffer,4," MB" ) ; break ;
		case 10: zuluCryptFormatSize_1( buffer,2," GB" ) ; break ;
		case 11: zuluCryptFormatSize_1( buffer,3," GB" ) ; break ;
		case 12: zuluCryptFormatSize_1( buffer,4," GB" ) ; break ;
		case 13: zuluCryptFormatSize_1( buffer,2," TB" ) ; break ;
		case 14: zuluCryptFormatSize_1( buffer,3," TB" ) ; break ;
		case 15: zuluCryptFormatSize_1( buffer,4," TB" ) ; break ;
	}	
}

char * zuluCryptGetUUIDFromMapper( const char * mapper )
{
	string_t uuid ;
	struct crypt_device * cd;
	const char * id ;
	const char * type ;
	const char * e = " UUID:   \t\"Nil\"" ;
	
	if( crypt_init_by_name( &cd,mapper ) < 0 ){
		uuid = String( e ) ;
	}else{
		type = crypt_get_type( cd ) ;
		if( StringHasNoComponent( type,"LUKS" ) ){
			uuid = String( e ) ;
		}else{
			id = crypt_get_uuid( cd ) ;
			if( id == NULL ){
				uuid = String( e ) ;
			}else{
				uuid = String( "" ) ;
				StringMultipleAppend( uuid," UUID:   \t\"",id,"\"",END ) ;
			}
		}
		crypt_free( cd ) ;
	}
	return StringDeleteHandle( &uuid ) ;
}

void zuluCryptFileSystemProperties( string_t p,const char * mapper,const char * m_point )
{
	const char * e ;
	blkid_probe blkid ;
	struct statvfs vfs ;
	uint64_t total ;
	uint64_t used ;
	uint64_t unused ;
	uint64_t block_size ;
	char buff[ SIZE ] ;
	char * buffer = buff ;
	char format[ SIZE ] ;
	string_t q ;
	ssize_t index ;
	struct stat statstr ;
	
	blkid = blkid_new_probe_from_filename( mapper ) ;
	
	if( blkid == NULL ){
		return ;
	}
	
	blkid_do_probe( blkid ) ;
	
	if( blkid_probe_lookup_value( blkid,"TYPE",&e,NULL ) == 0 ){
		StringMultipleAppend( p,"\n file system:\t",e,END ) ;
	}else{
		StringAppend( p,"\n file system:\tNil" ) ;
	}
	
	blkid_free_probe( blkid ) ;
	
	if( statvfs( m_point,&vfs ) != 0 ){
		return ;
	}
	
	block_size = vfs.f_frsize ;
	total = block_size * vfs.f_blocks  ;
	unused =  block_size * vfs.f_bavail  ;
	
	used = total - unused ;
	
	e = StringIntToString_1( buffer,SIZE,total ) ;
	zuluCryptFormatSize( format,e ) ;
	StringMultipleAppend( p,"\n total space:\t",format,END ) ;
	
	e = StringIntToString_1( buffer,SIZE,used )  ;
	zuluCryptFormatSize( format,e ) ;
	StringMultipleAppend( p,"\n used space:\t",format,END ) ;
	
	e = StringIntToString_1( buffer,SIZE,unused ) ;
	zuluCryptFormatSize( format,e ) ;
	StringMultipleAppend( p,"\n free space:\t",format,END ) ;
	
	snprintf( buff,SIZE,"%.2f%%",100 * ( ( float ) used / ( float ) total ) ) ;
	StringMultipleAppend( p,"\n used%:   \t",buff,"\n",END ) ;
		
	buffer = zuluCryptGetUUIDFromMapper( mapper ) ;
	StringAppend( p,buffer ) ;
	
	free( buffer ) ;
	
	StringMultipleAppend( p,"\n mount point1:\t",m_point,END ) ;
	
	q = String( m_point ) ;
	index = StringLastIndexOfChar( q,'/' ) ;
	if( index == -1 ){
		StringAppend( p,"\n mount point2:\tNil" ) ;
	}else{
		StringRemoveLeft( q,index ) ;
		e = StringPrepend( q,"/run/media/public" ) ;
		if( stat( e,&statstr ) == 0 ){
			StringMultipleAppend( p,"\n mount point2:\t",e,END ) ;
		}else{
			StringAppend( p,"\n mount point2:\tNil" ) ;
		}
	}
	
	StringDelete( &q ) ;
}

char * zuluCryptGetVolumeTypeFromMapperPath( const char * mapper )
{
	struct crypt_device * cd;
	const char * type ;
	char * r ;
	const char * nil = "Nil" ;
	
	if( !StringPrefixEqual( mapper,crypt_get_dir() ) ){
		return StringCopy_2( nil ) ;
	}
	
	if( crypt_init_by_name( &cd,mapper ) < 0 ){
		return StringCopy_2( nil ) ;
	}
		
	type = crypt_get_type( cd ) ;
	
	if( type == NULL ){
		r = StringCopy_2( nil ) ;
	}else{
		if( StringHasComponent( type,"LUKS" ) ){
			r = StringCopy_2( "crypto_LUKS" ) ;
		}else if( StringHasComponent( type,"PLAIN" ) ){
			r = StringCopy_2( "crypto_PLAIN" ) ;
		}else if( StringHasComponent( type,"TCRYPT" ) ){
			r = StringCopy_2( "crypto_TCRYPT" ) ;
		}else{
			r = StringCopy_2( nil ) ;
		}
	}
	
	crypt_free( cd ) ;
	return r ;
}

string_t zuluCryptConvertIfPathIsLVM( const char * path )
{
	/*
	 * An assumption is made here that the path is an LVM path if "path" is in /dev/mapper/abc-def format
	 * and there exist a path at /dev/abc/def format.
	 */
	string_t q = String( path ) ;
	ssize_t index = StringLastIndexOfChar( q,'-' ) ;
	const char * e ;
	struct stat st ;
	if( index != -1 ){
		StringSubChar( q,index,'/' ) ; 
		e = StringReplaceString( q,"/dev/mapper/","/dev/" ) ;
		if( stat( e,&st ) == 0 ){
			/*
			 * Path appear to be an LVM path since /dev/abc/def path exists
			 */
			;
		}else{
			StringAppendAt( q,0,path ) ;
		}
	}else{
		;
	}
	return q ;
}

char * zuluCryptVolumeStatus( const char * mapper )
{	
	char buff[ SIZE ] ;
	char * buffer = buff ;
	const char * z ;
	const char * type ;
	const char * device_name ;
	char * path ;
	int luks = 0 ;
	int i ;
	int j ;
	int k ;
	
	struct crypt_device * cd;
	struct crypt_active_device cad ;
	
	string_t p ;
	string_t q ;
	
	if( crypt_init_by_name( &cd,mapper ) != 0 ){
		return NULL ;
	}
	if( crypt_get_active_device( NULL,mapper,&cad ) != 0 ){
		crypt_free( cd ) ;
		return NULL ;
	}
	
	device_name = crypt_get_device_name( cd ) ;
	if( device_name == NULL ){
		crypt_free( cd ) ;
		return NULL ;
	}
	
	p = String( mapper ) ;
	
	switch( crypt_status( cd,mapper ) ){
		case CRYPT_INACTIVE :
			StringAppend( p," is inactive.\n" ) ;
			crypt_free( cd ) ;
			return StringDeleteHandle( &p ) ;
		case CRYPT_ACTIVE   : 
			StringAppend( p," is active.\n" ) ;
			break ;
		case CRYPT_BUSY     :
			StringAppend( p," is active and is in use.\n" ) ;
			break ;
		case CRYPT_INVALID  : 
			StringAppend( p," is invalid.\n" ) ;
			crypt_free( cd ) ;
			return StringDeleteHandle( &p ) ;
	}
	
	StringAppend( p," type:   \t" ) ;
	
	type = crypt_get_type( cd ) ;
	
	if( type == NULL ){
		StringAppend( p,"Nil" ) ;
	}else{
		if( StringPrefixMatch( type,"LUKS",4 ) ){
			luks = 1 ;
			q = String( type ) ;
			StringReplaceString( q,"LUKS","luks" ) ;
			StringAppendString( p,q ) ;
			StringDelete( &q ) ;
		}else if( StringsAreEqual( type,"PLAIN" ) ){
			StringAppend( p,"plain" ) ;
		}else if( StringsAreEqual( type,"TCRYPT" ) ){
			StringAppend( p,"tcrypt" ) ;
		}else{
			StringAppend( p,"Nil" ) ;
		}
	}
	
	StringMultipleAppend( p,"\n cipher:\t",crypt_get_cipher( cd ),"-",crypt_get_cipher_mode( cd ),END ) ;
	
	z = StringIntToString_1( buffer,SIZE,8 * crypt_get_volume_key_size( cd ) ) ;
	StringMultipleAppend( p,"\n keysize:\t",z," bits\n device:\t",END ) ;
	
	if( StringPrefixMatch( device_name,"/dev/loop",9 ) ){
		StringMultipleAppend( p,device_name,"\n loop:   \t",END ) ;
		path = zuluCryptLoopDeviceAddress_1( device_name ) ;
		if( path != NULL ){
			StringAppend( p,path ) ;
			free( path ) ;
		}else{
			StringAppend( p,"Nil" ) ;
		}
	}else{
		if( StringPrefixMatch( device_name,"/dev/mapper/",12 ) ){
			q = zuluCryptConvertIfPathIsLVM( device_name ) ;
			StringAppendString( p,q ) ;
			StringDelete( &q ) ;
		}else if( StringPrefixMatch( device_name,"/dev/md",7 ) ){
			/*
			 * zuluCryptResolveMDPath_1() is defined in process_mountinfo.c
			 */
			q = zuluCryptResolveMDPath_1( device_name ) ;
			StringAppendString( p,q ) ;
			StringDelete( &q ) ;	
		}else{
			StringAppend( p,device_name ) ;
		}
		StringAppend( p,"\n loop:   \tNil" ) ;
	}
	
	z = StringIntToString_1( buffer,SIZE,crypt_get_data_offset( cd ) ) ;
	StringMultipleAppend( p,"\n offset:\t",z," sectors",NULL ) ;
	
	if( cad.flags == 1 ){
		StringAppend( p,"\n mode:   \tread only" ) ;
	}else{
		StringAppend( p,"\n mode:   \tread and write" ) ;
	}
	
	if( luks == 1 ){
		i = 0 ;
		k = crypt_keyslot_max( type ) ;
		for( j = 0 ; j < k ; j++ ){
			switch( crypt_keyslot_status( cd,j ) ){
				case CRYPT_SLOT_INACTIVE    :     ; break ;
				case CRYPT_SLOT_ACTIVE_LAST : i++ ; break ;
				case CRYPT_SLOT_ACTIVE      : i++ ; break ;
				case CRYPT_SLOT_INVALID     :     ; break ;
			}
		}
		
		StringMultipleAppend( p,"\n active slots:\t",StringIntToString_1( buffer,SIZE,i ),END ) ;
		
		StringMultipleAppend( p," / ",StringIntToString_1( buffer,SIZE,k ),END ) ;
	}else{
		StringAppend( p,"\n active slots:\tNil" ) ;
	}
	
	/*
	 * zuluCryptGetMountPointFromPath() is defined in ./process_mountinfo.c
	 */
	path = zuluCryptGetMountPointFromPath( mapper ) ;
	if( path != NULL ){
		zuluCryptFileSystemProperties( p,mapper,path ) ; 
		free( path ) ;
	}
	
	crypt_free( cd ) ;
	return StringDeleteHandle( &p ) ;
}

char * zuluCryptVolumeDeviceName( const char * mapper )
{
	struct crypt_device * cd;
	char * path ;
	string_t address = StringVoid ;
	const char * e ;
	
	e = crypt_get_dir() ;
	
	if( !StringPrefixEqual( mapper,e ) ){
		return NULL ;
	}
	if( crypt_init_by_name( &cd,mapper ) < 0 ){
		return NULL ;
	}
	
	e = crypt_get_device_name( cd ) ;
	
	if( e != NULL ){
		if( StringPrefixMatch( e,"/dev/loop",9 ) ){
			/*
			 * zuluCryptLoopDeviceAddress_1() is defined in create_loop.c
			 */
			path = zuluCryptLoopDeviceAddress_1( e ) ;
			if( path != NULL ){
				address = StringInherit( &path ) ;
			}else{
				address = String( e ) ;
			}
		}else if( StringPrefixMatch( e,"/dev/mapper/",12 ) ){
			address = zuluCryptConvertIfPathIsLVM( e ) ;
		}else if( StringPrefixMatch( e,"/dev/md",7 ) ){
			/*
			 * zuluCryptResolveMDPath_1() is defined in process_mountinfo.c
			 */
			address = zuluCryptResolveMDPath_1( e ) ;
		}else{
			address = String( e ) ;
		}
	}else{
		;
	}
	
	crypt_free( cd ) ;
	
	return StringDeleteHandle( &address ) ;
}
