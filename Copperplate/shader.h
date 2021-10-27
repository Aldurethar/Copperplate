#pragma once

#include <glm\ext\matrix_float4x4.hpp>

#include <string>
#include <map>

namespace Copperplate {

	enum EShaderTypes : unsigned int {
		ST_Vertex = 1,
		ST_Geometry = 2,
		ST_Fragment = 4,
		ST_Compute = 8,
		ST_VertGeom = 3,
		ST_VertFrag = 5,
		ST_GeomFrag = 6,
		ST_VertGeomFrag = 7,
	};

	class Shader {
	public:
		Shader(EShaderTypes types, const char* vertexPath, const char* geometryPath, const char* fragmentPath);	
		Shader(EShaderTypes types, const char* vertexPath, const char* geometryPath, const char* fragmentPath, const char* feedbackVarying);

		void Use();
		void UpdateUniforms();
		unsigned int GetId();

		void SetMat4(const std::string& name, const glm::mat4& value);
		void SetVec3(const std::string& name, const glm::vec3& value);
		void SetFloat(const std::string& name, const float value);

		
	protected:
		
		std::string ReadShader(const char* filePath);

		unsigned int CompileShader(EShaderTypes shaderType, const std::string& code);

		void LinkProgram(unsigned int program);

		unsigned int m_ProgramID = 0;
		std::map<std::string, glm::mat4> m_UniformMats;
		std::map<std::string, glm::vec3> m_UniformVecs;
		std::map<std::string, float> m_UniformFloats;

	private:
	};

	class ComputeShader : public Shader {
	public:
		ComputeShader(const char* shaderPath);

		void Dispatch(int sizeX, int sizeY, int sizeZ);

	private:

	};
}