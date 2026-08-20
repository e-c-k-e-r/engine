#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase/gjk.h>
#include <uf/utils/math/physics/narrowphase/epa.h>

void impl::addOrRemoveBorder( uf::stl::vector<std::pair<pod::SupportPoint, pod::SupportPoint>>& edges, std::pair<pod::SupportPoint, pod::SupportPoint> e) {
	// look for reversed edge
	auto it = std::find_if( edges.begin(), edges.end(), [&]( auto& ex ) {
		return ( ex.first.p == e.second.p && ex.second.p == e.first.p );
	});

	if ( it != edges.end() ) edges.erase(it); // internal edge cancels out
	else edges.emplace_back(e); // keep border edge
}

bool impl::isValidSimplex( const pod::Simplex& s ) {
	if ( s.pts.size() < 4 ) return false;

	const auto& A = s.pts[0].p;
	const auto& B = s.pts[1].p;
	const auto& C = s.pts[2].p;
	const auto& D = s.pts[3].p;

	return fabs( uf::vector::dot( (B - A), uf::vector::cross(C - A, D - A) ) ) > EPS;
}

pod::Face impl::makeFace( const pod::SupportPoint& a, const pod::SupportPoint& b, const pod::SupportPoint& c ) {
	pod::Face face{ a, b, c };

	face.normal = uf::vector::normalize( uf::vector::cross( b.p - a.p, c.p - a.p ) );
	face.distance = uf::vector::dot( face.normal, a.p );

	if ( face.distance < 0 ) {
		std::swap( face.b, face.c );
		face.normal = -face.normal;
		face.distance = -face.distance;
	}
	return face;
};

void impl::getSupportFace( const pod::PhysicsBody& body, const pod::Vector3f& dir, pod::Vector3f outPoly[4], int& outCount ) {
	outCount = 0;
	const auto transform = impl::getTransform(body);
	pod::Vector3f localDir = uf::quaternion::rotate(uf::quaternion::inverse(transform.orientation), dir);

	switch (body.collider.type) {
		case pod::ShapeType::TRIANGLE: {
			outCount = 3;
			bool hasTransform = ( body.transform != nullptr );
			FOR_EACH(3, {
				outPoly[i] = hasTransform ? impl::apply( transform, body.collider.triangle.points[i] ) : body.collider.triangle.points[i];
			});
		} break;
		case pod::ShapeType::AABB: {
			outCount = 4;
			pod::Vector3f n = localDir;
			pod::Vector3f absN = uf::vector::abs( n );
			pod::Vector3f min = body.collider.aabb.min;
			pod::Vector3f max = body.collider.aabb.max;

			// dominant axis
			if ( absN.x > absN.y && absN.x > absN.z ) {
				float x = (n.x > 0) ? max.x : min.x;
				outPoly[0] = {x, min.y, max.z};
				outPoly[1] = {x, min.y, min.z};
				outPoly[2] = {x, max.y, min.z};
				outPoly[3] = {x, max.y, max.z};
				if ( n.x < 0 ) std::swap(outPoly[1], outPoly[3]);
			} else if ( absN.y > absN.z ) {
				float y = (n.y > 0) ? max.y : min.y;
				outPoly[0] = {max.x, y, max.z};
				outPoly[1] = {max.x, y, min.z};
				outPoly[2] = {min.x, y, min.z};
				outPoly[3] = {min.x, y, max.z};
				if ( n.y < 0 ) std::swap(outPoly[1], outPoly[3]);
			} else {
				float z = (n.z > 0) ? max.z : min.z;
				outPoly[0] = {min.x, max.y, z};
				outPoly[1] = {min.x, min.y, z};
				outPoly[2] = {max.x, min.y, z};
				outPoly[3] = {max.x, max.y, z};
				if (n.z < 0) std::swap(outPoly[1], outPoly[3]);
			}
			FOR_EACH(4, {
				outPoly[i] = impl::apply(transform, outPoly[i]);
			});
		} break;
		case pod::ShapeType::OBB: {
			outCount = 4;
			pod::Vector3f n = localDir;
			pod::Vector3f absN = uf::vector::abs( n );
			pod::Vector3f min = body.collider.obb.center - body.collider.obb.extent;
			pod::Vector3f max = body.collider.obb.center + body.collider.obb.extent;

			// dominant axis
			if ( absN.x > absN.y && absN.x > absN.z ) {
				float x = (n.x > 0) ? max.x : min.x;
				outPoly[0] = {x, min.y, max.z};
				outPoly[1] = {x, min.y, min.z};
				outPoly[2] = {x, max.y, min.z};
				outPoly[3] = {x, max.y, max.z};
				if ( n.x < 0 ) std::swap(outPoly[1], outPoly[3]);
			} else if ( absN.y > absN.z ) {
				float y = (n.y > 0) ? max.y : min.y;
				outPoly[0] = {max.x, y, max.z};
				outPoly[1] = {max.x, y, min.z};
				outPoly[2] = {min.x, y, min.z};
				outPoly[3] = {min.x, y, max.z};
				if ( n.y < 0 ) std::swap(outPoly[1], outPoly[3]);
			} else {
				float z = (n.z > 0) ? max.z : min.z;
				outPoly[0] = {min.x, max.y, z};
				outPoly[1] = {min.x, min.y, z};
				outPoly[2] = {max.x, min.y, z};
				outPoly[3] = {max.x, max.y, z};
				if (n.z < 0) std::swap(outPoly[1], outPoly[3]);
			}
			FOR_EACH(4, {
				outPoly[i] = impl::apply(transform, outPoly[i]);
			});
		} break;
		case pod::ShapeType::SPHERE: {
			outCount = 1;
			outPoly[0] = transform.position + uf::vector::normalize( dir ) * body.collider.sphere.radius;
		} break;
		case pod::ShapeType::CAPSULE: {
			auto up = uf::quaternion::rotate( transform.orientation, body.collider.capsule.up );
			auto p1 = transform.position + up;
			auto p2 = transform.position - up;

			if ( std::fabs( uf::vector::dot( dir, up ) ) < 0.01f ) {
				outCount = 2;
				pod::Vector3f offset = uf::vector::normalize( dir ) * body.collider.capsule.radius;
				outPoly[0] = p1 + offset;
				outPoly[1] = p2 + offset;
			} else {
				outCount = 1;
				auto end = ( uf::vector::dot( dir, p1 ) > uf::vector::dot( dir, p2 ) ) ? p1 : p2;
				outPoly[0] = end + uf::vector::normalize( dir ) * body.collider.capsule.radius;
			}
		} break;
		case pod::ShapeType::MESH:
		case pod::ShapeType::CONVEX_HULL: {
			if ( !body.collider.convexHull.mesh ) return;
			const auto& mesh = *body.collider.convexHull.mesh;
			auto selectedViewIdx = body.viewIndex;

			float bestDot = -FLT_MAX;
			pod::Triangle bestTri;

			for ( auto viewIdx = 0; viewIdx < mesh.buffer_views.size(); ++viewIdx ) {
				if ( 0 <= selectedViewIdx && selectedViewIdx != viewIdx ) continue;

				const auto& view = mesh.buffer_views[viewIdx];
				auto& indices = view["index"];
				auto& positions = view["position"];
				for ( size_t i = 0; i < view.index.count / 3; ++i ) {
					pod::Triangle tri = uf::mesh::fetchTriangle( view, indices, positions, i );
					pod::Vector3f normal = impl::triangleNormal( tri );
					float d = uf::vector::dot( normal, localDir );
					if ( d > bestDot ) {
						bestDot = d;
						bestTri = tri;
					}
				}
			}
			outCount = 3;
			FOR_EACH(3, {
				outPoly[i] = impl::apply(transform, bestTri.points[i]);
			});
		} break;
		// unsupported, fallback to single contact point
		default: {
			outCount = 0;
		} break;
	}
}

