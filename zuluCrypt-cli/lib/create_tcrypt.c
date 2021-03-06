 
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
 
#include "includes.h"
#include <sys/syscall.h>
#include <sys/syscall.h>
#include <libcryptsetup.h>   
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * this header is created at config time
 */
#include "truecrypt_support_1.h"

#if TRUECRYPT_CREATE

static int _create_file_system( const char * device,const char * fs,
				const char * key,size_t key_len,int volume_type )
{	
	string_t m = StringVoid ;
	
	int r ;
	
	const char * device_mapper ;
	const char * mapper ;
	
	size_t len ;
	
	m = String( crypt_get_dir() ) ;
	len = StringLength( m )   ;
	
	StringAppend( m,"/zuluCrypt-" ) ;
	device_mapper = StringAppendInt( m,syscall( SYS_gettid ) ) ;
	mapper = device_mapper + len + 1 ;
	
	/*
	 * zuluCryptOpenTcrypt() is defined in open_tcrypt.c
	 */
	if( zuluCryptOpenTcrypt( device,mapper,key,key_len,TCRYPT_PASSPHRASE,volume_type,NULL,0,0,NULL ) == 0 ){
		/*
		 * zuluCryptCreateFileSystemInAVolume() is defined in create_volume.c
		 */
		if( zuluCryptCreateFileSystemInAVolume( fs,device_mapper ) == 0 ){
			r = 0 ;
		}else{
			r = 3 ;
		}
		/*
		 * zuluCryptCloseMapper() is defined in close_mapper.c
		 */
		zuluCryptCloseMapper( device_mapper );
	}else{
		r = 3 ;
	}
	
	StringDelete( &m ) ;
	return r ;
}

static int _create_file_system_1( const char * device,const char * fs,
				const char * keyfile,int volume_type )
{	
	string_t m = StringVoid ;
	
	int r ;
	
	const char * device_mapper ;
	const char * mapper ;
	
	size_t len ;
	
	m = String( crypt_get_dir() ) ;
	len = StringLength( m )   ;
	
	StringAppend( m,"/zuluCrypt-" ) ;
	device_mapper = StringAppendInt( m,syscall( SYS_gettid ) ) ;
	mapper = device_mapper + len + 1 ;
	
	/*
	 * zuluCryptOpenTcrypt() is defined in open_tcrypt.c
	 */
	if( zuluCryptOpenTcrypt( device,mapper,keyfile,0,TCRYPT_KEYFILE_FILE,volume_type,NULL,0,0,NULL ) == 0 ){
		/*
		 * zuluCryptCreateFileSystemInAVolume() is defined in create_volume.c
		 */
		if( zuluCryptCreateFileSystemInAVolume( fs,device_mapper ) == 0 ){
			r = 0 ;
		}else{
			r = 3 ;
		}
		/*
		 * zuluCryptCloseMapper() is defined in close_mapper.c
		 */
		zuluCryptCloseMapper( device_mapper );
	}else{
		r = 3 ;
	}
	
	StringDelete( &m ) ;
	return r ;
}

static string_t _create_key_file( const char * key,size_t key_len,const char * e )
{
	string_t st ;
	struct stat statstr ;
	const char * file ;
	int fd ;
	
	#define path_does_not_exist( x ) stat( x,&statstr ) != 0
	
	if( path_does_not_exist( "/run" ) ){
		mkdir( "/run",S_IRWXU ) ;
		chown( "/run",0,0 ) ;
	}
	if( path_does_not_exist( "/run/zuluCrypt" ) ){
		mkdir( "/run/zuluCrypt",S_IRWXU ) ;
		chown( "/run/zuluCrypt",0,0 ) ;
	}
	
	st = String( "/run/zuluCrypt/create_tcrypt-" ) ;
	StringMultipleAppend( st,e,"-",END ) ;
	file = StringAppendInt( st,syscall( SYS_gettid ) ) ;
	fd = open( file,O_WRONLY|O_CREAT,S_IRUSR | S_IWUSR | S_IRGRP |S_IROTH ) ;
	
	if( fd == -1 ){
		StringDelete( &st ) ;
		return StringVoid ;
	}else{
		write( fd,key,key_len ) ;
		close( fd ) ;
		
		chown( file,0,0 ) ;
		chmod( file,S_IRWXU ) ;
		return st ;
	}
}

