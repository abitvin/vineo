// FIXME applicatie crashed als je sluit soms als je met Debug afspeelt


//#define PLATFORM_LINUX
//#define PLATFORM_WINDOWS
#define WINDOW_CLASS "Vineo"
#define WINDOW_TITLE "Vineo example"

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <psapi.h>
#else
    #include <X11/X.h>
    #include <X11/keysym.h>
    #include <GL/glx.h>
    #include <GL/gl.h>
    #include <pthread.h>
#endif
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


#ifdef PLATFORM_WINDOWS
    void disableOpenGL( HWND, HDC, HGLRC );
    void enableOpenGL( HWND hwnd, HDC*, HGLRC* );
    LRESULT CALLBACK WindowProc( HWND, UINT, WPARAM, LPARAM );
    void PrintMemoryInfo( DWORD processID );
#else
    void disableOpenGL();
    void enableOpenGL();
    void GLAPIENTRY gluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar );
#endif
void aSyncOpen( Vineo *v );
void disableOpenAL();
void enableOpenAL();
void resizeScene( int width, int height );
void texCube();


#ifdef PLATFORM_WINDOWS
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = WINDOW_CLASS;
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;


    if( !RegisterClassEx( &wcex )) {
        return 0;
    }

    hwnd = CreateWindowEx(
        0, WINDOW_CLASS, WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        screen_w + GetSystemMetrics( SM_CYFRAME ) * 2,
        screen_h + GetSystemMetrics( SM_CYFRAME ) * 2 + GetSystemMetrics( SM_CYCAPTION ),
        NULL, NULL, hInstance, NULL
    );


    ShowWindow( hwnd, nCmdShow );
    enableOpenGL( hwnd, &hDC, &hRC );
    enableOpenAL();
    resizeScene( screen_w, screen_h );

    glEnable( GL_TEXTURE_2D );
    glClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
    glClearDepth( 1.0f );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
    glShadeModel( GL_SMOOTH );
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );



    // FIXME NOPE...audio en video out of sync, packet queue is wel goed
    //char *file = "C:/Users/vin777/Documents/Dump/Films/Monsters.Vs.Aliens.DVDRiP.XviD-JUMANJi/jmj-mona.avi";

    // FIXME NOPE...geen audio codec gevonden
    //char *file = "C:/Users/vin777/Documents/Dump/Films/Monsters.Vs.Aliens.2009.1080p.BluRay.x264-CiNEFiLE.mkv";

    // FIXME OK...foutje in begin van de file maar voor de rest gaat het uitstekend
    //char *file = "C:/Users/vin777/Documents/Dump/Films/Transformers Revenge of the Fallen[2009]DvDrip[Eng]-FXG/Transformers Revenge of the Fallen[2009]DvDrip[Eng]-FXG.avi";

    // De avi file is ook niet helemaal perfect, loopt goed.
    // FIXME NOPE...foutje in begin file en video packet queue buffer altijd leeg?
    //char *file = "../stuff/video.avi";

    // FIXME UPDATE...OK! Stream had een pFormatCtx->start_time, werkt nu! Maar waarom is de audio packet queue nog vol aan het einde?
    // NOPE...audio en beeld out of sync, packet queue is wel goed
    //char *file = "../stuff/video.wmv";


    /*
    int i;
    int numVid = 1;
    int tex[numVid];

    char *media[] = {
        "C:/Users/vin777/Documents/Dump/Encode/fulco/avatar-tlrf_h720p_fulco.mov",
        "C:/Users/vin777/Documents/Dump/Films/Monsters.Vs.Aliens.DVDRiP.XviD-JUMANJi/jmj-mona.avi",
        "C:/Users/vin777/Documents/Dump/Films/Terminator.Salvation.DVDSCR.XViD-ANALSHiT.[www.FilmsBT.com]/Terminator.Salvation.DVDSCR.XViD-ANALSHiT.avi",
    };

    Vineo **v = malloc( numVid * sizeof(Vineo*) );

    for( i = 0; i < numVid; i++ )
    {
        v[i] = vineoNew();
        vineoOpen( v[i], media[i] );
        vineoPlay( v[i] );
        tex[i] = v[i]->texGL;
    }

    float r = 0.0f;
    float x = 0.0f;
    float xSpace = 3.0f;
    int numBox = 15;

    int64_t time = av_gettime();
    int64_t ptime = time;
    */



    char media[] = "C:/Users/vin777/Documents/Dump/Encode/fulco/valentinesday-tlr1_h720p_fulco.mp4";
    //char media[] = "http://scfire-mtc-aa03.stream.aol.com:80/stream/1025";
    //char media[] = "http://72.26.204.18:6256#.aac";
    //char media[] = "./stuff/image.png";
    //char media[] = "./stuff/video.gif";
    //char media[] = "./stuff/image.jpg";
    //char media[] = "http://tweakimg.net/g/if/v2/breadcrumb/award_2009_transparent.png";
    //char media[] = "http://ccms.e-billboard.eu/webcam/?id=19#.jpg";
    //char media[] = "./stuff/music.mp3";


    float r = 0.0f;
    int64_t time = av_gettime();
    int64_t ptime = time;

    //Vineo *v = NULL;
    Vineo *v = vineoNew();
    vineoOpen( v, media );
    vineoPlay( v );

    glBindTexture( GL_TEXTURE_2D, v->tex_gl );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );


    //Vineo *v2 = NULL;
    //Vineo *v2 = vineoNew();
    //vineoOpen( v2, media );
    //vineoPlay( v2 );

    //int tex = v->texGL;
    int tex = 0;
    //int mediaLoaded = 0;


    int64_t count = 0;


    while( !bQuit )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            if( msg.message == WM_QUIT )
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }
        else
        {
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
                    tex = v->tex_gl;
                    //tex = 0;
                }
            }

            if( keys['R'] )
            {
                vineoClose( v );
                v = vineoNew();
                vineoOpen( v, media );
                vineoPlay( v );
                tex = v->tex_gl;
                //tex = 0;
            }


            ptime = time;
            time = av_gettime();
            count += ( time - ptime );

