#include "main.h"
#include <uf/utils/io/iostream.h>
#include <uf/utils/time/time.h>

#include <uf/utils/math/quaternion.h>
#include <uf/utils/math/glm.h>

#include <uf/engine/entity/entity.h>
int main(int argc, char** argv){
/*
	srand(time(NULL));

	uf::Serializer json; json.readFromFile("cfg/noise.json");
	struct {
		pod::Vector2ui size;
		
		struct {
			std::size_t passes;
			pod::Vector3d coefficients;
			double amp;
			std::string form;
		} noise;

		struct {
			std::string mode;
			uf::Image gradient;
			struct {
				pod::Vector3d min;
				pod::Vector3d max;
			} interpolate;
		} color;
		struct {
			pod::Vector3d position;
			std::string salt;
		} seed;
	} settings; {
		settings.size.x = json["size"]["x"].asUInt64();
		settings.size.y = json["size"]["y"].asUInt64();

		settings.noise.passes = json["noise"]["passes"].asUInt64();
		settings.noise.coefficients.x = json["noise"]["coefficients"]["x"].asDouble();
		settings.noise.coefficients.y = json["noise"]["coefficients"]["y"].asDouble();
		settings.noise.coefficients.z = json["noise"]["coefficients"]["z"].asDouble();
		settings.noise.amp = json["noise"]["amp"].asDouble();
		settings.noise.form = json["noise"]["form"].asString();

		settings.color.mode = json["color"]["mode"].asString();
		settings.color.gradient.open(json["color"]["gradient"].asString());
		settings.color.interpolate.min.x = json["color"]["interpolate"]["min"]["r"].asDouble();
		settings.color.interpolate.min.y = json["color"]["interpolate"]["min"]["g"].asDouble();
		settings.color.interpolate.min.z = json["color"]["interpolate"]["min"]["b"].asDouble();
		settings.color.interpolate.max.x = json["color"]["interpolate"]["max"]["r"].asDouble();
		settings.color.interpolate.max.y = json["color"]["interpolate"]["max"]["g"].asDouble();
		settings.color.interpolate.max.z = json["color"]["interpolate"]["max"]["b"].asDouble();
		
		settings.seed.position.x = json["seed"]["position"]["x"].asDouble();
		settings.seed.position.y = json["seed"]["position"]["y"].asDouble();
		settings.seed.position.z = json["seed"]["position"]["z"].asDouble();
		settings.seed.salt = json["seed"]["salt"].asString();
	} {
		uf::iostream << "size" << ": " << settings.size.x << ", ";
		uf::iostream << settings.size.y << "\n";

		uf::iostream << "\n";
		
		uf::iostream << "noise.passes" << ": " << settings.noise.passes << "\n";
		uf::iostream << "noise.coefficients" << ": " << settings.noise.coefficients.x << ", ";
		uf::iostream << settings.noise.coefficients.y << ", ";
		uf::iostream << settings.noise.coefficients.z  << "\n";
		uf::iostream << "noise.amp" << ": " << settings.noise.amp << "\n";
		uf::iostream << "noise.form" << ": " << settings.noise.form << "\n";

		uf::iostream << "\n";
		
		uf::iostream << "color.mode" << ": " << settings.color.mode << "\n";
		uf::iostream << "color.interpolate.min" << ": " << settings.color.interpolate.min.x << ", ";
		uf::iostream << settings.color.interpolate.min.y << ", ";
		uf::iostream << settings.color.interpolate.min.z << "\n";
		uf::iostream << "color.interpolate.max" << ": " << settings.color.interpolate.max.x << ", ";
		uf::iostream << settings.color.interpolate.max.y << ", ";
		uf::iostream << settings.color.interpolate.max.z << "\n";
		
		uf::iostream << "\n";

		uf::iostream << "seed.position" << ": " << settings.seed.position.x << ", ";
		uf::iostream << settings.seed.position.y << ", ";
		uf::iostream << settings.seed.position.z << "\n";
		uf::iostream << "seed.salt" << ": " << settings.seed.salt << "\n";

		uf::iostream << "\n";
	}

	struct {
		std::vector<std::size_t> seeds;
		std::vector<uf::PerlinNoise> perlins;
		std::vector<std::vector<double>> noises;
		
		uf::Image image;
		uf::Image::container_t& pixels = image.getPixels();
	} data; {
		data.seeds.resize(settings.noise.passes);
		data.perlins.resize(settings.noise.passes);
		data.noises.resize(settings.noise.passes);

		data.image.getDimensions() = settings.size;
		data.pixels.reserve( settings.size.x * settings.size.y );
	}


	for ( std::size_t i = 0; i < settings.noise.passes; ++i ) {
		double r = (i>0)?rand():1.0;

		pod::Vector3d position;
		position.x = settings.seed.position.x * r;
		position.y = settings.seed.position.y * r;
		position.z = settings.seed.position.z * r;

		std::size_t hashedSalt = std::hash<std::string>{}(settings.seed.salt);
		pod::Vector3t<std::size_t> hashed;
		hashed.x = std::hash<double>{}(position.x);
		hashed.y = std::hash<double>{}(position.y);
		hashed.z = std::hash<double>{}(position.z);		
		data.seeds[i] = ( settings.seed.salt != "" ) ? hashed.x^hashed.y^hashed.z^hashedSalt : hashed.x^hashed.y^hashed.z;

		uf::iostream << "pos : " << position.x << ", " << position.y << ", " << position.z << "\n";
		uf::iostream << "hash: " << hashed.x << ", " << hashed.y << ", " << hashed.z << "\n";
		uf::iostream << "seed: " << data.seeds[i] << "\n";
		uf::iostream << "\n";
	}
	uf::iostream << "\n";

	for ( std::size_t i = 0; i < settings.noise.passes; ++i ) {
		data.perlins[i].seed(data.seeds[i]);
		std::vector<double>& noise = (data.noises[i] = data.perlins[i].collect( settings.size, settings.noise.coefficients, [&](double&d)->double{
			if ( settings.noise.form == "wood" ) {
				d *= 20;
				d = d - floor(d);
			}
			else {
				d *= settings.noise.amp;
			}
			return d;
		} ));
		data.pixels.clear(), data.pixels.reserve( settings.size.x * settings.size.y );;

		for ( uint j = 0; j < data.noises[i].size(); ++j ) {
			double n = std::max(0.0,std::min(1.0,noise.at(j)));

			pod::Vector3d color;
			if ( settings.color.mode == "gradient" ) {
				bool orientation = settings.color.gradient.getDimensions().x > settings.color.gradient.getDimensions().y;
				std::size_t upper = (orientation) ? settings.color.gradient.getDimensions().x-1 : settings.color.gradient.getDimensions().y-1;
				std::size_t i = std::floor(upper * n);
				uf::Image::vec2_t position;
				position.x = (orientation) ? i : 0;
				position.y = (orientation) ? 0 : i;
				uf::Image::pixel_t pixel = settings.color.gradient.at( position );
				color.x = pixel.r / 255.0;
				color.y = pixel.g / 255.0;
				color.z = pixel.b / 255.0;
			} else if ( settings.color.mode == "lerp" ) {
				color = uf::vector::lerp(settings.color.interpolate.min, settings.color.interpolate.max, n);
			} else if ( settings.color.mode == "slerp" ) {
				color = uf::vector::slerp(settings.color.interpolate.min, settings.color.interpolate.max, n);
			}
			
			data.pixels.push_back((char) std::max(0.0, std::min(255.0, std::floor(255 * color.x))));
			data.pixels.push_back((char) std::max(0.0, std::min(255.0, std::floor(255 * color.y))));
			data.pixels.push_back((char) std::max(0.0, std::min(255.0, std::floor(255 * color.z))));
			data.pixels.push_back(255);
		}
		data.image.save("noise-"+std::to_string(i)+".png");
		data.image.open("noise-"+std::to_string(i)+".png");
		data.image.save("noise-"+std::to_string(i)+".png");
		uf::iostream << "Completed pass #" << i << "\n";
	}


	if ( settings.noise.passes > 1 ) {
		data.pixels.clear(), data.pixels.reserve( settings.size.x * settings.size.y );; 
		for ( uint i = 0; i < data.noises[0].size(); ++i ) {
			double n = settings.noise.passes;
			for ( std::size_t j = 0; j < settings.noise.passes; ++j ) n *= std::min(1.0,std::max(0.0,data.noises[j][i]));
			n = std::max(0.0,std::min(1.0,n));

			pod::Vector3d color;
			if ( settings.color.mode == "gradient" ) {
				bool orientation = settings.color.gradient.getDimensions().x > settings.color.gradient.getDimensions().y;
				std::size_t upper = (orientation) ? settings.color.gradient.getDimensions().x-1 : settings.color.gradient.getDimensions().y-1;
				std::size_t i = std::floor(upper * n);
				uf::Image::vec2_t position;
				position.x = (orientation) ? i : 0;
				position.y = (orientation) ? 0 : i;
				uf::Image::pixel_t pixel = settings.color.gradient.at( position );
				color.x = pixel.r / 255.0;
				color.y = pixel.g / 255.0;
				color.z = pixel.b / 255.0;
			} else if ( settings.color.mode == "lerp" ) {
				color = uf::vector::lerp(settings.color.interpolate.min, settings.color.interpolate.max, n);
			} else if ( settings.color.mode == "slerp" ) {
				color = uf::vector::slerp(settings.color.interpolate.min, settings.color.interpolate.max, n);
			}
			data.pixels.push_back((char) std::max(0.0, std::min(255.0, std::floor(255 * color.x))));
			data.pixels.push_back((char) std::max(0.0, std::min(255.0, std::floor(255 * color.y))));
			data.pixels.push_back((char) std::max(0.0, std::min(255.0, std::floor(255 * color.z))));
			data.pixels.push_back(255);
		}
		data.image.save("noise-combined.png");
		data.image.open("noise-combined.png");
		data.image.save("noise-combined.png");
		uf::iostream << "Completed merging" << "\n";

		uf::iostream << "\n";
	}


	char x;
	uf::iostream << "Press any key to continue...";
	uf::iostream >> x;

	return 0;
*/
	std::atexit([]{
		uf::iostream << "Termination via std::atexit()!" << "\n";
		client::terminated = !(client::ready = ext::ready = false);
		client::terminate();
		ext::terminate();
	});

	client::initialize();
	ext::initialize();

	// For Multithreaded initialization
	while ( !client::ready || !ext::ready ) {
		static uf::Timer<long long> timer(false);
		static double next = 1;
		if ( !timer.running() ) timer.start();

		if ( timer.elapsed().asDouble() >= next ) {
			uf::iostream << "Waiting for " << ( client::ready ? "client" : "extension / engine" ) << " to initialize... Retrying in " << next << " seconds." << "\n";
			next *= 2;
		}
	}
	while ( client::ready && ext::ready ) {
		try {	
			client::tick();
			ext::tick();
			static bool first = false; if ( !first ) { first = true;
				uf::Serializer json;
				std::string hook = "window:Resized";
				json["type"] = hook;
				json["invoker"] = "ext";
				json["window"]["size"]["x"] = client::config["window"]["size"]["x"];
				json["window"]["size"]["y"] = client::config["window"]["size"]["y"];
				uf::hooks.call(hook, json);
			}
		} catch ( std::exception& e ) {
			uf::iostream << "ERROR: " << e.what() << "\n";
		} catch ( bool handled ) {
			if (!handled) uf::iostream << "ERROR: " << "???" << "\n";
		} catch ( ... ) {
			uf::iostream << "ERROR: " << "???" << "\n";
		}
		client::render();
		ext::render();
	}
	if ( !client::terminated ) {
		uf::iostream << "Natural termination!" << "\n";
		client::terminate();
		ext::terminate();
	}
	return 0;
}