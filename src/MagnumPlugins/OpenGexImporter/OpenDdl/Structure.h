#ifndef Magnum_OpenDdl_Structure_h
#define Magnum_OpenDdl_Structure_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/** @file
 * @brief Class @ref Magnum::OpenDdl::Structure, enum @ref Magnum::OpenDdl::Type
 */

#include <functional>
#include <Corrade/Utility/Assert.h>

#include "MagnumPlugins/OpenGexImporter/OpenDdl/Document.h"

namespace Magnum { namespace OpenDdl {

/**
@brief OpenDDL structure

See @ref Document for more information.

@attention The class consists just of reference to internal data in originating
    @ref Document instance, thus you must ensure that the document is available
    for whole instance lifetime. On the other hand you can copy the instance
    however you like without worrying about performance.
*/
class Structure {
    friend Document;

    public:
        /**
         * @brief Whether the structure is custom
         *
         * @see @ref type(), @ref identifier()
         */
        bool isCustom() const { return type() == Type::Custom; }

        /**
         * @brief Structure type
         *
         * @see @ref isCustom(), @ref identifier()
         */
        Type type() const { return std::min(Type::Custom, _data.get().primitive.type); }

        /**
         * @brief Custom structure identifier
         *
         * The structure must be custom.
         * @see @ref isCustom(), @ref UnknownIdentifier
         */
        Int identifier() const;

        /** @brief Structure name */
        const std::string& name() const { return _document.get()._strings[_data.get().name]; }

        /**
         * @brief Array size
         *
         * The structure must not be custom.
         * @see @ref isCustom()
         */
        std::size_t arraySize() const;

        /**
         * @brief Subarray size
         *
         * The structure must not be custom.
         * @see @ref isCustom()
         */
        std::size_t subArraySize() const;

        /**
         * @brief Structure data
         *
         * The structure must not be custom, must be of corresponding type and
         * the array must have exactly one item.
         * @see @ref isCustom(), @ref type(), @ref arraySize(), @ref asArray()
         */
        template<class T>
        #ifndef DOXYGEN_GENERATING_OUTPUT
        typename Implementation::ReturnTypeFor<T>::Type
        #else
        const T&
        #endif
        as() const;

        /**
         * @brief Structure data array
         *
         * The structure must not be custom and must be of corresponding type.
         * @see @ref isCustom(), @ref type(), @ref subArraySize(), @ref as()
         */
        template<class T> Containers::ArrayReference<const T> asArray() const;

        /**
         * @brief Find next sibling structure
         *
         * Returns `std::nullopt` if the structure is last in given level.
         * @see @ref findNextOf(), @ref firstChild()
         */
        std::optional<Structure> findNext() const {
            return _data.get().next ? std::make_optional(Structure{_document, _document.get()._structures[_data.get().next]}) : std::nullopt;
        }

        /**
         * @brief Find next custom sibling structure of given identifier
         *
         * Returns `std::nullopt` if there is no such structure.
         * @see @ref findNext(), @ref findFirstChildOf()
         */
        std::optional<Structure> findNextOf(Int identifier) const;

        /**
         * @brief Whether the structure has properties
         *
         * The structure must be custom.
         * @see @ref isCustom()
         */
        bool hasProperties() const { return propertyCount(); }

        /**
         * @brief Property count
         *
         * The structure must be custom.
         * @see @ref isCustom()
         */
        Int propertyCount() const;

        /**
         * @brief Find custom structure property of given identifier
         *
         * The structure must be custom. Returns `std::nullopt` if the
         * structure doesn't contain any property of given identifier.
         * @see @ref isCustom(), @ref propertyOf()
         */
        std::optional<Property> findPropertyOf(Int identifier) const;

        /**
         * @brief Custom structure property of given identifier
         *
         * The structure must be custom and there must be such property.
         * @see @ref isCustom(), @ref findPropertyOf()
         */
        Property propertyOf(Int identifier) const;

