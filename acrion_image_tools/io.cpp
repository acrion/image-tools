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

#include "io.hpp"

#include "fits.hpp"

#include <cbeam/convert/string.hpp>
#include <cbeam/logging/log_manager.hpp>
#include <cbeam/platform/system_folders.hpp>

#include <filesystem>
#include <iostream>

#include "imagemagick.hpp" // needs to be included after <cassert>, otherwise assert macro is messed up

namespace acrion::imagetools::io
{
    void ensure_magick_initialized()
    {
        static bool once = []{
            const auto im_root = cbeam::filesystem::get_current_binary_path(false);
            const auto coders  = im_root / "modules-Q32" / "coders";
            const auto filters = im_root / "modules-Q32" / "filters";

#ifdef _WIN32
            // Use wide APIs to preserve spaces/unicode.
            _wputenv_s(L"MAGICK_HOME",               im_root.wstring().c_str());
            _wputenv_s(L"MAGICK_CODER_MODULE_PATH",  coders.wstring().c_str());
            _wputenv_s(L"MAGICK_FILTER_MODULE_PATH", filters.wstring().c_str());
#else
            setenv("MAGICK_HOME",               im_root.string().c_str(), 1);
            setenv("MAGICK_CODER_MODULE_PATH",  coders.string().c_str(),  1);
            setenv("MAGICK_FILTER_MODULE_PATH", filters.string().c_str(), 1);
#endif

            // Pass a plausible argv[0] (full path incl. filename). Not critical for modules,
            // but consistent with the docs.
            const auto argv0 = cbeam::filesystem::get_current_binary_path(true).string();
            Magick::InitializeMagick(argv0.c_str());
            return true;
        }();
        (void)once;
    }

    template <typename T, typename U>
    void CopyFromImageMagickData(const acrion::image::Bitmap& bitmap, const U* pixels)
    {
        T* dest = (T*)bitmap.Buffer();
        {
#ifdef DBG
    #if defined(__clang__) || defined(__GNUC__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
    #endif
            int i = 0;
    #if defined(__clang__) || defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif
#endif
            const T* end = dest + bitmap.Width() * bitmap.Height() * bitmap.Channels();
            while (dest != end)
            {
                *dest++ = (T)*pixels++;
#ifdef DBG
                ++i;
#endif
            }
        }
    }

    std::shared_ptr<acrion::image::Bitmap> Read(const std::wstring& pathToImage, std::string& warning)
    {
        ensure_magick_initialized();
        std::filesystem::path inputPath(pathToImage);
        if (!std::filesystem::exists(pathToImage))
        {
            throw std::runtime_error("acrion::image_framework::Bitmap: input file does not exist: '" + inputPath.string() + "'");
        }
        CBEAM_LOG(L"acrion image framework: Reading '" + pathToImage + L"'...");

        const std::string extension = cbeam::convert::to_lower(inputPath.extension().string());
        const bool        bFits     = extension == ".fits" || extension == ".fit";

#pragma warning(suppress : 4244)
        std::string utf8(inputPath.string()); // TODO pin_ptr<Byte> byteArrayPtr = &(gcnew UTF8Encoding())->GetBytes(s + "\0")[0];

        if (bFits)
        {
            return std::make_shared<acrion::image::Bitmap>(ReadFits(utf8));
        }
        else
        {
            Magick::Image img;

            try
            {
                img.read(utf8);
                CBEAM_LOG("acrion image framework: Successfully read the image");
            }
            catch (Magick::Warning& warningException)
            {
                warning = warningException.what();
                CBEAM_LOG("acrion image framework: Successfully read the image, but ImageMagick reported a warning: '" + warning + "'");
            }
            catch (std::exception& errorException)
            {
                const std::string error(errorException.what());
                CBEAM_LOG("acrion image framework: " + error);
                throw std::runtime_error("Failed to read the image: '" + error + "'");
            }
            catch (...)
            {
                throw std::runtime_error("Unknown error while reading image");
            }

            CBEAM_LOG("acrion image framework: Depth    = " + std::to_string(img.depth())); // 8, 16, 32, 64

            const int channels = static_cast<int>(img.channels());
            const int width    = static_cast<int>(img.columns());
            const int height   = static_cast<int>(img.rows());

            CBEAM_LOG("acrion image framework: Width    = " + std::to_string(width));
            CBEAM_LOG("acrion image framework: Height   = " + std::to_string(height));
            CBEAM_LOG("acrion image framework: Channels = " + std::to_string(channels));
            {
                // img.modifyImage();
                Magick::Pixels         view(img);
                const Magick::Quantum* pixels = view.getConst(0, 0, width, height);

                using namespace Magick;

                if (!pixels)
                {
                    throw std::runtime_error("Unknown error in scope of Magick::Pixels::getConst");
                }

                if (img.depth() != 8 && img.depth() != 16 && img.depth() != 32 && img.depth() != 64)
                {
                    throw std::runtime_error("acrion::imagetools::io::Read(): Unsupported ImageMagick image depth " + std::to_string(img.depth()));
                }

                std::shared_ptr<acrion::image::Bitmap> bitmap = std::make_shared<acrion::image::Bitmap>(width, height, channels, img.depth() / 8);
                switch (img.depth())
                {
                case 8:
                {
                    CopyFromImageMagickData<uint8_t>(*bitmap, pixels);
                    break;
                }
                case 16:
                {
                    CopyFromImageMagickData<uint16_t>(*bitmap, pixels);
                    break;
                }
                case 32:
                {
                    CopyFromImageMagickData<uint32_t>(*bitmap, pixels);
                    break;
                }
                case 64:
                {
                    CopyFromImageMagickData<uint64_t>(*bitmap, pixels);
                    break;
                }
                default:
                    throw std::runtime_error("acrion::image::Bitmap::Read(): Unsupported image depth " + std::to_string(img.modulusDepth()) + " bits.");
                }

                CBEAM_LOG("acrion image framework: Successfully copied image from ImageMagick buffer");
                return bitmap;
            }

            // if (channels > 1 && !ContainsColors())
            // {
            //     // TODO: reduce channels to 1
            // }
        }
    }

