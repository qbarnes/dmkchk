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

int
track_check(struct dmk_state *dmkst, int side, int track)
{
	uint8_t	*data = NULL;

	if (!dmk_seek(dmkst, track, side)) {
		printf("Seek error on track %d, side %d!\n", track, side);
		goto error;
	}

	sector_info_t	si;
	int		serrs = 0;

	for (int sector = 0;
	     (sector < DMK_MAX_SECTOR) && dmk_read_id(dmkst, &si); ++sector) {

		size_t	data_size = 128 << si.size_code;
		data = malloc(data_size);

		if (!data) {
			fprintf(stderr, "Failed to allocate %lu bytes for "
				"sector buffer.\n", data_size);
			goto error;
		}

		if (!dmk_read_sector(dmkst, &si, data)) {
			if (++serrs == 1)
				printf("Track %d, side %d: ", track, side);

			if (serrs > 1)
				printf(", ");

			printf("%d", sector);
		}

		free(data);
		data = NULL;
	}
	
	if (serrs)
		printf("\n");

error:
	if (data)
		free (data);
	return serrs;
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

		for (int s = 0; s <= ds; ++s) {
			for (int t = 0; t < tracks; ++t)
				bse += track_check(dmkst, s, t);
		}

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
