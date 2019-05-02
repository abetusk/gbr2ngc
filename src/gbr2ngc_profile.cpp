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

#include "gbr2ngc.hpp"


void profile_start(void) {
  gettimeofday( &gProfileStart, NULL );
}

void profile_end(void) {
  gettimeofday( &gProfileEnd, NULL);
}

uint64_t profile_diff(void) {
  int64_t d_ms;
  uint64_t d;

  d = gProfileEnd.tv_sec - gProfileStart.tv_sec;
  d_ms = gProfileEnd.tv_usec - gProfileStart.tv_usec;
  d_ms *= 1000000;
  d += d_ms;
}
