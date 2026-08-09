#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>
#include <uf/utils/math/physics/broadphase/bvh.h>

bool impl::rayTriangleIntersect( const pod::Ray& ray, const pod::Triangle& tri, float& t, float& u, float& v ) {
	auto edge1 = tri.points[1] - tri.points[0];
	auto edge2 = tri.points[2] - tri.points[0];
	auto h = uf::vector::cross( ray.direction, edge2 );
	float a = uf::vector::dot( edge1, h );
	
	if ( fabs(a) < EPS ) return false;
	
	float f = 1.0f / a;
	auto s = ray.origin - tri.points[0];
	u = f * uf::vector::dot( s, h );
	if ( u < 0.0f || u > 1.0f ) return false;
	
	auto q = uf::vector::cross( s, edge1 );
	v = f * uf::vector::dot( ray.direction, q );
	if (v < 0.0f || u + v > 1.0f) return false;
	
	t = f * uf::vector::dot( edge2, q );
	return ( t > EPS );
}

bool impl::rayAabbIntersect( const pod::Ray& ray, const pod::AABB& box, float& tMin, float& tMax ) {
	tMin = 0.0f;
	tMax = FLT_MAX;

	for ( auto i = 0; i < 3; ++i ) {
		float minB = box.min[i];
		float maxB = box.max[i];

		// inflate degenerate slab
		if ( fabs(maxB - minB) < 1e-6f) {
			maxB += 1e-6f;
			minB -= 1e-6f;
		}

		float invD = 1.0f / ray.direction[i];
		float t0 = ( minB - ray.origin[i] ) * invD;
		float t1 = ( maxB - ray.origin[i] ) * invD;

		if ( invD < 0.0f ) std::swap(t0, t1);

		tMin = std::max(tMin, t0);
		tMax = std::min(tMax, t1);

		if ( tMax <= tMin ) return false;
	}
	return true;
}

