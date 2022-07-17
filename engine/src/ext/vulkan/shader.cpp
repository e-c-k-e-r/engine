#if UF_USE_VULKAN

#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/shader.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/camera/camera.h>

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <fstream>
#include <regex>

#define VK_DEBUG_VALIDATION_MESSAGE(x)\
//	VK_VALIDATION_MESSAGE(x);

#define UF_SHADER_PARSE_AS_JSON 0
#if UF_SHADER_PARSE_AS_JSON
ext::json::Value ext::vulkan::definitionToJson(/*const*/ ext::json::Value& definition ) {
	ext::json::Value member;
	// is object
	if ( !ext::json::isNull(definition["members"]) ) {
		ext::json::forEach(definition["members"], [&](/*const*/ ext::json::Value& value){
			uf::stl::string key = uf::string::split(value["name"].as<uf::stl::string>(), " ").back();
			member[key] = ext::vulkan::definitionToJson(value);
		});
	// is primitive
	} else if ( !ext::json::isNull(definition["value"]) ) {
		// is array of structs
		if ( definition["struct"].as<bool>() ) {
			ext::json::forEach(definition["value"], [&](/*const*/ ext::json::Value& value){
				ext::json::Value parsed;
				parsed["name"] = definition["name"];
				parsed["size"] = definition["size"];
				parsed["struct"] = definition["struct"];
				parsed["members"] = value;
				parsed = ext::vulkan::definitionToJson(parsed);
				member.emplace_back(parsed);
			});
		} else {
			member = definition["value"];
		}
	}
	return member;
}
ext::vulkan::userdata_t ext::vulkan::jsonToUserdata( const ext::json::Value& payload, const ext::json::Value& definition ) {
	size_t bufferLen = definition["size"].as<size_t>();
	
	ext::vulkan::userdata_t userdata;
	userdata.create(bufferLen);

	uint8_t* byteBuffer = (uint8_t*) (void*) userdata;
	uint8_t* byteBufferStart = byteBuffer;
	uint8_t* byteBufferEnd = byteBuffer + bufferLen;

#if UF_JSON_NLOHMANN_ORDERED
	// JSON is ordered, we can just push directly
#define UF_SHADER_TRACK_NAMES 0
#if UF_SHADER_TRACK_NAMES
	uf::stl::vector<uf::stl::string> variableName;	
#endif
	std::function<void(const ext::json::Value&)> parse = [&]( const ext::json::Value& value ){
		// is array or object
	#if UF_SHADER_TRACK_NAMES
		if ( ext::json::isObject(value) ) {
			ext::json::forEach(value, [&]( const uf::stl::string& name, const ext::json::Value& member ){
			#if UF_SHADER_TRACK_NAMES
				variableName.emplace_back(name);
			#endif
				parse(member);
			});
			#if UF_SHADER_TRACK_NAMES
				if ( !variableName.empty() ) variableName.pop_back();
			#endif
			return;
		}
		if ( ext::json::isArray(value) ) {
			ext::json::forEach(value, [&]( size_t i, const ext::json::Value& element ){
				variableName.emplace_back("["+std::to_string(i)+"]");
				parse(element);
			});
			#if UF_SHADER_TRACK_NAMES
				if ( !variableName.empty() ) variableName.pop_back();
			#endif
			return;
		}
	#else
		if ( ext::json::isArray(value) || ext::json::isObject(value) ) {
			ext::json::forEach(value, parse);
			return;
		}
	#endif
	#if UF_SHADER_TRACK_NAMES
		uf::stl::string path = uf::string::join(variableName, ".");
		path = uf::string::replace( path, ".[", "[" );
		VK_VALIDATION_MESSAGE("[" << (byteBuffer - byteBufferStart) << " / "<< (byteBufferEnd - byteBuffer) <<"]\tInserting: " << path << " = " << value.dump());
	#endif
		// is strictly an int
		if ( value.is<int>(true) ) {
			size_t size = sizeof(int32_t);
			auto get = value.as<int32_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		// is strictly an unsigned int
		} else if ( value.is<size_t>(true) ) {
			size_t size = sizeof(uint32_t);
			auto get = value.as<uint32_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		// is strictly a float
		} else if ( value.is<float>(true) ) {
			size_t size = sizeof(float);
			auto get = value.as<float>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		}
	#if UF_SHADER_TRACK_NAMES
		if ( !variableName.empty() ) variableName.pop_back();
	#endif
	};
#if UF_SHADER_TRACK_NAMES
	VK_VALIDATION_MESSAGE("Updating {} in {}", name, filename);
//	VK_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
	parse(payload);
#if UF_SHADER_TRACK_NAMES
//	VK_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
#else
	auto pushValue = [&]( const uf::stl::string& primitive, const ext::json::Value& input ){
		if ( primitive == "bool" ) {
			size_t size = sizeof(bool); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<bool>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "int8_t" ) {
			size_t size = sizeof(int8_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
	
		//	auto get = input.as<int8_t>();
		//	memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "uint8_t" ) {
			size_t size = sizeof(uint8_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
		//	auto get = input.as<uint8_t>();
		//	memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "int16_t" ) {
			size_t size = sizeof(int16_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
		//	auto get = input.as<int16_t>();
		//	memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "uint16_t" ) {
			size_t size = sizeof(uint16_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
		//	auto get = input.as<uint16_t>();
		//	memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "int32_t" ) {
			size_t size = sizeof(int32_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<int32_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "uint32_t" ) {
			size_t size = sizeof(uint32_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<int32_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "int64_t" ) {
			size_t size = sizeof(int64_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<uint64_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "uint64_t" ) {
			size_t size = sizeof(uint64_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<uint64_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "half" ) {
			size_t size = sizeof(float); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
		//	auto get = input.as<float>();
		//	memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "float" ) {
			size_t size = sizeof(float); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<float>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "double" ) {
			auto get = input.as<double>();
			size_t size = sizeof(double); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		}
		return true;
	};
	#define UF_SHADER_TRACK_NAMES 0
	#if UF_SHADER_TRACK_NAMES
		bool SKIP_ADD = false;
		uf::stl::vector<uf::stl::string> variableName;
	#endif
	std::function<void(const ext::json::Value&, const ext::json::Value&)> parseDefinition = [&](const ext::json::Value& input, const ext::json::Value& definition ){
	#if UF_SHADER_TRACK_NAMES
		if ( SKIP_ADD ) {
			SKIP_ADD = false;
		} else {
			auto split = uf::string::split(definition["name"].as<uf::stl::string>(), " ");
			uf::stl::string type = split.front();
			uf::stl::string name = split.back();
			variableName.emplace_back(name);
		}
	#endif
		// is object
		if ( !ext::json::isNull(definition["members"]) ) {
			ext::json::forEach(definition["members"], [&](const ext::json::Value& member){
				uf::stl::string key = uf::string::split(member["name"].as<uf::stl::string>(), " ").back();
				parseDefinition(input[key], member);
			});
		// is array or primitive
		} else if ( !ext::json::isNull(definition["value"]) ) {
			// is object
			auto split = uf::string::split(definition["name"].as<uf::stl::string>(), " ");
			uf::stl::string type = split.front();
			uf::stl::string name = split.back();
			std::regex regex("^(?:(.+?)\\<)?(.+?)(?:\\>)?(?:\\[(\\d+)\\])?$");
			std::smatch match;
			if ( !std::regex_search( type, match, regex ) ) {
				std::cout << "Ill formatted typename: " << definition["name"].as<uf::stl::string>() << std::endl;
				return;
			}
			uf::stl::string vectorMatrix = match[1].str();
			uf::stl::string primitive = match[2].str();
			uf::stl::string arraySize = match[3].str();	
			if ( ext::json::isObject(input) ) {
				ext::json::Value cloned;
				cloned["name"] = definition["name"];
				cloned["size"] = definition["size"].as<size_t>() / definition["value"].size();
				cloned["members"] = definition["value"][0];
				parseDefinition( input, cloned );
			}
			// is array
			else if ( ext::json::isArray(input) ) {
				ext::json::forEach( input, [&]( size_t i, const ext::json::Value& value){
				#if UF_SHADER_TRACK_NAMES
					variableName.emplace_back("["+std::to_string(i)+"]");
					SKIP_ADD = true;
				#endif
					parseDefinition(input[i], definition);
				});
			}
			// is primitive
			else {
			#if UF_SHADER_TRACK_NAMES
				uf::stl::string path = uf::string::join(variableName, ".");
				path = uf::string::replace( path, ".[", "[" );
				VK_VALIDATION_MESSAGE("[" << (byteBuffer - byteBufferStart) << " / "<< (byteBufferEnd - byteBuffer) <<"]\tInserting: " << path << " = (" << primitive << ") " << input.dump());
			#endif
				pushValue( primitive, input );
			}
		}
	#if UF_SHADER_TRACK_NAMES
		if ( !variableName.empty() ) variableName.pop_back();
	#endif
	};
	auto& definitions = metadata.json["definitions"]["uniforms"][name];
#if UF_SHADER_TRACK_NAMES
	VK_VALIDATION_MESSAGE("Updating " << name << " in " << filename);
	VK_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
	parseDefinition(payload, definitions);
#if UF_SHADER_TRACK_NAMES
	VK_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
#endif
	return userdata;
}
#endif
void ext::vulkan::Shader::initialize( ext::vulkan::Device& device, const uf::stl::string& filename, VkShaderStageFlagBits stage ) {
	this->device = &device;
	ext::vulkan::Buffers::initialize( device );
	
	uf::stl::string spirv;
	
	{
		std::ifstream is(this->filename = filename, std::ios::binary | std::ios::in | std::ios::ate);
		if ( !is.is_open() ) UF_EXCEPTION("Error: Could not open shader file: {}", filename);
		is.seekg(0, std::ios::end); spirv.reserve(is.tellg()); is.seekg(0, std::ios::beg);
		spirv.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
	
		assert(spirv.size() > 0);
	}
	
	{
	
		VkShaderModuleCreateInfo moduleCreateInfo = {};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = spirv.size();
		moduleCreateInfo.pCode = (uint32_t*) spirv.data();

		VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &module));
	}
	
	{
		descriptor.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		descriptor.stage = stage;
		descriptor.module = module;
		descriptor.pName = "main";

		assert(descriptor.module != VK_NULL_HANDLE);
	}
	// set up metadata
	{
		metadata.json["filename"] = filename;
		metadata.type = "";
	}
	// do reflection
	{
		spirv_cross::Compiler comp( (uint32_t*) &spirv[0], spirv.size() / 4 );
		spirv_cross::ShaderResources res = comp.get_shader_resources();
	#if UF_SHADER_PARSE_AS_JSON
		std::function<ext::json::Value(spirv_cross::TypeID)> parseMembers = [&]( spirv_cross::TypeID type_id ) {
			auto parseMember = [&]( auto type_id ){
				uf::Serializer payload;
				
				auto type = comp.get_type(type_id);

				uf::stl::string name = "";
				size_t size = 1;
				ext::json::Value value;
				switch ( type.basetype ) {
					case spirv_cross::SPIRType::BaseType::Boolean: 		name = "bool"; 		size = sizeof(bool); 		value = (bool) 0; 		break;
					case spirv_cross::SPIRType::BaseType::SByte: 		name = "int8_t"; 	size = sizeof(int8_t); 		value = (int8_t) 0; 	break;
					case spirv_cross::SPIRType::BaseType::UByte: 		name = "uint8_t"; 	size = sizeof(uint8_t); 	value = (uint8_t) 0; 	break;
					case spirv_cross::SPIRType::BaseType::Short: 		name = "int16_t"; 	size = sizeof(int16_t); 	value = (int16_t) 0; 	break;
					case spirv_cross::SPIRType::BaseType::UShort: 		name = "uint16_t";	size = sizeof(uint16_t); 	value = (uint16_t) 0; 	break;
					case spirv_cross::SPIRType::BaseType::Int: 			name = "int32_t"; 	size = sizeof(int32_t); 	value = (int32_t) 0; 	break;
					case spirv_cross::SPIRType::BaseType::UInt: 		name = "uint32_t";	size = sizeof(uint32_t); 	value = (uint32_t) 0; 	break;
					case spirv_cross::SPIRType::BaseType::Int64: 		name = "int64_t"; 	size = sizeof(int64_t); 	value = (int64_t) 0; 	break;
					case spirv_cross::SPIRType::BaseType::UInt64: 		name = "uint64_t";	size = sizeof(uint64_t); 	value = (uint64_t) 0; 	break;
					case spirv_cross::SPIRType::BaseType::Half: 		name = "half"; 		size = sizeof(float)/2; 	value = (float) 0; 		break;
					case spirv_cross::SPIRType::BaseType::Float: 		name = "float"; 	size = sizeof(float); 		value = (float) 0; 		break;
					case spirv_cross::SPIRType::BaseType::Double: 		name = "double"; 	size = sizeof(double); 		value = (double) 0; 		break;
					case spirv_cross::SPIRType::BaseType::Image: 		name = "image2D"; 	break;
					case spirv_cross::SPIRType::BaseType::SampledImage: name = "sampler2D"; break;
					case spirv_cross::SPIRType::BaseType::Sampler: 		name = "sampler"; 	break;
					default:
						name = comp.get_name(type_id);
						size = comp.get_declared_struct_size(type);
					break;
				}
				if ( name == "" ) name = comp.get_name(type.type_alias);
				if ( name == "" ) name = comp.get_name(type.type_alias);
				if ( name == "" ) name = comp.get_name(type.parent_type);
				if ( name == "" ) name = comp.get_fallback_name(type_id);
				if ( type.vecsize > 1 ) {
					name = "<"+name+">";
					if ( type.columns > 1 ) name = "Matrix"+std::to_string(type.vecsize)+"x"+std::to_string(type.columns)+name;
					else name = "Vector"+std::to_string(type.vecsize)+name;
				}
				{
					ext::json::Value source = value;
					value = ext::json::array();
					for ( size_t i = 0; i < type.vecsize * type.columns; ++i ) {
						value.emplace_back(source);
					}
					size *= type.columns * type.vecsize;
				}
				for ( auto arraySize : type.array ) {
					if ( arraySize > 1 ) {
						ext::json::Value source = value;
						value = ext::json::array();
						for ( size_t i = 0; i < arraySize; ++i ) {
							value.emplace_back(source);
						}
						name += "[" + std::to_string(arraySize) + "]";
						size *= arraySize;
					}
				}
				if ( ext::json::isArray(value) && value.size() == 1 ) {
					value = value[0];
				}
				payload["name"] = name;
				payload["size"] = size;
				payload["value"] = value;
				return payload;
			};
			uf::Serializer payload = ext::json::array();
			const auto& type = comp.get_type(type_id);
			if ( type.member_types.empty() ) {
				payload = parseMember( type_id );
			} else for ( auto& member_type_id : type.member_types ) {
				const auto& member_type = comp.get_type(member_type_id);
				uf::stl::string name = comp.get_member_name(type.type_alias, payload.size());
				if ( name == "" ) name = comp.get_member_name(type.parent_type, payload.size());
				if ( name == "" ) name = comp.get_member_name(type_id, payload.size());
				
				auto& entry = payload.emplace_back();
				auto parsed = parseMember(member_type_id);
				uf::stl::string type_name = parsed["name"];
				entry["name"] = type_name + " " + name; 
				if ( member_type.basetype == spirv_cross::SPIRType::BaseType::Struct ) {
					entry["struct"] = true;
					auto parsed = parseMembers(member_type_id);
					if ( !member_type.array.empty() && member_type.array[0] > 1 ) {
						size_t size = comp.get_declared_struct_size(member_type);
						for ( auto arraySize : member_type.array ) {
							ext::json::Value source = parsed;
							parsed = ext::json::array();
							for ( size_t i = 0; i < arraySize; ++i ) {
								parsed.emplace_back(source);
							}
							size = size * arraySize;
						}
						entry["size"] = size;
						entry["value"] = parsed;
					} else {
						entry["size"] = comp.get_declared_struct_size(member_type);
						entry["members"] = parsed;
					}
				} else {
					entry["size"] = parsed["size"];
					entry["value"] = parsed["value"];
				}
			}
			return payload;
		};
	#endif
		auto parseResource = [&]( const spirv_cross::Resource& resource, VkDescriptorType descriptorType, size_t index ) {			
			const auto& type = comp.get_type(resource.type_id);
			const auto& base_type = comp.get_type(resource.base_type_id);
			const size_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
			const uf::stl::string name = resource.name;
			size_t arraySize = 1;
			if ( !type.array.empty() ) {
				arraySize = type.array[0];
			}
			
			switch ( descriptorType ) {
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
					uf::stl::string tname = "";
					ext::vulkan::enums::Image::viewType_t etype{};
					switch ( type.image.dim ) {
						case spv::Dim::Dim1D: tname = "1D"; etype = ext::vulkan::enums::Image::VIEW_TYPE_1D; break;
						case spv::Dim::Dim2D: tname = "2D"; etype = ext::vulkan::enums::Image::VIEW_TYPE_2D; break;
						case spv::Dim::Dim3D: tname = "3D"; etype = ext::vulkan::enums::Image::VIEW_TYPE_3D; break;
						case spv::Dim::DimCube: tname = "Cube"; etype = ext::vulkan::enums::Image::VIEW_TYPE_CUBE; break;
						case spv::Dim::DimRect: tname = "Rect"; break;
						case spv::Dim::DimBuffer: tname = "Buffer"; break;
						case spv::Dim::DimSubpassData: tname = "SubpassData"; break;
					}
					uf::stl::string key = std::to_string(binding);
				#if UF_SHADER_PARSE_AS_JSON
					metadata.json["definitions"]["textures"][key]["name"] = name;
					metadata.json["definitions"]["textures"][key]["index"] = index;
					metadata.json["definitions"]["textures"][key]["binding"] = binding;
					metadata.json["definitions"]["textures"][key]["size"] = arraySize;
					metadata.json["definitions"]["textures"][key]["type"] = tname;
				#endif

					metadata.definitions.textures[binding] = Shader::Metadata::Definition::Texture{
						name,
						index,
						binding,
						arraySize,
						etype,
					};
				} break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
					size_t bufferSize = comp.get_declared_struct_size(base_type);
					if ( bufferSize <= 0 ) break;
					if ( bufferSize > device.properties.limits.maxUniformBufferRange ) {
						VK_DEBUG_VALIDATION_MESSAGE("Invalid uniform buffer length of " << bufferSize << " for shader " << filename);
						bufferSize = device.properties.limits.maxUniformBufferRange;
					}
					size_t misalignment = bufferSize % device.properties.limits.minStorageBufferOffsetAlignment;
					if ( misalignment != 0 ) {
						VK_DEBUG_VALIDATION_MESSAGE("Invalid uniform buffer alignment of " << misalignment << " for shader " << filename << ", correcting...");
						bufferSize += misalignment;
					}
					{
						VK_DEBUG_VALIDATION_MESSAGE("Uniform size of " << bufferSize << " for shader " << filename);
					//	auto& uniform = uniforms.emplace_back();
					//	uniform.create( bufferSize );
					}
					// generate definition to JSON
				#if UF_SHADER_PARSE_AS_JSON
					{
						metadata.json["definitions"]["uniforms"][name]["name"] = name;
						metadata.json["definitions"]["uniforms"][name]["index"] = index;
						metadata.json["definitions"]["uniforms"][name]["binding"] = binding;
						metadata.json["definitions"]["uniforms"][name]["size"] = bufferSize;
						metadata.json["definitions"]["uniforms"][name]["members"] = parseMembers(resource.type_id);
					}
				#endif
					// generate definition to unordered_map
					metadata.definitions.uniforms[name] = Shader::Metadata::Definition::Uniform{
						name,
						index,
						binding,
						bufferSize,
					};
				} break;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
					// generate definition to JSON
				#if UF_SHADER_PARSE_AS_JSON
					{
						metadata.json["definitions"]["storage"][name]["name"] = name;
						metadata.json["definitions"]["storage"][name]["index"] = index;
						metadata.json["definitions"]["storage"][name]["binding"] = binding;
						metadata.json["definitions"]["storage"][name]["members"] = parseMembers(resource.type_id);
					}
				#endif
					metadata.definitions.storage[name] = Shader::Metadata::Definition::Storage{
						name,
						index,
						binding
					};
				} break;
				case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
					// generate definition to JSON
				#if UF_SHADER_PARSE_AS_JSON
					{
						metadata.json["definitions"]["accelerationStructure"][name]["name"] = name;
						metadata.json["definitions"]["accelerationStructure"][name]["index"] = index;
						metadata.json["definitions"]["accelerationStructure"][name]["binding"] = binding;
						metadata.json["definitions"]["accelerationStructure"][name]["members"] = parseMembers(resource.type_id);
					}
				#endif
					metadata.definitions.accelerationStructure[name] = Shader::Metadata::Definition::AccelerationStructure{
						name,
						index,
						binding
					};
				} break;
			}
			descriptorSetLayoutBindings.push_back( ext::vulkan::initializers::descriptorSetLayoutBinding( descriptorType, stage, binding, arraySize ) );
		};	

		//for ( const auto& resource : res.key ) {

		#define LOOP_RESOURCES( key, type ) for ( size_t i = 0; i < res.key.size(); ++i ) {\
			const auto& resource = res.key[i];\
			VK_DEBUG_VALIDATION_MESSAGE("["<<filename<<"] Found resource: "#type " with binding: " << comp.get_decoration(resource.id, spv::DecorationBinding));\
			parseResource( resource, type, i );\
		}
		LOOP_RESOURCES( sampled_images, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER );
		LOOP_RESOURCES( separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE );
		LOOP_RESOURCES( storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE );
		LOOP_RESOURCES( separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER );
		LOOP_RESOURCES( subpass_inputs, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT );
		LOOP_RESOURCES( uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER );
		LOOP_RESOURCES( storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
		LOOP_RESOURCES( acceleration_structures, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR );
		#undef LOOP_RESOURCES
	
		{
			uniforms.reserve( metadata.definitions.uniforms.size() );
			uf::stl::vector<uf::stl::string> remap( metadata.definitions.uniforms.size() );
			for ( auto pair : metadata.definitions.uniforms ) {
				remap[pair.second.index] = pair.second.name;
			}
			for ( auto& name : remap ) {
				auto& definition = metadata.definitions.uniforms[name];

				if ( metadata.autoInitializeUniformBuffers ) {
					initializeBuffer(
						NULL,
						definition.size,
						VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
					);
				}
				if ( metadata.autoInitializeUniformUserdatas ) {
					auto& userdata = uniforms.emplace_back();
					userdata.create( definition.size, nullptr );
				}
			}
		}
	
		for ( auto i = 0; i < res.stage_inputs.size(); ++i ) {
			const auto& resource = res.stage_inputs[i];
			const auto& type = comp.get_type(resource.type_id);
			const auto& base_type = comp.get_type(resource.base_type_id);
			const size_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
			const uf::stl::string name = resource.name;
			size_t size = 0; // comp.get_declared_struct_size(type);
			// generate definition to JSON
		#if UF_SHADER_PARSE_AS_JSON
			{
				metadata.json["definitions"]["inputs"][name]["name"] = name;
				metadata.json["definitions"]["inputs"][name]["index"] = i;
				metadata.json["definitions"]["inputs"][name]["binding"] = binding;
				metadata.json["definitions"]["inputs"][name]["size"] = size;
				metadata.json["definitions"]["inputs"][name]["members"] = parseMembers(resource.type_id);
			}
		#endif
			// generate definition to unordered_map
			{
				metadata.definitions.inputs[name] = Shader::Metadata::Definition::InOut{
					name,
					i,
					binding,
					size,
				};
			}
		}
		for ( auto i = 0; i < res.stage_outputs.size(); ++i ) {
			const auto& resource = res.stage_outputs[i];
			const auto& type = comp.get_type(resource.type_id);
			const auto& base_type = comp.get_type(resource.base_type_id);
			const size_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
			const uf::stl::string name = resource.name;
			size_t size = 0; // comp.get_declared_struct_size(type);
			// generate definition to JSON
		#if UF_SHADER_PARSE_AS_JSON
			{
				metadata.json["definitions"]["outputs"][name]["name"] = name;
				metadata.json["definitions"]["outputs"][name]["index"] = i;
				metadata.json["definitions"]["outputs"][name]["binding"] = binding;
				metadata.json["definitions"]["outputs"][name]["size"] = size;
				metadata.json["definitions"]["outputs"][name]["members"] = parseMembers(resource.type_id);
			}
		#endif
			// generate definition to unordered_map
			{
				metadata.definitions.outputs[name] = Shader::Metadata::Definition::InOut{
					name,
					i,
					binding,
					size
				};
			}
		}
		for ( const auto& resource : res.push_constant_buffers ) {
			const auto& type = comp.get_type(resource.type_id);
			const auto& base_type = comp.get_type(resource.base_type_id);
			const size_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
			const uf::stl::string name = resource.name;
			size_t size = comp.get_declared_struct_size(type);
			if ( size <= 0 ) continue;
			// not a multiple of 4, for some reason
			if ( size % 4 != 0 ) {
				VK_DEBUG_VALIDATION_MESSAGE("Invalid push constant length of " << size << " for shader " << filename << ", must be multiple of 4, correcting...");
				size /= 4;
				++size; 
				size *= 4;
			}
			if ( size > device.properties.limits.maxPushConstantsSize ) {
				VK_DEBUG_VALIDATION_MESSAGE("Invalid push constant length of " << size << " for shader " << filename);
				size = device.properties.limits.maxPushConstantsSize;
			}
			VK_DEBUG_VALIDATION_MESSAGE("Push constant size of " << size << " for shader " << filename);
			{
				auto& pushConstant = pushConstants.emplace_back();
				pushConstant.create( size );
			}
			// generate definition to JSON
		#if UF_SHADER_PARSE_AS_JSON
			{
				metadata.json["definitions"]["pushConstants"][name]["name"] = name;
				metadata.json["definitions"]["pushConstants"][name]["index"] = pushConstants.size() - 1;
				metadata.json["definitions"]["pushConstants"][name]["binding"] = binding;
				metadata.json["definitions"]["pushConstants"][name]["size"] = size;
				metadata.json["definitions"]["pushConstants"][name]["members"] = parseMembers(resource.type_id);
			}
		#endif
			// generate definition to unordered_map
			{
				metadata.definitions.pushConstants[name] = Shader::Metadata::Definition::PushConstant{
					name,
					pushConstants.size() - 1,
					binding,
					size
				};
			}
		}
		
		size_t specializationSize = 0;
		uf::stl::vector<VkSpecializationMapEntry> specializationRemap;
		specializationRemap.resize( comp.get_specialization_constants().size() );

		for ( const auto& constant : comp.get_specialization_constants() ) {
			const auto& value = comp.get_constant(constant.id);
			const auto& type = comp.get_type(value.constant_type);
			uf::stl::string name = comp.get_name (constant.id);
			size_t size = 4; //comp.get_declared_struct_size(type);

			auto& specializationMapEntry = specializationRemap[constant.constant_id];
			specializationMapEntry.constantID = constant.constant_id;
			specializationMapEntry.size = size;
			specializationMapEntry.offset = specializationSize;
			specializationSize += size;
		}
		if ( specializationSize > 0 ) {			
			specializationRemap.reserve( specializationRemap.size() );
			for ( auto& specializationMapEntry : specializationRemap ) {
				specializationMapEntries.emplace_back(specializationMapEntry);
			}
			specializationConstants.create( specializationSize );
			VK_DEBUG_VALIDATION_MESSAGE("Specialization constants size of " << specializationSize << " for shader " << filename);

			uint8_t* s = (uint8_t*) (void*) specializationConstants;
			size_t offset = 0;
		#if UF_SHADER_PARSE_AS_JSON
			metadata.json["specializationConstants"] = ext::json::array();
		#endif
			for ( const auto& constant : comp.get_specialization_constants() ) {
				const auto& value = comp.get_constant(constant.id);
				const auto& type = comp.get_type(value.constant_type);
				uf::stl::string name = comp.get_name (constant.id);


				size_t size = 4;
				uint8_t buffer[size];
				memset( &buffer[0], size, 0 );
				
				auto& definition = metadata.definitions.specializationConstants[name];
				definition.name = name;
				definition.index = offset / size;
			#if UF_SHADER_PARSE_AS_JSON
				ext::json::Value member;
			#endif
				switch ( type.basetype ) {
					case spirv_cross::SPIRType::UInt: {
						auto v = value.scalar();
					#if UF_SHADER_PARSE_AS_JSON
						member["type"] = "uint32_t";
						member["value"] = v;
						member["validate"] = true;
					#endif
						memcpy( &buffer[0], &v, sizeof(v) );

						definition.type = "uint32_t";
						definition.value.ui = v;
						definition.validate = true;
					} break;
					case spirv_cross::SPIRType::Int: {
						auto v = value.scalar_i32();
					#if UF_SHADER_PARSE_AS_JSON
						member["type"] = "int32_t";
						member["value"] = v;
					#endif
						memcpy( &buffer[0], &v, sizeof(v) );

						definition.type = "int32_t";
						definition.value.i = v;
						definition.validate = false;
					} break;
					case spirv_cross::SPIRType::Float: {
						auto v = value.scalar_f32();
					#if UF_SHADER_PARSE_AS_JSON
						member["type"] = "float";
						member["value"] = v;
					#endif
						memcpy( &buffer[0], &v, sizeof(v) );

						definition.type = "float";
						definition.value.f = v;
						definition.validate = false;
					} break;
					case spirv_cross::SPIRType::Boolean: {
						auto v = value.scalar()!=0;
					#if UF_SHADER_PARSE_AS_JSON
						member["type"] = "bool";
						member["value"] = v;
					#endif
						memcpy( &buffer[0], &v, sizeof(v) );

						definition.type = "bool";
						definition.value.ui = v;
						definition.validate = false;
					} break;
					default: {
						VK_DEBUG_VALIDATION_MESSAGE("Unregistered specialization constant type at offset " << offset << " for shader " << filename );
					} break;
				}
			#if UF_SHADER_PARSE_AS_JSON
				member["name"] = name;
				member["size"] = size;
				member["default"] = member["value"];
				metadata.json["specializationConstants"].emplace_back(member);
				VK_DEBUG_VALIDATION_MESSAGE("Specialization constant: " << member["type"].as<uf::stl::string>() << " " << name << " = " << member["value"].dump() << "; at offset " << offset << " for shader " << filename );
			#endif

				memcpy( &s[offset], &buffer, size );
				offset += size;
			}
		}
		// set specialization info
		specializationInfo = {};
		specializationInfo.mapEntryCount = specializationMapEntries.size();
		specializationInfo.pMapEntries = specializationMapEntries.data();
		specializationInfo.dataSize = specializationConstants.size();
		specializationInfo.pData = (void*) specializationConstants;
		
		descriptor.pSpecializationInfo = &specializationInfo;
	/*
	*/
	//	LOOP_RESOURCES( stage_inputs, VkVertexInputAttributeDescription );
	//	LOOP_RESOURCES( stage_outputs, VkPipelineColorBlendAttachmentState );
	}

	// organize layouts
	std::sort( descriptorSetLayoutBindings.begin(), descriptorSetLayoutBindings.end(), [&]( const VkDescriptorSetLayoutBinding& l, const VkDescriptorSetLayoutBinding& r ){
		return l.binding < r.binding;
	} );
}

void ext::vulkan::Shader::destroy() {
	if ( !device ) return;

	ext::vulkan::Buffers::destroy();

	if ( module != VK_NULL_HANDLE ) {
		vkDestroyShaderModule( *device, module, nullptr );
		module = VK_NULL_HANDLE;
		descriptor = {};
	}
	for ( auto& userdata : uniforms ) userdata.destroy();
	for ( auto& userdata : pushConstants ) userdata.destroy();
	uniforms.clear();
	pushConstants.clear();
}

bool ext::vulkan::Shader::validate() {
	// check if uniforms match buffer size
	bool valid = true;
	auto it = uniforms.begin();
	for ( auto& buffer : buffers ) {
		if ( !(buffer.usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
		if ( it == uniforms.end() ) break;
		auto& uniform = *(it++);
		if ( uniform.data().len != buffer.allocationInfo.size ) {
			VK_DEBUG_VALIDATION_MESSAGE("Uniform size mismatch: Expected " << buffer.allocationInfo.size << ", got " << uniform.data().len << "; fixing...");
			uniform.destroy();
			uniform.create(buffer.allocationInfo.size);
			valid = false;
		}
	}
	return valid;
}
bool ext::vulkan::Shader::hasUniform( const uf::stl::string& name ) const {
	return metadata.definitions.uniforms.count(name) > 0;
}
ext::vulkan::Buffer& ext::vulkan::Shader::getUniformBuffer( const uf::stl::string& name ) {
	UF_ASSERT( hasUniform(name) );
	size_t uniformIndex = metadata.definitions.uniforms[name].index;
	for ( size_t bufferIndex = 0, uniformCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers[bufferIndex].usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
		if ( uniformCounter++ != uniformIndex ) continue;
		return buffers[bufferIndex];
	}
	UF_EXCEPTION("buffer not found: {}", name);
}
const ext::vulkan::Buffer& ext::vulkan::Shader::getUniformBuffer( const uf::stl::string& name ) const {
	UF_ASSERT( hasUniform(name) );
	size_t uniformIndex = metadata.definitions.uniforms.at(name).index;
	for ( size_t bufferIndex = 0, uniformCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers.at(bufferIndex).usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
		if ( uniformCounter++ != uniformIndex ) continue;
		return buffers.at(bufferIndex);
	}
	UF_EXCEPTION("buffer not found: {}", name);
}
ext::vulkan::userdata_t& ext::vulkan::Shader::getUniform( const uf::stl::string& name ) {
	UF_EXCEPTION("[DEPRECATED] {} {}", filename, name);
	UF_ASSERT( hasUniform(name) );
	return uniforms[metadata.definitions.uniforms[name].index];
}
const ext::vulkan::userdata_t& ext::vulkan::Shader::getUniform( const uf::stl::string& name ) const {
	UF_EXCEPTION("[DEPRECATED] {} {}", filename, name);
	UF_ASSERT( hasUniform(name) );
	return uniforms.at(metadata.definitions.uniforms.at(name).index);
}
bool ext::vulkan::Shader::updateUniform( const uf::stl::string& name, const void* data, size_t size ) const {
	if ( !hasUniform(name) ) return false;
	updateBuffer( data, size, getUniformBuffer(name) );
	return true;
}
//
bool ext::vulkan::Shader::hasStorage( const uf::stl::string& name ) const {
	return metadata.definitions.storage.count(name) > 0;
}
ext::vulkan::Buffer& ext::vulkan::Shader::getStorageBuffer( const uf::stl::string& name ) {
	UF_ASSERT( hasStorage(name) );
	size_t storageIndex = metadata.definitions.storage[name].index;
	for ( size_t bufferIndex = 0, storageCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers[bufferIndex].usage & uf::renderer::enums::Buffer::STORAGE) ) continue;
		if ( storageCounter++ != storageIndex ) continue;
		return buffers[bufferIndex];
	}
	UF_EXCEPTION("buffer not found: {}", name);
}
const ext::vulkan::Buffer& ext::vulkan::Shader::getStorageBuffer( const uf::stl::string& name ) const {
	UF_ASSERT( hasStorage(name) );
	size_t storageIndex = metadata.definitions.storage.at(name).index;
	for ( size_t bufferIndex = 0, storageCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers.at(bufferIndex).usage & uf::renderer::enums::Buffer::STORAGE) ) continue;
		if ( storageCounter++ != storageIndex ) continue;
		return buffers.at(bufferIndex);
	}
	UF_EXCEPTION("buffer not found: {}", name);
}
bool ext::vulkan::Shader::updateStorage( const uf::stl::string& name, const void* data, size_t size ) const {
	if ( !hasStorage(name) ) return false;
	updateBuffer( data, size, getStorageBuffer(name) );
	return true;
}
// JSON shit
#if 0 && UF_SHADER_PARSE_AS_JSON
uf::Serializer ext::vulkan::Shader::getUniformJson( const uf::stl::string& name, bool cache ) {
	if ( !hasUniform(name) ) return ext::json::null();
	if ( cache && !ext::json::isNull(metadata.json["uniforms"][name]) ) return metadata.json["uniforms"][name];
	auto& definition = metadata.json["definitions"]["uniforms"][name];
	if ( cache ) return metadata.json["uniforms"][name] = definitionToJson(definition);
	return definitionToJson(definition);
}
ext::vulkan::userdata_t ext::vulkan::Shader::getUniformUserdata( const uf::stl::string& name, const ext::json::Value& payload ) {
	if ( !hasUniform(name) ) return {};
	return jsonToUserdata(payload, metadata.json["definitions"]["uniforms"][name]);
}
bool ext::vulkan::Shader::updateUniform( const uf::stl::string& name, const ext::json::Value& payload ) {
	if ( !hasUniform(name) ) return false;
	return updateUniform( name, getUniformUserdata( name, payload ) );
}

uf::Serializer ext::vulkan::Shader::getStorageJson( const uf::stl::string& name, bool cache ) {
	if ( !hasStorage(name) ) return ext::json::null();
	if ( cache && !ext::json::isNull(metadata.json["storage"][name]) ) return metadata.json["storage"][name];
	auto& definition = metadata.json["definitions"]["storage"][name];
	if ( cache ) return metadata.json["storage"][name] = definitionToJson(definition);
	return definitionToJson(definition);
}
ext::vulkan::userdata_t ext::vulkan::Shader::getStorageUserdata( const uf::stl::string& name, const ext::json::Value& payload ) {
	if ( !hasStorage(name) ) return false;
	return jsonToUserdata(payload, metadata.json["definitions"]["storage"][name]);
}
#endif
#endif