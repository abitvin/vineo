// FIXME applicatie crashed als je sluit soms als je met Debug afspeelt


#include <X11/X.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <pthread.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include "vineo.h"



#define MOUSE_LB 250
#define MOUSE_RB 251


int screen_w = 854;
int screen_h = 480;
char keys[256];
int mouse_x = 0;
int mouse_y = 0;


ALfloat g_sndListPos[3];
ALfloat g_sndListOri[6];


void disableOpenGL();
void enableOpenGL();
void GLAPIENTRY gluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar );
void aSyncOpen( Vineo *v );
void disableOpenAL();
void enableOpenAL();
void resizeScene( int width, int height );
void texCube();


int main(int argc, char **argv)
{
    static int snglBuf[] = {GLX_RGBA, GLX_DEPTH_SIZE, 16, None};
    static int dblBuf[]  = {GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None};

    Display *dpy;
    Window win;
    GLboolean  doubleBuffer = GL_TRUE;
    XVisualInfo *vi;
    Colormap cmap;
    XSetWindowAttributes swa;
    GLXContext cx;
    XEvent event;
    GLboolean needRedraw = GL_FALSE, recalcModelView = GL_TRUE;
    int dummy;

    if( ( dpy = XOpenDisplay( NULL ) ) == NULL )
    {
        printf( "could not open display\n" );
        exit( 1 );
    }

    if( !glXQueryExtension( dpy, &dummy, &dummy ) )
    {
        printf( "X server has no OpenGL GLX extension\n" );
        exit( 1 );
    }


    if( ( vi = glXChooseVisual( dpy, DefaultScreen( dpy ), dblBuf ) ) == NULL )
    {
        if( ( vi = glXChooseVisual( dpy, DefaultScreen( dpy ), snglBuf ) ) == NULL )
        {
            printf( "no RGB visual with depth buffer\n" );
            exit( 1 );
        }

        doubleBuffer = GL_FALSE;
    }

    if( vi->class != TrueColor )
    {
        printf( "TrueColor visual required for this program\n" );
        exit( 1 );
    }

    if( ( cx = glXCreateContext( dpy, vi, None, GL_TRUE ) ) == NULL )
    {
        printf( "could not create rendering context\n" );
        exit( 1 );
    }

    cmap = XCreateColormap( dpy, RootWindow( dpy, vi->screen ), vi->visual, AllocNone );
    swa.colormap = cmap;
    swa.border_pixel = 0;
    swa.event_mask = KeyPressMask | KeyReleaseMask | ExposureMask | ButtonPressMask | StructureNotifyMask;

    win = XCreateWindow(
        dpy, RootWindow(dpy, vi->screen), 0, 0,
        screen_w, screen_h, 0, vi->depth, InputOutput, vi->visual,
        CWBorderPixel | CWColormap | CWEventMask, &swa
    );

    XSetStandardProperties( dpy, win, "main", "main", None, argv, argc, NULL );
    glXMakeCurrent( dpy, win, cx );
    XMapWindow( dpy, win );


    enableOpenAL();
    resizeScene( screen_w, screen_h );

    glEnable( GL_TEXTURE_2D );
    glClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
    glClearDepth( 1.0f );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
    glShadeModel( GL_SMOOTH );
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );




    char media[] = "/home/vin777/work/valentinesday-tlr1_h720p_fulco.mp4";
    //char media[] = "../../stuff/image.png";
    //char media[] = "./stuff/video.gif";
    //char media[] = "./stuff/image.jpg";
    //char media[] = "http://84.87.38.101/IMAGE.JPG";
    //char media[] = "http://tweakimg.net/g/if/v2/breadcrumb/award_2009_transparent.png";
    //char media[] = "http://ccms.e-billboard.eu/webcam/?id=19#.jpg";
    //char media[] = "http://scfire-mtc-aa03.stream.aol.com:80/stream/1025#.mp3";
    //char media[] = "http://scfire-ntc-aa03.stream.aol.com:80/stream/1007";
    //char media[] = "./stuff/music.mp3";




    float r = 0.0f;
    int64_t time = av_gettime();
    int64_t ptime = time;

    //Vineo *v = NULL;
    Vineo *v = vineoNew();
    //v->custom = media;
    vineoOpen( v, media );
    vineoPlay( v );
    //int tex = v->texGL;
    //int tex = 0;

    glBindTexture( GL_TEXTURE_2D, v->tex_gl );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );


    //pthread_t tVineo;
    //pthread_create( &tVineo, NULL, (void*)aSyncOpen, (void*)v );
    //pthread_join( tVineo, NULL );


    int64_t count = 0;
    int quit = 0;
    KeySym key;

    while( !quit )
    {
        while( XPending( dpy ) > 0 )
        {
            XNextEvent( dpy, &event );

            switch( event.type )
            {
                case Expose:
                {
                    if( event.xexpose.count != 0 ) {
                        break;
                    }

                    //drawGLScene();
                    break;
                }

                case ConfigureNotify:
                {
                    glViewport( 0, 0, event.xconfigure.width, event.xconfigure.height );
                    break;
                }

                /*case ButtonPress:
                {
                    done = 1;
                    break;
                }*/

                case KeyPress:
                {
                    keys[XLookupKeysym( &event.xkey, 0 )] = 1;
                    break;
                }

                case KeyRelease:
                {
                    keys[XLookupKeysym( &event.xkey, 0 )] = 0;
                    break;
                }

                case ClientMessage:
                {
                    if( *XGetAtomName( dpy, event.xclient.message_type) == *"WM_PROTOCOLS" )
                    {
                        printf( "Exiting sanely...\n" );
                        quit = 1;
                    }
                    break;
                }

                default:
                {
                    break;
                }
            }
        }



    /*
        if( keys[VK_ESCAPE] ) {
            bQuit = 1;
        }

        if( keys['C'] )
        {
            vineoClose( v );
            tex = 0;
            v = NULL;
        }

        if( keys['O'] )
        {
            if( !v )
            {
                v = vineoNew();
                vineoOpen( v, media );
                vineoPlay( v );
                tex = v->texGL;
                //tex = 0;
            }
        }

        if( keys['R'] )
        {
            vineoClose( v );
            v = vineoNew();
            vineoOpen( v, media );
            vineoPlay( v );
            tex = v->texGL;
            //tex = 0;
        }
*/

        ptime = time;
        time = av_gettime();
        count += ( time - ptime );

        /*
        if( count > AV_TIME_BASE * 5 )
        {
            count -= AV_TIME_BASE * 5;

            vineoClose( v );
            v = vineoNew();
            vineoOpen( v, media );
            vineoPlay( v );
            tex = v->texGL;
        }
        */


        if( keys['q'] ) {
            quit = 1;
        }

        if( keys['r'] ) {
            r *= 1.05f;
        }




        r += (float)( time - ptime ) / 100000.0f;

        if( v->is_playing ) {
            vineoDecode( v );
        }

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        glPushMatrix();
            glTranslatef( 0.0f, 0.0f, -6.0f );
            glRotatef( r, 0.0f, 1.0f, 0.0f );
            //glBindTexture( GL_TEXTURE_2D, tex );
            glBindTexture( GL_TEXTURE_2D, v->tex_gl );
            texCube();
        glPopMatrix();

        if( doubleBuffer ) {
            glXSwapBuffers( dpy, win );
        }
        else {
            glFlush();
        }


        /*
        MEM USAGE, van internet geplukt

        FILE *f;
        char buf[30];
        snprintf( buf, 30, "/proc/%u/statm", (unsigned)getpid() );

        if( f = fopen( buf, "r" ) )
        {
            unsigned size;      // total program size
            unsigned resident;  // resident set size
            unsigned share;     // shared pages
            unsigned text;      // text (code)
            unsigned lib;       // library
            unsigned data;      // data/stack
            unsigned dt;        // dirty pages (unused in Linux 2.6)
            fscanf( f, "%u %u %u %u %u %u", &size, &resident, &share, &text, &lib, &data );
            fclose( f );

            printf( "\rmem used: %uMb (%u bytes)", size / 1024, size );
        }
        */
    }

    vineoClose( v );
    disableOpenAL();

    return 0;
}


