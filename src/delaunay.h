/*
 *
 * Copyright (c) 2015 Simon Zeni (simonzeni@gmail.com)
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef H_DELAUNAY
#define H_DELAUNAY

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <vector>


/**
 * @brief use of machine epsilon to compare floating-point values for equality
 * http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
 */
template<class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
    almost_equal(T x, T y, int ulp=2)
{
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    return std::abs(x-y) <= std::numeric_limits<T>::epsilon() * std::abs(x+y) * ulp
    // unless the result is subnormal
        || std::abs(x-y) < std::numeric_limits<T>::min();
}

template<class T>
T half(T x){}

template <>
float half(float x){return 0.5f * x;}

template <>
double half(double x){return 0.5 * x;}

//--

template <typename T>
class Vector2
{
  public:
    //
    // Constructors
    //

    Vector2():x(0), y(0), z(0){}

    Vector2(T _x, T _y): x(_x), y(_y), z(0){}
    Vector2(T _x, T _y, T _z): x(_x), y(_y), z(_z) {}

    Vector2(const Vector2 &v): x(v.x), y(v.y), z(v.z){}

    //
    // Operations
    //
    T dist2(const Vector2 &v) const
    { 
      T dx = x - v.x;
      T dy = y - v.y;
      return dx * dx + dy * dy;
    }

    T dist(const Vector2 &v) const
    { 
      return sqrt(dist2(v));
    }

    T norm2() const
    { 
      return x * x + y * y;
    }

    T x;
    T y;
    T z;

};

template <>
float Vector2<float>::dist(const Vector2<float> &v) const { return hypotf(x - v.x, y - v.y);}

template <>
double Vector2<double>::dist(const Vector2<double> &v) const { return hypot(x - v.x, y - v.y);}

template<typename T>
std::ostream &operator << (std::ostream &str, Vector2<T> const &point)
{
  return str << "Point x: " << point.x << " y: " << point.y;
}

template<typename T>
bool operator == (const Vector2<T>& v1, const Vector2<T>& v2)
{
  return (v1.x == v2.x) && (v1.y == v2.y);
}

template<class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
    almost_equal(const Vector2<T>& v1, const Vector2<T>& v2, int ulp=2)
{
  return almost_equal(v1.x, v2.x, ulp) && almost_equal(v1.y, v2.y, ulp);
}

//--

template <class T>
class Edge
{
  public:
    using VertexType = Vector2<T>;

    Edge(const VertexType &p1, const VertexType &p2) : p1(p1), p2(p2), isBad(false) {};
    Edge(const Edge &e) : p1(e.p1), p2(e.p2), isBad(false) {};

    VertexType p1;
    VertexType p2;

    bool isBad;
};

template <class T>
inline std::ostream &operator << (std::ostream &str, Edge<T> const &e)
{
  return str << "Edge " << e.p1 << ", " << e.p2;
}

template <class T>
inline bool operator == (const Edge<T> & e1, const Edge<T> & e2)
{
  return  (e1.p1 == e2.p1 && e1.p2 == e2.p2) ||
      (e1.p1 == e2.p2 && e1.p2 == e2.p1);
}

template <class T>
inline bool almost_equal (const Edge<T> & e1, const Edge<T> & e2)
{
  return  (almost_equal(e1.p1, e2.p1) && almost_equal(e1.p2, e2.p2)) ||
      (almost_equal(e1.p1, e2.p2) && almost_equal(e1.p2, e2.p1));
}

//--


template <class T>
class Triangle
{
  public:
    using EdgeType = Edge<T>;
    using VertexType = Vector2<T>;

    Triangle(const VertexType &_p1, const VertexType &_p2, const VertexType &_p3)
    : p1(_p1), p2(_p2), p3(_p3),
      e1(_p1, _p2), e2(_p2, _p3), e3(_p3, _p1), isBad(false)
    {}

    bool containsVertex(const VertexType &v) const
    { 
      // return p1 == v || p2 == v || p3 == v;
      return almost_equal(p1, v) || almost_equal(p2, v) || almost_equal(p3, v);
    }

    bool circumCircleContains(const VertexType &v) const
    { 
      const T ab = p1.norm2();
      const T cd = p2.norm2();
      const T ef = p3.norm2();

      const T circum_x = (ab * (p3.y - p2.y) + cd * (p1.y - p3.y) + ef * (p2.y - p1.y)) / (p1.x * (p3.y - p2.y) + p2.x * (p1.y - p3.y) + p3.x * (p2.y - p1.y));
      const T circum_y = (ab * (p3.x - p2.x) + cd * (p1.x - p3.x) + ef * (p2.x - p1.x)) / (p1.y * (p3.x - p2.x) + p2.y * (p1.x - p3.x) + p3.y * (p2.x - p1.x));

      const VertexType circum(half(circum_x), half(circum_y));
      const T circum_radius = p1.dist2(circum);
      const T dist = v.dist2(circum);
      return dist <= circum_radius;
    }

    VertexType p1;
    VertexType p2;
    VertexType p3;
    EdgeType e1;
    EdgeType e2;
    EdgeType e3;
    bool isBad;
};

