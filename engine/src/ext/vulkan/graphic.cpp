#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/openvr/openvr.h>

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <fstream>
#include <regex>

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
}
ext::json::Value ext::vulkan::definitionToJson(/*const*/ ext::json::Value& definition ) {
	ext::json::Value member;
	// is object
	if ( !ext::json::isNull(definition["members"]) ) {
		ext::json::forEach(definition["members"], [&](/*const*/ ext::json::Value& value){
			std::string key = uf::string::split(value["name"].as<std::string>(), " ").back();
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
	std::vector<std::string> variableName;	
#endif
	std::function<void(const ext::json::Value&)> parse = [&]( const ext::json::Value& value ){
		// is array or object
	#if UF_SHADER_TRACK_NAMES
		if ( ext::json::isObject(value) ) {
			ext::json::forEach(value, [&]( const std::string& name, const ext::json::Value& member ){
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
		std::string path = uf::string::join(variableName, ".");
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
	VK_VALIDATION_MESSAGE("Updating " << name << " in " << filename);
	VK_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
	parse(payload);
#if UF_SHADER_TRACK_NAMES
	VK_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
#else
	auto pushValue = [&]( const std::string& primitive, const ext::json::Value& input ){
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
		}
 else if ( primitive == "uint32_t" ) {
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
		std::vector<std::string> variableName;
	#endif
	std::function<void(const ext::json::Value&, const ext::json::Value&)> parseDefinition = [&](const ext::json::Value& input, const ext::json::Value& definition ){
	#if UF_SHADER_TRACK_NAMES
		if ( SKIP_ADD ) {
			SKIP_ADD = false;
		} else {
			auto split = uf::string::split(definition["name"].as<std::string>(), " ");
			std::string type = split.front();
			std::string name = split.back();
			variableName.emplace_back(name);
		}
	#endif
		// is object
		if ( !ext::json::isNull(definition["members"]) ) {
			ext::json::forEach(definition["members"], [&](const ext::json::Value& member){
				std::string key = uf::string::split(member["name"].as<std::string>(), " ").back();
				parseDefinition(input[key], member);
			});
		// is array or primitive
		} else if ( !ext::json::isNull(definition["value"]) ) {
			// is object
			auto split = uf::string::split(definition["name"].as<std::string>(), " ");
			std::string type = split.front();
			std::string name = split.back();
			std::regex regex("^(?:(.+?)\\<)?(.+?)(?:\\>)?(?:\\[(\\d+)\\])?$");
			std::smatch match;
			if ( !std::regex_search( type, match, regex ) ) {
				std::cout << "Ill formatted typename: " << definition["name"].as<std::string>() << std::endl;
				return;
			}
			std::string vectorMatrix = match[1].str();
			std::string primitive = match[2].str();
			std::string arraySize = match[3].str();	
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
				std::string path = uf::string::join(variableName, ".");
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
	auto& definitions = metadata["definitions"]["uniforms"][name];
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
/*
ext::vulkan::Shader::~Shader() {
	if ( !aliased ) destroy();
}
*/
void ext::vulkan::Shader::initialize( ext::vulkan::Device& device, const std::string& filename, VkShaderStageFlagBits stage ) {
	this->device = &device;
	ext::vulkan::Buffers::initialize( device );
	aliased = false;
	
	std::string spirv;
	
	{
		std::ifstream is(this->filename = filename, std::ios::binary | std::ios::in | std::ios::ate);
		if ( !is.is_open() ) {
			VK_VALIDATION_MESSAGE("Error: Could not open shader file \"" << filename << "\"");
			return;
		}
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
		metadata["filename"] = filename;
		metadata["type"] = "";
	}
	// do reflection
	{
		spirv_cross::Compiler comp( (uint32_t*) &spirv[0], spirv.size() / 4 );
		spirv_cross::ShaderResources res = comp.get_shader_resources();

		std::function<ext::json::Value(spirv_cross::TypeID)> parseMembers = [&]( spirv_cross::TypeID type_id ) {
			auto parseMember = [&]( auto type_id ){
				uf::Serializer payload;
				
				auto type = comp.get_type(type_id);

				std::string name = "";
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
					if ( type.columns > 1 ) {
						name = "Matrix"+std::to_string(type.vecsize)+"x"+std::to_string(type.columns)+name;
					} else {
						name = "Vector"+std::to_string(type.vecsize)+name;
					}
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
			for ( auto& member_type_id : type.member_types ) {
				const auto& member_type = comp.get_type(member_type_id);
				std::string name = comp.get_member_name(type.type_alias, payload.size());
				if ( name == "" ) name = comp.get_member_name(type.parent_type, payload.size());
				if ( name == "" ) name = comp.get_member_name(type_id, payload.size());
				
				auto& entry = payload.emplace_back();
				auto parsed = parseMember(member_type_id);
				std::string type_name = parsed["name"];
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

		auto parseResource = [&]( const spirv_cross::Resource& resource, VkDescriptorType descriptorType, size_t index ) {			
			const auto& type = comp.get_type(resource.type_id);
			const auto& base_type = comp.get_type(resource.base_type_id);
			std::string name = resource.name;
			
			switch ( descriptorType ) {
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
					size_t size = comp.get_declared_struct_size(base_type);
					if ( size <= 0 ) break;
					if ( size > device.properties.limits.maxUniformBufferRange ) {
						VK_VALIDATION_MESSAGE("Invalid uniform buffer length of " << size << " for shader " << filename);
						size = device.properties.limits.maxUniformBufferRange;
					}
					size_t misalignment = size % device.properties.limits.minStorageBufferOffsetAlignment;
					if ( misalignment != 0 ) {
						VK_VALIDATION_MESSAGE("Invalid uniform buffer alignment of " << misalignment << " for shader " << filename << ", correcting...");
						size += misalignment;
					}
					{
						VK_VALIDATION_MESSAGE("Uniform size of " << size << " for shader " << filename);
						auto& uniform = uniforms.emplace_back();
						uniform.create( size );
					}		
					// generate definition to JSON
					{
						metadata["definitions"]["uniforms"][name]["name"] = name;
						metadata["definitions"]["uniforms"][name]["index"] = index;
						metadata["definitions"]["uniforms"][name]["size"] = size;
						metadata["definitions"]["uniforms"][name]["members"] = parseMembers(resource.type_id);
					}
				} break;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
					// generate definition to JSON
					{
						metadata["definitions"]["storage"][name]["name"] = name;
						metadata["definitions"]["storage"][name]["index"] = index;
						metadata["definitions"]["storage"][name]["members"] = parseMembers(resource.type_id);
					}
					// test
					{
					//	updateUniform(name, getUniform(name));
					}
				} break;
			}

			size_t size = 1;
			if ( !type.array.empty() ) {
				size = type.array[0];
			}
			descriptorSetLayoutBindings.push_back( ext::vulkan::initializers::descriptorSetLayoutBinding( descriptorType, stage, comp.get_decoration(resource.id, spv::DecorationBinding), size ) );
		};
		

		//for ( const auto& resource : res.key ) {
		#define LOOP_RESOURCES( key, type ) for ( size_t i = 0; i < res.key.size(); ++i ) {\
			const auto& resource = res.key[i];\
			VK_VALIDATION_MESSAGE("["<<filename<<"] Found resource: "#type " with binding: " << comp.get_decoration(resource.id, spv::DecorationBinding));\
			parseResource( resource, type, i );\
		}
		LOOP_RESOURCES( sampled_images, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER );
		LOOP_RESOURCES( separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE );
		LOOP_RESOURCES( storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE );
		LOOP_RESOURCES( separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER );
		LOOP_RESOURCES( subpass_inputs, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT );
		LOOP_RESOURCES( uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER );
		LOOP_RESOURCES( storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
		#undef LOOP_RESOURCES

	//	if ( filename == "./data/shaders/display.subpass.stereo.frag.spv" )
	//		std::cout << filename << ": " << ext::json::encode(metadata, true) << std::endl;
		
		for ( const auto& resource : res.push_constant_buffers ) {
			const auto& type = comp.get_type(resource.type_id);
			const auto& base_type = comp.get_type(resource.base_type_id);
			std::string name = resource.name;
			size_t size = comp.get_declared_struct_size(type);
			if ( size <= 0 ) continue;
			// not a multiple of 4, for some reason
			if ( size % 4 != 0 ) {
				VK_VALIDATION_MESSAGE("Invalid push constant length of " << size << " for shader " << filename << ", must be multiple of 4, correcting...");
				size /= 4;
				++size; 
				size *= 4;
			}
			if ( size > device.properties.limits.maxPushConstantsSize ) {
				VK_VALIDATION_MESSAGE("Invalid push constant length of " << size << " for shader " << filename);
				size = device.properties.limits.maxPushConstantsSize;
			}
			VK_VALIDATION_MESSAGE("Push constant size of " << size << " for shader " << filename);
			{
				auto& pushConstant = pushConstants.emplace_back();
				pushConstant.create( size );
			}
			// generate definition to JSON
			{
				metadata["definitions"]["pushConstants"][name]["name"] = name;
				metadata["definitions"]["pushConstants"][name]["size"] = size;
				metadata["definitions"]["pushConstants"][name]["index"] = pushConstants.size() - 1;
				metadata["definitions"]["pushConstants"][name]["members"] = parseMembers(resource.type_id);
			}
		}
		
		size_t specializationSize = 0;
		for ( const auto& constant : comp.get_specialization_constants() ) {
			const auto& value = comp.get_constant(constant.id);
			const auto& type = comp.get_type(value.constant_type);
			size_t size = 4; //comp.get_declared_struct_size(type);

			VkSpecializationMapEntry specializationMapEntry;
			specializationMapEntry.constantID = constant.constant_id;
			specializationMapEntry.size = size;
			specializationMapEntry.offset = specializationSize;
			specializationMapEntries.push_back(specializationMapEntry);
			specializationSize += size;
		}
		if ( specializationSize > 0 ) {			
			specializationConstants.create( specializationSize );
			VK_VALIDATION_MESSAGE("Specialization constants size of " << specializationSize << " for shader " << filename);

			uint8_t* s = (uint8_t*) (void*) specializationConstants;
			size_t offset = 0;

			metadata["specializationConstants"] = ext::json::array();
			for ( const auto& constant : comp.get_specialization_constants() ) {
				const auto& value = comp.get_constant(constant.id);
				const auto& type = comp.get_type(value.constant_type);
				std::string name = comp.get_name (constant.id);

				ext::json::Value member;

				size_t size = 4;
				uint8_t buffer[size];
				switch ( type.basetype ) {
					case spirv_cross::SPIRType::UInt: {
						auto v = value.scalar();
						member["type"] = "uint32_t";
						member["value"] = v;
						member["validate"] = true;
						memcpy( &buffer[0], &v, sizeof(v) );
					} break;
					case spirv_cross::SPIRType::Int: {
						auto v = value.scalar_i32();
						member["type"] = "int32_t";
						member["value"] = v;
						memcpy( &buffer[0], &v, sizeof(v) );
					} break;
					case spirv_cross::SPIRType::Float: {
						auto v = value.scalar_f32();
						member["type"] = "float";
						member["value"] = v;
						memcpy( &buffer[0], &v, sizeof(v) );
					} break;
					case spirv_cross::SPIRType::Boolean: {
						auto v = value.scalar()!=0;
						member["type"] = "bool";
						member["value"] = v;
						memcpy( &buffer[0], &v, sizeof(v) );
					} break;
					default: {
						VK_VALIDATION_MESSAGE("Unregistered specialization constant type at offset " << offset << " for shader " << filename );
					} break;
				}
				member["name"] = name;
				member["size"] = size;
				member["default"] = member["value"];
				VK_VALIDATION_MESSAGE("Specialization constant: " << member["type"].as<std::string>() << " " << name << " = " << member["value"].dump() << "; at offset " << offset << " for shader " << filename );
				metadata["specializationConstants"].emplace_back(member);

				memcpy( &s[offset], &buffer, size );
				offset += size;
			}
		/*
			{
				specializationInfo = {};
				specializationInfo.dataSize = specializationSize;
				specializationInfo.mapEntryCount = specializationMapEntries.size();
				specializationInfo.pMapEntries = specializationMapEntries.data();
				specializationInfo.pData = (void*) specializationConstants;
				descriptor.pSpecializationInfo = &specializationInfo;
			}
		*/
		}
	/*
	*/
	//	LOOP_RESOURCES( stage_inputs, VkVertexInputAttributeDescription );
	//	LOOP_RESOURCES( stage_outputs, VkPipelineColorBlendAttachmentState );
	}

	// organize layouts
	{
		std::sort( descriptorSetLayoutBindings.begin(), descriptorSetLayoutBindings.end(), [&]( const VkDescriptorSetLayoutBinding& l, const VkDescriptorSetLayoutBinding& r ){
			return l.binding < r.binding;
		} );
	}

	// update uniform buffers
	for ( auto& uniform : uniforms ) {
		auto& userdata = uniform.data();
		initializeBuffer(
			(void*) userdata.data,
			userdata.len,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			false
		);
	}
}

void ext::vulkan::Shader::destroy() {
	if ( aliased ) return;
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
	{
		auto it = uniforms.begin();
		for ( auto& buffer : buffers ) {
			if ( !(buffer.usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) ) continue;
			if ( it == uniforms.end() ) break;
			auto& uniform = *(it++);
			if ( uniform.data().len != buffer.allocationInfo.size ) {
				VK_VALIDATION_MESSAGE("Uniform size mismatch: Expected " << buffer.allocationInfo.size << ", got " << uniform.data().len << "; fixing...");
				uniform.destroy();
				uniform.create(buffer.allocationInfo.size);
				valid = false;
			}
		}
	}
	return valid;
}
bool ext::vulkan::Shader::hasUniform( const std::string& name ) {
	return !ext::json::isNull(metadata["definitions"]["uniforms"][name]);
}
ext::vulkan::Buffer* ext::vulkan::Shader::getUniformBuffer( const std::string& name ) {
	if ( !hasUniform(name) ) return NULL;
	size_t uniformIndex = metadata["definitions"]["uniforms"][name]["index"].as<size_t>();
	for ( size_t bufferIndex = 0, uniformCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers[bufferIndex].usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) ) continue;
		if ( uniformCounter++ != uniformIndex ) continue;
		return &buffers[bufferIndex];
	}
	return NULL;
}
ext::vulkan::userdata_t& ext::vulkan::Shader::getUniform( const std::string& name ) {
	if ( !hasUniform(name) ) {
		static ext::vulkan::userdata_t null;
		return null;
	}
	auto& definition = metadata["definitions"]["uniforms"][name];
	size_t uniformSize = definition["size"].as<size_t>();
	size_t uniformIndex = definition["index"].as<size_t>();
	auto& userdata = uniforms[uniformIndex];
	return userdata;
}
bool ext::vulkan::Shader::updateUniform( const std::string& name ) {
	if ( !hasUniform(name) ) return false;
	auto& uniform = getUniform(name);
	return updateUniform(name, uniform);
}
bool ext::vulkan::Shader::updateUniform( const std::string& name, const ext::vulkan::userdata_t& userdata ) {
	if ( !hasUniform(name) ) return false;
	auto* bufferObject = getUniformBuffer(name);
	if ( !bufferObject ) return false;
	size_t size = std::max(metadata["definitions"]["uniforms"][name]["size"].as<size_t>(), bufferObject->allocationInfo.size);
	updateBuffer( (void*) userdata, size, *bufferObject );
	return true;
}

uf::Serializer ext::vulkan::Shader::getUniformJson( const std::string& name, bool cache ) {
	if ( !hasUniform(name) ) return ext::json::null();
	if ( cache && !ext::json::isNull(metadata["uniforms"][name]) ) return metadata["uniforms"][name];
	auto& definition = metadata["definitions"]["uniforms"][name];
	if ( cache ) return metadata["uniforms"][name] = definitionToJson(definition);
	return definitionToJson(definition);
}
ext::vulkan::userdata_t ext::vulkan::Shader::getUniformUserdata( const std::string& name, const ext::json::Value& payload ) {
	if ( !hasUniform(name) ) return false;
	return jsonToUserdata(payload, metadata["definitions"]["uniforms"][name]);
}
bool ext::vulkan::Shader::updateUniform( const std::string& name, const ext::json::Value& payload ) {
	if ( !hasUniform(name) ) return false;

	auto* bufferObject = getUniformBuffer(name);
	if ( !bufferObject ) return false;

	auto uniform = getUniformUserdata( name, payload );	
	updateBuffer( (void*) uniform, uniform.data().len, *bufferObject );
	return true;
}

bool ext::vulkan::Shader::hasStorage( const std::string& name ) {
	return !ext::json::isNull(metadata["definitions"]["storage"][name]);
}

ext::vulkan::Buffer* ext::vulkan::Shader::getStorageBuffer( const std::string& name ) {
	if ( !hasStorage(name) ) return NULL;
	size_t storageIndex = metadata["definitions"]["storage"][name]["index"].as<size_t>();
	for ( size_t bufferIndex = 0, storageCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers[bufferIndex].usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) ) continue;
		if ( storageCounter++ != storageIndex ) continue;
		return &buffers[bufferIndex];
	}
	return NULL;
}
uf::Serializer ext::vulkan::Shader::getStorageJson( const std::string& name, bool cache ) {
	if ( !hasStorage(name) ) return ext::json::null();
	if ( cache && !ext::json::isNull(metadata["storage"][name]) ) return metadata["storage"][name];
	auto& definition = metadata["definitions"]["storage"][name];
	if ( cache ) return metadata["storage"][name] = definitionToJson(definition);
	return definitionToJson(definition);
}
ext::vulkan::userdata_t ext::vulkan::Shader::getStorageUserdata( const std::string& name, const ext::json::Value& payload ) {
	if ( !hasStorage(name) ) return false;
	return jsonToUserdata(payload, metadata["definitions"]["storage"][name]);
}

void ext::vulkan::Pipeline::initialize( Graphic& graphic ) {
	return this->initialize( graphic, graphic.descriptor );
}
void ext::vulkan::Pipeline::initialize( Graphic& graphic, GraphicDescriptor& descriptor ) {
	this->device = graphic.device;
	Device& device = *graphic.device;

	// VK_VALIDATION_MESSAGE(&graphic << ": Shaders: " << graphic.material.shaders.size() << " Textures: " << graphic.material.textures.size());
	assert( graphic.material.shaders.size() > 0 );

	RenderMode& renderMode = ext::vulkan::getRenderMode( descriptor.renderMode, true);
	auto& renderTarget = renderMode.getRenderTarget(  descriptor.renderTarget );
	{
		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
		std::vector<VkPushConstantRange> pushConstantRanges;
		std::vector<VkDescriptorPoolSize> poolSizes;
		std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes;

		for ( auto& shader : graphic.material.shaders ) {
			descriptorSetLayoutBindings.insert( descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.end() );

			std::size_t offset = 0;
			for ( auto& pushConstant : shader.pushConstants ) {
				size_t len = pushConstant.data().len;
				if ( len <= 0 || len > device.properties.limits.maxPushConstantsSize ) {
					VK_VALIDATION_MESSAGE("Invalid push constant length of " << len << " for shader " << shader.filename);
				//	goto PIPELINE_INITIALIZATION_INVALID;
					len = device.properties.limits.maxPushConstantsSize;
				}
				pushConstantRanges.push_back(ext::vulkan::initializers::pushConstantRange(
					shader.descriptor.stage,
					len,
					offset
				));
				offset += len;
			}
		}
		for ( auto& descriptor : descriptorSetLayoutBindings ) {
			if ( descriptorTypes.count( descriptor.descriptorType ) < 0 ) descriptorTypes[descriptor.descriptorType] = 0;
			descriptorTypes[descriptor.descriptorType] += descriptor.descriptorCount;
		}
		for ( auto pair : descriptorTypes ) {
			poolSizes.push_back(ext::vulkan::initializers::descriptorPoolSize(pair.first, pair.second));
		}
		VkDescriptorPoolCreateInfo descriptorPoolInfo = ext::vulkan::initializers::descriptorPoolCreateInfo(
			poolSizes.size(),
			poolSizes.data(),
			1
		);
		
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));


		VkDescriptorSetLayoutCreateInfo descriptorLayout = ext::vulkan::initializers::descriptorSetLayoutCreateInfo(
			descriptorSetLayoutBindings.data(),
			descriptorSetLayoutBindings.size()
		);

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout( device, &descriptorLayout, nullptr, &descriptorSetLayout ));

		VkDescriptorSetAllocateInfo allocInfo = ext::vulkan::initializers::descriptorSetAllocateInfo(
			descriptorPool,
			&descriptorSetLayout,
			1
		);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = ext::vulkan::initializers::pipelineLayoutCreateInfo(
			&descriptorSetLayout,
			1
		);
		pPipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
		pPipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}
	// Compute
	for ( auto& shader : graphic.material.shaders ) {
		if ( shader.descriptor.stage != VK_SHADER_STAGE_COMPUTE_BIT ) continue;

		// Create compute shader pipelines
		VkComputePipelineCreateInfo computePipelineCreateInfo = ext::vulkan::initializers::computePipelineCreateInfo(
			pipelineLayout,
			0
		);
		computePipelineCreateInfo.stage = shader.descriptor;
		VK_CHECK_RESULT(vkCreateComputePipelines(device, device.pipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipeline));
	
		return;
	}
	// Graphic
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = ext::vulkan::initializers::pipelineInputAssemblyStateCreateInfo(
			descriptor.topology,
			0,
			VK_FALSE
		);
		VkPipelineRasterizationStateCreateInfo rasterizationState = ext::vulkan::initializers::pipelineRasterizationStateCreateInfo(
			descriptor.fill,
			descriptor.cullMode, //	VK_CULL_MODE_NONE,
			descriptor.frontFace,
			0
		);
		rasterizationState.lineWidth = graphic.descriptor.lineWidth;
		rasterizationState.depthBiasEnable = graphic.descriptor.depth.bias.enable;
		rasterizationState.depthBiasConstantFactor = graphic.descriptor.depth.bias.constant;
		rasterizationState.depthBiasSlopeFactor = graphic.descriptor.depth.bias.slope;
		rasterizationState.depthBiasClamp = graphic.descriptor.depth.bias.clamp;

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;

		if ( renderMode.getType() != "Swapchain" ) {
			auto& subpass = renderTarget.passes[descriptor.subpass];
			for ( auto& color : subpass.colors ) {
				blendAttachmentStates.push_back(renderTarget.attachments[color.attachment].blendState);
			}
			// require blending if independentBlend is not an enabled feature
			if ( device.enabledFeatures.independentBlend == VK_FALSE ) {
				for ( size_t i = 1; i < blendAttachmentStates.size(); ++i ) {
					blendAttachmentStates[i] = blendAttachmentStates[0];
				}
			}
		} else {
			descriptor.subpass = 0;

			VkBool32 blendEnabled = VK_FALSE;
			VkColorComponentFlags writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

			if ( true ) {
				blendEnabled = VK_TRUE;
				writeMask |= VK_COLOR_COMPONENT_A_BIT;
			}

			VkPipelineColorBlendAttachmentState blendAttachmentState = ext::vulkan::initializers::pipelineColorBlendAttachmentState(
				writeMask,
				blendEnabled
			);
			if ( blendEnabled == VK_TRUE ) {
				blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
				blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
			}
			blendAttachmentStates.push_back(blendAttachmentState);
		}

		VkPipelineColorBlendStateCreateInfo colorBlendState = ext::vulkan::initializers::pipelineColorBlendStateCreateInfo(
			blendAttachmentStates.size(),
			blendAttachmentStates.data()
		);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = ext::vulkan::initializers::pipelineDepthStencilStateCreateInfo(
			descriptor.depth.test,
			descriptor.depth.write,
			descriptor.depth.operation //VK_COMPARE_OP_LESS_OR_EQUAL
		);
		VkPipelineViewportStateCreateInfo viewportState = ext::vulkan::initializers::pipelineViewportStateCreateInfo(
			1, 1, 0
		);
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		switch ( ext::vulkan::settings::msaa ) {
			case 64: samples = VK_SAMPLE_COUNT_64_BIT; break;
			case 32: samples = VK_SAMPLE_COUNT_32_BIT; break;
			case 16: samples = VK_SAMPLE_COUNT_16_BIT; break;
			case  8: samples =  VK_SAMPLE_COUNT_8_BIT; break;
			case  4: samples =  VK_SAMPLE_COUNT_4_BIT; break;
			case  2: samples =  VK_SAMPLE_COUNT_2_BIT; break;
			default: samples =  VK_SAMPLE_COUNT_1_BIT; break;
		}
		VkPipelineMultisampleStateCreateInfo multisampleState = ext::vulkan::initializers::pipelineMultisampleStateCreateInfo(
			samples,
			0
		);
		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState = ext::vulkan::initializers::pipelineDynamicStateCreateInfo(
			dynamicStateEnables.data(),
			dynamicStateEnables.size(),
			0
		);

		// Binding description
		std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions = {
			ext::vulkan::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID, 
				descriptor.geometry.sizes.vertex, 
				VK_VERTEX_INPUT_RATE_VERTEX
			)
		};
		// Attribute descriptions
		// Describes memory layout and shader positions
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {};
		for ( auto& attribute : descriptor.geometry.attributes ) {
			auto d = ext::vulkan::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				vertexAttributeDescriptions.size(),
				attribute.format,
				attribute.offset
			);
			vertexAttributeDescriptions.push_back(d);
		}

		VkPipelineVertexInputStateCreateInfo vertexInputState = ext::vulkan::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = vertexBindingDescriptions.size();
		vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions.data();
		vertexInputState.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
		vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

		std::vector<VkPipelineShaderStageCreateInfo> shaderDescriptors;
		for ( auto& shader : graphic.material.shaders ) {
			void* s = (void*) shader.specializationConstants;
			size_t len = shader.specializationConstants.data().len;
			for ( size_t i = 0; i < len / 4; ++i ) {
				auto& payload = shader.metadata["specializationConstants"][i];
				std::string type = payload["type"].as<std::string>();
				if ( type == "int32_t" ) {
					int32_t& v = ((int32_t*) s)[i];
					// failsafe, because for some reason things break
					if ( payload["validate"].as<bool>() && v == 0 ) {
						VK_VALIDATION_MESSAGE("Specialization constant of 0 for `" << payload.dump() << "` for shader `" << shader.filename << "`");
						v = payload["value"].is<int32_t>() ? payload["value"].as<int32_t>() : payload["default"].as<int32_t>();
					}
					payload["value"] = v;
				} else if ( type == "uint32_t" ) {
					uint32_t& v = ((uint32_t*) s)[i];
					// failsafe, because for some reason things break
					if ( payload["validate"].as<bool>() && v == 0 ) {
						VK_VALIDATION_MESSAGE("Specialization constant of 0 for `" << payload.dump() << "` for shader `" << shader.filename << "`");
						v = payload["value"].is<uint32_t>() ? payload["value"].as<uint32_t>() : payload["default"].as<uint32_t>();
					}
					payload["value"] = v;
				} else if ( type == "float" ) {
					float& v = ((float*) s)[i];
					// failsafe, because for some reason things break
					if ( payload["validate"].as<bool>() && v == 0 ) {
						VK_VALIDATION_MESSAGE("Specialization constant of 0 for `" << payload.dump() << "` for shader `" << shader.filename << "`");
						v = payload["value"].is<float>() ? payload["value"].as<float>() : payload["default"].as<float>();
					}
					payload["value"] = v;
				}
			}
			VK_VALIDATION_MESSAGE("Specialization constants for shader `" << shader.filename << "`: " << shader.metadata["specializationConstants"].dump(1, '\t'));


			{
				shader.specializationInfo = {};
				shader.specializationInfo.mapEntryCount = shader.specializationMapEntries.size();
				shader.specializationInfo.pMapEntries = shader.specializationMapEntries.data();
				shader.specializationInfo.pData = (void*) shader.specializationConstants;
				shader.specializationInfo.dataSize = shader.specializationConstants.data().len;
				shader.descriptor.pSpecializationInfo = &shader.specializationInfo;
			}
			shaderDescriptors.push_back(shader.descriptor);
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = ext::vulkan::initializers::pipelineCreateInfo(
			pipelineLayout,
			renderTarget.renderPass,
			0
		);
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderDescriptors.size();
		pipelineCreateInfo.pStages = shaderDescriptors.data();
		pipelineCreateInfo.subpass = descriptor.subpass;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines( device, device.pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
		VK_VALIDATION_MESSAGE("Created graphics pipeline");
	}

	graphic.process = true;
	return;
PIPELINE_INITIALIZATION_INVALID:
	graphic.process = false;
	VK_VALIDATION_MESSAGE("Pipeline initialization invalid, updating next tick...");
	uf::thread::add( uf::thread::get("Main"), [&]() -> int {
		this->initialize( graphic, descriptor );
	return 0;}, true );
	return;
}
void ext::vulkan::Pipeline::record( Graphic& graphic, VkCommandBuffer commandBuffer, size_t pass ) {
	auto bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	for ( auto& shader : graphic.material.shaders ) {
		if ( shader.descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) {
			bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		}
		size_t offset = 0;
		for ( auto& pushConstant : shader.pushConstants ) {
			// 
			if ( ext::json::isObject( shader.metadata["definitions"]["pushConstants"]["PushConstant"] ) ) {
				if ( shader.descriptor.stage == VK_SHADER_STAGE_VERTEX_BIT ) {
					struct PushConstant {
						uint32_t pass;
					} pushConstant = { pass };
					vkCmdPushConstants( commandBuffer, pipelineLayout, shader.descriptor.stage, 0, sizeof(pushConstant), &pushConstant );
				}
			} else {
				size_t len = pushConstant.data().len;
				void* pointer = pushConstant.data().data;
				if ( len > 0 && pointer ) {
					vkCmdPushConstants( commandBuffer, pipelineLayout, shader.descriptor.stage, 0, len, pointer );
				}
			}
		}
	}
	// Bind descriptor sets describing shader binding points
	vkCmdBindDescriptorSets(commandBuffer, bindPoint, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	// Bind the rendering pipeline
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
}
void ext::vulkan::Pipeline::update( Graphic& graphic ) {
	return this->update( graphic, graphic.descriptor );
}
void ext::vulkan::Pipeline::update( Graphic& graphic, GraphicDescriptor& descriptor ) {
	//
	if ( descriptorSet == VK_NULL_HANDLE ) return;
	// generate fallback empty texture
	auto& emptyTexture = Texture2D::empty;

	RenderMode& renderMode = ext::vulkan::getRenderMode(descriptor.renderMode, true);
	auto& renderTarget = renderMode.getRenderTarget(descriptor.renderTarget );

	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	for ( auto& shader : graphic.material.shaders ) {
		descriptorSetLayoutBindings.insert( descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.end() );
	}

	struct {
		std::vector<VkDescriptorBufferInfo> uniform;
		std::vector<VkDescriptorBufferInfo> storage;
		std::vector<VkDescriptorImageInfo> image;
		std::vector<VkDescriptorImageInfo> sampler;
		std::vector<VkDescriptorImageInfo> input;
	} infos;

	if ( descriptor.subpass < renderTarget.passes.size() ) {
		auto& subpass = renderTarget.passes[descriptor.subpass];
		for ( auto& input : subpass.inputs ) {
			infos.input.push_back(ext::vulkan::initializers::descriptorImageInfo( 
				renderTarget.attachments[input.attachment].view,
			//	input.layout
				input.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : input.layout
			));
		}
	}

	{
		for ( auto& texture : graphic.material.textures ) {
			infos.image.emplace_back(texture.descriptor);
		}
		for ( auto& sampler : graphic.material.samplers ) {
			infos.sampler.emplace_back(sampler.descriptor.info);
		}
	}

	for ( auto& shader : graphic.material.shaders ) {
		#define PARSE_BUFFER( buffers ) for ( auto& buffer : buffers ) {\
			if ( buffer.usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) {\
				infos.uniform.emplace_back(buffer.descriptor);\
			}\
			if ( buffer.usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT ) {\
				infos.storage.emplace_back(buffer.descriptor);\
			}\
		}

		PARSE_BUFFER(shader.buffers)
		PARSE_BUFFER(graphic.buffers)

		// check if we can even consume that many infos
		size_t consumes = 0;
		for ( auto& layout : shader.descriptorSetLayoutBindings ) {
			switch ( layout.descriptorType ) {
				// consume an texture image info
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
					consumes += layout.descriptorCount;
				} break;
			}
		}
		for ( size_t i = infos.image.size(); i < consumes; ++i ) {
			 infos.image.push_back(emptyTexture.descriptor);
		}
	}

	auto uniformBufferInfo = infos.uniform.begin();
	auto storageBufferInfo = infos.storage.begin();
	auto imageInfo = infos.image.begin();
	auto samplerInfo = infos.sampler.begin();
	auto inputInfo = infos.input.begin();

	#define BREAK_ASSERT(condition, ...) if ( condition ) { VK_VALIDATION_MESSAGE(#condition << "\t" << __VA_ARGS__); break; }
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	for ( auto& shader : graphic.material.shaders ) {
	//	std::cout << shader.filename << ": " << std::endl;
	//	std::cout << "\tAVAILABLE UNIFORM BUFFERS: " << infos.uniform.size() << std::endl;
	//	std::cout << "\tAVAILABLE STORAGE BUFFERS: " << infos.storage.size() << std::endl;
	//	std::cout << "\tCONSUMING : " << shader.descriptorSetLayoutBindings.size() << std::endl;
		for ( auto& layout : shader.descriptorSetLayoutBindings ) {
	//		VK_VALIDATION_MESSAGE(shader.filename << "\tType: " << layout.descriptorType << "\tConsuming: " << layout.descriptorCount);
			switch ( layout.descriptorType ) {
				// consume an texture image info
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
				//	std::cout << "\tINSERTING IMAGE" << std::endl;
					BREAK_ASSERT( imageInfo == infos.image.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*imageInfo),
						layout.descriptorCount
					));
					imageInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
				//	std::cout << "\tINSERTING INPUT_ATTACHMENT" << std::endl;
					BREAK_ASSERT( inputInfo == infos.input.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*inputInfo),
						layout.descriptorCount
					));
					inputInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_SAMPLER: {
				//	std::cout << "\tINSERTING SAMPLER" << std::endl;
					BREAK_ASSERT( samplerInfo == infos.sampler.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*samplerInfo),
						layout.descriptorCount
					));
					samplerInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
				//	std::cout << "\tINSERTING UNIFORM_BUFFER" << std::endl;
					BREAK_ASSERT( uniformBufferInfo == infos.uniform.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*uniformBufferInfo),
						layout.descriptorCount
					));
					uniformBufferInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
				//	std::cout << "\tINSERTING STORAGE_BUFFER" << std::endl;
					BREAK_ASSERT( storageBufferInfo == infos.storage.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*storageBufferInfo),
						layout.descriptorCount
					));
					storageBufferInfo += layout.descriptorCount;
				} break;
			}

		}
	}

	for ( auto& descriptor : writeDescriptorSets ) {
		for ( size_t i = 0; i < descriptor.descriptorCount; ++i ) {
			if ( descriptor.pBufferInfo ) {
				if ( descriptor.pBufferInfo[i].offset % device->properties.limits.minUniformBufferOffsetAlignment != 0 ) {
					VK_VALIDATION_MESSAGE("Invalid descriptor for buffer: " << descriptor.pBufferInfo[i].buffer << " (Offset: " << descriptor.pBufferInfo[i].offset << ", Range: " << descriptor.pBufferInfo[i].range << "), invalidating...");
					goto PIPELINE_UPDATE_INVALID;
				}
			}
			if ( descriptor.pImageInfo ) {
				if ( descriptor.pImageInfo[i].imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) {
					VK_VALIDATION_MESSAGE("Image layout is VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, fixing to VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL");
					auto pointer = const_cast<VkDescriptorImageInfo*>(&descriptor.pImageInfo[i]);
					pointer->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
				if ( descriptor.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && !descriptor.pImageInfo[i].sampler ) {
					VK_VALIDATION_MESSAGE("Image descriptor type is VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, yet lacks a sampler, adding default sampler...");
					auto pointer = const_cast<VkDescriptorImageInfo*>(&descriptor.pImageInfo[i]);
					pointer->sampler = emptyTexture.sampler.sampler;
				}
			}
		}
	}
	renderMode.rebuild = true;
	graphic.process = true;

	vkUpdateDescriptorSets(
		*device,
		writeDescriptorSets.size(),
		writeDescriptorSets.data(),
		0,
		NULL
	);
	return;

PIPELINE_UPDATE_INVALID:
	graphic.process = false;
	VK_VALIDATION_MESSAGE("Pipeline update invalid, updating next tick...");
	uf::thread::add( uf::thread::get("Main"), [&]() -> int {
		this->update( graphic, descriptor );
	return 0;}, true );
	return;
}
void ext::vulkan::Pipeline::destroy() {
	if ( aliased ) return;

	if ( descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool( *device, descriptorPool, nullptr );
		descriptorPool = VK_NULL_HANDLE;
	}
	if ( pipelineLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( *device, pipelineLayout, nullptr );
		pipelineLayout = VK_NULL_HANDLE;
	}
	if ( pipeline != VK_NULL_HANDLE ) {
		vkDestroyPipeline( *device, pipeline, nullptr );
		pipeline = VK_NULL_HANDLE;
	}
	if ( descriptorSetLayout != VK_NULL_HANDLE ) {
		vkDestroyDescriptorSetLayout( *device, descriptorSetLayout, nullptr );
		descriptorSetLayout = VK_NULL_HANDLE;
	}
}

