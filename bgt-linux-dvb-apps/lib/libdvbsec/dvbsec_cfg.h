/**
 * dvbsec_cfg (i.e. linuxtv SEC format) configuration file support.
 *
 * Copyright (c) 2006 by Andrew de Quincey <adq_dvb@lidskialf.net>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/**
 * This library allows SEC (Satellite Equipment Control) configurations
 * to be retrieved. Each configuration is identified by a unique satellite_id.
 *
 * In order to make things as easy as possible for users, there are a set of
 * defaults hardcoded into the library covering the majority of LNB types. When
 * these are used, the standard back-compatable sequence defined in the DISEQC
 * standard will be used - this will suffice for _most_ situations.
 *
 * UNIVERSAL - Europe, 10800 to 11800 MHz and 11600 to 12700 Mhz, Dual LO, loband 9750, hiband 10600 MHz.
 * DBS - Expressvu, North America, 12200 to 12700 MHz, Single LO, 11250 MHz.
 * STANDARD - 10945 to 11450 Mhz, Single LO, 10000Mhz.
 * ENHANCED - Astra, 10700 to 11700 MHz, Single LO, 9750MHz.
 * C-BAND - Big Dish, 3700 to 4200 MHz, Single LO, 5150Mhz.
 * C-MULTI - Big Dish - Multipoint LNBf, 3700 to 4200 MHz, Dual LO, H:5150MHz, V:5750MHz.
 *
 * However, for the power user with a more complex setup, these simple defaults
 * are not enough. Therefore, it is also possible to define additional SEC
 * configurations in an external configuration file. This file consists of multiple
 * entries in the following format:
 *
 * [sec]
 * name=<sec_id>
 * switch-frequency=<switching frequency (SLOF)>
 * lof-lo-v=<low band + V + frequency>
 * lof-lo-h=<low band + H + frequency>
 * lof-lo-l=<low band + L + frequency>
 * lof-lo-r=<low band + R + frequency>
 * lof-hi-v=<high band + V + frequency>
 * lof-hi-h=<high band + H + frequency>
 * lof-hi-l=<high band + L + frequency>
 * lof-hi-r=<high band + R + frequency>
 * config-type=<none|power|standard|advanced>
 * cmd-lo-v=<sec sequence>
 * cmd-lo-h=<sec sequence>
 * cmd-lo-r=<sec sequence>
 * cmd-lo-l=<sec sequence>
 * cmd-hi-v=<sec sequence>
 * cmd-hi-h=<sec sequence>
 * cmd-hi-r=<sec sequence>
 * cmd-hi-l=<sec sequence>
 *
 * The sec_id is whatever unique value you wish. If it is the same as one of the hardcoded defaults, the configuration
 * 	details from the file will be used instead of the hardcoded ones.
 * The switch-frequency (or SLOF) indicates the point seperating low band frequencies from high band frequencies.
 * 	Set this value to 0 if there is only one frequency band.
 * The lof-lo-v is the frequency adjustment for V + low band (i.e. less than SLOF), or is used if switch-frequency==0.
 * The lof-lo-h is the frequency adjustment for H + low band (i.e. less than SLOF), or is used if switch-frequency==0.
 * The lof-lo-l is the frequency adjustment for L + low band (i.e. less than SLOF), or is used if switch-frequency==0.
 * The lof-lo-r is the frequency adjustment for R + low band (i.e. less than SLOF), or is used if switch-frequency==0.
 * The lof-hi-v is the frequency adjustment for V + high band (unused if switch-frequency==0).
 * The lof-hi-h is the frequency adjustment for H + high band (unused if switch-frequency==0).
 * The lof-hi-l is the frequency adjustment for L + high band (unused if switch-frequency==0).
 * The lof-hi-r is the frequency adjustment for R + high band (unused if switch-frequency==0).
 *
 * config-type indicates the desired type of SEC command to use, it may be:
 * 	none - No SEC commands will be issued (frequency adjustment will still be performed).
 * 	power - Only the SEC power is turned on.
 * 	standard - The standard DISEQC back compatable sequence will be issued.
 * 	advanced - The DISEQC sequence described in the appropriate sec cmd string will be used.
 *
 * The cmd-<lo|hi>-<v|h|l|r> describes the SEC cmd string to use in advanced mode for each of the possible combinations of
 * frequency band and polarisation. If a certain combination is not required, it may be omitted. It consists of a
 * space seperated combination of commands - those available are as follows:
 *
 *	tone(<0|1>)  - control the 22kHz tone 0:off, 1:on
 *	voltage(<0|13|18>) - control the LNB voltage 0v, 13v, or 18v
 * 	toneburst(<a|b>) - issue a toneburst (mini command) for position A or B.
 *	highvoltage(<0|1>) - control high lnb voltage for long cable runs 0: normal, 1:add 1v to LNB voltage.
 *	dishnetworks(<integer>) - issue a dishnetworks legacy command.
 *	wait(<integer>) - wait for the given number of milliseconds.
 *	Dreset(<address>, <0|1>) - control the reset state of a DISEC device, 0:disable reset, 1:enable reset.
 *	Dpower(<address>, <0|1>) - control the power of a DISEC device, 0:off, 1:on.
 *	Dcommitted(<address>, <h|l|x>, <v|h|l|r|x>, <a|b|x>, <a|b|x>) - Write to the committed switches of a DISEC device.
 * 		The parameters are for band, polarisation, satelliteposition, switchoption:
 * 			band - h:high band, l:low band
 * 			polarisation - v: vertical, h:horizontal,r:right,l:left
 * 			satelliteposition - a:position A, b: position B
 * 			switchoption - a:position A, b: position B
 * 		The special value 'x' means "no change to this switch".
 *
 *	Duncommitted(<address>, <a|b|x>, <a|b|x>, <a|b|x>, <a|b|x>) - Write to the uncommitted switches of the a DISEC device.
 * 		The parameters are for switch1, switch2, switch3, switch4, and may be set to position a or b.
 * 		The special value 'x' means "no change to this switch".
 *
 * 	Dfrequency(<address>, <frequency in GHz>) - set the frequency of a DISEC device.
 * 	Dchannel(<address>, <channel id>) - set the desired channel id of a DISEC device.
 * 	Dgotopreset(<address>, <preset id>) - tell a DISEC satellite positioner to move to the given preset id.
 * 	Dgotobearing(<address>, <bearing in degrees>) - tell a DISEQC terrestrial rotator to go to the
 *		given bearing (range -256.0 -> 512.0 degrees, fractions allowed).
 *
 * 	In the above DISEQC commands, <address> is the integer (normally in hex format) address of the
 * 		diseqc device to communicate with. A list of possiblities is as follows:
 *
 * 	DISEQC_ADDRESS_ANY_DEVICE		= 0x00
 *
 *	DISEQC_ADDRESS_ANY_LNB_SWITCHER_SMATV	= 0x10
 *	DISEQC_ADDRESS_LNB			= 0x11
 *	DISEQC_ADDRESS_LNB_WITH_LOOP		= 0x12
 *	DISEQC_ADDRESS_SWITCHER			= 0x14
 *	DISEQC_ADDRESS_SWITCHER_WITH_LOOP	= 0x15
 *	DISEQC_ADDRESS_SMATV			= 0x18
 *
 *	DISEQC_ADDRESS_ANY_POLARISER		= 0x20
 *	DISEQC_ADDRESS_LINEAR_POLARISER		= 0x21
 *
 *	DISEQC_ADDRESS_ANY_POSITIONER		= 0x30
 *	DISEQC_ADDRESS_POLAR_AZIMUTH_POSITIONER	= 0x31
 *	DISEQC_ADDRESS_ELEVATION_POSITIONER	= 0x32
 *
 *	DISEQC_ADDRESS_ANY_INSTALLER_AID	= 0x40
 *	DISEQC_ADDRESS_SIGNAL_STRENGTH		= 0x41
 *
 *	DISEQC_ADDRESS_ANY_INTERFACE		= 0x70
 *	DISEQC_ADDRESS_HEADEND_INTERFACE	= 0x71
 *
 *	DISEQC_ADDRESS_REALLOC_BASE		= 0x60
 *	DISEQC_ADDRESS_OEM_BASE			= 0xf0
 */

