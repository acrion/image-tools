--[[
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
]]

local ext = {
    "*.tif *.tiff *.TIF *.TIFF",
    "*.fit *.fits *.FIT *.FITS",
    "*.png *.PNG",
    "*.jpg *.jpeg *.JPG *.JPEG",
    "*.bmp *.BMP",
    "*.tga *.TGA",
    "*.dcm *.DCM" }
local saveExt = "TIFF (" .. ext[1] .. ");; FITS (" .. ext[2] .. ");; PNG (" .. ext[3] .. ");; JPEG (" .. ext[4] .. ");; BMP (" .. ext[5] .. ");; TARGA (" .. ext[6] .. ")"

function CallOpenImageFile(parameters)
    import("acrion_image_tools", "OpenImageFile", "table(const char*)")
    return OpenImageFile(parameters.path)
end

addmessage("CallOpenImageFile", {
    displayname = "Open",
    description = "Open image file (same file left and right)",
    icon = "FileOpen.svg",
    parameters = {
        path = { type = "loadpath" },
        filter = { type = "string", internal = "yes", default = "All supported formats (" .. table.concat(ext, ' ') .. ");; " .. saveExt .. ";; DICOM (" .. ext[7] .. ");; All files (*.*)" }
    } })

function CallSaveImageFile(parameters)
    import("acrion_image_tools", "SaveImageFile", "table(table)")
    return SaveImageFile(parameters)
end

addmessage("CallSaveImageFile", {
    displayname = "Save right",
    description = "Save right image file (opens dialog to choose file name)",
    icon = "FileSave.svg",
    parameters = {
        path = { type = "savepath" },
        filter = { type = "string", internal = "yes", default = saveExt },
        imageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" },
        minBrightness = { type = "double" },
        maxBrightness = { type = "double" }
    } })

function CallSwap(parameters)
    import("acrion_image_tools", "Swap", "table(table)")
    return Swap(parameters)
end

addmessage("CallSwap", {
    displayname = "Swap images",
    description = "Swap left and right image",
    icon = "Swap.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" }
    } })

function CallCopyLeftToRight(parameters)
    import("acrion_image_tools", "CopyLeftToRight", "table(table)")
    return CopyLeftToRight(parameters)
end

addmessage("CallCopyLeftToRight", {
    displayname = "Copy left to right",
    description = "Overwrite right image with left image",
    icon = "CopyLeftToRight.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" }
    } })

function CallCopyRightToLeft(parameters)
    import("acrion_image_tools", "CopyRightToLeft", "table(table)")
    return CopyRightToLeft(parameters)
end

addmessage("CallCopyRightToLeft", {
    displayname = "Copy right to left",
    description = "Overwrite left image with right image",
    icon = "CopyRightToLeft.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" }
    } })

function CallInvertImage(parameters)
    import("acrion_image_tools", "InvertImage", "table(table)")
    return InvertImage(parameters)
end

addmessage("CallInvertImage", {
    displayname = "Invert Image",
    description = "In the right image, replace all pixel values with (max-value)",
    icon = "Invert.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" },
        minBrightness = { type = "double" },
        maxBrightness = { type = "double" }
    } })

function CallSubtractLeftRightNoWrap(parameters)
    import("acrion_image_tools", "SubtractWorkingImageFromReference", "table(table,long long)")
    return SubtractWorkingImageFromReference(parameters, 0)
end

addmessage("CallSubtractLeftRightNoWrap", {
    displayname = "left-right",
    description = "Subtract working image from reference (map negative values to 0)",
    icon = "Arithmetic.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" }
    } })

function CallSubtractRightLeftNoWrap(parameters)
    import("acrion_image_tools", "SubtractReferenceFromWorkingImage", "table(table,long long)")
    return SubtractReferenceFromWorkingImage(parameters, 0)
end

addmessage("CallSubtractRightLeftNoWrap", {
    displayname = "right-left",
    description = "Subtract reference from working image (map negative values to 0)",
    icon = "Arithmetic.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" }
    } })

function CallSubtractLeftRightWrap(parameters)
    import("acrion_image_tools", "SubtractWorkingImageFromReference", "table(table,long long)")
    return SubtractWorkingImageFromReference(parameters, 1)
end

addmessage("CallSubtractLeftRightWrap", {
    displayname = "left-right (wrap)",
    description = "Subtract working image from reference, wrap at min/max brightness (click twice to undo)",
    icon = "Arithmetic.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" }
    } })

function CallSubtractRightLeftWrap(parameters)
    import("acrion_image_tools", "SubtractReferenceFromWorkingImage", "table(table,long long)")
    return SubtractReferenceFromWorkingImage(parameters, 1)
end

addmessage("CallSubtractRightLeftWrap", {
    displayname = "right-left (wrap)",
    description = "Subtract reference from working image, wrap at min/max brightness",
    icon = "Arithmetic.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" }
    } })

