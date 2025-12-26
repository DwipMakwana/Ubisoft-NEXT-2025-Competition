//-----------------------------------------------------------------------------
// Mesh3D.cpp
//-----------------------------------------------------------------------------
#include "Mesh3D.h"
#include "../Utilities/Logger.h"
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cmath>
#include <AppSettings.h>

Mesh3D::Mesh3D()
    : boundsMin(0, 0, 0)
    , boundsMax(0, 0, 0)
    , center(0, 0, 0)
{
}

void Mesh3D::Clear() {
    vertices.clear();
    faces.clear();
    normals.clear();
    texCoords.clear();
}

bool Mesh3D::LoadFromOBJ(const std::string& filename) {
    Clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::LogFormat("ERROR: Could not open OBJ file: %s\n", filename.c_str());
        return false;
    }

    std::vector<Vec3> tempPositions;
    std::vector<Vec3> tempNormals;
    std::vector<Vec2> tempTexCoords;
    std::string line;
    int lineNum = 0;

    Logger::LogFormat("Loading OBJ: %s\n", filename.c_str());

    while (std::getline(file, line)) {
        lineNum++;
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        // Vertex position: v x y z
        if (prefix == "v") {
            Vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            tempPositions.push_back(pos);
        }
        // Vertex normal: vn x y z
        else if (prefix == "vn") {
            Vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            tempNormals.push_back(normal);
        }
        // Texture coordinate: vt u v
        else if (prefix == "vt") {
            Vec2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            tempTexCoords.push_back(texCoord);
        }
        // Face: f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
        else if (prefix == "f") {
            Face face;
            std::string vertexData;
            int vertCount = 0;

            while (iss >> vertexData && vertCount < 3) {
                // Parse formats: v, v/vt, v/vt/vn, v//vn
                int vIndex = 0, vtIndex = 0, vnIndex = 0;
                size_t slash1 = vertexData.find('/');

                if (slash1 == std::string::npos) {
                    // Format: v
                    vIndex = std::stoi(vertexData);
                }
                else {
                    size_t slash2 = vertexData.find('/', slash1 + 1);
                    vIndex = std::stoi(vertexData.substr(0, slash1));

                    if (slash2 != std::string::npos) {
                        // Format: v/vt/vn or v//vn
                        if (slash2 > slash1 + 1) {
                            vtIndex = std::stoi(vertexData.substr(slash1 + 1, slash2 - slash1 - 1));
                        }
                        vnIndex = std::stoi(vertexData.substr(slash2 + 1));
                    }
                    else {
                        // Format: v/vt
                        vtIndex = std::stoi(vertexData.substr(slash1 + 1));
                    }
                }

                // OBJ indices are 1-based, convert to 0-based
                face.vertexIndices[vertCount] = vIndex - 1;
                face.texCoordIndices[vertCount] = vtIndex > 0 ? vtIndex - 1 : -1;
                face.normalIndices[vertCount] = vnIndex > 0 ? vnIndex - 1 : -1;
                vertCount++;
            }

            if (vertCount == 3) {
                faces.push_back(face);
            }
        }
    }
    file.close();

    Logger::LogFormat("  Parsed: %d positions, %d normals, %d faces\n",
        (int)tempPositions.size(), (int)tempNormals.size(), (int)faces.size());

    // Build vertex array from faces
    for (const auto& face : faces) {
        for (int i = 0; i < 3; i++) {
            Vertex vert;
            int vIdx = face.vertexIndices[i];
            if (vIdx >= 0 && vIdx < (int)tempPositions.size()) {
                vert.position = tempPositions[vIdx];
            }

            int vnIdx = face.normalIndices[i];
            if (vnIdx >= 0 && vnIdx < (int)tempNormals.size()) {
                vert.normal = tempNormals[vnIdx];
            }

            int vtIdx = face.texCoordIndices[i];
            if (vtIdx >= 0 && vtIdx < (int)tempTexCoords.size()) {
                vert.texCoord = tempTexCoords[vtIdx];
            }

            vertices.push_back(vert);
        }
    }

    // Fix winding order - OBJ files use opposite convention
    Logger::LogLine("  Fixing winding order for back-face culling...");
    for (size_t i = 0; i < vertices.size(); i += 3) {
        if (i + 2 < vertices.size()) {
            std::swap(vertices[i + 1], vertices[i + 2]);
        }
    }

    // If no normals, calculate them
    if (tempNormals.empty()) {
        Logger::LogLine("  No normals found, calculating...");
        CalculateNormals();
    }

    CalculateBounds();

    Logger::LogFormat("  Loaded: %d vertices, %d faces\n", GetVertexCount(), GetFaceCount());
    Logger::LogFormat("  Bounds: (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)\n",
        boundsMin.x, boundsMin.y, boundsMin.z,
        boundsMax.x, boundsMax.y, boundsMax.z);
    Logger::LogFormat("  Center: (%.2f, %.2f, %.2f)\n",
        center.x, center.y, center.z);

    return true;
}

