template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::initialize( pod::Transform<T>& transform ) {
	transform.position = {0, 0, 0};
	transform.scale = {1, 1, 1};

	transform.up = {0, 1, 0};
	transform.right = {1, 0, 0};
	transform.forward = {0, 0, 1};

	transform.orientation = {0, 0, 0, 1};
	transform.model = uf::matrix::identity();
	transform.reference = nullptr;

	return transform;
}

template<typename T>
pod::Transform<T> /*UF_API*/ uf::transform::initialize() {
	pod::Transform<T> transform;
	return uf::transform::initialize(transform);
}

template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::lookAt( pod::Transform<T>& transform, const pod::Vector3t<T>& at ) {
	pod::Vector3t<T> forward = uf::vector::normalize( at - transform.position );
	pod::Vector3t<T> right = uf::vector::normalize(uf::vector::cross( transform.up, forward ));
	pod::Vector3t<T> up = uf::vector::normalize(uf::vector::cross( forward, right ));

	transform.up = up;
	transform.right = right;
	transform.forward = forward;

	transform.orientation = uf::quaternion::lookAt( forward, up );

	return transform;
}

template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::move( pod::Transform<T>& transform, const pod::Vector3t<T>& axis, pod::Math::num_t delta ) {
	transform.position += (axis * delta);
	return transform;
}

template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::move( pod::Transform<T>& transform, const pod::Vector3t<T>& delta ) {
	transform.position += delta;
	return transform;
}

template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::reorient( pod::Transform<T>& transform ) {
	pod::Quaternion<T> q = transform.orientation;
	transform.forward = { 2 * (q.x * q.z + q.w * q.y),  2 * (q.y * q.z - q.w * q.x), 1 - 2 * (q.x * q.x + q.y * q.y) };
	transform.up = { 2 * (q.x * q.y - q.w * q.z), 1 - 2 * (q.x * q.x + q.z * q.z), 2 * (q.y * q.z + q.w * q.x)};
	transform.right = { 1 - 2 * (q.y * q.y + q.z * q.z), 2 * (q.x * q.y + q.w * q.z), 2 * (q.x * q.z - q.w * q.y)};
	return transform;
}

template<typename T>
pod::Transform<T> /*UF_API*/ uf::transform::reorient( const pod::Transform<T>& _transform ) {
	pod::Transform<T> transform = _transform;
	return uf::transform::reorient(transform);
}

template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::rotate( pod::Transform<T>& transform, const pod::Vector3t<T>& axis, pod::Math::num_t delta ) {
	pod::Quaternion<> quat = uf::quaternion::axisAngle( axis, delta );
	transform.orientation = uf::vector::normalize(uf::quaternion::multiply(quat, transform.orientation));
	return uf::transform::reorient(transform);
}

template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::rotate( pod::Transform<T>& transform, const pod::Quaternion<T>& quat ) {
	transform.orientation = uf::vector::normalize(uf::quaternion::multiply(quat, transform.orientation));
	return uf::transform::reorient(transform);
}

template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::scale( pod::Transform<T>& transform, const pod::Vector3t<T>& factor ) {
	transform.scale = factor;
	return transform;
}

template<typename T>
inline pod::Transform<T> /*UF_API*/ uf::transform::condense( const pod::Transform<T>& transform ) {
	return uf::transform::fromMatrix( uf::transform::model( transform, false ) );
}

template<typename T>
pod::Transform<T> /*UF_API*/ uf::transform::flatten( const pod::Transform<T>& transform, size_t depth ) {
	if ( !transform.reference ) return transform;

	pod::Transform<T> combined = transform;
	combined.reference = nullptr;
	const pod::Transform<T>* pointer = transform.reference;

	while ( pointer && depth-- > 0 ) {
		combined.position = pointer->position + uf::quaternion::rotate(pointer->orientation, combined.position * pointer->scale);
		combined.orientation = uf::quaternion::multiply( pointer->orientation, combined.orientation );
		combined.scale = combined.scale * pointer->scale;
		combined.model = pointer->model * combined.model;

		if ( pointer == pointer->reference ) break;
		pointer = pointer->reference;
	}
	return uf::transform::reorient(combined);
}

template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::transform::model( const pod::Transform<T>& transform, bool flatten, size_t depth ) {
	if ( flatten ) {
		pod::Transform<T> flat = uf::transform::flatten(transform, depth);
		return uf::matrix::translate( uf::matrix::identity(), flat.position ) *
			uf::quaternion::matrix( flat.orientation ) *
			uf::matrix::scale( uf::matrix::identity(), flat.scale ) *
			flat.model;
	}

	pod::Matrix4t<T> matrix = uf::matrix::identity();
	const pod::Transform<T>* pointer = &transform;

	do {
		pod::Matrix4t<T> modelMat = uf::matrix::translate( uf::matrix::identity(), pointer->position ) *
			uf::quaternion::matrix( pointer->orientation ) *
			uf::matrix::scale( uf::matrix::identity(), pointer->scale ) *
			pointer->model;
		matrix = modelMat * matrix;

		if ( pointer == pointer->reference ) break;
		pointer = pointer->reference;
	} while ( pointer && --depth > 0 );

	return matrix;
}

