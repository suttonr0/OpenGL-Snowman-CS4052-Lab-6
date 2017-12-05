//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"

// Assimp includes

#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <stdio.h>

#include <math.h>
#include <vector> // STL dynamic memory.

// STB Image loader
// https://github.com/nothings/stb/blob/master/stb_image.h

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


/*----------------------------------------------------------------------------
                   MESHES TO LOAD
  ----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here

#define TREE_MESH "basicTree.dae"
#define SNOWMAN_MESH "UVSnowman.obj"
#define GROUND_MESH "Ground2.dae"
#define SNOWMAN_ARM_MESH "SnowmanArm.dae"
#define SNOWBALL_MESH "Snowball.dae"
/*----------------------------------------------------------------------------
				   TEXTURES TO LOAD
----------------------------------------------------------------------------*/

#define GROUND_TEXTURE "GroundSnow3.png"
#define SNOWMAN_TEXTURE "SnowmanTexture2.png"
#define TREE_TEXTURE "pineTree.png"
#define SNOWMAN_ARM_TEXTURE "ArmTexture2.png"
/*----------------------------------------------------------------------------
  ----------------------------------------------------------------------------*/

std::vector<float> g_vp, g_vn, g_vt;
int g_point_count = 0;
int ground_count = 0;
int tree_vertex_count = 0;
int snowman_vertex_count = 0;
int snowball_vertex_count = 0;
int snowman_arm_vertex_count = 0;


// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;

unsigned int mesh_vao = 0;

GLuint GROUND_ID = 1;
GLuint TREE_ID = 2;
GLuint SNOWMAN_ID = 3;
GLuint SNOWMAN_ARM_ID = 4;
GLuint SNOWBALL_ID = 5;

GLuint GROUND_TEX_ID = 1;
GLuint TREE_TEX_ID = 2;
GLuint SNOWMAN_TEX_ID = 3;
GLuint SNOWMAN_ARM_TEX_ID = 4;

int width = 1200;
int height = 800;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;
GLfloat translate_y = 0.0f;
GLfloat camerarotationy = 0.0f;

// Snowman AI 
bool armSwitch = true;
GLfloat armAngle = 0.0f;
GLfloat snowman1_rotationy = 0.0;
GLfloat marchDistance = 0.0;
bool marchOrder = true;
bool fleeing = false;
GLfloat fleeTime = 0.0;

// Snowball parameters
bool thrownSnowball = false;  // Has the snowball been thrown?
vec3 snowballDir = vec3(0.0f, 0.0f, 0.0f);  // Direction to throw the snowball
GLfloat snowballGravity = 0.0f;
vec3 hitLocation = vec3(0.0f, 0.0f, 0.0f); 

vec3 snowman1Pos = vec3(-10.0f, 0.0f, -10.0f);
vec3 snowman2Pos = vec3(-5.0f, 0.0f, -10.0f);
vec3 snowman3Pos = vec3(10.0f, 0.0f, 10.0f);
vec3 tree1Pos = vec3(5.0f, 0.0f, -4.0f);
vec3 tree2Pos = vec3(7.0f, 0.0f, 8.0f);
vec3 tree3Pos = vec3(-5.0f, 0.0f, 8.0f);
vec3 snowballPos = vec3(-10.0f, 10.0f, -5.0f);

vec3 cameraPosition = vec3(0.0f, 2.0f, -15.0f);
vec3 cameraDirection = vec3(0.0f, 0.0f, 1.0f); // start direction depends on camerarotationy, not this vector
vec3 cameraUpVector = vec3(0.0f, 1.0f, 0.0f);


#pragma region MESH LOADING
/*----------------------------------------------------------------------------
                   MESH LOADING FUNCTION
  ----------------------------------------------------------------------------*/

