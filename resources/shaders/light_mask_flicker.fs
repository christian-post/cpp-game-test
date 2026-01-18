#version 330
#define MAX_LIGHTS 10
uniform int u_lightCount;
uniform vec2 u_centers[MAX_LIGHTS];
uniform float u_radii[MAX_LIGHTS];
uniform float screenWidth;
uniform float screenHeight;
uniform float time;
in vec2 fragTexCoord;
out vec4 finalColor;

// Noise functions for randomness
float hash(float n)
{
    return fract(sin(n) * 43758.5453123);
}

float noise(float x)
{
    float i = floor(x);
    float f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), f);
}

void main()
{
    vec2 fragCoord = fragTexCoord * vec2(screenWidth, screenHeight);
    float alpha = 1.0;
    
    for (int i = 0; i < u_lightCount; i++)
    {
        // Give each light a unique seed based on its index
        float seed = float(i) * 7.919;
        
        // Fire-like animation for this light
        float slowPulse = sin(time * 0.9 + seed) * 0.5 + 0.5;
        float fastFlicker = noise(time * 7.0 + seed);
        float mediumFlicker = noise(time * 2.5 + seed);
        
        // Combine animations
        float flicker = mix(mediumFlicker, fastFlicker, 0.35);
        float pulse = mix(slowPulse, flicker, 0.4);
        
        // Animate the light radius (flicker between 0.85x to 1.15x of original)
        float radiusMultiplier = 0.85 + pulse * 0.3;
        float animatedRadius = u_radii[i] * radiusMultiplier;
        
        // slight position wobble
        vec2 wobble = vec2(
            sin(time * 2.0 + seed),
            cos(time * 1.7 + seed * 1.3)
        );
        vec2 animatedCenter = u_centers[i] + wobble;
        
        // Calculate distance and light falloff
        float dist = distance(fragCoord, animatedCenter);
        float falloff = max(animatedRadius * 0.9, 1.0);
        float fade = smoothstep(animatedRadius, animatedRadius - falloff, dist);
        
        alpha *= 1.0 - fade;
    }
    
    finalColor = vec4(0.0, 0.0, 0.0, alpha * 0.95);
}