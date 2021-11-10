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

int
track_check(struct dmk_state *dmkst, int side, int track)
{
	if (!dmk_seek(dmkst, track, side)) {
		printf("Seek error on track %d, side %d!\n", track, side);
		goto error;
	}

	uint8_t		*data = NULL;
	unsigned int	bad_sectors[uint_to_bits(256)] = { 0 };
	int		sector_errors = 0, total_sectors = 0;
	sector_info_t	si;

	for (int sector = 0;
	     (sector < DMK_MAX_SECTOR) && dmk_read_id(dmkst, &si); ++sector) {

		size_t	data_size = 128 << si.size_code;
		data = malloc(data_size);

		if (!data) {
			fprintf(stderr, "Failed to allocate %zu bytes for "
				"sector buffer.\n", data_size);
			goto error;
		}

		++total_sectors;
		if (!dmk_read_sector(dmkst, &si, data)) {
			set_bit(bad_sectors, si.sector);
			++sector_errors;
		}

		free(data);
		data = NULL;
	}

	if (sector_errors) {
		printf("Track %d, side %d (%d/%d): ",
			track, side, sector_errors, total_sectors);

		int reports = 0;
		int start_range = -1, end_range = -1;

		for (int i = 0; i < 256; ++i) {
			if (test_bit(bad_sectors, i)) {
				end_range = i;
				if (start_range == -1)
					start_range = i;
			}

			if ((!test_bit(bad_sectors, i) || i == 255) &&
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

error:
	if (data)
		free (data);

	return sector_errors;
}


void
usage(const char *pgmname, int exitval)
{
	fprintf(stderr, "Usage: %s: [option ...] {dmkfile ...}\n", pgmname);
	fprintf(stderr, "Report sector errors in DMK file.\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -h		Display help and exit\n");
	fprintf(stderr, "    -v		verbose\n");
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
			verbose = 1;
			break;
		case 'V':
			printf("Version: %s\n", VERSION);
			return 0;
		}

	} while (ch != -1);

	if (optind >= argc)
		fatal(argv[0], "Fatal: Must provide DMK file name argument.\n");


	for (int fi = optind; fi < argc; ++fi) {
		struct dmk_state *dmkst;
		int	tracks, ds, dd;

		char *fn = argv[fi];

		if (verbose)
			printf("File: %s\n", fn);

		dmkst = dmk_open_image(fn, 0, &ds, &tracks, &dd);
		if (!dmkst) {
			fprintf(stderr, "Failed to open '%s' (%d [%s]).\n",
				fn, errno, strerror(errno));
			return 2;
		}

		int	bse = 0;

		for (int t = 0; t < tracks; ++t)
			for (int s = 0; s <= ds; ++s)
				bse += track_check(dmkst, s, t);

		if (!bse)
			printf("No bad sectors found.\n");

		if (verbose && (fi < (argc-1)))
			printf("\n");

		if (!dmk_close_image(dmkst)) {
			fprintf(stderr, "Close of '%s' failed.\n", fn);
			return 1;
		}
	}

	return 0;
}
