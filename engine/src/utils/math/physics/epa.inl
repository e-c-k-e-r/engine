namespace {
	void addOrRemoveBorder( uf::stl::vector<std::pair<pod::SupportPoint, pod::SupportPoint>>& edges, std::pair<pod::SupportPoint, pod::SupportPoint> e) {
		// look for reversed edge
		auto it = std::find_if( edges.begin(), edges.end(), [&]( auto& ex ) {
			return ( ex.first.p == e.second.p && ex.second.p == e.first.p );
		});

		if ( it != edges.end() ) edges.erase(it); // internal edge cancels out
		else edges.emplace_back(e); // keep border edge
	}

	bool isValidSimplex( const pod::Simplex& s ) {
		if ( s.pts.size() < 4 ) return false;

		const auto& A = s.pts[0].p;
		const auto& B = s.pts[1].p;
		const auto& C = s.pts[2].p;
		const auto& D = s.pts[3].p;

		return fabs( uf::vector::dot( (B - A), uf::vector::cross(C - A, D - A) ) ) > EPS;
	}
	
	pod::Face makeFace( const pod::SupportPoint& a, const pod::SupportPoint& b, const pod::SupportPoint& c ) {
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

	uf::stl::vector<pod::Face> initialPolytope( const pod::Simplex& s ) {
		UF_ASSERT( s.pts.size() == 4 );

		const auto& A = s.pts[0];
		const auto& B = s.pts[1];
		const auto& C = s.pts[2];
		const auto& D = s.pts[3];

		return {
			::makeFace( A, B, C ),
			::makeFace( A, C, D ),
			::makeFace( A, D, B ),
			::makeFace( B, D, C )
		};
	}

	void expandPolytope( uf::stl::vector<pod::Face>& faces, const pod::SupportPoint& p ) {
		uf::stl::vector<uint32_t> remove;
		uf::stl::vector<std::pair<pod::SupportPoint, pod::SupportPoint>> borders;
		
		remove.reserve( faces.size() );
		borders.reserve( faces.size() );

		// find faces visible to point
		for ( auto i = 0; i < faces.size(); ++i ) {
			if ( uf::vector::dot( faces[i].normal, p.p - faces[i].a.p ) > 0) {
				remove.emplace_back(i);
				::addOrRemoveBorder( borders, { faces[i].a, faces[i].b } );
				::addOrRemoveBorder( borders, { faces[i].b, faces[i].c } );
				::addOrRemoveBorder( borders, { faces[i].c, faces[i].a } );
			}
		}

		// remove visible faces
		for (  auto i = (int32_t) remove.size() - 1; i >= 0; --i ) {
			auto idx = remove[i];
			faces[idx] = faces.back();
			faces.pop_back();
		}

		// stitch new faces from border edges to new point
		for ( auto& e : borders ) {
			faces.emplace_back(::makeFace( e.first, e.second, p ));
		}
	}

	pod::Contact epa( const pod::PhysicsBody& a, const pod::PhysicsBody& b, const pod::Simplex& simplex, uint32_t maxIterations, float eps ) {
		UF_ASSERT( ::isValidSimplex(simplex) );

		auto faces = ::initialPolytope(simplex);
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
			auto sp = ::supportMinkowskiDetailed( a, b, f.normal );
			float d = uf::vector::dot( sp.p, f.normal );

			// convergence check
			if ( fabs(d - f.distance) < eps || it == maxIterations - 1 ) {
				// project origin onto triangle (in Minkowski space)
				auto bary = ::computeBarycentric({}, f.a.p, f.b.p, f.c.p, true);

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

			::expandPolytope(faces, sp);
		}
		return {};
	}
}