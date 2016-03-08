uniform sampler2D uvTex;
uniform sampler2D tex;

uniform vec2 uvScale;
uniform vec2 screenSize;

in VertexData	{
	vec2 texCoord;
} vVertexIn;

out vec4 oFragColor;

void main() {
    vec2 targetUV = texture(uvTex, gl_FragCoord.xy/screenSize).rg;

    //early reject for out of bounds
    if (targetUV.x<=0.0 || targetUV.y<=0.0 || targetUV.x>=1.0 || targetUV.y>=1.0) {
        discard;
    }

    //perform a linear interpolation between the two textures
    vec2 sampleUV = targetUV;
    vec2 dirVec = (sampleUV - vVertexIn.texCoord)*(-1.0, 1.0) + (0.5,0.5);

    vec2 uvOffset = sampleUV/uvScale;
    vec2 dirScale = dirVec * uvScale;

    vec2 minDir = floor(dirScale) / uvScale;
    vec2 maxDir = ceil(dirScale) / uvScale;
    vec2 weight = fract(dirScale);

    vec3 colour1 = texture(tex, minDir + uvOffset).rgb;
    vec3 colour2 = texture(tex, vec2(minDir.x, maxDir.y) + uvOffset).rgb;
    vec3 colour3 = texture(tex, vec2(maxDir.x, minDir.y) + uvOffset).rgb;
    vec3 colour4 = texture(tex, maxDir + uvOffset).rgb;

    vec3 colour = mix(mix(colour1, colour3, weight.x), mix(colour2, colour4, weight.x), weight.y);

    oFragColor = vec4(colour, 1.0);
}
