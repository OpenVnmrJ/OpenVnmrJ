/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

/* 
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include "graphics.h"
#include "stderr.h"
#include "gtools.h"
#include "imginfo.h"
#include "file_format.h"
#include "initstart.h"
#include "msgprt.h"

Fileformat *fileformat = NULL;


Fileformat::Fileformat()
{
    char buf[100];
    int i;
    char initname[1024];	// init file
    const int maxrows = 10;
    Panel panel;
    int xitempos = 5;
    int xpos;
    int yitempos = 5;
    int ypos;

    fd_init = NULL;
    frame = NULL;
    popup = NULL;
    strcpy(label, "FDF");
    strcpy(datatype, "float");
    datasize = 32;
    *conversion_script = '\0';

    // Get the initialized file of window position
    (void)init_get_win_filename(initname);

    // Get the position of the control panel
    if (init_get_val(initname, "FILE_FORMAT", "dd", &xpos, &ypos) == NOT_OK){
	xpos = 50;
	ypos = 50;
    }

    frame = xv_create(NULL, FRAME, NULL);

    popup = xv_create(frame, FRAME_CMD,
		      XV_X, xpos,
		      XV_Y, ypos,
		      FRAME_LABEL, "Output File Format",
		      FRAME_CMD_PUSHPIN_IN, TRUE,
		      NULL);

    panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);

    formattype_widget =
    xv_create(panel, PANEL_CHOICE_STACK,
	      XV_X, xitempos,
	      XV_Y, yitempos,
	      PANEL_LABEL_STRING, "Format Type:",
	      PANEL_NOTIFY_PROC, Fileformat::format_type_callback,
	      NULL);
    open_init_file();		// Make sure we are at beginning of file
    for (i=0; get_next_label(buf, sizeof(buf)); i++){
	xv_set(formattype_widget, PANEL_CHOICE_STRING, i, buf, NULL);
    }
    close_init_file();

    yitempos += (int)xv_get(formattype_widget, XV_HEIGHT) + 5;
    datatype_widget =
    xv_create(panel, PANEL_CHOICE,
	      XV_X, xitempos,
	      XV_Y, yitempos,
	      PANEL_LABEL_STRING, "Data Type:",
	      PANEL_CHOICE_STRINGS, "Integer", "Float", NULL,
	      PANEL_VALUE, 1,
	      PANEL_NOTIFY_PROC, Fileformat::data_type_callback,
	      NULL);

    yitempos += (int)xv_get(datatype_widget, XV_HEIGHT) + 5;
    datasize_widget =
    xv_create(panel, PANEL_CHOICE,
	      XV_X, xitempos,
	      XV_Y, yitempos,
	      PANEL_LABEL_STRING, "Data Size:",
	      PANEL_CHOICE_STRINGS, "8", "16", "32", NULL,
	      PANEL_VALUE, 2,
	      PANEL_NOTIFY_PROC, Fileformat::data_size_callback,
	      NULL);

    window_fit(panel);
    window_fit(popup);
    window_fit(frame);
    xv_set(popup, XV_SHOW, TRUE, NULL);
}

void
Fileformat::close_init_file()
{
    if (fd_init){
	fclose(fd_init);
	fd_init = 0;
    }
}

//
//	Returns the type of data as a string: "integer" or "float"
//
char *
Fileformat::data_type()
{
    return fileformat->datatype;
}

//
//	Sets the type of data as a string: "integer" or "float"
//
void
Fileformat::set_data_type(char *type)
{
    if (strcasecmp(type, "integer") == 0){
	xv_set(datatype_widget, PANEL_VALUE, 0, NULL);
    }else if (strcasecmp(type, "float") == 0){
	xv_set(datatype_widget, PANEL_VALUE, 1, NULL);
    }else{
	msgerr_print("Illegal data type specified: \"%s\"", type);
	return;
    }
    strcpy(datatype, type);	// Note: string will fit, just checked it.
}

//
//	Callback routine for the "Data Type" choice.
//	Sets fileformat->datatype.
//	(STATIC)
//
void
Fileformat::data_type_callback(Panel_item, int value, Event *)
{
    switch (value){
      case 0: strcpy(fileformat->datatype, "integer"); break;
      case 1: strcpy(fileformat->datatype, "float"); break;
    }
}

//
//	Returns the data word size in bits.
//
int
Fileformat::data_size()
{
    return fileformat->datasize;
}

//
//	Sets the data word size in bits.
//
void
Fileformat::set_data_size(int size)
{
    switch (size){
      case 8:
	xv_set(datasize_widget, PANEL_VALUE, 0, NULL);
	break;
      case 16:
	xv_set(datasize_widget, PANEL_VALUE, 1, NULL);
	break;
      case 32:
	xv_set(datasize_widget, PANEL_VALUE, 2, NULL);
	break;
      default:
	msgerr_print("Illegal data size specified: %d", size);
	return;
    }
    datasize = size;
}

//
//	Callback routine for the "Data Size" choice.
//	Sets fileformat->datasize.
//	(STATIC)
//
void
Fileformat::data_size_callback(Panel_item, int value, Event *)
{
    switch (value){
      case 0: fileformat->datasize = 8; break;
      case 1: fileformat->datasize = 16; break;
      case 2: fileformat->datasize = 32; break;
    }
}

//
//	Callback routine for the "Format Type" menu.
//	Sets the "conversion_string".
//	(STATIC)
//
void
Fileformat::format_type_callback(Panel_item item, int value, Event *)
{
    char buf[16];
    char *err;
    char line[1024];

    // Set the new label value.
    strcpy(fileformat->label, (char *)xv_get(item, PANEL_CHOICE_STRING, value));
    //STDERR_2("menu choice = %d = %s", value, fileformat->label);/*CMP*/

    // Look through init file for format type "label".
    // Read in the associated "conversion_script", "data_type",
    // and "data_size".
    fileformat->open_init_file();
    // Find "format" line with type "label".
    while (err=fgets(line, sizeof(line), fileformat->fd_init)){
	if (strncmp("format", line, 6) == 0){
	    sscanf(&line[6],"%15s", buf);
	    if (strcmp(fileformat->label, buf) == 0){
		break;
	    }
	}
    }
    if (err == NULL){
	msgerr_print("Format type \"%s\" not found in fileformat.init file.",
		 fileformat->label);
	msgerr_print("Has the file changed since Image Browser was started?");
	return;
    }

    // Look for properties of this data format
    // --i.e., check lines only until next "format" line.
    while (fgets(line, sizeof(line), fileformat->fd_init)){
	if (strncmp("format", line, 6) == 0){
	    break;
	}else if (strncmp("script", line, 6) == 0){
	    strncpy(fileformat->conversion_script, &line[6], 1000);
	}
    }
}

