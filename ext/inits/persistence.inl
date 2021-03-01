#pragma once

/* Read persistent data */ if ( false ) {
	struct {
		bool exists = false;
		std::string filename = uf::io::root+"/persistent.json";
	} file;
	struct {
		uf::Serializer file;
	} config;
	/* Read from file */ if (( file.exists = config.file.readFromFile(file.filename) )) {
		persistent.window.size.x = config.file["window"]["size"]["x"].as<size_t>();
		persistent.window.size.y = config.file["window"]["size"]["y"].as<size_t>();
		if ( config.file["window"]["title"] != "null" ) {
			persistent.window.title = config.file["window"]["title"].as<std::string>();
		}

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