void ext::vulkan::Material::initialize( Device& device ) {
	this->device = &device;
}
void ext::vulkan::Material::destroy() {
	if ( aliased ) return;

	for ( auto& texture : textures ) texture.destroy();
	for ( auto& shader : shaders ) shader.destroy();
	for ( auto& sampler : samplers ) sampler.destroy();

	shaders.clear();
	textures.clear();
	samplers.clear();
}
void ext::vulkan::Material::attachShader( const std::string& filename, VkShaderStageFlagBits stage ) {
	auto& shader = shaders.emplace_back();
	shader.initialize( *device, filename, stage );

	// check how many samplers requested
	for ( auto& layout : shader.descriptorSetLayoutBindings ) {
		if ( layout.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER ) continue;
		Sampler& sampler = samplers.emplace_back();
		sampler.initialize( *device );
	}
	
	std::string type = "unknown";
	switch ( stage ) {
		case VK_SHADER_STAGE_VERTEX_BIT: type = "vertex"; break;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: type = "tessellation_control"; break;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: type = "tessellation_evaluation"; break;
		case VK_SHADER_STAGE_GEOMETRY_BIT: type = "geometry"; break;
		case VK_SHADER_STAGE_FRAGMENT_BIT: type = "fragment"; break;
		case VK_SHADER_STAGE_COMPUTE_BIT: type = "compute"; break;
		case VK_SHADER_STAGE_ALL_GRAPHICS: type = "all_graphics"; break;
		case VK_SHADER_STAGE_ALL: type = "all"; break;
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR: type = "raygen"; break;
		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: type = "any_hit"; break;
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: type = "closest_hit"; break;
		case VK_SHADER_STAGE_MISS_BIT_KHR: type = "miss"; break;
		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR: type = "intersection"; break;
		case VK_SHADER_STAGE_CALLABLE_BIT_KHR: type = "callable"; break;
	}
	metadata["shaders"][type]["index"] = shaders.size() - 1;
	metadata["shaders"][type]["filename"] = filename;
}
void ext::vulkan::Material::initializeShaders( const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& layout ) {
	shaders.clear(); shaders.reserve( layout.size() );
	for ( auto& request : layout ) {
		attachShader( request.first, request.second );
	}
}
bool ext::vulkan::Material::hasShader( const std::string& type ) {
	return !ext::json::isNull( metadata["shaders"][type] );
}
ext::vulkan::Shader& ext::vulkan::Material::getShader( const std::string& type ) {
	if ( !hasShader(type) ) {
		static ext::vulkan::Shader null;
		return null;
	}
	size_t index = metadata["shaders"][type]["index"].as<size_t>();
	return shaders.at(index);
}
bool ext::vulkan::Material::validate() {
	bool was = true;
	for ( auto& shader : shaders ) {
		if ( !shader.validate() ) was = false;
	}
	return was;
}
ext::vulkan::Graphic::~Graphic() {
	this->destroy();
}
void ext::vulkan::Graphic::initialize( const std::string& renderModeName ) {
	RenderMode& renderMode = ext::vulkan::getRenderMode(renderModeName, true);

	this->descriptor.renderMode = renderModeName;
	auto* device = renderMode.device;
	if ( !device ) device = &ext::vulkan::device;

	material.initialize( *device );

	ext::vulkan::Buffers::initialize( *device );
}
void ext::vulkan::Graphic::initializePipeline() {
	initializePipeline( this->descriptor, false );
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::initializePipeline( GraphicDescriptor& descriptor, bool update ) {
	auto& pipeline = pipelines[descriptor.hash()];

//	auto previous = this->descriptor;
//	this->descriptor = descriptor;
	pipeline.initialize(*this, descriptor);
	pipeline.update(*this, descriptor);
//	if ( !update ) this->descriptor = previous;

	initialized = true;
	material.validate();

	return pipeline;
}
bool ext::vulkan::Graphic::hasPipeline( GraphicDescriptor& descriptor ) {
	return pipelines.count( descriptor.hash() ) > 0;
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline() {
	return getPipeline( descriptor );
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline( GraphicDescriptor& descriptor ) {
	if ( !hasPipeline(descriptor) ) return initializePipeline( descriptor );
	return pipelines[descriptor.hash()];
}
void ext::vulkan::Graphic::updatePipelines() {
	for ( auto pair : this->pipelines ) {
		pair.second.update( *this );
	}
}
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer, size_t pass ) {
	return this->record( commandBuffer, descriptor, pass );
}
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer, GraphicDescriptor& descriptor, size_t pass ) {
	if ( !process ) return;
	if ( !this->hasPipeline( descriptor ) ) {
		VK_VALIDATION_MESSAGE(this << ": has no valid pipeline");
		return;
	}

	auto& pipeline = this->getPipeline( descriptor );
	if ( pipeline.descriptorSet == VK_NULL_HANDLE ) {
		VK_VALIDATION_MESSAGE(this << ": has no valid pipeline descriptor set");
		return;
	}

	Buffer* vertexBuffer = NULL;
	Buffer* indexBuffer = NULL;
	for ( auto& buffer : buffers ) {
		if ( buffer.usageFlags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ) vertexBuffer = &buffer;
		if ( buffer.usageFlags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT ) indexBuffer = &buffer;
	}
	assert( vertexBuffer && indexBuffer );

	pipeline.record(*this, commandBuffer, pass);
	// Bind triangle vertex buffer (contains position and colors)
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->buffer, offsets);
	// Bind triangle index buffer
	VkIndexType indicesType = VK_INDEX_TYPE_UINT32;
	switch ( descriptor.geometry.sizes.indices * 8 ) {
		case  8: indicesType = VK_INDEX_TYPE_UINT8_EXT; break;
		case 16: indicesType = VK_INDEX_TYPE_UINT16; break;
		case 32: indicesType = VK_INDEX_TYPE_UINT32; break;
		default:
			throw std::runtime_error("invalid indices size of " + std::to_string((int) descriptor.geometry.sizes.indices));
		break;
	}
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer, 0, indicesType);
	// Draw indexed triangle
	vkCmdDrawIndexed(commandBuffer, descriptor.indices, 1, 0, 0, 1);
}
void ext::vulkan::Graphic::destroy() {
	for ( auto& pair : pipelines ) pair.second.destroy();
	pipelines.clear();

	material.destroy();

	ext::vulkan::Buffers::destroy();

	ext::vulkan::states::rebuild = true;
}

