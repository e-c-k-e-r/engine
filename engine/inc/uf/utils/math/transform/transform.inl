template<typename T> pod::Transform<T>& /*UF_API*/ uf::transform::initialize( pod::Transform<T>& transform ) {
	transform.up = {0, 1, 0};
	transform.right = {1, 0, 0};
	transform.forward = {0, 0, 1};
	transform.orientation = {0, 0, 0, 1};
	transform.model = uf::matrix::identity();
	transform.reference = NULL;
	return transform;
}
template<typename T> pod::Transform<T> /*UF_API*/ uf::transform::initialize() {
	pod::Transform<T> transform;
	return transform = uf::transform::initialize(transform);
}
template<typename T> pod::Transform<T>& /*UF_API*/ uf::transform::lookAt( pod::Transform<T>& transform, const pod::Vector3t<T>& at ) {
	pod::Vector3t<T> forward = uf::vector::normalize( at - transform.position );
	pod::Vector3t<T> right = uf::vector::normalize(uf::vector::cross( forward, transform.up ));
	pod::Vector3t<T> up = uf::vector::normalize(uf::vector::cross(at, right));

	transform.up = up;
	transform.right = right;
	transform.forward = forward;

	transform.orientation = uf::quaternion::lookAt( at, up );
	return transform;
}
template<typename T> pod::Transform<T>& /*UF_API*/ uf::transform::move( pod::Transform<T>& transform, const pod::Vector3t<T>& axis, pod::Math::num_t delta ) {
	transform.position += (axis * delta);
	return transform;
}
template<typename T> pod::Transform<T>& /*UF_API*/ uf::transform::move( pod::Transform<T>& transform, const pod::Vector3t<T>& delta ) {
	transform.position += delta;
	return transform;
}
template<typename T> pod::Transform<T>& /*UF_API*/ uf::transform::reorient( pod::Transform<T>& transform ) {
/*
	transform.up = uf::vector::normalize(uf::quaternion::rotate(transform.orientation, pod::Vector3{0.0f, 1.0f, 0.0f}));
	transform.right = uf::vector::normalize(uf::quaternion::rotate(transform.orientation, pod::Vector3{1.0f, 0.0f, 0.0f}));
	transform.forward = uf::vector::normalize(uf::quaternion::rotate(transform.orientation, pod::Vector3{0.0f, 0.0f, 1.0f}));
*/
	pod::Quaternion<T> q = transform.orientation;
	transform.forward = { 2 * (q.x * q.z + q.w * q.y),  2 * (q.y * q.x - q.w * q.x), 1 - 2 * (q.x * q.x + q.y * q.y) };
	transform.up = { 2 * (q.x * q.y - q.w * q.z), 1 - 2 * (q.x * q.x + q.z * q.z), 2 * (q.y * q.z + q.w * q.x)};
	transform.right = { 1 - 2 * (q.y * q.y + q.z * q.z), 2 * (q.x * q.y + q.w * q.z), 2 * (q.x * q.z - q.w * q.y)};
	return transform;
}
template<typename T> pod::Transform<T> /*UF_API*/ uf::transform::reorient( const pod::Transform<T>& _transform ) {
/*
	transform.up = uf::vector::normalize(uf::quaternion::rotate(transform.orientation, pod::Vector3{0.0f, 1.0f, 0.0f}));
	transform.right = uf::vector::normalize(uf::quaternion::rotate(transform.orientation, pod::Vector3{1.0f, 0.0f, 0.0f}));
	transform.forward = uf::vector::normalize(uf::quaternion::rotate(transform.orientation, pod::Vector3{0.0f, 0.0f, 1.0f}));
*/
	pod::Transform<T> transform = _transform;
	pod::Quaternion<T> q = transform.orientation;
	transform.forward = { 2 * (q.x * q.z + q.w * q.y),  2 * (q.y * q.x - q.w * q.x), 1 - 2 * (q.x * q.x + q.y * q.y) };
	transform.up = { 2 * (q.x * q.y - q.w * q.z), 1 - 2 * (q.x * q.x + q.z * q.z), 2 * (q.y * q.z + q.w * q.x)};
	transform.right = { 1 - 2 * (q.y * q.y + q.z * q.z), 2 * (q.x * q.y + q.w * q.z), 2 * (q.x * q.z - q.w * q.y)};
	return transform;
}
template<typename T> pod::Transform<T>& /*UF_API*/ uf::transform::rotate( pod::Transform<T>& transform, const pod::Vector3t<T>& axis, pod::Math::num_t delta ) {
	pod::Quaternion<> quat = uf::quaternion::axisAngle( axis, delta );
	
	transform.orientation = uf::vector::normalize(uf::quaternion::multiply(transform.orientation, quat));
	transform = uf::transform::reorient(transform);

	return transform;
}
template<typename T> pod::Transform<T>& /*UF_API*/ uf::transform::rotate( pod::Transform<T>& transform, const pod::Quaternion<T>& quat ) {		
	transform.orientation = uf::vector::normalize(uf::quaternion::multiply(transform.orientation, quat));
	transform = uf::transform::reorient(transform);

	return transform;
}
template<typename T> pod::Transform<T>& /*UF_API*/ uf::transform::scale( pod::Transform<T>& transform, const pod::Vector3t<T>& factor ) {
	transform.scale = factor;
	return transform;
}
template<typename T> pod::Transform<T> /*UF_API*/ uf::transform::flatten( const pod::Transform<T>& transform, size_t depth ) {
	if ( !transform.reference ) return transform;
	pod::Transform<T> combined = transform;
	combined.reference = NULL;
	const pod::Transform<T>* pointer = transform.reference;
	while ( pointer && depth-- > 0 ) {
		// for some reason I can't just += for the camera
		combined.position = {
			combined.position.x + pointer->position.x,
			combined.position.y + pointer->position.y,
			combined.position.z + pointer->position.z,
		};
		pod::Quaternion<T> orientation = pointer->orientation;
		combined.orientation = uf::quaternion::multiply( orientation, combined.orientation );
		combined.scale = {
			combined.scale.x * pointer->scale.x,
			combined.scale.y * pointer->scale.y,
			combined.scale.z * pointer->scale.z,
		};
		combined.model = pointer->model * combined.model;
		pointer = pointer->reference;
	}
	return combined = uf::transform::reorient(combined);
}
template<typename T> pod::Matrix4t<T> /*UF_API*/ uf::transform::model( const pod::Transform<T>& transform, bool flatten, size_t depth ) {
	if ( flatten ) {
		pod::Transform<T> flatten = uf::transform::flatten(transform, depth);
		return uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
			uf::quaternion::matrix( flatten.orientation ) *
			uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
			flatten.model;
	}

	pod::Matrix4t<T> matrix = uf::matrix::identity();
	const pod::Transform<T>* pointer = &transform;
	do {
		pod::Matrix4t<T> model = uf::matrix::translate( uf::matrix::identity(), pointer->position ) *
			uf::quaternion::matrix( pointer->orientation ) *
			uf::matrix::scale( uf::matrix::identity(), pointer->scale ) *
			pointer->model;
		matrix = model * matrix;
		pointer = pointer->reference;
	} while ( pointer && --depth >= 0 );
	return matrix;
}
template<typename T> pod::Transform<T> uf::transform::fromMatrix( const pod::Matrix4t<T>& matrix ) {
	pod::Transform<T> transform;
	transform.position = uf::matrix::multiply<float>( matrix, pod::Vector3f{ 0, 0, 0 } );
	transform.orientation = uf::quaternion::fromMatrix( matrix );
	return transform = reorient( transform );
}