void aSyncOpen( Vineo *v )
{
    //vineoOpen( v, v->custom );
    vineoOpen( v, "/home/xxx/video.mp4" );
    vineoPlay( v );

    // FIXME netjes uit de thread komen ipv oneindige loop
    //while( 1 ) {
    //    vineoDecode( v );
    //}
}

void disableOpenAL()
{
	alutExit();
}

void enableOpenAL()
{
    alutInit( NULL, NULL );
    alGetError();

    // FIXME voor bilbo, moeten we dit instellen?
    g_sndListPos[0] = 0.0f;
    g_sndListPos[1] = 0.0f;
    g_sndListPos[2] = 0.0f;
    alListenerfv( AL_POSITION, g_sndListPos );

    // 2D(x,y) sound orientation
	g_sndListOri[0] = 0.0f;
	g_sndListOri[1] = 1.0f;
	g_sndListOri[2] = 0.0f;
	g_sndListOri[3] = 0.0f;
	g_sndListOri[4] = 0.0f;
	g_sndListOri[5] = 1.0f;
	alListenerfv( AL_ORIENTATION, g_sndListOri );

}


void resizeScene( int width, int height )
{
    glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	//glOrtho( 0, width, height, 0, -99999, 99999 );
	gluPerspective( 45.0f, width / height, 0.1f, 100.0f );

	glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
	glViewport( 0, 0, width, height );
}


