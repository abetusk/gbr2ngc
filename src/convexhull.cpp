// signe, then use 'cross_sign' below.
//
static cInt cross( const IntPoint &O, const IntPoint &A, const IntPoint &B )
{
  return ((A.X - O.X) * (B.Y - O.Y)) - ((A.Y - O.Y) * (B.X - O.X));
}


/*
 * Date: 2014-02-06 
 *
 * cross_sign adapted from signOfDet2x2 function in
 * 'RobustDeterminant.cpp' that had the following 
 * copyright and license notice:
 *
 * "
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (c) 1995 Olivier Devillers <Olivier.Devillers@sophia.inria.fr>
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 * Copyright (C) 2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 *
 * "
 *
 *
 * taken from:
 * http://trac.osgeo.org/geos/browser/trunk/src/algorithm/RobustDeterminant.cpp
 *
 * javascript version available here:
 * http://bjornharrtell.github.io/jsts/doc/api/symbols/src/src_jsts_algorithm_RobustDeterminant.js.html
 *
 */


//int signOfDet2x2(cInt x1,cInt y1,cInt x2,cInt y2) {
	// returns -1 if the determinant is negative,
	// returns  1 if the determinant is positive,
	// retunrs  0 if the determinant is null.
static int cross_sign( const IntPoint &O, const IntPoint &A, const IntPoint &B )
{
  cInt x1, x2, y1, y2;

  x1 = A.X - O.X;
  y1 = A.Y - O.Y;
  x2 = B.X - O.X;
  y2 = B.Y - O.Y;


	int sign=1;
	cInt swap;
	cInt k;

	/*
	*  testing null entries
	*/
	if ((x1==0) || (y2==0)) {
		if ((y1==0) || (x2==0)) {
			return 0;
		} else if (y1>0) {
			if (x2>0) {
				return -sign;
			} else {
				return sign;
			}
		} else {
			if (x2>0) {
				return sign;
			} else {
				return -sign;
			}
		}
	}
	if ((y1==0) || (x2==0)) {
		if (y2>0) {
			if (x1>0) {
				return sign;
			} else {
				return -sign;
			}
		} else {
			if (x1>0) {
				return -sign;
			} else {
				return sign;
			}
		}
	}

	/*
	*  making y coordinates positive and permuting the entries
	*  so that y2 is the biggest one
	*/
	if (0<y1) {
		if (0<y2) {
			if (y1<=y2) {
				;
			} else {
				sign=-sign;
				swap=x1;
				x1=x2;
				x2=swap;
				swap=y1;
				y1=y2;
				y2=swap;
			}
		} else {
			if (y1<=-y2) {
				sign=-sign;
				x2=-x2;
				y2=-y2;
			} else {
				swap=x1;
				x1=-x2;
				x2=swap;
				swap=y1;
				y1=-y2;
				y2=swap;
			}
		}
	} else {
		if (0<y2) {
			if (-y1<=y2) {
				sign=-sign;
				x1=-x1;
				y1=-y1;
			} else {
				swap=-x1;
				x1=x2;
				x2=swap;
				swap=-y1;
				y1=y2;
				y2=swap;
			}
		} else {
			if (y1>=y2) {
				x1=-x1;
				y1=-y1;
				x2=-x2;
				y2=-y2;
			} else {
				sign=-sign;
				swap=-x1;
				x1=-x2;
				x2=swap;
				swap=-y1;
				y1=-y2;
				y2=swap;
			}
		}
	}

	/*
	*  making x coordinates positive
	*/
	/*
	*  if |x2|<|x1| one can conclude
	*/
	if (0<x1) {
		if (0<x2) {
			if (x1 <= x2) {
				;
			} else {
				return sign;
			}
		} else {
			return sign;
		}
	} else {
		if (0<x2) {
			return -sign;
		} else {
			if (x1 >= x2) {
				sign=-sign;
				x1=-x1;
				x2=-x2;
			} else {
				return -sign;
			}
		}
	}

	/*
	*  all entries strictly positive   x1 <= x2 and y1 <= y2
	*/
	while (true) {
		//k=std::floor(x2/x1);
    k = x2/x1;
		x2=x2-k*x1;
		y2=y2-k*y1;

		/*
		*  testing if R (new U2) is in U1 rectangle
		*/
		if (y2<0) {
			return -sign;
		}
		if (y2>y1) {
			return sign;
		}

		/*
		*  finding R'
		*/
		if (x1>x2+x2) {
			if (y1<y2+y2) {
				return sign;
			}
		} else {
			if (y1>y2+y2) {
				return -sign;
			} else {
				x2=x1-x2;
				y2=y1-y2;
				sign=-sign;
			}
		}
		if (y2==0) {
			if (x2==0) {
				return 0;
			} else {
				return -sign;
			}
		}
		if (x2==0) {
			return sign;
		}

		/*
		*  exchange 1 and 2 role.
		*/
		//k=std::floor(x1/x2);
    k = x1/x2;
		x1=x1-k*x2;
		y1=y1-k*y2;
		
		/*
		*  testing if R (new U1) is in U2 rectangle
		*/
		if (y1<0) {
			return sign;
		}
		if (y1>y2) {
			return -sign;
		}

		/*
		*  finding R'
		*/
		if (x2>x1+x1) {
			if (y2<y1+y1) {
				return -sign;
			}
		} else {
			if (y2>y1+y1) {
				return sign;
			} else {
				x1=x2-x1;
				y1=y2-y1;
				sign=-sign;
			}
		}
		if (y1==0) {
			if (x1==0) {
				return 0;
			} else {
				return sign;
			}
		}
		if (x1==0) {
			return -sign;
		}
	}
}


// Adapted from C++ implementation from:
// http://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain
//

void ConvexHull( const Path& pnt, Path& solution)
{
  Path P = pnt;
  Path s;
  int n = P.size(), k = 0;

  s.resize(2*n);
  std::sort(P.begin(), P.end());

  for (int i = 0; i < n; i++) {
    while (k >= 2 && cross_sign(s[k-2], s[k-1], P[i]) <= 0) k--;
    s[k++] = P[i];
  }
     
  for (int i = n-2, t = k+1; i >= 0; i--) {
    while (k >= t && cross_sign(s[k-2], s[k-1], P[i]) <= 0) k--;
    s[k++] = P[i];
  }

  s.resize(k);
  solution = s;

}