template <class T>
inline std::ostream &operator << (std::ostream &str, const Triangle<T> & t)
{
  return str << "Triangle:" << std::endl << "\t" << t.p1 << std::endl << "\t" << t.p2 << std::endl << "\t" << t.p3 << std::endl << "\t" << t.e1 << std::endl << "\t" << t.e2 << std::endl << "\t" << t.e3 << std::endl;

}

template <class T>
inline bool operator == (const Triangle<T> &t1, const Triangle<T> &t2)
{
  return  (t1.p1 == t2.p1 || t1.p1 == t2.p2 || t1.p1 == t2.p3) &&
      (t1.p2 == t2.p1 || t1.p2 == t2.p2 || t1.p2 == t2.p3) &&
      (t1.p3 == t2.p1 || t1.p3 == t2.p2 || t1.p3 == t2.p3);
}

template <class T>
inline bool almost_equal(const Triangle<T> &t1, const Triangle<T> &t2)
{
  return  (almost_equal(t1.p1 , t2.p1) || almost_equal(t1.p1 , t2.p2) || almost_equal(t1.p1 , t2.p3)) &&
      (almost_equal(t1.p2 , t2.p1) || almost_equal(t1.p2 , t2.p2) || almost_equal(t1.p2 , t2.p3)) &&
      (almost_equal(t1.p3 , t2.p1) || almost_equal(t1.p3 , t2.p2) || almost_equal(t1.p3 , t2.p3));
}


//--

//#include "vector2.h"
//#include "edge.h"
//#include "triangle.h"

template <class T>
class Delaunay
{
	public:
		using TriangleType = Triangle<T>;
		using EdgeType = Edge<T>;
		using VertexType = Vector2<T>;

		const std::vector<TriangleType>& triangulate(std::vector<VertexType> &vertices)
		{
			// Store the vertices locally
			_vertices = vertices;

			// Determinate the super triangle
			T minX = vertices[0].x;
			T minY = vertices[0].y;
			T maxX = minX;
			T maxY = minY;

			for(std::size_t i = 0; i < vertices.size(); ++i)
			{
				if (vertices[i].x < minX) minX = vertices[i].x;
				if (vertices[i].y < minY) minY = vertices[i].y;
				if (vertices[i].x > maxX) maxX = vertices[i].x;
				if (vertices[i].y > maxY) maxY = vertices[i].y;
			}

			const T dx = maxX - minX;
			const T dy = maxY - minY;
			const T deltaMax = std::max(dx, dy);
			const T midx = half(minX + maxX);
			const T midy = half(minY + maxY);

			const VertexType p1(midx - 20 * deltaMax, midy - deltaMax);
			const VertexType p2(midx, midy + 20 * deltaMax);
			const VertexType p3(midx + 20 * deltaMax, midy - deltaMax);

			//std::cout << "Super triangle " << std::endl << Triangle(p1, p2, p3) << std::endl;

			// Create a list of triangles, and add the supertriangle in it
			_triangles.push_back(TriangleType(p1, p2, p3));

			for(auto p = begin(vertices); p != end(vertices); p++)
			{
				//std::cout << "Traitement du point " << *p << std::endl;
				//std::cout << "_triangles contains " << _triangles.size() << " elements" << std::endl;

				std::vector<EdgeType> polygon;

				for(auto & t : _triangles)
				{
					//std::cout << "Processing " << std::endl << *t << std::endl;

					if(t.circumCircleContains(*p))
					{
						//std::cout << "Pushing bad triangle " << *t << std::endl;
						t.isBad = true;
						polygon.push_back(t.e1);
						polygon.push_back(t.e2);
						polygon.push_back(t.e3);
					}
					else
					{
						//std::cout << " does not contains " << *p << " in his circum center" << std::endl;
					}
				}

				_triangles.erase(std::remove_if(begin(_triangles), end(_triangles), [](TriangleType &t){
					return t.isBad;
				}), end(_triangles));

				for(auto e1 = begin(polygon); e1 != end(polygon); ++e1)
				{
					for(auto e2 = e1 + 1; e2 != end(polygon); ++e2)
					{
						if(almost_equal(*e1, *e2))
						{
							e1->isBad = true;
							e2->isBad = true;
						}
					}
				}

				polygon.erase(std::remove_if(begin(polygon), end(polygon), [](EdgeType &e){
					return e.isBad;
				}), end(polygon));

				for(const auto e : polygon)
					_triangles.push_back(TriangleType(e.p1, e.p2, *p));

			}

			_triangles.erase(std::remove_if(begin(_triangles), end(_triangles), [p1, p2, p3](TriangleType &t){
				return t.containsVertex(p1) || t.containsVertex(p2) || t.containsVertex(p3);
			}), end(_triangles));

			for(const auto t : _triangles)
			{
				_edges.push_back(t.e1);
				_edges.push_back(t.e2);
				_edges.push_back(t.e3);
			}

			return _triangles;
		}

		const std::vector<TriangleType>& getTriangles() const { return _triangles; };
		const std::vector<EdgeType>& getEdges() const { return _edges; };
		const std::vector<VertexType>& getVertices() const { return _vertices; };

	private:
		std::vector<TriangleType> _triangles;
		std::vector<EdgeType> _edges;
		std::vector<VertexType> _vertices;
};

#endif
