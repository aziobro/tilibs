/* Hey EMACS -*- linux-c -*- */
/* $Id: link_nul.c 1059 2005-05-14 09:45:42Z roms $ */

/*  libCables - Ti Link Cable library, a part of the TiLP project
 *  Copyright (C) 1999-2005  Romain Lievin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
	TI86 support.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "ticonv.h"
#include "ticalcs.h"
#include "gettext.h"
#include "logging.h"
#include "error.h"
#include "pause.h"
#include "macros.h"

#include "dbus_pkt.h"
#include "cmd85.h"
#include "rom86.h"
//#include "keys83p.h"

// Screen coordinates of the TI86
#define TI86_ROWS  64
#define TI86_COLS  128

static char utf8[128];

static int		is_ready	(CalcHandle* handle)
{
	return 0;
}

static int		send_key	(CalcHandle* handle, uint16_t key)
{
	TRYF(ti85_send_KEY(key));
	TRYF(ti85_recv_ACK(&key));

	return 0;
}

static int		recv_screen	(CalcHandle* handle, CalcScreenCoord* sc, uint8_t** bitmap)
{
	uint16_t max_cnt;
	int err;

	sc->width = TI86_COLS;
	sc->height = TI86_ROWS;
	sc->clipped_width = TI86_COLS;
	sc->clipped_height = TI86_ROWS;

	*bitmap = (uint8_t *)malloc(TI86_COLS * TI86_ROWS * sizeof(uint8_t) / 8);
	if(*bitmap == NULL) return ERR_MALLOC;

	TRYF(ti85_send_SCR());
	TRYF(ti85_recv_ACK(NULL));

	err = ti85_recv_XDP(&max_cnt, *bitmap);	// pb with checksum
	if (err != ERR_CHECKSUM) { TRYF(err) };
	TRYF(ti85_send_ACK());

	return 0;
}

static int		get_dirlist	(CalcHandle* handle, TNode** vars, TNode** apps)
{
	TreeInfo *ti;
	TNode *folder;
	uint16_t unused;
	uint8_t hl, ll, lh;
	uint8_t mem[8];

	// get list of folders & FLASH apps
	(*vars) = t_node_new(NULL);
	ti = (TreeInfo *)malloc(sizeof(TreeInfo));
	ti->model = handle->model;
	ti->type = VAR_NODE_NAME;
	(*vars)->data = ti;

	(*apps) = t_node_new(NULL);
	ti = (TreeInfo *)malloc(sizeof(TreeInfo));
	ti->model = handle->model;
	ti->type = APP_NODE_NAME;
	(*apps)->data = ti;

	TRYF(ti85_send_REQ(0x0000, TI86_DIR, ""));
	TRYF(ti85_recv_ACK(&unused));

	TRYF(ti85_recv_XDP(&unused, mem));
	TRYF(ti85_send_ACK());

	hl = mem[0];
	ll = mem[1];
	lh = mem[2];
	ti->mem_free = (hl << 16) | (lh << 8) | ll;

	folder = t_node_new(NULL);
	t_node_append(*vars, folder);

	for (;;) 
	{
		VarEntry *ve = tifiles_ve_create();
		TNode *node;
		int err;

		err = ti85_recv_VAR((uint16_t *) & ve->size, &ve->type, ve->name);
		fixup(ve->size);
		TRYF(ti85_send_ACK());
		if (err == ERR_EOT)
			break;
		else if (err != 0)
			return err;

		node = t_node_new(ve);
		t_node_append(folder, node);

		ticonv_varname_to_utf8_s(handle->model,ve->name,utf8,ve->type);
		sprintf(update_->text, _("Reading of '%s'"), utf8);
		update_label();
	}

	return 0;
}

static int		get_memfree	(CalcHandle* handle, uint32_t* mem)
{
	return 0;
}

static int		send_backup	(CalcHandle* handle, BackupContent* content)
{
    int err = 0;
    uint16_t length;
    char varname[9] = { 0 };
    uint8_t rej_code;
    uint16_t status;

    length = content->data_length1;
    varname[0] = LSB(content->data_length2);
    varname[1] = MSB(content->data_length2);
    varname[2] = LSB(content->data_length3);
    varname[3] = MSB(content->data_length3);
    varname[4] = LSB(content->data_length4);
    varname[5] = MSB(content->data_length4);

    TRYF(ti85_send_VAR(content->data_length1, TI86_BKUP, varname));
    TRYF(ti85_recv_ACK(&status));

    sprintf(update_->text, _("Waiting user's action..."));
    update_label();
    do 
	{	// wait user's action
		if (update_->cancel)
			return ERR_ABORT;
		err = ti85_recv_SKP(&rej_code);
    }
    while (err == ERROR_READ_TIMEOUT);
    TRYF(ti85_send_ACK());

    switch (rej_code) 
	{
    case REJ_EXIT:
    case REJ_SKIP:
      return ERR_ABORT;
      break;
    case REJ_MEMORY:
      return ERR_OUT_OF_MEMORY;
      break;
    default:			// RTS
      break;
    }
    
	sprintf(update_->text, _("Sending..."));
    update_label();

	update_->max2 = 4;
	update_->cnt2 = 0;

    TRYF(ti85_send_XDP(content->data_length1, content->data_part1));
    TRYF(ti85_recv_ACK(&status));
    update_->cnt2++;
	update_->pbar();

    TRYF(ti85_send_XDP(content->data_length2, content->data_part2));
    TRYF(ti85_recv_ACK(&status));
    update_->cnt2++;
	update_->pbar();

    if (content->data_length3) 
	{
      TRYF(ti85_send_XDP(content->data_length3, content->data_part3));
      TRYF(ti85_recv_ACK(&status));
    }
    update_->cnt2++;
	update_->pbar();

    TRYF(ti85_send_XDP(content->data_length4, content->data_part4));
    TRYF(ti85_recv_ACK(&status));
    update_->cnt2++;
	update_->pbar();

	return 0;
}

static int		recv_backup	(CalcHandle* handle, BackupContent* content)
{
	char varname[9] = { 0 };

	sprintf(update_->text, _("Waiting for backup..."));
    update_label();

	content->model = CALC_TI86;
	strcpy(content->comment, tifiles_comment_set_backup());

    TRYF(ti85_recv_VAR(&(content->data_length1), &content->type, varname));
    content->data_length2 = (uint8_t)varname[0] | ((uint8_t)varname[1] << 8);
    content->data_length3 = (uint8_t)varname[2] | ((uint8_t)varname[3] << 8);
    content->data_length4 = (uint8_t)varname[4] | ((uint8_t)varname[5] << 8);
    TRYF(ti85_send_ACK());

    TRYF(ti85_send_CTS());
    TRYF(ti85_recv_ACK(NULL));

	update_->max2 = 4;
	update_->cnt2 = 0;

    content->data_part1 = tifiles_ve_alloc_data(65536);
    TRYF(ti85_recv_XDP(&content->data_length1, content->data_part1));
    TRYF(ti85_send_ACK());
    update_->cnt2++;
	update_->pbar();

    content->data_part2 = tifiles_ve_alloc_data(65536);
    TRYF(ti85_recv_XDP(&content->data_length2, content->data_part2));
    TRYF(ti85_send_ACK());
    update_->cnt2++;
	update_->pbar();

    if (content->data_length3) 
	{
      content->data_part3 = tifiles_ve_alloc_data(65536);
      TRYF(ti85_recv_XDP(&content->data_length3, content->data_part3));
      TRYF(ti85_send_ACK());
    } else
      content->data_part3 = NULL;
    update_->cnt2++;
	update_->pbar();

    content->data_part4 = tifiles_ve_alloc_data(65536);
    TRYF(ti85_recv_XDP(&content->data_length4, content->data_part4));
    TRYF(ti85_send_ACK());
    update_->cnt2++;
	update_->pbar();

	return 0;
}

static int		send_var	(CalcHandle* handle, CalcMode mode, FileContent* content)
{
	int i;
	uint8_t rej_code;
	uint16_t status;

	for (i = 0; i < content->num_entries; i++) 
	{
		VarEntry *entry = content->entries[i];
		
		if(entry->action == ACT_SKIP)
			continue;

		TRYF(ti85_send_RTS((uint16_t)entry->size, entry->type, entry->name));
		TRYF(ti85_recv_ACK(&status));

		TRYF(ti85_recv_SKP(&rej_code));
		TRYF(ti85_send_ACK());

		switch (rej_code) 
		{
		case REJ_EXIT:
		  return ERR_ABORT;
		  break;
		case REJ_SKIP:
		  continue;
		  break;
		case REJ_MEMORY:
		  return ERR_OUT_OF_MEMORY;
		  break;
		default:			// RTS
		  break;
		}
		ticonv_varname_to_utf8_s(handle->model, entry->name, utf8, entry->type);
		sprintf(update_->text, _("Sending '%s'"), utf8);
		update_label();

		TRYF(ti85_send_XDP(entry->size, entry->data));
		TRYF(ti85_recv_ACK(&status));

		TRYF(ti85_send_EOT());
		ticalcs_info("\n");
	}

	return 0;
}

static int		recv_var	(CalcHandle* handle, CalcMode mode, FileContent* content, VarRequest* vr)
{
	uint16_t unused;
	VarEntry *ve;

	content->model = CALC_TI86;
	strcpy(content->comment, tifiles_comment_set_single());
	content->num_entries = 1;
	content->entries = tifiles_ve_create_array(1);
	ve = content->entries[0] = tifiles_ve_create();
	memcpy(ve, vr, sizeof(VarEntry));

	ticonv_varname_to_utf8_s(handle->model, vr->name, utf8, vr->type);
	sprintf(update_->text, _("Receiving '%s'"), utf8);
	update_label();

	// silent request
	TRYF(ti85_send_REQ((uint16_t)vr->size, vr->type, vr->name));
	TRYF(ti85_recv_ACK(&unused));

	TRYF(ti85_recv_VAR((uint16_t *) & ve->size, &ve->type, ve->name));
	TRYF(ti85_send_ACK());
	fixup(ve->size);

	TRYF(ti85_send_CTS());
	TRYF(ti85_recv_ACK(NULL));

	ve->data = tifiles_ve_alloc_data(ve->size);
	TRYF(ti85_recv_XDP((uint16_t *) & ve->size, ve->data));
	TRYF(ti85_send_ACK());

	return 0;
}

static int		send_var_ns	(CalcHandle* handle, CalcMode mode, FileContent* content)
{
	return 0;
}

static int		recv_var_ns	(CalcHandle* handle, CalcMode mode, FileContent* content, VarEntry** ve)
{
	return 0;
}

static int		send_flash	(CalcHandle* handle, FlashContent* content)
{
	return 0;
}

static int		recv_flash	(CalcHandle* handle, FlashContent* content, VarRequest* vr)
{
	return 0;
}

static int		recv_idlist	(CalcHandle* handle, uint8_t* idlist)
{
	return 0;
}

extern int rom_dump(CalcHandle* h, FILE* f);
extern int rom_dump_ready(CalcHandle* h);

static int		dump_rom	(CalcHandle* handle, CalcDumpSize size, const char *filename)
{
	const char *prgname = "romdump.86p";
	FILE *f;
	int err;
	//uint16_t keys[] = { 
	//    0x76, 0x08, 0x08, 		/* Quit, Clear, Clear,	*/
	//	  0x28, 0x3A, 0x34,	0x11,	/* A, S, M, (,			*/
	//    0x39, 0x36, 0x34, 0x2B,   /* R, O, M, D	*/
	//    0x56, 0x4E, 0x51, 0x12,	/* u, m, p, )	*/
	//    0x06						/* Enter		*/
	//};               

	// Copies ROM dump program into a file
	f = fopen(prgname, "wb");
	if (f == NULL)
		return ERR_FILE_OPEN;
	fwrite(romDump86, sizeof(uint8_t), romDumpSize86, f);
	fclose(f);

	// Transfer program to calc
	handle->busy = 0;
	TRYF(ticalcs_calc_send_var2(handle, MODE_NORMAL, prgname));
	unlink(prgname);

	// Wait for user's action (execing program)
	sprintf(handle->updat->text, _("Waiting user's action..."));
	handle->updat->label();

	do
	{
		handle->updat->refresh();
		if (handle->updat->cancel)
			return ERR_ABORT;
		
		//send RDY request ???
		PAUSE(1000);
		err = rom_dump_ready(handle);
	}
	while (err == ERROR_READ_TIMEOUT);

	// Get dump
	f = fopen(filename, "wb");
	if (f == NULL)
		return ERR_OPEN_FILE;

	err = rom_dump(handle, f);
	if(err)
	{
		fclose(f);
		return err;
	}

	fclose(f);
	return 0;
}

