#pragma once

#include "../impl.h"

namespace impl {
	pod::Vector3f support( const pod::PhysicsBody& body, const pod::Vector3f& dir );
	pod::Vector3f supportMinkowski( const pod::PhysicsBody& A, const pod::PhysicsBody& B, const pod::Vector3f& dir );
	pod::SupportPoint supportMinkowskiDetailed( const pod::PhysicsBody& A, const pod::PhysicsBody& B, const pod::Vector3f& dir );
	bool updateSimplex( pod::Simplex& s, pod::Vector3f& dir );
	bool isDegenerate( const pod::Simplex& s, const pod::SupportPoint& newPt );
	pod::Vector3f closestPointOnSimplex( const pod::Vector3f& x, pod::Vector3f* simplex, int& sCount );

	bool gjk( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Simplex& simplex, int maxIterations = 20 );
	bool gjk( const pod::Ray& ray, const pod::PhysicsBody& body, float maxDist, float& outT, pod::Vector3f& outNormal );
}