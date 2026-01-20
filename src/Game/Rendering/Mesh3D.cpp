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
        //Logger::LogFormat("ERROR: Could not open OBJ file: %s\n", filename.c_str());
        return false;
    }

    std::vector<Vec3> tempPositions;
    std::vector<Vec3> tempNormals;
    std::vector<Vec2> tempTexCoords;
    std::string line;
    int lineNum = 0;

    //Logger::LogFormat("Loading OBJ: %s\n", filename.c_str());

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

    //Logger::LogFormat("  Parsed: %d positions, %d normals, %d faces\n",
        //(int)tempPositions.size(), (int)tempNormals.size(), (int)faces.size());

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
    //Logger::LogLine("  Fixing winding order for back-face culling...");
    for (size_t i = 0; i < vertices.size(); i += 3) {
        if (i + 2 < vertices.size()) {
            std::swap(vertices[i + 1], vertices[i + 2]);
        }
    }

    // If no normals, calculate them
    if (tempNormals.empty()) {
        //Logger::LogLine("  No normals found, calculating...");
        CalculateNormals();
    }

    CalculateBounds();

    //Logger::LogFormat("  Loaded: %d vertices, %d faces\n", GetVertexCount(), GetFaceCount());
    //Logger::LogFormat("  Bounds: (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)\n",
        //boundsMin.x, boundsMin.y, boundsMin.z,
        //boundsMax.x, boundsMax.y, boundsMax.z);
    //Logger::LogFormat("  Center: (%.2f, %.2f, %.2f)\n",
        //center.x, center.y, center.z);

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

Mesh3D Mesh3D::CreateCylinder(float radius, float height, int segments) {
    Mesh3D mesh;

    // Helper lambda to add vertex
    auto AddVertex = [&mesh](Vec3 pos, Vec3 norm, Vec2 uv) {
        Vertex v;
        v.position = pos;
        v.normal = norm;
        v.texCoord = uv;
        mesh.vertices.push_back(v);
        };

    float halfHeight = height * 0.5f;

    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / (float)segments * 2.0f * 3.14159265359f;
        float angle2 = (float)(i + 1) / (float)segments * 2.0f * 3.14159265359f;

        float x1 = cosf(angle1);
        float z1 = sinf(angle1);
        float x2 = cosf(angle2);
        float z2 = sinf(angle2);

        Vec3 bottomV1(x1 * radius, -halfHeight, z1 * radius);
        Vec3 bottomV2(x2 * radius, -halfHeight, z2 * radius);
        Vec3 bottomCenter(0, -halfHeight, 0);

        Vec3 topV1(x1 * radius, halfHeight, z1 * radius);
        Vec3 topV2(x2 * radius, halfHeight, z2 * radius);
        Vec3 topCenter(0, halfHeight, 0);

        Vec3 normalSide1(x1, 0, z1);
        Vec3 normalSide2(x2, 0, z2);

        // --- SIDE WALLS --- (FIXED WINDING ORDER)
        // First triangle: bottom1, top2, top1  (REVERSED)
        AddVertex(bottomV1, normalSide1, Vec2(0, 0));
        AddVertex(topV2, normalSide2, Vec2(1, 1));
        AddVertex(topV1, normalSide1, Vec2(0, 1));

        // Second triangle: bottom1, bottom2, top2  (REVERSED)
        AddVertex(bottomV1, normalSide1, Vec2(0, 0));
        AddVertex(bottomV2, normalSide2, Vec2(1, 0));
        AddVertex(topV2, normalSide2, Vec2(1, 1));

        // --- BOTTOM CAP --- (REVERSED - clockwise when looking from below)
        AddVertex(bottomCenter, Vec3(0, -1, 0), Vec2(0.5f, 0.5f));
        AddVertex(bottomV2, Vec3(0, -1, 0), Vec2(0, 0));
        AddVertex(bottomV1, Vec3(0, -1, 0), Vec2(1, 0));

        // --- TOP CAP --- (CORRECT - counter-clockwise when looking from above)
        AddVertex(topCenter, Vec3(0, 1, 0), Vec2(0.5f, 0.5f));
        AddVertex(topV1, Vec3(0, 1, 0), Vec2(0, 0));
        AddVertex(topV2, Vec3(0, 1, 0), Vec2(1, 0));
    }

    return mesh;
}

