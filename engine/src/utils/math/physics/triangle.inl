#define REORIENT_NORMALS_ON_FETCH 0
#define REORIENT_NORMALS_ON_CONTACT 1

#include <uf/utils/math/quant.h>

namespace {
	pod::Vector3f triangleCenter( const pod::Triangle& tri ) {
		return ( tri.points[0] + tri.points[1] + tri.points[2] ) / 3.0f;
	}
	pod::Vector3f triangleNormal( const pod::Triangle& tri ) {
		return uf::vector::normalize(uf::vector::cross(tri.points[1] - tri.points[0], tri.points[2] - tri.points[0]));
	}
	pod::Vector3f triangleNormal( const pod::TriangleWithNormal& tri ) {
		return tri.normal;
		//return uf::vector::normalize( tri.normals[0] + tri.normals[1] + tri.normals[2] );
	}

	bool triangleTriangleIntersect( const pod::Triangle& a, const pod::Triangle& b, float eps = EPS ) {
		const float eps2 = eps * eps;
		auto boxA = ::computeTriangleAABB( a );
		auto boxB = ::computeTriangleAABB( b );

		if ( !::aabbOverlap( boxA, boxB ) ) return false;

		// check vertices of a inside b or vice versa
		FOR_EACH(3, {
			auto q = ::closestPointOnTriangle( a.points[i], b );
			if ( uf::vector::magnitude( q - a.points[i] ) < eps2 ) return true;
		});
		FOR_EACH(3, {
			auto q = ::closestPointOnTriangle( b.points[i], a );
			if ( uf::vector::magnitude( q - b.points[i] ) < eps2 ) return true;
		});
		return false;
	}

