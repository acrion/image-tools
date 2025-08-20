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
#include "acrion/image/bitmap.hpp"
#include "acrion/image/bitmap_data.hpp"

#include <cstdint>
#include <filesystem>

namespace acrion::imagetools
{
    acrion::image::BitmapData<double> ReadFits(const std::filesystem::path& filename);
}
