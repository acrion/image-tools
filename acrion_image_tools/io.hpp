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

#pragma once

#include "acrion_image_tools_export.h"

#include "acrion/image/bitmap.hpp"

#include <memory>
#include <string>

namespace acrion::imagetools::io
{
    ACRION_IMAGE_TOOLS_EXPORT std::shared_ptr<acrion::image::Bitmap> Read(const std::wstring& filePath, std::string& warning);
    ACRION_IMAGE_TOOLS_EXPORT void                                   Write(const acrion::image::Bitmap& bitmap, std::wstring pathToImage, std::string& warning);
}