    void Write(const acrion::image::Bitmap& bitmap, std::wstring pathToImage, std::string& warning)
    {
        ensure_magick_initialized();
        if (bitmap.Depth() < 0)
        {
            throw std::runtime_error("acrion::image::Bitmap::Write: Please use FITS format for images with floating point type");
        }

        if (bitmap.Depth() > 4)
        {
            throw std::runtime_error("acrion::image::Bitmap::Write: Unsupported image depth " + std::to_string(bitmap.Depth()));
        }

        warning                     = "";
        const std::string extension = cbeam::convert::to_lower(std::filesystem::path(pathToImage).extension().string());
        const bool        bFits     = extension == ".fits" || extension == ".fit";

        CBEAM_LOG(L"acrion image framework: Writing image '" + pathToImage + L"'");

        try
        {
            bool bContainsColors = bitmap.ContainsColors();

            if (bContainsColors)
            {
                if (bFits)
                {
                    throw std::runtime_error("acrion_image_tools::io::Write: Creating colored FITS image files is not supported. Please use TIFF format for this.");
                }
            }
            else if (bitmap.Channels() > 1)
            {
                // TODO: reduce channels to 1
            }

            Magick::Image out;
            out.size(Magick::Geometry(bitmap.Width(), bitmap.Height()));
            out.depth(std::abs(bitmap.Depth()) * 8);

            if (bitmap.Channels() == 1)
            {
                out.magick("gray");
            }
            else if (bitmap.Channels() == 3)
            {
                out.magick("RGB");
            }
            else
            {
                throw std::runtime_error("acrion_image_tools::io::Write: Unsupported number of channels for writing: " + std::to_string(bitmap.Channels()));
            }
            out.read(Magick::Blob(bitmap.Buffer(), bitmap.Width() * bitmap.Channels() * bitmap.Depth() * bitmap.Height()));

            CBEAM_LOG(L"acrion image framework: Now writing image to '" + pathToImage + L"'");
            std::string utf8 = cbeam::convert::to_string(pathToImage);
            std::string ext  = cbeam::convert::to_lower(std::filesystem::path(pathToImage).extension().string());

            // see https://imagemagick.org/script/formats.php#supported
            // and https://www.imagemagick.org/Magick++/Image++.html#BLOBs
            if (ext == ".jpeg" || ext == ".jpg")
            {
                out.magick("JPEG");
            }
            else if (ext == ".tiff" || ext == ".tif")
            {
                out.magick("TIFF");
            }
            else if (ext == ".fit" || ext == ".fits")
            {
                out.magick("FITS");
            }
            else if (ext == ".png")
            {
                out.magick("PNG");
            }
            else if (ext == ".tga")
            {
                out.magick("TGA");
            }
            else if (ext == ".bmp")
            {
                out.magick("BMP");
            }
            else
            {
                throw std::runtime_error("Unsupported image file extension '" + ext + "' for writing");
            }

            out.write(utf8);
        }
        catch (Magick::WarningCoder& warningException)
        {
            warning = warningException.what();
            CBEAM_LOG("acrion image framework: Writing was successful, though ImageMagick reported a warning: '" + warning + "'");
        }
    }
}
