#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include "directfb.h"
#include "directfb_version.h"

#if 0
#define DP(...) printf(__VA_ARGS__);
#else
#define DP(...)
#endif

#define BCM_OLD_FONT_API  ((DIRECTFB_MAJOR_VERSION * 1000000 + \
                            DIRECTFB_MINOR_VERSION * 1000 + \
                            DIRECTFB_MICRO_VERSION) < 1007000)

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...)                                                     \
               err = x;                                                    \
               if (err != DFB_OK) {                                        \
                    fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
                    DirectFBErrorFatal( #x, err );                                                 \
                                }

#define MAX_DEPTH           20
#define TEST_ITERATIONS     10
#define LAYER_ITERATIONS    5
#define WINDOW_ITERATIONS   2000
#define CLOSER_TEXT_DEPTH   10

int main(int argc, char *argv[])
{
    IDirectFB               *pDfb = NULL;
    IDirectFBDisplayLayer   *pPrimary = NULL;
    int                     iScreenWidth, iScreenHeight, iWindowWidth, iWindowHeight;
    DFBResult               err;
    IDirectFBScreen         *pScreen;
    DFBDisplayLayerConfig   sLayerCfg;
    char                    str[32];
    IDirectFBSurface        *pImageSurface;
    IDirectFBSurface        *pLayerSurface = NULL;
    IDirectFBImageProvider  *pImageProvider = NULL;
    DFBRectangle            sImgRect, sWindowRect;
    DFBSurfaceDescription   sSurfaceDesc;
    DFBScreenEncoderConfig  sEncoderCfg;
    int                     i, j, k, x, y, z, *dx, *dy, *dz, dx0, dx1, dx2, dy0, dy1, dy2, dz0, dz1, dz2;
    IDirectFBFont           *pFont;
    DFBFontDescription      sFontDesc;
    DFBWindowDescription    sWindowDesc;
    IDirectFBWindow         *pWindow0, *pWindow1, *pWindow2, *pWindow;
    IDirectFBSurface        *pWindowSurface0, *pWindowSurface1, *pWindowSurface2;
    int                     x0, y0, z0, x1, y1, z1, x2, y2, z2;

    DirectFBInit( &argc, &argv );

    /* Set DirectFB system driver & Fusion options */
    DirectFBSetOption("system", "bcmnexus");
    DirectFBSetOption("madv-remove", NULL);

    /* Set DirectFB layer configuration */
    sprintf(str, "%d", DLID_PRIMARY);
    DirectFBSetOption("init-layer", str);
    DirectFBSetOption("layer-stacking", "upper");
    DirectFBSetOption("layer-stacking", "middle");

    /* Set DirectFB window manager to SawMan */
    DirectFBSetOption("wm", "sawman");

    /* create the super interface */
    DirectFBCreate( &pDfb );

    /* Wait for TV to sync and stuff. */
    sleep(3);

    /* Setup screen. */
    DFBCHECK(pDfb->GetDisplayLayer(pDfb, DLID_PRIMARY, &pPrimary));
    DFBCHECK(pPrimary->GetScreen(pPrimary, &pScreen));

    sEncoderCfg.flags   = (DFBScreenEncoderConfigFlags)(DSECONF_TV_STANDARD | DSECONF_SCANMODE | DSECONF_FREQUENCY |
                                                        DSECONF_CONNECTORS | DSECONF_RESOLUTION | DSECONF_FRAMING);
    sEncoderCfg.tv_standard = DSETV_DIGITAL;
    sEncoderCfg.out_connectors = (DFBScreenOutputConnectors)(DSOC_COMPONENT | DSOC_HDMI);
    sEncoderCfg.scanmode = DSESM_PROGRESSIVE;
    sEncoderCfg.frequency = DSEF_60HZ;
    sEncoderCfg.framing = DSEPF_STEREO_FRAME_PACKING;
    sEncoderCfg.framing = DSEPF_STEREO_SIDE_BY_SIDE_HALF;
    sEncoderCfg.resolution = DSOR_1280_720;

    DFBCHECK(pScreen->SetEncoderConfiguration(pScreen, 0, &sEncoderCfg));

    DFBCHECK(pScreen->GetSize(pScreen, &iScreenWidth, &iScreenHeight));
    iWindowWidth = iScreenWidth / 5;
    iWindowHeight = iScreenHeight / 5;

    /* Get fonts. */
    sFontDesc.flags = (DFBFontDescriptionFlags)(DFDESC_HEIGHT | DFDESC_WIDTH | DFDESC_ATTRIBUTES);
#if 0
#if BCM_OLD_FONT_API
    sFontDesc.attributes = (DFBFontAttributes)(DFFA_BOLD | DFFA_ITALIC);
#else
    sFontDesc.attributes = (DFBFontAttributes)(DFFA_STYLE_BOLD | DFFA_STYLE_ITALIC);
#endif
#endif
    sFontDesc.height = 30;
    sFontDesc.width = 30;
    DFBCHECK(pDfb->CreateFont(pDfb, FONT, &sFontDesc, &pFont));

    x0 = iScreenWidth / 3;
    y0 = iScreenHeight - iWindowHeight - 50;
    z0 = 0;
    x1 = iScreenWidth / 9;
    y1 = 50;
    z1 = 5;
    x2 = iScreenWidth - iWindowWidth - 50;
    y2 = iScreenHeight / 7;
    z2 = 20;
    dx0 = 1;
    dy0 = -1;
    dz0 = 0;
    dx1 = 1;
    dy1 = 1;
    dz1 = 1;
    dx2 = -1;
    dy2 = 1;
    dz2 = -1;
    for (k = 0; k < TEST_ITERATIONS; k++)
    {
        /*
         * L/R Mono Layer
         */

        DP("L / R Mono test\n");

        /* Setup primary layer as LR mono. */
        DFBCHECK(pPrimary->SetCooperativeLevel(pPrimary, DLSCL_ADMINISTRATIVE));
        sLayerCfg.flags  = (DFBDisplayLayerConfigFlags)(DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT |
                                                        DLCONF_SURFACE_CAPS | DLCONF_OPTIONS);
        sLayerCfg.width  = iScreenWidth;
        sLayerCfg.height = iScreenHeight;
        sLayerCfg.pixelformat = DSPF_ARGB;
        sLayerCfg.options = (DFBDisplayLayerOptions)(DLOP_LR_MONO | DLOP_ALPHACHANNEL);
        sLayerCfg.surface_caps = (DFBSurfaceCapabilities)(DSCAPS_PREMULTIPLIED | DSCAPS_VIDEOONLY);
        DFBCHECK(pPrimary->SetConfiguration(pPrimary, &sLayerCfg));

        DFBCHECK(pPrimary->SetBackgroundColor(pPrimary, 0, 0, 0, 0 ));
        DFBCHECK(pPrimary->SetBackgroundMode(pPrimary, DLBM_COLOR));
        DFBCHECK(pPrimary->EnableCursor(pPrimary, false));

        /* Draw primary mono image. */
        DFBCHECK(pPrimary->GetSurface(pPrimary, &pLayerSurface));
        DFBCHECK(pLayerSurface->Clear(pLayerSurface, 0, 0, 0, 0));

        DFBCHECK(pDfb->CreateImageProvider(pDfb, DATADIR"/swirl.png", &pImageProvider));
        DFBCHECK(pImageProvider->GetSurfaceDescription(pImageProvider, &sSurfaceDesc));
        DFBCHECK(pDfb->CreateSurface(pDfb, &sSurfaceDesc, &pImageSurface));

        DFBCHECK(pImageProvider->RenderTo(pImageProvider, pImageSurface, NULL));

        sImgRect.x = sImgRect.y = 0; sImgRect.w = sSurfaceDesc.width; sImgRect.h = sSurfaceDesc.height;

        DFBCHECK(pLayerSurface->Blit(pLayerSurface, pImageSurface, &sImgRect, (iScreenWidth-sImgRect.w)/2,
            (iScreenHeight-sImgRect.h)/2));

        DFBCHECK(pLayerSurface->SetFont(pLayerSurface, pFont));
        DFBCHECK(pLayerSurface->SetColor(pLayerSurface, 0xff, 0xff, 0xff, 0xff));
        DFBCHECK(pLayerSurface->DrawString(pLayerSurface, "Primary Layer", 13, iScreenWidth/2,
            (iScreenHeight-sImgRect.h)/2+10, DSTF_TOPCENTER));
        DFBCHECK(pLayerSurface->DrawString(pLayerSurface, "L/R Mono", 8,
            iScreenWidth/2, iScreenHeight/2, DSTF_CENTER));
        DFBCHECK(pLayerSurface->Flip(pLayerSurface, NULL, DSFLIP_WAITFORSYNC));

        DP("L / R Mono start\n");

        /* Change depth perspective. */
        for (j = 0; j < LAYER_ITERATIONS; j++)
        {
            DP("L / R Mono iter %d\n",j);

            for (i = 0; i <= MAX_DEPTH; i++)
            {
                DFBCHECK(pPrimary->SetStereoDepth(pPrimary, false, i));
                DP(".");
                fflush(stdout);
            }
            DP("\n");
            for (i = MAX_DEPTH; i >= 0; i--)
            {
                DFBCHECK(pPrimary->SetStereoDepth(pPrimary, false, i));
                DP("+");
                fflush(stdout);
            }
            DP("\n");
        }

        DFBCHECK(pImageProvider->Release(pImageProvider));
        DFBCHECK(pLayerSurface->Clear(pLayerSurface, 0, 0, 0, 0));
        DFBCHECK(pLayerSurface->Flip(pLayerSurface, NULL, DSFLIP_WAITFORSYNC));

        /*
         * Stereo Layer
         */

        DP("Stereo Layer test\n");

        /* Setup primary layer as stereo. */
        sLayerCfg.flags  = (DFBDisplayLayerConfigFlags)(DLCONF_SURFACE_CAPS | DLCONF_OPTIONS);
        sLayerCfg.options = (DFBDisplayLayerOptions)(DLOP_STEREO | DLOP_ALPHACHANNEL);
        sLayerCfg.surface_caps = (DFBSurfaceCapabilities)(DSCAPS_PREMULTIPLIED | DSCAPS_VIDEOONLY | DSCAPS_STEREO);
        DFBCHECK(pPrimary->SetConfiguration(pPrimary, &sLayerCfg));

        /* Draw primary stereo images. */
        DFBCHECK(pLayerSurface->SetStereoEye(pLayerSurface, DSSE_LEFT));
        DFBCHECK(pLayerSurface->Clear(pLayerSurface, 0, 0, 0, 0));
        DFBCHECK(pLayerSurface->Blit(pLayerSurface, pImageSurface, &sImgRect, (iScreenWidth-sImgRect.w)/2,
            (iScreenHeight-sImgRect.h)/2));
        DFBCHECK(pLayerSurface->DrawString(pLayerSurface, "Primary Layer", 13, iScreenWidth/2 + CLOSER_TEXT_DEPTH,
            (iScreenHeight-sImgRect.h)/2+10, DSTF_TOPCENTER));
        DFBCHECK(pLayerSurface->DrawString(pLayerSurface, "Stereo Left", 11,
            iScreenWidth/2, iScreenHeight/2, DSTF_CENTER));
        DFBCHECK(pLayerSurface->SetStereoEye(pLayerSurface, DSSE_RIGHT));
        DFBCHECK(pLayerSurface->Clear(pLayerSurface, 0, 0, 0, 0));
        DFBCHECK(pLayerSurface->Blit(pLayerSurface, pImageSurface, &sImgRect, (iScreenWidth-sImgRect.w)/2,
            (iScreenHeight-sImgRect.h)/2));
        DFBCHECK(pLayerSurface->DrawString(pLayerSurface, "Primary Layer", 13, iScreenWidth/2 - CLOSER_TEXT_DEPTH,
            (iScreenHeight-sImgRect.h)/2+10, DSTF_TOPCENTER));
        DFBCHECK(pLayerSurface->DrawString(pLayerSurface, "Stereo Right", 12,
            iScreenWidth/2, iScreenHeight/2, DSTF_CENTER));
        DFBCHECK(pLayerSurface->FlipStereo(pLayerSurface, NULL, NULL, DSFLIP_WAITFORSYNC));

        DP("Stereo Layer start\n");

        /* Change depth perspective via offset. */
        for (j = 0; j < LAYER_ITERATIONS; j++)
        {

            DP("Stereo iter %d\n",j);

            for (i = 0; i <= MAX_DEPTH; i++)
            {
                DFBCHECK(pPrimary->SetStereoDepth(pPrimary, false, i));
                DP(".");
                fflush(stdout);
            }
            DP("\n");
            for (i = MAX_DEPTH; i >= 0; i--)
            {
                DFBCHECK(pPrimary->SetStereoDepth(pPrimary, false, i));
                DP("+");
                fflush(stdout);
            }
            DP("\n");
        }

        DFBCHECK(pLayerSurface->SetStereoEye(pLayerSurface, DSSE_LEFT));
        DFBCHECK(pLayerSurface->Clear(pLayerSurface, 0, 0, 0, 0));
        DFBCHECK(pLayerSurface->SetStereoEye(pLayerSurface, DSSE_RIGHT));
        DFBCHECK(pLayerSurface->Clear(pLayerSurface, 0, 0, 0, 0));
        DFBCHECK(pLayerSurface->FlipStereo(pLayerSurface, NULL, NULL, DSFLIP_WAITFORSYNC));


        DP("Windowing test\n");

        /*
         * Mono Window
         */

        /* Create mono window */
        sWindowDesc.flags = (DFBWindowDescriptionFlags)(DWDESC_CAPS | DWDESC_WIDTH | DWDESC_HEIGHT |
                                                        DWDESC_PIXELFORMAT | DWDESC_POSX | DWDESC_POSY |
                                                        DWDESC_SURFACE_CAPS | DWDESC_OPTIONS);
        sWindowDesc.caps = (DFBWindowCapabilities)(DWCAPS_ALPHACHANNEL);
        sWindowDesc.width = iWindowWidth;
        sWindowDesc.height = iWindowHeight;
        sWindowDesc.posx = x0;
        sWindowDesc.posy = y0;
        sWindowDesc.pixelformat = DSPF_ARGB;
        sWindowDesc.options = (DFBWindowOptions)(DWOP_ALPHACHANNEL | DWOP_SCALE);
        sWindowDesc.surface_caps = (DFBSurfaceCapabilities)(DSCAPS_PREMULTIPLIED | DSCAPS_VIDEOONLY);
        DFBCHECK(pPrimary->CreateWindow(pPrimary, &sWindowDesc, &pWindow0));

        /* Draw window mono image. */
        DFBCHECK(pWindow0->GetSurface(pWindow0, &pWindowSurface0));
        DFBCHECK(pWindowSurface0->Clear(pWindowSurface0, 0, 0, 0, 0));
        sWindowRect.x = sWindowRect.y = 0; sWindowRect.w = sWindowDesc.width; sWindowRect.h = sWindowDesc.height;
        DFBCHECK(pWindowSurface0->StretchBlit(pWindowSurface0, pImageSurface, &sImgRect, &sWindowRect));
        DFBCHECK(pWindowSurface0->SetFont(pWindowSurface0, pFont));
        DFBCHECK(pWindowSurface0->SetColor(pWindowSurface0, 0xff, 0xff, 0xff, 0xff));
        DFBCHECK(pWindowSurface0->DrawString(pWindowSurface0, "Mono", 4, sWindowRect.w/2,
            sWindowRect.h/2-25, DSTF_CENTER));
        DFBCHECK(pWindowSurface0->DrawString(pWindowSurface0, "Window", 6, sWindowRect.w/2,
            sWindowRect.h/2, DSTF_CENTER));
        DFBCHECK(pWindowSurface0->Flip(pWindowSurface0, NULL, DSFLIP_WAITFORSYNC));
        DFBCHECK(pWindow0->SetOpacity(pWindow0, 0xff));

        /*
         * L/R Mono Window
         */

        /* Create L/R mono window */
        sWindowDesc.flags = (DFBWindowDescriptionFlags)(DWDESC_CAPS | DWDESC_WIDTH | DWDESC_HEIGHT |
                                                        DWDESC_PIXELFORMAT | DWDESC_POSX | DWDESC_POSY |
                                                        DWDESC_SURFACE_CAPS | DWDESC_OPTIONS);
        sWindowDesc.caps = (DFBWindowCapabilities)(DWCAPS_ALPHACHANNEL | DWCAPS_LR_MONO);
        sWindowDesc.width = iWindowWidth;
        sWindowDesc.height = iWindowHeight;
        sWindowDesc.posx = x1;
        sWindowDesc.posy = y1;
        sWindowDesc.pixelformat = DSPF_ARGB;
        sWindowDesc.options = (DFBWindowOptions)(DWOP_ALPHACHANNEL | DWOP_SCALE);
        sWindowDesc.surface_caps = (DFBSurfaceCapabilities)(DSCAPS_PREMULTIPLIED | DSCAPS_VIDEOONLY);
        DFBCHECK(pPrimary->CreateWindow(pPrimary, &sWindowDesc, &pWindow1));

        /* Draw window mono image. */
        DFBCHECK(pWindow1->GetSurface(pWindow1, &pWindowSurface1));
        DFBCHECK(pWindowSurface1->Clear(pWindowSurface1, 0, 0, 0, 0));
        sWindowRect.x = sWindowRect.y = 0; sWindowRect.w = sWindowDesc.width; sWindowRect.h = sWindowDesc.height;
        DFBCHECK(pWindowSurface1->StretchBlit(pWindowSurface1, pImageSurface, &sImgRect, &sWindowRect));
        DFBCHECK(pWindowSurface1->SetFont(pWindowSurface1, pFont));
        DFBCHECK(pWindowSurface1->SetColor(pWindowSurface1, 0xff, 0xff, 0xff, 0xff));
        DFBCHECK(pWindowSurface1->DrawString(pWindowSurface1, "L/R Mono", 8, sWindowRect.w/2,
            sWindowRect.h/2-25, DSTF_CENTER));
        DFBCHECK(pWindowSurface1->DrawString(pWindowSurface1, "Window", 6, sWindowRect.w/2,
            sWindowRect.h/2, DSTF_CENTER));
        DFBCHECK(pWindowSurface1->Flip(pWindowSurface1, NULL, DSFLIP_WAITFORSYNC));
        DFBCHECK(pWindow1->SetOpacity(pWindow1, 0xff));
        DFBCHECK(pWindow1->SetStereoDepth(pWindow1, z1));

        /*
         * Stereo Window
         */

        /* Create stereo window */
        sWindowDesc.caps &= ~DWCAPS_LR_MONO;
        sWindowDesc.caps |= DWCAPS_STEREO;
        sWindowDesc.posx = x2;
        sWindowDesc.posy = y2;
        sWindowDesc.surface_caps |= DSCAPS_STEREO;
        DFBCHECK(pPrimary->CreateWindow(pPrimary, &sWindowDesc, &pWindow2));

        /* Draw window stereo images. */
        DFBCHECK(pWindow2->GetSurface(pWindow2, &pWindowSurface2));
        DFBCHECK(pWindowSurface2->SetStereoEye(pWindowSurface2, DSSE_LEFT));
        DFBCHECK(pWindowSurface2->Clear(pWindowSurface2, 0, 0, 0, 0));
        DFBCHECK(pWindowSurface2->StretchBlit(pWindowSurface2, pImageSurface, &sImgRect, &sWindowRect));
        DFBCHECK(pWindowSurface2->SetFont(pWindowSurface2, pFont));
        DFBCHECK(pWindowSurface2->SetColor(pWindowSurface2, 0xff, 0xff, 0xff, 0xff));
        DFBCHECK(pWindowSurface2->DrawString(pWindowSurface2, "Stereo Window", 13, sWindowRect.w/2,
            sWindowRect.h/2-25, DSTF_CENTER));
        DFBCHECK(pWindowSurface2->DrawString(pWindowSurface2, "This is closer...", 17,
            sWindowRect.w/2 + CLOSER_TEXT_DEPTH, sWindowRect.h/2, DSTF_CENTER));
        DFBCHECK(pWindowSurface2->DrawString(pWindowSurface2, "than this.", 10, sWindowRect.w/2,
            sWindowRect.h/2+25, DSTF_CENTER));
        DFBCHECK(pWindowSurface2->SetStereoEye(pWindowSurface2, DSSE_RIGHT));
        DFBCHECK(pWindowSurface2->Clear(pWindowSurface2, 0, 0, 0, 0));
        DFBCHECK(pWindowSurface2->StretchBlit(pWindowSurface2, pImageSurface, &sImgRect, &sWindowRect));
        DFBCHECK(pWindowSurface2->DrawString(pWindowSurface2, "Stereo Window", 13, sWindowRect.w/2,
            sWindowRect.h/2-25, DSTF_CENTER));
        DFBCHECK(pWindowSurface2->DrawString(pWindowSurface2, "This is closer...", 17,
            sWindowRect.w/2 - CLOSER_TEXT_DEPTH, sWindowRect.h/2, DSTF_CENTER));
        DFBCHECK(pWindowSurface2->DrawString(pWindowSurface2, "than this.", 10, sWindowRect.w/2,
            sWindowRect.h/2+25, DSTF_CENTER));
        DFBCHECK(pWindowSurface2->FlipStereo(pWindowSurface2, NULL, NULL, DSFLIP_WAITFORSYNC));
        DFBCHECK(pWindow2->SetOpacity(pWindow2, 0xff));
        DFBCHECK(pWindow2->SetStereoDepth(pWindow2, z2));

        pWindow = pWindow1;
        dx = &dx1;
        dy = &dy1;
        dz = &dz1;
        for (j = 0; j < WINDOW_ITERATIONS; j++)
        {
            {
                for (i = 0; i < 3; i++)
                {
                    DFBCHECK(pWindow->GetPosition(pWindow, &x, &y));
                    x += *dx;
                    y += *dy;
                    DFBCHECK(pWindow->MoveTo(pWindow, x, y));
                    if (pWindow != pWindow0)
                    {
                        DFBCHECK(pWindow->GetStereoDepth(pWindow, &z));
                        z += *dz;
                        DFBCHECK(pWindow->SetStereoDepth(pWindow, z));
                    }
                    if (x + sWindowRect.w >= iScreenWidth || x <= 0)
                        *dx *= -1;
                    if (y + sWindowRect.h >= iScreenHeight || y <= 0)
                        *dy *= -1;
                    if (z >= MAX_DEPTH || z <= 0)
                        *dz *= -1;

                    pWindow = pWindow == pWindow0 ? pWindow1 : pWindow == pWindow1 ? pWindow2 : pWindow0;
                    dx = dx == &dx0 ? &dx1 : dx == &dx1 ? &dx2 : &dx0;
                    dy = dy == &dy0 ? &dy1 : dy == &dy1 ? &dy2 : &dy0;
                    dz = dz == &dz0 ? &dz1 : dz == &dz1 ? &dz2 : &dz0;
                }
            }
        }

        DFBCHECK(pWindow0->GetPosition(pWindow0, &x0, &y0));
        DFBCHECK(pWindow1->GetPosition(pWindow1, &x1, &y1));
        DFBCHECK(pWindow1->GetStereoDepth(pWindow1, &z1));
        DFBCHECK(pWindow2->GetPosition(pWindow2, &x2, &y2));
        DFBCHECK(pWindow2->GetStereoDepth(pWindow2, &z2));

        DFBCHECK(pWindowSurface0->ReleaseSource(pWindowSurface0));
        DFBCHECK(pWindowSurface0->Release(pWindowSurface0));
        DFBCHECK(pWindow0->Release(pWindow0));
        DFBCHECK(pWindowSurface1->ReleaseSource(pWindowSurface1));
        DFBCHECK(pWindowSurface1->Release(pWindowSurface1));
        DFBCHECK(pWindow1->Release(pWindow1));
        DFBCHECK(pWindowSurface2->ReleaseSource(pWindowSurface2));
        DFBCHECK(pWindowSurface2->Release(pWindowSurface2));
        DFBCHECK(pWindow2->Release(pWindow2));

        DFBCHECK(pLayerSurface->Clear(pLayerSurface, 0, 0, 0, 0));
        DFBCHECK(pLayerSurface->Flip(pLayerSurface, NULL, DSFLIP_WAITFORSYNC));

        DFBCHECK(pLayerSurface->ReleaseSource(pLayerSurface));
        DFBCHECK(pLayerSurface->Release(pLayerSurface));
    }

    DFBCHECK(pImageSurface->ReleaseSource(pImageSurface));
    DFBCHECK(pImageSurface->Release(pImageSurface));
    DFBCHECK(pPrimary->Release(pPrimary));
    DFBCHECK(pScreen->Release(pScreen));
    DFBCHECK(pDfb->Release(pDfb));

    return 0;
}

