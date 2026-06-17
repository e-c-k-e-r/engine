#include "./input/behavior.h"
#include "./movement/behavior.h"
#include "./interaction/behavior.h"
#include "./camera/behavior.h"
#include "./model/behavior.h"

namespace ext {
	class Player : public uf::Object {
	};
}

UF_OBJECT_REGISTER_BEGIN(ext::Player)
	UF_OBJECT_BIND_BEHAVIOR(ext::PlayerInputBehavior)
	UF_OBJECT_BIND_BEHAVIOR(ext::PlayerMovementBehavior)
	UF_OBJECT_BIND_BEHAVIOR(ext::PlayerInteractionBehavior)
	UF_OBJECT_BIND_BEHAVIOR(ext::PlayerCameraBehavior)
UF_OBJECT_REGISTER_END()