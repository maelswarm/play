#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include "portaudio.h"
#include "sndfile.h"

/*
 Data structure to pass to callback
 includes the sound file, info about the sound file, and a position
 cursor (where we are in the sound file)
 */

typedef struct SNDData {
    SNDFILE *sndFile;
    SF_INFO sfInfo;
    int position;
} SNDData;

SNDData *sndData;

int loop;
int *cursor;

/*
 Callback function for audio output
 */
int Callback(const void *input,
             void *output,
             unsigned long frameCount,
             const PaStreamCallbackTimeInfo* paTimeInfo,
             PaStreamCallbackFlags statusFlags,
             void *userData) {
    
    SNDData *data = (SNDData *)userData; /* we passed a data structure
                                          into the callback so we have something to work with */
    /* current pointer into the output  */
    int *out = (int *)output;
    int thisSize = frameCount;
    int thisRead;
    
    cursor = out; /* set the output cursor to the beginning */
    while (thisSize > 0) {
        /* seek to our current file position */
        sf_seek(data->sndFile, data->position, SEEK_SET);
        
        /* are we going to read past the end of the file?*/
        if (thisSize > (data->sfInfo.frames - data->position) && loop) {
            /*if we are, only read to the end of the file*/
            thisRead = data->sfInfo.frames - data->position;
            /* and then loop to the beginning of the file */
            data->position = 0;
        } else {
            /* otherwise, we'll just fill up the rest of the output buffer */
            thisRead = thisSize;
            /* and increment the file position */
            data->position += thisRead;
        }
        
        /* since our output format and channel interleaving is the same as
         sf_readf_int's requirements */
        /* we'll just read straight into the output buffer */
        sf_readf_int(data->sndFile, cursor, thisRead);
        /* increment the output cursor*/
        cursor += thisRead;
        /* decrement the number of samples left to process */
        thisSize -= thisRead;
    }
    return paContinue;
}

/*******************************************************************/

struct vertex {
    double x;
    double y;
    double z;
};

struct lines {
    struct vertex *vert1;
    struct vertex *vert2;
};

struct lines **lines = NULL;

void init() // Called before main loop to set up the program
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
}

// Called at the start of the program, after a glutPostRedisplay() and during idle
// to display a frame
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    free(lines[0]->vert1);
    free(lines[0]->vert2);
    free(lines[0]);
    
    for (int i=1; i<1000; i++) {
        lines[i]->vert1->x -= .1f;
        lines[i]->vert2->x -= .1f;
        lines[i-1] = lines[i];
        
        glBegin(GL_LINES);
        glVertex3f(lines[i-1]->vert1->x, lines[i-1]->vert1->y, lines[i-1]->vert1->z);
        glVertex3f(lines[i-1]->vert2->x,lines[i-1]->vert2->y, lines[i-1]->vert2->z);
        glEnd();
    }
    lines[999] = malloc(sizeof(struct lines));
    lines[999]->vert1 = malloc(sizeof(struct vertex));
    lines[999]->vert1->x = 49.9;
    lines[999]->vert1->y = lines[998]->vert2->y;
    lines[999]->vert1->z = -250;
    lines[999]->vert2 = malloc(sizeof(struct vertex));
    lines[999]->vert2->x = 50.0;
    if (cursor != NULL) {
        lines[999]->vert2->y = ((double)*cursor)/(INT_MAX/10.0);
    }
    lines[999]->vert2->z = -250;
    glutSwapBuffers();
}

// Called every time a window is resized to resize the projection matrix
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.1, 0.1, -1 * (float)h/(10.0*(float)w), (float)h/(10.0*(float)w), 0.5, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void glLoop()
{
    // Update the positions of the objects
    // ......
    // ......
    glutPostRedisplay();
    glutTimerFunc(1, glLoop, 0);
}

