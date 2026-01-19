//-----------------------------------------------------------------------------
// Mesh3D.h
// 3D mesh with vertex/face data
//-----------------------------------------------------------------------------
#ifndef MESH_3D_H
#define MESH_3D_H

#include "../Utilities/MathUtils3D.h"
#include <vector>
#include <string>
#include <map>

struct Vertex {
    Vec3 position;
    Vec3 normal;    // For lighting (optional)
    Vec2 texCoord;  // For textures (optional)

    Vertex() : position(0, 0, 0), normal(0, 1, 0), texCoord(0, 0) {}
    Vertex(const Vec3& pos) : position(pos), normal(0, 1, 0), texCoord(0, 0) {}
};

struct Face {
    int vertexIndices[3];     // Triangle: 3 vertices
    int normalIndices[3];     // Optional normals
    int texCoordIndices[3];   // Optional texture coords

    Face() {
        for (int i = 0; i < 3; i++) {
            vertexIndices[i] = 0;
            normalIndices[i] = -1;
            texCoordIndices[i] = -1;
        }
    }
};

class Mesh3D {
public:
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    std::vector<Vec3> normals;
    std::vector<Vec2> texCoords;

    // Bounding box
    Vec3 boundsMin;
    Vec3 boundsMax;
    Vec3 center;

    Mesh3D();

    // Load from OBJ file
    bool LoadFromOBJ(const std::string& filename);

    // Create primitive shapes
    static Mesh3D CreateCube(float size = 1.0f);
    static Mesh3D CreateSphere(float radius = 1.0f, int segments = 16);
    static Mesh3D CreatePlane(float width = 1.0f, float height = 1.0f);
    static Mesh3D CreatePyramid(float baseSize, float height);
    static Mesh3D CreateCylinder(float radius, float height, int segments = 16);
    static Mesh3D CreateRock(float radius = 1.0f, int detail = 2);

    void Subdivide(int iterations);
    int GetOrCreateVertex(std::vector<Vertex>& vertexList, const Vertex& v);
    int GetMidpoint(std::vector<Vertex>& vertexList, std::map<std::pair<int, int>, int>& cache, const Vertex& v0, const Vertex& v1);

    // Helpers
    void CalculateBounds();
    void CalculateNormals();  // Auto-generate normals if missing
    void Clear();

    int GetVertexCount() const { return (int)vertices.size(); }
    int GetFaceCount() const { return (int)faces.size(); }
};

#endif
