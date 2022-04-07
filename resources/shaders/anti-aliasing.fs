#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2DMS screenTexture;  //ovo moze biti jedini problem

uniform bool grayscaleEnabled;
uniform float SCR_WIDTH;
uniform float SCR_HEIGHT;

void main()
{
    const float gamma = 2.2f;
    ivec2 viewPortDim = ivec2(SCR_WIDTH, SCR_HEIGHT);
    ivec2 coord = ivec2(viewPortDim * TexCoords);
    vec3 sample0 = texelFetch(screenTexture, coord, 0).rgb;
    vec3 sample1 = texelFetch(screenTexture, coord, 1).rgb;
    vec3 sample2 = texelFetch(screenTexture, coord, 2).rgb;
    vec3 sample3 = texelFetch(screenTexture, coord, 3).rgb;

    vec3 col = 0.25 * (sample0 + sample1 + sample2 + sample3);

    if(grayscaleEnabled) {
        float grayscale = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
        vec3 result = pow(vec3(grayscale), vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
    }
    else{
        col = pow(col, vec3(1.0 / gamma));
        FragColor = vec4(col, 1.0);
    }
}