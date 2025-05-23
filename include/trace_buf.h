/**
 * @file trace_buf.h
 *
 * @remark 2025/05/09 13:57:10 by Benjamin: Modernize the whole format.
 * @remark 2004/04/12 21:47:10 by dietz: Added definitions for version field bytes and null location string
 * @remark 2004/04/08 17:25:45 by dietz: Added TRACE2_HEADER structure (for TYPE_TRACEBUF2 messages) which
 *         includes new location code and version fields.
 * @remark 2002/03/20 20:14:52 by lombard: Size of net string was specified as TRACE_CHAN_LEN and channel string
 *         as TRACE_NET_LEN. While these macros currently have the same value, they may not in the future.
 *         The string sizes have been corrected.
 * @remark 2001/03/15 01:22:44 by dietz: Added TRACE_LOC_LEN definition
 * @remark 2000/02/14 20:05:54 by lucky: Initial revision
 * @remark November 1996: KGL Added net and quality fields to accommodate needs expressed by Alex Bittenbinder and the Earthworm team
 * @remark February 1997: KGL Added pad field as used by Earthworm team Replaced hardcoded string lengths for S-C-N with
 *         #defines so that they can be used elsewhere. LV 5/1999
 *
 * @author Kent Lindquist and Roger Hansen, Geophysical Institute, University of Alaska, Fairbanks
 * @brief Header file for Earthworm packets that allow demultiplexed data from various sources to be handled efficiently.
 * @date 1996-02-01
 *
 * @copyright Copyright (c) 1996-now
 *
 */
#pragma once

/**
 * @brief Define maximum size of tracebuf message
 *
 */
#define MAX_TRACEBUF_SIZ 4096

/**
 * @name
 *
 */
#define TRACE_STA_LEN       7
#define TRACE_CHAN_LEN      9  /* 4 bytes plus padding for loc and version */
#define TRACE_NET_LEN       9
#define TRACE_LOC_LEN       3
/* */
#define NETWORK_NULL_STRING "-"

/**
 * @brief Definition of original TYPE_TRACEBUF header with CSS3.0-length SNC fields
 *
 * @note The principal time fields in the TRACE_HEADER are: starttime, nsamp, and samprate.
 *       The endtime field is included as a redundant convenience.
 *
 */
typedef struct {
	int     pinno;                 /* Pin number */
	int     nsamp;                 /* Number of samples in packet */
	double  starttime;             /* time of first sample in epoch seconds (seconds since midnight 1/1/1970) */
	double  endtime;               /* Time of last sample in epoch seconds */
	double  samprate;              /* Sample rate; nominal */
	char    sta[TRACE_STA_LEN];    /* Site name */
	char    net[TRACE_NET_LEN];    /* Network name */
	char    chan[TRACE_CHAN_LEN];  /* Component/channel code */
	char    datatype[3];           /* Data format code */
	char    quality[2];            /* Data-quality field */
	char    pad[2];                /* padding */
} TRACE_HEADER;

/**
 * @name
 *
 */
#define TRACE2_STA_LEN    7    /* SEED: 5 chars plus terminating NULL */
#define TRACE2_NET_LEN    9    /* SEED: 2 chars plus terminating NULL */
#define TRACE2_CHAN_LEN   4    /* SEED: 3 chars plus terminating NULL */
#define TRACE2_LOC_LEN    3    /* SEED: 2 chars plus terminating NULL */
/* */
#define TRACE2_VERSION0  '2'   /* version[0] for TYPE_TRACEBUF2       */
#define TRACE2_VERSION1  '0'   /* version[1] for TYPE_TRACEBUF2       */
#define TRACE2_VERSION11 '1'   /* version[1] for TYPE_TRACEBUF21      */
/* */
#define LOC_NULL_STRING  "--"  /* NULL string for location code field */

/**
 * @brief Definition of TYPE_TRACEBUF2 header with SEED SNCL fields.
 *
 * @remarks The new TRACE2_HEADER is the same length as the original TRACE_HEADER.
 *          + sta and net fields remain unchanged (longer than required by SEED).
 *          + chan field is shortened to appropriate SEED length.
 *          + loc and version fields were added in the extra chan field bytes.
 *          + all other fields remain unchanged (same length, same position).
 *
 * @note The principal time fields in the TRACE_HEADER are: starttime, nsamp, and samprate.
 *       The endtime field is included as a redundant convenience.
 *
 */
