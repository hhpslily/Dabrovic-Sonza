#include "../Externals/Include/Include.h"

bool timer_enabled = true;
unsigned int timer_speed = 16;

float moveX = 0;
float moveY = 0;
float spinX = 0;
float spinY = 0;
glm::vec3 mouse;
glm::vec3 eye;
glm::vec3 center;
glm::vec3 up = glm::vec3(0, 1, 0);
using namespace glm;
using namespace std;

mat4 mv_matrix;
mat4 proj_matrix;

GLint um4p;
GLint um4mv;
GLint tex_location;
GLint sky_view;
GLuint skybox_vao;
GLuint tex_envmap;
GLuint program;
GLuint skybox_prog;

// FBO parameter
GLuint FBO;
GLuint depthRBO;
GLuint FBODataTexture;
GLuint program2;
GLuint window_vao;
GLuint window_buffer;
GLuint mode_location;
GLuint noiseTexture;
GLuint maskTexture;
int mode = 0;
int W;
int H;

typedef struct _Shape
{
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;

	int materialID;
	int drawCount;
} Shape;

typedef struct _Material
{
	GLuint diffuse_tex;
} Material;

// AS3
static const GLfloat window_positions[] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};

char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

void freeShaderSource(char** srcp)
{
    delete[] srcp[0];
    delete[] srcp;
}

// define a simple data structure for storing texture image raw data
typedef struct _TextureData
{
    _TextureData(void) :
        width(0),
        height(0),
        data(0)
    {
    }

    int width;
    int height;
    unsigned char* data;
} TextureData;

// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
TextureData loadPNG(const char* const pngFilepath)
{
    TextureData texture;
    int components;

    // load the texture with stb image, force RGBA (4 components required)
    stbi_uc *data = stbi_load(pngFilepath, &texture.width, &texture.height, &components, 4);

    // is the image successfully loaded?
    if (data != NULL)
    {
		// copy the raw data
        size_t dataSize = texture.width * texture.height * 4 * sizeof(unsigned char);
        texture.data = new unsigned char[dataSize];
        memcpy(texture.data, data, dataSize);

        // mirror the image vertically to comply with OpenGL convention
        for (size_t i = 0; i < texture.width; ++i)
        {
            for (size_t j = 0; j < texture.height / 2; ++j)
            {
                for (size_t k = 0; k < 4; ++k)
                {
                    size_t coord1 = (j * texture.width + i) * 4 + k;
                    size_t coord2 = ((texture.height - j - 1) * texture.width + i) * 4 + k;
                    std::swap(texture.data[coord1], texture.data[coord2]);
                }
            }
        }

        // release the loaded image
        stbi_image_free(data);
    }

    return texture;
}

vector<Shape> m_shape(50);
vector<Material> m_material(50);

void My_LoadModel()
{	
	const aiScene *scene = aiImportFile("sponza.obj", aiProcessPreset_TargetRealtime_MaxQuality);
	if (scene) {
		printf("mesh: %d mtl: %d\n", scene->mNumMeshes, scene->mNumMaterials);
	}
	else {
		printf("Error\n");
	}
	for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
		aiMaterial *material = scene->mMaterials[i];
		aiString texturePath;
		glGenTextures(1, &m_material[i].diffuse_tex);
		glBindTexture(GL_TEXTURE_2D, m_material[i].diffuse_tex);
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS) {
			TextureData t_data = loadPNG(texturePath.C_Str());
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t_data.width, t_data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t_data.data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	    else {
			TextureData default_tex = loadPNG("01_S_kap.JPG");
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, default_tex.width, default_tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, default_tex.data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		// save material
	}
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
		aiMesh *mesh = scene->mMeshes[i];
		glGenVertexArrays(1, &m_shape[i].vao);
		glBindVertexArray(m_shape[i].vao);
		glGenBuffers(1, &m_shape[i].vbo_position);
		glGenBuffers(1, &m_shape[i].vbo_normal);
		glGenBuffers(1, &m_shape[i].vbo_texcoord);
		float *position = new float[mesh->mNumVertices * 3];
		float *normal = new float[mesh->mNumVertices * 3];
		float *texcoord = new float[mesh->mNumVertices * 2];
		unsigned int *face = new unsigned int[mesh->mNumFaces * 3];
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v){
			position[3*v] = mesh->mVertices[v][0];
			position[3*v+1] = mesh->mVertices[v][1];
			position[3*v+2] = mesh->mVertices[v][2];
			normal[3*v] = mesh->mNormals[v][0];
			normal[3*v+1] = mesh->mNormals[v][1];
			normal[3*v+2] = mesh->mNormals[v][2];
			texcoord[2*v] = mesh->mTextureCoords[0][v][0];
			texcoord[2*v+1] = mesh->mTextureCoords[0][v][1];
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_shape[i].vbo_position);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices*3*sizeof(float), position, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, m_shape[i].vbo_normal);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * 3 * sizeof(float), normal, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, m_shape[i].vbo_texcoord);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * 2 * sizeof(float), texcoord, GL_STATIC_DRAW);

		glGenBuffers(1, &m_shape[i].ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_shape[i].ibo);
		
		for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
			face[3*f] = mesh->mFaces[f].mIndices[0];
			face[3*f+1] = mesh->mFaces[f].mIndices[1];
			face[3*f+2] = mesh->mFaces[f].mIndices[2];
		}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->mNumFaces * 3 * sizeof(unsigned int), face, GL_STATIC_DRAW);
		
		m_shape[i].materialID = mesh->mMaterialIndex;
		m_shape[i].drawCount = mesh->mNumFaces * 3;
	}
	aiReleaseImport(scene);
}

