/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char **argv)
{
    int i;
    int fd;
    char buf[81];
    unsigned char par;
    int code;
    double ratio;
    double width, height;

    if (argc != 3) {
	fprintf(stderr,"Usage: %s gif_file aspect_ratio\n", argv[0]);
	return -4;
    }

    /* Read the aspect ratio */
    if (strspn(argv[2], "0123456789.") == strlen(argv[2])) {
	ratio = atof(argv[2]);
    } else {
	fd = open(argv[2], O_RDONLY);
	if (fd < 0) {
	    fprintf(stderr,"Cannot open FITS file: %s\n", argv[1]);
	    return -5;
	}
	width = height = 0;
	while (1) {
	    if (read(fd, buf, 80) != 80) {
		fprintf(stderr,"FITS file lacks aspect ratio info\n");
		return -6;
	    }
	    if (strncmp(buf,"CDELT1", 6) == 0) {
		/* Set pix width */
		width = atof(buf+10);
	    } else if (strncmp(buf,"CDELT2", 6) == 0) {
		/* Set pix height */
		height = atof(buf+10);
	    } else if (strncmp(buf,"END", 2) == 0) {
		/* End of header */
		break;
	    }
	    if (width && height) {
		break;
	    }
	}
	close(fd);
	ratio = width / height;
    }

    /* Convert aspect ratio to GIF's PixelAspectRatio code */
    code = (int)(64 * ratio - 15 + 0.5);
    if (code > 255) {
	code = 255;
    } else if (code < 1) {
	code = 1;
    }
    par = (unsigned char)code;

    /* Plug code into the GIF file */
    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
	fprintf(stderr,"Cannot open GIF file: %s\n", argv[1]);
	return -1;
    }

    if (read(fd, buf, 6) != 6) {
	fprintf(stderr,"File is too short\n");
	return -2;
    }
    if (strncmp(buf,"GIF89a", 6) != 0) {
	fprintf(stderr,"Not a GIF 89 file\n");
	return -3;
    }

    pwrite(fd, &par, 1, 12);	/* 13'th byte is aspect ratio code */
    close(fd);

    return 0;
}
