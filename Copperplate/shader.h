#pragma once

#include <glm\ext\matrix_float4x4.hpp>

#include <string>
#include <map>

namespace Copperplate {
	class Shader {
	public:
		Shader(const char* vertexPath, const char* fragmentPath);
		Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath);

		void Use();
		unsigned int GetId();

		void SetMat4(const std::string& name, const glm::mat4& value);
		void SetVec3(const std::string& name, const glm::vec3& value);
		void SetFloat(const std::string& name, const float value);

		bool useDepthTest = true;
		bool writeDepth = true;

	private:
		unsigned int m_ProgramID = 0;
		std::map<std::string, glm::mat4> m_UniformMats;
		std::map<std::string, glm::vec3> m_UniformVecs;
		std::map<std::string, float> m_UniformFloats;
	};
}