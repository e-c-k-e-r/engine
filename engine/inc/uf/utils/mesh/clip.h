#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/shapes.h>

namespace uf {
	namespace shapes {
		template <typename T>
		void clipPolygon( T* poly, int& polyCount, const pod::Plane& plane ) {
			if ( polyCount == 0 ) return;

			int outCount = 0;
			T out[16];

			for ( auto i = 0; i < polyCount; i++ ) {
				const T& curr = poly[i];
				const T& prev = poly[(i + polyCount - 1) % polyCount];

				float dCurr = uf::vector::dot(plane.normal, curr.position) - plane.offset;
				float dPrev = uf::vector::dot(plane.normal, prev.position) - plane.offset;

				if ( dCurr >= 0.0f ) {
					if ( dPrev < 0.0f ) {
						float t = dPrev / (dPrev - dCurr);
						out[outCount++] = T::interpolate( prev, curr, t );
					}
					out[outCount++] = curr;
				} else if ( dPrev >= 0.0f ) {
					float t = dPrev / (dPrev - dCurr);
					out[outCount++] = T::interpolate( prev, curr, t );
				}
			}

			polyCount = outCount;
			for ( auto i = 0; i < outCount; i++ ) poly[i] = out[i];
		}

		template <typename T>
		void clipPolygon( T* poly, int& polyCount, const pod::AABB& bounds ) {
			pod::Plane planes[6] = {
				{ pod::Vector3f{-1, 0, 0}, -bounds.max.x },
				{ pod::Vector3f{ 1, 0, 0},  bounds.min.x },
				{ pod::Vector3f{ 0,-1, 0}, -bounds.max.y },
				{ pod::Vector3f{ 0, 1, 0},  bounds.min.y },
				{ pod::Vector3f{ 0, 0,-1}, -bounds.max.z },
				{ pod::Vector3f{ 0, 0, 1},  bounds.min.z }
			};

			for ( int i = 0; i < 6; i++ ) {
				clipPolygon<T>( poly, polyCount, planes[i] );
				if ( polyCount < 3 ) {
					polyCount = 0;
					break;
				}
			}
		}

		template<typename T, typename U = uint32_t>
		void clip( uf::stl::vector<T>& vertices, uf::stl::vector<U>& indices, const pod::AABB& bounds ) {
			if ( indices.empty() || vertices.empty() ) return;

			uf::stl::vector<T> outVertices;
			uf::stl::vector<U> outIndices;

			outVertices.reserve( vertices.size() );
			outIndices.reserve( indices.size() );

			for ( size_t i = 0; i < indices.size(); i += 3 ) {
				T poly[16];
				poly[0] = vertices[indices[i + 0]];
				poly[1] = vertices[indices[i + 1]];
				poly[2] = vertices[indices[i + 2]];
				int polyCount = 3;

				clipPolygon<T>( poly, polyCount, bounds );

				if ( polyCount < 3 ) continue;

				U baseIndex = static_cast<U>(outVertices.size());

				for ( int j = 0; j < polyCount; ++j ) {
					outVertices.emplace_back( poly[j] );
				}

				for ( int j = 1; j < polyCount - 1; ++j ) {
					outIndices.emplace_back( baseIndex );
					outIndices.emplace_back( baseIndex + j );
					outIndices.emplace_back( baseIndex + j + 1 );
				}
			}

			vertices = std::move( outVertices );
			indices = std::move( outIndices );
		}

		template<typename T>
		void clip( uf::stl::vector<T>& vertices, const pod::AABB& bounds ) {
			if ( vertices.empty() ) return;

			uf::stl::vector<T> outVertices;
			outVertices.reserve( vertices.size() );

			for ( size_t i = 0; i < vertices.size(); i += 3 ) {
				T poly[16];
				poly[0] = vertices[i + 0];
				poly[1] = vertices[i + 1];
				poly[2] = vertices[i + 2];
				int polyCount = 3;

				clipPolygon<T>( poly, polyCount, bounds );

				if ( polyCount < 3 ) continue;

				for ( int j = 1; j < polyCount - 1; ++j ) {
					outVertices.emplace_back( poly[0] );
					outVertices.emplace_back( poly[j] );
					outVertices.emplace_back( poly[j + 1] );
				}
			}

			vertices = std::move( outVertices );
		}

	}
}