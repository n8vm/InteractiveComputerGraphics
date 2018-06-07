#include "OBJMesh.hpp"

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif

#include "tinyobjloader.h"
#include "Tools/HashCombiner.hpp"

#include <glm/glm.hpp>

namespace std
{
	template <>
	struct hash<Components::Meshes::MeshInterface::Vertex>
	{
		size_t operator()(const Components::Meshes::MeshInterface::Vertex& k) const
		{
			std::size_t h = 0;
			hash_combine(h, k.point.x, k.point.y, k.point.z, 
				k.color.x, k.color.y, k.color.z, k.color.a,
				k.normal.x, k.normal.y, k.normal.z,
				k.texcoord.x, k.texcoord.y);
			return h;
		}
	};
}

namespace Components::Meshes {
	void OBJMesh::loadFromOBJ(std::string objPath) {
		struct stat st;
		if (stat(objPath.c_str(), &st) != 0) {
			std::cout << objPath + " does not exist!" << std::endl;
			objPath = ResourcePath "Defaults/missing-model.obj";
		}

		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, objPath.c_str())) {
			throw std::runtime_error(err);
		}

		std::vector<Vertex> vertices;
		
		/* If the mesh has a set of shapes, merge them all into one */
		if (shapes.size() > 0) {
			for (const auto& shape : shapes) {
				for (const auto& index : shape.mesh.indices) {
					Vertex vertex = Vertex();
					vertex.point = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};
					if (attrib.colors.size() != 0) {
						vertex.color = {
							attrib.colors[3 * index.vertex_index + 0],
							attrib.colors[3 * index.vertex_index + 1],
							attrib.colors[3 * index.vertex_index + 2],
							1.f
						};
					}
					if (attrib.normals.size() != 0) {
						vertex.normal = {
							attrib.normals[3 * index.normal_index + 0],
							attrib.normals[3 * index.normal_index + 1],
							attrib.normals[3 * index.normal_index + 2]
						};
					}
					if (attrib.texcoords.size() != 0) {
						vertex.texcoord = {
							attrib.texcoords[2 * index.texcoord_index + 0],
							attrib.texcoords[2 * index.texcoord_index + 1]
						};
					}
					vertices.push_back(vertex);
				}
			}
		}

		/* If the obj has no shapes, eg polylines, then try looking for per vertex data */
		else if (shapes.size() == 0) {
			for (int idx = 0; idx < attrib.vertices.size() / 3; ++idx) {
				Vertex v = Vertex();
				v.point = glm::vec3(attrib.vertices[(idx * 3)], attrib.vertices[(idx * 3) + 1], attrib.vertices[(idx * 3) + 2]);
				if (attrib.normals.size() != 0) {
					v.normal = glm::vec3(attrib.normals[(idx * 3)], attrib.normals[(idx * 3) + 1], attrib.normals[(idx * 3) + 2]);
				}
				if (attrib.colors.size() != 0) {
					v.normal = glm::vec3(attrib.colors[(idx * 3)], attrib.colors[(idx * 3) + 1], attrib.colors[(idx * 3) + 2]);
				}
				if (attrib.texcoords.size() != 0) {
					v.texcoord = glm::vec2(attrib.texcoords[(idx * 2)], attrib.texcoords[(idx * 2) + 1]);
				}
				vertices.push_back(v);
			}
		}

		/* Eliminate duplicate points */
		std::unordered_map<Vertex, uint32_t> uniqueVertexMap = {};
		std::vector<Vertex> uniqueVertices;
		for (int i = 0; i < vertices.size(); ++i) {
			Vertex vertex = vertices[i];
			if (uniqueVertexMap.count(vertex) == 0) {
				uniqueVertexMap[vertex] = static_cast<uint32_t>(uniqueVertices.size());
				uniqueVertices.push_back(vertex);
			}
			indices.push_back(uniqueVertexMap[vertex]);
		}

		/* Map vertices to buffers */
		for (int i = 0; i < uniqueVertices.size(); ++i) {
			Vertex v = uniqueVertices[i];
			points.push_back(v.point);
			colors.push_back(v.color);
			normals.push_back(v.normal);
			texcoords.push_back(v.texcoord);
		}

		computeCentroid();
		createVertexBuffer();
		createColorBuffer();
		createIndexBuffer();
		createNormalBuffer();
		createTexCoordBuffer();
	}
}