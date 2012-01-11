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

int close_mapper( const char * mapper )
{
	int j = 0 ;
	/*
	 * For reasons currently unknown to me, the mapper fail to close sometimes so give it some room when it happens
	 */
	while( j++ < 10 ){
		if( crypt_deactivate( NULL, mapper ) == 0 )
			return 0 ;
		sleep( 1 );
	}
	return 1 ;
}
