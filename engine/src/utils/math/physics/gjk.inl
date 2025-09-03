namespace {
	pod::Vector3f support( const pod::PhysicsBody& body, const pod::Vector3f& dir ) {
		const auto transform = ::getTransform( body );
		switch ( body.collider.type ) {
			case pod::ShapeType::SPHERE: {
				return transform.position + uf::vector::normalize( dir ) * body.collider.u.sphere.radius;
			} break;
			case pod::ShapeType::AABB: {
				return {
					( dir.x >= 0.0f ) ? body.bounds.max.x : body.bounds.min.x,
					( dir.y >= 0.0f ) ? body.bounds.max.y : body.bounds.min.y,
					( dir.z >= 0.0f ) ? body.bounds.max.z : body.bounds.min.z
				};
			} break;
			case pod::ShapeType::CAPSULE: {
				auto up = uf::quaternion::rotate( transform.orientation, pod::Vector3f{0,1,0} );
				auto p1 = transform.position + up * body.collider.u.capsule.halfHeight;
				auto p2 = transform.position - up * body.collider.u.capsule.halfHeight;
				auto end = ( uf::vector::dot( dir, p1 ) > uf::vector::dot( dir, p2 ) ) ? p1 : p2; // get closest end
				return end + uf::vector::normalize( dir ) * body.collider.u.capsule.radius;
			}
			case pod::ShapeType::TRIANGLE: {
				const auto& tri = body.collider.u.triangle;
				float d0 = uf::vector::dot( tri.points[0], dir );
				float d1 = uf::vector::dot( tri.points[1], dir );
				float d2 = uf::vector::dot( tri.points[2], dir );

				if ( d0 > d1 && d0 > d2 ) return tri.points[0];
				if ( d1 > d2 ) return tri.points[1];
				return tri.points[2];
			} break;

			default: {
			} break;
		}
		return {};
	}

	pod::Vector3f supportMinkowski( const pod::PhysicsBody& A, const pod::PhysicsBody& B, const pod::Vector3f& dir ) {
		return ::support( A, dir ) - ::support( B, -dir );
	}

	pod::SupportPoint supportMinkowskiDetailed( const pod::PhysicsBody& A, const pod::PhysicsBody& B, const pod::Vector3f& dir ) {
		auto pA = ::support( A, dir );
		auto pB = ::support( B, -dir );
		return { pA - pB, pA, pB };
	}

	bool updateSimplex( pod::Simplex& s, pod::Vector3f& dir ) {
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
					return ::updateSimplex( s, dir );
				}
				if ( uf::vector::dot( uf::vector::cross( ABC, AC ), AO ) > 0 ) {
					s.pts = { A, C };
					return ::updateSimplex( s, dir );
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
					return ::updateSimplex( s, dir );
				}

				auto ACD = uf::vector::cross( C.p - A.p, D.p - A.p );
				if ( uf::vector::dot( ACD, AO ) > 0 ) {
					s.pts = { A, C, D };
					return ::updateSimplex( s, dir );
				}

				auto ADB = uf::vector::cross( D.p - A.p, B.p - A.p );
				if ( uf::vector::dot( ADB, AO ) > 0 ) {
					s.pts = { A, D, B };
					return ::updateSimplex( s, dir );
				}

				// origin inside tetrahedron
				return true;
			}
		}
		return false;
	}

	bool isDegenerate( const pod::Simplex& s, const pod::SupportPoint& newPt, float eps = EPS(1.0e-6f) ) {
		// compare to existing
		for ( auto& sp : s.pts ) {
			if ( uf::vector::magnitude( sp.p - newPt.p ) < eps ) return true; // duplicate point
		}
		// if simplex already has 3 points, check area
		if ( s.pts.size() >= 2 ) {
			auto& A = s.pts[0].p;
			auto& B = s.pts[1].p;
			auto& C = newPt.p;
			
			float area = uf::vector::magnitude( uf::vector::cross( B - A, C - A ) );
			if ( area < eps ) return true; // collinear with previous
		}
		if ( s.pts.size() >= 3 ) {
			auto& A = s.pts[0].p;
			auto& B = s.pts[1].p;
			auto& C = s.pts[2].p;
			auto& D = newPt.p;

			float volume = fabs( uf::vector::dot( D - A, uf::vector::cross( B - A, C - A ) ) );
			if ( volume < eps ) return true; // coplanar with triangle
		}
		return false;
	}

	bool gjk( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Simplex& simplex, int maxIterations = 20, float eps = EPS(1e-6f) ) {
		auto dir = ::getPosition( b ) - ::getPosition( a );
		if ( uf::vector::magnitude(dir) < eps ) dir = {1,0,0}; // fallback direction

		// initial condition
		simplex.pts.clear();
		simplex.pts.emplace_back(::supportMinkowskiDetailed( a, b, dir ));
		dir = -simplex.pts[0].p;

		for ( auto it = 0; it < maxIterations; ++it ) {
			auto newPt = ::supportMinkowskiDetailed( a, b, dir );
			if ( uf::vector::dot( newPt.p, dir ) < 0 ) return false; // didn't pass origin, no collision
			// would invalidate the simplex
			if ( ::isDegenerate( simplex, newPt ) ) {
			#if 1
				// nudge direction with a small orthogonal component
				if ( fabs(dir.x) < fabs(dir.y) && fabs(dir.x) < fabs(dir.z) ) dir = uf::vector::normalize( pod::Vector3f{1,0,0} + dir * 0.01f );
				else if ( fabs(dir.y) < fabs(dir.z) ) dir = uf::vector::normalize( pod::Vector3f{0,1,0} + dir * 0.01f );
				else dir = uf::vector::normalize( pod::Vector3f{0,0,1} + dir * 0.01f );
			#else
				// choose an alternate probe
				static pod::Vector3f fallbackDirs[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
				dir = fallbackDirs[ (it + simplex.pts.size()) % 3 ];
			#endif
				continue; // try again
			}
			// add new point to simplex
			simplex.pts.emplace_back( newPt );
			// update
			if ( ::updateSimplex(simplex, dir) ) return true; // simplex contains origin, finished
		}

	#if 1
		return false;
	#else
		// if overlap detected but simplex ended at triangle, fix it, as EPA requires a tetrahedron:
		if ( simplex.pts.size() == 3 ) {
			// points
			auto& A0 = simplex.pts[0].p;
			auto& B0 = simplex.pts[1].p;
			auto& C0 = simplex.pts[2].p;
			// triangle normal
			auto normal = uf::vector::normalize( uf::vector::cross( B0 - A0, C0 - A0 ) );

			// try support in +normal
			auto extra = ::supportMinkowskiDetailed( a, b, normal );
			float vol = fabs( uf::vector::dot( extra.p - A0, uf::vector::cross( B0 - A0, C0 - A0 ) ) );
			if ( vol < eps ) extra = ::supportMinkowskiDetailed( a, b, -normal ); // if still coplanar, try -normal
			simplex.pts.emplace_back(extra); // force tetrahedron
		}

		return !simplex.pts.empty();
	#endif
	}
}