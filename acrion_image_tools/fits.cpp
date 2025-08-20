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

#include "fits.hpp"

#include "fitsio.h"

#include <cbeam/container/shared_array.hpp>

#include <limits>
#include <memory>

namespace acrion::imagetools
{
    void ThrowFitsError(int status)
    {
        if (status)
        {
            std::filesystem::path tempFile = std::filesystem::temp_directory_path() / "test";
            fits_report_error(stderr, status);
            throw std::runtime_error("acrion::imagetools::ReadFits: fits error");
        }
    }

    acrion::image::BitmapData<double> ReadFits(const std::filesystem::path& filename)
    {
        fitsfile* fptr;
        int       status = 0;

        if (fits_open_file(&fptr, filename.string().c_str(), READONLY, &status))
        {
            ThrowFitsError(status);
        }

        long   naxes[2] = {1, 1};
        double min      = std::numeric_limits<double>::max();
        double max      = std::numeric_limits<double>::lowest();

        {
            int bitpix, naxis;

            if (fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status))
            {
                fits_close_file(fptr, &status);
                ThrowFitsError(status);
            }

            if (naxis > 2 || naxis == 0)
            {
                throw std::runtime_error("acrion::imagetools::ReadFits: only 1D or 2D images are supported");
            }
        }

        const int width  = (int)naxes[0];
        const int height = (int)naxes[1];

        acrion::image::BitmapData<double> result(width, height, 1);
        auto                              pixels    = cbeam::container::make_shared_array<double>(width);
        long                              fpixel[2] = {1, 1};

        /* loop over all the rows in the image, top to bottom */
        for (fpixel[1] = height; fpixel[1] >= 1; fpixel[1]--)
        {
            if (fits_read_pix(fptr, TDOUBLE, fpixel, naxes[0], nullptr, pixels.get(), nullptr, &status)) /* read row of pixels */
            {
                fits_close_file(fptr, &status);
                ThrowFitsError(status);
            }

            const int row = (int)fpixel[1] - 1;

            for (int column = 0; column < width; ++column)
            {
                const double value = pixels.get()[column];
                max                = std::max(max, value);
                min                = std::min(min, value);
                result.Plot(column, row, value);
            }
        }
        fits_close_file(fptr, &status);

        ThrowFitsError(status);

        // TODO read scale and zero from FITS header and make these parameters available in BitmapData
        result.SetBrightnessRangeForDisplay(min, max);

        return result;
    }
}
