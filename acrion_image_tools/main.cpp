/*
Copyright (c) 2025 acrion innovations GmbH
Authors: Stefan Zipproth, s.zipproth@acrion.ch

This file is part of acrion image tools, see https://github.com/acrion/image-tools

acrion image tools is offered under a commercial and under the AGPL license.
For commercial licensing, contact us at https://acrion.ch/sales. For AGPL licensing, see below.

AGPL licensing:

acrion image tools is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

acrion image tools is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with acrion image tools. If not, see <https://www.gnu.org/licenses/>.
*/

#include "acrion_image_tools_export.h"

#include <cbeam/container/nested_map.hpp>

#include <cbeam/serialization/xpod.hpp>

#include <cbeam/serialization/nested_map.hpp>

#include <cbeam/container/nested_map.hpp>
#include <cbeam/container/xpod.hpp>
#include <cbeam/serialization/direct.hpp>

#include "io.hpp"

#include "acrion/image/bitmap.hpp"

#include <cbeam/convert/string.hpp>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
using namespace acrion::imagetools;
using namespace std::string_literals;

template <typename T>
void Swap(T* right, T* left, const T* left_end)
{
    while (left != left_end)
    {
        const T leftValue = *left;
        *left++           = *right;
        *right++          = leftValue;
    }
}

template <typename T>
void CopyLeftToRight(T* right, T* left, const T* left_end)
{
    while (left != left_end)
    {
        *right++ = *left++;
    }
}

template <typename T>
void CopyRightToLeft(T* right, T* left, const T* left_end)
{
    while (left != left_end)
    {
        *left++ = *right++;
    }
}

template <typename T>
void InvertImage(T* right, const T* right_end, const T min, const T max)
{
    while (right != right_end)
    {
        *right = max - (*right - min);
        right++;
    }
}

template <typename T>
void SubtractWorkingImageFromReference(T* right, T* left, const T* left_end, const long long mode)
{
    while (left != left_end)
    {
        if (mode == 2)
        {
            *right = *left > *right ? *left - *right : *right - *left;
        }
        else if (mode == 1 || *left >= *right)
        {
            *right = *left - *right;
        }
        else
        {
            *right = 0;
        }

        ++left;
        ++right;
    }
}

template <typename T>
void SubtractReferenceFromWorkingImage(T* right, T* left, const T* left_end, const long long mode)
{
    while (left != left_end)
    {
        if (mode == 2)
        {
            *right = *left > *right ? *left - *right : *right - *left;
        }
        else if (mode == 1 || *right >= *left)
        {
            *right = *right - *left;
        }
        else
        {
            *right = 0;
        }

        ++left;
        ++right;
    }
}

extern "C" ACRION_IMAGE_TOOLS_EXPORT acrion::image::SerializedBitmapContainer OpenImageFile(const char* fileName)
{
    acrion::image::BitmapContainer image;

    try
    {
        std::string        warning;
        const std::wstring fileName16 = cbeam::convert::from_string<std::wstring>(fileName);

        image = (acrion::image::BitmapContainer)*io::Read(fileName16, warning);

        if (!warning.empty())
        {
            image.data["message"] = warning;
        }

        image.data["path"] = std::string(fileName);
    }
    catch (const std::exception& ex)
    {
        image.data["error"] = (std::string)ex.what();
    }

    auto buffer = cbeam::serialization::serialize(image);
    assert(buffer.use_count() > 1 && "Create an instance of cbeam::container::stable_reference_buffer::delay_deallocation prior using this function.");
    return buffer.get();
}

extern "C" ACRION_IMAGE_TOOLS_EXPORT acrion::image::SerializedBitmapContainer SaveImageFile(const acrion::image::SerializedBitmapContainer serializedImage)
{
    acrion::image::BitmapContainer result;

    try
    {
        acrion::image::BitmapContainer image    = cbeam::serialization::deserialize<acrion::image::BitmapContainer>(serializedImage);
        const std::string&             savePath = image.get_mapped_value_or_throw<std::string>("path", "acrion::imagetools::SaveImageFile()");
        acrion::image::Bitmap          bitmap(image);
        std::string                    warning;
        std::string                    error;
        io::Write(bitmap,
                  cbeam::convert::from_string<std::wstring>(savePath),
                  warning);

        if (!error.empty())
        {
            result.data["message"] = error;
            result.data["path"]    = savePath;
        }
        else if (!warning.empty())
        {
            result.data["message"] = warning;
            result.data["path"]    = savePath;
        }
    }
    catch (const std::exception& ex)
    {
        result.data["error"] = (std::string)ex.what();
    }

    return cbeam::serialization::serialize(result).safe_get();
}