bool impl::generateClippingManifold( const pod::PhysicsBody& a, const pod::PhysicsBody& b, const pod::Contact& contact, pod::Manifold& manifold ) {
	if ( !uf::vector::isValid(contact.point) ) return false;

	auto normal = contact.normal;

	pod::Vector3f polyA[4];
	pod::Vector3f polyB[4];
	int countA = 0;
	int countB = 0;
	impl::getSupportFace(a, normal, polyA, countA);
	impl::getSupportFace(b, -normal, polyB, countB);

	if ( countA < 3 || countB < 3 ) {
		if ( manifold.points.empty() ) manifold.points.emplace_back(contact);
		return true;
	}

	pod::Vector3f normalA = uf::vector::normalize(uf::vector::cross(polyA[1] - polyA[0], polyA[2] - polyA[0]));
	pod::Vector3f normalB = uf::vector::normalize(uf::vector::cross(polyB[1] - polyB[0], polyB[2] - polyB[0]));

	float dotA = uf::vector::dot(normalA, normal);
	float dotB = uf::vector::dot(normalB, -normal);

	pod::Vector3f* refPoly;
	pod::Vector3f* incPoly;
	int refCount, incCount;
	pod::Vector3f refNormal;

	if ( dotA > dotB ) {
		refPoly = polyA; refCount = countA; refNormal = normalA;
		incPoly = polyB; incCount = countB;
	} else {
		refPoly = polyB; refCount = countB; refNormal = normalB;
		incPoly = polyA; incCount = countA;

		normal = -normal;
	}

	pod::Vector3f clipBuffer[12]; // incCount (<=4) grows by at most 1 per reference edge (<=4)
	int clipCount = incCount;
	for ( auto i = 0; i < incCount; ++i ) clipBuffer[i] = incPoly[i];

	for ( auto i = 0; i < refCount; ++i ) {
		pod::Vector3f edgeStart = refPoly[i];
		pod::Vector3f edgeEnd = refPoly[(i + 1) % refCount];
		pod::Vector3f edgeVector = edgeEnd - edgeStart;

		pod::Plane edgePlane;
		edgePlane.normal = uf::vector::normalize(uf::vector::cross(edgeVector, refNormal));
		edgePlane.offset = uf::vector::dot(edgePlane.normal, edgeStart);

		impl::clipPolygon( clipBuffer, clipCount, edgePlane );
		if ( clipCount == 0 ) break;
	}

	float refOffset = uf::vector::dot(refNormal, refPoly[0]);

	for (int i = 0; i < clipCount; ++i) {
		float distance = uf::vector::dot(clipBuffer[i], refNormal) - refOffset;
		// point is penetrating or touching
		if ( distance <= EPS ) {
			pod::Contact c;
			c.point = clipBuffer[i] - refNormal * (distance * 0.5f);
			c.normal = normal;
			c.penetration = -distance;
			manifold.points.emplace_back(c);
		}
	}

	if ( manifold.points.empty() ) manifold.points.emplace_back(contact);
	return true;
}

