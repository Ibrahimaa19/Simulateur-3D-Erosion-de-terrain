#version 330 core

in float height;
out vec4 FragColor;

void main() {
    FragColor = vec4(height,height,height,1.); 

}