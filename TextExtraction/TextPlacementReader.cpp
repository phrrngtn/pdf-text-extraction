#include "TextPlacementReader.h"
#include "TextExtraction.h"

#include "InputFile.h"
#include "PDFParser.h"
#include "IByteReaderWithPosition.h"

#include <cstring>

using namespace PDFHummus;

/**
 * Simple adapter to read from a memory buffer.
 * Implements IByteReaderWithPosition interface from PDFHummus.
 */
class MemoryByteReader : public IByteReaderWithPosition {
public:
    MemoryByteReader(const char* data, size_t length)
        : data_(data), length_(length), position_(0) {}

    virtual ~MemoryByteReader() {}

    // IByteReader interface
    virtual IOBasicTypes::LongBufferSizeType Read(IOBasicTypes::Byte* inBuffer, IOBasicTypes::LongBufferSizeType inBufferSize) override {
        if (position_ >= length_) {
            return 0;
        }
        size_t bytesToRead = static_cast<size_t>(inBufferSize);
        if (position_ + bytesToRead > length_) {
            bytesToRead = length_ - position_;
        }
        std::memcpy(inBuffer, data_ + position_, bytesToRead);
        position_ += bytesToRead;
        return static_cast<IOBasicTypes::LongBufferSizeType>(bytesToRead);
    }

    virtual bool NotEnded() override {
        return position_ < length_;
    }

    // IByteReaderWithPosition interface
    virtual void SetPosition(IOBasicTypes::LongFilePositionType inOffsetFromStart) override {
        if (static_cast<size_t>(inOffsetFromStart) <= length_) {
            position_ = static_cast<size_t>(inOffsetFromStart);
        }
    }

    virtual void SetPositionFromEnd(IOBasicTypes::LongFilePositionType inOffsetFromEnd) override {
        if (static_cast<size_t>(inOffsetFromEnd) <= length_) {
            position_ = length_ - static_cast<size_t>(inOffsetFromEnd);
        }
    }

    virtual IOBasicTypes::LongFilePositionType GetCurrentPosition() override {
        return static_cast<IOBasicTypes::LongFilePositionType>(position_);
    }

    virtual void Skip(IOBasicTypes::LongBufferSizeType inSkipSize) override {
        size_t newPos = position_ + static_cast<size_t>(inSkipSize);
        if (newPos <= length_) {
            position_ = newPos;
        } else {
            position_ = length_;
        }
    }

private:
    const char* data_;
    size_t length_;
    size_t position_;
};

/**
 * Internal implementation details for TextPlacementReader.
 */
struct TextPlacementReader::Impl {
    std::vector<TextPlacement> placements;
    FontInfoMap fontInfoMap;
    size_t pageCount;
    std::vector<uint8_t> blobStorage; // Storage for blob data to keep it alive

    Impl() : pageCount(0) {}
};

// ============================================================================
// TextPlacementReader implementation
// ============================================================================

TextPlacementReader::TextPlacementReader(const std::string& filePath)
    : impl_(std::make_unique<Impl>()) {
    extractFromFile(filePath);
}

TextPlacementReader::TextPlacementReader(const char* data, size_t length)
    : impl_(std::make_unique<Impl>()) {
    extractFromBuffer(data, length);
}

TextPlacementReader::TextPlacementReader(const std::vector<uint8_t>& blob)
    : impl_(std::make_unique<Impl>()) {
    extractFromBuffer(reinterpret_cast<const char*>(blob.data()), blob.size());
}

TextPlacementReader::~TextPlacementReader() = default;

TextPlacementReader::TextPlacementReader(TextPlacementReader&& other) noexcept = default;
TextPlacementReader& TextPlacementReader::operator=(TextPlacementReader&& other) noexcept = default;

void TextPlacementReader::extractFromFile(const std::string& filePath) {
    TextExtraction extractor;
    EStatusCode status = extractor.ExtractText(filePath);

    if (status != eSuccess) {
        std::string errorMsg = "Failed to extract text from PDF";
        if (!extractor.LatestError.description.empty()) {
            errorMsg += ": " + extractor.LatestError.description;
        }
        throw std::runtime_error(errorMsg);
    }

    // Get font info
    impl_->fontInfoMap = extractor.GetFontInfoMap();

    // Convert results to our format
    impl_->pageCount = 0;
    unsigned long pageNum = 0;
    for (const auto& pageTexts : extractor.textsForPages) {
        for (const auto& tp : pageTexts) {
            TextPlacement placement;
            placement.pageNumber = pageNum;
            placement.fontID = tp.fontID;
            // Convert from [x1, y1, x2, y2] to [x, y, width, height]
            placement.bbox[0] = tp.globalBbox[0];
            placement.bbox[1] = tp.globalBbox[1];
            placement.bbox[2] = tp.globalBbox[2] - tp.globalBbox[0];
            placement.bbox[3] = tp.globalBbox[3] - tp.globalBbox[1];
            placement.text = tp.text;
            impl_->placements.push_back(std::move(placement));
        }
        ++pageNum;
    }
    impl_->pageCount = pageNum;
}

