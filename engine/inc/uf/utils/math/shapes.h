#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>

namespace pod {
	struct Plane {
		pod::Vector3f normal;
		float distance;
	};

	struct Sphere {
		pod::Vector3f origin;
		float radius;
	};

	struct Cube {
		pod::Vector3f origin;
		pod::Vector3f size;
	};

	struct Triangle {
		pod::Vector3f points[3];
	};

	template<typename T>
	struct Polygon {
		uf::stl::vector<T> points;
	};

	struct Cone {
		pod::Vector3f origin;
		pod::Vector3f direction;
		float height;
		float radius;
	};

	struct Torus {
		pod::Vector3f origin;
		float majorRadius;
		float minorRadius;
	};

	struct Cylinder {
		pod::Vector3f origin;
		pod::Vector3f direction;
		float height;
		float radius;
	};
}

namespace uf {
	namespace shapes {
		inline float planePointSide( const pod::Plane& plane, const pod::Vector3f& point ) {
			return uf::vector::dot( point, plane.normal ) - plane.distance;
		}

		inline float planeEdgeIntersection( const pod::Plane& plane, const pod::Vector3f& p1, const pod::Vector3f& p2 ) {
			return (plane.distance - uf::vector::dot(p1, plane.normal)) / uf::vector::dot(uf::vector::subtract(p2, p1), plane.normal);
		}

		template<typename T, typename U = uint32_t>
		uf::stl::vector<pod::Polygon<T>> verticesPolygonate( const uf::stl::vector<T>& vertices, const uf::stl::vector<U>& indices ) {
			uf::stl::vector<pod::Polygon<T>> polygons;
			polygons.reserve( indices.size() / 3 );

			for ( auto i = 0; i < indices.size(); i += 3 ) {
				polygons.emplace_back( pod::Polygon<T>{ { vertices[indices[i+0]], vertices[indices[i+1]], vertices[indices[i+2]] } });
			}

			return polygons;
		}

		template<typename T>
		uf::stl::vector<pod::Polygon<T>> verticesPolygonate( const uf::stl::vector<T>& vertices ) {
			uf::stl::vector<pod::Polygon<T>> polygons;
			polygons.reserve( vertices.size() / 3 );

			for ( auto i = 0; i < vertices.size(); i += 3 ) {
				polygons.emplace_back( pod::Polygon<T>{ { vertices[i+0], vertices[i+1], vertices[i+2] } });
			}

			return polygons;
		}

		template<typename T>
		uf::stl::vector<T> polygonsTriangulate( const uf::stl::vector<pod::Polygon<T>>& polygons ) {
			uf::stl::vector<T> vertices;
			vertices.reserve( polygons.size() * 3 );
			
			for ( const auto& polygon : polygons ) {
				for (size_t i = 1; i < polygon.points.size() - 1; ++i) {
					vertices.emplace_back(polygon.points[0]);
					vertices.emplace_back(polygon.points[i]);
					vertices.emplace_back(polygon.points[i + 1]);
				}
			}

			// rewind
			for ( auto i = 0; i < vertices.size(); i += 3 ) {
				for ( const auto& polygon : polygons ) {
				const auto& p0 = vertices[i+0];
				const auto& p1 = vertices[i+1];
				const auto& p2 = vertices[i+2];

				auto edge1 = p1.position - p0.position;
				auto edge2 = p2.position - p0.position;
				auto polygonNormal = uf::vector::cross(edge1, edge2);

				if ( uf::vector::dot( polygonNormal, p0.normal ) < 0 ) {
					std::swap( vertices[i+1], vertices[i+2] );
				}
			}
			}

			return vertices;
		}

		template<typename T, typename U = uint32_t>
		uf::stl::vector<U> verticesIndex( const uf::stl::vector<T>& vertices ) {
			uf::stl::vector<U> indices;
			indices.reserve( vertices.size() );

			for ( auto i = 0; i < vertices.size(); ++i ) {
				indices.emplace_back( i );
			}

			return indices;
		}

		template<typename T, typename U = uint32_t>
		uf::stl::vector<T> indexedVertices( const uf::stl::vector<T>& vertices, const uf::stl::vector<U>& indices ) {
			uf::stl::vector<T> newVertices;

			for ( auto idx : indices ) {
				newVertices.emplace_back( vertices[idx] );
			}

			return newVertices;
		}


