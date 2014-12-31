#ifndef __DIB_DEMOD_WATCH__
#define __DIB_DEMOD_WATCH__

#define err(args...)  fprintf(stderr,"error '%s': ",strerror(errno)); fprintf(stderr,args)
#define verb(args...) fprintf(stderr,args)

typedef enum {
	DIB3000MB = 0,
	DIB3000MC,
	DIB3000P,
} dib_demod_t;


struct dib_demod {
	int fd;
	__u8 i2c_addr;

	dib_demod_t rev;
};

struct dib3000mb_monitoring {
	int agc_lock;
	int carrier_lock;
	int tps_lock;
	int vit_lock;
	int ts_sync_lock;
	int ts_data_lock;

	int invspec;

	int per;
	int unc;

	int fft_pos;

	int nfft;

	double carrier_offset;
	double ber;
	double snr;
	double mer;
	double rf_power;
	double timing_offset_ppm;
};

#endif
