/**
 * @file sac2tnk.c
 * @author Benjamin Ming Yang @ National Taiwan University (b98204032@gmail.com)
 * @brief sac2tnk is a quick utility to convert a sac file to a tank player tank.
 *        The data from the tank can then be used in tankplayer.
 * @date 2025-05-09
 *
 * @copyright Copyright (c) 2025-now
 *
 */

/* */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
/* */
#include <sac.h>
#include <trace_buf.h>
#include <sachead.h>

/* */
#define PROG_NAME       "sac2tnk"
#define VERSION         "1.0.0 - 2025-05-09"
#define AUTHOR          "Benjamin Ming Yang"

/* */
#define MAX_SCNL_CODE_LEN  8
#define DEF_MAX_SAMPS      100
#define TIMESTAMP_FORMAT   "%04d/%02d/%02d_%02d:%02d:%05.2f"
/**
 * @name Internal Function Prototypes
 *
 */
static char *trim_string( char *, const int );
static char *timestamp_gen( char *, const double );
static int   proc_argv( int, char *[] );
static void  usage( void );

/* */
static char *InputFile   = NULL;
static char *OutputFile  = NULL;
static char *NewSta      = NULL;
static char *NewChan     = NULL;
static char *NewNet      = NULL;
static char *NewLoc      = NULL;
static int   MaxSample   = DEF_MAX_SAMPS;
static float NewSampRate = 0.0f;
static float Multiplier  = 1.0f;
static float GapValue    = SACUNDEF;
/* */
static bool  SeisanChanFix = false;
static bool  AppendOutput  = false;

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
int main( int argc, char **argv )
{
/* */
	struct SAChead sh;
	float         *seis     = NULL;
	float         *seis_ptr = NULL;
	float          seis_max = 0.0;
	float          seis_min = 0.0;
/* */
	FILE          *ofp  = stdout;
	register int   i      = 0;
	register int   size   = 0;
	register int   npts   = 0;
	int            result = -1;
	char           timestamp_str[32] = { 0 };
	double         starttime = 0.0;
	double         delta = 0.0;
	int32_t       *ldata_ptr = NULL;
	TracePacket    outbuf;

/* Check command line arguments */
	if ( proc_argv( argc, argv ) ) {
		usage();
		return -1;
	}

/* Load the SAC file to local memory */
	if ( (size = sac_file_load( InputFile, &sh, &seis )) < 0 )
		goto end_process;
/* */
	if ( sh.delta < 0.001 ){
		fprintf(stderr, "SAC sample period too small: %f! Exiting!\n", sh.delta);
		goto end_process;
	}
/* */
	starttime = sac_reftime_fetch( &sh );
	fprintf(
		stderr, "Input SAC file ref. time is %.3f, end at %.3f. Total %d samples with %.3f delta.\n",
		starttime, starttime + sh.e, sh.npts, sh.delta
	);
/* */
	starttime += sh.b;
	npts       = sh.npts;
	delta      = sh.delta;

/* The main process, include SCNL modify */
	sac_scnl_modify( &sh, NewSta, NewChan, NewNet, NewLoc );
/* */
	memset(&outbuf, 0, MAX_TRACEBUF_SIZ);
/* */
	strncpy(outbuf.trh2.sta, sh.kstnm, TRACE2_STA_LEN > K_LEN ? K_LEN : TRACE2_STA_LEN);
	trim_string( outbuf.trh2.sta, TRACE2_STA_LEN );
/* */
	strncpy(outbuf.trh2.chan, sh.kcmpnm, TRACE2_CHAN_LEN > K_LEN ? K_LEN : TRACE2_CHAN_LEN);
	trim_string( outbuf.trh2.chan, TRACE2_CHAN_LEN );
	if ( SeisanChanFix ) {
	/*
	 * some seisan chans look like: EH Z
	 *                              0123
	 */
		outbuf.trh2.chan[2] = outbuf.trh2.chan[3];
		outbuf.trh2.chan[3] = '\0';
	}
/* */
	strncpy(outbuf.trh2.net, sh.knetwk, TRACE2_NET_LEN > K_LEN ? K_LEN : TRACE2_NET_LEN);
	trim_string( outbuf.trh2.net, TRACE2_NET_LEN );
/* */
	strncpy(outbuf.trh2.loc, sh.khole, TRACE2_LOC_LEN > K_LEN ? K_LEN : TRACE2_LOC_LEN);
	trim_string( outbuf.trh2.loc, TRACE2_LOC_LEN );
	if ( strcmp( outbuf.trh2.loc, SACSTRUNDEF ) && strcmp( outbuf.trh2.loc, "  " ) )
		strcpy( outbuf.trh2.loc, LOC_NULL_STRING );

/* */
	outbuf.trh2.version[0] = TRACE2_VERSION0;
	outbuf.trh2.version[1] = TRACE2_VERSION1;
	strcpy(outbuf.trh2.quality, TRACE2_NO_QUALITY);
	strcpy(outbuf.trh2.pad, TRACE2_NO_PAD);

/* */
	if ( NewSampRate > 0.0 ) {
		delta = 1.0 / NewSampRate;
		outbuf.trh2.samprate = NewSampRate;
	}
	else {
		outbuf.trh2.samprate = 1.0 / delta;
	}
/* */
	strcpy(outbuf.trh2.datatype, "i4");

/* */
	fprintf(
		stderr, "tracebuf start time %s\n",
		timestamp_gen( timestamp_str, starttime )
	);
	fprintf(
		stderr, "tracebuf SCNL       %s.%s.%s.%s\n",
		outbuf.trh2.sta, outbuf.trh2.chan, outbuf.trh2.net, outbuf.trh2.loc
	);
/* If user chose to output the result to local file, then open the file descript to write */
	if ( OutputFile && (ofp = fopen(OutputFile, AppendOutput ? "ab" : "wb")) == (FILE *)NULL ) {
		fprintf(stderr, "ERROR!! Can't open %s for output! Exiting!\n", OutputFile);
		goto end_process;
	}

/* */
	seis_ptr = seis;
/* Then write the seismic data */
	while ( npts > 0 ) {
	/* */
		outbuf.trh2.pinno = 0;
		outbuf.trh2.nsamp = 0;
		ldata_ptr = (int32_t *)(&outbuf.trh2 + 1);
	/* */
		for ( i = 0; outbuf.trh2.nsamp < MaxSample && npts > 0; i++, seis_ptr++, npts-- ) {
			if ( *seis_ptr != GapValue ) {
			/* */
				if ( outbuf.trh2.nsamp == 0 )
					outbuf.trh2.starttime = starttime + delta * i;
			/* */
				if ( *seis_ptr < seis_min )
					seis_min = *seis_ptr;
				else if ( *seis_ptr > seis_max )
					seis_max = *seis_ptr;
			/* */
				*ldata_ptr = (int32_t)(Multiplier * (*seis_ptr));
			/* */
				outbuf.trh2.nsamp++;
				ldata_ptr++;
			}
			else if ( outbuf.trh2.nsamp ) {
				break;
			}
		}
	/* */
		starttime += delta * i;
		outbuf.trh2.endtime = starttime - delta;
	/* */
		if ( outbuf.trh2.nsamp ) {
			size = sizeof(TRACE2_HEADER) + outbuf.trh2.nsamp * sizeof(int32_t);
			if ( fwrite(&outbuf, size, 1, ofp) != 1 ) {
				fprintf(stderr, "Error writing SAC file: %s\n", strerror(errno));
				if ( OutputFile )
					remove(OutputFile);
				goto end_process;
			}
		}
	}
/* Output the final result information */
	fprintf(
		stderr, "SAC      min:max    %f:%f, multiplier %f\n",
		seis_min, seis_max, Multiplier
	);
	fprintf(
		stderr, "tracebuf min:max    %d:%d\n",
		(int32_t)(Multiplier * seis_min), (int32_t)(Multiplier * seis_max)
	);
/* */
	result = 0;

end_process:
	if ( ofp != stdout )
		fclose(ofp);
	if ( seis )
		free(seis);

	return result;
}