extern "C" ACRION_IMAGE_TOOLS_EXPORT acrion::image::SerializedBitmapContainer Swap(const acrion::image::SerializedBitmapContainer serializedParameters)
{
    acrion::image::BitmapContainer result;

    try
    {
        acrion::image::BitmapContainer parameters = cbeam::serialization::deserialize<acrion::image::BitmapContainer>(serializedParameters);

        const auto referenceImage = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>("referenceImageBuffer"s);
        const auto workingImage   = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>(std::string(acrion::image::Bitmap::bufferKey));
        const auto width          = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::widthKey));
        const auto height         = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::heightKey));
        const auto channels       = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::channelsKey));
        const auto depth          = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::depthKey));

        const size_t size = width * height * channels;

        switch (depth)
        {
        case 1:
            Swap((uint8_t*)workingImage, (uint8_t*)referenceImage, (uint8_t*)referenceImage + size);
            break;
        case 2:
            Swap((uint16_t*)workingImage, (uint16_t*)referenceImage, (uint16_t*)referenceImage + size);
            break;
        case 4:
            Swap((uint32_t*)workingImage, (uint32_t*)referenceImage, (uint32_t*)referenceImage + size);
            break;
        case 8:
            Swap((uint64_t*)workingImage, (uint64_t*)referenceImage, (uint64_t*)referenceImage + size);
            break;
        case -8:
            Swap((double*)workingImage, (double*)referenceImage, (double*)referenceImage + size);
            break;
        default:
            throw std::runtime_error("acrion image tools: unsupported depth " + std::to_string(depth) + "in function Swap");
        }
    }
    catch (const std::exception& ex)
    {
        result.data["error"] = (std::string)ex.what();
    }

    return cbeam::serialization::serialize(result).safe_get();
}

extern "C" ACRION_IMAGE_TOOLS_EXPORT acrion::image::SerializedBitmapContainer CopyLeftToRight(const acrion::image::SerializedBitmapContainer serializedParameters)
{
    acrion::image::BitmapContainer result;

    try
    {
        acrion::image::BitmapContainer parameters = cbeam::serialization::deserialize<acrion::image::BitmapContainer>(serializedParameters);

        const auto referenceImage = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>("referenceImageBuffer"s);
        const auto workingImage   = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>(std::string(acrion::image::Bitmap::bufferKey));
        const auto width          = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::widthKey));
        const auto height         = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::heightKey));
        const auto channels       = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::channelsKey));
        const auto depth          = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::depthKey));

        const size_t size = width * height * channels;

        switch (depth)
        {
        case 1:
            CopyLeftToRight((uint8_t*)workingImage, (uint8_t*)referenceImage, (uint8_t*)referenceImage + size);
            break;
        case 2:
            CopyLeftToRight((uint16_t*)workingImage, (uint16_t*)referenceImage, (uint16_t*)referenceImage + size);
            break;
        case 4:
            CopyLeftToRight((uint32_t*)workingImage, (uint32_t*)referenceImage, (uint32_t*)referenceImage + size);
            break;
        case 8:
            CopyLeftToRight((uint64_t*)workingImage, (uint64_t*)referenceImage, (uint64_t*)referenceImage + size);
            break;
        case -8:
            CopyLeftToRight((double*)workingImage, (double*)referenceImage, (double*)referenceImage + size);
            break;
        default:
            throw std::runtime_error("acrion image tools: unsupported depth " + std::to_string(depth) + "in function CopyLeftToRight");
        }
    }
    catch (const std::exception& ex)
    {
        result.data["error"] = (std::string)ex.what();
    }

    return cbeam::serialization::serialize(result).safe_get();
}

