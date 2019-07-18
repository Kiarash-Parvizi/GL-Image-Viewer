#include<iostream>
#define GLEW_STATIC
#include<glew.h>
#include<glfw3.h>

//Shader Class:
//----------------------------------------------------------------------
namespace Shader {

	static GLuint CompileShader(unsigned int type, const std::string& source) {
		unsigned int id = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(id, 1, &src, nullptr);
		glCompileShader(id);

		int result;
		glGetShaderiv(id, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE)
		{
			int length;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
			char* message = (char*)malloc(length);
			glGetShaderInfoLog(id, length, &length, message);
			std::cout << "!" << (type == GL_VERTEX_SHADER ? "Vert" : "Frag") << "ShaderCompiled\n";
			std::cout << "message: " << message << std::endl;
			glDeleteShader(id);
			return 0;
		}
		return id;
	}

	static GLuint Create(const std::string& vertexShader, const std::string& fragmentShader) {
		unsigned int program = glCreateProgram();
		unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
		unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		glValidateProgram(program);

		glDeleteShader(vs);
		glDeleteShader(fs);

		return program;
	}
}

///---------------------------------------------------------------------

//Texture Class:
//----------------------------------------------------------------------
#define STB_IMAGE_IMPLEMENTATION
#include"stb_image.h"

class Texture {
private:
	GLuint rendererId;
	std::string filePath;
	unsigned char* localBuffer;
	int width, height, BPP;
public:
	Texture(const std::string path) {
		stbi_set_flip_vertically_on_load(1);
		localBuffer = stbi_load(path.c_str(), &width, &height, &BPP, 4);

		glGenTextures(1, &rendererId);
		glBindTexture(GL_TEXTURE_2D, rendererId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localBuffer);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (localBuffer) {
			stbi_image_free(localBuffer);
		}
	}
	~Texture() {
		glDeleteTextures(1, &rendererId);
	}

	void Bind(GLuint slot = 0) const {
		glActiveTexture(GL_TEXTURE0+slot);
		glBindTexture(GL_TEXTURE_2D, rendererId);
	}
	void Unbind() const {
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Side : res[0] = width; res[1] = height;
	float* getScaleNormalized() {
		float* res = (float*)malloc(2*sizeof(float));
		if (this->width > this->height) {
			res[0] = 1; res[1] = (float)height / (float)width;
		}
		else {
			res[1] = 1; res[0] = (float)width / (float)height;
		}
		return res;
	}
};

///---------------------------------------------------------------------
#include<thread>
using namespace std::chrono_literals;

int main() {
	if (!glfwInit()) { std::cout << "Failed to initialize glfw\n"; exit(0); }
	GLFWwindow* window = glfwCreateWindow(600, 500, "GL ImageViewer", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK) { std::cout << "Failed to initialize glew\n"; exit(0); }
	if (!window) std::cout << "Failed to create the window\n";
	std::cout << "GL-ImageViewer by Kiarash Parvizi\n";
	glfwSwapInterval(1); glClearColor(0.2f, 0.4f, 0.8f, 1.0f);

	//0000000000000000000000000000000000000000000000000000000000000000000000000000
	// Shader
	std::string vertShader =
		"#version 330 core\n"
		"\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 texCoord;\n"
		"\n"
		"out vec2 v_TexCoord;\n"
		"uniform mat4 u_MVP;\n"
		"\n"
		"void main() {\n"
		"	v_TexCoord = texCoord;\n"
		"	gl_Position = position;\n"
		"}\n";

	std::string fragShader =
		"#version 330 core\n"
		"\n"
		"layout(location = 0) out vec4 color;\n"
		"\n"
		"in vec2 v_TexCoord;\n"
		"\n"
		"uniform sampler2D u_Texture;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	color = texture(u_Texture, v_TexCoord);\n"
		"}\n";

	GLuint texture_shader = Shader::Create(vertShader, fragShader);
	glUseProgram(texture_shader);
	///00000000000000000000000000000000000000000000000000000000000000000000000000

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Texture texture("res/Asset.png"); texture.Bind();
	glUniform1i(glGetUniformLocation(texture_shader, "u_Texture"), 0);// glUseProgram(redP_shader);

	//Scaling the Verts:
	float* scale = texture.getScaleNormalized();

	//0000000000000000000000000000000000000000000000000000000000000000000000000000
	// Verts
	float h = scale[1], w = scale[0];
	float positions[] = {
		-w, -h, 0.0, 0.0,
		-w, h, 0.0, 1.0,
		w, -h, 1.0, 0.0,
		//
		w, h, 1.0, 1.0
	};
	GLuint indices[] {
		0, 1, 2,
		1, 2, 3
	};

	GLuint vao; glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint buffer; glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(float), positions, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (char*)0 + 0 * sizeof(GLfloat));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (char*)0 + 2 * sizeof(GLfloat));

	GLuint ibo; glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indices, GL_STATIC_DRAW);
	///00000000000000000000000000000000000000000000000000000000000000000000000000

	bool newData = true; int win_width, win_height; glfwGetWindowSize(window, &win_width, &win_height);
	int preWin_width = win_width, preWin_height = win_height;
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glfwGetWindowSize(window, &win_width, &win_height);
		if (win_width != preWin_width || win_height != preWin_height) {
			preWin_width = win_width; preWin_height = win_height;
			newData = true;
		}
		if (newData) {
			glClear(GL_COLOR_BUFFER_BIT);

			//
			//glBindVertexArray(vao);
			//

			//Render
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

			//Events
			glfwSwapBuffers(window);
			newData = false;
		}
		std::this_thread::sleep_for(100ms);
	}

	//End
	glfwTerminate();
}