bool impl::rayAabb( const pod::Ray& ray, const pod::PhysicsBody& body, pod::RayQuery& rayHit ) {
	float tMin = 0.0f;
	float tMax = FLT_MAX;

	if ( !impl::rayAabbIntersect( ray, body.bounds, tMin, tMax ) ) return false;

	if ( tMin < rayHit.contact.penetration ) {
		rayHit.hit = true;
		rayHit.body = &body;
		rayHit.contact.point = ray.origin + ray.direction * tMin;
		rayHit.contact.penetration = tMin;

		auto local = rayHit.contact.point - (body.bounds.min + body.bounds.max) * 0.5f;
		auto extents = (body.bounds.max - body.bounds.min) * 0.5f;
		auto absLocal = uf::vector::abs( local );

		if ( absLocal.x > absLocal.y && absLocal.x > absLocal.z ) {
			rayHit.contact.normal = { (local.x > 0 ? 1.0f : -1.0f), 0, 0 };
		} else if ( absLocal.y > absLocal.z ) {
			rayHit.contact.normal = { 0, (local.y > 0 ? 1.0f : -1.0f), 0 };
		} else {
			rayHit.contact.normal = { 0, 0, (local.z > 0 ? 1.0f : -1.0f) };
		}
	}
	return true;
}
bool impl::rayObb( const pod::Ray& ray, const pod::PhysicsBody& body, pod::RayQuery& rayHit ) {
	auto tA = impl::getTransform( body );

	pod::Ray localRay;
	localRay.origin = impl::applyInverse( tA, ray.origin );
	localRay.direction = uf::quaternion::rotate( uf::quaternion::inverse(tA.orientation), ray.direction );

	auto box = impl::obbToAabb( body.collider.obb );

	float tMin, tMax;
	if ( !impl::rayAabbIntersect( localRay, box, tMin, tMax ) ) return false;

	if ( tMin < rayHit.contact.penetration ) {
		rayHit.hit = true;
		rayHit.body = &body;
		rayHit.contact.penetration = tMin;

		rayHit.contact.point = ray.origin + ray.direction * tMin;

		auto localPoint = localRay.origin + localRay.direction * tMin;
		auto localDelta = localPoint - body.collider.obb.center;
		auto absDelta = uf::vector::abs( localDelta / body.collider.obb.extent );

		pod::Vector3f localNormal = {0,0,0};
		if ( absDelta.x > absDelta.y && absDelta.x > absDelta.z ) localNormal.x = localDelta.x > 0 ? 1.0f : -1.0f;
		else if ( absDelta.y > absDelta.z ) localNormal.y = localDelta.y > 0 ? 1.0f : -1.0f;
		else localNormal.z = localDelta.z > 0 ? 1.0f : -1.0f;

		rayHit.contact.normal = uf::quaternion::rotate( tA.orientation, localNormal );
	}
	return true;
}
bool impl::raySphere( const pod::Ray& ray, const pod::PhysicsBody& body, pod::RayQuery& rayHit ) {
	auto center = impl::getPosition(body);
	float r = body.collider.sphere.radius;

	// vector from sphere center to ray origin
	auto oc = ray.origin - center;

	float a = uf::vector::dot( ray.direction, ray.direction );
	float b = 2.0f * uf::vector::dot( oc, ray.direction );
	float c = uf::vector::dot( oc, oc ) - r*r;

	float disc = b*b - 4*a*c;

	if ( disc < 0 ) return false;

	float sqrtDisc = std::sqrt(disc);
	float t0 = (-b - sqrtDisc) / (2*a);
	float t1 = (-b + sqrtDisc) / (2*a);

	float t = ( t0 >= 0 ) ? t0 : t1;

	if ( t < 0 ) return false; // both behind ray

	// compare against current best hit
	if ( t >= rayHit.contact.penetration ) return false;

	// record hit
	rayHit.hit = true;
	rayHit.body = &body;
	rayHit.contact.point = ray.origin + ray.direction*t;
	rayHit.contact.normal = uf::vector::normalize( rayHit.contact.point - center );
	rayHit.contact.penetration = t;
	return true;
}
bool impl::rayPlane( const pod::Ray& ray, const pod::PhysicsBody& body, pod::RayQuery& rayHit ) {
	auto& normal = body.collider.plane.normal;
	float offset = body.collider.plane.offset;

	float denom = uf::vector::dot( normal, ray.direction );
	if ( fabs(denom) < EPS ) return false; // parallel

	float t = (offset - uf::vector::dot( normal, ray.origin )) / denom;
	if ( t < 0.0f ) return false; // behind ray start
	if ( t >= rayHit.contact.penetration ) return false; // existing hit is closer

	// record hit
	rayHit.hit = true;
	rayHit.body = &body;
	rayHit.contact.point = ray.origin + ray.direction * t;
	rayHit.contact.normal = normal * ( denom < 0.0f ? 1.0f : -1.0f ); // face ray
	rayHit.contact.penetration = t;
	return true;
}
bool impl::rayCapsule( const pod::Ray& ray, const pod::PhysicsBody& body, pod::RayQuery& rayHit ) {
	auto [ p1, p2 ] = impl::getCapsuleSegment( body );
	float r = body.collider.capsule.radius;

	auto d = p2 - p1;			// segment direction
	auto m = ray.origin - p1;
	auto n = ray.direction;

	float md = uf::vector::dot(m, d);
	float nd = uf::vector::dot(n, d);
	float dd = uf::vector::dot(d, d);

	// coeffs for intersection with infinite cylinder
	float a = dd - nd*nd;
	float k = uf::vector::dot(m, m) - r*r;
	float c = dd - md*md;

	float b = dd * uf::vector::dot(m,n) - nd*md;
	float discr = b*b - a*c;
	if ( fabs(a) < EPS || discr < 0.0f ) return false;

	float t = (-b - std::sqrt(discr)) / a;  // nearer hit

	if ( t < 0 || t > rayHit.contact.penetration ) return false; // clipped by range

	// check hit actually lies within segment caps
	float y = md + t*nd;
	if ( y < 0.0f || y > dd ) {
		// check against spherical caps instead
		float tSphere = FLT_MAX;
		bool hit = false;

		for ( auto center : {p1, p2} ) {
			auto oc = ray.origin - center;
			float b = uf::vector::dot(oc, n);
			float c = uf::vector::dot(oc, oc) - r*r;
			float disc = b*b - c;
			if ( disc >= 0 ) {
				float tTmp = -b - std::sqrt(disc);
				if ( tTmp >= 0 && tTmp < tSphere ) {
					tSphere = tTmp;
					hit = true;
				}
			}
		}
		if ( !hit || tSphere > rayHit.contact.penetration ) return false;

		t = tSphere;
	}

	// register hit
	if ( t < rayHit.contact.penetration ) {
		rayHit.hit = true;
		rayHit.body = &body;
		rayHit.contact.point = ray.origin + n * t;
		rayHit.contact.penetration = t;

		// normal: from capsule axis to hit point
		auto closest = impl::closestPointOnSegment( rayHit.contact.point, p1, p2 );
		auto normal = uf::vector::normalize( rayHit.contact.point - closest );
		rayHit.contact.normal = normal;
	}
	return true;
}

