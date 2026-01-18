#version 330

uniform float screenWidth;
uniform float screenHeight;
uniform float vignetteIntensity;
uniform float vignetteSoftness;

in vec2 fragTexCoord;
out vec4 finalColor;

void main()
{
    vec2 fragCoord = fragTexCoord * vec2(screenWidth, screenHeight);
    
    // Calculate distance from center in pixel space
    vec2 center = vec2(screenWidth * 0.5, screenHeight * 0.5);
    float dist = distance(fragCoord, center);
    
    // Max distance to corner (for normalization)
    float maxDist = length(center);
    float normalizedDist = dist / maxDist;
    
    // Create vignette falloff
    float vignetteRadius = 0.8;
    float vignette = smoothstep(vignetteRadius, vignetteRadius - vignetteSoftness, normalizedDist);
    
    // Output darkness overlay (similar to light shader)
    // vignette is 1.0 at center, 0.0 at edges
    // So alpha should be 0.0 at center (no darkness), higher at edges
    float alpha = (1.0 - vignette) * vignetteIntensity;
    
    finalColor = vec4(0.0, 0.0, 0.0, alpha);
}