Mesh3D Mesh3D::CreateRock(float radius, int detail) {
    Mesh3D rock = CreateSphere(radius, 4);  // Higher base detail, NO subdivide

    for (size_t i = 0; i < rock.vertices.size(); i++) {
        Vertex& v = rock.vertices[i];

        float seed = (float)i * 0.123f;
        float noise = 0.0f;
        noise += sinf(v.position.x * 5.0f + seed) * 0.15f;
        noise += sinf(v.position.y * 6.2f + seed * 1.3f) * 0.10f;
        noise += sinf(v.position.z * 4.8f + seed * 2.1f) * 0.08f;
        noise *= 0.3f;

        v.position = v.position + v.normal * noise;

        float len = v.position.Length();
        if (len > 0.001f) {
            v.position = v.position * (radius / len);
        }
    }

    rock.CalculateNormals();

    for (Vertex& v : rock.vertices) {
        v.normal = v.normal * -1.0f;
    }

    rock.CalculateBounds();
    return rock;
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

void Mesh3D::Subdivide(int iterations) {
    for (int iter = 0; iter < iterations; iter++) {
        std::vector<Vertex> newVertices;
        std::vector<Face> newFaces;
        std::map<std::pair<int, int>, int> midpointCache;

        for (const Face& oldFace : faces) {
            // Get original vertices
            Vertex v0 = vertices[oldFace.vertexIndices[0]];
            Vertex v1 = vertices[oldFace.vertexIndices[1]];
            Vertex v2 = vertices[oldFace.vertexIndices[2]];

            // Original vertex indices
            int i0 = GetOrCreateVertex(newVertices, v0);
            int i1 = GetOrCreateVertex(newVertices, v1);
            int i2 = GetOrCreateVertex(newVertices, v2);

            // Midpoint indices
            int m01 = GetMidpoint(newVertices, midpointCache, v0, v1);
            int m12 = GetMidpoint(newVertices, midpointCache, v1, v2);
            int m20 = GetMidpoint(newVertices, midpointCache, v2, v0);

            // 4 new faces using your C-array Face constructor
            Face face0;
            face0.vertexIndices[0] = i0;
            face0.vertexIndices[1] = m01;
            face0.vertexIndices[2] = m20;
            newFaces.push_back(face0);

            Face face1;
            face1.vertexIndices[0] = m01;
            face1.vertexIndices[1] = i1;
            face1.vertexIndices[2] = m12;
            newFaces.push_back(face1);

            Face face2;
            face2.vertexIndices[0] = m12;
            face2.vertexIndices[1] = i2;
            face2.vertexIndices[2] = m20;
            newFaces.push_back(face2);

            Face face3;
            face3.vertexIndices[0] = m01;
            face3.vertexIndices[1] = m12;
            face3.vertexIndices[2] = m20;
            newFaces.push_back(face3);
        }

        vertices = std::move(newVertices);
        faces = std::move(newFaces);
        CalculateNormals();
    }
}

int Mesh3D::GetOrCreateVertex(std::vector<Vertex>& vertexList, const Vertex& v) {
    // Simple duplicate check (could optimize with hashmap)
    for (size_t i = 0; i < vertexList.size(); i++) {
        Vertex& existing = vertexList[i];
        if ((existing.position - v.position).LengthSquared() < 0.0001f) {
            return (int)i;
        }
    }
    vertexList.push_back(v);
    return (int)(vertexList.size() - 1);
}

int Mesh3D::GetMidpoint(std::vector<Vertex>& vertexList,
    std::map<std::pair<int, int>, int>& cache,
    const Vertex& v0, const Vertex& v1) {
    // Sort indices to make key unique
    int idx0 = 0, idx1 = 0;
    if (v0.position.LengthSquared() < v1.position.LengthSquared()) {
        idx0 = GetOrCreateVertex(vertexList, v0);
        idx1 = GetOrCreateVertex(vertexList, v1);
    }
    else {
        idx1 = GetOrCreateVertex(vertexList, v0);
        idx0 = GetOrCreateVertex(vertexList, v1);
    }

    std::pair<int, int> key = { idx0, idx1 };
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }

    // Create midpoint
    Vertex mid;
    mid.position = (v0.position + v1.position) * 0.5f;
    mid.normal = (v0.normal + v1.normal).Normalized();
    mid.texCoord = (v0.texCoord + v1.texCoord) * 0.5f;

    int newIdx = GetOrCreateVertex(vertexList, mid);
    cache[key] = newIdx;
    return newIdx;
}

void Mesh3D::CalculateNormals() {
    for (Vertex& v : vertices) {
        v.normal = Vec3(0, 0, 0);
    }

    for (const Face& f : faces) {
        Vertex& v0 = vertices[f.vertexIndices[0]];
        Vertex& v1 = vertices[f.vertexIndices[1]];
        Vertex& v2 = vertices[f.vertexIndices[2]];

        Vec3 edge1 = v1.position - v0.position;
        Vec3 edge2 = v2.position - v0.position;
        Vec3 faceNormal = edge1.Cross(edge2).Normalized();

        v0.normal = v0.normal + faceNormal;
        v1.normal = v1.normal + faceNormal;
        v2.normal = v2.normal + faceNormal;
    }

    for (Vertex& v : vertices) {
        v.normal = v.normal.Normalized();
    }
}
