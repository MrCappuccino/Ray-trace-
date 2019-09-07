#include "TriangleMesh.h"

TriangleMesh::TriangleMesh(const char *file) {
  std::string inputfile = file;
  bool ret =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str());

  if (!err.empty()) std::cerr << err << std::endl;

  if (!ret) exit(1);

  std::cout << "Model vertices: " << attrib.vertices.size() << std::endl;
}

double TriangleMesh::GetIntersection(const Ray &ray) {
  double distLowest = 1000000, intersection;
  bool polygon_hit = false;
  for (auto &shape : shapes) {
    size_t index_offset = 0;
    for (size_t f = 0; f < shape.mesh.num_face_vertices.size();
         ++f)  // faces (triangles)
    {
      int fv = shape.mesh.num_face_vertices[f];
      tinyobj::index_t &idx0 = shape.mesh.indices[index_offset + 0];  // v0
      tinyobj::index_t &idx1 = shape.mesh.indices[index_offset + 1];  // v1
      tinyobj::index_t &idx2 = shape.mesh.indices[index_offset + 2];  // v2

      // Vertices;
      tri.v0 = Vec3d(attrib.vertices[3 * idx0.vertex_index + 0],
                     attrib.vertices[3 * idx0.vertex_index + 1],
                     attrib.vertices[3 * idx0.vertex_index + 2]);
      tri.v1 = Vec3d(attrib.vertices[3 * idx1.vertex_index + 0],
                     attrib.vertices[3 * idx1.vertex_index + 1],
                     attrib.vertices[3 * idx1.vertex_index + 2]);
      tri.v2 = Vec3d(attrib.vertices[3 * idx2.vertex_index + 0],
                     attrib.vertices[3 * idx2.vertex_index + 1],
                     attrib.vertices[3 * idx2.vertex_index + 2]);

      // Vertex normals
      n0 = Vec3d(attrib.normals[3 * idx0.normal_index + 0],
                 attrib.normals[3 * idx0.normal_index + 1],
                 attrib.normals[3 * idx0.normal_index + 2]);
      n1 = Vec3d(attrib.normals[3 * idx1.normal_index + 0],
                 attrib.normals[3 * idx1.normal_index + 1],
                 attrib.normals[3 * idx1.normal_index + 2]);
      n2 = Vec3d(attrib.normals[3 * idx2.normal_index + 0],
                 attrib.normals[3 * idx2.normal_index + 1],
                 attrib.normals[3 * idx2.normal_index + 2]);

      intersection = tri.GetIntersection(ray, uv.x, uv.y);
      if (intersection && intersection < distLowest) {
        polygon_hit = true;
        distLowest = intersection;
        normal = n0 * (1 - uv.x - uv.y) + n1 * uv.x + n2 * uv.y;
      }
      index_offset += fv;
    }
  }
  if (polygon_hit)
    return distLowest;
  else
    return -1;
}

Vector3d TriangleMesh::GetNormalAt(const Vector3d &) { return normal; }

Vector3d TriangleMesh::GetTexCoords(Vector3d &, const Vector3d &) { return 0; }
