/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   blackTexture_jpg;
    const int            blackTexture_jpgSize = 75789;

    extern const char*   redTexture_png;
    const int            redTexture_pngSize = 137180;

    extern const char*   blackMetal_jpg;
    const int            blackMetal_jpgSize = 31507;

    extern const char*   metalKnob_eps;
    const int            metalKnob_epsSize = 1948838;

    extern const char*   metalKnob_png;
    const int            metalKnob_pngSize = 436566;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 5;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
