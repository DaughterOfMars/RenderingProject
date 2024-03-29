#include "Material.hpp"
#include <iostream>
#include <stb_image.h>
#include <functional>

Material::Material(tinyobj::material_t material) : material(material) {
	loadTextures();
}

void Material::draw(GeometryPipeline * pipeline, unsigned int numInstances, unsigned int instanceVBO) {

	// Ideally we should be sending the whole material struct, but since it doesn't match up with our shader...oh well

	glUniform1f(pipeline->getAttributeLocation("metallic"), material.metallic);
	//glUniform1f(attrLocation("subsurface"), material.subsurface); // Subsurface is apparently not loaded by tinyobj?
	//glUniform1f(attrLocation("specular"), material.specular); // Specular is loaded as a vec3???
	glUniform1f(pipeline->getAttributeLocation("roughness"), material.roughness);
	//glUniform1f(attrLocation("specularTint"), material.specularTint); // SpecularTint not loaded...
	glUniform1f(pipeline->getAttributeLocation("anisotropic"), material.anisotropy);
	glUniform1f(pipeline->getAttributeLocation("sheen"), material.sheen);
	//glUniform1f(attrLocation("sheenTint"), material.sheenTint); // SheenTint not loaded
	glUniform1f(pipeline->getAttributeLocation("clearcoat"), material.clearcoat_thickness);
	glUniform1f(pipeline->getAttributeLocation("clearcoatGloss"), material.clearcoat_roughness);

	glVertexArrayVertexBuffer(pipeline->vao.ID, 0, bufferIDs[0], 0, sizeof(Vertex));

	glVertexArrayVertexBuffer(pipeline->vao.ID, 1, instanceVBO, 0, sizeof(glm::mat4));

	auto texture = textures[material.diffuse_texname];
	if (texture) texture->draw(pipeline->getAttributeLocation("texSampler"));

	pipeline->vao.bind();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIDs[1]);
	glDrawElementsInstanced(GL_TRIANGLES, indexSize, GL_UNSIGNED_INT, (void*)0, numInstances);
}

void Material::createBuffer(unsigned int * bufferID, GLsizeiptr size, const void * data) {
	glGenBuffers(1, bufferID);
	glBindBuffer(GL_ARRAY_BUFFER, *bufferID);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

void Material::createIndexBuffer(unsigned int * bufferID, GLsizeiptr size, const void * data) {
	glGenBuffers(1, bufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *bufferID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

void Material::bindVertexBuffer() {
	glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[0]);
}

void Material::createBuffers() {
	//std::cout << "Creating model VBOs in OpenGL" << std::endl;

	//create vertex buffer
	createBuffer(&bufferIDs[0], vertices.size() * sizeof(Vertex), vertices.data());

	//create index buffer
	createIndexBuffer(&bufferIDs[1], indices.size() * sizeof(unsigned int), indices.data());

	indexSize = indices.size();

	vertices.clear();
	indices.clear();
}

void Material::loadTexture(std::string filepath) {
	int height, width, channels;
	unsigned char * pixels;

	auto texture = std::make_shared<Texture>(textures.size());
	pixels = stbi_load(filepath.c_str(), &width, &height, &channels, 4);
	texture->push(pixels, width, height, channels);
	textures[filepath] = texture;
}

void Material::loadTextures() {
	if (material.diffuse_texname != "") {
		loadTexture(material.diffuse_texname);
	}
	// Load the rest of the textures if we want them
}

Material::~Material() {
	//glDeleteBuffers(2, bufferIDs);
}