#pragma once

#include "structs.h"
#include "draw.h"

// to-do: organize this mess
namespace impl {
	size_t makePairKey( const pod::PhysicsBody& a, const pod::PhysicsBody& b );
	void wakeBody( pod::PhysicsBody& body );
	void sleepBody( pod::PhysicsBody& body );
	void updateActivity( pod::PhysicsBody& body, float dt );

	pod::Transform<> getTransform( const pod::PhysicsBody& body );
	pod::Vector3f getPosition( const pod::PhysicsBody& body, bool useTransform = false );
	pod::Vector3f apply( const pod::Transform<>& t, const pod::Vector3f& p );
	pod::Vector3f applyInverse( const pod::Transform<>& t, const pod::Vector3f& p );

	pod::PhysicsBody physicsBodyHullView( const pod::PhysicsBody& body, int32_t index = -1 );
	pod::PhysicsBody physicsBodyTriView( const pod::TriangleWithNormal triangle, const pod::PhysicsBody& body = {} );
	pod::PhysicsBody physicsBodyTriView( const pod::PhysicsBody& body, size_t triID );
	
	bool shouldCollide( const pod::Collider& a, const pod::Collider& b );
	bool shouldCollide( const pod::PhysicsBody& a, const pod::PhysicsBody& b );

	uf::stl::string similarMass( const pod::PhysicsBody& );
	
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
	bool testSeparatingAxis( const pod::Triangle& triangle, const pod::OBB& box, const pod::Vector3f& axis, const pod::Vector3f axes[3], float& outMinOverlap, pod::Vector3f& outBestAxis );
	bool testSeparatingAxis( const pod::OBB& boxA, const pod::OBB& boxB, const pod::Vector3f axesA[3], const pod::Vector3f axesB[3], const pod::Vector3f& axis, float& outMinOverlap, pod::Vector3f& outBestAxis );
	void clipPolygon( pod::Vector3f* poly, int& polyCount, const pod::Plane& plane );
	void clipPolygon( pod::Vector3f* poly, int& polyCount, const pod::AABB& plane );
	pod::Vector3f triangleCenter( const pod::Triangle& tri );
	pod::Vector3f triangleNormal( const pod::Triangle& tri );
	pod::Vector3f triangleNormal( const pod::TriangleWithNormal& tri );
	pod::TriangleWithNormal fetchTriangle( const uf::Mesh& mesh, size_t triID, const pod::PhysicsBody& body );

	float getMaterialTransmittance( const uf::stl::string& materialName );
	uf::stl::string getMaterialName( const pod::PhysicsBody& body, uint32_t triID );

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
	/*FORCE_INLINE*/ pod::Vector3f obbMin( const pod::OBB& obb );
	/*FORCE_INLINE*/ pod::Vector3f obbMax( const pod::OBB& obb );
	/*FORCE_INLINE*/ pod::OBB aabbToObb( const pod::AABB& aabb );
	/*FORCE_INLINE*/ pod::AABB obbToAabb( const pod::OBB& obb );
	/*FORCE_INLINE*/ void boxAxes( pod::Vector3f axes[3] );
	/*FORCE_INLINE*/ void boxAxes( pod::Vector3f axes[3], const pod::Transform<>& transform );
	/*FORCE_INLINE*/ pod::Vector3f extentFromAxes( const pod::OBB& box, const pod::Vector3f axes[3] );
	/*FORCE_INLINE*/ float projectExtents( const pod::OBB& box, const pod::Vector3f& normal, const pod::Vector3f axes[3] );

	void getCorners( const pod::AABB& aabb, pod::Vector3f corners[8] );
	void getCorners( const pod::AABB& aabb, const pod::Transform<>& transform, pod::Vector3f corners[8] );
	pod::AABB transformAabbToWorld( const pod::AABB& localBox, const pod::Transform<>& transform );
	std::pair<pod::Vector3f, pod::Vector3f> getCapsuleSegment( const pod::PhysicsBody& body );
	pod::AABB computeAABB( const pod::PhysicsBody& body );
	pod::AABB transformAabbToLocal( const pod::AABB& box, const pod::Transform<>& transform );

	/*FORCE_INLINE*/ bool aabbOverlap( const pod::qAABB& a, const pod::qAABB& b );
	/*FORCE_INLINE*/ pod::qAABB quantizeAABB( const pod::AABB& box, const pod::AABB& root, const pod::Vector3f& invScale );
	/*FORCE_INLINE*/ pod::AABB dequantizeAABB( const pod::qAABB& qbox, const pod::AABB& root );
	/*FORCE_INLINE*/ pod::AABB dequantizeAABB( const pod::qAABB& qbox, const pod::AABB& root, const pod::Vector3f& scale );
	/*FORCE_INLINE*/ pod::Vector3f computeDequantizeScale( const pod::AABB& root );
	/*FORCE_INLINE*/ pod::Vector3f computeQuantizeScale( const pod::AABB& root );

}