bool ext::vulkan::Graphic::hasStorage( const std::string& name ) {
	for ( auto& shader : material.shaders ) {
		if ( shader.hasStorage(name) ) return true;
	}
	return false;
}
ext::vulkan::Buffer* ext::vulkan::Graphic::getStorageBuffer( const std::string& name ) {
	size_t storageIndex = -1;
	for ( auto& shader : material.shaders ) {
		if ( !shader.hasStorage(name) ) continue;
		storageIndex = shader.metadata["definitions"]["storage"][name]["index"].as<size_t>();
		break;
	}
	for ( size_t bufferIndex = 0, storageCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers[bufferIndex].usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) ) continue;
		if ( storageCounter++ != storageIndex ) continue;
		return &buffers[bufferIndex];
	}
	return NULL;
}
uf::Serializer ext::vulkan::Graphic::getStorageJson( const std::string& name, bool cache ) {
	for ( auto& shader : material.shaders ) {
		if ( !shader.hasStorage(name) ) continue;
		return shader.getStorageJson(name, cache);
	}
	return ext::json::null();
}
ext::vulkan::userdata_t ext::vulkan::Graphic::getStorageUserdata( const std::string& name, const ext::json::Value& payload ) {
	for ( auto& shader : material.shaders ) {
		if ( !shader.hasStorage(name) ) continue;
		return shader.getStorageUserdata(name, payload);
	}
	return ext::vulkan::userdata_t();
}