typedef struct {
	int     pinno;                 /* Pin number */
	int     nsamp;                 /* Number of samples in packet */
	double  starttime;             /* time of first sample in epoch seconds (seconds since midnight 1/1/1970) */
	double  endtime;               /* Time of last sample in epoch seconds */
	double  samprate;              /* Sample rate; nominal */
	char    sta[TRACE2_STA_LEN];   /* Site name (NULL-terminated) */
	char    net[TRACE2_NET_LEN];   /* Network name (NULL-terminated) */
	char    chan[TRACE2_CHAN_LEN]; /* Component/channel code (NULL-terminated)*/
	char    loc[TRACE2_LOC_LEN];   /* Location code (NULL-terminated) */
	char    version[2];            /* version field */
	char    datatype[3];           /* Data format code (NULL-terminated) */
/* quality and pad are available in version 20, see TRACE2X_HEADER */
	char    quality[2];            /* Data-quality field */
	char    pad[2];                /* padding */
} TRACE2_HEADER;

/**
 * @brief Definition of TYPE_TRACEBUF2X header with SEED SNCL fields.
 *
 */
typedef struct {
	int     pinno;                 /* Pin number */
	int     nsamp;                 /* Number of samples in packet */
	double  starttime;             /* time of first sample in epoch seconds (seconds since midnight 1/1/1970) */
	double  endtime;               /* Time of last sample in epoch seconds */
	double  samprate;              /* Sample rate; nominal */
	char    sta[TRACE2_STA_LEN];   /* Site name (NULL-terminated) */
	char    net[TRACE2_NET_LEN];   /* Network name (NULL-terminated) */
	char    chan[TRACE2_CHAN_LEN]; /* Component/channel code (NULL-terminated)*/
	char    loc[TRACE2_LOC_LEN];   /* Location code (NULL-terminated) */
	char    version[2];            /* version field */
	char    datatype[3];           /* Data format code (NULL-terminated) */
	union {
		struct {
			char    quality[2];            /* Data-quality field */
			char    pad[2];                /* padding */
		} v20;
		struct {
			float   conversion_factor;     /* Conversion factor */
		} v21;
	} x;
} TRACE2X_HEADER;

/**
 * @brief Definition of a generic TraceBuf Packet
 *
 */
typedef union {
	char           msg[MAX_TRACEBUF_SIZ];
	TRACE_HEADER   trh;
	TRACE2_HEADER  trh2;
	TRACE2X_HEADER trh2x;
	int            i;
} TracePacket;

/**
 * @name Byte 0 of data quality flags, as in SEED format
 *
 */
#define AMPLIFIER_SATURATED    0x01
#define DIGITIZER_CLIPPED      0x02
#define SPIKES_DETECTED        0x04
#define GLITCHES_DETECTED      0x08
#define MISSING_DATA_PRESENT   0x10
#define TELEMETRY_SYNCH_ERROR  0x20
#define FILTER_CHARGING        0x40
#define TIME_TAG_QUESTIONABLE  0x80

/**
 * @brief CSS datatype codes
 *
 * t4      SUN IEEE single precision real
 * t8      SUN IEEE double precision real
 * s4      SUN IEEE integer
 * s2      SUN IEEE short integer
 * f4      VAX/Intel IEEE single precision real
 * f8      VAX/Intel IEEE double precision real
 * i4      VAX/Intel IEEE integer
 * i2      VAX/Intel IEEE short integer
 * g2      NORESS gain-ranged
 *
 */

/**
 * @name
 *
 */
#define TRACE2_NO_QUALITY            "\0"
#define TRACE2_NO_PAD                "\0"
#define TRACE2_NO_CONVERSION_FACTOR  0.0f

/**
 * @name Macros
 *
 */
#define TRACE2_HEADER_VERSION_IS_VALID(__TRH2) \
		((__TRH2)->version[0] == TRACE2_VERSION0 && \
		((__TRH2)->version[1] == TRACE2_VERSION1 || \
		(__TRH2)->version[1] == TRACE2_VERSION11))

#define TRACE2_HEADER_VERSION_IS_20(__TRH2) \
		((__TRH2)->version[0] == TRACE2_VERSION0 && \
		(__TRH2)->version[1] == TRACE2_VERSION1)

#define TRACE2_HEADER_VERSION_IS_21(__TRH2) \
		((__TRH2)->version[0] == TRACE2_VERSION0 && \
		(__TRH2)->version[1] == TRACE2_VERSION11)

#define GET_TRACE2_QUALITY(__TRH2) \
		(TRACE2_HEADER_VERSION_IS_20(__TRH2) ? \
		(__TRH2)->quality : TRACE2_NO_QUALITY)

#define GET_TRACE2_PAD(__TRH2) \
		(TRACE2_HEADER_VERSION_IS_20(__TRH2) ? \
		(__TRH2)->pad : TRACE2_NO_PAD)

#define GET_TRACE2_CONVERSION_FACTOR(__TRH2) \
		(TRACE2_HEADER_VERSION_IS_21(__TRH2) ? \
		((TRACE2X_HEADER *)(__TRH2))->x.v21.conversion_factor : TRACE2_NO_CONVERSION_FACTOR)