int main(int argc, char **argv)
{
    loop = 0;
    char *filename;
    int visualization = 0;
    int verbose = 0;
    double duration = 0.0f;
    
    cursor = NULL;
    
    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-w")) {
            visualization = 1;
        } else if (!strcmp(argv[i], "-l")) {
            loop = 1;
        } else if (!strcmp(argv[i], "-v")) {
            verbose = 1;
        } else {
            filename = argv[i];
        }
    }
    
    lines = malloc(1000*sizeof(struct lines*));
    
    if (lines == NULL) {
        return 1;
    }
    
    for (int i=0; i<1000; i++) {
        lines[i] = malloc(sizeof(struct lines));
        lines[i]->vert1 = malloc(sizeof(struct vertex));
        lines[i]->vert1->x = 0.0;
        lines[i]->vert1->y = 0.0;
        lines[i]->vert1->z = -500.0;
        lines[i]->vert2 = malloc(sizeof(struct vertex));
        lines[i]->vert2->x = 0.0;
        lines[i]->vert2->y = 0.0;
        lines[i]->vert2->z = -500.0;
    }
    
    sndData = (SNDData *)malloc(sizeof(SNDData));
    PaStream *stream;
    PaError error;
    PaStreamParameters outputParameters;
    
    sndData->position = 0;
    sndData->sfInfo.format = 0;
    sndData->sndFile = sf_open(filename, SFM_READ, &sndData->sfInfo);
    
    if (!sndData->sndFile) {
        printf("error opening file\n");
        return 1;
    }
    duration = ((double)sndData->sfInfo.frames/(double)sndData->sfInfo.samplerate);
    /* start portaudio */
    Pa_Initialize();
    
    /* set the output parameters */
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* use the
                                                            default device */
    outputParameters.channelCount = sndData->sfInfo.channels; /* use the
                                                            same number of channels as our sound file */
    outputParameters.sampleFormat = paInt32; /* 32bit int format */
    outputParameters.suggestedLatency = 0.01; /* 200 ms ought to satisfy
                                              even the worst sound card */
    outputParameters.hostApiSpecificStreamInfo = 0; /* no api specific data */
    
    /* try to open the output */
    error = Pa_OpenStream(&stream,  /* stream is a 'token' that we need
                                     to save for future portaudio calls */
                          0,  /* no input */
                          &outputParameters,
                          sndData->sfInfo.samplerate,  /* use the same
                                                     sample rate as the sound file */
                          paFramesPerBufferUnspecified,  /* let
                                                          portaudio choose the buffersize */
                          paNoFlag,  /* no special modes (clip off, dither off) */
                          Callback,  /* callback function defined above */
                          sndData); /* pass in our data structure so the
                                   callback knows what's up */
    
    /* if we can't open it, then bail out */
    if (error)
    {
        printf("error opening output, error code = %i\n", error);
        Pa_Terminate();
        return 1;
    }
    
    /* when we start the stream, the callback starts getting called */
    Pa_StartStream(stream);
    
    if (verbose) {
        printf("Filename: %s\n", filename);
        printf("Duration: %f seconds\n", duration);
        printf("Sample Rate: %d\n", sndData->sfInfo.samplerate);
    }
    
    if (visualization) {
        //glutInit(&argc, argv); // Initializes glut
        
        // Sets up a double buffer with RGBA components and a depth component
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
        
        // Sets the window size to 512*512 square pixels
        glutInitWindowSize(512, 512);
        
        // Sets the window position to the upper left
        glutInitWindowPosition(0, 0);
        // Creates a window using internal glut functionality
        glutCreateWindow(filename);
        // passes reshape and display functions to the OpenGL machine for callback
        glutReshapeFunc(reshape);
        glutDisplayFunc(display);
        glutIdleFunc(display);
        
        init();
        
        glutTimerFunc(1, glLoop, 0);
        // Starts the program.
        glutMainLoop();
    }
    
    if (!visualization && !loop) {
        Pa_Sleep(duration*1000.0f);
    } else if (!visualization && loop) {
        Pa_Sleep(LONG_MAX);
    }
    
    Pa_StopStream(stream);
    
    return 0;
}
