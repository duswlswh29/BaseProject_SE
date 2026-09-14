#pragma once

namespace RenderShaders
{
static const char *WorldVS = R"GLSL(
#version 330 compatibility
out vec3 normalEye;
out vec4 tint;
out vec2 uv;
out vec4 shadowCoord;
uniform int effect;
uniform float effectTime;

void main()
{
    vec4 local = gl_Vertex;
    vec3 surfaceNormal = gl_Normal;
    if (effect == 1)
    {
        float a = (local.x + .5) * 6.2831853 + effectTime * 1.4;
        float b = (local.z + .5) * 6.2831853 + effectTime * .9;
        local.y += sin(a) * cos(b) * .035;
        surfaceNormal = normalize(vec3(-cos(a) * cos(b) * .22, 1, sin(a) * sin(b) * .22));
    }
    if (effect == 2)
    {
        local.x += sin(effectTime * 4.0 + local.y * 5.0) * local.y * .13;
    }
    vec4 eye = gl_ModelViewMatrix * local;
    normalEye = normalize(gl_NormalMatrix * surfaceNormal);
    tint = gl_Color;
    uv = gl_MultiTexCoord0.xy;
    shadowCoord = gl_TextureMatrix[1] * eye;
    gl_Position = gl_ProjectionMatrix * eye;
}
)GLSL";
static const char *WorldFS = R"GLSL(
#version 330 compatibility
in vec3 normalEye;
in vec4 tint;
in vec2 uv;
in vec4 shadowCoord;
uniform sampler2D materialTex;
uniform sampler2D shadowTex;
uniform int textured;
uniform float emission;
uniform float textureRepeat;
uniform int effect;
uniform float effectTime;
out vec4 result;

void main()
{
    if (effect == 2)
    {
        float flicker =
            sin(uv.y * 19.0 - effectTime * 7.0) * .06 + sin(uv.y * 31.0 - effectTime * 4.0) * .035;
        float width = (1.0 - uv.y) * .40;
        float mask =
            (1.0 - smoothstep(width - .07, width + .04, abs(uv.x - .5 + flicker))) * (1.0 - uv.y);
        if (mask < .035)
            discard;
        vec3 flame = mix(vec3(1.0, .72, .14), vec3(1.0, .16, .015), uv.y);
        result = vec4(flame * 2.8, mask);
        return;
    }
    vec3 n = normalize(normalEye);
    vec3 sun = normalize(gl_LightSource[0].position.xyz);
    float ndl = max(dot(n, sun), 0.0);
    vec3 q = shadowCoord.xyz / shadowCoord.w;
    float visibility = 1.0;
    if (q.x > 0.0 && q.x < 1.0 && q.y > 0.0 && q.y < 1.0 && q.z > 0.0 && q.z < 1.0)
    {
        float bias = max(0.00018, 0.00085 * (1.0 - ndl));
        float lit = 0.0;
        for (int y = -1; y <= 1; y++)
            for (int x = -1; x <= 1; x++)
            {
                float d = texture(shadowTex, q.xy + vec2(x, y) / 2048.0).r;
                lit += q.z - bias <= d ? 1.0 : 0.0;
            }
        visibility = lit / 9.0;
    }
    vec4 base = tint;
    if (effect == 1)
    {
        float ripple = sin((uv.x + uv.y) * 31.4159 - effectTime * 1.5) * .025;
        base = vec4(.17 + ripple, .49 + ripple, .65 + ripple, 1.0);
    }
    if (textured != 0)
        base *= texture(materialTex, uv * textureRepeat);
    vec3 lighting = vec3(.40, .45, .51) + vec3(.78, .70, .56) * ndl * mix(.26, 1.0, visibility);
    result = vec4(base.rgb * (lighting + emission), base.a);
}
)GLSL";
static const char *PostVS = R"GLSL(
#version 330 compatibility
out vec2 uv;

void main()
{
    uv = gl_MultiTexCoord0.xy;
    gl_Position = gl_Vertex;
}
)GLSL";
static const char *PostFS = R"GLSL(
#version 330 compatibility
in vec2 uv;
uniform sampler2D scene;
uniform vec2 texel;
out vec4 result;

float luma(vec3 c)
{
    return dot(c, vec3(.299, .587, .114));
}

void main()
{
    vec3 c = texture(scene, uv).rgb;
    vec3 n = texture(scene, uv + vec2(0, texel.y)).rgb;
    vec3 s = texture(scene, uv - vec2(0, texel.y)).rgb;
    vec3 e = texture(scene, uv + vec2(texel.x, 0)).rgb;
    vec3 w = texture(scene, uv - vec2(texel.x, 0)).rgb;
    float edge = max(max(luma(n), luma(s)), max(luma(e), luma(w))) -
                 min(min(luma(n), luma(s)), min(luma(e), luma(w)));
    c = mix(c, (n + s + e + w + c * 4.0) / 8.0, smoothstep(.14, .45, edge) * .42);
    vec3 bloom = vec3(0);
    for (int y = -2; y <= 2; y++)
        for (int x = -2; x <= 2; x++)
            bloom += max(texture(scene, uv + vec2(x, y) * texel * 3.0).rgb - vec3(1.0), vec3(0));
    c += bloom * .032;
    c = c / (c + vec3(.75));
    c = pow(c, vec3(1.0 / 1.9));
    c *= vec3(1.025, 1.005, .98);
    vec2 v = uv * 2.0 - 1.0;
    c *= 1.0 - .11 * dot(v, v);
    result = vec4(c, 1);
}
)GLSL";
}
