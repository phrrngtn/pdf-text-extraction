#pragma once

#include "PDFObjectCast.h"
#include "PDFIndirectObjectReference.h"

#include "../interpreter/IPDFRecursiveInterpreterHandler.h"

#include "TextPlacement.h"
#include "TPCollectionState.h"

#include <string>
#include <map>
#include <list>

struct GSState {
    GSState(RefCountPtr<PDFObject> inRef, double inSize) {
        fontRef = inRef;
        fontSize = inSize;
    }

    RefCountPtr<PDFObject> fontRef;
    double fontSize;
};


typedef std::map<std::string, GSState> StringToGStateMap;


struct Font {
    Font(RefCountPtr<PDFObject> inRef) {
        fontRef = inRef;
    }

    RefCountPtr<PDFObject> fontRef;
};

typedef std::map<std::string, Font> StringToFontMap;

struct Resources {
    StringToGStateMap gStates;
    StringToFontMap fonts;
};

typedef std::list<Resources> ResourcesList;

class TextPlacementsCollector: public IPDFRecursiveInterpreterHandler {
public:
    TextPlacementsCollector();
    virtual ~TextPlacementsCollector();


    PlacedTextOperationWithEnvList& onDone(); // finished notification from our benefactors. for cleanup and result return

    // IPDFRecursiveInterpreterHandler implementation
    virtual bool onOperation(const std::string& inOperation,  const PDFObjectVector& inOperands);

    virtual bool onResourcesRead(IInterpreterContext* inContext);
    virtual void onXObjectDoEnd(
        const std::string& inXObjectRefName,
        ObjectIDType inXObjectObjectID,
        PDFStreamInput* inXObject,
        PDFParser* inParser);

private:
    TPCollectionState state;
    ResourcesList resourcesStack;


    void Tc(double inCharSpace);
    void Tw(double inWordSpace);
    void TL(double inLeading);
    void Td(double inX, double inY);
    void setTm(const double (&matrix)[6]);
    void TStar();
    ByteList ToBytesList(PDFObject* inObject);
    void Quote(PDFObject* inObject);

    void textPlacement(const PlacedTextOperation& inTextPlacementOperation);
    void textPlacement(const PlacedTextOperationList& inTextPlacementOperations);


};
