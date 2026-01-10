#pragma once

#include "lib/font-translation/FontDecoder.h"
#include "ObjectsBasicTypes.h"

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <stdexcept>
#include <memory>

#include <nlohmann/json.hpp>

// ADL-based JSON serialization for FontInfo (defined in FontDecoder.h)
inline void to_json(nlohmann::json& j, const FontInfo& f) {
    j = nlohmann::json{
        {"font_id", f.fontID},
        {"font_name", f.fontName},
        {"family_name", f.familyName},
        {"font_stretch", f.fontStretch},
        {"font_weight", f.fontWeight},
        {"font_flags", f.fontFlags},
        {"ascent", f.ascent},
        {"descent", f.descent},
        {"space_width", f.spaceWidth}
    };
}

/**
 * TextPlacement represents a single text placement in a PDF document.
 * Each placement contains the text content, its position (bounding box),
 * the page it appears on, and the font used.
 */
struct TextPlacement {
    unsigned long pageNumber;    // 0-indexed page number
    ObjectIDType fontID;         // Font identifier (can be used to look up FontInfo)
    double bbox[4];              // Bounding box as [x, y, width, height] in page coordinates
    std::string text;            // The text content (UTF-8 encoded)

    /**
     * Convert to JSON object.
     */
    nlohmann::json to_json() const {
        return nlohmann::json{
            {"page", pageNumber},
            {"font_id", fontID},
            {"x", bbox[0]},
            {"y", bbox[1]},
            {"width", bbox[2]},
            {"height", bbox[3]},
            {"text", text}
        };
    }
};

// ADL-based JSON serialization for nlohmann::json
inline void to_json(nlohmann::json& j, const TextPlacement& tp) {
    j = tp.to_json();
}

/**
 * TextPlacementReader provides a clean, iterator-based API for extracting
 * text placements with bounding boxes from PDF documents.
 *
 * Designed for easy binding to Python (nanobind) and DuckDB extensions.
 *
 * Example usage:
 *
 *   // From file
 *   TextPlacementReader pdf("document.pdf");
 *
 *   // From blob
 *   std::vector<uint8_t> blob = loadFromDatabase();
 *   TextPlacementReader pdf(blob.data(), blob.size());
 *
 *   // Get page count
 *   std::cout << "Pages: " << pdf.pageCount() << std::endl;
 *
 *   // Iterate all placements
 *   for (const auto& tp : pdf) {
 *       std::cout << "Page " << tp.pageNumber << ": " << tp.text << std::endl;
 *   }
 *
 *   // Iterate specific page range (pages 5-10)
 *   for (const auto& tp : pdf.pages(5, 10)) {
 *       // ...
 *   }
 */
class TextPlacementReader {
public:
    // Forward declarations
    class Iterator;
    class PageRange;

    /**
     * Construct from a file path.
     * @param filePath Path to the PDF file
     * @throws std::runtime_error if the file cannot be opened or parsed
     */
    explicit TextPlacementReader(const std::string& filePath);

    /**
     * Construct from a memory buffer (blob).
     * @param data Pointer to the PDF data
     * @param length Length of the data in bytes
     * @throws std::runtime_error if the data cannot be parsed as a PDF
     */
    TextPlacementReader(const char* data, size_t length);

    /**
     * Construct from a vector of bytes.
     * @param blob Vector containing the PDF data
     * @throws std::runtime_error if the data cannot be parsed as a PDF
     */
    explicit TextPlacementReader(const std::vector<uint8_t>& blob);

    ~TextPlacementReader();

    // Non-copyable but movable
    TextPlacementReader(const TextPlacementReader&) = delete;
    TextPlacementReader& operator=(const TextPlacementReader&) = delete;
    TextPlacementReader(TextPlacementReader&& other) noexcept;
    TextPlacementReader& operator=(TextPlacementReader&& other) noexcept;

    /**
     * Get the number of pages in the document.
     */
    size_t pageCount() const;

    /**
     * Get the total number of text placements in the document.
     */
    size_t placementCount() const;

    /**
     * Get font information for all fonts used in the document.
     * @return Map from font ID to FontInfo
     */
    const FontInfoMap& fonts() const;

    /**
     * Get document summary as JSON.
     * Returns an object with page_count, placement_count, and fonts array.
     */
    nlohmann::json summary_json() const;

    /**
     * Get an iterator to the beginning of all text placements.
     */
    Iterator begin() const;

    /**
     * Get an iterator to the end of all text placements.
     */
    Iterator end() const;

    /**
     * Get a range adapter for iterating over a specific page range.
     * @param startPage First page to include (0-indexed)
     * @param endPage One past the last page to include. Use -1 for end of document.
     * @return PageRange object that can be used in range-based for loops
     */
    PageRange pages(long startPage, long endPage = -1) const;

    /**
     * Standard forward iterator over TextPlacement objects.
     */
    class Iterator {
    public:
        using value_type = TextPlacement;
        using reference = const TextPlacement&;
        using pointer = const TextPlacement*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        Iterator();
        Iterator(const TextPlacementReader* extractor, size_t index);
        Iterator(const TextPlacementReader* extractor, size_t index, long startPage, long endPage);

        reference operator*() const;
        pointer operator->() const;
        Iterator& operator++();
        Iterator operator++(int);
        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;

    private:
        const TextPlacementReader* extractor_;
        size_t index_;
        long startPage_;
        long endPage_;

        void advanceToValidPage();
    };

    /**
     * Range adapter for page-filtered iteration.
     */
    class PageRange {
    public:
        PageRange(const TextPlacementReader* extractor, long startPage, long endPage);

        Iterator begin() const;
        Iterator end() const;

    private:
        const TextPlacementReader* extractor_;
        long startPage_;
        long endPage_;
    };

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    void extractFromFile(const std::string& filePath);
    void extractFromBuffer(const char* data, size_t length);
};