bool impl::rayMesh( const pod::Ray& r, const pod::PhysicsBody& body, pod::RayQuery& rayHit ) {
	const auto& bvh  = *body.collider.mesh.bvh;
	const auto& meshData = *body.collider.mesh.mesh;

	const auto transform = impl::getTransform( body );

	pod::Ray ray;
	ray.origin	= impl::applyInverse( transform, r.origin );
	ray.direction = uf::quaternion::rotate( uf::quaternion::inverse( transform.orientation ), r.direction );

	thread_local uf::stl::vector<pod::BVH::index_t> candidates;
	candidates.clear();
	impl::queryBVH( bvh, ray, candidates );

	for ( auto packedID : candidates ) {
		uint32_t viewID = pod::BVH::unpackView(packedID);
		uint32_t triID  = pod::BVH::unpackTri(packedID);
		auto tri = uf::mesh::fetchTriangle( meshData, triID );

		float t, u, v;
		if ( !impl::rayTriangleIntersect( ray, tri, t, u, v ) ) continue;
		if ( t >= rayHit.contact.penetration ) continue;

		auto l = ray.origin + ray.direction * t;
		auto n = impl::triangleNormal( tri );

		// push back to world
		auto p = impl::apply( transform, l);
		n = uf::quaternion::rotate( transform.orientation, n );

		rayHit.hit = true;
		rayHit.body = &body;
		rayHit.contact.point = p;
		rayHit.contact.normal = n;
		rayHit.contact.penetration = t;
		rayHit.contact.featureA = triID;
	}

	return rayHit.hit;
}

bool impl::rayHull( const pod::Ray& r, const pod::PhysicsBody& body, pod::RayQuery& rayHit ) {
	const auto& bvh  = *body.collider.convexHull.bvh;
	const auto& meshData = *body.collider.convexHull.mesh;

	const auto transform = impl::getTransform( body );

	pod::Ray ray;
	ray.origin	= impl::applyInverse( transform, r.origin );
	ray.direction = uf::quaternion::rotate( uf::quaternion::inverse( transform.orientation ), r.direction );

	thread_local uf::stl::vector<pod::BVH::index_t> candidates;
	candidates.clear();
	impl::queryBVH( bvh, ray, candidates );

	for ( auto hullID : candidates ) {
		auto hullView = impl::physicsBodyHullView( body, hullID );

		float t;
		pod::Vector3f normal;

		if ( !impl::gjk( ray, hullView, rayHit.contact.penetration, t, normal ) ) continue;
		if ( t >= rayHit.contact.penetration ) continue;

		rayHit.hit = true;
		rayHit.body = &body;

		rayHit.contact.point = impl::apply( transform, ray.origin + ray.direction * t );
		rayHit.contact.normal = uf::quaternion::rotate( transform.orientation, normal );
		rayHit.contact.penetration = t;
		rayHit.contact.featureA = hullID;
	}

	return rayHit.hit;
}

void impl::drawRay( const pod::Ray& ray, const pod::RayQuery& query ) {
	auto& start = ray.origin;
	auto  end = ray.origin + ray.direction * query.contact.penetration;

	uf::debug::addLine( start, end, query.hit ? pod::Vector4f{ 0, 1, 0, 1 } : pod::Vector4f{ 1, 0, 0, 1 } );
}

