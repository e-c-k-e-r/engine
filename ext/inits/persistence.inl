#pragma once

/* Read persistent data */ {
	struct {
		bool exists = false;
		std::string filename = "cfg/persistent.json";
	} file;
	struct {
		uf::Serializer file;
	} config;
	/* Read from file */ if (( file.exists = config.file.readFromFile(file.filename) )) {
		persistent.window.size.x = config.file["window"]["size"]["x"].asUInt64();
		persistent.window.size.y = config.file["window"]["size"]["y"].asUInt64();
		if ( config.file["window"]["title"] != "null" ) {
			persistent.window.title = config.file["window"]["title"].asString();
		}

		{
			double limit = config.file["engine"]["frame limit"].asDouble();
			if ( limit != 0 ) 
				uf::thread::limiter = 1.0 / config.file["engine"]["frame limit"].asDouble();
			else uf::thread::limiter = 0;
		}
		{
			double limit = config.file["engine"]["delta limit"].asDouble();
			if ( limit != 0 ) 
				uf::physics::time::clamp = 1.0 / config.file["engine"]["delta limit"].asDouble();
			else uf::physics::time::clamp = 0;
		}
		
		uf::thread::workers = config.file["engine"]["workers"].asUInt64();
		ext::vulkan::validation = config.file["engine"]["validation"].asBool();

		/* Update window size */ {
			uf::Serializer json;
			std::string hook = "window:Resized";
			json["type"] = hook;
			json["invoker"] = "ext";
			json["window"]["size"]["x"] = persistent.window.size.x;
			json["window"]["size"]["y"] = persistent.window.size.y;
			if ( persistent.window.size.x != 0 && persistent.window.size.y != 0 )
				uf::hooks.call(hook, json);
		}
		/* Update window title */ {
		uf::Serializer json;
			std::string hook = "window:Title.Changed";
			json["type"] = hook;
			json["invoker"] = "ext";
			json["window"]["title"] = std::string(persistent.window.title);
			uf::hooks.call(hook, json);
		}
	}
}