extern "C" ACRION_IMAGE_TOOLS_EXPORT acrion::image::SerializedBitmapContainer CopyRightToLeft(const acrion::image::SerializedBitmapContainer serializedParameters)
{
    acrion::image::BitmapContainer result;

    try
    {
        acrion::image::BitmapContainer parameters = cbeam::serialization::deserialize<acrion::image::BitmapContainer>(serializedParameters);

        const auto   referenceImage = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>("referenceImageBuffer"s);
        const auto   workingImage   = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>(std::string(acrion::image::Bitmap::bufferKey));
        const auto   width          = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::widthKey));
        const auto   height         = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::heightKey));
        const auto   channels       = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::channelsKey));
        const auto   depth          = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::depthKey));
        const size_t size           = width * height * channels;

        switch (depth)
        {
        case 1:
            CopyRightToLeft((uint8_t*)workingImage, (uint8_t*)referenceImage, (uint8_t*)referenceImage + size);
            break;
        case 2:
            CopyRightToLeft((uint16_t*)workingImage, (uint16_t*)referenceImage, (uint16_t*)referenceImage + size);
            break;
        case 4:
            CopyRightToLeft((uint32_t*)workingImage, (uint32_t*)referenceImage, (uint32_t*)referenceImage + size);
            break;
        case 8:
            CopyRightToLeft((uint64_t*)workingImage, (uint64_t*)referenceImage, (uint64_t*)referenceImage + size);
            break;
        case -8:
            CopyRightToLeft((double*)workingImage, (double*)referenceImage, (double*)referenceImage + size);
            break;
        default:
            throw std::runtime_error("acrion image tools: unsupported depth " + std::to_string(depth) + "in function CopyRightToLeft");
        }
    }
    catch (const std::exception& ex)
    {
        result.data["error"] = (std::string)ex.what();
    }

    return cbeam::serialization::serialize(result).safe_get();
}

extern "C" ACRION_IMAGE_TOOLS_EXPORT acrion::image::SerializedBitmapContainer InvertImage(const acrion::image::SerializedBitmapContainer serializedParameters)
{
    acrion::image::BitmapContainer result;

    try
    {
        acrion::image::BitmapContainer parameters = cbeam::serialization::deserialize<acrion::image::BitmapContainer>(serializedParameters);

        const auto workingImage  = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>(std::string(acrion::image::Bitmap::bufferKey));
        const auto width         = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::widthKey));
        const auto height        = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::heightKey));
        const auto channels      = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::channelsKey));
        const auto minBrightness = parameters.get_mapped_value_or_throw<double>(std::string(acrion::image::Bitmap::minBrightnessKey));
        const auto maxBrightness = parameters.get_mapped_value_or_throw<double>(std::string(acrion::image::Bitmap::maxBrightnessKey));
        const auto depth         = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::depthKey));

        const size_t size = width * height * channels;

        switch (depth)
        {
        case 1:
            InvertImage((uint8_t*)workingImage, (uint8_t*)workingImage + size, (uint8_t)minBrightness, (uint8_t)maxBrightness);
            break;
        case 2:
            InvertImage((uint16_t*)workingImage, (uint16_t*)workingImage + size, (uint16_t)minBrightness, (uint16_t)maxBrightness);
            break;
        case 4:
            InvertImage((uint32_t*)workingImage, (uint32_t*)workingImage + size, (uint32_t)minBrightness, (uint32_t)maxBrightness);
            break;
        case 8:
            InvertImage((uint64_t*)workingImage, (uint64_t*)workingImage + size, (uint64_t)minBrightness, (uint64_t)maxBrightness);
            break;
        case -8:
            InvertImage((double*)workingImage, (double*)workingImage + size, minBrightness, maxBrightness);
            break;
        default:
            throw std::runtime_error("acrion image tools: unsupported depth " + std::to_string(depth) + "in function InvertImage");
        }
    }
    catch (const std::exception& ex)
    {
        result.data["error"] = (std::string)ex.what();
    }

    return cbeam::serialization::serialize(result).safe_get();
}

