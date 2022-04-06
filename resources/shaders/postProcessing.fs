#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2DMS screenTexture;
uniform sampler2DMS hdrBuffer;

uniform bool grayscaleEnabled;
uniform float SCR_WIDTH;
uniform float SCR_HEIGHT;

uniform bool hdr;
uniform float exposure;

void main()
{
    ivec2 viewPortDim = ivec2(SCR_WIDTH, SCR_HEIGHT);
    ivec2 coord = ivec2(viewPortDim * TexCoords);
    vec3 sample0 = texelFetch(screenTexture, coord, 0).rgb;
    vec3 sample1 = texelFetch(screenTexture, coord, 1).rgb;
    vec3 sample2 = texelFetch(screenTexture, coord, 2).rgb;
    vec3 sample3 = texelFetch(screenTexture, coord, 3).rgb;

    vec3 col = 0.25 * (sample0 + sample1 + sample2 + sample3);

    sample0 = texelFetch(hdrBuffer, coord, 0).rgb;
    sample1 = texelFetch(hdrBuffer, coord, 1).rgb;
    sample2 = texelFetch(hdrBuffer, coord, 2).rgb;
    sample3 = texelFetch(hdrBuffer, coord, 3).rgb;

    vec3 hdrColor = 0.25 * (sample0 + sample1 + sample2 + sample3);

    vec3 hdrResult;
    if(hdr)
        hdrResult = vec3(1.0) - exp(-hdrColor * exposure);
    else
        hdrResult = hdrColor;

    if(grayscaleEnabled) {
        float grayscale = (0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b + 0.2126 * hdrResult.r + 0.7152 * hdrResult.g + 0.0722 * hdrResult.b ) / 2;
        FragColor = vec4(vec3(grayscale), 1.0);
    }
    else
        FragColor = vec4(mix(col, hdrResult, 0.5), 1.0);
} 