bool load_mesh (const char* file_name) {
  const aiScene* scene = aiImportFile (file_name, aiProcess_Triangulate); // TRIANGLES!
  if (!scene) {
    fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
    return false;
  }
  printf ("  %i animations\n", scene->mNumAnimations);
  printf ("  %i cameras\n", scene->mNumCameras);
  printf ("  %i lights\n", scene->mNumLights);
  printf ("  %i materials\n", scene->mNumMaterials);
  printf ("  %i meshes\n", scene->mNumMeshes);
  printf ("  %i textures\n", scene->mNumTextures);
  
  for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
    const aiMesh* mesh = scene->mMeshes[m_i];
    printf ("    %i vertices in mesh\n", mesh->mNumVertices);

	g_vp.clear();
	g_vn.clear();
	g_vt.clear();

	if (file_name == TREE_MESH) {
		tree_vertex_count = mesh->mNumVertices;
		printf("Found tree");
	}
	else if (file_name == SNOWMAN_MESH) {
		printf("Found snowman");
		snowman_vertex_count = mesh->mNumVertices;
	}
	else if (file_name == GROUND_MESH) {
		printf("Found ground plane");
		ground_count = mesh->mNumVertices;
	}
	else if (file_name == SNOWMAN_ARM_MESH) {
		printf("Found snowman arm");
		snowman_arm_vertex_count = mesh->mNumVertices;
	}
	else if (file_name == SNOWBALL_MESH) {
		printf("Found snowball");
		snowball_vertex_count = mesh->mNumVertices;
	}
	else
		g_point_count = mesh->mNumVertices;
    
	
	for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
      if (mesh->HasPositions ()) {
        const aiVector3D* vp = &(mesh->mVertices[v_i]);
        //printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);
        g_vp.push_back (vp->x);
        g_vp.push_back (vp->y);
        g_vp.push_back (vp->z);
      }
      if (mesh->HasNormals ()) {
        const aiVector3D* vn = &(mesh->mNormals[v_i]);
        //printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
        g_vn.push_back (vn->x);
        g_vn.push_back (vn->y);
        g_vn.push_back (vn->z);
      }
      if (mesh->HasTextureCoords (0)) {
        const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
        //printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
        g_vt.push_back (vt->x);
        g_vt.push_back (vt->y);
      }
      if (mesh->HasTangentsAndBitangents ()) {
        // NB: could store/print tangents here
      }
    }
	printf("      vt size: %i\n", g_vt.size());
  }
  aiReleaseImport (scene);
  return true;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {   
    FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

    if ( fp == NULL ) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';

    fclose(fp);

    return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	const char* pShaderSource = readShaderSource( pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}


GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

	// Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "../Shaders/ToonVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "../Shaders/ToonFragmentShader.txt", GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS

void generateObjectBufferMesh(GLuint &vao, const char* meshname, int &count) {
/*----------------------------------------------------------------------------
                   LOAD MESH HERE AND COPY INTO BUFFERS
  ----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	load_mesh (meshname);
	unsigned int vp_vbo = 0;
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	glGenBuffers (1, &vp_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glBufferData (GL_ARRAY_BUFFER, count * 3 * sizeof (float), &g_vp[0], GL_STATIC_DRAW);
	unsigned int vn_vbo = 0;
	glGenBuffers (1, &vn_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glBufferData (GL_ARRAY_BUFFER, count * 3 * sizeof (float), &g_vn[0], GL_STATIC_DRAW);

//	This is for texture coordinates which you don't currently need, so I have commented it out
	unsigned int vt_vbo = 0;
	glGenBuffers (1, &vt_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	glBufferData (GL_ARRAY_BUFFER, count * 2 * sizeof (float), &g_vt[0], GL_STATIC_DRAW);
	
	//unsigned int vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray (vao);

	glEnableVertexAttribArray (loc1);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer (loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray (loc2);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer (loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

//	This is for texture coordinates which you don't currently need, so I have commented it out
	glEnableVertexAttribArray (loc3);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
}

#pragma endregion VBO_FUNCTIONS

// --------------------------------------------------------
// https://open.gl/textures
// --------------------------------------------------------
void loadTextures(GLuint& tex, const char* file_name) {
	int img_width, img_height, n;

	// STB image loader
	unsigned char* loaded_image = stbi_load(file_name, &img_width, &img_height, &n, STBI_rgb);

	glGenTextures(1, &tex);
	glActiveTexture(GL_TEXTURE0);  // Specifies which texture unit a texture object is bound to with glBindTexture
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB,
		GL_UNSIGNED_BYTE, loaded_image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // repeat across x coordinate if texture too small 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // repeat across y coordinate if texture too small 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Type of interpolation used
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Type of interpolation used
	glGenerateMipmap(GL_TEXTURE_2D);
}

GLfloat xz_length(const vec3& v) {
	return sqrt(v.v[0] * v.v[0] +  v.v[2] * v.v[2]);
}


void display(){

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.2f, 0.5f, 0.7f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram (shaderProgramID);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation (shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation (shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation (shaderProgramID, "proj");
	int texture_location = glGetUniformLocation(shaderProgramID, "tex");
	int no_specular = glGetUniformLocation(shaderProgramID, "no_specular");

	// Root of the Hierarchy
	cameraDirection.v[0] = sin(camerarotationy);
	cameraDirection.v[2] = cos(camerarotationy);
	mat4 view = look_at(cameraPosition, cameraPosition + cameraDirection, cameraUpVector);
	mat4 persp_proj = perspective(45.0, (float)width/(float)height, 0.1, 100.0);
	

	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);

	// GROUND 1 ------------------------
	mat4 ground_matrix = identity_mat4 ();
	ground_matrix = scale(ground_matrix, vec3(15.0, 15.0, 15.0));
	ground_matrix = rotate_x_deg(ground_matrix, -90);
	ground_matrix = translate(ground_matrix, vec3(0.0, 1.5, 0.0));
	// update uniforms & draw
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, ground_matrix.m);
	glUniform1i(no_specular, 1);  // No specular component for ground

	glBindTexture(GL_TEXTURE_2D, GROUND_TEX_ID);
	glUniform1i(texture_location, 0);

	glBindVertexArray(GROUND_ID);
	glDrawArrays (GL_TRIANGLES, 0, ground_count);

	// ----------------------------------------
	// TREES
	//     ^
	//    / \
	//   /   \
	//     |
	// ----------------------------------------

	//Declare your uniform variables that will be used in your shader
	glBindTexture(GL_TEXTURE_2D, TREE_TEX_ID);
	glUniform1i(texture_location, 0);

	mat4 tree1_local = identity_mat4();
	tree1_local = rotate_x_deg(tree1_local, -90);
	tree1_local = scale(tree1_local, vec3(2.0, 2.0, 2.0));
	tree1_local = translate(tree1_local, tree1Pos);
	mat4 tree1_global = tree1_local;
	// update uniforms & draw
	glUniform1i(no_specular, 0);  // Specular component for rest of models
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, tree1_global.m);
	glBindVertexArray(TREE_ID);
	glDrawArrays(GL_TRIANGLES, 0, tree_vertex_count);

	mat4 tree2_local = identity_mat4();
	tree2_local = rotate_x_deg(tree2_local, -90);
	tree2_local = scale(tree2_local, vec3(2.5, 2.5, 2.5));
	tree2_local = translate(tree2_local, tree2Pos);
	mat4 tree2_global = tree2_local;
	// update uniforms & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, tree2_global.m);
	glBindVertexArray(TREE_ID);
	glDrawArrays(GL_TRIANGLES, 0, tree_vertex_count);

	mat4 tree3_local = identity_mat4();
	tree3_local = rotate_x_deg(tree3_local, -90);
	tree3_local = scale(tree3_local, vec3(2.5, 2.5, 2.5));
	tree3_local = translate(tree3_local, tree3Pos);
	mat4 tree3_global = tree3_local;
	// update uniforms & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, tree3_global.m);
	glBindVertexArray(TREE_ID);
	glDrawArrays(GL_TRIANGLES, 0, tree_vertex_count);
	
	// -----------------------------------------------------------
	// SNOWMEN
	//    O
	//   ( )
	//  (   )
	// -----------------------------------------------------------
	glBindTexture(GL_TEXTURE_2D, SNOWMAN_TEX_ID);
	glUniform1i(texture_location, 0);

	mat4 snowman1_local = identity_mat4();
	snowman1_local = rotate_y_deg(snowman1_local, snowman1_rotationy);
	snowman1_local = translate(snowman1_local,snowman1Pos);
	mat4 snowman1_global = snowman1_local;
	// update uniforms & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowman1_global.m);
	glBindVertexArray(SNOWMAN_ID);
	glDrawArrays(GL_TRIANGLES, 0, snowman_vertex_count);

	mat4 snowman2_local = identity_mat4();
	snowman2_local = translate(snowman2_local, snowman2Pos);
	mat4 snowman2_global = snowman2_local;
	// update uniforms & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowman2_global.m);
	glDrawArrays(GL_TRIANGLES, 0, snowman_vertex_count);

	mat4 snowman3_local = identity_mat4();
	snowman3_local = translate(snowman3_local, snowman3Pos);
	mat4 snowman3_global = snowman3_local;
	// update uniforms & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowman3_global.m);
	glDrawArrays(GL_TRIANGLES, 0, snowman_vertex_count);

	// ------------------
	// Snowball
	// 
	//    o
	// ------------------
	mat4 snowball_local = identity_mat4();
	snowball_local = scale(snowball_local, vec3(0.2, 0.2, 0.2));
	snowball_local = translate(snowball_local, snowballPos);
	mat4 snowball_global = snowball_local;
	// update uniforms & draw
	if (thrownSnowball == false) {
		snowballPos = cameraPosition;
		snowballPos.v[1] = snowballPos.v[1] - 0.3;
		// Move snowball left a bit relative to camera direction
		snowballPos.v[0] = snowballPos.v[0] - 0.4*cameraDirection.v[2];  // x1' = x1 + y2
		snowballPos.v[2] = snowballPos.v[2] + 0.4*cameraDirection.v[0];  // y1' = y1 - x2
	}
	else if (snowballPos.v[1] > -1.0) {
		snowballPos = snowballPos + snowballDir*0.01;
		snowballDir.v[1] = snowballDir.v[1] - snowballGravity;  // Was changing snowball position
		snowballGravity = snowballGravity + 0.000004f;
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowball_global.m);
		glBindVertexArray(SNOWBALL_ID);
		glDrawArrays(GL_TRIANGLES, 0, snowball_vertex_count);
	}
	else {
		thrownSnowball = false;
	}
	
	// ------------------------
	// ARMS
	//  _____/_
	//       \
	// ------------------------
	// ARMS FOR SNOWMAN 1

	//Declare your uniform variables that will be used in your shader
	glBindTexture(GL_TEXTURE_2D, SNOWMAN_ARM_TEX_ID);
	glUniform1i(texture_location, 0);

	mat4 snowman_arm_11_local = identity_mat4();
	if(fleeing)
		snowman_arm_11_local = rotate_x_deg(snowman_arm_11_local, 330 - armAngle);
	else
		snowman_arm_11_local = rotate_x_deg(snowman_arm_11_local, 90 - armAngle);
	snowman_arm_11_local = scale(snowman_arm_11_local, vec3(0.2, 0.2, 0.2));
	snowman_arm_11_local = translate(snowman_arm_11_local, vec3(0.8f, 2.5f, 0.0f));
	mat4 snowman_arm_11_global = snowman1_global * snowman_arm_11_local;
	// update uniforms & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowman_arm_11_global.m);
	glBindVertexArray(SNOWMAN_ARM_ID);
	glDrawArrays(GL_TRIANGLES, 0, snowman_arm_vertex_count);

	mat4 snowman_arm_12_local = identity_mat4();
	if(fleeing)
		snowman_arm_12_local = rotate_x_deg(snowman_arm_12_local, 240 + armAngle);
	else
		snowman_arm_12_local = rotate_x_deg(snowman_arm_12_local, armAngle);
	snowman_arm_12_local = scale(snowman_arm_12_local, vec3(0.2, 0.2, 0.2));
	snowman_arm_12_local = translate(snowman_arm_12_local, vec3(-0.8f, 2.5f, 0.0f));
	mat4 snowman_arm_12_global = snowman1_global * snowman_arm_12_local;
	// update uniforms & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowman_arm_12_global.m);
	glBindVertexArray(SNOWMAN_ARM_ID);
	glDrawArrays(GL_TRIANGLES, 0, snowman_arm_vertex_count);


	// ARMS FOR SNOWMAN 2
	mat4 snowman_arm_21_local = identity_mat4();
	if (fleeing)
		snowman_arm_21_local = rotate_x_deg(snowman_arm_21_local, 330 - armAngle);
	else
		snowman_arm_21_local = rotate_x_deg(snowman_arm_21_local, 90 - armAngle);
	snowman_arm_21_local = scale(snowman_arm_21_local, vec3(0.2, 0.2, 0.2));
	snowman_arm_21_local = translate(snowman_arm_21_local, vec3(0.8f, 2.5f, 0.0f));
	mat4 snowman_arm_21_global = snowman2_global * snowman_arm_21_local;
	// update uniforms & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowman_arm_21_global.m);
	glBindVertexArray(SNOWMAN_ARM_ID);
	glDrawArrays(GL_TRIANGLES, 0, snowman_arm_vertex_count);

	mat4 snowman_arm_22_local = identity_mat4();
	if (fleeing)
		snowman_arm_22_local = rotate_x_deg(snowman_arm_22_local, 240 + armAngle);
	else
		snowman_arm_22_local = rotate_x_deg(snowman_arm_22_local, armAngle);
	snowman_arm_22_local = scale(snowman_arm_22_local, vec3(0.2, 0.2, 0.2));
	snowman_arm_22_local = translate(snowman_arm_22_local, vec3(-0.8f, 2.5f, 0.0f));
	mat4 snowman_arm_22_global = snowman2_global * snowman_arm_22_local;
	// update uniforms & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowman_arm_22_global.m);
	glBindVertexArray(SNOWMAN_ARM_ID);
	glDrawArrays(GL_TRIANGLES, 0, snowman_arm_vertex_count);

    glutSwapBuffers();
}


void updateScene() {	

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// Arm movement
	if (armSwitch == true) {
		armAngle = armAngle + 0.1;
		if (armAngle > 90) {
			armSwitch = false;
		}
	}
	else if (armSwitch == false) {
		armAngle = armAngle - 0.1;
		if (armAngle < 0) {
			armSwitch = true;
		}
	}

	// Basic Snowman AI
	vec3 oldSnowman1Pos = snowman1Pos;
	vec3 oldSnowman2Pos = snowman2Pos;
	vec3 vectorToSnowman1 = snowman1Pos - cameraPosition;
	vec3 vectorToSnowman2 = snowman2Pos - cameraPosition;
	vectorToSnowman1.v[1] = 0;  // Project to xz plane
	vectorToSnowman2.v[1] = 0;  // Project to xz plane
	GLfloat snowman1Collision = xz_length(vectorToSnowman1);
	GLfloat snowman2Collision = xz_length(vectorToSnowman2);
	
	// BASIC AI CALCULATIONS
	if (snowman1Collision < 10.0) {
		snowman1Pos = snowman1Pos + (normalise(vectorToSnowman1)*0.003);
	}
	else {
		if (marchOrder == true) {
			marchDistance += 0.001;
			snowman1Pos.v[2] = snowman1Pos.v[2] + 0.002;
		}
		if (marchOrder == false) {
			marchDistance -= 0.001;
			snowman1Pos.v[2] = snowman1Pos.v[2] - 0.002;
		}
		if (marchDistance > 10) {
			marchOrder = false;
		}
		if (marchDistance < 0) {
			marchOrder = true;
		}
	}	

	// Snowball Collision
	GLfloat SbSnowman1Coll = xz_length(snowballPos - snowman1Pos);
	GLfloat SbSnowman2Coll = xz_length(snowballPos - snowman2Pos);
	GLfloat SbSnowman3Coll = xz_length(snowballPos - snowman3Pos);
	GLfloat Sbtree1Collision = xz_length(snowballPos - tree1Pos);
	GLfloat Sbtree2Collision = xz_length(snowballPos - tree2Pos);
	GLfloat Sbtree3Collision = xz_length(snowballPos - tree3Pos);
	// Collision with snowmen
	if (SbSnowman1Coll < 1.0 || SbSnowman2Coll < 1.0 || SbSnowman3Coll < 1.0) {
		thrownSnowball = false;
		hitLocation = snowballPos;
		printf("x: %f z: %f ", hitLocation.v[0], hitLocation.v[2]);
		fleeing = true;
	}
	// Collision with trees
	else if (Sbtree1Collision  < 1.5 || Sbtree2Collision < 1.5 || Sbtree3Collision < 1.5) {
		thrownSnowball = false;
		hitLocation = snowballPos;
		printf("x: %f z: %f ", hitLocation.v[0], hitLocation.v[2]);
	}

	if (fleeing) {
		snowman1Pos = snowman1Pos + (normalise(vectorToSnowman1)*0.01);
		snowman2Pos = snowman2Pos + (normalise(vectorToSnowman2)*0.01);
		fleeTime += 0.0002;
		if (fleeTime > 1.0) {
			fleeing = false;
			fleeTime = 0.0;
		}
	}

	// Snowman1 - Tree Collision and world boundary collision
	GLfloat snowman1ToTree1 = xz_length(snowman1Pos - tree1Pos);
	GLfloat snowman1ToTree2 = xz_length(snowman1Pos - tree2Pos);
	GLfloat snowman1ToTree3 = xz_length(snowman1Pos - tree3Pos);
	if (snowman1ToTree1 < 3.0 || snowman1ToTree2 < 3.0 || snowman1ToTree3 < 3.0) {
		snowman1Pos = oldSnowman1Pos;
	}
	if (abs(snowman1Pos.v[0]) > 50 || abs(snowman1Pos.v[2]) > 50) {
		snowman1Pos = oldSnowman1Pos;
	}

	// Snowman2 - Tree Collision and world boundary collision
	GLfloat snowman2ToTree1 = xz_length(snowman2Pos - tree1Pos);
	GLfloat snowman2ToTree2 = xz_length(snowman2Pos - tree2Pos);
	GLfloat snowman2ToTree3 = xz_length(snowman2Pos - tree3Pos);
	if (snowman2ToTree1 < 3.0 || snowman2ToTree2 < 3.0 || snowman2ToTree3 < 3.0) {
		snowman2Pos = oldSnowman2Pos;
	}
	if (abs(snowman1Pos.v[0]) > 50 || abs(snowman1Pos.v[2]) > 50) {
		snowman1Pos = oldSnowman1Pos;
	}

	// Spin snowman 2
	snowman1_rotationy += 0.1;

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// load mesh into a vertex buffer array
	generateObjectBufferMesh(GROUND_ID, GROUND_MESH, ground_count);
	generateObjectBufferMesh(TREE_ID, TREE_MESH, tree_vertex_count);
	generateObjectBufferMesh(SNOWMAN_ID, SNOWMAN_MESH, snowman_vertex_count);
	generateObjectBufferMesh(SNOWMAN_ARM_ID, SNOWMAN_ARM_MESH, snowman_arm_vertex_count);
	generateObjectBufferMesh(SNOWBALL_ID, SNOWBALL_MESH, snowball_vertex_count);

	loadTextures(GROUND_TEX_ID, GROUND_TEXTURE);
	loadTextures(TREE_TEX_ID, TREE_TEXTURE);
	loadTextures(SNOWMAN_TEX_ID, SNOWMAN_TEXTURE);
	loadTextures(SNOWMAN_ARM_TEX_ID, SNOWMAN_ARM_TEXTURE);
}

// Placeholder code for the keypress
void processNormalKeys(unsigned char key, int x, int y)
{
	vec3 oldCameraPosition = cameraPosition;  // Store in case of collision

	if (key == 'i') {
		translate_y = translate_y + 0.1;
	}
	if (key == 'w') {
		cameraPosition = cameraPosition + cameraDirection;  // Move forward in direction of camera
	}
	if (key == 's') {
		cameraPosition = cameraPosition - cameraDirection;  // Move backward in direction of camera
	}
	if (key == 'a') {
		// Move left relative to camera direction (in direction 90 deg left of camera direction) 
		cameraPosition.v[0] = cameraPosition.v[0] + cameraDirection.v[2];  // x1' = x1 + y2
		cameraPosition.v[2] = cameraPosition.v[2] - cameraDirection.v[0];  // y1' = y1 - x2
	}
	if (key == 'd') {
		// Move right relative to camera direction (in direction 90 deg right of camera direction) 
		cameraPosition.v[0] = cameraPosition.v[0] - cameraDirection.v[2];  // x1' = x1 - y2
		cameraPosition.v[2] = cameraPosition.v[2] + cameraDirection.v[0];  // y1' = y1 + x2
	}
	if (key == 'q') {
		// Rotate counter-clockwise about y-axis (turn left)
		camerarotationy += 0.030f;
	}
	if (key == 'e') {
		// Rotate clockwise aboutengl y-axis (turn right)
		camerarotationy -= 0.030f;
	}
	if (key == 'r') {
		if (thrownSnowball == false) {
			snowballGravity = 0.0f;
			snowballDir = normalise(cameraDirection) * 5;
			thrownSnowball = true;
		}
	}

	// CAMERA COLLISION CALCULATIONS
	// "FOR LOOP" THIS WITH A VECTOR OF OBJECT POSITIONS
	GLfloat snowman1Collision = xz_length(cameraPosition - snowman1Pos);
	GLfloat snowman2Collision = xz_length(cameraPosition - snowman2Pos);
	GLfloat snowman3Collision = xz_length(cameraPosition - snowman3Pos);
	GLfloat tree1Collision = xz_length(cameraPosition - tree1Pos);
	GLfloat tree2Collision = xz_length(cameraPosition - tree2Pos);
	GLfloat tree3Collision = xz_length(cameraPosition - tree3Pos);
	GLfloat snowman1ToTree = xz_length(snowman1Pos - tree1Pos);
	// Collision with snowmen
	if (snowman1Collision < 2.0 || snowman2Collision < 2.0 || snowman3Collision < 2.0) {
		cameraPosition = oldCameraPosition;
	}
	// Collision with trees
	else if (tree1Collision < 3.0 || tree2Collision < 3.0 || tree3Collision < 3.0) {
		cameraPosition = oldCameraPosition;
	}
	// collision with world boundary
	else if (abs(cameraPosition.v[0]) > 50.0 || abs(cameraPosition.v[2]) > 50.0) {
		cameraPosition = oldCameraPosition;
	}

	// Player position check (debug)
	// printf("x: %f z: %f ", cameraPosition.v[0], cameraPosition.v[2]);
	glutPostRedisplay();
}

int main(int argc, char** argv){

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Merry Christmas!");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(processNormalKeys);

	 // A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();

    return 0;
}