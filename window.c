#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4duw_SDL2.h>
#include <SDL_image.h>
#include <assert.h>

/* Prototypes des fonctions statiques contenues dans ce fichier C */
static void init(void);
static void resize(int w, int h);
static void draw(void);
static void quit(void);

/*!\brief largeur et hauteur de la fenêtre */
static int _wW = 1280, _wH = 1024;
/*!\brief identifiant du (futur) GLSL program */
static GLuint _pId[2] = { 0 };
/*!\brief identifiant de framebuffer */
static GLuint _fbo = 0;
/*!\brief identifiant du plan */
static GLuint _quad = 0;
static GLuint _tore = 0;
/*!\brief identifiant de textures */
static GLuint _texId[] = { 0, 0 };

int main(int argc, char ** argv) {
  if(!gl4duwCreateWindow(argc, argv, "GL4Dummies", 20, 20, 
			 _wW, _wH, GL4DW_RESIZABLE | GL4DW_SHOWN))
    return 1;
  init();
  atexit(quit);
  gl4duwDisplayFunc(draw);
  gl4duwMainLoop();
  return 0;
}

static void init(void) {
  _quad = gl4dgGenQuadf();
  _tore = gl4dgGenTorusf(20, 20, 0.33f);
  _pId[0] = gl4duCreateProgram("<vs>shaders/light.vs", "<fs>shaders/light.fs", NULL);
  _pId[1] = gl4duCreateProgram("<vs>shaders/filter.vs", "<fs>shaders/filter.fs", NULL);
  glGenTextures(2, _texId);
  for(int i = 0; i < 2; ++i) {
    glBindTexture(GL_TEXTURE_2D, _texId[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _wW, _wH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  }
  glBindTexture(GL_TEXTURE_2D, 0);  

  glGenFramebuffers(1, &_fbo);

  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
  gl4duGenMatrix(GL_FLOAT, "modelMatrix");
  gl4duGenMatrix(GL_FLOAT, "viewMatrix");
  resize(_wW, _wH);
}


void resize(int w, int h) {
  GLfloat ratio = h / (GLfloat)w;
  _wW = w; _wH = h;
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-1.0f, 1.0f, -1.0f * ratio, 1.0f * ratio, 2.0f, 100.0f);
  glViewport(0, 0, _wW, _wH);
}

float untruc = 1000.0f;

/*!\brief Cette fonction dessine dans le contexte OpenGL actif. */
static void draw(void) {
  static GLfloat angle = 0.0f;
  static double t0 = 0.0;
  double t = gl4dGetElapsedTime(), dt = (t - t0) / 1000.0;
  t0 = t;



  glEnable(GL_DEPTH_TEST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(_pId[0]);

  gl4duBindMatrix("viewMatrix");
  gl4duLoadIdentityf();
  gl4duLookAtf(1.0f, 1.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
  gl4duBindMatrix("modelMatrix");
  gl4duLoadIdentityf();
  gl4duRotatef(angle, 1.0f, 0.0f, 0.0f);
  gl4duSendMatrices();

  glUniform1f(glGetUniformLocation(_pId[0], "temps"), t / 1000.0f);
  glUniform4f(glGetUniformLocation(_pId[0], "Lp"), 0.0f, 0.0f, 1.5f, 1.0f);
  glUniform4f(glGetUniformLocation(_pId[0], "sdiffus"), 0.8f, 0.0f, 0.0f, 1.0f);
  glUniform4f(glGetUniformLocation(_pId[0], "sambient"), 0.85f, 1.0f, 0.85f, 1.0f);

  gl4dgDraw(_tore);

  angle += 18.0f * dt;

  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texId[0], 0);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, _wW, _wH, 0, 0, _wW, _wH, GL_COLOR_BUFFER_BIT, GL_NEAREST);


  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texId[1], 0); /* le output */
  untruc -= 1.0f;
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(_pId[1]);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _texId[0]); /* le input */
  glUniform1i(glGetUniformLocation(_pId[1], "tex"), 0); // le 0 correspond à glActiveTexture(GL_TEXTURE0);
  glUniform1f(glGetUniformLocation(_pId[1], "time"), -(t/1000.0f));
  glUniform3fv(glGetUniformLocation(_pId[1], "rgb_shift"), 1, (GLfloat[]){t/untruc, t/(untruc/2), t/(untruc/3)});
  gl4dgDraw(_quad);
  glBindTexture(GL_TEXTURE_2D, 0);
  
  
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
  glBlitFramebuffer(0, 0, _wW, _wH, 0, 0, _wW, _wH, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

/*!\brief appelée au moment de sortir du programme (atexit), elle
 *  libère les éléments OpenGL utilisés.*/
static void quit(void) {
  /* suppression de la texture */
  if(_texId[0])
    glDeleteTextures(2, _texId);
  /* suppression du fbo */
  if(_fbo)
    glDeleteFramebuffers(1, &_fbo);
  /* nettoyage des éléments utilisés par la bibliothèque GL4Dummies */
  gl4duClean(GL4DU_ALL);
}
