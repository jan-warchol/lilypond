/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1997--2014 Han-Wen Nienhuys <hanwen@xs4all.nl>

  LilyPond is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LilyPond is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "file-path.hh"
#include "main.hh"

/*
 * Global options that can be overridden through command line.
 */

/* Names of header fields to be dumped to a separate file. */
vector<string> dump_header_fieldnames_global;

/* Name of initialisation file. */
string init_name_global;

/* Output formats to generate.  */
string output_format_global = "";

/* Current output name. */
string output_name_global;

/* Run in safe mode? */
bool be_safe_global = false;

/* Scheme code to execute before parsing, after .scm init.
   This is where -e arguments are appended to.  */
string init_scheme_code_global;
string init_scheme_variables_global;

bool relocate_binary = true;

/*
 * Miscellaneous global stuff.
 */
File_path global_path;

/* Where the init files live.  Typically:
   LILYPOND_DATADIR = /usr/share/lilypond
*/
string lilypond_datadir;

vector<string> start_environment_global;
