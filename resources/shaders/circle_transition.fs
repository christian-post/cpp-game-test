#version 330

uniform float screenWidth;
uniform float screenHeight;
uniform vec2 center;
uniform float startTime;
uniform float currentTime;
uniform float duration;
uniform int opening;

in vec2 fragTexCoord;
out vec4 finalColor;

float startRadius = max(screenWidth, screenHeight);


void main()
{
    vec2 fragCoord = fragTexCoord * vec2(screenWidth, screenHeight);

    float radius;
    if (opening)
        radius = startRadius * ((currentTime - startTime) / duration);
    else
        radius = startRadius - (startRadius * ((currentTime - startTime) / duration));

    float alpha = 1.0f;
    if (distance(fragCoord, center) < radius)
    {
        alpha = 0.0f;
    }

    finalColor = vec4(0.0, 0.0, 0.0, alpha);
}