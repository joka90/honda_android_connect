/*
* Copyright(C) 2012 FUJITSU TEN LIMITED
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; version 2
* of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#define LOG_TAG "dabluetoothd"

#include <ada/log.h>
#include <utils/Log.h>
#include <OomManager.h>

int CMN_LOG_ISLOGGABLE(const char* tag, int level)
{
	return ada_log_isLoggable(tag, level);
}


void CMN_LOG_ERROR( const char * fmt, ...)
{
    va_list     args;
    char        buf[512] = {0};

    va_start(args, fmt);
    vsnprintf(&buf[0], sizeof(buf)-1, fmt, args);
    va_end(args);

    (void)ada_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s", &buf[0]);
}

void CMN_LOG_WARNING( const char * fmt, ...)
{
    va_list     args;
    char        buf[512] = {0};

    va_start(args, fmt);
    vsnprintf(&buf[0], sizeof(buf)-1, fmt, args);
    va_end(args);

    (void)ada_log_print(ANDROID_LOG_WARN, LOG_TAG, "%s", &buf[0]);
}

void CMN_LOG_INFO( const char * fmt, ...)
{
    va_list     args;
    char        buf[512] = {0};

    va_start(args, fmt);
    vsnprintf(&buf[0], sizeof(buf)-1, fmt, args);
    va_end(args);

    (void)ada_log_print(ANDROID_LOG_INFO,  LOG_TAG, "%s", &buf[0]);
}

void CMN_LOG_DEBUG( const char * fmt, ...)
{
    va_list     args;
    char        buf[512] = {0};

    va_start(args, fmt);
    vsnprintf(&buf[0], sizeof(buf)-1, fmt, args);
    va_end(args);

    (void)ada_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s", &buf[0]);
}

int bsm_setOom(void){
  if(OOM_OK != setOomAdjSystem()){
    return 1;
    }
  return 0;
}
