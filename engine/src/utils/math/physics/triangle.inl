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

	bool triangleTriangleIntersect( const pod::Triangle& a, const pod::Triangle& b, float eps = EPS(1e-6f) ) {
		auto boxA = ::computeTriangleAABB( a );
		auto boxB = ::computeTriangleAABB( b );

		if ( !::aabbOverlap( boxA, boxB ) ) return false;

		// check vertices of a inside b or vice versa
		FOR_EACH(3, {
			auto q = ::closestPointOnTriangle( a.points[i], b );
			if ( uf::vector::magnitude( q - a.points[i] ) < eps ) return true;
		});
		FOR_EACH(3, {
			auto q = ::closestPointOnTriangle( b.points[i], a );
			if ( uf::vector::magnitude( q - b.points[i] ) < eps ) return true;
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
		static thread_local uf::stl::unordered_map<const uf::Mesh*, uf::stl::vector<uf::Mesh::View>> cachedViews;
		if ( cachedViews.count(&mesh) == 0 ) cachedViews[&mesh] = mesh.makeViews({"position"});
		auto& views = cachedViews[&mesh];
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

	bool computeTriangleTriangleSegment( const pod::TriangleWithNormal& A, const pod::TriangleWithNormal& B, pod::Vector3f& p0, pod::Vector3f& p1, float eps = EPS(1e-6f) ) {
		uf::stl::vector<pod::Vector3f> intersections;
		intersections.reserve(3);

		auto checkAndPush = [&]( const pod::Vector3f& pt ) {
			// avoid duplicates
			for ( auto& v : intersections ) {
				if ( uf::vector::distanceSquared( v, pt ) < eps*eps ) return;
			}
			intersections.emplace_back(pt);
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

		if ( intersections.empty() ) return false;

		// degenerate intersection
		if ( intersections.size() == 1 ) {
			p0 = p1 = intersections[0];
			return true;
		}

		// find two furthest apart points for intersection segment
		float maxDist2 = -1.0f;
		for ( auto i = 0 ; i < intersections.size(); i++ ) {
			for ( auto j = i + 1 ; j<intersections.size(); j++ ) {
				float d2 = uf::vector::distanceSquared( intersections[i], intersections[j] );
				if ( d2 > maxDist2 ) {
					maxDist2 = d2;
					p0 = intersections[i];
					p1 = intersections[j];
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
	bool triangleTriangle( const pod::TriangleWithNormal& a, const pod::TriangleWithNormal& b, pod::Manifold& manifold, float eps ) {
		// if ( !::triangleTriangleIntersect( a, b ) ) return false;

		uf::stl::vector<pod::Vector3f> axes = { ::triangleNormal( a ), ::triangleNormal( b ) };
		axes.reserve(2+3);

		pod::Vector3f p0 = {}, p1 = {};
		if ( !::computeTriangleTriangleSegment(a, b, p0, p1) ) {
			auto contact = ( p0 + p1 ) * 0.5f;
			auto normal   = uf::vector::normalize( axes[0] + axes[1] );
			manifold.points.emplace_back(pod::Contact{ contact, normal, eps });
			return true;
		}

		auto contact = ( p0 + p1 ) * 0.5f;
		float penetration = std::numeric_limits<float>::max();
		pod::Vector3f normal;

		// check edge cross-products
		for ( auto i = 0; i < 3; i++ ) {
			auto ea = a.points[( i + 1 ) % 3] - a.points[i];
			for ( auto j = 0; j < 3; j++ ) {
				auto eb = b.points[( j + 1 ) % 3] - b.points[j];
				auto axis = uf::vector::cross(ea, eb);
				if ( uf::vector::magnitude( axis ) > eps*eps ) axes.emplace_back( axis );
			}
		}

		// project onto each axis
		for ( auto axis : axes ) {
			axis = uf::vector::normalize( axis );
			pod::Vector2f aP = ::projectTriangleOntoAxis( a, axis );
			pod::Vector2f bP = ::projectTriangleOntoAxis( b, axis );

			float overlap = std::min( aP.x, bP.x ) - std::max( aP.y, bP.y );
			if ( overlap < 0) return false; // separating axis
			if ( overlap < penetration ) {
				penetration = overlap;
				normal = axis;
			}
		}

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}

	bool triangleAabb( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps ) {
		const auto& aabb = body;

		auto closest = ::closestPointOnTriangle( ::getPosition( aabb ), tri );
		auto closestAabb = ::closestPointOnAABB( closest, aabb.bounds );

		if ( !uf::vector::isValid( closest ) ) return false;
		
		// to-do: derive proper delta
		auto delta = closestAabb - closest;
		float dist2 = uf::vector::dot( delta, delta );
		float tolerance = 1.0e-3;
		if ( dist2 >= tolerance ) return false;
		float dist = std::sqrt( dist2 );

		// to-do: properly derive the contact information
		auto contact = closest; // ( closest + closestAabb ) * 0.5f;
		auto normal = ( dist > eps ) ? ( delta / dist ) : ::triangleNormal( tri );
		float penetration = tolerance - dist;

	#if REORIENT_NORMALS_ON_CONTACT
		if ( uf::vector::dot( normal, delta ) < 0.0f ) normal = -normal;
	#endif

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool triangleSphere( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps ) {
		const auto& sphere = body;

		float r = sphere.collider.sphere.radius;
		auto center = ::getPosition( sphere );
		auto closest = ::closestPointOnTriangle( center, tri.points[0], tri.points[1], tri.points[2] );

		if ( !uf::vector::isValid( closest ) ) return false;

		// to-do: derive proper delta
		auto delta = center - closest;
		float dist2 = uf::vector::dot( delta, delta );
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