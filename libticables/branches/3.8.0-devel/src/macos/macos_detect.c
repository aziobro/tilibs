/* Hey EMACS -*- macos-c -*- */
/* $Id$ */

/*  libticables - Ti Link Cable library, a part of the TiLP project
 *  Copyright (C) 1999-2004  Romain Lievin
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

/* macos probing module */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_STDINT_H
# include <stdint.h>
#else
# include <inttypes.h>
#endif

#include "../intl.h"
#include "../cabl_def.h"
#include "../cabl_err.h"
#include "../export.h"
#include "../externs.h"
#include "../printl.h"

int macos_detect_os(char **os_type)
{
	struct utsname buf;

	uname(&buf);
  	printl(0, _("Getting OS type...\r\n"));
  	printl(0, _("  System name: %s\r\n"), buf.sysname);
  	printl(0, _("  Node name: %s\r\n"), buf.nodename);
  	printl(0, _("  Release: %s\r\n"), buf.release);
  	printl(0, _("  Version: %s\r\n"), buf.version);
  	printl(0, _("  Machine: %s\r\n"), buf.machine);
	printl(0, _("Done.\r\n"));
	*os_type = OS_MACOS;

	return 0;
}

int macos_detect_port(TicablePortInfo * pi)
{
	// I will put them later...

	return 0;
}

int macos_detect_resources(void)
{
	printl(0, _("libticables: checking resources...\r\n"));
	resources = IO_OSX;
	
	/* API: for use with all */
	
	resources |= IO_API;
	printl(0, _("  IO_API: %sfound at compile time.\n"),
		resources & IO_API ? "" : "not ");	

	return 0;
}
