#pragma once

#include "structs.h"

namespace impl {
	struct Vertex {
		pod::Vector3f position;
		pod::Vector4f color;

		static uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};

	/*FORCE_INLINE*/ void addLine( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color = { 1, 1, 1, 1 } );
	/*FORCE_INLINE*/ void addTransientLine( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color = { 1, 1, 1, 1 }, const pod::PhysicsBody* a = NULL, const pod::PhysicsBody* b = NULL );

	void drawManifold( const pod::Manifold& manifold );
	void drawBody( const pod::PhysicsBody& body );
	void draw( const pod::World& world, float dt = 0 );
}