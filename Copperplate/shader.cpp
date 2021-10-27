#pragma once

#include "shader.h"

#include <glad/glad.h>
#include <glm\gtc\type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


namespace Copperplate {

	Shader::Shader(EShaderTypes types, const char* vertexPath, const char* geometryPath, const char* fragmentPath) {
		bool hasVert = (types & ST_Vertex);
		bool hasGeom = (types & ST_Geometry);
		bool hasFrag = (types & ST_Fragment);

		// Read Shader Files
		std::string vertexCode, geometryCode, fragmentCode;
		if (hasVert) vertexCode = ReadShader(vertexPath);
		if (hasGeom) geometryCode = ReadShader(geometryPath);
		if (hasFrag) fragmentCode = ReadShader(fragmentPath);

		// Compile Shaders
		unsigned int vertex, geometry, fragment;
		if (hasVert) vertex = CompileShader(ST_Vertex, vertexCode);
		if (hasGeom) geometry = CompileShader(ST_Geometry, geometryCode);
		if (hasFrag) fragment = CompileShader(ST_Fragment, fragmentCode);
		
		// Create and link program
		m_ProgramID = glCreateProgram();
		if (hasVert) glAttachShader(m_ProgramID, vertex);
		if (hasGeom) glAttachShader(m_ProgramID, geometry);
		if (hasFrag) glAttachShader(m_ProgramID, fragment);
		LinkProgram(m_ProgramID);
		
		// delete the shaders as they're linked into our program now and no longer necessary
		if (hasVert) glDeleteShader(vertex);
		if (hasGeom) glDeleteShader(geometry);
		if (hasFrag) glDeleteShader(fragment);
	}

	Shader::Shader(EShaderTypes types, const char* vertexPath, const char* geometryPath, const char* fragmentPath, const char* feedbackVarying) {
		bool hasVert = (types & ST_Vertex);
		bool hasGeom = (types & ST_Geometry);
		bool hasFrag = (types & ST_Fragment);

		// Read Shader Files
		std::string vertexCode, geometryCode, fragmentCode;
		if (hasVert) vertexCode = ReadShader(vertexPath);
		if (hasGeom) geometryCode = ReadShader(geometryPath);
		if (hasFrag) fragmentCode = ReadShader(fragmentPath);

		// Compile Shaders
		unsigned int vertex, geometry, fragment;
		if (hasVert) vertex = CompileShader(ST_Vertex, vertexCode);
		if (hasGeom) geometry = CompileShader(ST_Geometry, geometryCode);
		if (hasFrag) fragment = CompileShader(ST_Fragment, fragmentCode);

		// Create program
		m_ProgramID = glCreateProgram();
		if (hasVert) glAttachShader(m_ProgramID, vertex);
		if (hasGeom) glAttachShader(m_ProgramID, geometry);
		if (hasFrag) glAttachShader(m_ProgramID, fragment);

		// Setup Transform Feedback and Link Program
		const char* feedbackVaryings[] = { feedbackVarying };
		glTransformFeedbackVaryings(m_ProgramID, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
		LinkProgram(m_ProgramID);

		// delete the shaders as they're linked into our program now and no longer necessary
		if (hasVert) glDeleteShader(vertex);
		if (hasGeom) glDeleteShader(geometry);
		if (hasFrag) glDeleteShader(fragment);
	}

	std::string Shader::ReadShader(const char* filePath) {
		std::string shaderCode;
		std::ifstream shaderFile;
		shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try {
			std::stringstream shaderStream;
			shaderFile.open(filePath);
			shaderStream << shaderFile.rdbuf();
			shaderFile.close();
			shaderCode = shaderStream.str();
		}
		catch (std::ifstream::failure e) {
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ:" << filePath << std::endl;
		}
		return shaderCode;
	}

	unsigned int Shader::CompileShader(EShaderTypes shaderType, const std::string& code) {
		int success;
		char infoLog[512];
		unsigned int shader;
		const char* shaderCode = code.c_str();

		if (shaderType == ST_Vertex) shader = glCreateShader(GL_VERTEX_SHADER);
		else if (shaderType == ST_Geometry) shader = glCreateShader(GL_GEOMETRY_SHADER);
		else if (shaderType == ST_Fragment) shader = glCreateShader(GL_FRAGMENT_SHADER);
		else if (shaderType == ST_Compute) shader = glCreateShader(GL_COMPUTE_SHADER);
		else {
			std::cout << "ERROR: CompileShader: Incompatible Shader Type\n" << std::endl;
			return 0;
		}

		glShaderSource(shader, 1, &shaderCode, NULL);
		glCompileShader(shader);
		// print compile errors if any
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shader, 512, NULL, infoLog);
			if (shaderType == ST_Vertex) std::cout << "ERROR:Vertex Shader Compilation Failed\n" << infoLog << std::endl;
			else if (shaderType == ST_Geometry) std::cout << "ERROR:Geometry Shader Compilation Failed\n" << infoLog << std::endl;
			else if (shaderType == ST_Fragment) std::cout << "ERROR:Fragment Shader Compilation Failed\n" << infoLog << std::endl;
			else if (shaderType == ST_Compute) std::cout << "ERROR:Compute Shader Compilation Failed\n" << infoLog << std::endl;
		};
		return shader;
	}

	void Shader::LinkProgram(unsigned int program) {
		int success;
		char infoLog[512];
		glLinkProgram(m_ProgramID);
		// print linking errors if any
		glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(m_ProgramID, 512, NULL, infoLog);
			std::cout << "ERROR: Shader Program Linking Failed\n" << infoLog << std::endl;
		}
	}

	void Shader::Use() {
		glUseProgram(m_ProgramID);

		UpdateUniforms();
	}

	void Shader::UpdateUniforms()
	{
		for (const auto & [name, value] : m_UniformMats) {
			int location = glGetUniformLocation(m_ProgramID, name.c_str());
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
		}

		for (const auto& [name, value] : m_UniformVecs) {
			int location = glGetUniformLocation(m_ProgramID, name.c_str());
			glUniform3fv(location, 1, glm::value_ptr(value));
		}

		for (const auto& [name, value] : m_UniformFloats) {
			int location = glGetUniformLocation(m_ProgramID, name.c_str());
			glUniform1f(location, value);
		}
	}

	void Shader::SetMat4(const std::string& name, const glm::mat4& value) {
		m_UniformMats[name] = value;		
	}

	void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
		m_UniformVecs[name] = value;
	}

	void Shader::SetFloat(const std::string& name, const float value)
	{
		m_UniformFloats[name] = value;
	}


	unsigned int Shader::GetId() {
		return m_ProgramID;
	}

	ComputeShader::ComputeShader(const char* shaderPath) 
		: Shader((EShaderTypes)0, nullptr, nullptr, nullptr){
		std::string shaderCode = ReadShader(shaderPath);
		unsigned int shader = CompileShader(ST_Compute, shaderCode);
		glAttachShader(m_ProgramID, shader);
		LinkProgram(m_ProgramID);
		glDeleteShader(shader);
	}

	void ComputeShader::Dispatch(int sizeX, int sizeY, int sizeZ) {
		glDispatchCompute(sizeX, sizeY, sizeZ);
	}
}