#include <glm\ext\matrix_float4x4.hpp>
#include <string>

namespace Copperplate {
	class Shader {
	public:
		Shader(const char* vertexPath, const char* fragmentPath);
		Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath);

		void Use();
		unsigned int GetId();

		void SetMat4(const std::string& name, const glm::mat4 &value);
		void SetVec3(const std::string& name, const glm::vec3& value);

	private:
		unsigned int m_ProgramID = 0;
	};
}