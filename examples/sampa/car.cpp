#include "car.hpp"

#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>

// Explicit specialization of std::hash for Vertex
template <> struct std::hash<Vertex> {
  size_t operator()(Vertex const &vertex) const noexcept {
    auto const h1{std::hash<glm::vec3>()(vertex.position)};
    return h1;
  }
};

void Car::createBuffers() {
  // Delete previous buffers
  abcg::glDeleteBuffers(1, &m_EBO);
  abcg::glDeleteBuffers(1, &m_VBO);

  // VBO
  abcg::glGenBuffers(1, &m_VBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  abcg::glBufferData(GL_ARRAY_BUFFER,
                     sizeof(m_vertices.at(0)) * m_vertices.size(),
                     m_vertices.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // EBO
  abcg::glGenBuffers(1, &m_EBO);
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  abcg::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     sizeof(m_indices.at(0)) * m_indices.size(),
                     m_indices.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Car::loadObj(std::string_view path, bool standardize) {
  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(path.data())) {
    if (!reader.Error().empty()) {
      throw abcg::RuntimeError(
          fmt::format("Failed to load Predio {} ({})", path, reader.Error()));
    }
    throw abcg::RuntimeError(fmt::format("Failed to load Predio {}", path));
  }

  if (!reader.Warning().empty()) {
    fmt::print("Warning: {}\n", reader.Warning());
  }

  auto const &attrib{reader.GetAttrib()};
  auto const &shapes{reader.GetShapes()};

  m_vertices.clear();
  m_indices.clear();

  // A key:value map with key=Vertex and value=index
  std::unordered_map<Vertex, GLuint> hash{};

  // Loop over shapes
  for (auto const &shape : shapes) {
    // Loop over indices
    for (auto const offset : iter::range(shape.mesh.indices.size())) {
      // Access to vertex
      auto const index{shape.mesh.indices.at(offset)};

      // Vertex position
      auto const startIndex{3 * index.vertex_index};
      auto const vx{attrib.vertices.at(startIndex + 0)};
      auto const vy{attrib.vertices.at(startIndex + 1)};
      auto const vz{attrib.vertices.at(startIndex + 2)};

      Vertex const vertex{.position = {vx, vy, vz}};

      // If hash doesn't contain this vertex
      if (!hash.contains(vertex)) {
        // Add this index (size of m_vertices)
        hash[vertex] = m_vertices.size();
        // Add this vertex
        m_vertices.push_back(vertex);
      }

      m_indices.push_back(hash[vertex]);
    }
  }

  if (standardize) {
    Car::standardize();
  }

  createBuffers();
}

void Car::render(int numTriangles) const {
  abcg::glBindVertexArray(m_VAO);

  auto const numIndices{(numTriangles < 0) ? m_indices.size()
                                           : numTriangles * 3};

  abcg::glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr);

  abcg::glBindVertexArray(0);
}

void Car::setupVAO(GLuint program) {
  // Release previous VAO
  abcg::glDeleteVertexArrays(1, &m_VAO);

  // Create VAO
  abcg::glGenVertexArrays(1, &m_VAO);
  abcg::glBindVertexArray(m_VAO);

  // Bind EBO and VBO
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

  // Bind vertex attributes
  auto const positionAttribute{
      abcg::glGetAttribLocation(program, "inPosition")};
  if (positionAttribute >= 0) {
    abcg::glEnableVertexAttribArray(positionAttribute);
    abcg::glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex), nullptr);
  }

  // End of binding
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
  abcg::glBindVertexArray(0);
}

void Car::standardize() {
  // Center to origin and normalize largest bound to [-1, 1]

  // Get bounds
  glm::vec3 max(std::numeric_limits<float>::lowest());
  glm::vec3 min(std::numeric_limits<float>::max());
  for (auto const &vertex : m_vertices) {
    max = glm::max(max, vertex.position);
    min = glm::min(min, vertex.position);
  }

  // Center and scale
  auto const center{(min + max) / 2.0f};
  auto const scaling{2.0f / glm::length(max - min)};
  for (auto &vertex : m_vertices) {
    vertex.position = (vertex.position - center) * scaling;
  }
}

void Car::destroy() const {
  abcg::glDeleteBuffers(1, &m_EBO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteVertexArrays(1, &m_VAO);
}

void Car::dolly(float speed) {
  // Compute forward vector (view direction)
  auto const forward{glm::normalize(m_position - m_direction)};

  // Move eye and center forward (speed > 0) or backward (speed < 0)
  m_position += forward * speed;
  m_direction += forward * speed;
  
}

void Car::truck(float speed) {
  // Compute forward vector (view direction)
  auto const forward{glm::normalize(m_position - m_direction)};
  // Compute vector to the left
  auto const left{glm::cross(m_up, forward)};

  // Move eye and center to the left (speed < 0) or to the right (speed > 0)
  m_position -= left * speed;
  m_direction -= left * speed;
}

void Car::pan(float speed) {
  // Exemplo: girar o carro para esquerda ou direita
  glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), speed, glm::vec3(0.0f, 1.0f, 0.0f));
  m_direction = glm::vec3(rotation * glm::vec4(m_direction, 0.0f));

}
