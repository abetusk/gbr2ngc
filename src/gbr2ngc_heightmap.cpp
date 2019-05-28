#include "gbr2ngc.hpp"

#include "delaunay.h"

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

int HeightMap::setup_catmull_rom(std::vector<double> &_heightmap) {
  int i, j, k, x_idx, y_idx, _x_idx_actual, _y_idx_actual, xyz_idx;
  int _ix, _iy;
  double d, _n, x, y, s_x, s_y;
  double interpolated_z;
  std::vector< double > x_pnt_wdup, y_pnt_wdup;
  double _eps = 1.0/1024.0;

  std::vector< std::vector< double > > __vvd;
  std::vector< double > __vd;

  m_heightmap = _heightmap;
  qsort( &(m_heightmap[0]), m_heightmap.size()/3, sizeof(double)*3, _heightmap_cmp);

  // grab all x and y positions
  //
  for (i=0; i<m_heightmap.size(); i+=3) {
    x_pnt_wdup.push_back(m_heightmap[i]);
    y_pnt_wdup.push_back(m_heightmap[i+1]);
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
  m_x_pnt.push_back(d);
  for (i=0; i<x_pnt_wdup.size(); i++) {
    if ( fabs(d - x_pnt_wdup[i]) > _eps) {
      d = x_pnt_wdup[i];
      m_x_pnt.push_back(d);
    }
  }

  d = y_pnt_wdup[0];
  m_y_pnt.push_back(d);
  for (i=0; i<y_pnt_wdup.size(); i++) {
    if ( fabs(d - y_pnt_wdup[i]) > _eps) {
      d = y_pnt_wdup[i];
      m_y_pnt.push_back(d);
    }
  }

  if ((m_x_pnt.size() * m_y_pnt.size()) != (m_heightmap.size()/3)) { return -2; }

  // grid is uniform so assume first point represents the delta x and y
  // in each dimension and calculate it.
  //
  m_del_x = 1.0; m_del_y = 1.0;
  if (m_x_pnt.size()>1) { m_del_x = m_x_pnt[1] - m_x_pnt[0]; } else { return -3; }
  if (m_y_pnt.size()>1) { m_del_y = m_y_pnt[1] - m_y_pnt[0]; } else { return -4; }

  // allocate subgrid
  //
  for (i=0; i<4; i++) {
    m_subgrid.push_back( __vvd );
    for (j=0; j<4; j++) {
      m_subgrid[i].push_back( __vd );
      for (k=0; k<3; k++) {
        m_subgrid[i][j].push_back(0.0);
      }
    }
  }



  return 0;
}

int HeightMap::zOffset_catmull_rom(double &z, double x, double y) {
  int x_idx, y_idx, _iy, _ix, _x_idx_actual, _y_idx_actual;
  double s_x, s_y;
  //std::vector< std::vector< std::vector< double > > > subgrid;

  x_idx = _get_list_index(x, m_x_pnt);
  y_idx = _get_list_index(y, m_y_pnt);

  for (_iy=0; _iy<4; _iy++) {
    for (_ix=0; _ix<4; _ix++) {
      _x_idx_actual = _iclamp( x_idx+_ix-1, 0, (int)(m_x_pnt.size()-1) );
      _y_idx_actual = _iclamp( y_idx+_iy-1, 0, (int)(m_y_pnt.size()-1) );

      m_subgrid[_ix][_iy][0] = m_x_pnt[ _x_idx_actual ];
      m_subgrid[_ix][_iy][1] = m_y_pnt[ _y_idx_actual ];
      m_subgrid[_ix][_iy][2] = m_heightmap[ 3*(_y_idx_actual * m_x_pnt.size()  + _x_idx_actual) + 2 ];
    }
  }

  s_x = (x - m_x_pnt[x_idx] ) / m_del_x;
  s_y = (y - m_y_pnt[y_idx] ) / m_del_y;

  z = catmull_rom_2d(s_x, s_y, m_subgrid);

  return 0;
}


// `xyz1 is updated in place with the interpolated z-coordinate
// both `xyz` and `heightmap` have a packed x,y,z format.
// `heightmap` is altered to be sorted by x-ascending, y-ascending order.
//
int interpolate_height_catmull_rom_grid(std::vector< double > &xyz, std::vector< double > &heightmap) {
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

//----

//  _     _
// (_) __| |_      __
// | |/ _` \ \ /\ / /
// | | (_| |\ V  V /
// |_|\__,_| \_/\_/
//

// inverse distance weighting (idw)
// https://en.wikipedia.org/wiki/Inverse_distance_weighting
//
int interpolate_height_idw(std::vector< double > &xyz, std::vector< double > &heightmap, double exponent, double _eps) {
  int i, j;
  double x, y, dx, dy, d, interpolated_z, d_me, R;
  std::vector<double> dist_p;

  double W = 1.0;

  // sort the resultin `xyz` array
  //
  qsort( &(heightmap[0]), heightmap.size()/3, sizeof(double)*3, _heightmap_cmp);

  for (i=0; i<xyz.size(); i+=3) {

    x = xyz[i];
    y = xyz[i+1];

    R = 0.0;
    dist_p.clear();
    for (j=0; j<heightmap.size(); j+=3) {
      dx = heightmap[j]   - x;
      dy = heightmap[j+1] - y;
      d = sqrt(dx*dx + dy*dy);

      if (d <= _eps) { xyz[i+2] = heightmap[j+2]; break; }

      d_me = W / pow(d, exponent);
      R += d_me;
      dist_p.push_back(d_me);
    }
    if (j!=heightmap.size()) { continue; }

    interpolated_z = 0.0;
    for (j=0; j<dist_p.size(); j++) {
      interpolated_z += dist_p[j] * heightmap[3*j+2] / R;
    }

    xyz[i+2] = interpolated_z;

  }

  // sort the resultin `xyz` array
  //
  qsort( &(xyz[0]), xyz.size()/3, sizeof(double)*3, _heightmap_cmp);

  return 0;
}

// Inverse distance weighting is inefficient now, looping through the whole
// heightmap to get the weighted height.
// Possible speedups include:
// * specifcying an epsilon for distance, discarding points that are past that
//   epsilon and storing the relevant points in a grid
// * I think there are other ways to speed this up (from my memory, some methods
//   to speed this up are used in astronomy?) but I'm having trouble remembering
//   them.
//
int HeightMap::setup_idw(std::vector< double > &_heightmap, double exponent) {
  m_heightmap = _heightmap;
  m_exponent = exponent;

  qsort( &(m_heightmap[0]), m_heightmap.size()/3, sizeof(double)*3, _heightmap_cmp);

  return 0;
}

int HeightMap::zOffset_idw(double &z, double x, double y, double _eps) {
  int i, j;
  double dx, dy, d, interpolated_z, d_me, R;
  std::vector<double> dist_p;

  double W = 1.0;

  R = 0.0;
  dist_p.clear();
  for (j=0; j<m_heightmap.size(); j+=3) {
    dx = m_heightmap[j]   - x;
    dy = m_heightmap[j+1] - y;
    d = sqrt(dx*dx + dy*dy);

    if (d <= _eps) {
      z = m_heightmap[j+2];
      return 0;
    }

    d_me = W / pow(d, m_exponent);
    R += d_me;
    dist_p.push_back(d_me);
  }

  interpolated_z = 0.0;
  for (j=0; j<dist_p.size(); j++) {
    //interpolated_z += dist_p[j] * m_heightmap[3*j+2] / R;
    interpolated_z += dist_p[j] * m_heightmap[3*j+2];
  }
  interpolated_z /= R;

  z = interpolated_z;

  return 0;
}

//----

//      _      _
//   __| | ___| | __ _ _   _ _ __   __ _ _   _
//  / _` |/ _ \ |/ _` | | | | '_ \ / _` | | | |
// | (_| |  __/ | (_| | |_| | | | | (_| | |_| |
//  \__,_|\___|_|\__,_|\__,_|_| |_|\__,_|\__, |
//                                       |___/

static int _lerp_z_tri( Vector2<double> &pnt, Vector2<double> &a, Vector2<double> &b, Vector2<double> &c, double _eps = 0.0000001) {
  Vector2<double> p, q;
  double s, t, det;

  p.x = b.x - a.x; p.y = b.y - a.y; p.z = b.z - a.z;
  q.x = c.x - a.x; q.y = c.y - a.y; q.z = c.z - a.z;

  det = q.y*p.x - q.x*p.y;
  if (fabs(det) <= _eps) {
    pnt.x = a.x; pnt.y = a.y; pnt.z = a.z;
    return -1;
  }

  t =  (p.x*(pnt.y - a.y) - p.y*(pnt.x - a.x))/det;
  s = -(q.x*(pnt.y - a.y) - q.y*(pnt.x - a.x))/det;

  pnt.z = p.z*s + q.z*t + a.z;

  if ((s<0.0) || (t<0.0) || (s>1.0) || (t>1.0)) { return 0; }
  if ((s+t) > 1.0) { return 0; }
  return 1;
}

static int _lerp_z_tri_v( double *pnt, double *tri_v, double _eps = 0.0000001 ) {
  double p[3], q[3];
  double s, t, det;

  double *a, *b, *c;

  a = tri_v;
  b = tri_v+3;
  c = tri_v+6;

  p[0] = b[0] - a[0];
  p[1] = b[1] - a[1];
  p[2] = b[2] - a[2];

  q[0] = c[0] - a[0];
  q[1] = c[1] - a[1];
  q[2] = c[2] - a[2];

  det = q[1]*p[0] - q[0]*p[1];
  if (fabs(det) <= _eps) {
    pnt[0] = a[0];
    pnt[1] = a[1];
    pnt[2] = a[2];
    return -1;
  }

  t =  (p[0]*(pnt[1] - a[1]) - p[1]*(pnt[0] - a[0]))/det;
  s = -(q[0]*(pnt[1] - a[1]) - q[1]*(pnt[0] - a[0]))/det;

  pnt[2] = p[2]*s + q[2]*t + a[2];

  if ((s<0.0) || (t<0.0) || (s>1.0) || (t>1.0)) { return 0; }
  if ((s+t) > 1.0) { return 0; }
  return 1;
}


int HeightMap::setup_delaunay(std::vector< double > &_heightmap) {
  int i, j, k, idx, idx_x, idx_y;
  double x, y, dx, dy, d, interpolated_z, d_me, R;
  double mx, my, Mx, My;

  Delaunay<double> triangulation;
  std::vector< Triangle<double> > tris;
  //std::vector< Edge<double> > edges;
  std::vector< Vector2<double>> pnts;

  Vector2<double> _pnt;

  std::vector<double> tri3;

  //int grid_nx, grid_ny;
  //double grid_dx, grid_dy, grid_ds;
  //double min_x, min_y, max_x, max_y;
  //std::vector< std::vector< int > > grid_idx;
  std::vector< int > _vi;

  int tri_idx;
  double _eps = 0.0000001;

  //double min_z, max_z;

  m_heightmap = _heightmap;
  m_grid_idx.clear();
  tris.clear();

  if (m_heightmap.size() < 3) { return -1; }

  // find bounding box
  //

  m_min_x = m_heightmap[0]; m_min_y = m_heightmap[1];
  m_max_x = m_min_x; m_max_y = m_min_y;

  m_min_z = m_heightmap[2];
  m_max_z = m_min_z;

  for (i=0; i<m_heightmap.size(); i+=3) {
    if (m_min_x > m_heightmap[i]) { m_min_x = m_heightmap[i]; }
    if (m_max_x < m_heightmap[i]) { m_max_x = m_heightmap[i]; }

    if (m_min_y > m_heightmap[i+1]) { m_min_y = m_heightmap[i+1]; }
    if (m_max_y < m_heightmap[i+1]) { m_max_y = m_heightmap[i+1]; }

    if (m_min_z > m_heightmap[i+2]) { m_min_z = m_heightmap[i+2]; }
    if (m_max_z < m_heightmap[i+2]) { m_max_z = m_heightmap[i+2]; }

    pnts.push_back( Vector2<double>(m_heightmap[i], m_heightmap[i+1], m_heightmap[i+2]) );
  }

  // extend the min/max to accomodate any points not in the heightmap
  // but are needed by the input xyz points
  //
  dx = m_max_x - m_min_x;
  dy = m_max_y - m_min_y;

  m_min_x -= dx/2;
  m_max_x += dx/2;
  m_min_y -= dy/2;
  m_max_y += dy/2;

  pnts.push_back( Vector2<double>( m_min_x, m_min_y, m_max_z ) );
  pnts.push_back( Vector2<double>( m_min_x, m_max_y, m_max_z ) );
  pnts.push_back( Vector2<double>( m_max_x, m_max_y, m_max_z ) );
  pnts.push_back( Vector2<double>( m_max_x, m_min_y, m_max_z ) );

  m_grid_ds = sqrt((double)(m_heightmap.size()/3));
  m_grid_dx = (m_max_x - m_min_x) / m_grid_ds;
  m_grid_dy = (m_max_y - m_min_y) / m_grid_ds;

  if (m_grid_dx <= _eps) { m_grid_dx = 1.0; }
  if (m_grid_dy <= _eps) { m_grid_dy = 1.0; }

  m_grid_nx = (int)( ((m_max_x - m_min_x)/m_grid_dx) + 0.5 );
  m_grid_ny = (int)( ((m_max_y - m_min_y)/m_grid_dy) + 0.5 );
  m_grid_nx ++;
  m_grid_ny ++;

  tris  = triangulation.triangulate(pnts);

  tri3.resize(9);
  for (i=0; i<tris.size(); i++) {

    tri3[0] = tris[i].p1.x; tri3[1] = tris[i].p1.y; tri3[2] = tris[i].p1.z;
    tri3[3] = tris[i].p2.x; tri3[4] = tris[i].p2.y; tri3[5] = tris[i].p2.z;
    tri3[6] = tris[i].p3.x; tri3[7] = tris[i].p3.y; tri3[8] = tris[i].p3.z;

    m_tris.push_back(tri3);

  }

  // allocate grid
  //
  for (i=0; i<m_grid_nx*m_grid_ny; i++) {
    m_grid_idx.push_back( _vi );
  }

  // populate decimated grid for faster lookup
  //
  for (i=0; i<tris.size(); i++) {

    mx = tris[i].p1.x;
    my = tris[i].p1.y;

    Mx = mx;
    My = my;

    if (mx > tris[i].p2.x) { mx = tris[i].p2.x; }
    if (Mx < tris[i].p2.x) { Mx = tris[i].p2.x; }
    if (my > tris[i].p2.y) { my = tris[i].p2.y; }
    if (My < tris[i].p2.y) { My = tris[i].p2.y; }

    if (mx > tris[i].p3.x) { mx = tris[i].p3.x; }
    if (Mx < tris[i].p3.x) { Mx = tris[i].p3.x; }
    if (my > tris[i].p3.y) { my = tris[i].p3.y; }
    if (My < tris[i].p3.y) { My = tris[i].p3.y; }

    for (x = (mx-m_grid_dx); x <= (Mx + m_grid_dx); x += m_grid_dx) {
      for (y = (my-m_grid_dy); y <= (My + m_grid_dy); y += m_grid_dy) {

        idx_x = (x - m_min_x) / m_grid_dx;
        idx_y = (y - m_min_y) / m_grid_dy;

        idx_x = _iclamp(idx_x, 0, m_grid_nx-1);
        idx_y = _iclamp(idx_y, 0, m_grid_ny-1);

        idx = (idx_y * m_grid_nx) + idx_x;

        m_grid_idx[idx].push_back(i);

      }
    }

  }


  return 0;
}

int HeightMap::zOffset_delaunay(double &z, double x, double y) {
  int i, j, k, idx;
  int idx_x, idx_y, tri_idx;

  double _p3[3];

  idx_x = (x - m_min_x) / m_grid_dx;
  idx_y = (y - m_min_y) / m_grid_dy;

  idx_x = _iclamp(idx_x, 0, m_grid_nx-1);
  idx_y = _iclamp(idx_y, 0, m_grid_ny-1);

  idx = (idx_y*m_grid_nx) + idx_x;

  _p3[0] = x;
  _p3[1] = y;
  _p3[2] = -1.0;

  for (j=0; j<m_grid_idx[idx].size(); j++) {
    tri_idx = m_grid_idx[idx][j];

    if ((k=_lerp_z_tri_v(_p3, &(m_tris[tri_idx][0]))) > 0) {
      z = _p3[2];
      break;
    }

  }

  if (j==m_grid_idx[idx].size()) { return -1; }
  return 0;
}

int HeightMap::zOffset( double &z, double x, double y ) {

  if      (m_algorithm == HEIGHTMAP_DELAUNAY) { return zOffset_delaunay(z, x, y); }
  else if (m_algorithm == HEIGHTMAP_CATMULL_ROM) { return zOffset_catmull_rom(z, x, y); }
  else if (m_algorithm == HEIGHTMAP_INVERSE_DISTANCE_WEIGHT) { return zOffset_idw(z, x, y); }

  return -1;
}


int interpolate_height_delaunay(std::vector< double > &xyz, std::vector< double > &heightmap) {
  int i, j, k, idx, idx_x, idx_y;
  double x, y, dx, dy, d, interpolated_z, d_me, R;
  double mx, my, Mx, My;

  Delaunay<double> triangulation;
  std::vector< Triangle<double> > tris;
  //std::vector< Edge<double> > edges;
  std::vector< Vector2<double>> pnts;

  Vector2<double> _pnt;

  int grid_nx, grid_ny;
  double grid_dx, grid_dy, grid_ds;
  double min_x, min_y, max_x, max_y;
  std::vector< std::vector< int > > grid_idx;
  std::vector< int > _vi;

  int tri_idx;
  double _eps = 0.0000001;

  double min_z, max_z;


  if (heightmap.size() < 3) { return -1; }

  // find bounding box
  //

  min_x = heightmap[0]; min_y = heightmap[1];
  max_x = min_x; max_y = min_y;

  min_z = heightmap[2];
  max_z = min_z;

  for (i=0; i<heightmap.size(); i+=3) {
    if (min_x > heightmap[i]) { min_x = heightmap[i]; }
    if (max_x < heightmap[i]) { max_x = heightmap[i]; }

    if (min_y > heightmap[i+1]) { min_y = heightmap[i+1]; }
    if (max_y < heightmap[i+1]) { max_y = heightmap[i+1]; }

    if (min_z > heightmap[i+2]) { min_z = heightmap[i+2]; }
    if (max_z < heightmap[i+2]) { max_z = heightmap[i+2]; }

    pnts.push_back( Vector2<double>(heightmap[i], heightmap[i+1], heightmap[i+2]) );
  }

  for (i=0; i<xyz.size(); i+=3) {
    if (min_x > xyz[i]) { min_x = xyz[i]; }
    if (max_x < xyz[i]) { max_x = xyz[i]; }

    if (min_y > xyz[i+1]) { min_y = xyz[i+1]; }
    if (max_y < xyz[i+1]) { max_y = xyz[i+1]; }
  }

  // extend the min/max to accomodate any points not in the heightmap
  // but are needed by the input xyz points
  //
  dx = max_x - min_x;
  dy = max_y - min_y;

  min_x -= dx/2;
  max_x += dx/2;
  min_y -= dy/2;
  max_y += dy/2;

  pnts.push_back( Vector2<double>( min_x, min_y, max_z ) );
  pnts.push_back( Vector2<double>( min_x, max_y, max_z ) );
  pnts.push_back( Vector2<double>( max_x, max_y, max_z ) );
  pnts.push_back( Vector2<double>( max_x, min_y, max_z ) );

  grid_ds = sqrt((double)(heightmap.size()/3));
  grid_dx = (max_x - min_x) / grid_ds;
  grid_dy = (max_y - min_y) / grid_ds;

  if (grid_dx <= _eps) { grid_dx = 1.0; }
  if (grid_dy <= _eps) { grid_dy = 1.0; }

  grid_nx = (int)( ((max_x - min_x)/grid_dx) + 0.5 );
  grid_ny = (int)( ((max_y - min_y)/grid_dy) + 0.5 );
  grid_nx ++;
  grid_ny ++;

  tris  = triangulation.triangulate(pnts);

  // allocate grid
  //
  for (i=0; i<grid_nx*grid_ny; i++) {
    grid_idx.push_back( _vi );
  }

  // populate decimated grid for faster lookup
  //
  for (i=0; i<tris.size(); i++) {

    mx = tris[i].p1.x;
    my = tris[i].p1.y;

    Mx = mx;
    My = my;

    if (mx > tris[i].p2.x) { mx = tris[i].p2.x; }
    if (Mx < tris[i].p2.x) { Mx = tris[i].p2.x; }
    if (my > tris[i].p2.y) { my = tris[i].p2.y; }
    if (My < tris[i].p2.y) { My = tris[i].p2.y; }

    if (mx > tris[i].p3.x) { mx = tris[i].p3.x; }
    if (Mx < tris[i].p3.x) { Mx = tris[i].p3.x; }
    if (my > tris[i].p3.y) { my = tris[i].p3.y; }
    if (My < tris[i].p3.y) { My = tris[i].p3.y; }

    for (x = (mx-grid_dx); x <= (Mx + grid_dx); x += grid_dx) {
      for (y = (my-grid_dy); y <= (My + grid_dy); y += grid_dy) {

        idx_x = (x - min_x) / grid_dx;
        idx_y = (y - min_y) / grid_dy;

        idx_x = _iclamp(idx_x, 0, grid_nx-1);
        idx_y = _iclamp(idx_y, 0, grid_ny-1);

        idx = (idx_y * grid_nx) + idx_x;

        grid_idx[idx].push_back(i);

      }
    }

  }


  // finally, do the interpolation
  //
  for (i=0; i<xyz.size(); i+=3) {

    x = xyz[i];
    y = xyz[i+1];

    idx_x = (x - min_x) / grid_dx;
    idx_y = (y - min_y) / grid_dy;

    idx_x = _iclamp(idx_x, 0, grid_nx-1);
    idx_y = _iclamp(idx_y, 0, grid_ny-1);

    idx = (idx_y*grid_nx) + idx_x;

    for (j=0; j<grid_idx[idx].size(); j++) {
      tri_idx = grid_idx[idx][j];

      _pnt.x = xyz[i];
      _pnt.y = xyz[i+1];
      _pnt.z = -1.0;

      if ((k=_lerp_z_tri(_pnt, tris[tri_idx].p1, tris[tri_idx].p2, tris[tri_idx].p3)) > 0) {
        xyz[i+2] = _pnt.z;
        break;
      }

    }

    // we haven't found the triangle in the grid, an error,
    // so return error
    //
    if (j==grid_idx[idx].size()) { return -1; }

  }

  return 0;
}