void Mesh3D::CalculateBounds() {
    if (vertices.empty()) return;

    boundsMin = vertices[0].position;
    boundsMax = vertices[0].position;

    for (const auto& vert : vertices) {
        boundsMin.x = (boundsMin.x < vert.position.x) ? boundsMin.x : vert.position.x;
        boundsMin.y = (boundsMin.y < vert.position.y) ? boundsMin.y : vert.position.y;
        boundsMin.z = (boundsMin.z < vert.position.z) ? boundsMin.z : vert.position.z;
        boundsMax.x = (boundsMax.x > vert.position.x) ? boundsMax.x : vert.position.x;
        boundsMax.y = (boundsMax.y > vert.position.y) ? boundsMax.y : vert.position.y;
        boundsMax.z = (boundsMax.z > vert.position.z) ? boundsMax.z : vert.position.z;
    }

    center = (boundsMin + boundsMax) * 0.5f;
}

void Mesh3D::CalculateNormals() {
    // Calculate face normals
    for (size_t i = 0; i < vertices.size(); i += 3) {
        if (i + 2 >= vertices.size()) break;

        Vec3 v0 = vertices[i].position;
        Vec3 v1 = vertices[i + 1].position;
        Vec3 v2 = vertices[i + 2].position;

        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        Vec3 normal = edge1.Cross(edge2).Normalized();

        vertices[i].normal = normal;
        vertices[i + 1].normal = normal;
        vertices[i + 2].normal = normal;
    }
}

//-----------------------------------------------------------------------------
// Primitive Shapes
//-----------------------------------------------------------------------------

Mesh3D Mesh3D::CreateCube(float size) {
    Mesh3D mesh;
    float s = size * 0.5f;

    Vec3 corners[8] = {
        Vec3(-s, -s, -s), Vec3(s, -s, -s), Vec3(s, s, -s), Vec3(-s, s, -s),
        Vec3(-s, -s, s), Vec3(s, -s, s), Vec3(s, s, s), Vec3(-s, s, s)
    };

    int indices[36] = {
        0,1,2, 0,2,3,
        1,5,6, 1,6,2,
        5,4,7, 5,7,6,
        4,0,3, 4,3,7,
        3,2,6, 3,6,7,
        4,5,1, 4,1,0
    };

    for (int i = 0; i < 36; i++) {
        mesh.vertices.push_back(Vertex(corners[indices[i]]));
    }

    mesh.CalculateNormals();
    mesh.CalculateBounds();
    return mesh;
}