template<typename T> 														// Normalizes a vector
std::string /*UF_API*/ uf::transform::toString( const pod::Transform<T>& t, bool flatten ) {
	pod::Transform<T> transform = flatten ? uf::transform::flatten(t) : t;
	std::stringstream ss;
	ss << "Transform(" << uf::string::toString(transform.position) << "; " << uf::string::toString(transform.orientation) << ")";
	return ss.str();
}

template<typename T>
ext::json::Value /*UF_API*/ uf::transform::encode( const pod::Transform<T>& t, bool flatten ) {
	pod::Transform<T> transform = flatten ? uf::transform::flatten(t) : t;
	ext::json::Value json;
	json["position"] = uf::vector::encode(transform.position);
	json["orientation"] = uf::vector::encode(transform.orientation);
	json["scale"] = uf::vector::encode(transform.scale);
	json["model"] = uf::matrix::encode(transform.model);
	return json;
}
template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::decode( const ext::json::Value& _json, pod::Transform<T>& transform ) {
	ext::json::Value json = _json;
//	transform.position = uf::vector::decode<T, 3>(json["position"], transform.position);
//	transform.scale = uf::vector::decode<T, 3>(json["scale"], transform.scale);
//	transform.orientation = uf::vector::decode<T, 4>(json["orientation"], transform.orientation);
	if ( ext::json::isArray(json["position"]) || ext::json::isObject(json["position"]) ) transform.position = uf::vector::decode<T, 3>(json["position"], transform.position);
	if ( ext::json::isArray(json["scale"]) || ext::json::isObject(json["scale"]) ) transform.scale = uf::vector::decode<T, 3>(json["scale"], transform.scale);
	if ( ext::json::isArray(json["orientation"]) || ext::json::isObject(json["orientation"]) ) transform.orientation = uf::vector::decode<T, 4>(json["orientation"], transform.orientation);
	if ( ext::json::isObject(json["rotation"]) && !ext::json::isNull(json["rotation"]["axis"]) && !ext::json::isNull(json["rotation"]["angle"]) ) {
		pod::Vector3t<T> axis = uf::vector::decode<T, 3>(json["rotation"]["axis"]);
		T angle = json["rotation"]["angle"].as<T>();
		transform.orientation = uf::quaternion::axisAngle( axis, angle );
	}
	if ( ext::json::isArray(json["model"]) ) transform.model = uf::matrix::decode<T, 4, 4>(json["model"]);
	return transform;
}
template<typename T>
pod::Transform<T> /*UF_API*/ uf::transform::decode( const ext::json::Value& _json, const pod::Transform<T>& t ) {
	ext::json::Value json = _json;
	pod::Transform<T> transform = t;
//	transform.position = uf::vector::decode<T, 3>(json["position"], transform.position);
//	transform.scale = uf::vector::decode<T, 3>(json["scale"], transform.scale);
//	transform.orientation = uf::vector::decode<T, 4>(json["orientation"], transform.orientation);
	if ( ext::json::isArray(json["position"]) || ext::json::isObject(json["position"]) ) transform.position = uf::vector::decode<T, 3>(json["position"], transform.position);
	if ( ext::json::isArray(json["scale"]) || ext::json::isObject(json["scale"]) ) transform.scale = uf::vector::decode<T, 3>(json["scale"], transform.scale);
	if ( ext::json::isArray(json["orientation"]) || ext::json::isObject(json["orientation"]) ) transform.orientation = uf::vector::decode<T, 4>(json["orientation"], transform.orientation);
	if ( ext::json::isObject(json["rotation"]) && !ext::json::isNull(json["rotation"]["axis"]) && !ext::json::isNull(json["rotation"]["angle"]) ) {
		pod::Vector3t<T> axis = uf::vector::decode<T, 3>(json["rotation"]["axis"]);
		T angle = json["rotation"]["angle"].as<T>();
		transform.orientation = uf::quaternion::axisAngle( axis, angle );
	}
	if ( ext::json::isArray(json["model"]) ) transform.model = uf::matrix::decode<T, 4, 4>(json["model"]);
	return transform;
}