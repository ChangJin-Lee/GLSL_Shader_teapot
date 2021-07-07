#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define my_PI 3.141592

static char* vsSource = "#version 140 \n\
in vec4 aPosition; \n\
in vec4 aNormal; \n\
out vec4 vColor; \n\
uniform mat4 uscale; \n\
uniform mat4 utranslate; \n\
uniform vec4 light_position;\n\
uniform vec4 light_ambient; \n\
uniform vec4 light_diffuse; \n\
uniform vec4 light_att; \n\
uniform vec4 material_ambient; \n\
uniform vec4 material_diffuse; \n\
void main(void) { \n\
  vec4 vPosition = uscale * utranslate * aPosition; \n\
  vec4 ambient = light_ambient * material_ambient; \n\
  mat4 mNormal = transpose(inverse(uscale*utranslate)); \n\
  vec4 vNormal = mNormal * aNormal; \n\
  vec3 N = normalize(vNormal.xyz); \n\
  vec3 L = normalize(light_position.xyz - vPosition.xyz); \n\
  float d = length(light_position.xyz - vPosition.xyz); \n\
  float denom = light_att.x + light_att.y * d + light_att.z * d * d; \n\
  vec4 diffuse = max(dot(L, N), 0.0) * light_diffuse * material_diffuse / denom; \n\
  vColor = ambient + diffuse ; \n\
//vColor = ambient;\n\
//vColor = aColor;\n\
  gl_Position = vPosition; \n\
}";

static char* fsSource = "#version 140 \n\
  in vec4 vColor; \n\
void main(void) { \n\
  gl_FragColor = vColor; \n\
}";

GLuint vs = 0;
GLuint fs = 0;
GLuint prog = 0;

char buf[3072];

int num_vertices, num_faces;

GLfloat* normal;
GLfloat* vertices;
GLushort* indices;

void myinit(void) {
	GLuint status;

	printf("init func\n");
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vsSource, NULL);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
	printf("vs compile status = %s\n", (status == GL_TRUE) ? "true" : "false");
	glGetShaderInfoLog(vs, sizeof(buf), NULL, buf);
	printf("vs log = [%s]\n", buf);

	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fsSource, NULL);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
	printf("fs compile status = %s\n", (status == GL_TRUE) ? "true" : "false");
	glGetShaderInfoLog(fs, sizeof(buf), NULL, buf);
	printf("fs log = [%s]\n", buf);

	prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	printf("program link status = %s\n", (status == GL_TRUE) ? "true" : "false");
	glGetProgramInfoLog(prog, sizeof(buf), NULL, buf);
	printf("link log = [%s]\n", buf);
	glValidateProgram(prog);
	glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
	printf("program validate status = %s\n", (status == GL_TRUE) ? "true" : "false");
	glGetProgramInfoLog(prog, sizeof(buf), NULL, buf);
	printf("validate log = [%s]\n", buf);
	glUseProgram(prog);

	GLuint loc;
	GLuint vbo[1];
	// using vertex buffer object
	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 2 * num_vertices * 4 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, num_vertices * 4 * sizeof(GLfloat), vertices);
	glBufferSubData(GL_ARRAY_BUFFER, num_vertices * 4 * sizeof(GLfloat), num_vertices * 4 * sizeof(GLfloat),
	normal);

	loc = glGetAttribLocation(prog, "aPosition");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

	loc = glGetAttribLocation(prog, "aNormal");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)(num_vertices * 4 * sizeof(GLfloat)));

	glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
	glEnable(GL_DEPTH_TEST);
}

void mykeyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 27: // ESCAPE
		exit(0);
		break;
	}
}

GLfloat m[16];


void print_mat(float *m)
{
	for (int i = 0; i < 4; i++) {
		printf("%f  %f  %f   %f\n", m[i], m[i + 4], m[i + 8], m[i + 12]);
	}
}

