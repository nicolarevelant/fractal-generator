#version 400

in vec2 position;
out vec4 FragColor;
uniform dvec3 mb; // x, y, zoom
uniform dvec3 julia; // julia_x, julia_y, julia_zoom
uniform float ratio;  // aspect ratio, max iterations
uniform int maxItr;
uniform vec3 palette[501];

void main()
{
	dvec2 c;
	if (julia.z != 0)
		// Julia
		c = dvec2((position.x * ratio) / julia.z + julia.x,
					position.y / julia.z + julia.y);
	else
		// Mandelbrot
		c = dvec2((position.x * ratio) / mb.z + mb.x,
					position.y / mb.z + mb.y);
	dvec2 pos = c;

	int itr;
	for (itr = 0; itr < 500; itr++) {
		if (itr == maxItr || dot(pos, pos) > 4.0) break;
		if (julia.z != 0)
			// Julia
			pos = dvec2(pos.x*pos.x - pos.y*pos.y, 2.0*pos.x*pos.y) + mb.xy;
		else
			// Mandelbrot
			pos = dvec2(pos.x*pos.x - pos.y*pos.y, 2.0*pos.x*pos.y) + c;
	}

	FragColor = vec4(palette[itr], 1.0f);
}