static int _create_tcrypt_volume( const char * device,const char * file_system,
				  const char * rng,const char * key,size_t key_len,
				  int key_source,size_t hidden_volume_size,
				  const char * file_system_h,const char * key_h,size_t key_len_h,int key_source_h )
{
	string_t st = StringVoid ;
	string_t xt = StringVoid ;
	
	tc_api_opts api_opts ;
	
	int r = 3 ;
	
	const char * keyfiles[ 2 ] ;
	const char * keyfiles_h[ 2 ] ;
	
	if( zuluCryptPathIsNotValid( device ) ){
		return 1 ;
	}
	
	memset( &api_opts,'\0',sizeof( api_opts ) ) ;
	
	api_opts.tc_device          = device ;
	api_opts.tc_cipher          = "AES-256-XTS";
	api_opts.tc_prf_hash        = "RIPEMD160"  ;
	api_opts.tc_no_secure_erase = 1 ;
	
	if( StringPrefixMatch( rng,"/dev/urandom",12 ) ){
		api_opts.tc_use_weak_keys = 1 ;
	}
	
	api_opts.tc_size_hidden_in_bytes = hidden_volume_size ;
	
	if( key_source == TCRYPT_PASSPHRASE ){
		
		api_opts.tc_passphrase  = key ;
		
		if( hidden_volume_size > 0 ){
			
			if( key_source_h == TCRYPT_PASSPHRASE ){
				api_opts.tc_passphrase_hidden = key_h ;
				if( tc_api_init( 0 ) == TC_OK ){
					
					r = tc_api_create_volume( &api_opts );
					tc_api_uninit() ;
					
					if( r == TC_OK ){
						r = _create_file_system( device,file_system,key,key_len,TCRYPT_NORMAL ) ;
						if( r == TC_OK ){
							r = _create_file_system( device,file_system_h,key_h,key_len_h,TCRYPT_HIDDEN ) ;
						}else{
							r = 3 ;
						}
					}else{
						r = 3 ;
					}
				}else{
					r = 3 ;
				}
			}else{
				st = _create_key_file( key_h,key_len_h,"1" ) ;
				
				if( st != StringVoid ){
					
					keyfiles_h[ 0 ] = StringContent( st ) ;
					keyfiles_h[ 1 ] = NULL ;
					api_opts.tc_keyfiles_hidden = keyfiles_h ;
					
					if( tc_api_init( 0 ) == TC_OK ){
						r = tc_api_create_volume( &api_opts );
						tc_api_uninit() ;
						if( r == TC_OK ){
							r = _create_file_system( device,file_system,key,key_len,TCRYPT_NORMAL ) ;
							if( r == TC_OK ){
								r = _create_file_system_1( device,file_system_h,keyfiles_h[ 0 ],TCRYPT_HIDDEN ) ;
							}else{
								r = 3 ;
							}
						}else{
							r = 3 ;
						}
					}else{
						r = 3 ;
					}
					/*
					 * zuluCryptDeleteFile() is defined in file_path_security.c
					 */
					zuluCryptDeleteFile( keyfiles_h[ 0 ]  ) ;
					StringDelete( & st ) ;
				}else{
					r = 3 ;
				}
			}
		}else{
			if( tc_api_init( 0 ) == TC_OK ){
				r = tc_api_create_volume( &api_opts );
				tc_api_uninit() ;
				if( r == TC_OK ){
					r = _create_file_system( device,file_system,key,key_len,TCRYPT_NORMAL ) ;
				}else{
					r = 3 ;
				}
			}else{
				r = 3 ;
			}
		}
	}else{
		if( hidden_volume_size > 0 ){
			if( key_source_h == TCRYPT_PASSPHRASE ){
				
				api_opts.tc_passphrase_hidden = key_h ;				
				st = _create_key_file( key,key_len,"1" ) ;
				
				if( st != StringVoid ){
					
					keyfiles[ 0 ] = StringContent( st ) ;
					keyfiles[ 1 ] = NULL ;
					
					api_opts.tc_keyfiles = keyfiles ;
					
					if( tc_api_init( 0 ) == TC_OK ){
						
						r = tc_api_create_volume( &api_opts );
						tc_api_uninit() ;
						
						if( r == TC_OK ){
							r = _create_file_system_1( device,file_system,keyfiles[ 0 ],TCRYPT_NORMAL ) ;
							if( r == TC_OK ){
								r = _create_file_system( device,file_system,key_h,key_len_h,TCRYPT_HIDDEN ) ;
							}else{
								r = 3 ;
							}
						}else{
							r = 3 ;
						}
					}else{
						r = 3 ;
					}
					zuluCryptDeleteFile( keyfiles[ 0 ] ) ;
					StringDelete( &st ) ;
				}else{
					r = 3 ;
				}
			}else{
				st = _create_key_file( key,key_len,"1" ) ;
				if( st != StringVoid ){
					xt = _create_key_file( key_h,key_len_h,"2" ) ;
					if( xt != StringVoid ){
						
						keyfiles[ 0 ]   = StringContent( st ) ;
						keyfiles[ 1 ]   = NULL ;
						keyfiles_h[ 0 ] = StringContent( xt ) ;
						keyfiles_h[ 1 ] = NULL ;
						
						api_opts.tc_keyfiles        = keyfiles ;
						api_opts.tc_keyfiles_hidden = keyfiles_h ;
						
						if( tc_api_init( 0 ) == TC_OK ){
							
							r = tc_api_create_volume( &api_opts );
							tc_api_uninit() ;

							if( r == TC_OK ){
								r = _create_file_system_1( device,file_system,keyfiles[ 0 ],TCRYPT_NORMAL ) ;

								if( r == TC_OK ){
									r = _create_file_system_1( device,file_system,keyfiles_h[ 0 ],TCRYPT_HIDDEN ) ;
								}else{
									r = 3 ;
								}
							}else{
								r = 3 ;
							}
						}else{
							r = 3 ;
						}
						zuluCryptDeleteFile( keyfiles_h[ 0 ] ) ;
						StringDelete( &xt ) ;
					}
					zuluCryptDeleteFile( keyfiles[ 0 ] ) ;
					StringDelete( &st ) ;
				}else{
					r = 3 ;
				}
			}
		}else{
			st = _create_key_file( key,key_len,"1" ) ;
			if( st != StringVoid ){
				
				keyfiles[ 0 ] = StringContent( st ) ;
				keyfiles[ 1 ] = NULL ;
				
				api_opts.tc_keyfiles = keyfiles ;
				
				if( tc_api_init( 0 ) == TC_OK ){
					
					r = tc_api_create_volume( &api_opts );
					tc_api_uninit() ;
					
					if( r == TC_OK ){
						r = _create_file_system_1( device,file_system,keyfiles[ 0 ],TCRYPT_NORMAL ) ;
					}else{
						r = 3 ;
					}
				}
				zuluCryptDeleteFile( keyfiles[ 0 ] ) ;
				StringDelete( &st ) ;
			}else{
				r = 3 ;
			}
		}
	}
	
	return r ;
}

