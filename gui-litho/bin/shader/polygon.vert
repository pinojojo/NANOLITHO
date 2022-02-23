#version 330 core
layout (location = 0) in vec2 vPosition;

uniform mat4 mvp;

void main(){
	vec4 initialPos=vec4(vPosition,0.0,1.0);
	gl_Position = mvp*initialPos;
	//gl_Position = initialPos;
}