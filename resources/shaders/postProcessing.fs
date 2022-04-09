#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2DMS screenTexture;
uniform sampler2DMS hdrBuffer;
uniform sampler2DMS bloomBlur;

uniform bool grayscaleEnabled;
uniform float SCR_WIDTH;
uniform float SCR_HEIGHT;

uniform bool hdr;
uniform bool bloom;
uniform float exposure;

uniform bool sharpenKernelEnabled;
uniform bool blurKernelEnabled;
uniform bool edgeDetectionKernelEnabled;
uniform bool ridgeDetectionKernelEnabled;

ivec2 offsets[9] = ivec2[](
            ivec2(-1,  1), // top-left
            ivec2( 0,    1), // top-center
            ivec2( 1,  1), // top-right
            ivec2(-1,  0),   // center-left
            ivec2( 0,    0),   // center-center
            ivec2( 1,  0),   // center-right
            ivec2(-1, -1), // bottom-left
            ivec2( 0,   -1), // bottom-center
            ivec2( 1, -1)  // bottom-right
);

float noKernel[9] = float[](
        0, 0, 0,
        0, 1, 0,
        0, 0, 0
);

float sharpenKernel[9] = float[](
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
);

float blurKernel[9] = float[](
    1.0 / 9, 1.0 / 9, 1.0 / 9,
    1.0 / 9, 1.0 / 9, 1.0 / 9,
    1.0 / 9, 1.0 / 9, 1.0 / 9
);

float edgeDetectionKernel[9] = float[](
        1, 1, 1,
        1, -8, 1,
        1, 1, 1
);

float ridgeDetectionKernel[9] = float[](
        -1, -1, -1,
        -1, 8, -1,
        -1, -1, -1
);

vec3 CalcColorWithKernel(float[9] kernel);

void main()
{
    vec3 col;
    if(sharpenKernelEnabled)
        col = CalcColorWithKernel(sharpenKernel);
    else if(blurKernelEnabled)
        col = CalcColorWithKernel(blurKernel);
    else if(edgeDetectionKernelEnabled)
        col = CalcColorWithKernel(edgeDetectionKernel);
    else if(ridgeDetectionKernelEnabled)
        col = CalcColorWithKernel(ridgeDetectionKernel);
    else
        col = CalcColorWithKernel(noKernel);

    FragColor = vec4(col, 1.0);
}

vec3 CalcColorWithKernel(float[9] kernel)
{
    ivec2 viewPortDim = ivec2(SCR_WIDTH, SCR_HEIGHT);
    ivec2 coord = ivec2(viewPortDim * TexCoords);
    vec3 sampleTexAA[9];
    vec3 sampleTexHDR[9];
    vec3 sampleTexBloom[9];
    vec3 finalColor[9];
    for(int i = 0; i < 9; i++)
    {
        vec3 sample0 = texelFetch(screenTexture, coord + offsets[i], 0).rgb;
        vec3 sample1 = texelFetch(screenTexture, coord + offsets[i], 1).rgb;
        vec3 sample2 = texelFetch(screenTexture, coord + offsets[i], 2).rgb;
        vec3 sample3 = texelFetch(screenTexture, coord + offsets[i], 3).rgb;

        sampleTexAA[i] = 0.25 * (sample0 + sample1 + sample2 + sample3);

        sample0 = texelFetch(hdrBuffer, coord + offsets[i], 0).rgb;
        sample1 = texelFetch(hdrBuffer, coord + offsets[i], 1).rgb;
        sample2 = texelFetch(hdrBuffer, coord + offsets[i], 2).rgb;
        sample3 = texelFetch(hdrBuffer, coord + offsets[i], 3).rgb;

        sampleTexHDR[i] = 0.25 * (sample0 + sample1 + sample2 + sample3);

        sample0 = texelFetch(bloomBlur, coord + offsets[i], 0).rgb;
        sample1 = texelFetch(bloomBlur, coord + offsets[i], 1).rgb;
        sample2 = texelFetch(bloomBlur, coord + offsets[i], 2).rgb;
        sample3 = texelFetch(bloomBlur, coord + offsets[i], 3).rgb;

        sampleTexBloom[i] = 0.25 * (sample0 + sample1 + sample2 + sample3);

        if(bloom)
            sampleTexHDR[i] += sampleTexBloom[i];

        vec3 hdrResult;
        if(hdr)
            hdrResult = vec3(1.0) - exp(-sampleTexHDR[i] * exposure);
        else
            hdrResult = sampleTexHDR[i];

        if(grayscaleEnabled) {
            float grayscale = (0.2126 * sampleTexAA[i].r + 0.7152 * sampleTexAA[i].g + 0.0722 * sampleTexAA[i].b + 0.2126 * hdrResult.r + 0.7152 * hdrResult.g + 0.0722 * hdrResult.b ) / 2;
            finalColor[i] = vec3(grayscale);
        }
        else
            finalColor[i] = mix(sampleTexAA[i], hdrResult, 0.5);
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
        col += finalColor[i] * kernel[i];

    return col;
}

