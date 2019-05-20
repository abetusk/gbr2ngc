#include "gbr2ngc.hpp"

// store heightmap in `heightmap` vector.
// `heightmap` should result in n points where `heightmap` should have 3*n entries,
//
//int read_heightmap( FILE *fp, std::vector< double > &heightmap ) {
int read_heightmap( std::string &fn, std::vector< double > &heightmap ) {
  FILE *fp;
  int ch, _is_comment=0;
  std::string buf;
  double d=0.0;

  fp = fopen(fn.c_str(), "r");
  if (!fp) { return errno; }

  buf.clear();
  heightmap.clear();

  while ( !feof(fp) ) {
    ch = fgetc(fp);
    if (ch == EOF) { continue; }

    if (_is_comment) {
      if ((ch == '\n') || (ch == '\r') || (ch == '\f')) { _is_comment=0; }
      continue;
    }

    if ((ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r') || (ch == '\f') || (ch == '#')) {
      if (ch == '#') { _is_comment=1; }

      if (buf.size()>0) {
        d = strtod(buf.c_str(), NULL);
        if ((d==0.0) && (errno!=0)) { fclose(fp); return errno; }
        heightmap.push_back(d);
        buf.clear();
      }

      continue;
    }

    buf.push_back(ch);
  }

  if (buf.size()>0) {
    d = strtod(buf.c_str(), NULL);
    if ((d==0.0) && (errno!=0)) { fclose(fp); return errno; }
    heightmap.push_back(d);
  }

  fclose(fp);

  return 0;
}

// http://www.cs.cmu.edu/~462/projects/assn2/assn2/catmullRom.pdf
//
int catmull_rom( double *q, double s, double *p_m2, double *p_m1, double *p, double *p_1 ) {
  double tau = 0.5;
  double s2 = s*s;
  double s3 = s2*s;
  double a, b, c, d;

  q[0] = q[1] = q[2] = 0.0;

  a = -(tau*s) + (2.0*tau*s2) - (tau*s3);
  b = 1.0 + (s2*(tau-3.0)) + (s3*(2.0-tau));
  c = (tau*s) + (s2*(3.0-(2.0*tau))) + (s3*(tau-2.0));
  d = -(tau*s2) + (s3*tau);

  q[0] = (a*p_m2[0]) + (b*p_m1[0]) + (c*p[0]) + (d*p_1[0]);
  q[1] = (a*p_m2[1]) + (b*p_m1[1]) + (c*p[1]) + (d*p_1[1]);
  q[2] = (a*p_m2[2]) + (b*p_m1[2]) + (c*p[2]) + (d*p_1[2]);

  return 0;
}

//double catmull_rom_2d(double x, double y, double *grid) {
double catmull_rom_2d(double x, double y, std::vector< std::vector< std::vector< double > > > &grid) {
  int ret;
  double v0[3], v1[3], v2[3], v3[3];
  double r[3];

  catmull_rom( v0, x, &(grid[0][0][0]), &(grid[1][0][0]), &(grid[2][0][0]), &(grid[3][0][0]));
  catmull_rom( v1, x, &(grid[0][1][0]), &(grid[1][1][0]), &(grid[2][1][0]), &(grid[3][1][0]));
  catmull_rom( v2, x, &(grid[0][2][0]), &(grid[1][2][0]), &(grid[2][2][0]), &(grid[3][2][0]));
  catmull_rom( v3, x, &(grid[0][3][0]), &(grid[1][3][0]), &(grid[2][3][0]), &(grid[3][3][0]));

  /*
  catmull_rom( v0, x, g[0*4 + 0], g[1*4 + 0], g[2*4 + 0], g[3*4 + 0]);
  catmull_rom( v1, x, g[0*4 + 1], g[1*4 + 1], g[2*4 + 0], g[3*4 + 1]);
  catmull_rom( v2, x, g[0*4 + 2], g[1*4 + 2], g[2*4 + 0], g[3*4 + 2]);
  catmull_rom( v3, x, g[0*4 + 3], g[1*4 + 3], g[2*4 + 0], g[3*4 + 3]);
  */

  ret = catmull_rom(r, y, v0, v1, v2, v3);

  return r[2];
}

int _get_list_index(double p, std::vector< double > &v) {
  int _l, _r, _m;
  _l = 0;
  _r = (int)v.size()-1;
  _m = _r/2;
  while ((_r-_l) > 1) {
    if (v[_m] == p) { return _m; }
    if (v[_m] < p) { _l = _m; }
    else { _r = _m; }
    _m = _l + ((_r-_l)/2);
  }
  if (v[_r] == p) { return _r; }
  return _l;
}

double _clamp(double v, double l, double u) {
  if (v<l) { return l; }
  if (v>u) { return u; }
  return v;
}

static int _dcmp(const void *a, const void *b) {
  double _da, _db;

  _da = (double)(*((double *)a));
  _db = (double)(*((double *)b));

  if (_da < _db) { return -1; }
  if (_da > _db) { return  1; }
  return 0;
}

int catmull_rom_grid(std::vector< double > &heightmap) {
  int i, j, k;
  double d, _n;
  std::vector< double > x_pnt_wdup, y_pnt_wdup;
  std::vector< double > x_pnt, y_pnt;
  double _eps = 1.0/1024.0;

  std::vector< std::vector< std::vector< double > > > subgrid;
  std::vector< std::vector< double > > __vvd;
  std::vector< double > __vd;

  // grab all x and y positions
  //
  for (i=0; i<heightmap.size(); i+=3) {
    x_pnt_wdup.push_back(heightmap[i]);
    y_pnt_wdup.push_back(heightmap[i+1]);
  }

  // sort and de-duplicate them
  //
  qsort( &(x_pnt_wdup[0]), x_pnt_wdup.size(), sizeof(double), _dcmp);
  qsort( &(y_pnt_wdup[0]), y_pnt_wdup.size(), sizeof(double), _dcmp);

  _n = (double)x_pnt_wdup.size();
  if (_n < 1.0) { return -1; }
  d = fabs(x_pnt_wdup[ x_pnt_wdup.size()-1 ] - x_pnt_wdup[0]);
  if ((d > 0.0) && ((1.0/(_n*d)) < _eps)) { _eps = 1.0/(_n*d); }

  _n = (double)y_pnt_wdup.size();
  if (_n < 1.0) { return -1; }
  d = fabs(y_pnt_wdup[ y_pnt_wdup.size()-1 ] - y_pnt_wdup[0]);
  if ((d > 0.0) && ((1.0/(_n*d)) < _eps)) { _eps = 1.0/(_n*d); }

  d = x_pnt_wdup[0];
  x_pnt.push_back(d);
  for (i=0; i<x_pnt_wdup.size(); i++) {
    if ( fabs(d - x_pnt_wdup[i]) > _eps) {
      d = x_pnt_wdup[i];
      x_pnt.push_back(d);
    }
  }

  d = y_pnt_wdup[0];
  y_pnt.push_back(d);
  for (i=0; i<y_pnt_wdup.size(); i++) {
    if ( fabs(d - y_pnt_wdup[i]) > _eps) {
      d = y_pnt_wdup[i];
      y_pnt.push_back(d);
    }
  }

  printf("# x %i\n", (int)x_pnt.size());
  for (i=0; i<x_pnt.size(); i++) {
    printf("%f\n", (float)x_pnt[i]);
  }

  printf("\n\n");
  printf("# y %i\n", (int)y_pnt.size());
  for (i=0; i<y_pnt.size(); i++) {
    printf("%f\n", (float)y_pnt[i]);
  }

  exit(0);

  for (i=0; i<4; i++) {
    subgrid.push_back( __vvd );
    for (j=0; j<4; j++) {
      subgrid[i].push_back( __vd );
      for (k=0; k<4; k++) {
        subgrid[i][j].push_back(0.0);
      }
    }
  }


  for (i=0; i<4; i++) {
    for (j=0; j<4; j++) {
      //x_pos = clam
    }
  }

}