pod::RayQuery uf::physics::rayCast( const pod::Ray& ray, const pod::PhysicsBody& body, float maxDistance, uint32_t mask ) {
	return rayCast( ray, body.world ? *body.world : uf::physics::getWorld(), &body, maxDistance );
}
pod::RayQuery uf::physics::rayCast( const pod::Ray& ray, const pod::World& world, float maxDistance, uint32_t mask ) {
	return rayCast( ray, world, NULL, maxDistance );
}
pod::RayQuery uf::physics::rayCast( const pod::Ray& ray, const pod::World& world, const pod::PhysicsBody* body, float maxDistance, uint32_t mask ) {
	pod::RayQuery rayHit;
	rayHit.invoker = body;
	rayHit.contact.penetration = maxDistance;

	auto& dynamicBvh = world.dynamicBvh;
	auto& staticBvh = world.staticBvh;
	auto& bodies = world.bodies;

	STATIC_THREAD_LOCAL(uf::stl::vector<pod::BVH::index_t>, candidates);
	impl::queryBVH( dynamicBvh, ray, candidates );
	if ( uf::physics::settings.useSplitBvhs ) impl::queryBVH( staticBvh, ray, candidates );


	for ( auto i : candidates ) {
		auto* b = bodies[i];
		
		if ( body == b ) continue;

		if ( !(b->collider.category & mask) ) continue;

		switch ( b->collider.type ) {
			case pod::ShapeType::AABB: impl::rayAabb( ray, *b, rayHit ); break;
			case pod::ShapeType::OBB: impl::rayObb( ray, *b, rayHit ); break;
			case pod::ShapeType::SPHERE: impl::raySphere( ray, *b, rayHit ); break;
			case pod::ShapeType::PLANE: impl::rayPlane( ray, *b, rayHit ); break;
			case pod::ShapeType::CAPSULE: impl::rayCapsule( ray, *b, rayHit ); break;
			case pod::ShapeType::MESH: impl::rayMesh( ray, *b, rayHit ); break;
			case pod::ShapeType::CONVEX_HULL: impl::rayHull( ray, *b, rayHit ); break;
		}
	}
	
	if ( uf::physics::settings.debugDraw.rays ) impl::drawRay( ray, rayHit );

	return rayHit;
}

float uf::physics::occlusion( const pod::Vector3f& to, const pod::Vector3f& from ) {
	pod::Vector3f dir = from - to;
	float mag = uf::vector::magnitude( dir );
	if ( mag <= EPS ) return 1.0f;
	float dist = std::sqrt( mag );
	dir /= dist;

	pod::Ray ray;
	ray.origin = from;
	ray.direction = dir;

	pod::RayQuery hit = uf::physics::rayCast( ray, uf::physics::getWorld(), dist, pod::Collider::MASK_PHYSICAL );

	if ( hit.contact.penetration >= dist ) return 1.0f;

	auto materialName = uf::physics::getRayMaterialName( hit );
	return impl::getMaterialTransmittance( materialName );
}
uf::stl::string uf::physics::getRayMaterialName( const pod::RayQuery& query ) {
	return impl::getMaterialName( *query.body, query.contact.featureA );
}

pod::AcousticBounce uf::physics::acousticReflection( const pod::Vector3f& sourcePos, const pod::Vector3f& rayDirection, const pod::Vector3f& listenerPos, float maxDistance ) {
	pod::AcousticBounce result;

	pod::Ray primaryRay;
	primaryRay.origin = sourcePos;
	primaryRay.direction = rayDirection;

	pod::RayQuery firstHit = uf::physics::rayCast( primaryRay, uf::physics::getWorld(), maxDistance, pod::Collider::MASK_PHYSICAL );

	if ( firstHit.contact.penetration >= maxDistance ) return result;

	auto materialName = uf::physics::getRayMaterialName( firstHit );
	float transmittance = impl::getMaterialTransmittance( materialName );

	float reflectance = 1.0f - transmittance;

	pod::Vector3f N = firstHit.contact.normal;
	pod::Vector3f D = rayDirection;

	float dotProduct = uf::vector::dot( D, N );
	pod::Vector3f reflectDir = D - (N * (2.0f * dotProduct));
	uf::vector::normalize( reflectDir );

	pod::Vector3f bounceOrigin = firstHit.contact.point + (N * EPS);
	pod::Vector3f toListener = listenerPos - bounceOrigin;
	float distToListener = uf::vector::magnitude( toListener );

	pod::Ray secondaryRay;
	secondaryRay.origin = bounceOrigin;
	secondaryRay.direction = toListener / distToListener;

	pod::RayQuery secondHit = uf::physics::rayCast( secondaryRay, uf::physics::getWorld(), distToListener, pod::Collider::MASK_PHYSICAL );

	if ( secondHit.contact.penetration >= distToListener ) {
		result.valid = true;
		result.totalDistance = firstHit.contact.penetration + distToListener;
		result.retainedEnergy = reflectance;
		result.hitPoint = firstHit.contact.point;
	}

	return result;
}