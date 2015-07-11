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


#include <algorithm>
#include "Waifu2x_Resize.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int Waifu2x_Resize_Data::arguments_process(const VSMap *in, VSMap *out)
{
    if (_Mybase::arguments_process(in, out))
    {
        return 1;
    }

    int error;

    // width - int
    para.width = int64ToIntS(vsapi->propGetInt(in, "width", 0, &error));

    if (error)
    {
        para.width = vi->width * 2;
    }
    else if (para.width <= 0)
    {
        setError(out, "\'width\' must be a positive integer");
        return 1;
    }
    else if (para.width % (1 << vi->format->subSamplingW) != 0)
    {
        setError(out, "\'width\' must be a multiplicate of horizontal sub-sampling ratio");
        return 1;
    }

    // height - int
    para.height = int64ToIntS(vsapi->propGetInt(in, "height", 0, &error));

    if (error)
    {
        para.height = vi->height * 2;
    }
    else if (para.height <= 0)
    {
        setError(out, "\'height\' must be a positive integer");
        return 1;
    }
    else if (para.height % (1 << vi->format->subSamplingH) != 0)
    {
        setError(out, "\'height\' must be a multiplicate of vertical sub-sampling ratio");
        return 1;
    }

    // filter - data
    auto filter_cstr = vsapi->propGetData(in, "filter", 0, &error);

    if (error)
    {
        para.filter = para_default.filter;
    }
    else
    {
        std::string filter = filter_cstr;
        std::transform(filter.begin(), filter.end(), filter.begin(), tolower);
        para.filter = zimg_translate_filter(filter.c_str());

        if (para.filter < 0)
        {
            setError(out, "invalid \'filter\' specified");
            return 1;
        }
    }

    // filter_param_a - float
    para.filter_param_a = vsapi->propGetFloat(in, "filter_param_a", 0, &error);

    if (error)
    {
        if (para.filter == ZIMG_RESIZE_LANCZOS)
        {
            para.filter_param_a = 3;
        }
        else
        {
            para.filter_param_a = para_default.filter_param_a;
        }
    }

    // filter_param_b - float
    para.filter_param_b = vsapi->propGetFloat(in, "filter_param_b", 0, &error);

    if (error)
    {
        if (para.filter == ZIMG_RESIZE_LANCZOS)
        {
            para.filter_param_b = 0;
        }
        else
        {
            para.filter_param_b = para_default.filter_param_b;
        }
    }

    // filter_uv - data
    auto filter_uv_cstr = vsapi->propGetData(in, "filter_uv", 0, &error);

    if (error)
    {
        para.filter_uv = para_default.filter_uv;
    }
    else
    {
        std::string filter_uv = filter_uv_cstr;
        std::transform(filter_uv.begin(), filter_uv.end(), filter_uv.begin(), tolower);
        para.filter_uv = zimg_translate_filter(filter_uv.c_str());

        if (para.filter_uv < 0)
        {
            setError(out, "invalid \'filter_uv\' specified");
            return 1;
        }
    }

    // filter_param_a_uv - float
    para.filter_param_a_uv = vsapi->propGetFloat(in, "filter_param_a_uv", 0, &error);

    if (error)
    {
        if (para.filter_uv == ZIMG_RESIZE_LANCZOS)
        {
            para.filter_param_a_uv = 3;
        }
        else
        {
            para.filter_param_a_uv = para_default.filter_param_a_uv;
        }
    }

    // filter_param_b_uv - float
    para.filter_param_b_uv = vsapi->propGetFloat(in, "filter_param_b_uv", 0, &error);

    if (error)
    {
        if (para.filter_uv == ZIMG_RESIZE_LANCZOS)
        {
            para.filter_param_b_uv = 0;
        }
        else
        {
            para.filter_param_b_uv = para_default.filter_param_b_uv;
        }
    }

    // chroma_loc - data
    auto chroma_loc_cstr = vsapi->propGetData(in, "chroma_loc", 0, &error);

    if (error)
    {
        para.chroma_loc = para_default.chroma_loc;
    }
    else
    {
        std::string chroma_loc = chroma_loc_cstr;
        std::transform(chroma_loc.begin(), chroma_loc.end(), chroma_loc.begin(), tolower);

        if (chroma_loc == "mpeg1")
        {
            para.chroma_loc = CHROMA_LOC_MPEG1;
        }
        else if (chroma_loc == "mpeg2")
        {
            para.chroma_loc = CHROMA_LOC_MPEG2;
        }
        else
        {
            setError(out, "invalid \'chroma_loc\' specified, should be \'mpeg1\' or \'mpeg2\'");
            return 1;
        }
    }

    // process
    chroma = vi->format->colorFamily != cmGray;

    return 0;
}


