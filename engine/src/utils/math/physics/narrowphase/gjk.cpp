#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase/gjk.h>

pod::Vector3f impl::support( const pod::PhysicsBody& body, const pod::Vector3f& dir ) {
	const auto transform = impl::getTransform( body );
	switch ( body.collider.type ) {
		case pod::ShapeType::AABB: {
			return {
				( dir.x >= 0.0f ) ? body.bounds.max.x : body.bounds.min.x,
				( dir.y >= 0.0f ) ? body.bounds.max.y : body.bounds.min.y,
				( dir.z >= 0.0f ) ? body.bounds.max.z : body.bounds.min.z
			};
		} break;
		case pod::ShapeType::OBB: {
			auto box = impl::obbToAabb( body.collider.obb );
			pod::Vector3f localDir = uf::quaternion::rotate( uf::quaternion::inverse(transform.orientation), dir );
			pod::Vector3f localPt = {
				( localDir.x >= 0.0f ) ? box.max.x : box.min.x,
				( localDir.y >= 0.0f ) ? box.max.y : box.min.y,
				( localDir.z >= 0.0f ) ? box.max.z : box.min.z
			};
			return impl::apply( transform, localPt );
		} break;
		case pod::ShapeType::SPHERE: {
			return transform.position + uf::vector::normalize( dir ) * body.collider.sphere.radius;
		} break;
		case pod::ShapeType::PLANE: {
			const auto& plane = body.collider.plane;
			pod::Vector3f n = plane.normal;
			float d = plane.offset;

			pod::Vector3f basePoint = n * d;
			float dot = uf::vector::dot( dir, n );
			if ( std::fabs(dot) > 0.9999f ) return basePoint;

			pod::Vector3f tangent = uf::vector::normalize( dir - (n * dot) );
			return basePoint + tangent * 100000.0f;
		} break;
		case pod::ShapeType::CAPSULE: {
			auto up = uf::quaternion::rotate( transform.orientation, body.collider.capsule.up );
			auto p1 = transform.position + up;
			auto p2 = transform.position - up;
			auto end = ( uf::vector::dot( dir, p1 ) > uf::vector::dot( dir, p2 ) ) ? p1 : p2; // get closest end
			return end + uf::vector::normalize( dir ) * body.collider.capsule.radius;
		}
		case pod::ShapeType::TRIANGLE: {
			const auto& tri = body.collider.triangle;
			float d0 = uf::vector::dot( tri.points[0], dir );
			float d1 = uf::vector::dot( tri.points[1], dir );
			float d2 = uf::vector::dot( tri.points[2], dir );

			if ( d0 > d1 && d0 > d2 ) return tri.points[0];
			if ( d1 > d2 ) return tri.points[1];
			return tri.points[2];
		} break;
		case pod::ShapeType::MESH:
		case pod::ShapeType::CONVEX_HULL: {
			const auto transform = impl::getTransform( body );
			const auto& mesh = *body.collider.convexHull.mesh;
			auto selectedViewIdx = body.viewIndex;

			pod::Vector3f localDir = uf::quaternion::rotate( uf::quaternion::inverse(transform.orientation), dir );
			float maxDist = -FLT_MAX;
			pod::Vector3f furthestVertex = {};

			for ( auto viewIdx = 0; viewIdx < mesh.buffer_views.size(); ++viewIdx ) {
				if ( 0 <= selectedViewIdx && selectedViewIdx != viewIdx ) continue; // cringe, but saves code duplication (could just alter the bounds above)
				const auto& view = mesh.buffer_views[viewIdx];
				auto& positions = view["position"_hash];
				for ( size_t i = 0; i < view.vertex.count; ++i ) {
					pod::Vector3f v = uf::mesh::fetchVertex( view, positions, i );
					float dist = uf::vector::dot( v, localDir );
					if ( dist > maxDist ) {
						maxDist = dist;
						furthestVertex = v;
					}
				}
			}

			return impl::apply( transform, furthestVertex );
		} break;

		default: {
			UF_EXCEPTION("unsupported shape: {}", (int)body.collider.type);
		} break;
	}
	return {};
}

