/*
 * libdvbnet - a DVB network support library
 *
 * Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef LIBDVBAUDIO_H
#define LIBDVBAUDIO_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * Open a DVB audio device.
 *
 * @param adapter DVB adapter ID.
 * @param audiodeviceid Id of audio device of that adapter to open.
 * @return A unix file descriptor on success, or -1 on failure.
 */
extern int dvbaudio_open(int adapter, int audiodeviceid);

/**
 * Control audio bypass - i.e. output decoded audio, or the raw bitstream (e.g. AC3).
 *
 * @param fd Audio device opened with dvbaudio_open().
 * @param bypass 1=> enable bypass, 0=> disable.
 * @return 0 on success, nonzero on failure.
 */
extern int dvbaudio_set_bypass(int fd, int bypass);

// FIXME: this is a stub library

#ifdef __cplusplus
}
#endif

#endif // LIBDVBAUDIO_H
