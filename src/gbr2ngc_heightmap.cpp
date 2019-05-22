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

//----

class _iEdge {
  public:
    int64_t v[2];
};

int interpolate_height_delaunay(std::vector< double > &xyz, std::vector< double > &heightmap) {
  int i, j, k, idx, idx_x, idx_y;
  double x, y, dx, dy, d, interpolated_z, d_me, R;
  double mx, my, Mx, My;

  Delaunay<double> triangulation;
  std::vector< Triangle<double> > tris;
  //std::vector< Edge<double> > edges;
  std::vector< Vector2<double>> pnts;

  int grid_nx, grid_ny;
  double grid_dx, grid_dy, grid_ds;
  double min_x, min_y, max_x, max_y;
  std::vector< std::vector< int > > grid_idx;
  std::vector< int > _vi;

  double _eps = 0.0000001;

  if (heightmap.size() < 3) { return -1; }

  min_x = heightmap[0]; min_y = heightmap[1];
  max_x = min_x; max_y = min_y;

  for (i=0; i<heightmap.size(); i+=3) {
    if (min_x > heightmap[i]) { min_x = heightmap[i]; }
    if (max_x < heightmap[i]) { max_x = heightmap[i]; }

    if (min_y > heightmap[i+1]) { min_y = heightmap[i+1]; }
    if (max_y < heightmap[i+1]) { max_y = heightmap[i+1]; }

    pnts.push_back( Vector2<double>(heightmap[i], heightmap[i+1], heightmap[i+2]) );
  }

  grid_ds = sqrt((double)(heightmap.size()/3));
  grid_dx = (max_x - min_x) / grid_ds;
  grid_dy = (max_y - min_y) / grid_ds;

  if (grid_dx <= _eps) { grid_dx = 1.0; }
  if (grid_dy <= _eps) { grid_dy = 1.0; }

  grid_nx = (int)( ((max_x - min_x)/grid_dx) + 0.5 );
  grid_ny = (int)( ((max_y - min_y)/grid_dy) + 0.5 );
  grid_nx ++;
  grid_ny ++;

  //printf("grid_ds %f, grid_dx %f, grid_dy %f, grid_nx %i, grid_ny %i\n", (float)grid_ds, (float)grid_dx, (float)grid_dy, grid_nx, grid_ny);

  tris  = triangulation.triangulate(pnts);

  for (i=0; i<grid_nx*grid_ny; i++) {
    grid_idx.push_back( _vi );
  }

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

    for (x = mx; x <= (Mx + grid_dx); x += grid_dx) {
      for (y = my; y <= (My + grid_dy); y += grid_dy) {

        idx_x = (x - min_x) / grid_dx;
        idx_y = (y - min_y) / grid_dy;

        idx_x = _iclamp(idx_x, 0, grid_nx-1);
        idx_y = _iclamp(idx_y, 0, grid_ny-1);

        idx = (idx_y * grid_nx) + idx_x;

        printf("idx_x %i, idx_y %i, idx %i m(%f,%f) M(%f,%f) mxy(%f,%f), Mxy(%f,%f)\n",
            idx_x, idx_y, idx,
            min_x, min_y,
            max_x, max_y,
            mx, my,
            Mx, My
            );
        fflush(stdout);

        grid_idx[idx].push_back(i);

      }
    }

  }

  for (i=0; i<xyz.size(); i+=3) {

    x = xyz[i];
    y = xyz[i+1];

    idx_x = (x - min_x) / grid_dx;
    idx_y = (y - min_y) / grid_dy;

    idx_x = _iclamp(idx_x, 0, grid_nx-1);
    idx_y = _iclamp(idx_y, 0, grid_ny-1);

    idx = (idx_y*grid_nx) + idx_x;

    for (j=0; j<grid_idx[idx].size(); j++) {
    }

  }


  for (i=0; i<grid_nx; i++) {
    for (j=0; j<grid_ny; j++) {
      printf(" (");
      for (k=0; k<grid_idx[j*grid_nx+i].size(); k++) {
        printf(" %i", grid_idx[j*grid_nx + i][k]);
      }
      printf(")");
    }
    printf("\n");
  }

  return 0;
}


