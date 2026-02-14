#version 400

precision lowp float;

layout (location = 0) in vec2 aPos;
out vec2 position;

void main()
{
	position = aPos;
    gl_Position = vec4(aPos, 0.0, 1.0);
}