#include <uf/utils/string/hash.h>
void ext::vulkan::GraphicDescriptor::parse( ext::json::Value& metadata ) {
	if ( metadata["front face"].is<std::string>() ) {
		if ( metadata["front face"].as<std::string>() == "ccw" ) {
			frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		} else if ( metadata["front face"].as<std::string>() == "cw" ) {
			frontFace = VK_FRONT_FACE_CLOCKWISE;
		}
	}
	if ( metadata["cull mode"].is<std::string>() ) {
		if ( metadata["cull mode"].as<std::string>() == "back" ) {
			cullMode = VK_CULL_MODE_BACK_BIT;
		} else if ( metadata["cull mode"].as<std::string>() == "front" ) {
			cullMode = VK_CULL_MODE_FRONT_BIT;
		} else if ( metadata["cull mode"].as<std::string>() == "none" ) {
			cullMode = VK_CULL_MODE_NONE;
		} else if ( metadata["cull mode"].as<std::string>() == "both" ) {
			cullMode = VK_CULL_MODE_FRONT_AND_BACK;
		}
	}
	if ( ext::json::isObject(metadata["depth bias"]) ) {
		depth.bias.enable = VK_TRUE;
		depth.bias.constant = metadata["depth bias"]["constant"].as<float>();
		depth.bias.slope = metadata["depth bias"]["slope"].as<float>();
		depth.bias.clamp = metadata["depth bias"]["clamp"].as<float>();
	}
}
std::string ext::vulkan::GraphicDescriptor::hash() const {
	uf::Serializer serializer;

	serializer["subpass"] = subpass;
	if ( settings::experimental::individualPipelines ) serializer["renderMode"] = renderMode;
	serializer["renderTarget"] = renderTarget;
	serializer["geometry"]["sizes"]["vertex"] = geometry.sizes.vertex;
	serializer["geometry"]["sizes"]["indices"] = geometry.sizes.indices;

	{
		int i = 0;
		for ( auto& attribute : geometry.attributes ) {
			serializer["geometry"]["attributes"][i]["format"] = attribute.format;
			serializer["geometry"]["attributes"][i]["offset"] = attribute.offset;
			++i;
		}
	}
	serializer["topology"] = topology;
	serializer["cullMode"] = cullMode;
	serializer["fill"] = fill;
	serializer["lineWidth"] = lineWidth;
	serializer["frontFace"] = frontFace;
	serializer["depth"]["test"] = depth.test;
	serializer["depth"]["write"] = depth.write;
	serializer["depth"]["operation"] = depth.operation;
	serializer["depth"]["bias"]["enable"] = depth.bias.enable;
	serializer["depth"]["bias"]["constant"] = depth.bias.constant;
	serializer["depth"]["bias"]["slope"] = depth.bias.slope;
	serializer["depth"]["bias"]["clamp"] = depth.bias.clamp;

//	if ( renderMode != "Gui" ) uf::iostream << this << ": " << indices << ": " << renderMode << ": " << subpass << ": " << serializer << "\n";
//	return uf::string::sha256( serializer.serialize() );
	return serializer.dump();
}