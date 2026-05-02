#version 460 core

in vec3 v_worldPos;
in vec3 v_normal;
in vec2 v_texCoord;

uniform sampler2D u_diffuseTexture;
uniform vec3 u_diffuseColor;
uniform float u_opacity;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform vec3 u_ambientColor;

#ifdef ALPHA_TEST
uniform float u_alphaThreshold;
#endif

out vec4 fragColor;

void main()
{
    vec4 texColor = texture(u_diffuseTexture, v_texCoord);
    vec3 color = texColor.rgb * u_diffuseColor;
    float alpha = texColor.a * u_opacity;

#ifdef ALPHA_TEST
    if (alpha < u_alphaThreshold)
        discard;
#endif

    vec3 normal = normalize(v_normal);
    float diffuse = max(dot(normal, -u_lightDir), 0.0);
    vec3 lighting = u_ambientColor + u_lightColor * diffuse;

    fragColor = vec4(color * lighting, alpha);
}