pod::Vector3f impl::supportMinkowski( const pod::PhysicsBody& A, const pod::PhysicsBody& B, const pod::Vector3f& dir ) {
	return impl::support( A, dir ) - impl::support( B, -dir );
}

pod::SupportPoint impl::supportMinkowskiDetailed( const pod::PhysicsBody& A, const pod::PhysicsBody& B, const pod::Vector3f& dir ) {
	auto pA = impl::support( A, dir );
	auto pB = impl::support( B, -dir );
	return { pA - pB, pA, pB };
}

bool impl::updateSimplex( pod::Simplex& s, pod::Vector3f& dir ) {
	switch (s.pts.size()) {
		case 2: {
			// points
			auto& A = s.pts.back();	// newest
			auto& B = s.pts.front();
			// edges
			auto AB = B.p - A.p;
			auto AO = -A.p;

			if (uf::vector::dot(AB, AO) > 0) {
				dir = uf::vector::cross( uf::vector::cross( AB, AO ), AB );
			} else {
				s.pts = {A};
				dir = AO;
			}
			return false;
		}
		case 3: {
			// points
			auto& A = s.pts[2]; // newest
			auto& B = s.pts[1];
			auto& C = s.pts[0];
			
			// edges
			auto AB = B.p - A.p;
			auto AC = C.p - A.p;
			auto AO = -A.p;

			auto ABC = uf::vector::cross( AB, AC );
			if ( uf::vector::dot( uf::vector::cross( AB, ABC ), AO ) > 0 ) {
				s.pts = { A, B };
				return impl::updateSimplex( s, dir );
			}
			if ( uf::vector::dot( uf::vector::cross( ABC, AC ), AO ) > 0 ) {
				s.pts = { A, C };
				return impl::updateSimplex( s, dir );
			}

			if ( uf::vector::dot( ABC, AO ) > 0 ) dir = ABC;
			else { s.pts = { A, C, B }; dir = -ABC; }
			return false;
		}
		case 4: {
			// points
			auto& A = s.pts[3]; // newest
			auto& B = s.pts[2];
			auto& C = s.pts[1];
			auto& D = s.pts[0];
			// line to origin
			auto AO = -A.p;
			// faces
			auto ABC = uf::vector::cross( B.p - A.p, C.p - A.p );
			if ( uf::vector::dot( ABC, AO ) > 0 ) {
				s.pts = { A, B, C };
				return impl::updateSimplex( s, dir );
			}

			auto ACD = uf::vector::cross( C.p - A.p, D.p - A.p );
			if ( uf::vector::dot( ACD, AO ) > 0 ) {
				s.pts = { A, C, D };
				return impl::updateSimplex( s, dir );
			}

			auto ADB = uf::vector::cross( D.p - A.p, B.p - A.p );
			if ( uf::vector::dot( ADB, AO ) > 0 ) {
				s.pts = { A, D, B };
				return impl::updateSimplex( s, dir );
			}

			// origin inside tetrahedron
			return true;
		}
	}
	return false;
}

bool impl::isDegenerate( const pod::Simplex& s, const pod::SupportPoint& newPt ) {
	// compare to existing
	for ( auto& sp : s.pts ) {
		if ( uf::vector::magnitude( sp.p - newPt.p ) < EPS2 ) return true; // duplicate point
	}
	// if simplex already has 3 points, check area
	if ( s.pts.size() >= 2 ) {
		auto& A = s.pts[0].p;
		auto& B = s.pts[1].p;
		auto& C = newPt.p;
		
		float area = uf::vector::magnitude( uf::vector::cross( B - A, C - A ) );
		if ( area < EPS2 ) return true; // collinear with previous
	}
	if ( s.pts.size() >= 3 ) {
		auto& A = s.pts[0].p;
		auto& B = s.pts[1].p;
		auto& C = s.pts[2].p;
		auto& D = newPt.p;

		float volume = fabs( uf::vector::dot( D - A, uf::vector::cross( B - A, C - A ) ) );
		if ( volume < EPS ) return true; // coplanar with triangle
	}
	return false;
}