		// to-do: fix why these causes problems
		// the polygon approach works fine enough
		template<typename T>
		uf::stl::vector<T> verticesPlaneClip( const uf::stl::vector<T>& vertices, const pod::Plane& plane ) {
			uf::stl::vector<T> newVertices;
			
			for ( auto i = 0; i < vertices.size(); i += 3 ) {
				const auto& v0 = vertices[i+0];
				const auto& v1 = vertices[i+1];
				const auto& v2 = vertices[i+2];

			/*
				float s0 = uf::shapes::planePointSide( plane, v0.position ) > 0;
				float s1 = uf::shapes::planePointSide( plane, v1.position ) > 0;
				float s2 = uf::shapes::planePointSide( plane, v2.position ) > 0;

				if ( s0 == s1 && s1 == s2 ) {
					newVertices.emplace_back( v0 );
					newVertices.emplace_back( v1 );
					newVertices.emplace_back( v2 );

					continue;
				}

				if ( s0 != s1 ) {
					float t = planeEdgeIntersection( plane, v0.position, v1.position );
					T intersection = T::interpolate( v0, v1, t );

					if ( s0 ) {
		                newVertices.emplace_back( v0 );
		                newVertices.emplace_back( intersection );
		                newVertices.emplace_back( v1 );
					} else {
                		newVertices.emplace_back( v1 );
                		newVertices.emplace_back( intersection );
                		newVertices.emplace_back( v0 );
                	}
				}

				if ( s1 != s2 ) {
					float t = planeEdgeIntersection( plane, v1.position, v2.position );
					T intersection = T::interpolate( v1, v2, t );

					if ( s1 ) {
						newVertices.emplace_back( v1 );
						newVertices.emplace_back( intersection );
						newVertices.emplace_back( v2 );
					} else {
						newVertices.emplace_back( v2 );
						newVertices.emplace_back( intersection );
						newVertices.emplace_back( v1 );
					}
				}

				if ( s2 != s0 ) {
					float t = planeEdgeIntersection( plane, v2.position, v0.position );
					T intersection = T::interpolate( v2, v0, t );

					if ( s2 ) {
						newVertices.emplace_back( v2 );
						newVertices.emplace_back( intersection );
						newVertices.emplace_back( v0 );
					} else {
						newVertices.emplace_back( v0 );
						newVertices.emplace_back( intersection );
						newVertices.emplace_back( v2 );
					}
				}
			*/
				float d0 = uf::shapes::planePointSide( plane, v0.position );
				float d1 = uf::shapes::planePointSide( plane, v1.position );
				float d2 = uf::shapes::planePointSide( plane, v2.position );

				if (d0 < 0 && d1 < 0 && d2 < 0) {
					continue;
				}

				if (d0 >= 0 && d1 >= 0 && d2 >= 0) {
					newVertices.emplace_back( v0 );
					newVertices.emplace_back( v1 );
					newVertices.emplace_back( v2 );
					continue;
				}
				
				{
					// Some vertices are in front and some are behind
					bool edge0Crosses = (d0 >= 0) != (d1 >= 0);
					bool edge1Crosses = (d1 >= 0) != (d2 >= 0);
					bool edge2Crosses = (d2 >= 0) != (d0 >= 0);

					T intersection01;
					T intersection12;
					T intersection20;

					if ( edge0Crosses ) {
						auto t = planeEdgeIntersection( plane, v0.position, v1.position );
						intersection01 = T::interpolate( v0, v1, t );
					}
					if ( edge1Crosses ) {
						auto t = planeEdgeIntersection( plane, v1.position, v2.position );
						intersection12 = T::interpolate( v1, v2, t );
					}
					if ( edge2Crosses ) {
						auto t = planeEdgeIntersection( plane, v2.position, v0.position );
						intersection20 = T::interpolate( v2, v0, t );
					}

					if (edge0Crosses && edge1Crosses && edge2Crosses) {
						// All edges cross the plane: split into three triangles
						newVertices.emplace_back( intersection01 );
						newVertices.emplace_back( intersection12 );
						newVertices.emplace_back( intersection20 );
					} else if (edge0Crosses && edge1Crosses) {
						// Edges 0 and 1 cross: create a quad
						newVertices.emplace_back( intersection01 );
						newVertices.emplace_back( intersection12 );
						newVertices.emplace_back( v1 );

						newVertices.emplace_back( intersection01 );
						newVertices.emplace_back( v1 );
						newVertices.emplace_back( v0 );
					} else if (edge1Crosses && edge2Crosses) {
						// Edges 1 and 2 cross: create a quad
						newVertices.emplace_back( intersection12 );
						newVertices.emplace_back( intersection20 );
						newVertices.emplace_back( v2 );

						newVertices.emplace_back( intersection12 );
						newVertices.emplace_back( v2 );
						newVertices.emplace_back( v1 );
					} else if (edge2Crosses && edge0Crosses) {
						// Edges 2 and 0 cross: create a quad
						newVertices.emplace_back( intersection20 );
						newVertices.emplace_back( intersection01 );
						newVertices.emplace_back( v0 );

						newVertices.emplace_back( intersection20 );
						newVertices.emplace_back( v0 );
						newVertices.emplace_back( v2 );
					} else if (edge0Crosses) {
						// Only edge 0 crosses: create a triangle
						newVertices.emplace_back( intersection01 );
						newVertices.emplace_back( v1 );
						newVertices.emplace_back( v0 );
					} else if (edge1Crosses) {
						// Only edge 1 crosses: create a triangle
						newVertices.emplace_back( intersection12 );
						newVertices.emplace_back( v2 );
						newVertices.emplace_back( v1 );
					} else if (edge2Crosses) {
						// Only edge 2 crosses: create a triangle
						newVertices.emplace_back( intersection20 );
						newVertices.emplace_back( v0 );
						newVertices.emplace_back( v2 );
					}
				}
			}

			return newVertices;
		}