void My_Init()
{
    glClearColor(0.0f, 0.6f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	skybox_prog = glCreateProgram();
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** fs_source = loadShaderSource("skybox.fs.glsl");
	glShaderSource(fs, 1, fs_source, NULL);
	glCompileShader(fs);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	char** vs_source = loadShaderSource("skybox.vs.glsl");
	glShaderSource(vs, 1, vs_source, NULL);
	glCompileShader(vs);

	glAttachShader(skybox_prog, vs);
	glAttachShader(skybox_prog, fs);
	shaderLog(vs);
	shaderLog(fs);

	glLinkProgram(skybox_prog);
	glUseProgram(skybox_prog);

	sky_view = glGetUniformLocation(skybox_prog, "view_matrix");

	TextureData envmap_data = loadPNG("mountaincube.png");
	glGenTextures(1, &tex_envmap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex_envmap);
	for (int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, envmap_data.width, envmap_data.height / 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, envmap_data.data + i * (envmap_data.width * (envmap_data.height / 6) * sizeof(unsigned char) * 4));
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	delete[] envmap_data.data;

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glGenVertexArrays(1, &skybox_vao);

	program = glCreateProgram();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	um4p = glGetUniformLocation(program, "um4p");
	um4mv = glGetUniformLocation(program, "um4mv");

	glUseProgram(program);
	My_LoadModel();
	eye = glm::vec3(-10, 5, 0);
	center = glm::vec3(1, 6, -5);

	/*************** AS3 FBO ******************/
	program2 = glCreateProgram();

	GLuint vs2 = glCreateShader(GL_VERTEX_SHADER);
	char** vs2_source = loadShaderSource("filter.vs.glsl");
	char** fs2_source = loadShaderSource("filter.fs.glsl");
	
	glShaderSource(vs2, 1, vs2_source, NULL);
	glCompileShader(vs2);
	shaderLog(vs2);

	GLuint fs2 = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs2, 1, fs2_source, NULL);
	glCompileShader(fs2);
	shaderLog(fs2);

	glAttachShader(program2, vs2);
	glAttachShader(program2, fs2);
	glLinkProgram(program2);


	glGenVertexArrays(1, &window_vao);
	glBindVertexArray(window_vao);

	glGenBuffers(1, &window_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, window_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(window_positions), window_positions, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (const GLvoid*)(sizeof(GL_FLOAT) * 2));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glGenFramebuffers(1, &FBO);

	glutWarpPointer(300, 300);

	mode_location = glGetUniformLocation(program2, "mode");
	

	// load noise texture
	TextureData noise_tex = loadPNG("noise.png");
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, noise_tex.width, noise_tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, noise_tex.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load mask texture
	TextureData mask_tex = loadPNG("mask.jpg");
	glGenTextures(1, &maskTexture);
	glBindTexture(GL_TEXTURE_2D, maskTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mask_tex.width, mask_tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mask_tex.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void My_Display()
{
	/*************************** AS3 FBO ***************************/

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat one = 1.0f;

	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_DEPTH, 0, &one);

	/***************************draw sky box***************************/
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex_envmap);

	glUseProgram(skybox_prog);
	glBindVertexArray(skybox_vao);
	mat4 sky_matrix;
	glUniformMatrix4fv(sky_view, 1, GL_FALSE, value_ptr(sky_matrix));

	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);
	
	/***************************draw scene ***************************/

	glUseProgram(program);
	mouse = vec3(1, 5 - radians(spinY), radians(spinX));
	mv_matrix = lookAt(eye, center+mouse, up);
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(mv_matrix));
	for (int i = 0; i < 36; i++) {
		glBindVertexArray(m_shape[i].vao);
		int materialID = m_shape[i].materialID;
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_material[materialID].diffuse_tex);
		glUniform1i(glGetUniformLocation(program, "tex"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glUniform1i(glGetUniformLocation(program, "noiseMap"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, maskTexture);
		glUniform1i(glGetUniformLocation(program, "mask"), 2);
		glDrawElements(GL_TRIANGLES, m_shape[i].drawCount, GL_UNSIGNED_INT, 0);
	}
	
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(proj_matrix));

	/*************************** rebind FBO ***************************/

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, FBODataTexture);

	glBindVertexArray(window_vao);
	glUseProgram(program2);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform1i(mode_location, mode);
	glUniform1f(glGetUniformLocation(program2, "width"), W);
	glUniform1f(glGetUniformLocation(program2, "height"), H);
	
	float t = glutGet(GLUT_ELAPSED_TIME);
	glUniform1f(glGetUniformLocation(program2, "time"), t/1000);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(radians(60.0f), viewportAspect, 0.1f, 1000.0f);
	W = width;
	H = height;

	// If the windows is reshaped, we need to reset some settings of framebuffer
	glDeleteRenderbuffers(1, &depthRBO);
	glDeleteTextures(1, &FBODataTexture);
	glGenRenderbuffers(1, &depthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);


	// TODO :
	// (1) Generate a texture for FBO
	// (2) Bind it so that we can specify the format of the textrue
	glGenTextures(1, &FBODataTexture);
	glBindTexture(GL_TEXTURE_2D, FBODataTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// TODO :
	// (1) Bind the framebuffer with first parameter "GL_DRAW_FRAMEBUFFER" 
	// (2) Attach a renderbuffer object to a framebuffer object
	// (3) Attach a texture image to a framebuffer object
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBODataTexture, 0);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(timer_speed, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{
	moveX = x;
	moveY = y;
	if(button == GLUT_LEFT_BUTTON) {
		glUniform1f(glGetUniformLocation(program2, "mouse_x"), (float)x / (float)W);
	}
}

void mouseMove(int x, int y)
{
	float dx = x - moveX;
	float dy = y - moveY;
	//printf("dx %d, dy %d\n", dx, dy);
	spinX += dx;
	spinY += dy;
	glutPostRedisplay();
	moveX = x;
	moveY = y;
}

void My_Keyboard(unsigned char key, int x, int y)
{
	switch (key) 
	{
	case 'w':
		eye += normalize(center+mouse-eye)*0.5f;
		break;
	case 'd': 
		eye += normalize(cross(center+mouse-eye, up))*0.5f;
		center += normalize(cross(center+mouse-eye, up))*0.5f;
		break;
	case 's':
		eye -= normalize(center+mouse-eye)*0.5f;
		break;
	case 'a':
		eye += normalize(cross(up, center+mouse-eye))*0.5f;
		center += normalize(cross(up, center+mouse-eye))*0.5f;
		break;
	case 'z': // down
		eye += up;
		center += up;
		break;
	case 'x': // up
		eye -= up;
		center -= up;
		break;
	}
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch(id)
	{
	case 0:
		mode = 0;
		break;
	case 1:
		mode = 1;
		break;
	case 2:
		mode = 2;
		break;
	case 3:
		mode = 3;
		break;
	case 4:
		mode = 4;
		break;
	case 5:
		mode = 5;
		break;
	case 6:
		mode = 6;
		break;
	case 7:
		mode = 7;
		break;
	case 8:
		mode = 8;
		break;
	case 9:
		mode = 9;
		break;
	case 10:
		mode = 10;
		break;
	case 11:
		mode = 11;
		break;
	case 12:
		mode = 12;
		break;
	case 13:
		mode = 13;
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 600);
	glutCreateWindow("AS2_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
    glPrintContextInfo();
	My_Init();

	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddMenuEntry("Image Abstraction", 0);
	glutAddMenuEntry("Laplacian", 1);
	glutAddMenuEntry("Sharpen", 2);
	glutAddMenuEntry("Pixelation", 3);
	glutAddMenuEntry("Red-Blue Stereo", 4);
	glutAddMenuEntry("Fish Eye", 5);
	glutAddMenuEntry("Halftone", 6);
	glutAddMenuEntry("Water Color", 7);
	glutAddMenuEntry("Black and White", 8);
	glutAddMenuEntry("Swirl", 9);
	glutAddMenuEntry("Sin Wave", 10);
	glutAddMenuEntry("Night View", 11);
	glutAddMenuEntry("Snow", 12);
	glutAddMenuEntry("Bloom", 13);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutPassiveMotionFunc(mouseMove);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0); 

	// Enter main event loop.
	glutMainLoop();

	return 0;
}
