#include "craeture.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/gl/camera/camera.h>
#include <uf/gl/mesh/skeletal/mesh.h>

namespace {
	bool lockMouse = true;
}

void ext::Craeture::initialize() {	
	ext::Object::initialize();
	
	this->addComponent<pod::Physics>();
	this->addComponent<pod::Transform<>>();
	this->addComponent<uf::Camera>(); 

	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	transform = uf::transform::initialize(transform); {
		this->getComponent<uf::Camera>().getTransform().reference = this->getComponentPointer<pod::Transform<>>();
	}


	pod::Physics& physics = this->getComponent<pod::Physics>();
	physics.linear.velocity = {0,0,0};
	physics.linear.acceleration = {0,-9.81,0};
	physics.rotational.velocity = uf::quaternion::axisAngle( {0,1,0}, (pod::Math::num_t) 0 );
	physics.rotational.acceleration = {0,0,0,0};

	uf::Serializer& serializer = this->getComponent<uf::Serializer>(); {
		for( auto it = serializer["animation"]["names"].begin() ; it != serializer["animation"]["names"].end() ; ++it ) { 
			const std::string& name = it->asString();
			if ( this->m_animation.transforms.find(name) == this->m_animation.transforms.end() ) this->m_animation.transforms[name];
		}

		/* Gravity */ {
			if ( serializer["collision"]["gravity"] != Json::nullValue ) {
				physics.linear.acceleration.x = serializer["collision"]["gravity"][0].asFloat();
				physics.linear.acceleration.y = serializer["collision"]["gravity"][1].asFloat();
				physics.linear.acceleration.z = serializer["collision"]["gravity"][2].asFloat();
			}
		}
	}
	/* Collider */ {
		uf::CollisionBody& collider = this->getComponent<uf::CollisionBody>();
	}
}
void ext::Craeture::tick() {
	ext::Object::tick();

	uf::Serializer& serializer = this->getComponent<uf::Serializer>();
	for ( auto it = serializer["animation"]["status"].begin(); it != serializer["animation"]["status"].end(); ++it ) {
		if ( it.key() == "rest" ) continue;
		if ( it->asBool() ) serializer["animation"]["status"]["rest"] = false;
	}
	this->animate();

	/* Collision */ if ( this->m_parent && serializer["collision"]["should"].asBool() ) {
		uf::Entity& parent = this->getParent();
		if ( this->hasComponent<uf::CollisionBody>() && parent.hasComponent<uf::CollisionBody>() ) {
			uf::CollisionBody& collider = this->getComponent<uf::CollisionBody>();
			uf::CollisionBody& pCollider = parent.getComponent<uf::CollisionBody>();

			pod::Transform<>& transform = this->getComponent<pod::Transform<>>(); {
				collider.clear();
				uf::Collider* box = new uf::AABBox( uf::vector::add({0, 1.5, 0}, transform.position), {0.7, 1.6, 0.7} );
				collider.add(box);
			}

			pod::Physics& physics = this->getComponent<pod::Physics>();
			auto result = pCollider.intersects(collider);
			uf::Collider::Manifold strongest;
			strongest.depth = 0.001;
			bool useStrongest = true;
			for ( auto manifold : result ) {
				if ( manifold.colliding && manifold.depth > 0 ) {
					if ( strongest.depth < manifold.depth ) strongest = manifold;
					if ( !useStrongest ) {
						pod::Vector3 correction = uf::vector::normalize(manifold.normal) * -(manifold.depth * manifold.depth * 1.001);
						transform.position += correction;
						if ( manifold.normal.x == 1 || manifold.normal.x == -1 ) physics.linear.velocity.x = 0;
						if ( manifold.normal.y == 1 || manifold.normal.y == -1 ) physics.linear.velocity.y = 0;
						if ( manifold.normal.z == 1 || manifold.normal.z == -1 ) physics.linear.velocity.z = 0;
					}
				}
			}
			if ( useStrongest && strongest.colliding ) {
				pod::Vector3 correction = uf::vector::normalize(strongest.normal) * -(strongest.depth * strongest.depth * 1.001);
				transform.position += correction;

			//	std::cout << "Collision! " << ( strongest.colliding ? "yes" : "no" ) << " " << strongest.normal.x << ", " << strongest.normal.y << ", " << strongest.normal.z << " / " << strongest.depth << std::endl;
				if ( strongest.normal.x == 1 || strongest.normal.x == -1 ) physics.linear.velocity.x = 0;
				if ( strongest.normal.y == 1 || strongest.normal.y == -1 ) physics.linear.velocity.y = 0;
				if ( strongest.normal.z == 1 || strongest.normal.z == -1 ) physics.linear.velocity.z = 0;
			}
		}
	}
}
void ext::Craeture::render() {
	if ( !this->m_parent ) return;
	ext::Object::render();
}