extern "C" ACRION_IMAGE_TOOLS_EXPORT acrion::image::SerializedBitmapContainer SubtractWorkingImageFromReference(const acrion::image::SerializedBitmapContainer serializedParameters, long long mode)
{
    acrion::image::BitmapContainer result;

    try
    {
        acrion::image::BitmapContainer parameters = cbeam::serialization::deserialize<acrion::image::BitmapContainer>(serializedParameters);

        const auto referenceImage = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>("referenceImageBuffer"s);
        const auto workingImage   = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>(std::string(acrion::image::Bitmap::bufferKey));
        const auto width          = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::widthKey));
        const auto height         = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::heightKey));
        const auto channels       = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::channelsKey));
        const auto depth          = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::depthKey));

        const size_t size = width * height * channels;

        switch (depth)
        {
        case 1:
            SubtractWorkingImageFromReference((uint8_t*)workingImage, (uint8_t*)referenceImage, (uint8_t*)referenceImage + size, mode);
            break;
        case 2:
            SubtractWorkingImageFromReference((uint16_t*)workingImage, (uint16_t*)referenceImage, (uint16_t*)referenceImage + size, mode);
            break;
        case 4:
            SubtractWorkingImageFromReference((uint32_t*)workingImage, (uint32_t*)referenceImage, (uint32_t*)referenceImage + size, mode);
            break;
        case 8:
            SubtractWorkingImageFromReference((uint64_t*)workingImage, (uint64_t*)referenceImage, (uint64_t*)referenceImage + size, mode);
            break;
        case -8:
            SubtractWorkingImageFromReference((double*)workingImage, (double*)referenceImage, (double*)referenceImage + size, mode);
            break;
        default:
            throw std::runtime_error("acrion image tools: unsupported depth " + std::to_string(depth) + "in function SubtractWorkingImageFromReference");
        }
    }
    catch (const std::exception& ex)
    {
        result.data["error"] = (std::string)ex.what();
    }

    return cbeam::serialization::serialize(result).safe_get();
}

extern "C" ACRION_IMAGE_TOOLS_EXPORT acrion::image::SerializedBitmapContainer SubtractReferenceFromWorkingImage(const acrion::image::SerializedBitmapContainer serializedParameters, long long mode)
{
    acrion::image::BitmapContainer result;

    try
    {
        acrion::image::BitmapContainer parameters = cbeam::serialization::deserialize<acrion::image::BitmapContainer>(serializedParameters);

        const auto referenceImage = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>("referenceImageBuffer"s);
        const auto workingImage   = (void*)parameters.get_mapped_value_or_throw<cbeam::memory::pointer>(std::string(acrion::image::Bitmap::bufferKey));
        const auto width          = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::widthKey));
        const auto height         = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::heightKey));
        const auto channels       = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::channelsKey));
        const auto depth          = parameters.get_mapped_value_or_throw<long long>(std::string(acrion::image::Bitmap::depthKey));

        const size_t size = width * height * channels;

        switch (depth)
        {
        case 1:
            SubtractReferenceFromWorkingImage((uint8_t*)workingImage, (uint8_t*)referenceImage, (uint8_t*)referenceImage + size, mode);
            break;
        case 2:
            SubtractReferenceFromWorkingImage((uint16_t*)workingImage, (uint16_t*)referenceImage, (uint16_t*)referenceImage + size, mode);
            break;
        case 4:
            SubtractReferenceFromWorkingImage((uint32_t*)workingImage, (uint32_t*)referenceImage, (uint32_t*)referenceImage + size, mode);
            break;
        case 8:
            SubtractReferenceFromWorkingImage((uint64_t*)workingImage, (uint64_t*)referenceImage, (uint64_t*)referenceImage + size, mode);
            break;
        case -8:
            SubtractReferenceFromWorkingImage((double*)workingImage, (double*)referenceImage, (double*)referenceImage + size, mode);
            break;
        default:
            throw std::runtime_error("acrion image tools: unsupported depth " + std::to_string(depth) + "in function SubtractReferenceFromWorkingImage");
        }
    }
    catch (const std::exception& ex)
    {
        result.data["error"] = (std::string)ex.what();
    }

    return cbeam::serialization::serialize(result).safe_get();
}

#pragma clang diagnostic pop