#pragma once

#include "../structs.h"

namespace impl {
	void addOrRemoveBorder( uf::stl::vector<std::pair<pod::SupportPoint, pod::SupportPoint>>& edges, std::pair<pod::SupportPoint, pod::SupportPoint> e);
	bool isValidSimplex( const pod::Simplex& s );
	pod::Face makeFace( const pod::SupportPoint& a, const pod::SupportPoint& b, const pod::SupportPoint& c );
	void getSupportFace( const pod::PhysicsBody& body, const pod::Vector3f& dir, pod::Vector3f outPoly[4], int& outCount );
	bool generateClippingManifold( const pod::PhysicsBody& a, const pod::PhysicsBody& b, const pod::Contact& contact, pod::Manifold& manifold );
	uf::stl::vector<pod::Face> initialPolytope( const pod::Simplex& s );
	void expandPolytope( uf::stl::vector<pod::Face>& faces, const pod::SupportPoint& p );

	pod::Contact epa( const pod::PhysicsBody& a, const pod::PhysicsBody& b, const pod::Simplex& simplex, uint32_t maxIterations = 64 );
}