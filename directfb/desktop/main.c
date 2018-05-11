#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>

#include <pthread.h>
#include <math.h>
#include <time.h>

#include <sawman.h>

#include <directfb.h>
#include <directfb_util.h>
#include <directfb_version.h>

#include <direct/util.h>
#include <direct/messages.h>

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...) \
     {                                                                \
          err = x;                                                    \
          if (err != DFB_OK) {                                        \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
               DirectFBErrorFatal( #x, err );                         \
          }                                                           \
     }


DFBResult load_image(const char             *filename,
                     DFBSurfacePixelFormat   pixelformat,
                     IDirectFBSurface      **surface,
                     int                    *ret_width,
                     int                    *ret_height,
                     DFBImageDescription    *desc);
int run_task(char* path);
int run_term();


typedef struct _icon_default icon_default;
struct _icon_default
{
     IDirectFBSurface *surface;
};
icon_default icon_def;


typedef struct _icon icon;
struct _icon
{
     int count;
     IDirectFBSurface *surface;

     char* task_path;

     icon *next;
};

icon *icon_list = NULL;

void create_task_icon(char *task_path, IDirectFBSurface **surface)
{
     *surface = NULL;

     FILE *fp;
     char strLine[4096];
     if (NULL == (fp = fopen("icon.cfg", "r")))
     {
          return;
     }

     while (!feof(fp))
     {
          if (fgets(strLine, 4096, fp))
          {
               if (0 == strncmp(strLine, task_path, strlen(task_path)))
               {
                    char* path = strLine + strlen(task_path);

                    if (' ' == *path) 
                    {
                         while ('\0' != *path && ' ' == *path) 
                              path++;

                         int len = strlen(path);
                         int i;
                         for (i = len-1; i >= 0; i--)
                         {
                              if ('\n' == path[i] ||
                                  '\r' == path[i] ||
                                  '\t' == path[i] ||
                                  ' ' == path[i])
                                   len--;
                              else
                                   break;
                         }
                         if (len > 0)
                         {
                              path[len] = '\0';

                              load_image(path, 0, surface, NULL, NULL, NULL);
                         }
                    }
               }
          }
     }

     fclose(fp);

     if (!*surface)
     {
          *surface = icon_def.surface;
     }
}
void drawtext(char* str);
IDirectFBSurface *get_icon(char* task)
{
     icon* p = icon_list;
     while (p)
     {
          if (0 == strcmp(p->task_path, task))
          {
               p->count++;

               return p->surface;
          }

          p = p->next;
     }

     p = malloc(sizeof(icon));
     p->next = icon_list;
     icon_list = p;

     p->task_path = malloc(strlen(task) + 1);
     strcpy(p->task_path, task);

     create_task_icon(task, &p->surface);

     p->count = 1;

     return p->surface;
}

void release_icon(IDirectFBSurface *surface)
{
     icon* prev = NULL;
     icon* p = icon_list;
     while (p)
     {
          if (surface == p->surface)
          {
               p->count--;

               if (0 == p->count)
               {
                    p->surface->Release(p->surface);

                    if (prev)
                         prev->next = p->next;
                    else
                         icon_list = p->next;

                    p->next = NULL;
                    free(p->task_path);
                    free(p);
               }

               break;
          }

          prev = p;
          p = p->next;
     }
}

void release_icon_2(char* task)
{
     icon* prev = NULL;
     icon* p = icon_list;
     while (p)
     {
          if (0 == strcmp(task, p->task_path))
          {
               p->count--;

               if (0 == p->count)
               {
                    p->surface->Release(p->surface);

                    if (prev)
                         prev->next = p->next;
                    else
                         icon_list = p->next;

                    p->next = NULL;
                    free(p->task_path);
                    free(p);
               }

               break;
          }

          prev = p;
          p = p->next;
     }
}


IDirectFB             *dfb   = NULL;
IDirectFBDisplayLayer *layer = NULL;

ISaWMan        *saw     = NULL; 
ISaWManManager *manager = NULL; 

IDirectFBWindow        *window          = NULL;
IDirectFBSurface       *window_surface  = NULL;

IDirectFBWindow        *taskbar         = NULL;
IDirectFBSurface       *taskbar_surface = NULL;

int press_ctrl = 0;
int press_alt = 0;

bool desktop_init = false;
int  desktop_width = 0;
int  desktop_height = 0;
int  taskbar_height = 40;
int  iconbar_width = 175;



typedef struct _desktop_shortcut desktop_shortcut;
struct _desktop_shortcut
{
     IDirectFBSurface *surface;

     char* task_path;

     desktop_shortcut *next;
};

desktop_shortcut *desktop_shortcut_head  = NULL;
desktop_shortcut *desktop_shortcut_tail  = NULL;
int		  desktop_shortcut_count = 0;

bool create_desktop_shortcut(char *task_path)
{
     desktop_shortcut *node = malloc(sizeof(desktop_shortcut));
     if (!node)
          return false;

     node->surface = NULL;
     node->next = NULL;

     node->task_path = malloc(strlen(task_path) + 1);
     strcpy(node->task_path, task_path);

     node->surface = get_icon(task_path);

     /* lock */
     manager->Lock( manager );

     if (!desktop_shortcut_head)
     {
          desktop_shortcut_head = node;
          desktop_shortcut_tail = node;
     }
     else
     {
          desktop_shortcut_tail->next = node;
          desktop_shortcut_tail = node;
     }
     desktop_shortcut_count++;

     /* unlock */
     manager->Unlock( manager );

     return true;
}

void remove_desktop_shortcut(char *task_path)
{
     /* lock */
     manager->Lock( manager );

     desktop_shortcut* prev = NULL;
     desktop_shortcut* p = desktop_shortcut_head;
     while (p)
     {
          if (0 == strcmp(task_path, p->task_path))
          {
               if (prev)
                    prev->next = p->next;
               else
                    desktop_shortcut_head = p->next;

               if (!p->next)
                    desktop_shortcut_tail = prev;

               desktop_shortcut_count--;

               release_icon(p->surface);
               p->next = NULL;
               free(p->task_path);
               free(p);

               break;
          }

          prev = p;
          p = p->next;
     }

     /* unlock */
     manager->Unlock( manager );
}

void desktop_draw()
{
     int xpos = 4;
     int ypos = 4;

     /* lock */
     manager->Lock( manager );

     DFBRegion old_clip;
     window_surface->GetClip( window_surface, &old_clip );

     window_surface->Clear( window_surface, 0, 0, 0, 0 );

     desktop_shortcut *p = desktop_shortcut_head;
     while (p)
     {
          DFBRegion clip = { xpos, ypos, xpos+48-1, ypos+48+20-1 };
          window_surface->SetClip( window_surface, &clip );

          DFBRectangle rect = { xpos, ypos, 48, 48 };
          window_surface->SetBlittingFlags( window_surface, DSBLIT_BLEND_ALPHACHANNEL );
          window_surface->StretchBlit( window_surface, p->surface, NULL, &rect );

          window_surface->SetColor( window_surface, 0xCF, 0xCF, 0xFF, 0xFF );
          window_surface->SetDrawingFlags( window_surface, DSDRAW_SRC_PREMULTIPLY );
          char *name = strrchr(p->task_path, '/') + 1;
          window_surface->DrawString( window_surface, name, -1, xpos+24, ypos+48, DSTF_TOP | DSTF_CENTER );

          ypos += 48 + 30;
          if ((ypos+48+30) > (desktop_height - taskbar_height))
          {
               xpos += 48 + 30;
               ypos = 4;
          }

          p = p->next;
     }

     window_surface->SetClip( window_surface, &old_clip );

     /* unlock */
     manager->Unlock( manager );

     window_surface->Flip( window_surface, NULL, 0 );
}

void desktop_event_handler(DFBWindowEvent evt)
{
     int xpos = 4;
     int ypos = 4;

     /* lock */
     manager->Lock( manager );

     desktop_shortcut *p = desktop_shortcut_head;
     while (p)
     {
          DFBRectangle rect = { xpos, ypos, 48, 48 };

          if (evt.x >= rect.x && 
              evt.y >= rect.y &&
              evt.x < (rect.x+rect.w) && 
              evt.y < (rect.y+rect.h) )
          {
               run_task(p->task_path);
               break;
          }

          ypos += 48 + 30;
          if ((ypos+48+30) > (desktop_height - taskbar_height))
          {
               xpos += 48 + 30;
               ypos = 4;
          }

          p = p->next;
     }

     /* unlock */
     manager->Unlock( manager );
}

void desktop_refresh()
{
     desktop_shortcut *node = NULL;
     while (node = desktop_shortcut_head)
     {
          desktop_shortcut_head = desktop_shortcut_head->next;
          free(node);
     }

     desktop_shortcut_head  = NULL;
     desktop_shortcut_tail  = NULL;
     desktop_shortcut_count = 0;

     FILE *fp;
     char strLine[4096];
     if (NULL == (fp = fopen("desktop.cfg", "r")))
     {
          return;
     }

     while (!feof(fp))
     {
          if (fgets(strLine, 4096, fp))
          {
               if ('\0' != strLine[0] && ' ' != strLine[0] && 
                   '\n' != strLine[0] && '\r' != strLine[0])
               {
                    int len = strlen(strLine);
                    int i;
                    for (i = len-1; i >= 0; i--)
                    {
                         if ('\n' == strLine[i] ||
                             '\r' == strLine[i] ||
                             '\t' == strLine[i] ||
                             ' ' == strLine[i])
                              len--;
                         else
                              break;
                    }

                    if (len > 0)
                    {
                         strLine[ len ] = '\0';
                         create_desktop_shortcut(strLine);
                    }
               }
          }
     }

     fclose(fp);

     desktop_draw();
}


typedef struct _window_task window_task;
struct _window_task
{
     IDirectFBSurface *surface;

     SaWManWindowHandle handle;
     SaWManProcess process_info;

     bool   enableclose;
     time_t closetime;

     window_task *next;
};

window_task            *task_head       = NULL;
window_task            *task_tail       = NULL;
int			task_count	= 0;

void add_window_task(SaWManWindowHandle handle)
{
     window_task *node = malloc(sizeof(window_task));
          
     node->surface = NULL;
     node->handle = handle;
     node->enableclose = false;
     node->next = NULL;

     /* lock */
     manager->Lock( manager );

     if (!task_head)
     {
          task_head = node;
          task_tail = node;
     }
     else
     {
          task_tail->next = node;
          task_tail = node;
     }
     task_count++;

     /* get process info. */
     manager->GetProcessInfo( manager, handle, &node->process_info );

     /* unlock */
     manager->Unlock( manager );

     char link[128];
     char path[4096];
     sprintf( link, "/proc/%d/exe", node->process_info.pid );
     int i = readlink( link, path, sizeof(path) );
     path[i] = '\0';

     node->surface = get_icon(path);
}

void del_window_task(SaWManWindowHandle handle)
{
     /* lock */
     manager->Lock( manager );

     window_task* prev = NULL;
     window_task* p = task_head;
     while (p)
     {
          if (handle == p->handle)
          {
               if (prev)
                    prev->next = p->next;
               else
                    task_head = p->next;

               if (!p->next)
                    task_tail = prev;

               task_count--;

               release_icon(p->surface);
               p->next = NULL;
               free(p);

               break;
          }

          prev = p;
          p = p->next;
     }

     /* unlock */
     manager->Unlock( manager );
}

window_task* get_window_task(SaWManWindowHandle handle)
{
     /* lock */
     manager->Lock( manager );

     window_task* p = task_head;
     while (p)
     {
          if (handle == p->handle)
          {
               break;
          }

          p = p->next;
     }

     /* unlock */
     manager->Unlock( manager );

     return p;
}

void taskbar_window_draw(SaWManWindowReconfig *reconfig)
{
     if (task_count <= 0)
     {
          /* lock */
          manager->Lock( manager );

          taskbar_surface->SetDrawingFlags( taskbar_surface, DSDRAW_NOFX );
          taskbar_surface->SetColor( taskbar_surface, 0, 128, 128, 0xff );
          taskbar_surface->FillRectangle( taskbar_surface, 0, 0, desktop_width - iconbar_width, taskbar_height );

          /* unlock */
          manager->Unlock( manager );

          taskbar_surface->Flip( taskbar_surface, NULL, 0 );
          return;
     }

     int xpos = 0;
     int width = ((desktop_width - iconbar_width) - (task_count-1) * 2) / task_count;
     if (width > 150)
          width = 150;

     /* lock */
     manager->Lock( manager );

     taskbar_surface->SetDrawingFlags( taskbar_surface, DSDRAW_NOFX );
     taskbar_surface->SetColor( taskbar_surface, 0, 128, 128, 0xff );
     taskbar_surface->FillRectangle( taskbar_surface, 0, 0, desktop_width - iconbar_width, taskbar_height );

     DFBRegion old_clip;
     taskbar_surface->GetClip( taskbar_surface, &old_clip );

     window_task *p = task_head;
     while (p)
     {
          DFBRegion clip = { xpos, 0, xpos+width-1, taskbar_height-1 };
          taskbar_surface->SetClip( taskbar_surface, &clip );

          taskbar_surface->SetDrawingFlags( taskbar_surface, DSDRAW_NOFX );
          taskbar_surface->SetColor( taskbar_surface, 255, 128, 0, 0xff );
          taskbar_surface->FillRectangle( taskbar_surface, xpos, 0, width, taskbar_height );

          if (p->surface)
          {
               taskbar_surface->SetBlittingFlags( taskbar_surface, DSBLIT_BLEND_ALPHACHANNEL );

               DFBRectangle rect = { xpos+4, 4, 32, 32 };
               taskbar_surface->StretchBlit( taskbar_surface, p->surface, NULL, &rect );
          }

          char window_name[256];
          if (reconfig && reconfig->handle == p->handle)
          {
               window_name[0] = '\0';
               int l = strlen(reconfig->request.name);
               int n = (sizeof(window_name) - 1) > l ? l : (sizeof(window_name) - 1);
               strncpy(window_name, reconfig->request.name, n);
               window_name[n] = '\0';
          }
          else
          {
               manager->GetWindowName( manager, p->handle, window_name, sizeof(window_name));
          }

          if ('\0' != window_name[0])
          {
               DFBBoolean hide;
               manager->GetHide( manager, p->handle, &hide );

               if (hide)
               {
                    taskbar_surface->SetColor( taskbar_surface, 0, 0, 0, 0xff );
               }
               else
               {
                    taskbar_surface->SetColor( taskbar_surface, 0xCF, 0xCF, 0xFF, 0xFF );
               }

               taskbar_surface->SetDrawingFlags( taskbar_surface, DSDRAW_SRC_PREMULTIPLY );
               taskbar_surface->DrawString( taskbar_surface, window_name, -1, xpos+40, 10, DSTF_LEFT | DSTF_TOP );
          }

          xpos += width + 2;
          p = p->next;
     }

     taskbar_surface->SetClip( taskbar_surface, &old_clip );

     /* unlock */
     manager->Unlock( manager );

     taskbar_surface->Flip( taskbar_surface, NULL, 0 );
}

void taskbar_window_event_handler(DFBWindowEvent evt)
{
     if (task_count <= 0)
          return;

     int xpos = 0;
     int width = ((desktop_width - iconbar_width) - (task_count-1) * 2) / task_count;
     if (width > 150)
          width = 150;

     /* lock */
     manager->Lock( manager );

     window_task *p = task_head;
     while (p)
     {
          DFBRectangle rect = { xpos, 0, width, taskbar_height };

          if (evt.x >= rect.x && 
              evt.y >= rect.y &&
              evt.x < (rect.x+rect.w) && 
              evt.y < (rect.y+rect.h) )
          {
               DFBBoolean hide;

               manager->GetHide( manager, p->handle, &hide );
               manager->SetHide( manager, p->handle, !hide );


               DFBRegion old_clip;
               taskbar_surface->GetClip( taskbar_surface, &old_clip );

               DFBRegion clip = { xpos, 0, xpos+width-1, taskbar_height-1 };
               taskbar_surface->SetClip( taskbar_surface, &clip );

               taskbar_surface->SetDrawingFlags( taskbar_surface, DSDRAW_NOFX );
               taskbar_surface->SetColor( taskbar_surface, 255, 128, 0, 0xff );
               taskbar_surface->FillRectangle( taskbar_surface, xpos, 0, width, taskbar_height );

               if (p->surface)
               {
                    taskbar_surface->SetBlittingFlags( taskbar_surface, DSBLIT_BLEND_ALPHACHANNEL );

                    DFBRectangle rect = { xpos+4, 4, 32, 32 };
                    taskbar_surface->StretchBlit( taskbar_surface, p->surface, NULL, &rect );
               }

               char window_name[256];
               manager->GetWindowName( manager, p->handle, window_name, sizeof(window_name));

               if ('\0' != window_name[0])
               {
                    if (!hide)
                    {
                         taskbar_surface->SetColor( taskbar_surface, 0, 0, 0, 0xff );
                    }
                    else
                    {
                         manager->RaiseToTop( manager, p->handle );

                         taskbar_surface->SetColor( taskbar_surface, 0xCF, 0xCF, 0xFF, 0xFF );
                    }

                    taskbar_surface->SetDrawingFlags( taskbar_surface, DSDRAW_SRC_PREMULTIPLY );
                    taskbar_surface->DrawString( taskbar_surface, window_name, -1, xpos+40, 10, DSTF_LEFT | DSTF_TOP );
               }

               taskbar_surface->SetClip( taskbar_surface, &old_clip );

               taskbar_surface->Flip( taskbar_surface, NULL, 0 );

               break;
          }

          xpos += width + 2;
          p = p->next;
     }

     /* unlock */
     manager->Unlock( manager );
}

void window_task_timer_handler()
{
     time_t nowtime;
     time(&nowtime);

     /* lock */
     manager->Lock( manager );

     window_task* prev = NULL;
     window_task* p = task_head;
     while (p)
     {
          if (p->enableclose && (nowtime - p->closetime) > 30)
          {
               if (prev)
                    prev->next = p->next;
               else
                    task_head = p->next;

               if (!p->next)
                    task_tail = prev;

               task_count--;

               release_icon(p->surface);
               p->next = NULL;
               free(p);

               /* kill process */
               char command[128];
               sprintf(command, "kill -9 %d", p->process_info.pid);
               system(command);
          }

          prev = p;
          p = p->next;
     }

     /* unlock */
     manager->Unlock( manager );
}


static DirectResult
start_request( void       *context,
               const char *name,
               pid_t      *ret_pid )
{
     D_INFO( "SaWMan/Sample1: Start request for '%s'!\n", name );

     return DFB_UNIMPLEMENTED;
}

static DirectResult
stop_request( void     *context,
              pid_t     pid,
              FusionID  caller )
{
     D_INFO( "SaWMan/Sample1: Stop request from Fusion ID 0x%lx for pid %d!\n", caller, pid );

     return DFB_OK;
}

static DirectResult
process_added( void          *context,
               SaWManProcess *process )
{
     D_INFO( "SaWMan/Sample1: Process added (%d) [%lu]!\n", process->pid, process->fusion_id );

     return DFB_OK;
}

static DirectResult
process_removed( void          *context,
                 SaWManProcess *process )
{
     D_INFO( "SaWMan/Sample1: Process removed (%d) [%lu]!\n", process->pid, process->fusion_id );

     return DFB_OK;
}

static DirectResult
input_filter( void          *context,
              DFBInputEvent *event )
{
     if (desktop_init)
     {
          manager->Lock( manager );

          switch (event->type) 
          {
               case DIET_KEYPRESS:
               {
                    switch (event->key_symbol) 
                    {
                         case DIKS_CONTROL:
                              press_ctrl = 1;
                              break;
                         case DIKS_ALT:
                         case DIKS_ALTGR:
                              press_alt = 1;
                              break;
                         case DIKS_SMALL_T:
                         case DIKS_CAPITAL_T:
                              if( 1==press_ctrl && 1==press_alt )
                              {
                                   manager->Unlock( manager );
                                   run_term();
                                   return DFB_BUSY;
                              }
                              break;

                         default:
                              break;
                    }

                    break;
               }

               case DIET_KEYRELEASE:
               {
                    switch (event->key_symbol) 
                    {
                         case DIKS_CONTROL:
                              press_ctrl = 0;
                              break;
                         case DIKS_ALT:
                         case DIKS_ALTGR:
                              press_alt = 0;
                              break;
                         case DIKS_SMALL_T:
                         case DIKS_CAPITAL_T:
                              if( 1==press_ctrl && 1==press_alt )
                              {
                                   manager->Unlock( manager );
                                   return DFB_BUSY;
                              }
                              break;

                         default:
                              break;
                    }

                    break;
               }

               default:
                    break;
          }

          manager->Unlock( manager );
     }

     return DFB_NOIMPL;
}

static DirectResult
window_added( void               *context,
              SaWManWindowInfo   *info )
{
//     D_INFO( "SaWMan/Sample1: Window added (%d,%d-%d,%d)!\n", DFB_RECTANGLE_VALS(&info->config.bounds) );

     if (desktop_init)
     {
          add_window_task(info->handle);
          taskbar_window_draw(NULL);
     }

     return DFB_NOIMPL;
}

static DirectResult
window_removed( void               *context,
                SaWManWindowInfo   *info )
{
//     D_INFO( "SaWMan/Sample1: Window removed (%d,%d-%d,%d)!\n", DFB_RECTANGLE_VALS(&info->config.bounds) );

     if (desktop_init)
     {
          /* remove timer. */
          window_task* p = get_window_task(info->handle);
          p->enableclose = false;

          del_window_task(info->handle);
          taskbar_window_draw(NULL);
     }

     return DFB_OK;
}

static DirectResult
window_reconfig( void                   *context,
                 SaWManWindowReconfig   *reconfig )
{
//     D_INFO( "SaWMan/Sample1: Window reconfig!\n" );

     SaWManWindowConfig *current;
     SaWManWindowConfig *request;

     current = &reconfig->current;
     request = &reconfig->request;

     SaWManWindowHandle handle = reconfig->handle;

     if (desktop_init)
     {
          if (reconfig->flags & SWMCF_NAME) 
          {
               taskbar_window_draw(reconfig);
          }
     }

     return DFB_NOIMPL;
}

static DirectResult 
process_close( void               *context,
               SaWManWindowInfo   *info )
{
     manager->Lock( manager );

     manager->CloseWindow( manager, info->handle );

     /* setup timer. */
     window_task* p = get_window_task(info->handle);
     time(&p->closetime);
     p->enableclose = true;

     manager->Unlock( manager );

     return DFB_OK;
}

static DirectResult 
process_min( void               *context,
             SaWManWindowInfo   *info )
{
     manager->Lock( manager );

     manager->SetHide( manager, info->handle, true );

     manager->Unlock( manager );

     /* update taskbar. */
     taskbar_window_draw(NULL);

     return DFB_OK;
}

static DirectResult 
process_max( void               *context,
             SaWManWindowInfo   *info )
{
     manager->Lock( manager );
 
     DFBBoolean max;
     manager->GetMaximization( manager, info->handle, &max );     
     manager->SetMaximization( manager, info->handle, max );

     manager->Unlock( manager );

     return DFB_OK;
}

static const SaWManCallbacks callbacks = {
     Start:          start_request,
     Stop:           stop_request,
     ProcessAdded:   process_added,
     ProcessRemoved: process_removed,
     InputFilter:    input_filter,
     WindowAdded:    window_added,
     WindowRemoved:  window_removed,
     WindowReconfig: window_reconfig,
     WindowClose:    process_close,
     WindowMin:      process_min,
     WindowMax:      process_max,
};

int main (int argc, char *argv[])
{
     DFBResult               ret;
     DFBRectangle            rect;
     DFBFontDescription      desc;
     DFBDisplayLayerConfig   layer_config;
     IDirectFBEventBuffer   *desktop_event;
     IDirectFBEventBuffer   *taskbar_event;
     IDirectFBFont          *font;
     IDirectFBImageProvider *provider;
     IDirectFBSurface       *bgsurface;
     IDirectFBSurface       *cursurface;

     int fontheight;
     int rotation;
     int err;
     int quit = 0;

     ret = DirectFBInit (&argc, &argv);
     if (ret) {
          DirectFBError ("DirectFBInit", ret);
          return -1;
     }

     ret = DirectFBCreate (&dfb);
     if (ret) {
          DirectFBError ("DirectFBCreate", ret);
          return -2;
     }

     ret = dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &layer);
     if (ret) {
          dfb->Release (dfb);
          return -3;
     }

     layer->GetConfiguration( layer, &layer_config );
#ifdef DIRECTFB_ROTATE
     layer->GetRotation( layer, &rotation );
#endif

     desktop_width = layer_config.width;
     desktop_height = layer_config.height;


     /* Load background and cursor image. */
     {
          DFBSurfaceDescription desc;

          // load background
          DFBCHECK(dfb->CreateImageProvider( dfb,
                                             "desktop.png",
                                             &provider ));

          desc.flags  = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_CAPS;
          desc.width  = layer_config.width;
          desc.height = layer_config.height;
          desc.caps   = DSCAPS_SHARED;

#ifdef DIRECTFB_ROTATE
          if (rotation == 90 || rotation == 270)
               D_UTIL_SWAP( desc.width, desc.height );
#endif

          DFBCHECK(dfb->CreateSurface( dfb, &desc, &bgsurface ) );

          provider->RenderTo( provider, bgsurface, NULL );
          provider->Release( provider );

          // load cursor
          load_image("cursor.png", 0, &cursurface, NULL, NULL, NULL);
     }


     /* Set background and cursor. */
     layer->SetCooperativeLevel(layer,DLSCL_ADMINISTRATIVE);

     DFBCHECK(layer->SetCursorShape(layer,cursurface,0,0));  
     layer->EnableCursor ( layer, 1 );

     //layer->SetBackgroundColor(layer, 0x0, 0x0, 0xff, 0xff);
     //layer->SetBackgroundMode(layer,DLBM_COLOR);
     layer->SetBackgroundImage( layer, bgsurface );
     layer->SetBackgroundMode( layer, DLBM_IMAGE );

     layer->SetCooperativeLevel(layer,DLSCL_SHARED);


     /* Load font */
     desc.flags  = DFDESC_HEIGHT;
     desc.height = 16;

     ret = dfb->CreateFont (dfb, "Misc-Fixed.pfa", &desc, &font);
     if (ret) {
          dfb->Release (dfb);
          return -4;
     }
     font->GetHeight( font, &fontheight );


     /* Create window */
     {
          DFBWindowDescription  desc;

          desc.flags = ( DWDESC_POSX | DWDESC_POSY |
                         DWDESC_WIDTH | DWDESC_HEIGHT | 
                         DWDESC_STACKING );

          desc.caps = DWCAPS_ALPHACHANNEL;
          desc.surface_caps = DSCAPS_PREMULTIPLIED;
          desc.flags |= DWDESC_CAPS | DWDESC_SURFACE_CAPS;

          desc.posx   = 0;
          desc.posy   = 0;
          desc.width  = layer_config.width;
          desc.height = layer_config.height;

          desc.stacking = DWSC_LOWER;

          DFBCHECK( layer->CreateWindow( layer, &desc, &window ) );
          window->GetSurface( window, &window_surface );

          DFBCHECK(window->CreateEventBuffer( window, &desktop_event ));

          DFBCHECK(window->SetCursorShape( window, cursurface, 0, 0 ));

          DFBCHECK(window_surface->SetFont( window_surface, font ));

          window->SetOpacity( window, 0xFF );
     }

     /* Create window */
     {
          DFBWindowDescription  desc;

          desc.flags = ( DWDESC_POSX | DWDESC_POSY |
                         DWDESC_WIDTH | DWDESC_HEIGHT | 
                         DWDESC_STACKING );

          desc.caps = DWCAPS_ALPHACHANNEL;
          desc.surface_caps = DSCAPS_PREMULTIPLIED;
          desc.flags |= DWDESC_CAPS | DWDESC_SURFACE_CAPS;

          desc.posx   = 0;
          desc.posy   = layer_config.height - taskbar_height;
          desc.width  = layer_config.width;
          desc.height = taskbar_height;

          desc.stacking = DWSC_UPPER;

          DFBCHECK( layer->CreateWindow( layer, &desc, &taskbar ) );
          taskbar->GetSurface( taskbar, &taskbar_surface );

          DFBCHECK(taskbar->CreateEventBuffer( taskbar, &taskbar_event ));

          DFBCHECK(taskbar->SetCursorShape( taskbar, cursurface, 0, 0 ));

          DFBCHECK( taskbar_surface->SetFont( taskbar_surface, font ) );

          taskbar->SetOpacity( taskbar, 0xFF );
     }

     window->RequestFocus( window );

     DFBCHECK( SaWManCreate( &saw ) );
     DFBCHECK( saw->CreateManager( saw, &callbacks, NULL, &manager ) );
     desktop_init = true;


     load_image("default.png", 0, &icon_def.surface, NULL, NULL, NULL);


     IDirectFBSurface *close_system = NULL;
     DFBRectangle close_system_rect = { desktop_width-20, 2, 16, 16 };
     load_image("close-system.png", 0, &close_system, NULL, NULL, NULL);

     IDirectFBSurface *refresh = NULL;
     DFBRectangle refresh_rect = { desktop_width-40, 2, 16, 16 };
     load_image("refresh.png", 0, &refresh, NULL, NULL, NULL);


     // display desktop
     desktop_refresh();

     // display taskbar
     DFBCHECK( taskbar_surface->Clear( taskbar_surface, 0, 128, 128, 0xff ) );
     DFBCHECK( taskbar_surface->Flip( taskbar_surface, NULL, 0 ) );


     while (1) 
     {
          window_task_timer_handler();

          DFBWindowEvent desktop_evt;
          while (desktop_event->GetEvent( desktop_event, DFB_EVENT(&desktop_evt) ) == DFB_OK) 
          {
               switch (desktop_evt.type) 
               {
                    case DWET_BUTTONDOWN:
                    {
                         switch (desktop_evt.button) 
                         {
                              case DIBI_LEFT:
                              {
                                   desktop_event_handler(desktop_evt);

                                   break;
                              }

                              default:
                                   break;
                         }

                         break;
                    }

                    case DWET_BUTTONUP:
                    {
                         break;
                    }

                    case DWET_MOTION:
                    case DWET_ENTER:
                    case DWET_LEAVE:
                    {
                         break;
                    }

                    case DWET_KEYDOWN:
                    {
                         break;
                    }

                    case DWET_KEYUP:
                    {
                         break;
                    }

                    default:
                         break;
               }    
          }

          DFBWindowEvent taskbar_evt;
          while (taskbar_event->GetEvent( taskbar_event, DFB_EVENT(&taskbar_evt) ) == DFB_OK) 
          {
               switch (taskbar_evt.type) 
               {
                    case DWET_BUTTONDOWN:
                    {
                         switch (taskbar_evt.button) 
                         {
                              case DIBI_LEFT:
                              {
                                   if (taskbar_evt.x >= refresh_rect.x && 
                                       taskbar_evt.y >= refresh_rect.y &&
                                       taskbar_evt.x < (refresh_rect.x+refresh_rect.w) && 
                                       taskbar_evt.y < (refresh_rect.y+refresh_rect.h) )
                                   {
                                        desktop_refresh();
                                   }
                                   else if (taskbar_evt.x >= close_system_rect.x && 
                                       taskbar_evt.y >= close_system_rect.y &&
                                       taskbar_evt.x < (close_system_rect.x+close_system_rect.w) && 
                                       taskbar_evt.y < (close_system_rect.y+close_system_rect.h) )
                                   {
                                        system("poweroff");
                                   }
                                   else
                                   {
                                        taskbar_window_event_handler(taskbar_evt);
                                   }

                                   break;
                              }

                              default:
                                   break;
                         }

                         break;
                    }

                    case DWET_BUTTONUP:
                    {
                         break;
                    }

                    case DWET_MOTION:
                    case DWET_ENTER:
                    case DWET_LEAVE:
                    {
                         break;
                    }

                    case DWET_KEYDOWN:
                    {
                         break;
                    }

                    case DWET_KEYUP:
                    {
                         break;
                    }

                    default:
                         break;
               }    
          }


          time_t now;
          struct tm *timenow;
          time(&now);
          timenow = localtime(&now);

          /* lock */
          manager->Lock( manager );

          DFBRectangle rect = { 0, 0, iconbar_width, taskbar_height };
          //font->GetStringExtents( font, asctime(timenow), -1, &rect, NULL );

          taskbar_surface->SetDrawingFlags( taskbar_surface, DSDRAW_NOFX );
          taskbar_surface->SetColor( taskbar_surface, 0, 128, 128, 0xff );
          taskbar_surface->FillRectangle( taskbar_surface, desktop_width-rect.w, 0, rect.w, taskbar_height );

          taskbar_surface->SetDrawingFlags( taskbar_surface, DSDRAW_SRC_PREMULTIPLY );
          taskbar_surface->SetColor( taskbar_surface, 0xCF, 0xCF, 0xFF, 0xFF );
          taskbar_surface->DrawString( taskbar_surface,
                                 asctime(timenow),
                                 -1, desktop_width-rect.w, 18, DSTF_LEFT | DSTF_TOP );


          taskbar_surface->SetDrawingFlags( taskbar_surface, DSDRAW_NOFX );
          taskbar_surface->SetBlittingFlags( taskbar_surface, DSBLIT_BLEND_ALPHACHANNEL );
          taskbar_surface->StretchBlit( taskbar_surface, close_system, NULL, &close_system_rect );
          taskbar_surface->StretchBlit( taskbar_surface, refresh, NULL, &refresh_rect );

          /* unlock */
          manager->Unlock( manager );

          taskbar_surface->Flip( taskbar_surface, NULL, 0 );
     }

     taskbar_event->Release( taskbar_event );
     desktop_event->Release( desktop_event );

     font->Release( font );
     window_surface->Release( window_surface );
     window->Release( window );
     layer->Release( layer );
//     bgsurface->Release( bgsurface );

     if (manager)
          manager->Release( manager );

     if (saw)
          saw->Release( saw );

     dfb->Release (dfb);

     return 0;
}