		template<typename T>
		pod::Polygon<T> polygonPlaneClip( const pod::Polygon<T>& polygon, const pod::Plane& plane ) {
			pod::Polygon<T> clipped;

			auto count = polygon.points.size();
			bool modified = false;
			for (auto i = 0; i < count; ++i) {
				const T& current = polygon.points[i];
				const T& previous = polygon.points[(i - 1 + count) % count];
				
				float currentSide = uf::shapes::planePointSide( plane, current.position );
				float previousSide = uf::shapes::planePointSide( plane, previous.position );

				if (currentSide >= 0) {
					if (previousSide < 0) {
						float t = uf::shapes::planeEdgeIntersection( plane, previous.position, current.position );
						T interpolated = T::interpolate( previous, current, t );
						clipped.points.emplace_back(interpolated);

						modified = true;
					}
					clipped.points.emplace_back(current);
				} else if (previousSide >= 0) {
					float t = uf::shapes::planeEdgeIntersection( plane, previous.position, current.position );
					T interpolated = T::interpolate( previous, current, t );
					clipped.points.emplace_back(interpolated);
					
					modified = true;
				}
			}

			if ( modified ) {
				std::reverse( clipped.points.begin(), clipped.points.end() );
			}

			return clipped;
		}

		template<typename T>
		uf::stl::vector<pod::Polygon<T>>& clip(
			uf::stl::vector<pod::Polygon<T>>& polygons,

			const pod::Vector3f& min,
			const pod::Vector3f& max
		) {
			// inverted because its needed
			uf::stl::vector<pod::Plane> planes = {
				// Right plane: x = max.x
				{ pod::Vector3f{-1, 0, 0}, -max.x },
				// Left plane: x = min.x
				{ pod::Vector3f{1, 0, 0}, min.x },
				// Top plane: y = max.y
				{ pod::Vector3f{0, -1, 0}, -max.y },
				// Bottom plane: y = min.y
				{ pod::Vector3f{0, 1, 0}, min.y },
				// Front plane: z = max.z
				{ pod::Vector3f{0, 0, -1}, -max.z },
				// Back plane: z = min.z
				{ pod::Vector3f{0, 0, 1}, min.z }
			};

			// clip against planes
			for ( const auto& plane : planes ) {
				uf::stl::vector<pod::Polygon<T>> clippedPolygons;
				for ( const auto& polygon : polygons  ) {
					auto clipped = uf::shapes::polygonPlaneClip<T>( polygon, plane );
					if ( clipped.points.size() >= 3 ) clippedPolygons.emplace_back( clipped );
				}
				polygons = std::move(clippedPolygons);
			}

			return polygons;
		}
		
		template<typename T, typename U = uint32_t>
		uf::stl::vector<T>& clip(
			uf::stl::vector<T>& vertices,

			const pod::Vector3f& min,
			const pod::Vector3f& max,

			bool polygons = true
		) {
			if ( polygons ) {
				// convert to polys
				auto polygons = uf::shapes::verticesPolygonate<T>( vertices );
				// clip against planes
				uf::shapes::clip<T>( polygons, min, max );
				// triangulate
				vertices = uf::shapes::polygonsTriangulate( polygons );

				return vertices;
			}

			// inverted because its needed
			uf::stl::vector<pod::Plane> planes = {
				// Right plane: x = max.x
				{ pod::Vector3f{-1, 0, 0}, -max.x },
				// Left plane: x = min.x
				{ pod::Vector3f{1, 0, 0}, min.x },
				// Top plane: y = max.y
				{ pod::Vector3f{0, -1, 0}, -max.y },
				// Bottom plane: y = min.y
				{ pod::Vector3f{0, 1, 0}, min.y },
				// Front plane: z = max.z
				{ pod::Vector3f{0, 0, -1}, -max.z },
				// Back plane: z = min.z
				{ pod::Vector3f{0, 0, 1}, min.z }
			};

			// clip against planes
			for ( const auto& plane : planes ) {
				vertices = uf::shapes::verticesPlaneClip( vertices, plane );
			}

			return vertices;
		}

		template<typename T, typename U = uint32_t>
		void clip(
			uf::stl::vector<T>& vertices,
			uf::stl::vector<U>& indices,

			const pod::Vector3f& min,
			const pod::Vector3f& max,

			bool polygons = true
		) {
			if ( polygons ) {
				// convert to polys
				auto polygons = uf::shapes::verticesPolygonate<T, U>( vertices, indices );
				// clip against planes
				uf::shapes::clip<T>( polygons, min, max );
				// triangulate
				vertices = uf::shapes::polygonsTriangulate( polygons );
				indices = uf::shapes::verticesIndex( vertices );
				return;
			}

			// deindex vertices
			vertices = uf::shapes::indexedVertices( vertices, indices );
			// clip vertices
			uf::shapes::clip( vertices, min, max );
			// reindex
			indices = uf::shapes::verticesIndex( vertices );
		}
	}
}