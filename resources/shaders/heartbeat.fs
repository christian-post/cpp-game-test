#version 330
uniform float screenWidth;
uniform float screenHeight;
uniform float intensity;
uniform float softness;
uniform float time;
uniform float frequency;
in vec2 fragTexCoord;
out vec4 finalColor;

float heartbeat(float t) {
    float beat = fract(t * frequency);
    float pulse = 0.0;
    if (beat < 0.15)
        pulse = sin(beat * 3.14159 / 0.15);
    else if (beat < 0.3)
        pulse = sin((beat - 0.15) * 3.14159 / 0.15) * 0.7;
    return pulse * 0.15 + 1.0;
}

void main() {
    vec2 fragCoord = fragTexCoord * vec2(screenWidth, screenHeight);
    
    vec2 center = vec2(screenWidth * 0.5, screenHeight * 0.5);
    float dist = distance(fragCoord, center);
    
    float maxDist = length(center);
    float normalizedDist = dist / maxDist;
    
    float pulse = heartbeat(time);
    float radius = 0.8 * pulse;
    float fade = smoothstep(radius, radius - softness, normalizedDist);
    
    float alpha = (1.0 - fade) * intensity;
    
    finalColor = vec4(0.6, 0.0, 0.0, alpha);
}