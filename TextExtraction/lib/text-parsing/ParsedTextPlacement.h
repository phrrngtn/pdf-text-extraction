#pragma once

#include "../math/Transformations.h"

#include "ObjectsBasicTypes.h"

#include <string>
#include <list>

struct ParsedTextPlacement {
    ParsedTextPlacement(
        const std::string& inText,
        ObjectIDType inFontID,
        const double (&inMatrix)[6],
        const double (&inLocalBox)[4],
        const double (&inGlobalBox)[4],
        const double inSpaceWidth,
        const double (&inGlobalSpaceWidth)[2]
    ) {
        text = inText;
        fontID = inFontID;
        CopyMatrix(inMatrix, matrix);
        CopyBox(inLocalBox, localBbox);
        CopyBox(inGlobalBox, globalBbox);
        spaceWidth = inSpaceWidth;
        CopyVector(inGlobalSpaceWidth, globalSpaceWidth);
    }

    std::string text;
    ObjectIDType fontID;
    double matrix[6];
    double localBbox[4];
    double globalBbox[4];
    double spaceWidth;
    double globalSpaceWidth[2];
};


typedef std::list<ParsedTextPlacement> ParsedTextPlacementList;