Mesh3D Mesh3D::CreateSphere(float radius, int segments) {
    Mesh3D mesh;

    for (int lat = 0; lat < segments; lat++) {
        float theta1 = lat * PI / segments;
        float theta2 = (lat + 1) * PI / segments;

        for (int lon = 0; lon < segments; lon++) {
            float phi1 = lon * 2 * PI / segments;
            float phi2 = (lon + 1) * 2 * PI / segments;

            Vec3 v1(
                radius * sinf(theta1) * cosf(phi1),
                radius * cosf(theta1),
                radius * sinf(theta1) * sinf(phi1)
            );
            Vec3 v2(
                radius * sinf(theta2) * cosf(phi1),
                radius * cosf(theta2),
                radius * sinf(theta2) * sinf(phi1)
            );
            Vec3 v3(
                radius * sinf(theta2) * cosf(phi2),
                radius * cosf(theta2),
                radius * sinf(theta2) * sinf(phi2)
            );
            Vec3 v4(
                radius * sinf(theta1) * cosf(phi2),
                radius * cosf(theta1),
                radius * sinf(theta1) * sinf(phi2)
            );

            Vertex vert1(v1);
            vert1.normal = v1.Normalized();
            mesh.vertices.push_back(vert1);

            Vertex vert2(v2);
            vert2.normal = v2.Normalized();
            mesh.vertices.push_back(vert2);

            Vertex vert3(v3);
            vert3.normal = v3.Normalized();
            mesh.vertices.push_back(vert3);

            mesh.vertices.push_back(vert1);
            mesh.vertices.push_back(vert3);

            Vertex vert4(v4);
            vert4.normal = v4.Normalized();
            mesh.vertices.push_back(vert4);
        }
    }

    mesh.CalculateBounds();
    return mesh;
}

Mesh3D Mesh3D::CreatePyramid(float baseSize, float height) {
    Mesh3D mesh;
    float hs = baseSize * 0.5f;

    Vec3 tip(0, height, 0);
    Vec3 c1(-hs, 0, -hs);
    Vec3 c2(hs, 0, -hs);
    Vec3 c3(hs, 0, hs);
    Vec3 c4(-hs, 0, hs);

    mesh.vertices.push_back(Vertex(c2));
    mesh.vertices.push_back(Vertex(tip));
    mesh.vertices.push_back(Vertex(c1));

    mesh.vertices.push_back(Vertex(c3));
    mesh.vertices.push_back(Vertex(tip));
    mesh.vertices.push_back(Vertex(c2));

    mesh.vertices.push_back(Vertex(c4));
    mesh.vertices.push_back(Vertex(tip));
    mesh.vertices.push_back(Vertex(c3));

    mesh.vertices.push_back(Vertex(c1));
    mesh.vertices.push_back(Vertex(tip));
    mesh.vertices.push_back(Vertex(c4));

    mesh.vertices.push_back(Vertex(c1));
    mesh.vertices.push_back(Vertex(c3));
    mesh.vertices.push_back(Vertex(c2));
    mesh.vertices.push_back(Vertex(c1));
    mesh.vertices.push_back(Vertex(c4));
    mesh.vertices.push_back(Vertex(c3));

    mesh.CalculateNormals();
    mesh.CalculateBounds();
    return mesh;
}

Mesh3D Mesh3D::CreatePlane(float width, float height) {
    Mesh3D mesh;
    float hw = width * 0.5f;
    float hh = height * 0.5f;

    Vec3 v1(-hw, 0, -hh);
    Vec3 v2(hw, 0, -hh);
    Vec3 v3(hw, 0, hh);
    Vec3 v4(-hw, 0, hh);
    Vec3 normal(0, 1, 0);

    Vertex vert1(v1);
    vert1.normal = normal;
    mesh.vertices.push_back(vert1);

    Vertex vert2(v2);
    vert2.normal = normal;
    mesh.vertices.push_back(vert2);

    Vertex vert3(v3);
    vert3.normal = normal;
    mesh.vertices.push_back(vert3);

    mesh.vertices.push_back(vert1);
    mesh.vertices.push_back(vert3);

    Vertex vert4(v4);
    vert4.normal = normal;
    mesh.vertices.push_back(vert4);

    mesh.CalculateBounds();
    return mesh;
}
