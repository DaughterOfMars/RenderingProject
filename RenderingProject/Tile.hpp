#pragma once
#include "MeshObject.hpp"
#include <glm/gtc/matrix_transform.hpp>

class SubMesh {
public:
	glm::vec3 location;
	std::shared_ptr<MeshObject> tileMesh;

	SubMesh(std::shared_ptr<MeshObject> mesh, glm::vec3 location) : tileMesh(mesh), location(location) {}

	void draw(glm::mat4 tileMvp, int mvpLoc) const {
		glm::mat4 mvp = tileMvp * glm::translate(glm::mat4(), location);
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
		tileMesh->draw();
	}
};

class Tile {
public:
	glm::vec3 location;
	std::unique_ptr<MeshObject> tileMesh;
	std::vector<SubMesh> subMeshes;

	Tile(std::unique_ptr<MeshObject> mesh, glm::vec3 location) : tileMesh(std::move(mesh)), location(location) {}

	void draw(glm::mat4 vp, const Map::MinimalPiece * piece, int mvpLoc, int xpos, int zpos) const {
		// First draw the tile
		glm::mat4 flip = glm::scale(glm::mat4(), glm::vec3(piece->flp ? -1.0f : 1.0f, 1.0f, 1.0f));
		glm::mat4 rotate = glm::rotate(glm::mat4(), (glm::pi<float>() / 2.0f) * piece->rot, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 translate = glm::translate(glm::mat4(), glm::vec3((float)xpos * 20.0f, 0.0f, (float)zpos * 20.0f));
		glm::mat4 mvp = vp * translate * rotate * flip;
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
		tileMesh->draw();
		for (const auto & mesh : subMeshes) {
			// Next draw any sub-meshes it has
			mesh.draw(mvp, mvpLoc);
		}
	}
};