/**
 * @brief
 *
 * @param str
 * @param size
 * @return char*
 */
static char *trim_string( char *str, const int size )
{
	char *p = str + size - 1;

/* */
	if ( str && *str ) {
		while ( p >= str && (isspace(*p) || !(*p)) ) {
			*p-- = '\0';
		}
	}

	return str;
}

/**
 * @brief
 *
 * @param buffer
 * @param timestamp
 * @return char*
 */
static char *timestamp_gen( char *buffer, const double timestamp )
{
	struct tm    sptime;
	const time_t _timestamp = (time_t)timestamp;
	double sec_f = timestamp - (double)_timestamp;

/* Checking for buffer size */
	if ( buffer == NULL )
		return buffer;
/* */
	gmtime_r(&_timestamp, &sptime);
	sec_f += (double)sptime.tm_sec;
	sprintf(
		buffer, TIMESTAMP_FORMAT,
		sptime.tm_year + 1900, sptime.tm_mon + 1, sptime.tm_mday,
		sptime.tm_hour, sptime.tm_min, sec_f
	);

	return buffer;
}

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
static int proc_argv( int argc, char *argv[] )
{
/* Parse command line args */
	for ( register int i = 1; i < argc; i++ ) {
	/* check switches */
		if ( !strcmp(argv[i], "-v") ) {
			fprintf(stdout, "%s\n", PROG_NAME);
			fprintf(stdout, "Version: %s\n", VERSION);
			fprintf(stdout, "Author:  %s\n", AUTHOR);
			fprintf(stdout, "Compiled at %s %s\n", __DATE__, __TIME__);
			exit(0);
		}
		else if ( !strcmp(argv[i], "-h") ) {
			usage();
			exit(0);
		}
		else if ( !strcmp(argv[i], "-N") ) {
			if ( strlen(argv[++i]) > MAX_SCNL_CODE_LEN ) {
				fprintf(stderr, "Error: SCNL code length must be less than %d\n", MAX_SCNL_CODE_LEN);
				return -1;
			}
			NewNet = argv[i];
		}
		else if ( !strcmp(argv[i], "-S") ) {
			if ( strlen(argv[++i]) > MAX_SCNL_CODE_LEN ) {
				fprintf(stderr, "Error: SCNL code length must be less than %d\n", MAX_SCNL_CODE_LEN);
				return -1;
			}
			NewSta = argv[i];
		}
		else if ( !strcmp(argv[i], "-C") ) {
			if ( strlen(argv[++i]) > MAX_SCNL_CODE_LEN ) {
				fprintf(stderr, "Error: SCNL code length must be less than %d\n", MAX_SCNL_CODE_LEN);
				return -1;
			}
			NewChan = argv[i];
		}
		else if ( !strcmp(argv[i], "-L") ) {
			if ( strlen(argv[++i]) > MAX_SCNL_CODE_LEN ) {
				fprintf(stderr, "Error: SCNL code length must be less than %d\n", MAX_SCNL_CODE_LEN);
				return -1;
			}
			NewLoc = argv[i];
		}
		else if ( !strcmp(argv[i], "-s") ) {
			NewSampRate = atof( argv[++i] );
		}
		else if ( !strcmp(argv[i], "-c") ) {
			SeisanChanFix = true;
		}
		else if ( !strcmp(argv[i], "-n") ) {
			MaxSample = atoi(argv[++i]);
		}
		else if ( !strcmp(argv[i], "-m") ) {
			Multiplier = atof(argv[++i]);
		}
		else if ( !strcmp(argv[i], "-g") ) {
			GapValue = atof(argv[++i]);
		}
		else if ( !strcmp(argv[i], "-a") ) {
			AppendOutput = true;
		}
		else if ( i == argc - 1 ) {
			InputFile = argv[i];
			OutputFile = NULL;
		}
		else if ( i == argc - 2 ) {
			InputFile = argv[i++];
			OutputFile = argv[i];
		/* Just in case */
			break;
		}
		else {
			fprintf(stderr, "Unknown option: %s\n\n", argv[i]);
			return -1;
		}
	} /* end of command line args for loop */

/* check command line args */
	if ( !InputFile ) {
		fprintf(stderr, "Error, an input file name must be provided\n");
		return -2;
	}
	if ( MaxSample < 1 ) {
		fprintf(stderr, "New max samples is too small (<1): %d\n", MaxSample);
		return -2;
	}
	if ( MaxSample > (MAX_TRACEBUF_SIZ - sizeof(TRACE2_HEADER)) / 4 ) {
		fprintf(
			stderr, "New max samples is too large (>%ld): %d\n",
			(MAX_TRACEBUF_SIZ - sizeof(TRACE2_HEADER)) / 4, MaxSample
		);
		return -2;
	}

	return 0;
}