//
//	Read in the next file format label.
//	Puts the first "buflen" characters of the label into "buf".
//	On error, returns FALSE;
//
int
Fileformat::get_next_label(char *buf, int buflen)
{
    char *err;
    char fmtlabel[16];
    char line[1024];
    int rtn = TRUE;

    // Find next "format" line.
    while (err=fgets(line, sizeof(line), fd_init)){
	if (strncmp("format", line, 6) == 0){
	    sscanf(&line[6],"%15s", fmtlabel);
	    strncpy(buf, fmtlabel, buflen);
	    buf[buflen-1] = '\0';
	    break;
	}
    }
    if (err == NULL){
	rtn = FALSE;
    }

    return rtn;
}

//
//	Get the script for converting from the standard output format
//	to the desired format.
//	Reads the "conversion_script" and
//	substitutes the specified strings "infile" and "outfile" for
//	the tokens "$1" and "$2", respectively.
//	("$x" is converted into "x" if "x" is neither "1" nor "2".)
//	Returns a string pointer to the appropriate "sh" command(s).
//
char *
Fileformat::get_script(char *infile, char *outfile)
{
    static char buf[1024];
    char *end;
    char *start;

    *buf = '\0';
    start = conversion_script;
    while (end = strchr(start, '$')){
	strncat(buf, start, end-start);
	end++;
	if (*end == '1'){
	    strcat(buf, infile);
	}else if (*end == '2'){
	    strcat(buf, outfile);
	}else{
	    strncat(buf, end, 1);
	}
	start = end + 1;
    }
    strcat(buf, start);
    return buf;
}

//
//	Opens the "fileformats" init file for reading--or rewinds it if
//	it is already open.
//	Returns TRUE on success, FALSE on failure.
//
int
Fileformat::open_init_file()
{
    char path[1025];

    if (fd_init){
	rewind(fd_init);
    }else{
	char *pc;
	if (!(pc=getenv("BROWSERDIR"))){
	    msgerr_print("Environment variable \"BROWSERDIR\" undefined");
	    return FALSE;
	}
	sprintf(path,"%.1000s/fileformats.init", pc);
	fd_init = fopen(path, "r");
	if (!fd_init){
	    msgerr_print("Format init file missing: %s", path);
	    return FALSE;
	}
    }
    return TRUE;
}

//
//	Show the output file format properties window.
//	(STATIC)
//
void
Fileformat::show_window()
{
    if (fileformat == NULL){
	fileformat = new Fileformat;
    }else{
	xv_set(fileformat->popup, XV_SHOW, TRUE, NULL);
    }
}

//
//	Return TRUE if FDF output format is requested.
//	(Otherwise, return FALSE.)
//
int
Fileformat::want_fdf()
{
    return strcasecmp(fileformat->label, "FDF") == 0;
}