void Waifu2x_Resize_Data::init(VSCore *core)
{
    // Initialize z_resize
    int src_width = vi->width;
    int src_height = vi->height;
    int dst_width = para.width;
    int dst_height = para.height;
    double shift_w = para.shift_w;
    double shift_h = para.shift_h;
    double subwidth = para.subwidth <= 0 ? src_width - para.subwidth : para.subwidth;
    double subheight = para.subheight <= 0 ? src_height - para.subheight : para.height;

    double scaleH = static_cast<double>(dst_width) / subwidth;
    double scaleV = static_cast<double>(dst_height) / subheight;
    scale = Max(scaleH, scaleV);
    double sub_w = 1 << vi->format->subSamplingW;
    double sub_h = 1 << vi->format->subSamplingH;

    bool sCLeftAlign = para.chroma_loc == CHROMA_LOC_MPEG2;
    double sHCPlace = sCLeftAlign ? 0 : 0.5 - sub_w / 2;
    double sVCPlace = 0;
    bool dCLeftAlign = para.chroma_loc == CHROMA_LOC_MPEG2;
    double dHCPlace = dCLeftAlign ? 0 : 0.5 - sub_h / 2;
    double dVCPlace = 0;

    int src_width_uv = src_width >> vi->format->subSamplingW;
    int src_height_uv = src_height >> vi->format->subSamplingH;
    int dst_width_uv = dst_width >> vi->format->subSamplingW;
    int dst_height_uv = dst_height >> vi->format->subSamplingH;
    double shift_w_uv = ((shift_w - sHCPlace) * scaleH + dHCPlace) / scaleH / sub_w;
    double shift_h_uv = ((shift_h - sVCPlace) * scaleV + dVCPlace) / scaleV / sub_h;
    double subwidth_uv = subwidth / sub_w;
    double subheight_uv = subheight / sub_h;

    init_z_resize(z_resize_pre, ZIMG_RESIZE_POINT, src_width, src_height,
        dst_width, dst_height, shift_w, shift_h, subwidth, subheight,
        para.filter_param_a, para.filter_param_b);
    init_z_resize(z_resize_uv, para.filter_uv,
        src_width_uv, src_height_uv, dst_width_uv, dst_height_uv, shift_w_uv, shift_h_uv, subwidth_uv, subheight_uv,
        para.filter_param_a_uv, para.filter_param_b_uv);

    // Initialize waifu2x
    init_waifu2x(waifu2x, waifu2x_mutex, 0, dst_width, dst_height, core, vsapi);
}


void Waifu2x_Resize_Data::release()
{
    delete z_resize_pre;
    delete z_resize;
    delete z_resize_uv;

    z_resize_pre = nullptr;
    z_resize = nullptr;
    z_resize_uv = nullptr;
}


void Waifu2x_Resize_Data::moveFrom(_Myt &right)
{
    z_resize_pre = right.z_resize_pre;
    z_resize = right.z_resize;
    z_resize_uv = right.z_resize_uv;
    
    right.z_resize_pre = nullptr;
    right.z_resize = nullptr;
    right.z_resize_uv = nullptr;
}


void Waifu2x_Resize_Data::init_z_resize(ZimgResizeContext *&context,
    int filter_type, int src_width, int src_height, int dst_width, int dst_height,
    double shift_w, double shift_h, double subwidth, double subheight,
    double filter_param_a, double filter_param_b)
{
    context = new ZimgResizeContext(filter_type, src_width, src_height,
        dst_width, dst_height, shift_w, shift_h, subwidth, subheight, filter_param_a, filter_param_b);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void Waifu2x_Resize_Process::Kernel(FLType *dst, const FLType *src) const
{
    FLType *tempY = nullptr;

    AlignedMalloc(tempY, dst_pcount[0]);

    const int pixel_type = ZIMG_PIXEL_FLOAT;
    void *buf = nullptr;
    size_t buf_size = d.z_resize_pre->tmp_size(pixel_type);
    VS_ALIGNED_MALLOC(&buf, buf_size, 32);

    d.z_resize_pre->process(src, tempY, buf, src_width[0], src_height[0], dst_width[0], dst_height[0],
        src_stride[0] * sizeof(FLType), dst_stride[0] * sizeof(FLType), pixel_type);
    waifu2x->process(dst, tempY, dst_width[0], dst_height[0], dst_stride[0], false);

    VS_ALIGNED_FREE(buf);
    AlignedFree(tempY);
}


void Waifu2x_Resize_Process::Kernel(FLType *dstY, FLType *dstU, FLType *dstV,
    const FLType *srcY, const FLType *srcU, const FLType *srcV) const
{
    FLType *tempY = nullptr;
    AlignedMalloc(tempY, dst_pcount[0]);

    const int pixel_type = ZIMG_PIXEL_FLOAT;
    void *buf = nullptr;
    size_t buf_size = d.z_resize_pre->tmp_size(pixel_type);
    size_t buf_size_uv = d.z_resize_uv->tmp_size(pixel_type);
    buf_size = buf_size_uv > buf_size ? buf_size_uv : buf_size;
    VS_ALIGNED_MALLOC(&buf, buf_size, 32);

    d.z_resize_pre->process(srcY, tempY, buf, src_width[0], src_height[0], dst_width[0], dst_height[0],
        src_stride[0] * sizeof(FLType), dst_stride[0] * sizeof(FLType), pixel_type);
    waifu2x->process(dstY, tempY, dst_width[0], dst_height[0], dst_stride[0], false);

    d.z_resize_uv->process(srcU, dstU, buf, src_width[1], src_height[1], dst_width[1], dst_height[1],
        src_stride[1] * sizeof(FLType), dst_stride[1] * sizeof(FLType), pixel_type);
    d.z_resize_uv->process(srcV, dstV, buf, src_width[2], src_height[2], dst_width[2], dst_height[2],
        src_stride[2] * sizeof(FLType), dst_stride[2] * sizeof(FLType), pixel_type);

    VS_ALIGNED_FREE(buf);
    AlignedFree(tempY);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
