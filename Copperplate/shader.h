class Shader {
public:
	Shader(const char* vertexPath, const char* fragmentPath);

	void use();

private:
	unsigned int m_ProgramID = 0;
};