bool impl::gjk( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Simplex& simplex, int maxIterations ) {
	auto dir = impl::getPosition( b ) - impl::getPosition( a );
	if ( uf::vector::magnitude( dir ) < EPS2 ) dir = {1,0,0}; // fallback direction

	// initial condition
	simplex.pts.clear();
	simplex.pts.emplace_back(impl::supportMinkowskiDetailed( a, b, dir ));
	dir = -simplex.pts[0].p;

	for ( auto it = 0; it < maxIterations; ++it ) {
		auto newPt = impl::supportMinkowskiDetailed( a, b, dir );
		if ( uf::vector::dot( newPt.p, dir ) < 0 ) return false; // didn't pass origin, no collision
		// would invalidate the simplex
		if ( impl::isDegenerate( simplex, newPt ) ) {
			// nudge direction with a small orthogonal component
			if ( fabs(dir.x) < fabs(dir.y) && fabs(dir.x) < fabs(dir.z) ) dir = uf::vector::normalize( pod::Vector3f{1,0,0} + dir * 0.01f );
			else if ( fabs(dir.y) < fabs(dir.z) ) dir = uf::vector::normalize( pod::Vector3f{0,1,0} + dir * 0.01f );
			else dir = uf::vector::normalize( pod::Vector3f{0,0,1} + dir * 0.01f );
			continue; // try again
		}
		// add new point to simplex
		simplex.pts.emplace_back( newPt );
		// update
		if ( impl::updateSimplex(simplex, dir) ) return true; // simplex contains origin, finished
	}

	return false;
}

// GJK ray-cast 
pod::Vector3f impl::closestPointOnSimplex( const pod::Vector3f& x, pod::Vector3f* simplex, int& sCount ) {
	if ( sCount == 1 ) {
		return simplex[0];
	} else if ( sCount == 2 ) {
		auto p = impl::closestPointOnSegment( x, simplex[0], simplex[1] );
		return p;
	} else if ( sCount == 3 ) {
		auto p = impl::closestPointOnTriangle( x, simplex[0], simplex[1], simplex[2] );
		return p;
	}

	return x;
}

bool impl::gjk( const pod::Ray& ray, const pod::PhysicsBody& body, float maxDist, float& outT, pod::Vector3f& outNormal ) {
	float t = 0.0f;
	pod::Vector3f x = ray.origin;
	pod::Vector3f n = {0, 0, 0};

	pod::Vector3f supportPoint = impl::support( body, -ray.direction );
	pod::Vector3f v = x - supportPoint;

	pod::Vector3f simplex[4];
	int sCount = 0;

	for ( int iter = 0; iter < 32; ++iter ) {
		float vSq = uf::vector::dot( v, v );
		// origin was inside the shape to begin with, or we perfectly converged
		if ( vSq < EPS * EPS ) break;

		pod::Vector3f dir = -v;
		pod::Vector3f p = impl::support( body, dir );
		pod::Vector3f w = x - p;

		if ( uf::vector::dot( v, w ) > 0.0f ) {
			float vDotD = uf::vector::dot( v, ray.direction );
			if ( vDotD >= 0.0f ) return false; // miss

			float dt = uf::vector::dot( v, w ) / vDotD;
			t = t - dt;

			if ( t > maxDist ) return false;

			x = ray.origin + ray.direction * t;
			n = uf::vector::normalize( v );

			// reset
			sCount = 0;
		}

		simplex[sCount++] = p;

		pod::Vector3f closest = impl::closestPointOnSimplex( x, simplex, sCount );
		v = x - closest;

		if ( sCount == 4 ) break; // collided
	}

	outT = t;
	outNormal = n;
	return true;
}