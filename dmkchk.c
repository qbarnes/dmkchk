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


/* prev_total_sectors is both an input and output parameter. */
int
track_check(struct dmk_state *dmkst, int side, int track,
		int verbose, int *prev_total_sectors)
{
	uint8_t	*data = NULL;

	if (!dmk_seek(dmkst, track, side)) {
		printf("Seek error on track %d, side %d!\n", track, side);
		goto error;
	}

	unsigned int	bad_ids[uint_to_bits(256)] = { 0 };
	unsigned int	bad_sectors[uint_to_bits(256)] = { 0 };
	int		id_errors = 0, sector_errors = 0, total_sectors = 0;
	sector_info_t	si;

	int	idr;
	while ((idr = dmk_read_id_with_crcs(dmkst, &si, NULL, NULL))) {
		if (idr == -1) {
			++id_errors;
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
		if (dmk_read_sector_with_crcs(dmkst, &si, data,
							NULL, NULL) != 1) {
			set_bit(bad_sectors, si.sector);
			++sector_errors;
		}

		free(data);
		data = NULL;
	}

	if (*prev_total_sectors == -1)
		*prev_total_sectors = total_sectors;

	if (id_errors ||
	    sector_errors || (*prev_total_sectors != total_sectors) ||
	    (verbose > 1)) {

		printf("%3d/%1d      %3d       ", track, side, total_sectors);

		if (*prev_total_sectors == total_sectors) {
			printf("          ");
		} else {
			printf("Y      %3d", *prev_total_sectors);
		}

		printf("     %3d     %3d      ", sector_errors, id_errors);

		int reports = 0;
		int start_range = -1, end_range = -1;

		for (int i = 0; i < 256; ++i) {
			int bad = test_bit(bad_ids, i) ||
				  test_bit(bad_sectors, i);

			if (bad) {
				end_range = i;
				if (start_range == -1)
					start_range = i;
			}

			if ((!bad || i == 255) &&
			     end_range != -1) {

				if (reports)
					printf(", ");
				else
					reports = 1;

				if (start_range != -1 &&
				    start_range != end_range)
					printf("%u-", start_range);

				printf("%u", end_range);

				start_range = end_range = -1;
			}
		}

		printf("\n");
	}

	*prev_total_sectors = total_sectors;

error:
	if (data)
		free (data);

	return sector_errors + id_errors;
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
process_files(int file_count, char **file_list, int verbose)
{
	for (int fi = 0; fi < file_count; ++fi) {
		struct dmk_state *dmkst;
		int	tracks, ds, dd;

		char *fn = file_list[fi];

		if (file_count > 1)
			printf("%sFile: %s\n", (fi ? "\n\n" : ""), fn);

		if (verbose)
			print_header();

		dmkst = dmk_open_image(fn, 0, &ds, &tracks, &dd);
		if (!dmkst) {
			fprintf(stderr, "Failed to open '%s' (%d [%s]).\n",
				fn, errno, strerror(errno));
			return 2;
		}

		int	errs = 0;
		int	total_sectors = -1;

		for (int t = 0; t < tracks; ++t)
			for (int s = 0; s <= ds; ++s)
				errs += track_check(dmkst, s, t, verbose,
						   &total_sectors);

		printf("\nTotal errors found: %d\n", errs);

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
	fprintf(stderr, "Usage: %s: [option ...] {dmkfile ...}\n", pgmname);
	fprintf(stderr, "Report sector errors in DMK file.\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -h		Display help and exit\n");
	fprintf(stderr, "    -v		verbose (repeat for more verbosity)\n");
	fprintf(stderr, "    -V		Display version and exit\n");

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
	int	verbose = 0;
	int	ch;

	do {
		ch = getopt(argc, argv, "hvV?");

		switch(ch) {
		case 'h':
		case '?':
			usage(argv[0], 0);
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

	return process_files(argc - optind, &argv[optind], verbose);
}