#ifndef DVBSEC_CFG_H
#define DVBSEC_CFG_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdint.h>
#include <libdvbsec/dvbsec_api.h>

/**
 * Callback function used in dvbsec_cfg_load().
 *
 * @param arg Private information to caller.
 * @param channel The current channel details.
 * @return 0 to continue, 1 to stop loading.
 */
typedef int (*dvbsec_cfg_callback)(void *arg, struct dvbsec_config *sec);

/**
 * Load an SEC file.
 *
 * @param f File to load from.
 * @param arg Value to pass to 'arg' in callback above.
 * @param cb Callback function called for each sec loaded from the file.
 * @return 0 on success, or nonzero error code on failure.
 */
extern int dvbsec_cfg_load(FILE *f, void *arg,
			   dvbsec_cfg_callback cb);

/**
 * Convenience function to parse an SEC config file. This will also consult the set
 * of hardcoded defaults if no config file was supplied, or a match was not found in
 * the config file.
 *
 * @param config_file Config filename to load, or NULL to just check defaults.
 * @param sec_id ID of SEC configuration.
 * @param sec Where to put the details if found.
 * @return 0 on success, nonzero on error.
 */
extern int dvbsec_cfg_find(const char *config_file,
			   const char *sec_id,
			   struct dvbsec_config *sec);

/**
 * Save SEC format config file.
 *
 * @param f File to save to.
 * @param secs Pointer to array of SECs to save.
 * @param count Number of entries in the above array.
 * @return 0 on success, or nonzero error code on failure.
 */
extern int dvbsec_cfg_save(FILE *f,
			   struct dvbsec_config *secs,
			   int count);

#ifdef __cplusplus
}
#endif

#endif