	size_t getIndex( const void* pointer, size_t stride, size_t index ) { 
		#define CAST_INDEX(T) case sizeof(T): return ((T*) pointer)[index];
		switch ( stride ) {
			CAST_INDEX(uint8_t);
			CAST_INDEX(uint16_t);
			CAST_INDEX(uint32_t);
			default: {
				UF_EXCEPTION("invalid stride type: {}", stride);
			} break;
		}
	}
	size_t getIndex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, size_t index ) { 
		return ::getIndex( indices.data(view.index.first), indices.stride(), index );
	}
	pod::Vector3f getVertex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& positions, size_t index ) {
		const auto stride = positions.stride();
		#define CAST_VERTEX(T) {\
			const T* vertices = (T*) positions.data(view.vertex.first + index);\
			return { vertices[0], vertices[1], vertices[2], };\
		}
		#define DEQUANTIZE_VERTEX(T) {\
			const T* vertices = (T*) positions.data(view.vertex.first + index);\
			return { uf::quant::dequantize(vertices[0]), uf::quant::dequantize(vertices[1]), uf::quant::dequantize(vertices[2]), };\
		}

		switch ( positions.attribute.descriptor.type ) {
			// dequantize
			case uf::renderer::enums::Type::USHORT:
			case uf::renderer::enums::Type::SHORT: {
				DEQUANTIZE_VERTEX(uint16_t);
			} break;
			case uf::renderer::enums::Type::FLOAT: {
				CAST_VERTEX(float);
			} break;
		#if UF_USE_FLOAT16
			case uf::renderer::enums::Type::HALF: {
				CAST_VERTEX(std::float16_t);
			} break;
		#endif
		#if UF_USE_BFLOAT16
			case uf::renderer::enums::Type::BFLOAT: {
				CAST_VERTEX(std::bfloat16_t);
			} break;
		#endif
			default: UF_EXCEPTION("unsupported vertex type: {}", positions.attribute.descriptor.type); break;
		}
	//	return ::getVertex( positions.data(view.vertex.first), positions.stride(), index );
	}

	pod::Triangle fetchTriangle( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, const uf::Mesh::AttributeView& positions, size_t triID ) {
		auto index = triID * 3;
		pod::Triangle tri;
		FOR_EACH(3, {
			tri.points[i] = ::getVertex( view, positions, ::getIndex( view, indices, index + i ) );
		});
		return tri;
	}

	pod::TriangleWithNormal fetchTriangle( const uf::Mesh& mesh, size_t triID ) {
		const auto& views = mesh.buffer_views;
		UF_ASSERT(!views.empty());

		// find which view contains this triangle index.
		size_t triBase = 0;
		const uf::Mesh::View* view = nullptr;
		for ( auto& v : views ) {
			auto trisInView = v.index.count / 3;
			if (triID < triBase + trisInView) {
				view = &v;
				triID -= triBase; // local triangle index inside this view
				break;
			}
			triBase += trisInView;
		}
		UF_ASSERT( view );

		auto& positions = (*view)["position"];
		auto& indices   = (*view)["index"];
		
		pod::TriangleWithNormal tri = { ::fetchTriangle( *view, indices, positions, triID ) };
		tri.normal = uf::vector::normalize(uf::vector::cross(tri.points[1] - tri.points[0], tri.points[2] - tri.points[0]));

		return tri;
	}

	// if body is a mesh, apply its transform to the triangles, else reorient the normal with respect to the body
	pod::TriangleWithNormal fetchTriangle( const uf::Mesh& mesh, size_t triID, const pod::PhysicsBody& body, bool fast = false ) {
		auto tri = ::fetchTriangle( mesh, triID );

		auto transform = ::getTransform( body );

		if ( body.collider.type == pod::ShapeType::MESH ) {
			if ( fast ) {
				FOR_EACH(3, {
					tri.points[i] += transform.position;
				});
			} else {
				FOR_EACH(3, {
					tri.points[i]  = uf::transform::apply( transform, tri.points[i] );
				});
				tri.normal = uf::quaternion::rotate( transform.orientation, tri.normal );
			}
		}
		else {
		#if REORIENT_NORMALS_ON_FETCH
			auto triCenter = ::triangleCenter( tri );
			auto delta = ::getPosition( body ) - triCenter;
			if ( uf::vector::dot(tri.normal, delta) < 0.0f ) tri.normal = -tri.normal;
		#endif
		}

		return tri;
	}

	bool computeTriangleTriangleSegment( const pod::TriangleWithNormal& A, const pod::TriangleWithNormal& B, pod::Vector3f& p0, pod::Vector3f& p1, float eps = EPS ) {
		int intersections = 0;
		pod::Vector3f intersectionBuffers[6];

		auto checkAndPush = [&]( const pod::Vector3f& pt ) {
			// avoid duplicates
			for ( auto& v : intersectionBuffers ) {
				if ( uf::vector::distanceSquared( v, pt ) < eps*eps ) return;
			}
			intersectionBuffers[intersections++] = pt;
		};

		// segment-plane intersection
		auto intersectSegmentPlane = [&](const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& n, float d, pod::Vector3f& out)->bool {
			pod::Vector3f ab = b - a;
			float denom = uf::vector::dot( n, ab );
			if (fabs(denom) < eps) return false; // parallel

			float t = (d - uf::vector::dot( n, a )) / denom;
			if ( t < -eps || t > 1.0f + eps ) return false;
			out = a + ab * t;
			return true;
		};

		// planes
		auto nA = ::triangleNormal( A );
		auto nB = ::triangleNormal( B );
		float dA = uf::vector::dot( nA, A.points[0] );
		float dB = uf::vector::dot( nB, B.points[0] );

		// clip edges of A against plane of B
		const pod::Vector3f At[3] = { A.points[0], A.points[1], A.points[2] };
		FOR_EACH(3, {
			auto j = ( i + 1 ) % 3;
			pod::Vector3f p;
			if ( intersectSegmentPlane( At[i], At[j], nB, dB, p ) ) {
				// check if intersection lies inside triangle B
				if ( ::pointInTriangle( p, B ) ) checkAndPush(p);
			}
		});

		// clip edges of B against plane of A
		const pod::Vector3f Bt[3] = { B.points[0], B.points[1], B.points[2] };
		FOR_EACH(3, {
			auto j = ( i + 1 ) % 3;
			pod::Vector3f p;
			if ( intersectSegmentPlane( Bt[i], Bt[j], nA, dA, p ) ) {
				if ( ::pointInTriangle( p, A ) ) checkAndPush(p);
			}
		});

		if ( intersections == 0 ) return false;

		// degenerate intersection
		if ( intersections == 1 ) {
			p0 = p1 = intersectionBuffers[0];
			return true;
		}

		// find two furthest apart points for intersection segment
		float maxDist2 = -1.0f;
		for ( auto i = 0 ; i < intersections; i++ ) {
			for ( auto j = i + 1; j < intersections; j++ ) {
				float d2 = uf::vector::distanceSquared( intersectionBuffers[i], intersectionBuffers[j] );
				if ( d2 > maxDist2 ) {
					maxDist2 = d2;
					p0 = intersectionBuffers[i];
					p1 = intersectionBuffers[j];
				}
			}
		}

		return maxDist2 >= 0.0f;
	}

	pod::Vector2f projectTriangleOntoAxis( const pod::TriangleWithNormal& tri, const pod::Vector3f& axis ) {
		pod::Vector3f normal = uf::vector::normalize( axis );

		float p0 = uf::vector::dot( tri.points[0], normal );
		float p1 = uf::vector::dot( tri.points[1], normal );
		float p2 = uf::vector::dot( tri.points[2], normal );

		return { std::min({ p0, p1, p2 }), std::max({ p0, p1, p2 }) };
	}

}

