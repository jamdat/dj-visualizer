#pragma once

// =============================================================================
// GL33Loader.h - Cross-Platform OpenGL 3.3 Core Profile Loader
// Native <OpenGL/gl3.h> on macOS; GLFW proc-address loader on Windows/Linux
// =============================================================================

#if defined(__APPLE__)
    #ifndef GL_SILENCE_DEPRECATION
        #define GL_SILENCE_DEPRECATION
    #endif
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
    namespace Visualizer::Graphics {
        inline bool InitGL33Loader() { return true; }
    }
#else
    #if defined(_WIN32)
        #ifndef WIN32_LEAN_AND_MEAN
            #define WIN32_LEAN_AND_MEAN
        #endif
        #ifndef NOMINMAX
            #define NOMINMAX
        #endif
        #include <windows.h>
    #endif
    #include <GL/gl.h>
    #include <cstddef>
    #include <cstdint>

    #ifndef APIENTRY
        #define APIENTRY
    #endif
    #ifndef APIENTRYP
        #define APIENTRYP APIENTRY *
    #endif

    typedef char GLchar;
    typedef ptrdiff_t GLsizeiptr;
    typedef ptrdiff_t GLintptr;

    #ifndef GL_TEXTURE0
        #define GL_TEXTURE0                       0x84C0
        #define GL_TEXTURE1                       0x84C1
        #define GL_TEXTURE2                       0x84C2
        #define GL_TEXTURE3                       0x84C3
        #define GL_TEXTURE4                       0x84C4
        #define GL_TEXTURE5                       0x84C5
        #define GL_TEXTURE6                       0x84C6
        #define GL_TEXTURE7                       0x84C7
        #define GL_CLAMP_TO_EDGE                  0x812F
        #define GL_ARRAY_BUFFER                   0x8892
        #define GL_UNIFORM_BUFFER                 0x8A11
        #define GL_DYNAMIC_DRAW                   0x88E8
        #define GL_FRAGMENT_SHADER                0x8B30
        #define GL_VERTEX_SHADER                  0x8B31
        #define GL_COMPILE_STATUS                 0x8B81
        #define GL_LINK_STATUS                    0x8B82
        #define GL_INFO_LOG_LENGTH                0x8B84
        #define GL_FRAMEBUFFER                    0x8D40
        #define GL_READ_FRAMEBUFFER               0x8CA8
        #define GL_DRAW_FRAMEBUFFER               0x8CA9
        #define GL_COLOR_ATTACHMENT0              0x8CE0
        #define GL_FRAMEBUFFER_COMPLETE           0x8CD5
        #define GL_R32F                           0x822E
        #define GL_RED                            0x1903
        #define GL_RGBA8                          0x8058
        #define GL_STATIC_DRAW                    0x88E4
        #define GL_ELEMENT_ARRAY_BUFFER           0x8893
        #define GL_INVALID_INDEX                  0xFFFFFFFFu
        #define GL_DEPTH_ATTACHMENT               0x8D00
        #define GL_DEPTH_COMPONENT24              0x81A6
        #define GL_DEPTH_COMPONENT                0x1902
    #endif

    typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC)(GLenum texture);
    typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC)(GLenum type);
    typedef void (APIENTRYP PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
    typedef void (APIENTRYP PFNGLCOMPILESHADERPROC)(GLuint shader);
    typedef void (APIENTRYP PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
    typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    typedef void (APIENTRYP PFNGLDELETESHADERPROC)(GLuint shader);
    typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC)(void);
    typedef void (APIENTRYP PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
    typedef void (APIENTRYP PFNGLLINKPROGRAMPROC)(GLuint program);
    typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
    typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    typedef void (APIENTRYP PFNGLUSEPROGRAMPROC)(GLuint program);
    typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC)(GLuint program);
    typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
    typedef void (APIENTRYP PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
    typedef void (APIENTRYP PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    typedef GLuint (APIENTRYP PFNGLGETUNIFORMBLOCKINDEXPROC)(GLuint program, const GLchar *uniformBlockName);
    typedef void (APIENTRYP PFNGLUNIFORMBLOCKBINDINGPROC)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
    typedef void (APIENTRYP PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
    typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
    typedef void (APIENTRYP PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
    typedef void (APIENTRYP PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
    typedef void (APIENTRYP PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
    typedef void (APIENTRYP PFNGLBINDBUFFERBASEPROC)(GLenum target, GLuint index, GLuint buffer);
    typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
    typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
    typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint array);
    typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
    typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
    typedef void (APIENTRYP PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
    typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    typedef GLenum (APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
    typedef void (APIENTRYP PFNGLGENSAMPLERSPROC)(GLsizei count, GLuint *samplers);
    typedef void (APIENTRYP PFNGLDELETESAMPLERSPROC)(GLsizei count, const GLuint *samplers);
    typedef void (APIENTRYP PFNGLBINDSAMPLERPROC)(GLuint unit, GLuint sampler);
    typedef void (APIENTRYP PFNGLSAMPLERPARAMETERIPROC)(GLuint sampler, GLenum pname, GLint param);
    typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
    typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);
    typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
    typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    typedef void (APIENTRYP PFNGLGENERATEMIPMAPPROC)(GLenum target);

    extern PFNGLACTIVETEXTUREPROC        glActiveTexture;
    extern PFNGLCREATESHADERPROC         glCreateShader;
    extern PFNGLSHADERSOURCEPROC         glShaderSource;
    extern PFNGLCOMPILESHADERPROC        glCompileShader;
    extern PFNGLGETSHADERIVPROC          glGetShaderiv;
    extern PFNGLGETSHADERINFOLOGPROC     glGetShaderInfoLog;
    extern PFNGLDELETESHADERPROC         glDeleteShader;
    extern PFNGLCREATEPROGRAMPROC        glCreateProgram;
    extern PFNGLATTACHSHADERPROC         glAttachShader;
    extern PFNGLLINKPROGRAMPROC          glLinkProgram;
    extern PFNGLGETPROGRAMIVPROC         glGetProgramiv;
    extern PFNGLGETPROGRAMINFOLOGPROC    glGetProgramInfoLog;
    extern PFNGLUSEPROGRAMPROC           glUseProgram;
    extern PFNGLDELETEPROGRAMPROC        glDeleteProgram;
    extern PFNGLGETUNIFORMLOCATIONPROC   glGetUniformLocation;
    extern PFNGLUNIFORM1IPROC            glUniform1i;
    extern PFNGLUNIFORM3FPROC            glUniform3f;
    extern PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
    extern PFNGLUNIFORMBLOCKBINDINGPROC  glUniformBlockBinding;
    extern PFNGLGENBUFFERSPROC           glGenBuffers;
    extern PFNGLDELETEBUFFERSPROC        glDeleteBuffers;
    extern PFNGLBINDBUFFERPROC           glBindBuffer;
    extern PFNGLBUFFERDATAPROC           glBufferData;
    extern PFNGLBUFFERSUBDATAPROC        glBufferSubData;
    extern PFNGLBINDBUFFERBASEPROC       glBindBufferBase;
    extern PFNGLGENVERTEXARRAYSPROC      glGenVertexArrays;
    extern PFNGLDELETEVERTEXARRAYSPROC   glDeleteVertexArrays;
    extern PFNGLBINDVERTEXARRAYPROC      glBindVertexArray;
    extern PFNGLGENFRAMEBUFFERSPROC      glGenFramebuffers;
    extern PFNGLDELETEFRAMEBUFFERSPROC   glDeleteFramebuffers;
    extern PFNGLBINDFRAMEBUFFERPROC      glBindFramebuffer;
    extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
    extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
    extern PFNGLGENSAMPLERSPROC          glGenSamplers;
    extern PFNGLDELETESAMPLERSPROC       glDeleteSamplers;
    extern PFNGLBINDSAMPLERPROC          glBindSampler;
    extern PFNGLSAMPLERPARAMETERIPROC    glSamplerParameteri;
    extern PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray;
    extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
    extern PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer;
    extern PFNGLUNIFORMMATRIX4FVPROC         glUniformMatrix4fv;
    extern PFNGLGENERATEMIPMAPPROC           glGenerateMipmap;

    namespace Visualizer::Graphics {
        bool InitGL33Loader();
    }
#endif