//
//	Write the data displayed in the frame pointed to by "gptr" into
//	the file specified by "path".
//	Data is written in FITS format; integer/float, 8/16/32 bits as
//	specified by the user.  Then the user-specified script is run
//	corresponding to the user specified "Format Type", which (maybe)
//	converts the data into some other format.
//	The "gptr" frame must contain data!
//
void
Fileformat::write_data(Gframe *gptr, char *path)
{
    int bitpix;			// The FITS BITPIX value
    char buf[81];
    const int buflen = sizeof(buf) - 1;
    int i;
    Imginfo *img = gptr->imginfo;

    // Initialize various parameters
    bitpix = data_size();
    if (strcasecmp(data_type(), "float") == 0){
	bitpix *= -1;
    }
    int nx, ny, nz;
    double lenx, leny, lenz;
    img->get_spatial_dimensions(&nx, &ny, &nz);
    img->get_spatial_spans(&lenx, &leny, &lenz);
    if (nx == 0 || ny == 0){
	msgerr_print("A spatial dimension is zero.");
	return;
    }

    // Find min and max data values for header
    int datawords = nx * ny;
    float *fdata = (float *)img->GetData();
    float *edata = fdata + datawords;
    double datamin = *fdata++;
    double datamax = datamin;
    for ( ; fdata<edata; fdata++){
	if (*fdata < datamin){
	    datamin = *fdata;
	}else if (*fdata > datamax){
	    datamax = *fdata;
	}
    }

    //* Set the scaling factor
    double max_d = img->vsfunc->max_data;
    double min_d = img->vsfunc->min_data;
    double vscale;		// Set below for each case
    double voffset;

    // Convert to the desired data type.
    int databytes =  datawords * data_size() / 8;
    unsigned char *databuf = new unsigned char[databytes];
    char *pdata;
    switch (bitpix){
      case 8:
	vscale = 0xff / (max_d - min_d);
	voffset = - (0xff * min_d) / (max_d - min_d);
	datamin = datamin * vscale + voffset;
	datamax = datamax * vscale + voffset;
	if (datamin < 0) datamin = 0;
	if (datamax > 0xff) datamax = 0xff;
	img->convertdata((float *)img->GetData(), databuf,
			 datawords, vscale, voffset);
	pdata = (char *)databuf;
	break;
      case 16:
	vscale = 0x7fff / (max_d - min_d);
	voffset = - (0x7fff * min_d) / (max_d - min_d);
	datamin = datamin * vscale + voffset;
	datamax = datamax * vscale + voffset;
	if (datamin < -0x7fff) datamin = -0x7fff;
	if (datamax > 0x7fff) datamax = 0x7fff;
	img->convertdata((float *)img->GetData(), (short *)databuf,
			 datawords, vscale, voffset);
	pdata = (char *)databuf;
	break;
      case 32:
	vscale = 0x7fffffff / (max_d - min_d);
	voffset = - (0x7fffffff * min_d) / (max_d - min_d);
	datamin = datamin * vscale + voffset;
	datamax = datamax * vscale + voffset;
	if (datamin < -0x7fffffff) datamin = -0x7fffffff;
	if (datamax > 0x7fffffff) datamax = 0x7fffffff;
	img->convertdata((float *)img->GetData(), (int *)databuf,
			 datawords, vscale, voffset);
	pdata = (char *)databuf;
	break;
      case -32:
	pdata = (char *)img->GetData();
	break;
      default:
	msgerr_print("Illegal data type (%s) for size (%d)",
		 data_type(), data_size());
	delete[] databuf;
	return;
    }

    // Write data to tmp file in FITS format
    char *tmppath = new char [strlen(path) + 5];
    sprintf(tmppath,"%s.tmp", path);
    int fd = open(tmppath, O_RDWR | O_CREAT, 0664);

    // Write the header
    i=0;
    sprintf(buf,"%-8s=%21s%50s", "SIMPLE", "T", "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21d%50s", "BITPIX", bitpix, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21d%50s", "NAXIS", 2, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21d%50s", "NAXIS1", nx, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21d%50s", "NAXIS2", ny, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s= '%-8.8s'%60s", "CTYPE1", "CM", "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s= '%-8.8s'%60s", "CTYPE2", "CM", "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21G%50s", "CDELT1", lenx / nx, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21G%50s", "CDELT2", leny / ny, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21G%50s", "DATAMIN", datamin, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21G%50s", "DATAMAX", datamax, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s%-72s", "HISTORY", "Created by ImageBrowser");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-80s", "END");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%80s", "");
    for ( ; i<36; i++){
	write(fd, buf, buflen);
    }

    // Write the data
    write(fd, pdata, databytes);
    delete[] databuf;

    // Pad out the file to multiple of 2880 bytes
    int npad = 2880 - (databytes % 2880);
    if (npad != 2880){
	char *padbuf = new char[npad];
	memset(padbuf, 0, npad);
	write(fd, padbuf, npad);
	delete[] padbuf;
    }

    close(fd);

    system(get_script(tmppath, path));
    unlink(tmppath);
}
