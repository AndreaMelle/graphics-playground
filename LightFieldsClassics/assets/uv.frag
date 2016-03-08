#version 150

in VertexData	{
	vec2 texCoord;
} vVertexIn;

out vec4 oFragColor;

void main() {
    oFragColor = vec4(vVertexIn.texCoord, 0.0, 1.0);
}
