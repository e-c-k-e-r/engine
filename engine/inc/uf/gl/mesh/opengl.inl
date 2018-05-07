template<typename T>
void spec::ogl::Vao::render( T& vbo ) {
	this->bind();
	this->bindAttributes(true);
//	vbo.bindAttribute(true);
	vbo.render();
//	vbo.bindAttribute(false);
	this->bindAttributes(false);
}
//
template<typename T>
spec::ogl::Ibo<T>::Ibo() :
	m_index(0)
{

}
template<typename T>
spec::ogl::Ibo<T>::Ibo( Ibo<T>::indices_t&& indices ) : 
	Ibo()
{
	this->m_indices = std::move( indices );
}
template<typename T>
spec::ogl::Ibo<T>::Ibo( const Ibo<T>::indices_t& indices ) : 
	Ibo()
{
	this->m_indices = indices;
}
template<typename T>
spec::ogl::Ibo<T>::~Ibo() {
	this->clear();
	this->destroy();
}
template<typename T>
void spec::ogl::Ibo<T>::clear() {
	this->m_indices.clear();
}
template<typename T>
void spec::ogl::Ibo<T>::destroy() {
	if ( this->generated() ) glDeleteBuffers( 1, &this->m_index );
}

template<typename T>
void spec::ogl::Ibo<T>::bind() const {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_index);
}
template<typename T>
void spec::ogl::Ibo<T>::generate() {
	if ( !this->loaded() ) return;
	if ( this->generated() ) glDeleteBuffers( 1, &this->m_index );

	glGenBuffers(1, &this->m_index);
	this->bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->m_indices.size() * sizeof(T), &this->m_indices[0], GL_STATIC_DRAW);
}
template<typename T>
void spec::ogl::Ibo<T>::render( GLuint mode, void* offset ) const {
	static GLuint type = this->deduceType();
	glDrawElements( mode, this->m_indices.size(), type, offset );
}

template<typename T>
GLuint& spec::ogl::Ibo<T>::getIndex() {
	return this->m_index;
}
template<typename T>
GLuint spec::ogl::Ibo<T>::getIndex() const {
	return this->m_index;
}
template<typename T>
bool spec::ogl::Ibo<T>::loaded() const {
	return !this->m_indices.empty();
}
template<typename T>
bool spec::ogl::Ibo<T>::generated() const {
	return this->m_index;
}


#include <typeinfo>
template<typename T>
GLuint spec::ogl::Ibo<T>::deduceType() const {
	static GLuint type = 0;
	if ( type == 0 ) {	
		static const std::string type_t = typeid(T).name();
		static const std::string byte_t = typeid(char).name();
		static const std::string ubyte_t = typeid(unsigned char).name();
		static const std::string short_t = typeid(short).name();
		static const std::string ushort_t = typeid(unsigned short).name();
		static const std::string int_t = typeid(int).name();
		static const std::string fix_t = typeid(int).name();
		static const std::string uint_t = typeid(unsigned int).name();

		if ( type_t == byte_t ) 		type = GL_BYTE;
		else if ( type_t == ubyte_t ) 	type = GL_UNSIGNED_BYTE;
		else if ( type_t == short_t ) 	type = GL_SHORT;
		else if ( type_t == ushort_t ) 	type = GL_UNSIGNED_SHORT;
		else if ( type_t == int_t ) 	type = GL_INT;
		else if ( type_t == fix_t ) 	type = GL_FIXED;
		else if ( type_t == uint_t ) 	type = GL_UNSIGNED_INT;
		else 							type = GL_UNSIGNED_INT;
	}
	return type;
}