static int		set_clock	(CalcHandle* handle, CalcClock* clock)
{
	return 0;
}

static int		get_clock	(CalcHandle* handle, CalcClock* clock)
{
	return 0;
}

static int		del_var		(CalcHandle* handle, VarRequest* vr)
{
	return 0;
}

static int		new_folder  (CalcHandle* handle, VarRequest* vr)
{
	return 0;
}

static int		get_version	(CalcHandle* handle, CalcInfos* infos)
{
	return 0;
}

static int		send_cert	(CalcHandle* handle, FlashContent* content)
{
	return 0;
}

static int		recv_cert	(CalcHandle* handle, FlashContent* content)
{
	return 0;
}

const CalcFncts calc_86 = 
{
	CALC_TI86,
	"TI86",
	N_("TI-86"),
	N_("TI-86"),
	OPS_SCREEN | OPS_DIRLIST | OPS_BACKUP | OPS_VARS | OPS_ROMDUMP |
	FTS_SILENT | FTS_MEMFREE,
	&is_ready,
	&send_key,
	&recv_screen,
	&get_dirlist,
	&get_memfree,
	&send_backup,
	&recv_backup,
	&send_var,
	&recv_var,
	&send_var_ns,
	&recv_var_ns,
	&send_flash,
	&recv_flash,
	&recv_idlist,
	&dump_rom,
	&set_clock,
	&get_clock,
	&del_var,
	&new_folder,
	&get_version,
	&send_cert,
	&recv_cert,
};