uf::stl::vector<pod::Face> impl::initialPolytope( const pod::Simplex& s ) {
	UF_ASSERT( s.pts.size() == 4 );

	const auto& A = s.pts[0];
	const auto& B = s.pts[1];
	const auto& C = s.pts[2];
	const auto& D = s.pts[3];

	return {
		impl::makeFace( A, B, C ),
		impl::makeFace( A, C, D ),
		impl::makeFace( A, D, B ),
		impl::makeFace( B, D, C )
	};
}

void impl::expandPolytope( uf::stl::vector<pod::Face>& faces, const pod::SupportPoint& p ) {
	uf::stl::vector<uint32_t> remove;
	uf::stl::vector<std::pair<pod::SupportPoint, pod::SupportPoint>> borders;
	
	remove.reserve( faces.size() );
	borders.reserve( faces.size() );

	// find faces visible to point
	for ( auto i = 0; i < faces.size(); ++i ) {
		if ( uf::vector::dot( faces[i].normal, p.p - faces[i].a.p ) > 0) {
			remove.emplace_back(i);
			impl::addOrRemoveBorder( borders, { faces[i].a, faces[i].b } );
			impl::addOrRemoveBorder( borders, { faces[i].b, faces[i].c } );
			impl::addOrRemoveBorder( borders, { faces[i].c, faces[i].a } );
		}
	}

	// remove visible faces
	for ( auto i = (int32_t) remove.size() - 1; i >= 0; --i ) {
		auto idx = remove[i];
		faces[idx] = faces.back();
		faces.pop_back();
	}

	// stitch new faces from border edges to new point
	for ( auto& e : borders ) {
		faces.emplace_back(impl::makeFace( e.first, e.second, p ));
	}
}

pod::Contact impl::epa( const pod::PhysicsBody& a, const pod::PhysicsBody& b, const pod::Simplex& simplex, uint32_t maxIterations ) {
	UF_ASSERT( impl::isValidSimplex(simplex) );

	auto faces = impl::initialPolytope(simplex);
	if ( faces.empty() ) return {};

	for ( auto it = 0; it < maxIterations; ++it ) {
		// find closest face
		int32_t idx = -1;
		float minDist = FLT_MAX;
		for ( auto i = 0; i < faces.size(); ++i ) {
			if ( faces[i].distance < minDist ) {
				minDist = faces[i].distance;
				idx = i;
			}
		}
		
		UF_ASSERT( idx != -1 );
		auto& f = faces[idx];

		// new support
		auto sp = impl::supportMinkowskiDetailed( a, b, f.normal );
		float d = uf::vector::dot( sp.p, f.normal );

		// convergence check
		if ( fabs(d - f.distance) < EPS || it == maxIterations - 1 ) {
			// project origin onto triangle (in Minkowski space)
			auto bary = impl::computeBarycentric({}, f.a.p, f.b.p, f.c.p, true);

			// use barycentrics to blend real-body points
			auto pA = ( f.a.pA * bary.x ) + ( f.b.pA * bary.y ) + ( f.c.pA * bary.z );
			auto pB = ( f.a.pB * bary.x ) + ( f.b.pB * bary.y ) + ( f.c.pB * bary.z );

			auto contact = ( pA + pB ) * 0.5f;
			auto normal = f.normal;
			float penetration = uf::vector::dot( (pB - pA), normal );

			// flip normal
			if ( penetration < 0.0f ) {
				f.normal = -f.normal;
				penetration = -penetration;
			}

			return { contact, normal, penetration };
		}

		impl::expandPolytope(faces, sp);
	}
	return {};
}