#include <unordered_set>
#include <map>
#include <uf/utils/io/iostream.h>
#include <uf/spec/time/time.h>
#include <uf/utils/time/time.h>
template<typename T>
void spec::ogl::Ibo<T>::index( uf::Vertices3f& position, uf::Vertices3f& normal, uf::Vertices4f& color, uf::Vertices2f& uv ) {
//	uf::iostream << "Indexing..." << "\n";
	uf::Timer<long long> timer;

	class Vertex {
	public:
		T 			 index;
		uf::Vector3f position;
		uf::Vector3f normal;
		uf::Vector4f color;
		uf::Vector2f uv;

		bool operator==( const Vertex& that ) const {
			return 	this->position 	== that.position 	&&
					this->normal 	== that.normal 		&&
					this->color 	== that.color 		&&
					this->uv 		== that.uv;
		}
		bool operator!=( const Vertex& that ) const { return !(*this == that); }
		bool operator< ( const Vertex& that ) const { return this->index <  that.index; }
		bool operator<=( const Vertex& that ) const { return this->index <= that.index; }
		bool operator> ( const Vertex& that ) const { return this->index >  that.index; }
		bool operator>=( const Vertex& that ) const { return this->index >= that.index; }
	};

	struct {
		bool position;
		bool normal;
		bool color;
		bool uv;
		bool bones_id;
		bool bones_weight;
	} use; /* Set up */ {
		use.position = position.verticesCount() > 0;
		use.normal = normal.verticesCount() > 0;
		use.color = color.verticesCount() > 0;
		use.uv = uv.verticesCount() > 0;
	}
	struct {
		uf::Vertices3f position;
		uf::Vertices3f normal;
		uf::Vertices4f color;
		uf::Vertices2f uv;
	} mesh; /* Set up mesh */ {
		mesh.position.getVertices().reserve(position.getVertices().size());
		mesh.normal.getVertices().reserve(normal.getVertices().size());
		mesh.color.getVertices().reserve(color.getVertices().size());
		mesh.uv.getVertices().reserve(uv.getVertices().size());
	}

	std::map<Vertex, T> map;

	auto& p = position.getVertices();
	auto& n = normal.getVertices();
	auto& c = color.getVertices();
	auto& u = uv.getVertices();

	std::size_t len = -1;
	if ( use.position ) 	len = std::min(len, position.verticesCount());
	if ( use.normal ) 		len = std::min(len, normal.verticesCount());
	if ( use.color ) 		len = std::min(len, color.verticesCount());
	if ( use.uv ) 			len = std::min(len, uv.verticesCount());
	for ( std::size_t i = 0; i < len; ++i ) {
		/* Retrieve vertex */ Vertex vertex; {
			/* Index */ {
				vertex.index = map.size();
			}
			/* Position */ if ( use.position ) {
				vertex.position.x = p[i*3+0];
				vertex.position.y = p[i*3+1];
				vertex.position.z = p[i*3+2];
			}
			/* Normal */ if ( use.normal ) {
				vertex.normal.x = n[i*3+0];
				vertex.normal.y = n[i*3+1];
				vertex.normal.z = n[i*3+2];
			}
			/* Color */ if ( use.color ) {
				vertex.color.r = c[i*4+0];
				vertex.color.g = c[i*4+1];
				vertex.color.b = c[i*4+2];
				vertex.color.a = c[i*4+3];
			}
			/* UV */ if ( use.uv ) {
				vertex.uv.x = u[i*2+0];
				vertex.uv.y = u[i*2+1];
			}
		}
		GLuint index = vertex.index;
		/* If vertex doesn't exist... */ if ( map.count(vertex) == 0 ) {
			map[vertex] = index;
			/* Store unique vertex */ {
				/* Position */ if ( use.position ) {
					mesh.position.add( vertex.position );
				}
				/* Normal */ if ( use.normal ) {
					mesh.normal.add( vertex.normal );
				}
				/* Color */ if ( use.color ) {
					mesh.color.add( vertex.color );
				}
				/* UV */ if ( use.uv ) {
					mesh.uv.add( vertex.uv );
				}
			}
		} /* Vertex exists; look up previous index */ else {
			index = map[vertex];
		}
		/* Store index to indices */ {
			this->m_indices.push_back(index);
		}
	}

	position = std::move( mesh.position );
	normal = std::move( mesh.normal );
	color = std::move( mesh.color );
	uv = std::move( mesh.uv );

//	uf::iostream << "Indexing took " << timer.elapsed().asDouble() << " seconds!" << "\n";
}