int zuluCryptCreateTCrypt( const char * device,const char * file_system,const char * rng,
			   const char * key,size_t key_len,int key_source,
			   size_t hidden_volume_size,const char * file_system_h,const char * key_h,size_t key_len_h,int key_source_h )
{
	int fd ;
	string_t q = StringVoid ;
	int r ;
	if( StringPrefixMatch( device,"/dev/",5 ) ){
		r = _create_tcrypt_volume( device,file_system,rng,key,key_len,key_source,
					   hidden_volume_size,file_system_h,key_h,key_len_h,key_source_h ) ;
	}else{
		/*
		 * zuluCryptAttachLoopDeviceToFile() is defined in create_loop_device.c
		 */
		if( zuluCryptAttachLoopDeviceToFile( device,O_RDWR,&fd,&q ) ){
			device = StringContent( q ) ;
			r = _create_tcrypt_volume( device,file_system,rng,key,key_len,key_source,
						   hidden_volume_size,file_system_h,key_h,key_len_h,key_source_h ) ;
			close( fd ) ;
			StringDelete( &q ) ;
		}else{
			r = 3 ;
		}
	}
	
	return r ;
}

#else
int zuluCryptCreateTCrypt( const char * device,const char * file_system,const char * rng,
			   const char * key,size_t key_len,int key_source,
			   size_t hidden_volume_size,const char * file_system_h,const char * key_h,size_t key_len_h,int key_source_h )
{
	if( 0 && device && file_system && rng && key_len && key_source && key
		&& hidden_volume_size && key_len_h && key_h && key_source_h && file_system_h ){;}
	return 3 ;
}
#endif