// triangle colliders
namespace {
	// i feel like it'd just be better to treat an AABB as a 12-triangle mesh and do a triangleTriangle collision instead of 3 + 1 + 3 * 3 axis tests
	// but what do i know
	bool triangleAabbSAT( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps = 1e-6f ) {
		const float eps2 = eps * eps;

		const auto& aabb = body.bounds;

		// box center and half extents
		pod::Vector3f boxCenter = ::aabbCenter( aabb );
		pod::Vector3f boxHalf   = ::aabbExtent( aabb );

		// move triangle into box-local space
		pod::Vector3f v0 = tri.points[0] - boxCenter;
		pod::Vector3f v1 = tri.points[1] - boxCenter;
		pod::Vector3f v2 = tri.points[2] - boxCenter;

		// triangle edges
		pod::Vector3f e0 = v1 - v0;
		pod::Vector3f e1 = v2 - v1;
		pod::Vector3f e2 = v0 - v2;

		float minOverlap = FLT_MAX;
		pod::Vector3f bestAxis;

		auto testAxis = [&](const pod::Vector3f& axis) -> bool {
			if ( uf::vector::magnitude( axis ) < eps2 ) return true; // skip degenerate

			pod::Vector3f n = uf::vector::normalize(axis);

			// project triangle
			float t0 = uf::vector::dot(v0, n);
			float t1 = uf::vector::dot(v1, n);
			float t2 = uf::vector::dot(v2, n);
			float triMin = std::min({t0, t1, t2});
			float triMax = std::max({t0, t1, t2});

			// project box (radius along axis)
			float r = boxHalf.x * fabs(n.x) + boxHalf.y * fabs(n.y) + boxHalf.z * fabs(n.z); // to-do: use boxHalf + uf::vector::abs( n ) or something

			// overlap test
			if ( triMin > r || triMax < -r ) return false; // separating axis

			// compute overlap depth
			float overlap = std::min(triMax + r, r - triMin);
			if ( overlap < minOverlap ) {
				minOverlap = overlap;
				bestAxis = n;
			}
			return true;
		};

		if ( !testAxis({1,0,0}) ) return false;
		if ( !testAxis({0,1,0}) ) return false;
		if ( !testAxis({0,0,1}) ) return false;
		if ( !testAxis(uf::vector::cross(e0, e1)) ) return false;

		pod::Vector3f boxAxes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
		pod::Vector3f edges[3]   = { e0, e1, e2 };
		for ( auto& edge : edges ) {
			for ( auto& axis : boxAxes ) if ( !testAxis(uf::vector::cross(edge, axis)) ) return false;
		}

		pod::Vector3f triNormal = uf::vector::normalize(uf::vector::cross(e0, e1));
		float planeDist = uf::vector::dot(triNormal, v0);
		if ( uf::vector::dot(bestAxis, triNormal) < 0.0f ) bestAxis = -bestAxis;
		pod::Vector3f contact = boxCenter - bestAxis * (boxHalf.x * fabs(bestAxis.x) + boxHalf.y * fabs(bestAxis.y) + boxHalf.z * fabs(bestAxis.z));
		
		//pod::Vector3f contact = boxCenter - triNormal * planeDist;
	
	/*
		float d = uf::vector::dot(triNormal, v0);
		float dist = uf::vector::dot(triNormal, -boxCenter) - d;
		pod::Vector3f contact = boxCenter - triNormal * dist;

		if ( uf::vector::dot(bestAxis, triNormal) < 0.0f ) bestAxis = -bestAxis;
	*/

		manifold.points.emplace_back( pod::Contact{ contact, bestAxis, minOverlap } );
		return true;
	}