template<typename T>
void spec::ogl::Ibo<T>::index( uf::Vertices3f& position, uf::Vertices3f& normal, uf::Vertices4f& color, uf::Vertices2f& uv, uf::Vertices4i& bones_id, uf::Vertices4f& bones_weight ) {
//	uf::iostream << "Indexing..." << "\n";
	uf::Timer<long long> timer;

	class Vertex {
	public:
		T 			 index;
		uf::Vector3f position;
		uf::Vector3f normal;
		uf::Vector4f color;
		uf::Vector2f uv;
		uf::Vector4i bones_id;
		uf::Vector4f bones_weight;

		bool operator==( const Vertex& that ) const {
			return 	this->position 	== that.position 	&&
					this->normal 	== that.normal 		&&
					this->color 	== that.color 		&&
					this->uv 		== that.uv 			&&
					this->bones_id 	== that.bones_id	&&
					this->bones_weight 	== that.bones_weight;
		}
		bool operator!=( const Vertex& that ) const { return !(*this == that); }
		bool operator< ( const Vertex& that ) const { return this->index <  that.index; }
		bool operator<=( const Vertex& that ) const { return this->index <= that.index; }
		bool operator> ( const Vertex& that ) const { return this->index >  that.index; }
		bool operator>=( const Vertex& that ) const { return this->index >= that.index; }
	};

	struct {
		bool position;
		bool normal;
		bool color;
		bool uv;
		bool bones_id;
		bool bones_weight;
	} use; /* Set up */ {
		use.position = position.verticesCount() > 0;
		use.normal = normal.verticesCount() > 0;
		use.color = color.verticesCount() > 0;
		use.uv = uv.verticesCount() > 0;
		use.bones_id = bones_id.verticesCount() > 0;
		use.bones_weight = bones_weight.verticesCount() > 0;
	}
	struct {
		uf::Vertices3f position;
		uf::Vertices3f normal;
		uf::Vertices4f color;
		uf::Vertices2f uv;
		uf::Vertices4i bones_id;
		uf::Vertices4f bones_weight;
	} mesh; /* Set up mesh */ {
		mesh.position.getVertices().reserve(position.getVertices().size());
		mesh.normal.getVertices().reserve(normal.getVertices().size());
		mesh.color.getVertices().reserve(color.getVertices().size());
		mesh.uv.getVertices().reserve(uv.getVertices().size());
		mesh.bones_id.getVertices().reserve(bones_id.getVertices().size());
		mesh.bones_weight.getVertices().reserve(bones_weight.getVertices().size());
	}

	std::map<Vertex, T> map;

	auto& p = position.getVertices();
	auto& n = normal.getVertices();
	auto& c = color.getVertices();
	auto& u = uv.getVertices();
	auto& bi = bones_id.getVertices();
	auto& bw = bones_weight.getVertices();

	std::size_t len = -1;
	if ( use.position ) 	len = std::min(len, position.verticesCount());
	if ( use.normal ) 		len = std::min(len, normal.verticesCount());
	if ( use.color ) 		len = std::min(len, color.verticesCount());
	if ( use.uv ) 			len = std::min(len, uv.verticesCount());
	if ( use.bones_id ) 	len = std::min(len, bones_id.verticesCount());
	if ( use.bones_weight ) len = std::min(len, bones_weight.verticesCount());

	for ( std::size_t i = 0; i < len; ++i ) {
		/* Retrieve vertex */ Vertex vertex; {
			/* Index */ {
				vertex.index = map.size();
			}
			/* Position */ if ( use.position ) {
				vertex.position.x = p[i*3+0];
				vertex.position.y = p[i*3+1];
				vertex.position.z = p[i*3+2];
			}
			/* Normal */ if ( use.normal ) {
				vertex.normal.x = n[i*3+0];
				vertex.normal.y = n[i*3+1];
				vertex.normal.z = n[i*3+2];
			}
			/* Color */ if ( use.color ) {
				vertex.color.r = c[i*4+0];
				vertex.color.g = c[i*4+1];
				vertex.color.b = c[i*4+2];
				vertex.color.a = c[i*4+3];
			}
			/* UV */ if ( use.uv ) {
				vertex.uv.x = u[i*2+0];
				vertex.uv.y = u[i*2+1];
			}
			/* Bone ID */ if ( use.bones_id ) {
				vertex.bones_id.r = bi[i*4+0];
				vertex.bones_id.g = bi[i*4+1];
				vertex.bones_id.b = bi[i*4+2];
				vertex.bones_id.a = bi[i*4+3];
			}
			/* Bone Weight */ if ( use.bones_weight ) {
				vertex.bones_weight.r = bw[i*4+0];
				vertex.bones_weight.g = bw[i*4+1];
				vertex.bones_weight.b = bw[i*4+2];
				vertex.bones_weight.a = bw[i*4+3];
			}
		}
		GLuint index = vertex.index;
		/* If vertex doesn't exist... */ if ( map.count(vertex) == 0 ) {
			map[vertex] = index;
			/* Store unique vertex */ {
				/* Position */ if ( use.position ) {
					mesh.position.add( vertex.position );
				}
				/* Normal */ if ( use.normal ) {
					mesh.normal.add( vertex.normal );
				}
				/* Color */ if ( use.color ) {
					mesh.color.add( vertex.color );
				}
				/* UV */ if ( use.uv ) {
					mesh.uv.add( vertex.uv );
				}
				/* Bones */ if ( use.bones_id ) {
					mesh.bones_id.add( vertex.bones_id );
				}
				/* Bones */ if ( use.bones_weight ) {
					mesh.bones_weight.add( vertex.bones_weight );
				}
			}
		} /* Vertex exists; look up previous index */ else {
			index = map[vertex];
		}
		/* Store index to indices */ {
			this->m_indices.push_back(index);
		}
	}

	position = std::move( mesh.position );
	normal = std::move( mesh.normal );
	color = std::move( mesh.color );
	uv = std::move( mesh.uv );
	bones_id = std::move( mesh.bones_id );
	bones_weight = std::move( mesh.bones_weight );

//	uf::iostream << "Indexing took " << timer.elapsed().asDouble() << " seconds!" << "\n";
}