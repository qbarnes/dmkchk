/*
 * Reports bad sectors in DMK floppy file.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "libdmk.h"

#ifndef VERSION
#define VERSION "0.uncontrolled"
#endif

#define DIV_ROUND_UP(n, d)      (((n) + (d) - 1) / (d))
#define type_to_bits(type)      (sizeof(type) * 8)
#define uint_to_bits(nr)        DIV_ROUND_UP(nr, type_to_bits(unsigned int))

#define set_bit(A,k)            (A[(k)/type_to_bits(*A)] |= \
                                   ((typeof(*A))1 << ((k) % type_to_bits(*A))))
#define clear_bit(A,k)          (A[(k)/type_to_bits(*A)] &= \
                                  ~((typeof(*A))1 << ((k) % type_to_bits(*A))))
#define test_bit(A,k)        (!!(A[(k)/type_to_bits(*A)] & \
                                   ((typeof(*A))1 << ((k) % type_to_bits(*A)))))

struct error_stats {
	unsigned int	id_crc_errors;
	unsigned int	sector_crc_errors;
	unsigned int	sector_missing_errors;
};


int
format_range(const unsigned int *bitmap,
	     unsigned int max_range,
	     size_t bufsz, char *buf)
{
	int	reports = 0;
	int	start_range = -1, end_range = -1;
	char	*orig_buf = buf;

	for (int i = 0; i < max_range+1; ++i) {
		int bad = test_bit(bitmap, i);

		if (bad) {
			end_range = i;
			if (start_range == -1)
				start_range = i;
		}

		if ((!bad || i == max_range) &&
		     end_range != -1) {

			if (reports)
				buf += snprintf(buf, bufsz, ", ");
			else
				reports = 1;

			if (start_range != -1 &&
			    start_range != end_range)
				buf += snprintf(buf, bufsz, "%u-", start_range);

			buf += snprintf(buf, bufsz, "%u", end_range);

			start_range = end_range = -1;
		}
	}

	return buf - orig_buf;
}

/* prev_total_sectors and es are both input and output parameters. */
unsigned int
track_check(struct dmk_state *dmkst, int side, int track,
		int verbose, int *prev_total_sectors,
		struct error_stats *es)
{
	uint8_t	*data = NULL;

	if (!dmk_seek(dmkst, track, side)) {
		printf("Seek error on track %d, side %d!\n", track, side);
		goto error;
	}

	unsigned int	bad_ids[uint_to_bits(256)] = { 0 };
	unsigned int	bad_sectors[uint_to_bits(256)] = { 0 };
	unsigned int	missing_sectors[uint_to_bits(256)] = { 0 };

	struct error_stats	les = {0};
	int			total_sectors = 0;
	sector_info_t		si;

	int	idr;
	while ((idr = dmk_read_id_with_crcs(dmkst, &si, NULL, NULL))) {
		if (idr == -1) {
			++les.id_crc_errors;
			set_bit(bad_ids, si.sector);
		}

		size_t	data_size = dmk_sector_size(&si);
		data = malloc(data_size);

		if (!data) {
			fprintf(stderr, "Failed to allocate %zu bytes for "
				"sector buffer.\n", data_size);
			goto error;
		}

		++total_sectors;

		int srr = dmk_read_sector_with_crcs(dmkst, &si, data,
						    NULL, NULL);

		if (srr == -1) {
			set_bit(bad_sectors, si.sector);
			++les.sector_crc_errors;
		} else if (srr == 0 && idr != -1) {
			/* Only count if there wasn't also an ID error. */
			set_bit(missing_sectors, si.sector);
			++les.sector_missing_errors;
		}

		free(data);
		data = NULL;
	}

	if (*prev_total_sectors == -1)
		*prev_total_sectors = total_sectors;

