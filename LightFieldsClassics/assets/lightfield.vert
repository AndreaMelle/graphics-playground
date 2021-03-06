#version 150

uniform mat4 ciModelViewProjection;

in vec4		ciPosition;
in vec2		ciTexCoord0;

out VertexData {
	vec2 texCoord;
} vVertexOut;

void main() {
  vVertexOut.texCoord = ciTexCoord0;
  gl_Position = ciModelViewProjection * ciPosition;
}
