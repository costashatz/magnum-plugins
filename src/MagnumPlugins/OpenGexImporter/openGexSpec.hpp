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

#include "MagnumPlugins/OpenGexImporter/OpenDdl/Validation.h"

#ifndef DOXYGEN_GENERATING_OUTPUT
namespace Magnum { namespace Trade { namespace OpenGex {

/* Version 1.1.1 except for the following:
    - Half types
*/

using namespace OpenDdl;
using namespace OpenDdl::Validation;

enum: Int {
    Animation,
    Atten,
    BoneCountArray,
    BoneIndexArray,
    BoneNode,
    BoneRefArray,
    BoneWeightArray,
    CameraNode,
    CameraObject,
    Clip,
    Color,
    Extension,
    GeometryNode,
    GeometryObject,
    IndexArray,
    Key,
    LightNode,
    LightObject,
    Material,
    MaterialRef,
    Mesh,
    Metric,
    Morph,
    MorphWeight,
    Name,
    Node,
    ObjectRef,
    Param,
    Rotation,
    Scale,
    Skeleton,
    Skin,
    Texture,
    Time,
    Track,
    Transform,
    Translation,
    Value,
    VertexArray
};

#define _c(val) #val
const std::initializer_list<CharacterLiteral> structures{
    _c(Animation),
    _c(Atten),
    _c(BoneCountArray),
    _c(BoneIndexArray),
    _c(BoneNode),
    _c(BoneRefArray),
    _c(BoneWeightArray),
    _c(CameraNode),
    _c(CameraObject),
    _c(Clip),
    _c(Color),
    _c(Extension),
    _c(GeometryNode),
    _c(GeometryObject),
    _c(IndexArray),
    _c(Key),
    _c(LightNode),
    _c(LightObject),
    _c(Material),
    _c(MaterialRef),
    _c(Mesh),
    _c(Metric),
    _c(Morph),
    _c(MorphWeight),
    _c(Name),
    _c(Node),
    _c(ObjectRef),
    _c(Param),
    _c(Rotation),
    _c(Scale),
    _c(Skeleton),
    _c(Skin),
    _c(Texture),
    _c(Time),
    _c(Track),
    _c(Transform),
    _c(Translation),
    _c(Value),
    _c(VertexArray)
};
#undef _c

enum: Int {
    applic,
    attrib,
    begin,
    clip,
    curve,
    end,
    front,
    index,
    key,
    kind,
    lod,
    material,
    morph,
    motion_blur,
    object,
    primitive,
    restart,
    shadow,
    target,
    texcoord,
    two_sided,
    type,
    visible
};

#define _c(val) #val
const std::initializer_list<CharacterLiteral> properties{
    _c(applic),
    _c(attrib),
    _c(begin),
    _c(clip),
    _c(curve),
    _c(end),
    _c(front),
    _c(index),
    _c(key),
    _c(kind),
    _c(lod),
    _c(material),
    _c(morph),
    _c(motion_blur),
    _c(object),
    _c(primitive),
    _c(restart),
    _c(shadow),
    _c(target),
    _c(texcoord),
    _c(two_sided),
    _c(type),
    _c(visible)
};
#undef _c

const Structures rootStructures{
    {BoneNode, {}},
    {CameraNode, {}},
    {CameraObject, {}},
    {Clip, {}},
    {Extension, {}},
    {GeometryNode, {}},
    {GeometryObject, {}},
    {LightNode, {}},
    {LightObject, {}},
    {Material, {}},
    {Metric, {}},
    {Node, {}}
};

const std::initializer_list<Validation::Structure> structureInfo{
    {Animation,         Properties{{clip, PropertyType::UnsignedInt, OptionalProperty},
                                   {begin, PropertyType::Float, OptionalProperty},
                                   {end, PropertyType::Float, OptionalProperty}},
                        Structures{{Track, {1, 0}},
                                   {Extension, {}}}},
    {Atten,             Properties{{kind, PropertyType::String, OptionalProperty},
                                   {curve, PropertyType::String, OptionalProperty}},
                        Structures{{Param, {}},
                                   {Extension, {}}}},
    {BoneCountArray,    Primitives{Type::UnsignedByte,
                                   Type::UnsignedShort,
                                   Type::UnsignedInt,
                                   #ifndef MAGNUM_TARGET_WEBGL
                                   Type::UnsignedLong
                                   #endif
                                   }, 1, 0,
                        Structures{{Extension, {}}}},
    {BoneIndexArray,    Primitives{Type::UnsignedByte,
                                   Type::UnsignedShort,
                                   Type::UnsignedInt,
                                   #ifndef MAGNUM_TARGET_WEBGL
                                   Type::UnsignedLong
                                   #endif
                                   }, 1, 0,
                        Structures{{Extension, {}}}},
    {BoneNode,          Structures{{Name, {0, 1}},
                                   {Transform, {}},
                                   {Translation, {}},
                                   {Rotation, {}},
                                   {Scale, {}},
                                   {Animation, {}},
                                   {Node, {}},
                                   {BoneNode, {}},
                                   {GeometryNode, {}},
                                   {CameraNode, {}},
                                   {LightNode, {}},
                                   {Extension, {}}}},
    {BoneRefArray,      Primitives{Type::Reference}, 1, 0,
                        Structures{{Extension, {}}}},
    {BoneWeightArray,   Primitives{Type::Float}, 1, 0,
                        Structures{{Extension, {}}}},
    {CameraNode,        Structures{{Name, {0, 1}},
                                   {ObjectRef, {1, 1}},
                                   {Transform, {}},
                                   {Translation, {}},
                                   {Rotation, {}},
                                   {Scale, {}},
                                   {Animation, {}},
                                   {Node, {}},
                                   {BoneNode, {}},
                                   {GeometryNode, {}},
                                   {CameraNode, {}},
                                   {LightNode, {}},
                                   {Extension, {}}}},
    {CameraObject,      Structures{{Param, {}},
                                   {Extension, {}}}},
    {Clip,              Properties{{index, PropertyType::UnsignedInt, OptionalProperty}},
                        Structures{{Name, {0, 1}},
                                   {Param, {}},
                                   {Extension, {}}}},
    {Color,             Properties{{attrib, PropertyType::String, RequiredProperty}},
                        Primitives{Type::Float}, 1, 0,
                        Structures{{Extension, {}}}},
    {Extension,         Properties{{applic, PropertyType::String, OptionalProperty},
                                   {type, PropertyType::String, RequiredProperty}},
                        Primitives{Type::Bool,
                                   Type::UnsignedByte,
                                   Type::Byte,
                                   Type::UnsignedShort,
                                   Type::Short,
                                   Type::UnsignedInt,
                                   Type::Int,
                                   #ifndef MAGNUM_TARGET_WEBGL
                                   Type::UnsignedLong,
                                   Type::Long,
                                   #endif
                                   /** @todo Half */
                                   Type::Float,
                                   #ifndef MAGNUM_TARGET_GLES
                                   Type::Double,
                                   #endif
                                   Type::String,
                                   Type::Reference,
                                   Type::Type}, 0, 0,
                        Structures{{Extension, {}}}},
    {GeometryNode,      Properties{{visible, PropertyType::Bool, OptionalProperty},
                                   {shadow, PropertyType::Bool, OptionalProperty},
                                   {motion_blur, PropertyType::Bool, OptionalProperty}},
                        Structures{{Name, {0, 1}},
                                   {ObjectRef, {1, 1}},
                                   {MaterialRef, {}},
                                   {MorphWeight, {}},
                                   {Transform, {}},
                                   {Translation, {}},
                                   {Rotation, {}},
                                   {Scale, {}},
                                   {Animation, {}},
                                   {Node, {}},
                                   {BoneNode, {}},
                                   {GeometryNode, {}},
                                   {CameraNode, {}},
                                   {LightNode, {}},
                                   {Extension, {}}}},
    {GeometryObject,    Properties{{visible, PropertyType::Bool, OptionalProperty},
                                   {shadow, PropertyType::Bool, OptionalProperty},
                                   {motion_blur, PropertyType::Bool, OptionalProperty}},
                        Structures{{Mesh, {1, 0}},
                                   {Morph, {}},
                                   {Extension, {}}}},
    {IndexArray,        Properties{{material, PropertyType::UnsignedInt, OptionalProperty},
                                   #ifndef MAGNUM_TARGET_WEBGL
                                   {restart, PropertyType::UnsignedLong, OptionalProperty},
                                   #else
                                   {restart, PropertyType::UnsignedInt, OptionalProperty},
                                   #endif
                                   {front, PropertyType::String, OptionalProperty}},
                        Primitives{Type::UnsignedByte,
                                   Type::UnsignedShort,
                                   Type::UnsignedInt,
                                   #ifndef MAGNUM_TARGET_WEBGL
                                   Type::UnsignedLong
                                   #endif
                                   }, 1, 0,
                        Structures{{Extension, {}}}},
    {Key,               Properties{{kind, PropertyType::String, OptionalProperty}},
                        Primitives{Type::Float}, 1, 0,
                        Structures{{Extension, {}}}},
    {LightNode,         Properties{{shadow, PropertyType::Bool, OptionalProperty}},
                        Structures{{Name, {0, 1}},
                                   {ObjectRef, {1, 1}},
                                   {Transform, {}},
                                   {Translation, {}},
                                   {Rotation, {}},
                                   {Scale, {}},
                                   {Animation, {}},
                                   {Node, {}},
                                   {BoneNode, {}},
                                   {GeometryNode, {}},
                                   {CameraNode, {}},
                                   {LightNode, {}},
                                   {Extension, {}}}},
    {LightObject,       Properties{{type, PropertyType::String, RequiredProperty},
                                   {shadow, PropertyType::Bool, OptionalProperty}},
                        Structures{{Color, {0, 1}},
                                   {Param, {0, 1}},
                                   {Texture, {0, 1}},
                                   {Atten, {}},
                                   {Extension, {}}}},
    {Material,          Properties{{two_sided, PropertyType::Bool, OptionalProperty}},
                        Structures{{Name, {0, 1}},
                                   {Color, {}},
                                   {Param, {}},
                                   {Texture, {}},
                                   {Extension, {}}}},
    {MaterialRef,       Properties{{index, PropertyType::UnsignedInt, OptionalProperty}},
                        Primitives{Type::Reference}, 1, 1,
                        Structures{{Extension, {}}}},
    {Mesh,              Properties{{lod, PropertyType::UnsignedInt, OptionalProperty},
                                   {primitive, PropertyType::String, OptionalProperty}},
                        Structures{{VertexArray, {1, 0}},
                                   {IndexArray, {}},
                                   {Skin, {0, 1}},
                                   {Extension, {}}}},
    {Metric,            Properties{{key, PropertyType::String, RequiredProperty}},
                        Primitives{Type::Float,
                                   Type::String}, 1, 1,
                        Structures{{Extension, {}}}},
    {Morph,             Properties{{index, PropertyType::UnsignedInt, OptionalProperty}},
                        Structures{{Name, {0, 1}},
                                   {Extension, {}}}},
    {MorphWeight,       Properties{{index, PropertyType::UnsignedInt, OptionalProperty}},
                        Primitives{Type::Float}, 1, 1,
                        Structures{{Extension, {}}}},
    {Name,              Primitives{Type::String}, 1, 1,
                        Structures{{Extension, {}}}},
    {Node,              Structures{{Name, {0, 1}},
                                   {Transform, {}},
                                   {Translation, {}},
                                   {Rotation, {}},
                                   {Scale, {}},
                                   {Animation, {}},
                                   {Node, {}},
                                   {BoneNode, {}},
                                   {GeometryNode, {}},
                                   {CameraNode, {}},
                                   {LightNode, {}},
                                   {Extension, {}}}},
    {ObjectRef,         Primitives{Type::Reference}, 1, 1,
                        Structures{{Extension, {}}}},
    {Param,             Properties{{attrib, PropertyType::String, RequiredProperty}},
                        Primitives{Type::Float}, 1, 1,
                        Structures{{Extension, {}}}},
    {Rotation,          Properties{{kind, PropertyType::String, OptionalProperty},
                                   {object, PropertyType::Bool, OptionalProperty}},
                        Primitives{Type::Float}, 1, 0,
                        Structures{{Extension, {}}}},
    {Scale,             Properties{{kind, PropertyType::String, OptionalProperty},
                                   {object, PropertyType::Bool, OptionalProperty}},
                        Primitives{Type::Float}, 1, 0,
                        Structures{{Extension, {}}}},
    {Skeleton,          Structures{{BoneRefArray, {1, 1}},
                                   {Transform, {1, 1}},
                                   {Extension, {}}}},
    {Skin,              Structures{{Transform, {0, 1}},
                                   {Skeleton, {1, 1}},
                                   {BoneCountArray, {1, 1}},
                                   {BoneIndexArray, {1, 1}},
                                   {BoneWeightArray, {1, 1}},
                                   {Extension, {}}}},
    {Texture,           Properties{{attrib, PropertyType::String, RequiredProperty},
                                   {texcoord, PropertyType::UnsignedInt, OptionalProperty}},
                        Primitives{Type::String}, 1, 1,
                        Structures{{Transform, {}},
                                   {Translation, {}},
                                   {Rotation, {}},
                                   {Scale, {}},
                                   {Animation, {}},
                                   {Extension, {}}}},
    {Time,              Properties{{curve, PropertyType::String, OptionalProperty}},
                        Structures{{Key, {1, 3}},
                                   {Extension, {}}}},
    {Track,             Properties{{target, PropertyType::Reference, RequiredProperty}},
                        Structures{{Time, {1, 1}},
                                   {Value, {1, 1}},
                                   {Extension, {}}}},
    {Transform,         Properties{{object, PropertyType::Bool, OptionalProperty}},
                        Primitives{Type::Float}, 1, 0,
                        Structures{{Extension, {}}}},
    {Translation,       Properties{{kind, PropertyType::String, OptionalProperty},
                                   {object, PropertyType::Bool, OptionalProperty}},
                        Primitives{Type::Float}, 1, 0,
                        Structures{{Extension, {}}}},
    {Value,             Properties{{curve, PropertyType::String, OptionalProperty}},
                        Structures{{Key, {1, 4}},
                                   {Extension, {}}}},
    {VertexArray,       Properties{{attrib, PropertyType::String, RequiredProperty},
                                   {morph, PropertyType::UnsignedInt, OptionalProperty}},
                        Primitives{/** @todo half */
                                   Type::Float,
                                   #ifndef MAGNUM_TARGET_GLES
                                   Type::Double
                                   #endif
                                   }, 1, 0,
                        Structures{{Extension, {}}}}};

}}}
#endif