	bool triangleTriangle( const pod::TriangleWithNormal& a, const pod::TriangleWithNormal& b, pod::Manifold& manifold, float eps ) {
		const float eps2 = eps * eps;

		size_t axes = 0;
		pod::Vector3f axesBuffer[12];
		axesBuffer[axes++] = ::triangleNormal(a);
		axesBuffer[axes++] = ::triangleNormal(b);

		for (int i = 0; i < 3; i++) {
			auto ea = a.points[(i+1)%3] - a.points[i];
			for (int j = 0; j < 3; j++) {
				auto eb = b.points[(j+1)%3] - b.points[j];
				auto axis = uf::vector::cross(ea, eb);
				if ( uf::vector::magnitude( axis ) > eps2 ) axesBuffer[axes++] = uf::vector::normalize(axis);
			}
		}

		// SAT test
		float minOverlap = FLT_MAX;
		pod::Vector3f bestAxis;

		for ( auto& axis : axesBuffer ) {
			auto projA = ::projectTriangleOntoAxis(a, axis);
			auto projB = ::projectTriangleOntoAxis(b, axis);

			float overlap = std::min(projA.y, projB.y) - std::max(projA.x, projB.x);
			if (overlap < 0) return false; // separating axis

			if (overlap < minOverlap) {
				minOverlap = overlap;
				bestAxis = axis;
			}
		}


		// clip polygons
		int polyCount = 0;
		pod::Vector3f poly[8];
		poly[polyCount++] = b.points[0];
		poly[polyCount++] = b.points[1];
		poly[polyCount++] = b.points[2];

		auto clipAgainstPlane = [&](const pod::Vector3f& n, const pod::Vector3f& p) {
			int outCount = 0;
			pod::Vector3f out[8];

			for ( auto i = 0; i < polyCount; i++ ) {
				auto curr = poly[i];
				auto prev = poly[(i+polyCount-1)%polyCount];
				float dCurr = uf::vector::dot(n, curr - p);
				float dPrev = uf::vector::dot(n, prev - p);

				if ( dCurr >= 0 ) {
					if ( dPrev < 0 ) {
						float t = dPrev / (dPrev - dCurr);
						out[outCount++] = prev + (curr - prev) * t;
					}
					out[outCount++] = curr;
				} else if ( dPrev >= 0 ) {
					float t = dPrev / (dPrev - dCurr);
					out[outCount++] = prev + (curr - prev) * t;
				}
			}
			// copy back
			polyCount = outCount;
			for ( auto i = 0; i < outCount; i++ ) poly[i] = out[i];
		};
		
		if ( uf::vector::dot(bestAxis, ::triangleCenter(b) - ::triangleCenter(a)) < 0.0f ) bestAxis = -bestAxis;
		/*
		pod::Vector3f centroid{0,0,0};
		for ( auto i = 0; i < polyCount; i++ ) centroid += poly[i];
		centroid /= (float) polyCount;
		if ( uf::vector::dot(bestAxis, centroid - ::triangleCenter(a)) < 0.0f ) bestAxis = -bestAxis;
		*/

		for ( auto i = 0; i < 3; i++ ) {
			auto p0 = a.points[i];
			auto p1 = a.points[(i+1)%3];
			auto edge = p1 - p0;
			auto edgeNormal = uf::vector::normalize(uf::vector::cross(bestAxis, edge));
			clipAgainstPlane(edgeNormal, p0);
			if ( polyCount == 0 ) return false;
		}

		// build manifold
		float penetration = std::max( minOverlap, 0.05f ); // slop
		for (int i = 0; i < polyCount; i++) {
			manifold.points.emplace_back(pod::Contact{ poly[i], bestAxis, penetration });
		}

		return ( polyCount > 0 );
	}

