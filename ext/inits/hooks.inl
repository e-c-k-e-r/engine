#pragma once

/* Initialize hooks */ {
	if ( persistent.hook.mode == "Both" || persistent.hook.mode == "Readable" ) {
		uf::hooks.addHook( "system:Tick", [&](const std::string& event)->std::string{
			times.prevTime = times.curTime;
			times.curTime = times.sys.elapsed().asDouble();
			times.deltaTime = times.curTime - times.prevTime;
			return "true";
		});
		uf::hooks.addHook( "window:Text.Entered", [&](const std::string& event)->std::string{
			uf::Serializer json = event;
			
			uint32_t utf32 = json["text"]["uint32_t"].asUInt64();
			if ( utf32 == '\n' || utf32 == '\r' ) io.input += '\n';
			else if ( utf32 == '\b' ) io.input--;
			else io.input += utf32;

			return "true";
		} );
		uf::hooks.addHook( "window:Title.Changed", [&](const std::string& event)->std::string{
			uf::Serializer json = event;
			if ( json["invoker"] != "ext") persistent.window.title = json["window"]["title"].asString();
			return "true";
		} );
	}
}