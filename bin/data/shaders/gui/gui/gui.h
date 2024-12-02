struct Gui {
	vec4 offset;
	vec4 color;
	
	int mode;
	float depth;
	float padding1;
	float padding2;
};

struct Glyph {
	vec4 stroke;
	
	ivec2 range;
	int spread;
	float weight;

	float scale;
	uint padding1;
	uint padding2;
	uint padding3;
};