void TextPlacementReader::extractFromBuffer(const char* data, size_t length) {
    // Store the data to keep it alive during parsing
    impl_->blobStorage.assign(reinterpret_cast<const uint8_t*>(data),
                              reinterpret_cast<const uint8_t*>(data) + length);

    MemoryByteReader reader(reinterpret_cast<const char*>(impl_->blobStorage.data()),
                            impl_->blobStorage.size());

    TextExtraction extractor;
    EStatusCode status = extractor.ExtractText(&reader);

    if (status != eSuccess) {
        std::string errorMsg = "Failed to extract text from PDF buffer";
        if (!extractor.LatestError.description.empty()) {
            errorMsg += ": " + extractor.LatestError.description;
        }
        throw std::runtime_error(errorMsg);
    }

    // Get font info
    impl_->fontInfoMap = extractor.GetFontInfoMap();

    // Convert results to our format
    impl_->pageCount = 0;
    unsigned long pageNum = 0;
    for (const auto& pageTexts : extractor.textsForPages) {
        for (const auto& tp : pageTexts) {
            TextPlacement placement;
            placement.pageNumber = pageNum;
            placement.fontID = tp.fontID;
            // Convert from [x1, y1, x2, y2] to [x, y, width, height]
            placement.bbox[0] = tp.globalBbox[0];
            placement.bbox[1] = tp.globalBbox[1];
            placement.bbox[2] = tp.globalBbox[2] - tp.globalBbox[0];
            placement.bbox[3] = tp.globalBbox[3] - tp.globalBbox[1];
            placement.text = tp.text;
            impl_->placements.push_back(std::move(placement));
        }
        ++pageNum;
    }
    impl_->pageCount = pageNum;
}

size_t TextPlacementReader::pageCount() const {
    return impl_->pageCount;
}

size_t TextPlacementReader::placementCount() const {
    return impl_->placements.size();
}

const FontInfoMap& TextPlacementReader::fonts() const {
    return impl_->fontInfoMap;
}

nlohmann::json TextPlacementReader::summary_json() const {
    nlohmann::json summary;
    summary["page_count"] = impl_->pageCount;
    summary["placement_count"] = impl_->placements.size();

    nlohmann::json fonts_array = nlohmann::json::array();
    for (const auto& [id, font] : impl_->fontInfoMap) {
        fonts_array.push_back(font);  // Uses ADL to_json for FontInfo
    }
    summary["fonts"] = fonts_array;

    return summary;
}

TextPlacementReader::Iterator TextPlacementReader::begin() const {
    return Iterator(this, 0);
}

TextPlacementReader::Iterator TextPlacementReader::end() const {
    return Iterator(this, impl_->placements.size());
}

TextPlacementReader::PageRange TextPlacementReader::pages(long startPage, long endPage) const {
    return PageRange(this, startPage, endPage);
}

// ============================================================================
// Iterator implementation
// ============================================================================

TextPlacementReader::Iterator::Iterator()
    : extractor_(nullptr), index_(0), startPage_(-1), endPage_(-1) {}

TextPlacementReader::Iterator::Iterator(const TextPlacementReader* extractor, size_t index)
    : extractor_(extractor), index_(index), startPage_(-1), endPage_(-1) {}

TextPlacementReader::Iterator::Iterator(const TextPlacementReader* extractor, size_t index,
                                      long startPage, long endPage)
    : extractor_(extractor), index_(index), startPage_(startPage), endPage_(endPage) {
    advanceToValidPage();
}

void TextPlacementReader::Iterator::advanceToValidPage() {
    if (!extractor_ || startPage_ < 0) return;

    while (index_ < extractor_->impl_->placements.size()) {
        long page = static_cast<long>(extractor_->impl_->placements[index_].pageNumber);
        if (page >= startPage_ && (endPage_ < 0 || page < endPage_)) {
            break;
        }
        if (endPage_ >= 0 && page >= endPage_) {
            // Past the range, go to end
            index_ = extractor_->impl_->placements.size();
            break;
        }
        ++index_;
    }
}

TextPlacementReader::Iterator::reference TextPlacementReader::Iterator::operator*() const {
    return extractor_->impl_->placements[index_];
}

TextPlacementReader::Iterator::pointer TextPlacementReader::Iterator::operator->() const {
    return &extractor_->impl_->placements[index_];
}

TextPlacementReader::Iterator& TextPlacementReader::Iterator::operator++() {
    ++index_;
    if (startPage_ >= 0) {
        advanceToValidPage();
    }
    return *this;
}

TextPlacementReader::Iterator TextPlacementReader::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

bool TextPlacementReader::Iterator::operator==(const Iterator& other) const {
    return extractor_ == other.extractor_ && index_ == other.index_;
}

bool TextPlacementReader::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

// ============================================================================
// PageRange implementation
// ============================================================================

TextPlacementReader::PageRange::PageRange(const TextPlacementReader* extractor,
                                        long startPage, long endPage)
    : extractor_(extractor), startPage_(startPage), endPage_(endPage) {}

TextPlacementReader::Iterator TextPlacementReader::PageRange::begin() const {
    return Iterator(extractor_, 0, startPage_, endPage_);
}

TextPlacementReader::Iterator TextPlacementReader::PageRange::end() const {
    return Iterator(extractor_, extractor_->impl_->placements.size());
}