        /**
         * @brief Whether the structure has children
         *
         * The structure must be custom.
         * @see @ref isCustom()
         */
        bool hasChildren() const;

        /**
         * @brief Find first child structure
         *
         * The structure must be custom. Returns `std::nullopt` if the
         * structure has no children.
         * @see @ref isCustom(), @ref firstChild(), @ref findNext(),
         *      @ref findFirstChildOf()
         */
        std::optional<Structure> findFirstChild() const;

        /**
         * @brief First child structure
         *
         * The structure must be custom and must have at least one child.
         * @see @ref isCustom(), @ref hasChildren(), @ref findFirstChild(),
         *      @ref firstChildOf()
         */
        Structure firstChild() const;

        /**
         * @brief Find first custom child structure of given type
         *
         * The structure must be custom. Returns `std::nullopt` if there is no
         * such structure.
         * @see @ref isCustom(), @ref firstChildOf()
         */
        std::optional<Structure> findFirstChildOf(Type type) const;

        /**
         * @brief Find first custom child structure of given identifier
         *
         * The structure must be custom. Returns `std::nullopt` if there is no
         * such structure.
         * @see @ref isCustom(), @ref firstChildOf(), @ref findNextOf()
         */
        std::optional<Structure> findFirstChildOf(Int identifier) const;

        /**
         * @brief First custom child structure of given type
         *
         * The structure must be custom and there must be such child structure.
         * @see @ref isCustom(), @ref Document::validate(),
         *      @ref findFirstChildOf()
         */
        Structure firstChildOf(Type type) const;

        /**
         * @brief First custom child structure of given identifier
         *
         * The structure must be custom and there must be such child structure.
         * @see @ref isCustom(), @ref findFirstChildOf()
         */
        Structure firstChildOf(Int identifier) const;

    private:
        explicit Structure(const Document& document, const Document::StructureData& data) noexcept: _document{document}, _data{data} {}

        std::reference_wrapper<const Document> _document;
        std::reference_wrapper<const Document::StructureData> _data;
};

namespace Implementation {
    template<class> bool isStructureType(Type);
    template<> inline bool isStructureType<bool>(Type type) { return type == Type::Bool; }
    template<> inline bool isStructureType<std::string>(Type type) {
        return type == Type::String || type == Type::Reference;
    }
    #ifndef DOXYGEN_GENERATING_OUTPUT
    #define _c(T) \
        template<> inline bool isStructureType<T>(Type type) { return type == Type::T; }
    _c(UnsignedByte)
    _c(Byte)
    _c(UnsignedShort)
    _c(Short)
    _c(UnsignedInt)
    _c(Int)
    #ifndef MAGNUM_TARGET_WEBGL
    _c(UnsignedLong)
    _c(Long)
    #endif
    /** @todo Half */
    _c(Float)
    #ifndef MAGNUM_TARGET_GLES
    _c(Double)
    #endif
    #undef _c
    #endif
}

template<class T>
#ifndef DOXYGEN_GENERATING_OUTPUT
typename Implementation::ReturnTypeFor<T>::Type
#else
const T&
#endif
Structure::as() const {
    CORRADE_ASSERT(arraySize() == 1,
        "OpenDdl::Structure::as(): not a single value", _document.get().data<T>().front());
    CORRADE_ASSERT(Implementation::isStructureType<T>(type()),
        "OpenDdl::Structure::as(): not of given type", _document.get().data<T>().front());
    return _document.get().data<T>()[_data.get().primitive.begin];
}

template<class T> Containers::ArrayReference<const T> Structure::asArray() const {
    CORRADE_ASSERT(Implementation::isStructureType<T>(type()),
        "OpenDdl::Structure::asArray(): not of given type", nullptr);
    return {&_document.get().data<T>()[0] + _data.get().primitive.begin, _data.get().primitive.size};
}

}}

#endif