/*
            if( count > AV_TIME_BASE * 5 )
            {
                count -= AV_TIME_BASE * 5;

                vineoClose( v );
                vineoClose( v2 );
                PrintMemoryInfo( GetCurrentProcessId() );
                printf( "%i ###########################################\n", ++mediaLoaded );
                v = vineoNew();
                v2 = vineoNew();
                vineoOpen( v, media );
                vineoOpen( v2, media );
                vineoPlay( v );
                vineoPlay( v2 );
                tex = v->texGL;
            }
*/

            r += (float)( time - ptime ) / 100000.0f;

            vineoDecode( v );
            //vineoDecode( v2 );

            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
            glPushMatrix();
                glTranslatef( 0.0f, 0.0f, -6.0f );
                glRotatef( r, 0.0f, 1.0f, 0.0f );
                glBindTexture( GL_TEXTURE_2D, v->tex_gl );
                texCube();
            glPopMatrix();




            /*
            ptime = time;
            time = av_gettime();


            if( keys[VK_ESCAPE] ) {
                bQuit = 1;
            }

            if( keys['R'] )
            {
                for( i = 0; i < numVid; i++ )
                {
                    vineoClose( v[i] );
                    v[i] = vineoNew();
                    vineoOpen( v[i], media[i] );
                    vineoPlay( v[i] );
                    tex[i] = v[i]->texGL;
                }
            }


            for( i = 0; i < numVid; i++ ) {
                vineoDecode( v[i] );
            };


            x += (float)( time - ptime ) / 1000000.0f;

            if( x > numVid * xSpace ) {
                x -= numVid * xSpace;
            }

            r += (float)( time - ptime ) / 100000.0f;


            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();


            // camera
            glRotatef( 45.0f, 0.0f, 1.0f, 0.0f );
            glTranslatef( -x, -0.8f, -8.0f );


            // spiegel reflectie
            for( i = 0; i < numBox; i++ )
            {
                glPushMatrix();
                    glBindTexture( GL_TEXTURE_2D, tex[i%numVid] );
                    glColor3f( 1.0f, 1.0f, 1.0f );
                    glTranslatef( i * xSpace, -1.2f, 0.0f );
                    glRotatef( r, 0.0f, 1.0f, 0.0f );
                    glScalef( 1.0f, -1.0f, 1.0f );
                    texCube();
                glPopMatrix();
            }


            // vloer
            glPushMatrix();
                glBindTexture( GL_TEXTURE_2D, 0 );
                glColor4f( 0.0f, 0.0f, 0.0f, 0.8f );
                glBegin( GL_QUADS );
                    glVertex3f( 100.0f, 0.0f,-100.0f );
                    glVertex3f(-100.0f, 0.0f,-100.0f );
                    glVertex3f(-100.0f, 0.0f, 100.0f );
                    glVertex3f( 100.0f, 0.0f, 100.0f );
                glEnd();
            glPopMatrix();


            // top
            for( i = 0; i < numBox; i++ )
            {
                glPushMatrix();
                    glBindTexture( GL_TEXTURE_2D, tex[i%numVid] );
                    glColor3f( 1.0f, 1.0f, 1.0f );
                    glTranslatef( i * xSpace, 1.2f, 0.0f );
                    glRotatef( r, 0.0f, 1.0f, 0.0f );
                    texCube();
                glPopMatrix();
            }
            */


            SwapBuffers( hDC );


            PrintMemoryInfo( GetCurrentProcessId() );
        }
    }


    /*
    for( i = 0; i < numVid; i++ ) {
        vineoClose( v[i] );
    };

    free( v );
    */

    vineoClose( v );
    //vineoClose( v2 );

    disableOpenAL();
    disableOpenGL( hwnd, hDC, hRC );
    DestroyWindow( hwnd );
    return msg.wParam;
}
#else
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
    enableOpenGL();
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




    //char media[] = "/home/vin777/tests/avatar.mov";
    //char media[] = "../../stuff/image.png";
    //char media[] = "./stuff/video.gif";
    //char media[] = "./stuff/image.jpg";
    //char media[] = "http://84.87.38.101/IMAGE.JPG";
    //char media[] = "http://tweakimg.net/g/if/v2/breadcrumb/award_2009_transparent.png";
    //char media[] = "http://ccms.e-billboard.eu/webcam/?id=19#.jpg";
    //char media[] = "http://scfire-mtc-aa03.stream.aol.com:80/stream/1025#.mp3";
    char media[] = "http://scfire-ntc-aa03.stream.aol.com:80/stream/1007";
    //char media[] = "./stuff/music.mp3";




    float r = 0.0f;
    int64_t time = av_gettime();
    int64_t ptime = time;

    //Vineo *v = NULL;
    Vineo *v = vineoNew();
    v->custom = media;
    //vineoOpen( v, media );
    //vineoPlay( v );
    //int tex = v->texGL;
    //int tex = 0;


    pthread_t tVineo;
    pthread_create( &tVineo, NULL, (void*)aSyncOpen, (void*)v );
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

        if( v->isPlaying ) {
            vineoDecode( v );
        }

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        glPushMatrix();
            glTranslatef( 0.0f, 0.0f, -6.0f );
            glRotatef( r, 0.0f, 1.0f, 0.0f );
            //glBindTexture( GL_TEXTURE_2D, tex );
            glBindTexture( GL_TEXTURE_2D, v->texGL );
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
    disableOpenGL();
    disableOpenAL();

    return 0;
}
#endif


