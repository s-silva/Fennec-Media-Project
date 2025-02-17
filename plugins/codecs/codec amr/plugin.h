/**----------------------------------------------------------------------------

 Fennec Codec Plug-in 1.0 (Sound Files).
 Copyright (C) 2007 Chase <c-h@users.sf.net>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

-------------------------------------------------------------------------------

----------------------------------------------------------------------------**/

#include "..\..\..\include\system.h"
#include "..\..\..\include\fennec.h"
#include <stdio.h>
#include "amr/typedef.h"



struct plugin_stream
{
	unsigned char     initialized;
	FILE             *hstream;
	int              *dstate;
	short             synth[160];
	int               written;
	int               eos;
	int               fsize;
	int               olen;

	unsigned long     frequency;
	unsigned long     channels;
	unsigned long     bitspersample;
	unsigned long     bitrate;
	unsigned long     duration; /* duration in ms */
};
struct plugin_encoder_stream
{
	int               initialized;
	int               bmode;         /* base mode, fennec_plugintype... */
	int               smode;         /* sub mode, fenne_encoder_file etc. */

	letter            filepath[v_sys_maxpath];
	t_sys_file_handle fhandle;

	unsigned long     cfrequency;    /* current frequency */
	unsigned long     cbitspersample;
	unsigned long     cchannels;

	char*             outdata;

	double            speech[160];

#ifndef ETSI
	unsigned char     serial_data[32];
#else
	short             serial_data[250];
#endif

	enum Mode         req_mode;
	int              *enstate;
	int               dtx;

	int               c;

	int               firstwrite;
};

extern struct plugin_stream* pstreams;
extern struct plugin_encoder_stream* pestreams;
extern HINSTANCE hInst;

int           decoder_load(unsigned long id, const string sname);
int           decoder_close(unsigned long id);
int           decoder_seek(unsigned long id, double pos);
unsigned long decoder_read(unsigned long id, void* adata, unsigned long dsize);
int           tagread(const string fname,  struct fennec_audiotag *rtag);

int proc_settings_fileencoder(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

unsigned long callc fennec_initialize_input(struct general_input_data* gin);
int callc           fennec_plugin_initialize(void);
int callc           fennec_plugin_getformat(unsigned long id, unsigned long* ftype, void* fdata);
int callc           fennec_plugin_getposition(unsigned long id, unsigned long* ptype, void* pos);
int callc           fennec_plugin_setposition(unsigned long id, unsigned long ptype, void* pos);
int callc           fennec_plugin_getduration(unsigned long id, unsigned long* ptype, void* pos);
int callc           fennec_plugin_getstatus(unsigned long id, unsigned long* stype, void* stt);
int callc           fennec_plugin_read(unsigned long id, unsigned long dsize, unsigned long* dread, void* rdata);
int callc           fennec_plugin_about(void* pdata);
int callc           fennec_plugin_settings(void* pdata);
int callc           fennec_plugin_get_initialization_info(unsigned long inforreq, void* outinfo, unsigned long* outsize);
int callc           fennec_plugin_get_extension(int id, string out_ext, string out_desc, int* newdsc);
int callc           fennec_plugin_tagread (const string fname, struct fennec_audiotag* rtag);
int callc           fennec_plugin_close(unsigned long id);
int callc           fennec_plugin_uninitialize(void);
int callc           fennec_plugin_tagwrite (const string fname, struct fennec_audiotag* rtag);
unsigned long callc fennec_plugin_open(unsigned long otype, unsigned long osize, const string odata);

int callc           encoder_fennec_plugin_initialize(void);
int callc           encoder_fennec_plugin_uninitialize(void);
unsigned long callc encoder_file_create(string fname, unsigned long fcreatemode);
int callc           encoder_file_write(unsigned long id, void* pcmin, unsigned long sfreq, unsigned long sbps, unsigned long schannels, unsigned long dsize);
int callc           encoder_file_close(unsigned long id);
int callc           encoder_file_flush(unsigned long id);
int callc           encoder_file_seek(unsigned long id, double spos);
double callc        encoder_file_tell(unsigned long id, unsigned long tellmode);
unsigned long callc encode_initialize(void);
int callc           encode_encode(unsigned long id, void* pcmin, void* eout, unsigned long sfreq, unsigned long sbps, unsigned long schannels, unsigned long dsize, unsigned long* osize, encode_realloc reallocfunc);
int callc           encode_uninitialize(unsigned long id);
int callc           encoder_about(unsigned long id, void* odata);
int callc           settings_fileencoder(unsigned long id, void* odata);
int callc           settings_encoder(unsigned long id, void* odata);
int callc           encoder_get_initialization_info(unsigned long inforreq, void* outinfo, unsigned long* outsize);
unsigned long       encoder_local_locateindex(void);

int                 encoder_setdefaults(unsigned long id);
int                 encoder_appendextension(unsigned long id, string fpath);
int                 encoder_deleteextension(unsigned long id, string fpath);
int                 encoder_initialize(unsigned long id, int fcreatemode);
int                 encoder_uninitialize(unsigned long id);
int                 encoder_set(unsigned long id);
int                 encoder_write(unsigned long id, void* rbuffer, unsigned long bsize);

/*-----------------------------------------------------------------------------
 2007
-----------------------------------------------------------------------------*/