	bool triangleAabbTri( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps ) {
		// 8 corners
		pod::Vector3f v[8] = {
			{body.bounds.min.x, body.bounds.min.y, body.bounds.min.z},
			{body.bounds.max.x, body.bounds.min.y, body.bounds.min.z},
			{body.bounds.max.x, body.bounds.max.y, body.bounds.min.z},
			{body.bounds.min.x, body.bounds.max.y, body.bounds.min.z},
			{body.bounds.min.x, body.bounds.min.y, body.bounds.max.z},
			{body.bounds.max.x, body.bounds.min.y, body.bounds.max.z},
			{body.bounds.max.x, body.bounds.max.y, body.bounds.max.z},
			{body.bounds.min.x, body.bounds.max.y, body.bounds.max.z}
		};

		pod::TriangleWithNormal tris[12] = {
			{ {v[0], v[4], v[7]}, {-1,0,0} }, { {v[0], v[7], v[3]}, {-1,0,0} }, // left (x-)
			{ {v[1], v[5], v[6]}, {1,0,0} }, { {v[1], v[6], v[2]}, {1,0,0} }, // right (x+)
			{ {v[0], v[1], v[5]}, {0,-1,0} }, { {v[0], v[5], v[4]}, {0,-1,0} }, // bottom (y-)
			{ {v[3], v[2], v[6]}, {0,1,0} }, { {v[3], v[6], v[7]}, {0,1,0} }, // top (y+)
			{ {v[0], v[1], v[2]}, {0,0,-1} }, { {v[0], v[2], v[3]}, {0,0,-1} }, // back (z-)
			{ {v[4], v[5], v[6]}, {0,0,1} }, { {v[4], v[6], v[7]}, {0,0,1} }, // front (z+)
		};

		bool hit = false;
		for ( auto& t : tris ) {
			if ( ::triangleTriangle( tri, t, manifold, eps ) ) hit = true;
		}
		return hit;
	}
	bool triangleAabb( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps ) {
		return ::triangleAabbSAT( tri, body, manifold, eps );
		//return ::triangleAabbTri( tri, body, manifold, eps );
	}
	bool triangleSphere( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps ) {
		const auto& sphere = body;

		float r = sphere.collider.sphere.radius;
		auto center = ::getPosition( sphere );
		auto closest = ::closestPointOnTriangle( center, tri.points[0], tri.points[1], tri.points[2] );

		if ( !uf::vector::isValid( closest ) ) return false;

		// to-do: derive proper delta
		auto delta = center - closest;
		float dist2 = uf::vector::magnitude( delta );
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt(dist2);

		auto contact = ( center + closest ) * 0.5f;
		auto normal = ( dist > eps ) ? ( delta / dist ) : ::triangleNormal( tri );
		float penetration = r - dist;

	#if REORIENT_NORMALS_ON_CONTACT
		if ( uf::vector::dot( normal, delta ) < 0.0f ) normal = -normal;
	#endif

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	// to-do: implement
	bool trianglePlane( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps ) {
		const auto& plane = body;
		auto normal = plane.collider.plane.normal;
		float d = plane.collider.plane.offset;

		bool hit = false;
		pod::Vector3f dist;
		FOR_EACH(3, {
			dist[i] = uf::vector::dot(normal, tri.points[i] ) - d;
		});

		// completely on one side
		bool allAbove = ( dist.x >  eps && dist.y >  eps && dist.z >  eps );
		bool allBelow = ( dist.x < -eps && dist.y < -eps && dist.z < -eps );
		if ( allAbove )
			return hit;

		if ( allBelow ) {
			hit = true;
			FOR_EACH(3, {
				float penetration = -dist[i];
				manifold.points.emplace_back(pod::Contact{tri.points[i], normal, -dist[i]});
			});
			return hit;
		}

		// points touching plane
		for ( auto i = 0; i < 3; i++ )
			if ( fabs( dist[i] ) <= eps ) {
				hit = true;
				manifold.points.emplace_back(pod::Contact{ tri.points[i], normal, 0.0f });
			}

		// edges that cross plane
		for ( auto i = 0; i < 3; i++ ) {
			auto j = (i + 1) % 3;
			if ( ( dist[i] > 0 && dist[j] < 0 ) || ( dist[i] < 0 && dist[j] > 0 ) ) {
				hit = true;
				float t = dist[i] / ( dist[i] - dist[j] );
				auto contact = tri.points[i] + ( tri.points[j] - tri.points[i] ) * t;
				float penetration = -std::min( dist[i], dist[j] );
				manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
			}
		}
		return hit;
	}
	bool triangleCapsule( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps ) {
		const auto& capsule = body;

		float r = capsule.collider.capsule.radius;
		auto [ p1, p2 ] = ::getCapsuleSegment( capsule );
		auto bounds = ::computeSegmentAABB( p1, p2, r );

		// to-do: derive proper delta
		pod::Vector3f closestSeg = {}, closest = {};
		float dist2 = ::segmentTriangleDistanceSq( p1, p2, tri, closestSeg, closest );

		if ( !uf::vector::isValid( closest ) ) return false;
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt( dist2 );
		auto delta = ( closestSeg - closest );

		// to-do: properly derive the contact information
		auto contact = closest; // ( closestSeg + closest ) * 0.5f;
		auto normal = ( dist > eps ) ? ( delta / dist ) : ::triangleNormal( tri );
		float penetration = r - dist;

	#if REORIENT_NORMALS_ON_CONTACT
		if ( uf::vector::dot( normal, delta ) < 0.0f ) normal = -normal;
	#endif

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}

	bool triangleTriangle( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( TRIANGLE, TRIANGLE );
		return ::triangleTriangle( a.collider.triangle, b.collider.triangle, manifold, eps );
	}
	bool triangleAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( TRIANGLE, AABB );
		return ::triangleAabb( a.collider.triangle, b, manifold, eps );
	}
	bool triangleSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( TRIANGLE, SPHERE );
		return ::triangleSphere( a.collider.triangle, b, manifold, eps );
	}
	bool trianglePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( TRIANGLE, PLANE );
		return ::trianglePlane( a.collider.triangle, b, manifold, eps );
	}
	bool triangleCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( TRIANGLE, CAPSULE );
		return ::triangleCapsule( a.collider.triangle, b, manifold, eps );
	}
}