void aSyncOpen( Vineo *v )
{
    vineoOpen( v, v->custom );
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

#ifdef PLATFORM_WINDOWS
void disableOpenGL( HWND hwnd, HDC hDC, HGLRC hRC )
{
    wglMakeCurrent( NULL, NULL );
    wglDeleteContext( hRC );
    ReleaseDC( hwnd, hDC );
}

void enableOpenGL( HWND hwnd, HDC* hDC, HGLRC* hRC )
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory( &pfd, sizeof(pfd) );
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    *hDC = GetDC( hwnd );
    SetPixelFormat( *hDC, ChoosePixelFormat(*hDC, &pfd), &pfd );
    *hRC = wglCreateContext(*hDC);
    wglMakeCurrent( *hDC, *hRC );
}
#else
void disableOpenGL()
{
    //
}

void enableOpenGL()
{
    //
}
#endif

void draw()
{

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


#ifdef PLATFORM_WINDOWS
LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_CLOSE: {
            PostQuitMessage(0); break;
        }

        case WM_DESTROY: {
            return 0;
        }

        case WM_LBUTTONDOWN: {
            keys[MOUSE_LB] = 1; break;
        }

        case WM_LBUTTONUP: {
            keys[MOUSE_LB] = 0; break;
        }

        case WM_KEYDOWN: {
            keys[wParam] = 1; break;
        }

        case WM_KEYUP: {
            keys[wParam] = 0; break;
        }

        case WM_MOUSEMOVE:
        {
            mouse_x = LOWORD( lParam );
            mouse_y = HIWORD( lParam );
            break;
        }

        case WM_RBUTTONDOWN: {
            keys[MOUSE_RB] = 1; break;
        }

        case WM_RBUTTONUP: {
            keys[MOUSE_RB] = 0; break;
        }

        case WM_SIZE:
		{
			resizeScene( screen_w = LOWORD(lParam), screen_h = HIWORD(lParam) ); break;
		}

        default: {
            return DefWindowProc( hwnd, uMsg, wParam, lParam );
        }
    }

    return 0;
}
#endif