namespace {
	void animate( uf::Skeleton& skeleton, uf::Skeleton::Bone& bone, const pod::Matrix4& parent, int indent = 0 ) {
		bone.matrix = parent * bone.inverse * bone.animation * bone.offset;
		for ( uint child : bone.children ) animate( skeleton, skeleton.getBone( child ), bone.matrix, indent + 1 );
	}
}

void ext::Craeture::animate() {
	if ( !this->hasComponent<uf::SkeletalMesh>() ) return;

	uf::SkeletalMesh& mesh = this->getComponent<uf::SkeletalMesh>();
	uf::Skeleton& skeleton = mesh.getSkeleton();
	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	auto& metadata = this->getComponent<uf::Serializer>()["animation"];

	if ( metadata["status"]["rest"].asBool() ) {
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		{
			pod::Transform<>& leftShoulder = this->m_animation.transforms[metadata["names"]["leftShoulder"].asString()];
			if ( metadata["states"]["leftShoulder"]["delta"] == Json::nullValue ) {
				metadata["states"]["leftShoulder"]["delta"] = 0;

				pod::Transform<>& leftShoulder = this->m_animation.transforms[metadata["names"]["leftShoulder"].asString()];
				uf::transform::rotate( leftShoulder, {metadata["states"]["leftShoulder"]["rest"]["axis"][0].asFloat(), metadata["states"]["leftShoulder"]["rest"]["axis"][1].asFloat(), metadata["states"]["leftShoulder"]["rest"]["axis"][2].asFloat() }, metadata["states"]["leftShoulder"]["rest"]["angle"].asFloat() );
			} else {
				float limit = metadata["states"]["leftShoulder"]["limit"].asFloat();
				float current = metadata["states"]["leftShoulder"]["delta"].asFloat();
				float direction = fabs(metadata["states"]["leftShoulder"]["direction"].asFloat()); if ( current > 0 ) direction *= -1;
				float delta = uf::physics::time::delta * direction;

				if ( (direction < 0 && current + delta > 0) || (direction > 0 && current + delta < 0) ) {
					uf::transform::rotate( leftShoulder, { 1, 0, 0 }, delta );
					metadata["states"]["leftShoulder"]["delta"] = current + delta;
				}
			}
		}
		{
			pod::Transform<>& rightShoulder = this->m_animation.transforms[metadata["names"]["rightShoulder"].asString()];
			if ( metadata["states"]["rightShoulder"]["delta"] == Json::nullValue ) {
				metadata["states"]["rightShoulder"]["delta"] = 0;

				pod::Transform<>& rightShoulder = this->m_animation.transforms[metadata["names"]["rightShoulder"].asString()];
				uf::transform::rotate( rightShoulder, {metadata["states"]["rightShoulder"]["rest"]["axis"][0].asFloat(), metadata["states"]["rightShoulder"]["rest"]["axis"][1].asFloat(), metadata["states"]["rightShoulder"]["rest"]["axis"][2].asFloat() }, metadata["states"]["rightShoulder"]["rest"]["angle"].asFloat() );
			} else {
				float limit = metadata["states"]["rightShoulder"]["limit"].asFloat();
				float current = metadata["states"]["rightShoulder"]["delta"].asFloat();
				float direction = fabs(metadata["states"]["rightShoulder"]["direction"].asFloat()); if ( current > 0 ) direction *= -1;
				float delta = uf::physics::time::delta * direction;
				
				if ( (direction < 0 && current + delta > 0) || (direction > 0 && current + delta < 0) ) {
					uf::transform::rotate( rightShoulder, { 1, 0, 0 }, delta );
					metadata["states"]["rightShoulder"]["delta"] = current + delta;
				}
			}
		}
		{
			pod::Transform<>& leftLeg = this->m_animation.transforms[metadata["names"]["leftLeg"].asString()];
			if ( metadata["states"]["leftLeg"]["delta"] == Json::nullValue ) {
				metadata["states"]["leftLeg"]["delta"] = 0;

				pod::Transform<>& leftLeg = this->m_animation.transforms[metadata["names"]["leftLeg"].asString()];
				uf::transform::rotate( leftLeg, {metadata["states"]["leftLeg"]["rest"]["axis"][0].asFloat(), metadata["states"]["leftLeg"]["rest"]["axis"][1].asFloat(), metadata["states"]["leftLeg"]["rest"]["axis"][2].asFloat() }, metadata["states"]["leftLeg"]["rest"]["angle"].asFloat() );
			} else {
				float limit = metadata["states"]["leftLeg"]["limit"].asFloat();
				float current = metadata["states"]["leftLeg"]["delta"].asFloat();
				float direction = fabs(metadata["states"]["leftLeg"]["direction"].asFloat()); if ( current > 0 ) direction *= -1;
				float delta = uf::physics::time::delta * direction;

				if ( (direction < 0 && current + delta > 0) || (direction > 0 && current + delta < 0) ) {
					uf::transform::rotate( leftLeg, { 0, 0, 1 }, delta );
					metadata["states"]["leftLeg"]["delta"] = current + delta;
				}
			}
		}
		{
			pod::Transform<>& rightLeg = this->m_animation.transforms[metadata["names"]["rightLeg"].asString()];
			if ( metadata["states"]["rightLeg"]["delta"] == Json::nullValue ) {
				metadata["states"]["rightLeg"]["delta"] = 0;

				pod::Transform<>& rightLeg = this->m_animation.transforms[metadata["names"]["rightLeg"].asString()];
				uf::transform::rotate( rightLeg, {metadata["states"]["rightLeg"]["rest"]["axis"][0].asFloat(), metadata["states"]["rightLeg"]["rest"]["axis"][1].asFloat(), metadata["states"]["rightLeg"]["rest"]["axis"][2].asFloat() }, metadata["states"]["rightLeg"]["rest"]["angle"].asFloat() );
			} else {
				float limit = metadata["states"]["rightLeg"]["limit"].asFloat();
				float current = metadata["states"]["rightLeg"]["delta"].asFloat();
				float direction = fabs(metadata["states"]["rightLeg"]["direction"].asFloat()); if ( current > 0 ) direction *= -1;
				float delta = uf::physics::time::delta * direction;
				
				if ( (direction < 0 && current + delta > 0) || (direction > 0 && current + delta < 0) ) {
					uf::transform::rotate( rightLeg, { 0, 0, 1 }, delta );
					metadata["states"]["rightLeg"]["delta"] = current + delta;
				}
			}
		}
	}
	if ( metadata["status"]["walk"].asBool() ) {
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		/* Right Shoulder */ {
			pod::Transform<>& rightShoulder = this->m_animation.transforms[metadata["names"]["rightShoulder"].asString()];
			float direction = metadata["states"]["rightShoulder"]["direction"].asFloat();
			float limit = metadata["states"]["rightShoulder"]["limit"].asFloat();
			float current = metadata["states"]["rightShoulder"]["delta"].asFloat();
			float delta = uf::physics::time::delta * direction;

			uf::transform::rotate( rightShoulder, { 1, 0, 0 }, delta );
			metadata["states"]["rightShoulder"]["delta"] = (delta = current + delta); {
				if ( ( delta > limit && direction > 0 ) || ( delta < -limit && direction < 0 ) )
					metadata["states"]["rightShoulder"]["direction"] = metadata["states"]["rightShoulder"]["direction"].asFloat() * -1;
			}
		}
		/* Left Shoulder */ {
			pod::Transform<>& leftShoulder = this->m_animation.transforms[metadata["names"]["leftShoulder"].asString()];
			float direction = metadata["states"]["leftShoulder"]["direction"].asFloat();
			float limit = metadata["states"]["leftShoulder"]["limit"].asFloat();
			float current = metadata["states"]["leftShoulder"]["delta"].asFloat();
			float delta = uf::physics::time::delta * direction;

			uf::transform::rotate( leftShoulder, { 1, 0, 0 }, delta );
			metadata["states"]["leftShoulder"]["delta"] = (delta = current + delta); {
				if ( ( delta > limit && direction > 0 ) || ( delta < -limit && direction < 0 ) )
					metadata["states"]["leftShoulder"]["direction"] = metadata["states"]["leftShoulder"]["direction"].asFloat() * -1;
			}
		}
	}
	if ( metadata["status"]["walk"].asBool() || metadata["status"]["turn"].asBool() ) {
		/* Right Leg */ {
			pod::Transform<>& rightLeg = this->m_animation.transforms[metadata["names"]["rightLeg"].asString()];
			float direction = metadata["states"]["rightLeg"]["direction"].asFloat();
			float limit = metadata["states"]["rightLeg"]["limit"].asFloat();
			float current = metadata["states"]["rightLeg"]["delta"].asFloat();
			float delta = uf::physics::time::delta * direction;

			uf::transform::rotate( rightLeg, { 0, 0, 1 }, delta );
			metadata["states"]["rightLeg"]["delta"] = (delta = current + delta); {
				if ( ( delta > limit && direction > 0 ) || ( delta < -limit && direction < 0 ) )
					metadata["states"]["rightLeg"]["direction"] = metadata["states"]["rightLeg"]["direction"].asFloat() * -1;
			}
		}
		/* Left Leg */ {
			pod::Transform<>& leftLeg = this->m_animation.transforms[metadata["names"]["leftLeg"].asString()];
			float direction = metadata["states"]["leftLeg"]["direction"].asFloat();
			float limit = metadata["states"]["leftLeg"]["limit"].asFloat();
			float current = metadata["states"]["leftLeg"]["delta"].asFloat();
			float delta = uf::physics::time::delta * direction;

			uf::transform::rotate( leftLeg, { 0, 0, 1 }, delta );
			metadata["states"]["leftLeg"]["delta"] = (delta = current + delta); {
				if ( ( delta > limit && direction > 0 ) || ( delta < -limit && direction < 0 ) )
					metadata["states"]["leftLeg"]["direction"] = metadata["states"]["leftLeg"]["direction"].asFloat() * -1;
			}
		}	
	}

	for ( auto it = this->m_animation.transforms.begin(); it != this->m_animation.transforms.end(); ++it ) {
		std::string name = it->first;
		pod::Transform<> transform = it->second;
		if ( skeleton.hasBone( name ) ) {
			uf::Skeleton::Bone& bone = skeleton.getBone( name );
			bone.animation = uf::quaternion::matrix(transform.orientation);
		}
	}
	for ( auto& bone : skeleton.getBones() ) if ( bone.index == bone.parent ) ::animate( skeleton, bone, uf::matrix::identity() );
}