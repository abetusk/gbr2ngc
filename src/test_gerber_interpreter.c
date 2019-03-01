/*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* This program was written while working at Bright Works Computer Consulting
* and allowed to be GPL'd under express permission of the current president
* John Guttridge
* Dated May 20th 2013
*/

#ifdef GERBER_TEST

#include "gerber_interpreter.h"

#define N_LINEBUF 4099

//------------------------

int main(int argc, char **argv) {
  int i, j, k, n_line;
  char *chp, *ts;
  FILE *fp;

  int image_parameter_code;
  int function_code;

  gerber_state_t gs;

  gerber_state_init(&gs);

  if (argc!=2) { printf("provide filename\n"); exit(0); }

  k = gerber_state_load_file(&gs, argv[1]);
  if (k<0) {
    perror(argv[1]);
  }

  gerber_report_state(&gs);
}

#endif
