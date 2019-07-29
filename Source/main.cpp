//
// COMP 371 Assignment Framework
//
// Created by Nicolas Bergeron on 22/7/14.
// Based on Tutorial from www.opengl-tutorial.org
//
// Copyright (c) 2014 Concordia University. All rights reserved.
//


// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW - OpenGL Extension Wrangler
#define GLEW_STATIC
#include <GL/glew.h>

// Include GLFW - OpenGL Framework (Multi Platform framework to Setup a window and OpenGL Context
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM - OpenGL Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "shader.h"
#include "sphere.h"


// Material Coefficients
const float ka = 0.2f;
const float kd = 0.8f;
const float ks = 0.2f;
const float n = 50.0f;

// Light Coefficients
const vec3 lightColor(1.0f, 1.0f, 1.0f);
const float lightKc = 0.0f;
const float lightKl = 0.0f;
const float lightKq = 1.0f;
const vec4 lightPosition(5.0f, 5.0f, -5.0f, 1.0f); // If w = 1.0f, we have a point light
//const vec4 lightPosition(5.0f, -5.0f, 5.0f, 0.0f); // If w = 0.0f, we have a directional light


int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}

#if defined(PLATFORM_OSX)
    // OS X would use legacy opengl by default, and wouldn't be able to load shaders
    // This is the proper way to setup GLFW to use modern OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    
#else
    // Allow older laptops to run the framework, even though, our framework
    // is compatible with OpenGL 3.3 (which removed many deprecated features)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
#endif
    
	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Assignment 2 Testbed - Shading", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);

	// Enable depth test
	// Accept fragment if it closer to the camera than the former one
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("../Source/Shaders/Phong.vertexshader", "../Source/Shaders/Phong.fragmentshader");
//	GLuint programID = LoadShaders("../Source/Shaders/Gouraud.vertexshader", "../Source/Shaders/Gouraud.fragmentshader");


	// Get a handle for our Transformation Matrices uniform
	GLuint WorldMatrixID = glGetUniformLocation(programID, "WorldTransform");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "ViewTransform");
	GLuint ProjMatrixID = glGetUniformLocation(programID, "ProjectonTransform");

	// Get a handle for Light Attributes uniform
	GLuint LightPositionID = glGetUniformLocation(programID, "WorldLightPosition");
	GLuint LightColorID = glGetUniformLocation(programID, "lightColor");
	GLuint LightAttenuationID = glGetUniformLocation(programID, "lightAttenuation");

	// Get a handle for Material Attributes uniform
	GLuint MaterialID = glGetUniformLocation(programID, "materialCoefficients");

	// World Matrix - Identity - Draw Sphere at the origin
	glm::mat4 World(1.0f);
	
	// View matrix
	glm::mat4 View       = glm::lookAt(
								glm::vec3(0,0,-5), // Camera is at (0,0,-5), in World Space
								glm::vec3(0,0,0), // and looks at the origin
								glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
						   );

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);

	// Or, for an ortho camera :
	//glm::mat4 Projection = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f); // In world coordinates

	unsigned int numVertices;
	Vertex* vertexBuffer = GetRedSphereVertexBuffer(numVertices);

	// Upload Vertex Buffer to the GPU
	GLuint vertexBufferID;
	glGenBuffers(1, &vertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, numVertices*sizeof(Vertex), vertexBuffer, GL_STATIC_DRAW);

	float size = sizeof(*vertexBuffer);

	size *= 2;

	do{
		// Clear the screen
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		// Use our shader
		glUseProgram(programID);

		// Send our transformation to the currently bound shader, 
		// in the "World / View / Projection" uniform

		glUniformMatrix4fv(WorldMatrixID, 1, GL_FALSE, &World[0][0]);
		glUniformMatrix4fv(ViewMatrixID,  1, GL_FALSE, &View[0][0]);
		glUniformMatrix4fv(ProjMatrixID,  1, GL_FALSE, &Projection[0][0]);


		// Draw the Vertex Buffer
		// Note this draws a unit Sphere
		// The Model View Projection transforms are computed in the Vertex Shader

		// Set shader constants
		glUniform4f(MaterialID, ka, kd, ks, n);
		glUniform4f(LightPositionID, lightPosition.x, lightPosition.y, lightPosition.z, lightPosition.w);
		glUniform3f(LightColorID, lightColor.r, lightColor.g, lightColor.b);
		glUniform3f(LightAttenuationID, lightKc, lightKl, lightKq);


		// 1st attribute buffer : vertex Positions
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0,				// attribute. No particular reason for 0, but must match the layout in the shader.
			3,				// size
			GL_FLOAT,		// type
			GL_FALSE,		// normalized?
			sizeof(Vertex), // stride
			(void*)0        // array buffer offset
			);

		// 2nd attribute buffer : vertex normal
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(1,
			3,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Vertex),
			(void*)sizeof(vec3)    // Normal is Offseted by vec3 (see class Vertex)
			);


		// 3rd attribute buffer : vertex color
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(2,
			3,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Vertex),
			(void*)(2 * sizeof(vec3)) // Color is Offseted by 2 vec3 (see class Vertex)
			);

		// Draw the triangles !
		// Notice it is using TRIANGLE STRIPS which requires less vertices than triangle lists
		glDrawArrays(GL_TRIANGLE_STRIP, 0, numVertices);

		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

		if (glGetError() != GL_NO_ERROR)
		{
			fprintf(stderr, "OpengGL error %d", glGetError());
		}

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexBufferID);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
