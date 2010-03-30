// FIXME applicatie crashed als je sluit soms als je met Debug afspeelt

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <psapi.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include "vineo.h"


#define WINDOW_CLASS "Vineo"
#define WINDOW_TITLE "Vineo example"
#define MOUSE_LB 250
#define MOUSE_RB 251


typedef struct ArgsOpenVineo {
    Vineo *v;
    char file[256];
} ArgsOpenVineo;

int screen_w = 854;
int screen_h = 480;
char keys[256];
int mouse_x = 0;
int mouse_y = 0;


ALfloat g_sndListPos[3];
ALfloat g_sndListOri[6];


void disableOpenGL( HWND, HDC, HGLRC );
void enableOpenGL( HWND hwnd, HDC*, HGLRC* );
LRESULT CALLBACK WindowProc( HWND, UINT, WPARAM, LPARAM );
void PrintMemoryInfo( DWORD processID );
void disableOpenAL();
void enableOpenAL();
void resizeScene( int width, int height );
void texCube();


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    WNDCLASSEX wcex;
    HWND hwnd;
    MSG msg;
    BOOL bQuit = FALSE;
    HDC hDC;
    HGLRC hRC;


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


    if( !RegisterClassEx( &wcex ) ) {
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



    int i;
    int num_vid = 3;

    char *media[] = {
        "C:/Users/vin777/Documents/Dump/Encode/fulco/howtotrainyourdragon-tlr2_h720p_fulco.mp4",
        "C:/Users/vin777/Documents/Dump/Encode/fulco/julieandjulia-tlr1_h720p_fulco.mp4",
        "http://tweakimg.net/g/if/v2/breadcrumb/award_2009_transparent.png",
    };

    Vineo *v[num_vid];

    for( i = 0; i < num_vid; i++ )
    {
        v[i] = vineoNew();

        vineoOpen( v[i], media[i] );
        vineoPlay( v[i] );

        glBindTexture( GL_TEXTURE_2D, v[i]->tex_gl );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    }

    float r = 0.0f;
    float x = 0.0f;
    float x_space = 3.0f;
    int num_box = 15;

    int64_t time = av_gettime();
    int64_t ptime = time;


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

            float fps = (float)AV_TIME_BASE / (float)( time - ptime );
            char title[32];
            printf( "fps: %f\n", fps );
            sprintf( title, "fps: %f", fps );
            SetWindowText( hwnd, title );


            ptime = time;
            time = av_gettime();


            if( keys[VK_ESCAPE] ) {
                bQuit = 1;
            }

            if( keys['R'] )
            {
                for( i = 0; i < num_vid; i++ )
                {
                    vineoClose( v[i] );
                    v[i] = vineoNew();
                    vineoOpen( v[i], media[i] );
                    vineoPlay( v[i] );
                }
            }


            for( i = 0; i < num_vid; i++ )
            {
                vineoDecode( v[i] );
                vineoFlush( v[i] );
            };


            x += (float)( time - ptime ) / 1000000.0f;

            if( x > num_vid * x_space ) {
                x -= num_vid * x_space;
            }

            r += (float)( time - ptime ) / 100000.0f;


            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();


            // camera
            glRotatef( 45.0f, 0.0f, 1.0f, 0.0f );
            glTranslatef( -x, -0.8f, -8.0f );


            // spiegel reflectie
            for( i = 0; i < num_box; i++ )
            {
                glPushMatrix();
                    glBindTexture( GL_TEXTURE_2D, v[i%num_vid]->tex_gl );
                    glColor3f( 1.0f, 1.0f, 1.0f );
                    glTranslatef( i * x_space, -1.2f, 0.0f );
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
            for( i = 0; i < num_box; i++ )
            {
                glPushMatrix();
                    glBindTexture( GL_TEXTURE_2D, v[i%num_vid]->tex_gl );
                    glColor3f( 1.0f, 1.0f, 1.0f );
                    glTranslatef( i * x_space, 1.2f, 0.0f );
                    glRotatef( r, 0.0f, 1.0f, 0.0f );
                    texCube();
                glPopMatrix();
            }


            SwapBuffers( hDC );
            //PrintMemoryInfo( GetCurrentProcessId() );
        }
    }


    for( i = 0; i < num_vid; i++ ) {
        vineoClose( v[i] );
    };


    disableOpenAL();
    disableOpenGL( hwnd, hDC, hRC );
    DestroyWindow( hwnd );
    return msg.wParam;
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