#ifdef PLATFORM_LINUX
void GLAPIENTRY gluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
    GLdouble ymax = zNear * tan( fovy * M_PI / 360.0f );
    GLdouble ymin = -ymax;
    GLdouble xmin = ymin * aspect;
    GLdouble xmax = ymax * aspect;

    glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}
#endif


#ifdef PLATFORM_WINDOWS
void PrintMemoryInfo( DWORD processID )
{
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    //printf( "Process ID: %u\n", processID );

    if( ( hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID ) ) == NULL ) {
        return;
    }

    if( GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc) ) )
    {
        printf( "current: %u, peak: %u\n", pmc.WorkingSetSize, pmc.PeakWorkingSetSize );
        //printf( "\tPageFaultCount:             %u\n", pmc.PageFaultCount );
        //printf( "\tYour app's PEAK MEMORY CONSUMPTION:    %u\n", pmc.PeakWorkingSetSize );
        //printf( "\tYour app's CURRENT MEMORY CONSUMPTION: %u\n", pmc.WorkingSetSize );
        //printf( "\tQuotaPeakPagedPoolUsage:    %u\n", pmc.QuotaPeakPagedPoolUsage );
        //printf( "\tQuotaPagedPoolUsage:        %u\n", pmc.QuotaPagedPoolUsage );
        //printf( "\tQuotaPeakNonPagedPoolUsage: %u\n", pmc.QuotaPeakNonPagedPoolUsage );
        //printf( "\tQuotaNonPagedPoolUsage:     %u\n", pmc.QuotaNonPagedPoolUsage );
        //printf( "\tPagefileUsage:              %u\n", pmc.PagefileUsage );
        //printf( "\tPeakPagefileUsage:          %u\n", pmc.PeakPagefileUsage );
    }

    CloseHandle( hProcess );
}
#endif
