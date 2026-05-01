#pragma once

#include "impl.h"

// to-do: organize this mess
namespace impl {
	uint64_t makePairKey( const pod::PhysicsBody& a, const pod::PhysicsBody& b );
	void wakeBody( pod::PhysicsBody& body );
	void sleepBody( pod::PhysicsBody& body );
	void updateActivity( pod::PhysicsBody& body, float dt );

	pod::Transform<> getTransform( const pod::PhysicsBody& body );
	pod::Vector3f getPosition( const pod::PhysicsBody& body, bool useTransform = false );
	pod::PhysicsBody physicsBodyHullView( const pod::PhysicsBody& body, int32_t index = -1 );
	pod::PhysicsBody physicsBodyTriView( const pod::PhysicsBody& body, const pod::TriangleWithNormal triangle );
	pod::PhysicsBody physicsBodyTriView( const pod::PhysicsBody& body, size_t triID );
	bool shouldCollide( const pod::Collider& a, const pod::Collider& b );
	bool shouldCollide( const pod::PhysicsBody& a, const pod::PhysicsBody& b );
	pod::Matrix3f computeWorldInverseInertia( const pod::PhysicsBody& b );
	pod::Vector3f normalizeDelta( const pod::Vector3f& delta, float dist, const pod::Vector3f& fallback = pod::Vector3f{0,1,0} );
	pod::Vector3f computeTangent( const pod::Vector3f& normal );
	pod::Vector3f closestPointOnSegment( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b );
	std::pair<pod::Vector3f, pod::Vector3f> closestSegmentSegment( const pod::Vector3f& A, const pod::Vector3f& B, const pod::Vector3f& C, const pod::Vector3f& D );
	pod::Vector3f closestPointSegmentAabb( const pod::Vector3f& p1, const pod::Vector3f& p2, const pod::AABB& box );
	pod::Vector3f computeBarycentric( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c, bool clamps = false );
	pod::Vector3f computeBarycentric(const pod::Vector3f& p, const pod::Triangle& tri, bool clamps = false );
	pod::Vector3f interpolateWithBarycentric( const pod::Vector3f& bary, const pod::Vector3f a, const pod::Vector3f b, const pod::Vector3f c );
	pod::Vector3f interpolateWithBarycentric( const pod::Vector3f& bary, const pod::Vector3f points[3] );
	bool pointInTriangle( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c );
	bool pointInTriangle( const pod::Vector3f& p, const pod::Triangle& tri );
	pod::Vector3f closestPointOnTriangle( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c );
	pod::Vector3f closestPointOnTriangle( const pod::Vector3f& p, const pod::Triangle& tri );
	pod::Vector3f orientNormalToAB( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Vector3f n );
	float segmentTriangleDistanceSq( const pod::Vector3f& p0, const pod::Vector3f& p1, const pod::Triangle& tri, pod::Vector3f& outSeg, pod::Vector3f& outTri );
	int clipPolygonAgainstPlane( const pod::Vector3f* inPoly, int inCount, const pod::Vector3f& planeNormal, float planeOffset, pod::Vector3f* outPoly );
	pod::Vector3f triangleCenter( const pod::Triangle& tri );
	pod::Vector3f triangleNormal( const pod::Triangle& tri );
	pod::Vector3f triangleNormal( const pod::TriangleWithNormal& tri );
	bool triangleTriangleIntersect( const pod::Triangle& a, const pod::Triangle& b );
	size_t getIndex( const void* pointer, size_t stride, size_t index ); 
	size_t getIndex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, size_t index ); 
	pod::Vector3f getVertex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& positions, size_t index );
	pod::Triangle fetchTriangle( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, const uf::Mesh::AttributeView& positions, size_t triID );
	/*FORCE_INLINE*/ pod::Triangle fetchTriangle( const uf::Mesh::View& view, size_t triID );
	pod::TriangleWithNormal fetchTriangle( const uf::Mesh& mesh, size_t triID );
	pod::TriangleWithNormal fetchTriangle( const uf::Mesh& mesh, size_t triID, const pod::PhysicsBody& body );
	bool computeTriangleTriangleSegment( const pod::TriangleWithNormal& A, const pod::TriangleWithNormal& B, pod::Vector3f& p0, pod::Vector3f& p1 );
	pod::Vector2f projectTriangleOntoAxis( const pod::TriangleWithNormal& tri, const pod::Vector3f& axis );

	/*FORCE_INLINE*/ bool aabbOverlap( const pod::AABB& a, const pod::AABB& b );
	/*FORCE_INLINE*/ float aabbSurfaceArea( const pod::AABB& aabb );
	/*FORCE_INLINE*/ pod::AABB computeSegmentAABB( const pod::Vector3f& p1, const pod::Vector3f p2, float r );
	/*FORCE_INLINE*/ pod::Vector3f closestPointOnAABB( const pod::Vector3f& p, const pod::AABB& box );
	/*FORCE_INLINE*/ pod::AABB computeTriangleAABB( const pod::Triangle& tri );
	pod::AABB computeConvexHullAABB( const uf::Mesh::View& view, const uf::Mesh::AttributeView& positions, pod::AABB bounds = { {  FLT_MAX,  FLT_MAX,  FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } }  );
	/*FORCE_INLINE*/ pod::AABB computeConvexHullAABB( const uf::Mesh::View& view, pod::AABB bounds = { {  FLT_MAX,  FLT_MAX,  FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } }  );
	/*FORCE_INLINE*/ pod::AABB mergeAabb( const pod::AABB& a, const pod::AABB& b );
	/*FORCE_INLINE*/ pod::Vector3f aabbCenter( const pod::AABB& aabb );
	/*FORCE_INLINE*/ pod::Vector3f aabbExtent( const pod::AABB& aabb );
	pod::AABB transformAabbToWorld( const pod::AABB& localBox, const pod::Transform<>& transform );
	std::pair<pod::Vector3f, pod::Vector3f> getCapsuleSegment( const pod::PhysicsBody& body );
	pod::AABB computeAABB( const pod::PhysicsBody& body );
	float triAabbDistanceSq( const pod::Triangle& tri, const pod::AABB& box );
	bool triAabbOverlap( const pod::Triangle& tri, const pod::AABB& box );
	pod::AABB transformAabbToLocal( const pod::AABB& box, const pod::Transform<>& transform );
}