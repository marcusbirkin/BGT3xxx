#ifndef __DIB3000_H__
#define __DIB3000_H__

/* most of this is taken from dib3000-common.h, dib3000mc_priv.h and dib3000mb_priv.h */

#define DIB3000_REG_MANUFACTOR_ID       (  1025)
#define DIB3000_I2C_ID_DIBCOM           (0x01b3)

#define DIB3000_REG_DEVICE_ID           (  1026)
#define DIB3000MB_DEVICE_ID             (0x3000)
#define DIB3000MC_DEVICE_ID             (0x3001)
#define DIB3000P_DEVICE_ID              (0x3002)

/* dib3000mb_priv.h */

#define DIB3000MB_REG_DDS_INV           (     5)
#define DIB3000MB_REG_AGC_LOCK          (   324)
#define DIB3000MB_REG_CARRIER_LOCK      (   355)
#define DIB3000MB_REG_TPS_LOCK          (   394)
#define DIB3000MB_REG_VIT_LCK           (   421)
#define DIB3000MB_REG_TS_SYNC_LOCK      (   423)
#define DIB3000MB_REG_TS_RS_LOCK        (   424)

#define DIB3000MB_REG_DDS_FREQ_MSB      (     6)
#define DIB3000MB_REG_DDS_FREQ_LSB      (     7)
#define DIB3000MB_REG_DDS_VALUE_MSB     (   339)
#define DIB3000MB_REG_DDS_VALUE_LSB     (   340)

#define DIB3000MB_REG_BER_MSB           (   414)
#define DIB3000MB_REG_BER_LSB           (   415)
#define DIB3000MB_REG_PACKET_ERROR_RATE (   417)
#define DIB3000MB_REG_UNC               (   420)

#define DIB3000MB_REG_FFT_WINDOW_POS    (   353)
#define DIB3000MB_REG_TPS_FFT			(   404)

#define DIB3000MB_REG_NOISE_POWER_MSB	(   372)
#define DIB3000MB_REG_NOISE_POWER_LSB	(   373)

#define DIB3000MB_REG_SIGNAL_POWER		(   380)

#define DIB3000MB_REG_MER_MSB			(   381)
#define DIB3000MB_REG_MER_LSB			(   382)

#define DIB3000MB_REG_AGC_POWER			(   325)
#define DIB3000MB_REG_RF_POWER			(   328)

#define DIB3000MB_REG_TIMING_OFFSET_MSB (   341)
#define DIB3000MB_REG_TIMING_OFFSET_LSB (   342)

#define DEF_agc_ref_dB     -14
#define DEF_gain_slope_dB  100
#define DEF_gain_delta_dB  -2
#define DEF_SampFreq_KHz     27700

#endif
