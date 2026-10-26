#pragma once

namespace RenderShaders
{
static const char *WorldVS = R"GLSL(
#version 330 compatibility
out vec3 normalEye;
out vec4 tint;
out vec2 uv;
out vec4 shadowCoord;
out vec3 worldPosition;
out vec3 localPosition;
out vec3 worldNormal;
uniform mat4 cameraToWorld;
uniform int effect;
uniform float effectTime;

void main()
{
    vec4 local = gl_Vertex;
    vec3 surfaceNormal = gl_Normal;
    if (effect == 1)
    {
        // Keep adjacent water tiles flush; animate pixel highlights in the fragment stage.
        surfaceNormal = vec3(0, 1, 0);
    }
    if (effect == 2)
    {
        local.x += sin(effectTime * 4.0 + local.y * 5.0) * local.y * .13;
    }
    vec4 eye = gl_ModelViewMatrix * local;
    normalEye = normalize(gl_NormalMatrix * surfaceNormal);
    worldNormal = normalize(mat3(cameraToWorld)*normalEye);
    worldPosition = (cameraToWorld*eye).xyz;
    localPosition = local.xyz;
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
in vec3 worldPosition;
in vec3 localPosition;
in vec3 worldNormal;
uniform int pointLightCount;
uniform vec4 pointLights[8];
uniform vec3 pointColors[8];
uniform sampler2D materialTex;
uniform sampler2D shadowTex;
uniform int textured;
uniform float emission;
uniform float textureRepeat;
uniform int effect;
uniform float effectTime;
out vec4 result;

float hashCell(vec3 p)
{
    return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);
}
vec3 materialDetail(vec3 base, int tag, vec3 p, vec3 n)
{
    vec3 cell=floor(p*32.0);
    float grain=hashCell(cell);
    if (tag==2)
    {
        vec2 woodUV=abs(n.y)>.65 ? p.xz : abs(n.x)>.65 ? p.zy : p.xy;
        float rings=sin(woodUV.x*95.0+sin(woodUV.y*11.0)*1.5);
        float seam=step(.94,fract(woodUV.x*5.0));
        base*=.72+.23*grain+.17*rings;
        base*=1.0-seam*.28;
    }
    if (tag==3 || tag==7)
    {
        vec2 masonryUV=abs(n.y)>.65 ? p.xz : abs(n.x)>.65 ? p.zy : p.xy;
        vec2 tileUV=masonryUV*vec2(4.0,7.0);
        tileUV.x+=mod(floor(tileUV.y),2.0)*.5;
        vec2 edge=fract(tileUV);
        float mortar=step(.93,edge.x)+step(.88,edge.y);
        float weather=hashCell(floor(p*9.0));
        base*=.77+.3*grain;
        base*=1.0-min(1.0,mortar)*.36;
        float moss=step(.73,weather)*(.25+.55*max(n.y,0.0));
        base=mix(base,vec3(.16,.24,.065),moss*.6);
    }
    if (tag==4) base*=.68+grain*.55;
    if (tag==6) base*=.92+grain*.12;
    return base;
}

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
    vec3 wn=normalize(worldNormal);
    int surfaceTag=uv.y < -99.0 ? int(floor(-uv.y-100.0+.5)) : 0;
    if (textured != 0) base*=texture(materialTex,uv*textureRepeat);
    if (surfaceTag>0) base.rgb=materialDetail(base.rgb,surfaceTag,localPosition,wn);
    if (textured==1 || surfaceTag==8)
    {
        // World-aligned pixel clusters continue across flat ground and terrace tops.
        vec2 p=worldPosition.xz;
        vec2 pixel=floor(p*8.0);
        float coarse=hashCell(vec3(floor(p*2.0),17));
        float fine=hashCell(vec3(pixel,3));
        float broad=sin(p.x*.22+sin(p.y*.19)*2.0)*cos(p.y*.27);
        vec3 darkGrass=vec3(.095,.17,.045);
        vec3 lightGrass=vec3(.23,.30,.085);
        base.rgb=mix(darkGrass,lightGrass,.3+.40*coarse+.25*(broad*.5+.5));
        base.rgb*=.75+fine*.50;
        float blade=step(.77,fine)*step(.45,coarse);
        base.rgb+=blade*vec3(.035,.045,.008);
        float soilPatch=sin(p.x*.31+sin(p.y*.24)*1.7)*cos(p.y*.23);
        float soil=smoothstep(.45,.85,soilPatch)*step(.33,coarse);
        vec3 dirt=mix(vec3(.14,.095,.048),vec3(.28,.20,.10),fine);
        base.rgb=mix(base.rgb,dirt,soil*.72);

    }
    vec3 ambient=mix(vec3(.105,.13,.16),vec3(.25,.30,.29),wn.y*.5+.5);
    vec3 lighting=ambient+vec3(1.55,1.27,.86)*ndl*mix(.07,1.0,visibility);
    vec3 specular=vec3(0);
    vec3 viewDirection=normalize(vec3(-.57735,.57735,.57735));
    if (effect==1)
    {
        vec2 p=worldPosition.xz;
        float wave=sin(p.x*2.1+effectTime*.7)*cos(p.y*1.7-effectTime*.6);
        wn=normalize(vec3(cos(p.x*2.1+effectTime*.7)*.075,1.0,sin(p.y*1.7-effectTime*.6)*.075));
        float fresnel=.04+.42*pow(1.0-max(dot(wn,viewDirection),0.0),5.0);
        vec3 reflected=reflect(-viewDirection,wn);
        vec3 sky=mix(vec3(.28,.42,.45),vec3(.55,.70,.77),max(reflected.y,0.0));
        float caustic=pow(max(0.0,sin(p.x*4.0+wave)+cos(p.y*4.5-wave))*.5,6.0);
        base=vec4(mix(vec3(.025,.20,.20)+caustic*vec3(.08,.18,.12),sky,fresnel),1);
        float sunSpark=pow(max(dot(reflect(-normalize(vec3(.329,.819,.470)),wn),viewDirection),0.0),90.0);
        specular+=vec3(1.7,1.4,.85)*sunSpark*visibility;
        lighting=ambient+vec3(.65,.75,.68)*mix(.3,1.0,visibility);
    }
    for(int i=0;i<pointLightCount;++i)
    {
        vec3 delta=pointLights[i].xyz-worldPosition;
        float distanceToLight=length(delta);
        float falloff=pow(max(0.0,1.0-distanceToLight/pointLights[i].w),2.0);
        vec3 lightDirection=delta/max(distanceToLight,.001);
        lighting+=pointColors[i]*falloff*(.12+max(dot(wn,lightDirection),0.0))*2.7;
        if(effect==1)
            specular+=pointColors[i]*falloff*pow(max(dot(reflect(-lightDirection,wn),viewDirection),0.0),32.0)*3.0;
    }
    vec3 color=base.rgb*(lighting+emission)+specular;
    if(surfaceTag==5) color=vec3(3.6,1.65,.32);
    result=vec4(color,base.a);

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
uniform sampler2D sceneDepth;
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
    float depth=texture(sceneDepth,uv).r;
    vec2 gradient=vec2(dFdx(depth),dFdy(depth));
    float occlusion=0.0;
    // Orthographic depth spans 200 world units; compare against the local tangent plane.
    for(int i=0;i<12;++i)
    {
        float angle=float(i)*2.399963;
        float radius=3.0+float(i%4)*3.0;
        vec2 pixels=vec2(cos(angle),sin(angle))*radius;
        float neighbor=texture(sceneDepth,uv+pixels*texel).r;
        float expected=depth+dot(gradient,pixels);
        float separation=(expected-neighbor)*200.0;
        occlusion+=smoothstep(.045,.30,separation)*(1.0-smoothstep(.5,1.4,separation));
    }
    if(depth<.999) c*=1.0-.48*occlusion/12.0;
    vec3 bloom=vec3(0);
    for(int y=-2;y<=2;++y)
        for(int x=-2;x<=2;++x)
            bloom+=max(texture(scene,uv+vec2(x,y)*texel*3.0).rgb-vec3(1.2),vec3(0));
    c+=bloom*.022;
    // Filmic curve retains rich shadow colors instead of lifting the whole image to grey.
    c*=1.12;
    c=clamp((c*(2.51*c+.03))/(c*(2.43*c+.59)+.14),0.0,1.0);
    c=pow(c,vec3(1.0/2.2));
    vec2 vignette=uv*2.0-1.0;
    c*=1.0-.045*dot(vignette,vignette);
    result = vec4(c, 1);
}
)GLSL";
}