static DFBResult 
_util_load_image(const char             *filename,
                 DFBSurfacePixelFormat   pixelformat,
                 IDirectFBSurface      **surface,
                 int                    *ret_width,
                 int                    *ret_height,
                 DFBImageDescription    *desc,
                 bool                    premultiply)
{
     DFBResult               ret;
     DFBSurfaceDescription   dsc;
     IDirectFBSurface       *image = NULL;
     IDirectFBImageProvider *provider = NULL;

     if(NULL == filename)
          return DFB_INVARG;
     if(NULL == surface)
          return DFB_INVARG;

     *surface = NULL;

     /* Create an image provider for loading the file */
     ret = dfb->CreateImageProvider(dfb, filename, &provider);
     if (ret) {
          D_DERROR(ret,
                   "CreateImageProvider for '%s' failed\n",
                   filename);
          return ret;
     }

     /* Retrieve a surface description for the image */
     ret = provider->GetSurfaceDescription(provider, &dsc);
     if (ret) {
          D_DEBUG_AT(LiteUtilDomain,
                   "GetSurfaceDescription for '%s': %s\n",
                   filename, DirectFBErrorString(ret));
          provider->Release(provider);
          return ret;
     }

     /* Use the specified pixelformat if the image's pixelformat is not ARGB */
     if (pixelformat && (!(dsc.flags & DSDESC_PIXELFORMAT) || dsc.pixelformat != DSPF_ARGB)) {
          dsc.flags      |= DSDESC_PIXELFORMAT;
          dsc.pixelformat = pixelformat;
     }
    
     if (premultiply && DFB_PIXELFORMAT_HAS_ALPHA(dsc.pixelformat)) {
          dsc.flags |= DSDESC_CAPS;
          dsc.caps   = DSCAPS_PREMULTIPLIED;
     }

     /* Create a surface using the description */
     ret = dfb->CreateSurface(dfb, &dsc, &image);
     if (ret) {
          D_DERROR(ret,
                   "load_image: CreateSurface %dx%d failed\n",
                   dsc.width, dsc.height);
          provider->Release(provider);
          return ret;
     }

     /* Render the image to the created surface */
     ret = provider->RenderTo(provider, image, NULL);
     if (ret) {
          D_DEBUG_AT(LiteUtilDomain,
                   "load_image: RenderTo for '%s': %s\n",
                   filename, DirectFBErrorString(ret));
          image->Release(image);
          provider->Release(provider);
          return ret;
     }

     /* Return surface */
     *surface = image;

     /* Return width? */
     if (ret_width)
          *ret_width = dsc.width;

     /* Return height? */
     if (ret_height)
          *ret_height  = dsc.height;

     /* Retrieve the image description? */
     if (desc)
          provider->GetImageDescription(provider, desc);

     /* Release the provider */
     provider->Release(provider);

     return DFB_OK;
}

DFBResult load_image(const char             *filename,
                     DFBSurfacePixelFormat   pixelformat,
                     IDirectFBSurface      **surface,
                     int                    *ret_width,
                     int                    *ret_height,
                     DFBImageDescription    *desc)
{
     return _util_load_image( filename, pixelformat, surface, ret_width, ret_height, desc, false );
}

int run_task(char* path)
{
     int id;
     id = fork();
     if(0 == id)
     {
          char* name = strrchr(path, '/') + 1;

          int len = name - path;
          char* dir = malloc(len + 1);
          strncpy(dir, path, len);
          dir[len] = '\0';

          chdir(dir);

          execl(path, name, NULL);

          perror("Could not exec");
          _exit(127);
     }
     if(-1 == id)
     {
          fprintf(stderr, "fork failed.\n");
          return -6;
     }

     return 0;
}

int run_term()
{
     int id;
     id = fork();
     if(0 == id)
     {
          chdir("../dfb-term/bin/");

          execl("dfbterm", "dfbterm", NULL);

          perror("Could not exec");
          _exit(127);
     }
     if(-1 == id)
     {
          fprintf(stderr, "fork failed.\n");
          return -6;
     }

     return 0;
}