/**
 * @brief
 *
 */
static void usage( void )
{
	fprintf(stdout, "\n%s\n", PROG_NAME);
	fprintf(stdout, "Version: %s\n", VERSION);
	fprintf(stdout, "Author:  %s\n", AUTHOR);
	fprintf(stdout, "Compiled at %s %s\n", __DATE__, __TIME__);
	fprintf(stdout, "***************************\n");
	fprintf(stderr, "Usage: %s [-c][-m multiplier] [-s sps] [-N NN] [-C CCC] [-S SSSSS] [-L LL] [-n max-samples] <infile> >> <outfile>\n", PROG_NAME);
	fprintf(stderr, "    or %s [-c][-m multiplier] [-s sps] [-N NN] [-C CCC] [-S SSSSS] [-L LL] [-n max-samples] [-a] <infile> <outfile>\n", PROG_NAME);
	fprintf(stdout,
		"*** Options ***\n"
		" -N network_code   The network code to use from the cmdline instead of SAC file\n"
		" -L location_code  The location code to use from the cmdline instead of SAC file\n"
		" -C channel_code   The chan code to use from the cmdline instead of SAC file\n"
		" -S station_code   The station name to use from the cmdline instead of SAC file\n"
		" -s samp_rate      Use this sample rate instead of from SAC file\n"
		" -c                A flag to fix a SEISAN problem with chans written in as EH Z\n"
		" -m multiplier     A scale factor applied to the SAC float data\n"
		" -g gap_value      A gap value to the SAC float data that will be skiped\n"
		" -a                A flag to append output to named outfile\n"
		" -h                Show this usage message\n"
		" -v                Report program version\n"
		"\n"
		"This program will convert the input SAC file to a SAC file.\n"
		"\n"
	);

	return;
}
