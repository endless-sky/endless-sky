/* DelaunayTriangulation.h
Copyright (c) 2023 by thewierdnut

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef DELAUNAY_GRAPH_INCLUDED
#define DELAUNAY_GRAPH_INCLUDED

#include <cstddef>
#include <map>
#include <set>
#include <vector>

#include "../Point.h"


// Build a Delaunay triangulation from a set of points using the Bowyer-Watson
// algorithm
class DelaunayTriangulation
{
public:
	DelaunayTriangulation()
	{
		// Need to seed the algorithm with a super-triangle that contains every
		// possible point.
		static constexpr float SUPER_TRIANGLE_EXTENT = 10000;
		m_points.emplace_back(Point{0.f, SUPER_TRIANGLE_EXTENT});
		m_points.emplace_back(Point{-SUPER_TRIANGLE_EXTENT, -SUPER_TRIANGLE_EXTENT});
		m_points.emplace_back(Point{SUPER_TRIANGLE_EXTENT, -SUPER_TRIANGLE_EXTENT});
		m_triangles.emplace_back(Triangle{0, 1, 2});
	}

	void AddPoint(const Point &p)
	{
		// TODO: this could be sped up using some sort of point sorting, but I
		//       don't expect more than a few dozen points at maximum.
		// Find every triangle whose circumcircles overlap with the new point,
		// and remove them.
		std::map<Edge, size_t> edges;
		for(auto it = m_triangles.begin(); it != m_triangles.end();)
		{
			if(PointInCircumCircle(p, *it))
			{
				// Track the edges. We want to find the unique edges (ie the edges
				// that describe the polygon surrounding our bad triangles). Make
				// sure that the point indexes are sorted, so that edge ab is
				// counted the same as ba
				++edges[Sort2(it->a, it->b)];
				++edges[Sort2(it->b, it->c)];
				++edges[Sort2(it->a, it->c)];

				// remove this bad triangle
				it = m_triangles.erase(it);
			}
			else
				++it;
		}

		size_t new_point = m_points.size();
		m_points.emplace_back(p);

		// The bad triangles have been removed. Now add in new triangles with the
		// new point and every unique edge for the triangles we removed
		for(auto &kv : edges)
		{
			if(kv.second == 1)
			{
				m_triangles.emplace_back(Triangle{kv.first.first, kv.first.second, new_point});
			}
		}
	}

	std::set<std::pair<size_t, size_t>> Edges(bool include_alternative_edges = true) const
	{
		// Return all edges that are not attached to seed triangles.
		std::set<std::pair<size_t, size_t>> ret;

		// As a bonus feature, also include edges that are alternatives to the
		// given solution (think rectangles with edges attaching both opposite
		// corners, instead of just one, since either one is a valid solution)
		// there is probably a more generalized solution that doesn't involve
		// checking for right triangles, but this is going to be the most common
		// case for my use-case.
		std::map<Edge, size_t> hypotenuses;
		auto RightEdgeCheck = [&](size_t a, size_t b, size_t c)
		{
			if(IsRightAngle(a, b, c))
			{
				auto result = hypotenuses.emplace(Sort2(a, c), b);
				if(!result.second) // if it fails to add, we found an existing one
				{
					// we found the triangle attached to this one
					ret.emplace(Sort2(result.first->second - 3, b - 3));
					// We don't need to remember it anymore.
					hypotenuses.erase(result.first);
				}
			}
		};

		for(const Triangle &triangle : m_triangles)
		{
			// Don't count any edges containing our seed triangle
			if(triangle.a >= 3 && triangle.b >= 3) ret.emplace(Sort2(triangle.a - 3, triangle.b - 3));
			if(triangle.a >= 3 && triangle.c >= 3) ret.emplace(Sort2(triangle.a - 3, triangle.c - 3));
			if(triangle.b >= 3 && triangle.c >= 3) ret.emplace(Sort2(triangle.b - 3, triangle.c - 3));

			if(include_alternative_edges)
			{
				RightEdgeCheck(triangle.a, triangle.b, triangle.c);
				RightEdgeCheck(triangle.b, triangle.c, triangle.a);
				RightEdgeCheck(triangle.c, triangle.a, triangle.b);
			}
		}
		return ret;
	}

	std::vector<Point> Points() const { return std::vector<Point>(m_points.begin() + 3, m_points.end()); }

private:
	struct Triangle { size_t a, b, c; };
	typedef std::pair<size_t, size_t> Edge;

	bool PointInCircumCircle(const Point &p, const Triangle &t)
	{
		auto &a = m_points[t.a];
		auto &b = m_points[t.b];
		auto &c = m_points[t.c];
		// Compute the center of the circle. Translating the points so that a is
		// at the origin greatly simplifies the math.
		Point bp = b - a;
		Point cp = c - a;
		float dp = 2 * (bp.X() * cp.Y() - bp.Y() * cp.X());
		// u', the center, relative to a
		Point up {
			((bp.LengthSquared()) * cp.Y() - (cp.LengthSquared()) * (bp.Y())) / dp,
			((cp.LengthSquared()) * bp.X() - (bp.LengthSquared()) * (cp.X())) / dp
		};

		// Translate p to the same coordinates as the other points, then check if
		// it is within the radius of the center
		Point pp = p - a - up;
		return pp.LengthSquared() < up.LengthSquared();
	}

	static Edge Sort2(size_t a, size_t b)
	{
		return a < b ? Edge(a, b) : Edge(b, a);
	};

	bool IsRightAngle(size_t a, size_t b, size_t c) const
	{
		// dot product of right angle is zero
		auto &pa = m_points[a];
		auto &pb = m_points[b];
		auto &pc = m_points[c];
		Point da = pa - pb;
		Point dc = pc - pb;
		float dp = da.Dot(dc);
		return -.0001 < dp && dp < .0001;
	}


private:
	std::vector<Point> m_points;

	// We mutate this structure randomly, but vectors are usually faster than
	// maps for small data sets.
	std::vector<Triangle> m_triangles;
};

#endif