function CallSubtractLeftRightAbs(parameters)
    import("acrion_image_tools", "SubtractWorkingImageFromReference", "table(table,long long)")
    return SubtractWorkingImageFromReference(parameters, 2)
end

addmessage("CallSubtractLeftRightAbs", {
    displayname = "Difference",
    description = "Calculate absolute difference of the two images (just like menu View --> Overlay Diff)",
    icon = "Arithmetic.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" }
    } })

function CallSubtractLeftRightLua(parameters)
    local size = parameters.width * parameters.height * parameters.channels
    local depth = parameters.depth
    local imageBuffer = touserdata(parameters.imageBuffer)
    local referenceImageBuffer = touserdata(parameters.referenceImageBuffer)
    for i = 0, size do
        local addressRight = addoffset(imageBuffer, i, depth)
        local addressLeft = addoffset(referenceImageBuffer, i, depth)
        local val = peek(addressLeft, depth) - peek(addressRight, depth)
        poke(addressRight, val, depth)
    end

    return {}
end

addmessage("CallSubtractLeftRightLua", {
    displayname = "left-right (Lua)",
    description = "Subtract working image from reference using Lua (click twice to undo)",
    icon = "",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" }
    } })

function CallSubtractRightLeftLua(parameters)
    local size = parameters.width * parameters.height * parameters.channels
    local depth = parameters.depth
    local imageBuffer = touserdata(parameters.imageBuffer)
    local referenceImageBuffer = touserdata(parameters.referenceImageBuffer)
    for i = 0, size do
        local addressRight = addoffset(imageBuffer, i, depth)
        local addressLeft = addoffset(referenceImageBuffer, i, depth)
        local val = peek(addressRight, depth) - peek(addressLeft, depth)
        poke(addressRight, val, depth)
    end

    return {}
end

addmessage("CallSubtractRightLeftLua", {
    displayname = "right-left (Lua)",
    description = "Subtract reference from working image using Lua",
    icon = "",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" }
    } })

function DrawWhitePixel(parameters)
    local width = parameters.width
    local height = parameters.height
    local x = parameters.x
    local y = parameters.y
    local channels = parameters.channels
    local depth = parameters.depth
    local maxBrightness = parameters.maxBrightness
    local imageBuffer = touserdata(parameters.imageBuffer)
    local address = (y * width + x) * channels
    for channel = 0, (channels - 1) do
        poke(addoffset(imageBuffer, address + channel, depth), maxBrightness, depth)
    end
    return {} -- TODO add a return value containing a string defining the invalidation: parameter.x .. " " .. parameter.y .. " 1 1"
end

addmessage("DrawWhitePixel", {
    displayname = "Draw white pixel",
    description = "Draw a white pixel",
    icon = "SetPixel.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" },
        x = { type = "long long" },
        y = { type = "long long" },
        maxBrightness = { type = "double" } }
})

function GetPixelValueOfChannel(parameters)
    local width = parameters.width
    local height = parameters.height
    local channels = parameters.channels
    local depth = parameters.depth
    local x = parameters.x
    local y = parameters.y
    local imageBuffer = touserdata(parameters.imageBuffer)
    local address = addoffset(imageBuffer, (y * width + x) * channels, depth)
    local val = "Pixel value at " .. x .. "/" .. y .. ": "
    for channel = 0, (channels - 1) do
        if channel > 0 then
            val = val .. ", "
        end
        val = val .. tostring(peek(addoffset(address, channel, depth), depth))
    end
    return { message = val }
end

addmessage("GetPixelValueOfChannel", {
    displayname = "Get pixel value",
    description = "Get pixel values of all channels at the selected coordinates",
    icon = "GetPixel.svg",
    parameters = {
        imageBuffer = { type = "void*" },
        referenceImageBuffer = { type = "void*" },
        width = { type = "long long" },
        height = { type = "long long" },
        channels = { type = "long long" },
        depth = { type = "long long" },
        x = { type = "long long" },
        y = { type = "long long" } }
})

-- function TestMouseEvents(parameters)
--     log("isRightImage=" .. tostring(parameters.isRightImage) .. ", mouseX/Y=" .. parameters.mouseX .. "/" .. parameters.mouseY .. ", lb=" .. tostring(parameters.mouseLeftButtonPressed) .. ", rb=" .. tostring(parameters.mouseRightButtonPressed) .. ", mb=" .. tostring(parameters.mouseMiddleButtonPressed) .. ", wheelUp=" .. tostring(parameters.mouseWheelUp) .. ", wheelDown=" .. tostring(parameters.mouseWheelDown))
--     return {}
-- end
--
-- addmessage("TestMouseEvents", {
--     parameters = {
--         requestUserInput = { }
--     }
-- })