void mat_translate(float *m, float* t_v)
{
	m[0] = 1.0;      m[4] = 0.0;     m[8] = 0.0;      m[12] = t_v[0];
	m[1] = 0.0;      m[5] = 1.0;     m[9] = 0.0;      m[13] = t_v[1];
	m[2] = 0.0;      m[6] = 0.0;     m[10] = 1.0;     m[14] = t_v[2];
	m[3] = 0.0;      m[7] = 0.0;     m[11] = 0.0;     m[15] = 1.0;
}
void mat_scale(float *m, float *s_v)
{
	m[0] = s_v[0];   m[4] = 0.0;     m[8] = 0.0;      m[12] = 0.0;
	m[1] = 0.0;      m[5] = s_v[1];  m[9] = 0.0;      m[13] = 0.0;
	m[2] = 0.0;      m[6] = 0.0;     m[10] = s_v[2];  m[14] = 0.0;
	m[3] = 0.0;      m[7] = 0.0;     m[11] = 0.0;     m[15] = 1.0;
}

void setLightAndMaterial(void) {
	GLuint loc;
	GLfloat light_pos[4] = { 1.5, 1.5, -1.0, 1.0 };
	GLfloat light_amb[4] = { 0.3, 0.3, 0.3, 1.0 };
	GLfloat light_dif[4] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat light_att[4] = { 1.0, 0.0, 0.0, 1.0 };
	GLfloat mat_amb[4] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_dif[4] = { 1.0, 1.0, 1.0, 1.0 };
	

	loc = glGetUniformLocation(prog, "light_position");
	glUniform4fv(loc, 1, light_pos);

	loc = glGetUniformLocation(prog, "light_ambient");
	glUniform4fv(loc, 1, light_amb);

	loc = glGetUniformLocation(prog, "light_diffuse");
	glUniform4fv(loc, 1, light_dif);

	loc = glGetUniformLocation(prog, "light_att");
	glUniform4fv(loc, 1, light_att);

	loc = glGetUniformLocation(prog, "material_ambient");
	glUniform4fv(loc, 1, mat_amb);

	loc = glGetUniformLocation(prog, "material_diffuse");
	glUniform4fv(loc, 1, mat_dif);
}


void mydisplay(void) {
	GLuint loc;
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // gray
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float v_scale[3] = { 0.01, 0.01, 0.01 };
	float v_trans[3] = { 0.0, 0.0, 0.0 };

	mat_scale(m, v_scale);
	loc = glGetUniformLocation(prog, "uscale");
	glUniformMatrix4fv(loc, 1, GL_FALSE, m);

	setLightAndMaterial();

	mat_translate(m, v_trans);
	loc = glGetUniformLocation(prog, "utranslate");
	glUniformMatrix4fv(loc, 1, GL_FALSE, m);

	glDrawElements(GL_TRIANGLES, num_faces * 3, GL_UNSIGNED_SHORT, indices);
	glFlush();

	glutSwapBuffers();
}


int main(int argc, char* argv[]) {
	int i, j;
	FILE* fp = fopen("teapot.obj", "r"); //read mode
	fscanf(fp, "%d %d", &num_vertices, &num_faces);

	vertices = malloc(sizeof(GLfloat)*num_vertices * 4);
	normal = malloc(sizeof(GLfloat)*num_vertices * 4);
	indices = malloc(sizeof(GLushort)*num_faces * 3);

	j = 0;
	for (i = 0; i < num_vertices; i++) {
		fscanf(fp, "%f %f %f", &vertices[j], &vertices[j + 1], &vertices[j + 2]);
		vertices[j + 3] = 1.0;
		j += 4;
	}
	j = 0;
	for (i = 0; i < num_vertices; i++) {
		fscanf(fp, "%f %f %f", &normal[j], &normal[j + 1], &normal[j + 2]);
		normal[j + 3] = 0.0;
		j += 4;
	}
	j = 0;
	for (i = 0; i < num_faces; i++) {
		fscanf(fp, "%d %d %d", &indices[j], &indices[j + 1], &indices[j + 2]);
		j += 3;
	}

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(300, 300);
	glutCreateWindow("window");
	glutDisplayFunc(mydisplay);
	glutKeyboardFunc(mykeyboard);

	glewInit();
	myinit();
	glutMainLoop();
	return 0;
}