	if (les.id_crc_errors ||
	    les.sector_crc_errors ||
	    les.sector_missing_errors ||
	    (*prev_total_sectors != total_sectors) ||
	    (verbose > 1)) {

		printf("%3d/%1d      %3d       ", track, side, total_sectors);

		if (*prev_total_sectors == total_sectors) {
			printf("          ");
		} else {
			printf("Y      %3d", *prev_total_sectors);
		}

		printf("     %3d     %3d      ",
		       les.sector_crc_errors + les.sector_missing_errors,
		       les.id_crc_errors);

		char sbuf[256];
		static const char btsem[] =
			"Buffer for format range too small (%d).\n";

		int blen = format_range(bad_sectors, 255, sizeof(sbuf), sbuf);
		if (blen >= sizeof(sbuf)) {
			fprintf(stderr, btsem, blen);
			goto error;
		}

		if (blen) printf("C: %s", sbuf);

		int mlen = format_range(missing_sectors, 255,
					sizeof(sbuf), sbuf);
		if (mlen >= sizeof(sbuf)) {
			fprintf(stderr, btsem, mlen);
			goto error;
		}

		if (mlen) printf("%sM: %s", (blen ? "; " : ""), sbuf);

		int ilen = format_range(bad_ids, 255, sizeof(sbuf), sbuf);

		if (ilen >= sizeof(sbuf)) {
			fprintf(stderr, btsem, ilen);
			goto error;
		}

		if (ilen) printf("%sID: %s", (blen||mlen ? "; " : ""), sbuf);

		printf("\n");
	}

	*prev_total_sectors = total_sectors;

error:
	if (data)
		free (data);

	es->sector_crc_errors     += les.sector_crc_errors;
	es->sector_missing_errors += les.sector_missing_errors;
	es->id_crc_errors         += les.id_crc_errors;

	return les.sector_crc_errors +
	       les.sector_missing_errors +
	       les.id_crc_errors;
}


void
print_header()
{
	static const char *const hdr[] = {
		"Trk/Side  Sector  Sec Cnt  Prev    Sector  ID Field  Error",
		"          Count   Change?  SecCnt  Errors  Errors    List"
	};

	puts(hdr[0]);
	puts(hdr[1]);

	for (const char *s = hdr[0]; *s; ++s)
		putchar('-');

	putchar('\n');
}


int
process_files(int file_count, char **file_list,
	      int print_headfoot, int verbose)
{
	for (int fi = 0; fi < file_count; ++fi) {
		struct dmk_state *dmkst;
		int	tracks, ds, dd;

		char *fn = file_list[fi];

		if (print_headfoot) {
			printf("%sFile: %s\n\n", (fi ? "\n\n" : ""), fn);
			print_header();
		} else {
			if (fi) printf("\n\n");
		}

		dmkst = dmk_open_image(fn, 0, &ds, &tracks, &dd);
		if (!dmkst) {
			fprintf(stderr, "Failed to open '%s' (%d [%s]).\n",
				fn, errno, strerror(errno));
			return 2;
		}

		int			total_sectors = -1;
		struct error_stats	restats = {0};

		for (int t = 0; t < tracks; ++t)
			for (int s = 0; s <= ds; ++s)
				track_check(dmkst, s, t, verbose,
					    &total_sectors,
					    &restats);

		printf("\n");

		if (print_headfoot && restats.sector_crc_errors)
			printf("Total sector CRC errors (C): %u\n",
				restats.sector_crc_errors);

		if (print_headfoot && restats.sector_missing_errors)
			printf("Total sectors missing (M): %u\n",
				restats.sector_missing_errors);

		if (print_headfoot && restats.id_crc_errors)
			printf("Total ID Field CRC errors (ID): %u\n",
				restats.id_crc_errors);

		if (!dmk_close_image(dmkst)) {
			fprintf(stderr, "Close of '%s' failed.\n", fn);
			return 1;
		}
	}

	return 0;
}


void
usage(const char *pgmname, int exitval)
{
	static const char usage[] =
		"Usage: %s: [option ...] {dmkfile ...}\n"
		"Report ID Field and sector errors in the DMK files.\n"
		"Options:\n"
		"    -h		Display help and exit\n"
		"    -s		suppress headers and footers\n"
		"    -v		verbose (repeat for more verbosity)\n"
		"    -V		Display version and exit\n";

	fprintf(stderr, usage, pgmname);

	exit(exitval);
}


void
fatal(const char *pgmname, const char *fmsg)
{
	fprintf(stderr, fmsg);
	usage(pgmname, 1);
}


int
main(int argc, char **argv)
{
	int	print_headfoot = 1;
	int	verbose = 0;
	int	ch;

	do {
		ch = getopt(argc, argv, "hsvV?");

		switch(ch) {
		case 'h':
		case '?':
			usage(argv[0], 0);
			break;
		case 's':
			print_headfoot = 0;
			break;
		case 'v':
			++verbose;
			break;
		case 'V':
			printf("Version: %s\n", VERSION);
			return 0;
		}

	} while (ch != -1);

	if (optind >= argc)
		fatal(argv[0], "Fatal: Must provide DMK file name argument.\n");

	return process_files(argc - optind, &argv[optind],
			     print_headfoot, verbose);
}
