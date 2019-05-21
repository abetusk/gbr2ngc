#include "gbr2ngc.hpp"

// store heightmap in `heightmap` vector.
// `heightmap` should result in n points where `heightmap` should have 3*n entries,
//
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

//            _                   _ _
//   ___ __ _| |_ _ __ ___  _   _| | |      _ __ ___  _ __ ___
//  / __/ _` | __| '_ ` _ \| | | | | |_____| '__/ _ \| '_ ` _ \
// | (_| (_| | |_| | | | | | |_| | | |_____| | | (_) | | | | | |
//  \___\__,_|\__|_| |_| |_|\__,_|_|_|     |_|  \___/|_| |_| |_|
//


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

// Catmull-Rom on a 2D grid.
// First do x interpolation then do y.
// `s` and `t` are "x" and "y" parameters to the polynomial, respectively.
// A common use case is to vary `s` and `t` from [0,1], representing the parameter to be passed
//   in for the spline-polynomial.
//
double catmull_rom_2d(double s, double t, std::vector< std::vector< std::vector< double > > > &grid) {
  int ret;
  double v0[3], v1[3], v2[3], v3[3];
  double r[3];

  catmull_rom( v0, s, &(grid[0][0][0]), &(grid[1][0][0]), &(grid[2][0][0]), &(grid[3][0][0]));
  catmull_rom( v1, s, &(grid[0][1][0]), &(grid[1][1][0]), &(grid[2][1][0]), &(grid[3][1][0]));
  catmull_rom( v2, s, &(grid[0][2][0]), &(grid[1][2][0]), &(grid[2][2][0]), &(grid[3][2][0]));
  catmull_rom( v3, s, &(grid[0][3][0]), &(grid[1][3][0]), &(grid[2][3][0]), &(grid[3][3][0]));

  ret = catmull_rom(r, t, v0, v1, v2, v3);

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

int _iclamp(int v, int l, int u) {
  if (v<l) { return l; }
  if (v>u) { return u; }
  return v;
}

static int _heightmap_cmp(const void *a, const void *b) {
  double *_p, *_q;

  _p = (double *)((double *)a);
  _q = (double *)((double *)b);

  if      (_p[1] < _q[1]) { return -1; }
  else if (_p[1] > _q[1]) { return  1; }

  if      (_p[0] < _q[0]) { return -1; }
  else if (_p[0] > _q[0]) { return  1; }

  return 0;
}

static int _dcmp(const void *a, const void *b) {
  double _da, _db;

  _da = (double)(*((double *)a));
  _db = (double)(*((double *)b));

  if (_da < _db) { return -1; }
  if (_da > _db) { return  1; }
  return 0;
}

// `xyz1 is updated in place with the interpolated z-coordinate
// both `xyz` and `heightmap` have a packed x,y,z format.
// `heightmap` is altered to be sorted by x-ascending, y-ascending order.
//
int catmull_rom_grid(std::vector< double > &xyz, std::vector< double > &heightmap) {
  int i, j, k, x_idx, y_idx, _x_idx_actual, _y_idx_actual, xyz_idx;
  int _ix, _iy;
  double d, _n, x, y, del_x, del_y, s_x, s_y;
  double interpolated_z;
  std::vector< double > x_pnt_wdup, y_pnt_wdup;
  std::vector< double > x_pnt, y_pnt;
  double _eps = 1.0/1024.0;

  std::vector< std::vector< std::vector< double > > > subgrid;
  std::vector< std::vector< double > > __vvd;
  std::vector< double > __vd;

  qsort( &(heightmap[0]), heightmap.size()/3, sizeof(double)*3, _heightmap_cmp);

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

  if ((x_pnt.size() * y_pnt.size()) != (heightmap.size()/3)) { return -2; }

  // grid is uniform so assume first point represents the delta x and y
  // in each dimension and calculate it.
  //
  del_x = 1.0; del_y = 1.0;
  if (x_pnt.size()>1) { del_x = x_pnt[1] - x_pnt[0]; } else { return -3; }
  if (y_pnt.size()>1) { del_y = y_pnt[1] - y_pnt[0]; } else { return -4; }

  // allocate subgrid
  //
  for (i=0; i<4; i++) {
    subgrid.push_back( __vvd );
    for (j=0; j<4; j++) {
      subgrid[i].push_back( __vd );
      for (k=0; k<3; k++) {
        subgrid[i][j].push_back(0.0);
      }
    }
  }

  // interpolate for each point in `xyz`, placing resulting interpolated z
  // point in the `xyz` array.
  // This is inefficient in the sense that the subgrid is being repopulated
  // each iteration.
  // In theory, the subgrid could be re-used if the data in the `xyz` array
  // were properly sorted.
  // I suspect this will be 'good enough' for most purposes.
  // If speed becomes an issue, feel free to open an issue (or fix it yourself
  // and push changes to the main repo)
  //
  for (xyz_idx = 0; xyz_idx<xyz.size(); xyz_idx+=3) {
    
    x = xyz[xyz_idx];
    y = xyz[xyz_idx+1];

    x_idx = _get_list_index(x, x_pnt);
    y_idx = _get_list_index(y, y_pnt);

    for (_iy=0; _iy<4; _iy++) {
      for (_ix=0; _ix<4; _ix++) {

        _x_idx_actual = _iclamp( x_idx+_ix-1, 0, (int)(x_pnt.size()-1) );
        _y_idx_actual = _iclamp( y_idx+_iy-1, 0, (int)(y_pnt.size()-1) );

        subgrid[_ix][_iy][0] = x_pnt[ _x_idx_actual ];
        subgrid[_ix][_iy][1] = y_pnt[ _y_idx_actual ];
        subgrid[_ix][_iy][2] = heightmap[ 3*(_y_idx_actual * x_pnt.size()  + _x_idx_actual) + 2 ];

      }
    }

    s_x = (x - x_pnt[x_idx] ) / del_x;
    s_y = (y - y_pnt[y_idx] ) / del_y;
    xyz[xyz_idx+2] = catmull_rom_2d(s_x, s_y, subgrid);


  }

  // sort the resultin `xyz` array
  //
  qsort( &(xyz[0]), xyz.size()/3, sizeof(double)*3, _heightmap_cmp);

  return 0;
}