void texCube()
{
    glBegin( GL_QUADS );
        glTexCoord2f( 1, 0 ); glVertex3f( 1.0f, 1.0f,-1.0f );			// Top Right Of The Quad (Top)
        glTexCoord2f( 0, 0 ); glVertex3f(-1.0f, 1.0f,-1.0f );			// Top Left Of The Quad (Top)
        glTexCoord2f( 0, 1 ); glVertex3f(-1.0f, 1.0f, 1.0f );			// Bottom Left Of The Quad (Top)
        glTexCoord2f( 1, 1 ); glVertex3f( 1.0f, 1.0f, 1.0f );			// Bottom Right Of The Quad (Top)

        glTexCoord2f( 1, 0 ); glVertex3f( 1.0f,-1.0f, 1.0f );			// Top Right Of The Quad (Bottom)
        glTexCoord2f( 0, 0 ); glVertex3f(-1.0f,-1.0f, 1.0f );			// Top Left Of The Quad (Bottom)
        glTexCoord2f( 0, 1 ); glVertex3f(-1.0f,-1.0f,-1.0f );			// Bottom Left Of The Quad (Bottom)
        glTexCoord2f( 1, 1 ); glVertex3f( 1.0f,-1.0f,-1.0f );			// Bottom Right Of The Quad (Bottom)

        glTexCoord2f( 1, 0 ); glVertex3f( 1.0f, 1.0f, 1.0f );			// Top Right Of The Quad (Front)
        glTexCoord2f( 0, 0 ); glVertex3f(-1.0f, 1.0f, 1.0f );			// Top Left Of The Quad (Front)
        glTexCoord2f( 0, 1 ); glVertex3f(-1.0f,-1.0f, 1.0f );			// Bottom Left Of The Quad (Front)
        glTexCoord2f( 1, 1 ); glVertex3f( 1.0f,-1.0f, 1.0f );			// Bottom Right Of The Quad (Front)

        glTexCoord2f( 1, 1 ); glVertex3f( 1.0f,-1.0f,-1.0f );			// Bottom Left Of The Quad (Back)
        glTexCoord2f( 0, 1 ); glVertex3f(-1.0f,-1.0f,-1.0f );			// Bottom Right Of The Quad (Back)
        glTexCoord2f( 0, 0 ); glVertex3f(-1.0f, 1.0f,-1.0f );			// Top Right Of The Quad (Back)
        glTexCoord2f( 1, 0 ); glVertex3f( 1.0f, 1.0f,-1.0f );			// Top Left Of The Quad (Back)

        glTexCoord2f( 1, 0 ); glVertex3f(-1.0f, 1.0f, 1.0f );			// Top Right Of The Quad (Left)
        glTexCoord2f( 0, 0 ); glVertex3f(-1.0f, 1.0f,-1.0f );			// Top Left Of The Quad (Left)
        glTexCoord2f( 0, 1 ); glVertex3f(-1.0f,-1.0f,-1.0f );			// Bottom Left Of The Quad (Left)
        glTexCoord2f( 1, 1 ); glVertex3f(-1.0f,-1.0f, 1.0f );			// Bottom Right Of The Quad (Left)

        glTexCoord2f( 1, 0 ); glVertex3f( 1.0f, 1.0f,-1.0f );			// Top Right Of The Quad (Right)
        glTexCoord2f( 0, 0 ); glVertex3f( 1.0f, 1.0f, 1.0f );			// Top Left Of The Quad (Right)
        glTexCoord2f( 0, 1 ); glVertex3f( 1.0f,-1.0f, 1.0f );			// Bottom Left Of The Quad (Right)
        glTexCoord2f( 1, 1 ); glVertex3f( 1.0f,-1.0f,-1.0f );			// Bottom Right Of The Quad (Right)
    glEnd();
}


void GLAPIENTRY gluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
    GLdouble ymax = zNear * tan( fovy * M_PI / 360.0f );
    GLdouble ymin = -ymax;
    GLdouble xmin = ymin * aspect;
    GLdouble xmax = ymax * aspect;

    glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}
