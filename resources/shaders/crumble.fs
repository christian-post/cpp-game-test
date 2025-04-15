#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float time;

out vec4 finalColor;

uniform int flipX;

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
    vec2 uv = fragTexCoord;
    vec4 texColor = texture(texture0, uv);

    if (flipX == 1) {
        uv.x = 1.0 - uv.x;
    }

    float duration = 2.0;
    float threshold = mod(time, duration) / duration * 1.5;
    float noise = rand(floor(uv * 100.0));

    float mask = step(threshold, uv.y + noise * 0.5);
    finalColor = mix(vec4(0.0), texColor, mask) * fragColor;
}
