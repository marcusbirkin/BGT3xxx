/*
 * test_front.c - Test program for new API
 *
 * Copyright (C) 2000 Ralph  Metzler <ralph@convergence.de>
 *                  & Marcus Metzler <marcus@convergence.de>
                      for convergence integrated media GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <ost/dmx.h>
#include <ost/frontend_old.h>
#include <ost/sec.h>
#include <ost/audio.h>
#include <sys/poll.h>

int OSTSelftest(int fd)
{
	int ans;

	if ((ans = ioctl(fd,OST_SELFTEST,0)) < 0) {
		perror("OST SELF TEST: ");
		return -1;
	}

	return 0;
}

int OSTSetPowerState(int fd, uint32_t state)
{
	int ans;

	if ((ans = ioctl(fd,OST_SET_POWER_STATE,state)) < 0) {
		perror("OST SET POWER STATE: ");
		return -1;
	}

	return 0;
}

int OSTGetPowerState(int fd, uint32_t *state)
{
	int ans;

	if ((ans = ioctl(fd,OST_GET_POWER_STATE,state)) < 0) {
		perror("OST GET POWER STATE: ");
		return -1;
	}

	switch(*state){
	case OST_POWER_ON:
		printf("POWER ON (%d)\n",*state);
		break;
	case OST_POWER_STANDBY:
		printf("POWER STANDBY (%d)\n",*state);
		break;
	case OST_POWER_SUSPEND:
		printf("POWER SUSPEND (%d)\n",*state);
		break;
	case OST_POWER_OFF:
		printf("POWER OFF (%d)\n",*state);
		break;
	default:
		printf("unknown (%d)\n",*state);
		break;
	}

	return 0;
}

int FEReadStatus(int fd)
{
	int ans;
	feStatus stat;

	if ((ans = ioctl(fd,FE_READ_STATUS,&stat)) < 0) {
		perror("FE READ STATUS: ");
		return -1;
	}

	if (stat & FE_HAS_POWER)
		printf("FE HAS POWER\n");

	if (stat & FE_HAS_SIGNAL)
		printf("FE HAS SIGNAL\n");

	if (stat & QPSK_SPECTRUM_INV)
		printf("QPSK SPEKTRUM INV\n");

	return 0;
}

int FEReadBER(int fd, uint32_t *ber)
{
	int ans;

	if ((ans = ioctl(fd,FE_READ_BER, ber)) < 0) {
		perror("FE READ_BER: ");
		return -1;
	}
	printf("BER: %d\n",*ber);

	return 0;
}

int FEReadSignalStrength(int fd, int32_t *strength)
{
	int ans;

	if ((ans = ioctl(fd,FE_READ_SIGNAL_STRENGTH, strength)) < 0) {
		perror("FE READ SIGNAL STRENGTH: ");
		return -1;
	}
	printf("SIGNAL STRENGTH: %d\n",*strength);

	return 0;
}

int FEReadSNR(int fd, int32_t *snr)
{
	int ans;

	if ((ans = ioctl(fd,FE_READ_SNR, snr)) < 0) {
		perror("FE READ_SNR: ");
		return -1;
	}
	printf("SNR: %d\n",*snr);

	return 0;
}


int FEReadUncorrectedBlocks(int fd, uint32_t *ucb)
{
	int ans;

	if ((ans = ioctl(fd,FE_READ_UNCORRECTED_BLOCKS, ucb)) < 0) {
		perror("FE READ UNCORRECTED BLOCKS: ");
		return -1;
	}
	printf("UBLOCKS: %d\n",*ucb);

	return 0;
}

int FEGetNextFrequency(int fd, uint32_t *nfr)
{
	int ans;

	if ((ans = ioctl(fd,FE_GET_NEXT_FREQUENCY, nfr)) < 0) {
		perror("FE GET NEXT FREQUENCY: ");
		return -1;
	}
	printf("Next Frequency: %d\n",*nfr);

	return 0;
}

int FEGetNextSymbolRate(int fd, uint32_t *nsr)
{
	int ans;

	if ((ans = ioctl(fd,FE_GET_NEXT_SYMBOL_RATE, nsr)) < 0) {
		perror("FE GET NEXT SYMBOL RATE: ");
		return -1;
	}
	printf("Next Symbol Rate: %d\n",*nsr);

	return 0;
}

int QPSKTune(int fd, struct qpskParameters *param)
{
	int ans;

	if ((ans = ioctl(fd,QPSK_TUNE, param)) < 0) {
		perror("QPSK TUNE: ");
		return -1;
	}

	return 0;
}

int QPSKGetEvent (int fd, struct qpskEvent *event)
{
	int ans;

	if ((ans = ioctl(fd,QPSK_GET_EVENT, event)) < 0) {
		perror("QPSK GET EVENT: ");
		return -1;
	}

	return 0;
}

int QPSKFEInfo (int fd, struct qpskFrontendInfo *info)
{
	int ans;

	if ((ans = ioctl(fd,QPSK_FE_INFO, info)) < 0) {
		perror("QPSK FE INFO: ");
		return -1;
	}

	printf("min Frequency   : %d\n", info->minFrequency);
	printf("max Frequency   : %d\n", info->maxFrequency);
	printf("min Symbol Rate : %d\n", info->minSymbolRate);
	printf("max Symbol Rate : %d\n", info->maxSymbolRate);
	printf("Hardware Type   : %d\n", info->hwType);
	printf("Hardware Version: %d\n", info->hwVersion);

	return 0;
}

int SecGetStatus (int fd, struct secStatus *state)
{
	int ans;

	if ((ans = ioctl(fd,SEC_GET_STATUS, state)) < 0) {
		perror("QPSK GET EVENT: ");
		return -1;
	}

	switch (state->busMode){
	case SEC_BUS_IDLE:
		printf("SEC BUS MODE:  IDLE (%d)\n",state->busMode);
		break;
	case SEC_BUS_BUSY:
		printf("SEC BUS MODE:  BUSY (%d)\n",state->busMode);
		break;
	case SEC_BUS_OFF:
		printf("SEC BUS MODE:  OFF  (%d)\n",state->busMode);
		break;
	case SEC_BUS_OVERLOAD:
		printf("SEC BUS MODE:  OVERLOAD (%d)\n",state->busMode);
		break;
	default:
		printf("SEC BUS MODE:  unknown  (%d)\n",state->busMode);
		break;
	}


	switch (state->selVolt){
	case SEC_VOLTAGE_OFF:
		printf("SEC VOLTAGE:  OFF (%d)\n",state->selVolt);
		break;
	case SEC_VOLTAGE_LT:
		printf("SEC VOLTAGE:  LT  (%d)\n",state->selVolt);
		break;
	case SEC_VOLTAGE_13:
		printf("SEC VOLTAGE:  13  (%d)\n",state->selVolt);
		break;
	case SEC_VOLTAGE_13_5:
		printf("SEC VOLTAGE:  13.5 (%d)\n",state->selVolt);
		break;
	case SEC_VOLTAGE_18:
		printf("SEC VOLTAGE:  18 (%d)\n",state->selVolt);
		break;
	case SEC_VOLTAGE_18_5:
		printf("SEC VOLTAGE:  18.5 (%d)\n",state->selVolt);
		break;
	default:
		printf("SEC VOLTAGE:  unknown (%d)\n",state->selVolt);
		break;
	}

	printf("SEC CONT TONE: %s\n", (state->contTone ? "ON" : "OFF"));
	return 0;
}


main(int argc, char **argv)
{
	int fd,fd_sec;
	uint32_t state;
	int32_t strength;
	struct qpskFrontendInfo info;
	struct secStatus sec_state;

	if ((fd = open("/dev/ost/qpskfe",O_RDWR)) < 0){
		perror("FRONTEND DEVICE: ");
		return -1;
	}
	if ((fd_sec = open("/dev/ost/sec",O_RDWR)) < 0){
		perror("SEC DEVICE: ");
		return -1;
	}

	OSTSelftest(fd);
	OSTSetPowerState(fd, OST_POWER_ON);
	OSTGetPowerState(fd, &state);
	FEReadStatus(fd);
	FEReadBER(fd, &state);
	FEReadSignalStrength(fd, &strength);
	FEReadSNR(fd, &state);
	FEReadUncorrectedBlocks(fd, &state);
	state = 12567000;
	FEGetNextFrequency(fd, &state);
	FEGetNextSymbolRate(fd, &state);
	QPSKFEInfo (fd, &info);
	SecGetStatus (fd_sec, &sec_state);

	close(fd);
	close(fd_sec);
}
