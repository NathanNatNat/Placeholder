#version 460 core

in vec3 v_worldPos;
in vec3 v_normal;
in vec2 v_texCoord;

uniform sampler2D u_diffuseTexture;
uniform vec3 u_diffuseColor;
uniform float u_opacity;

uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform vec3 u_skyColor;
uniform vec3 u_groundColor;
uniform vec3 u_fillLightDir;
uniform vec3 u_fillLightColor;

#ifdef ALPHA_TEST
uniform float u_alphaThreshold;
#endif

out vec4 fragColor;

void main()
{
    vec4 texColor = texture(u_diffuseTexture, v_texCoord);
    vec3 color = texColor.rgb * u_diffuseColor;

    // Hemisphere ambient
    float hemiFactor = dot(v_normal, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
    vec3 ambient = mix(u_groundColor, u_skyColor, hemiFactor);

    // Key light
    float NdotL = max(dot(v_normal, -u_lightDir), 0.0);
    vec3 diffuse = u_lightColor * NdotL;

    // Fill light
    float NdotF = max(dot(v_normal, -u_fillLightDir), 0.0);
    vec3 fill = u_fillLightColor * NdotF;

    color *= (ambient + diffuse + fill);

    float alpha = texColor.a * u_opacity;

#ifdef ALPHA_TEST
    if (alpha < u_alphaThreshold)
        discard;
#endif

    fragColor = vec4(color, alpha);
}
