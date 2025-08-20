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

#include <cbeam/lifecycle/singleton.hpp>

#include <gtest/gtest.h>

// using namespace acrion::imagetools;

class ImageToolsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
        cbeam::lifecycle::singleton_control::reset();           // delete all singletons (and setting shutdown state to true to avoid further accesses)
        cbeam::lifecycle::singleton_control::set_operational(); // end shutting down state for singletons to enable further tests
    }
};

TEST_F(ImageToolsTest, BasicAssertions)
{
}
