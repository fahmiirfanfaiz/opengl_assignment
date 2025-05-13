#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

// Phong lighting model parameters
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

void main() {
    // Normalize normal vector karena bisa saja berubah setelah interpolasi
    vec3 norm = normalize(Normal);
    
    // === Ambient lighting ===
    // Komponen ambient adalah pencahayaan konstan dari segala arah
    vec3 ambient = ambientStrength * lightColor;
    
    // === Diffuse lighting ===
    // Hitung arah cahaya (dari fragment ke sumber cahaya)
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // Hitung faktor diffuse (dot product antara normal dan arah cahaya)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;
    
    // === Specular lighting ===
    // Arah pandangan (dari fragment ke kamera)
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Arah refleksi cahaya (reflect(-lightDir, norm) memberikan arah refleksi)
    vec3 reflectDir = reflect(-lightDir, norm);
    
    // Hitung faktor specular (dot product antara arah refleksi dan arah pandangan)
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Gabungkan semua komponen pencahayaan
    vec3 result = (ambient + diffuse + specular) * objectColor;
    
    FragColor = vec4(result, 1.0);
}