template<typename T>
pod::Transform<T> uf::transform::fromMatrix( const pod::Matrix4t<T>& matrix ) {
	pod::Transform<T> transform;
	transform.position = uf::matrix::multiply<float>( matrix, pod::Vector4f{ 0, 0, 0, 1 } );
	transform.orientation = uf::quaternion::fromMatrix( matrix );
	transform.model = matrix;
	return reorient( transform );
}

template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::reference( pod::Transform<T>& transform, const pod::Transform<T>& parent, bool reorient ) {
	transform.reference = const_cast<pod::Transform<T>*>(&parent);
	if ( !reorient ) {
		return transform;
	}

	transform.position = parent.position - transform.position;
	return transform;
}

template<typename T>
pod::Transform<T> /*UF_API*/ uf::transform::interpolate( const pod::Transform<T>& from, const pod::Transform<T>& to, float factor, bool reorient ) {
	pod::Transform transform = to;
	transform.position = uf::vector::lerp( from.position, to.position, factor );
	transform.orientation = uf::quaternion::slerp( from.orientation, to.orientation, factor );
	return reorient ? uf::transform::reorient( transform ) : transform;
}

template<typename T>
pod::Transform<T> uf::transform::inverse(const pod::Transform<T>& t) {
	pod::Transform<T> inv;

	pod::Quaternion<T> qInv = uf::quaternion::conjugate(uf::vector::normalize(t.orientation));

	pod::Vector3t<T> sInv {
		(fabs(t.scale.x) > 1e-8) ? (1.0f / t.scale.x) : 0.0f,
		(fabs(t.scale.y) > 1e-8) ? (1.0f / t.scale.y) : 0.0f,
		(fabs(t.scale.z) > 1e-8) ? (1.0f / t.scale.z) : 0.0f
	};

	inv.scale = sInv;
	inv.orientation = qInv;

	pod::Vector3t<T> negPos = -t.position;
	inv.position = uf::quaternion::rotate(qInv, pod::Vector3f{
		negPos.x * sInv.x,
		negPos.y * sInv.y,
		negPos.z * sInv.z
	});

	return uf::transform::reorient(inv);
}

template<typename T>
pod::Vector3t<T> /*UF_API*/ uf::transform::apply( const pod::Transform<T>& t, const pod::Vector3t<T>& p ) {
	return uf::quaternion::rotate( t.orientation, p * t.scale ) + t.position;
}

template<typename T>
pod::Vector3t<T> uf::transform::applyInverse( const pod::Transform<T>& t, const pod::Vector3t<T>& worldPoint ) {
	pod::Vector3t<T> translated = worldPoint - t.position;
	return uf::quaternion::rotate(uf::quaternion::inverse(t.orientation), translated) / t.scale;
}

template<typename T>
pod::Transform<T> uf::transform::relative( const pod::Transform<T>& a, const pod::Transform<T>& b ) {
	pod::Transform<T> rel;
	auto invOriA = uf::quaternion::inverse(a.orientation);
	rel.position = uf::quaternion::rotate(invOriA, b.position - a.position);
	rel.orientation = invOriA * b.orientation;
	return rel;
}

template<typename T>
uf::stl::string /*UF_API*/ uf::transform::toString( const pod::Transform<T>& t, bool flatten ) {
	pod::Transform<T> transform = flatten ? uf::transform::flatten(t) : t;
	uf::stl::stringstream ss;
	ss << "Transform(" << uf::string::toString(transform.position) << "; " << uf::string::toString(transform.orientation) << ")";
	return ss.str();
}

template<typename T>
ext::json::Value /*UF_API*/ uf::transform::encode( const pod::Transform<T>& t, bool flatten, const ext::json::EncodingSettings& settings ) {
	pod::Transform<T> transform = flatten ? uf::transform::flatten(t) : t;
	ext::json::Value json;
	json["position"] = uf::vector::encode(transform.position, settings);
	json["orientation"] = uf::vector::encode(transform.orientation, settings);
	json["scale"] = uf::vector::encode(transform.scale, settings);
	json["model"] = uf::matrix::encode(transform.model, settings);
	return json;
}

template<typename T>
pod::Transform<T>& /*UF_API*/ uf::transform::decode( const ext::json::Value& _json, pod::Transform<T>& transform ) {
	ext::json::Value json = _json; // cringe null behavior
	transform.position = uf::vector::decode<T, 3>(json["position"], transform.position);
	transform.scale = uf::vector::decode<T, 3>(json["scale"], transform.scale);
	transform.orientation = uf::vector::decode<T, 4>(json["orientation"], transform.orientation);

	if ( ext::json::isObject(json["rotation"]) && !ext::json::isNull(json["rotation"]["axis"]) && !ext::json::isNull(json["rotation"]["angle"]) ) {
		pod::Vector3t<T> axis = uf::vector::decode<T, 3>(json["rotation"]["axis"]);
		T angle = json["rotation"]["angle"].as<T>();
		transform.orientation = uf::quaternion::axisAngle( axis, angle );
	}

	transform.model = uf::matrix::decode<T, 4, 4>(json["model"], transform.model);
	return transform;
}

template<typename T>
pod::Transform<T> /*UF_API*/ uf::transform::decode( const ext::json::Value& json, const pod::Transform<T>& t ) {
	pod::Transform<T> transform = t;
	return uf::transform::decode(json, transform);
}