#pragma once

/* Load camera */ {
	struct {
		uf::Serializer file;
		uf::Serializer fallback;
		uf::Serializer merged;
		uf::Serializer sys = ext::getConfig();
		struct {
			uf::Serializer file, embedded;
		} fallbacks;
	} config;
	struct {
		bool exists = false;
		std::string directory = "cfg/";
		std::string localFilename = "./camera.json";
		std::string filename = directory + localFilename;
	} file;
	/* Read from file */  {
		file.exists = config.file.readFromFile(file.filename);
	}
	/* Initialize fallback */ {
		config.fallback["fov"] = 90.0;
		config.fallback["clip"]["near"] = 0.01;
		config.fallback["clip"]["far"] = 100.0;
		config.fallback["size"] = config.sys["window"]["size"];
		config.fallback["size"]["auto"] = true;

		config.fallback["position"]["x"] = 0;
		config.fallback["position"]["y"] = 0;
		config.fallback["position"]["z"] = 0;

		config.fallback["orientation"]["x"] = 0;
		config.fallback["orientation"]["y"] = 0;
		config.fallback["orientation"]["z"] = 0;
		config.fallback["orientation"]["w"] = 1.0;

		config.fallback["up"]["x"] = 0;
		config.fallback["up"]["y"] = 1;
		config.fallback["up"]["z"] = 0;

		config.fallback["right"]["x"] = 1;
		config.fallback["right"]["y"] = 0;
		config.fallback["right"]["z"] = 0;

		config.fallback["forward"]["x"] = 0;
		config.fallback["forward"]["y"] = 0;
		config.fallback["forward"]["z"] = 1;

		config.fallback["offset"]["x"] = 0;
		config.fallback["offset"]["y"] = 0;
		config.fallback["offset"]["z"] = 0;
	}

	/* Merge */ if ( file.exists ){
		config.merged = config.file;
		config.merged.merge( config.fallback, true );
	} else {
		config.merged = config.fallback;
	}
	/* if ( file.exists ) */ {
		struct {
			pod::Math::num_t fov;
			pod::Vector2 size;
			pod::Vector2 bounds;
			pod::Vector3 offset;
			pod::Vector3 offsetScale;
		} settings;
		settings.fov 		= config.merged["fov"].asDouble();
		settings.bounds.x 	= config.merged["clip"]["near"].asDouble();
		settings.bounds.y 	= config.merged["clip"]["far"].asDouble();

		if ( config.merged["size"]["auto"].asBool() ) {
			config.merged["size"]["x"] = config.fallback["size"]["x"];
			config.merged["size"]["y"] = config.fallback["size"]["y"];
		}
		settings.size.x = config.merged["size"]["x"].asDouble();
		settings.size.y = config.merged["size"]["y"].asDouble();

		settings.offset.x = config.merged["offset"]["x"].asDouble();
		settings.offset.y = config.merged["offset"]["y"].asDouble();
		settings.offset.z = config.merged["offset"]["z"].asDouble();

		settings.offset.x = config.merged["offset"]["x"].asDouble();
		settings.offset.y = config.merged["offset"]["y"].asDouble();
		settings.offset.z = config.merged["offset"]["z"].asDouble();

		pod::Transform<>& transform = ::world.master.getPlayer().getComponent<pod::Transform<>>();
		/* Transform initialization */ {
			transform.position.x = config.merged["position"]["x"].asDouble();
			transform.position.y = config.merged["position"]["y"].asDouble();
			transform.position.z = config.merged["position"]["z"].asDouble();

			transform.orientation.x = config.merged["orientation"]["x"].asDouble();
			transform.orientation.y = config.merged["orientation"]["y"].asDouble();
			transform.orientation.z = config.merged["orientation"]["z"].asDouble();
			transform.orientation.w = config.merged["orientation"]["w"].asDouble();
		}
		
		uf::Camera& camera = ::world.master.getPlayer().getComponent<uf::Camera>();
		camera.setFov(settings.fov);
		camera.setSize(settings.size);
		camera.setOffset(settings.offset);		
		camera.update(true);

	}
	/* Write default to file */ {
	//	config.merged.writeToFile(file.filename);
	}
}