/*
* Waifu2x-opt image restoration filter - VapourSynth plugin
* Copyright (C) 2015  mawen1250
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "Waifu2x_Denoise.h"
#include "Waifu2x_Resize.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: waifu2x.Denoise


static void VS_CC Waifu2x_Denoise_Init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    Waifu2x_Denoise_Data *d = reinterpret_cast<Waifu2x_Denoise_Data *>(*instanceData);

    vsapi->setVideoInfo(d->vi, 1, node);
}

static const VSFrameRef *VS_CC Waifu2x_Denoise_GetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    Waifu2x_Denoise_Data *d = reinterpret_cast<Waifu2x_Denoise_Data *>(*instanceData);

    if (activationReason == arInitial)
    {
        vsapi->requestFrameFilter(n, d->node, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        Waifu2x_Denoise_Process p(*d, n, frameCtx, core, vsapi);

        p.set_mutex(d->waifu2x, d->waifu2x_mutex);

        return p.process();
    }

    return nullptr;
}

static void VS_CC Waifu2x_Denoise_Free(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    Waifu2x_Denoise_Data *d = reinterpret_cast<Waifu2x_Denoise_Data *>(instanceData);

    delete d;
}


static void VS_CC Waifu2x_Denoise_Create(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    Waifu2x_Denoise_Data *d = new Waifu2x_Denoise_Data(vsapi);

    if (d->arguments_process(in, out))
    {
        delete d;
        return;
    }

    d->init(core);

    // Create filter
    vsapi->createFilter(in, out, "Denoise", Waifu2x_Denoise_Init, Waifu2x_Denoise_GetFrame, Waifu2x_Denoise_Free, fmParallel, 0, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: waifu2x.Resize


static void VS_CC Waifu2x_Resize_Init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    Waifu2x_Resize_Data *d = reinterpret_cast<Waifu2x_Resize_Data *>(*instanceData);

    VSVideoInfo dvi = *(d->vi);
    dvi.width = d->para.width;
    dvi.height = d->para.height;

    vsapi->setVideoInfo(&dvi, 1, node);
}

static const VSFrameRef *VS_CC Waifu2x_Resize_GetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    Waifu2x_Resize_Data *d = reinterpret_cast<Waifu2x_Resize_Data *>(*instanceData);

    if (activationReason == arInitial)
    {
        vsapi->requestFrameFilter(n, d->node, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        Waifu2x_Resize_Process p(*d, n, frameCtx, core, vsapi);

        p.set_mutex(d->waifu2x, d->waifu2x_mutex);

        return p.process();
    }

    return nullptr;
}

static void VS_CC Waifu2x_Resize_Free(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    Waifu2x_Resize_Data *d = reinterpret_cast<Waifu2x_Resize_Data *>(instanceData);

    delete d;
}


static void VS_CC Waifu2x_Resize_Create(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    Waifu2x_Resize_Data *d = new Waifu2x_Resize_Data(vsapi);

    if (d->arguments_process(in, out))
    {
        delete d;
        return;
    }

    d->init(core);

    // Create filter
    vsapi->createFilter(in, out, "Resize", Waifu2x_Resize_Init, Waifu2x_Resize_GetFrame, Waifu2x_Resize_Free, fmParallel, 0, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: plugin initialization


VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
    configFunc("Ay.is.baka", "waifu2x",
        "Waifu2x-opt image restoration filter for VapourSynth.",
        VAPOURSYNTH_API_VERSION, 1, plugin);

    registerFunc("Denoise",
        "input:clip;"
        "mode:int:opt;"
        "matrix:int:opt;"
        "full:int:opt;",
        Waifu2x_Denoise_Create, nullptr, plugin);

    registerFunc("Resize",
        "input:clip;"
        "width:int:opt;"
        "height:int:opt;"
        "filter:data:opt;"
        "filter_param_a:float:opt;"
        "filter_param_b:float:opt;"
        "filter_uv:data:opt;"
        "filter_param_a_uv:float:opt;"
        "filter_param_b_uv:float:opt;"
        "chroma_loc:data:opt;"
        "matrix:int:opt;"
        "full:int:opt;",
        Waifu2x_Resize_Create, nullptr, plugin);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
