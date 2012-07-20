/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "ColladaImporter.h"

#include <QtCore/QFile>
#include <QtCore/QStringList>

#include "Utility/Directory.h"
#include "SizeTraits.h"
#include "Trade/PhongMaterialData.h"
#include "Trade/MeshData.h"
#include "Trade/MeshObjectData.h"
#include "Trade/SceneData.h"
#include "TgaImporter/TgaImporter.h"

using namespace std;
using namespace Corrade::Utility;
using namespace Corrade::PluginManager;

namespace Magnum { namespace Trade { namespace ColladaImporter {

const QString ColladaImporter::namespaceDeclaration =
    "declare default element namespace \"http://www.collada.org/2005/11/COLLADASchema\";\n";

ColladaImporter::ColladaImporter(AbstractPluginManager* manager, const string& plugin): AbstractImporter(manager, plugin), d(0), zero(0), app(qApp ? 0 : new QCoreApplication(zero, 0)) {}

ColladaImporter::~ColladaImporter() {
    close();
    delete app;
}

ColladaImporter::Document::~Document() {
    for(auto i: meshes) delete i;
    for(auto i: materials) delete i;
}

bool ColladaImporter::open(const string& filename) {
    /* Close previous document */
    if(d) close();

    QXmlQuery query;

    /* Open the file and load it into XQuery */
    QFile file(QString::fromStdString(filename));
    if(!file.open(QIODevice::ReadOnly)) {
        Error() << "ColladaImporter: cannot open file" << filename;
        return false;
    }
    if(!query.setFocus(&file)) {
        Error() << "ColladaImporter: cannot load XML";
        return false;
    }

    QString tmp;

    /* Check namespace */
    query.setQuery("namespace-uri(/*:COLLADA)");
    query.evaluateTo(&tmp);
    tmp = tmp.trimmed();
    if(tmp != "http://www.collada.org/2005/11/COLLADASchema") {
        Error() << "ColladaImporter: unsupported namespace" << ('"'+tmp+'"').toStdString();
        return false;
    }

    /* Check version */
    query.setQuery(namespaceDeclaration + "/COLLADA/@version/string()");
    query.evaluateTo(&tmp);
    tmp = tmp.trimmed();
    if(tmp != "1.4.1") {
        Error() << "ColladaImporter: unsupported version" << ('"'+tmp+'"').toStdString();
        return false;
    }

    /* Scenes */
    query.setQuery(namespaceDeclaration + "count(/COLLADA/library_visual_scenes/visual_scene)");
    query.evaluateTo(&tmp);
    GLuint sceneCount = ColladaType<GLuint>::fromString(tmp);

    /* Objects */
    query.setQuery(namespaceDeclaration + "count(/COLLADA/library_visual_scenes/visual_scene//node)");
    query.evaluateTo(&tmp);
    GLuint objectCount = ColladaType<GLuint>::fromString(tmp);

    QStringList tmpList;

    /* Create camera name -> camera id map */
    query.setQuery(namespaceDeclaration + "/COLLADA/library_cameras/camera/@id/string()");
    query.evaluateTo(&tmpList);
    std::unordered_map<std::string, unsigned int> camerasForName;
    for(const QString id: tmpList)
        camerasForName.insert(make_pair(id.trimmed().toStdString(), camerasForName.size()));

    /* Create light name -> light id map */
    query.setQuery(namespaceDeclaration + "/COLLADA/library_lights/light/@id/string()");
    tmpList.clear();
    query.evaluateTo(&tmpList);
    std::unordered_map<std::string, unsigned int> lightsForName;
    for(const QString id: tmpList)
        lightsForName.insert(make_pair(id.trimmed().toStdString(), lightsForName.size()));

    /* Create material name -> material id map */
    query.setQuery(namespaceDeclaration + "/COLLADA/library_materials/material/@id/string()");
    tmpList.clear();
    query.evaluateTo(&tmpList);
    std::unordered_map<std::string, unsigned int> materialsForName;
    for(const QString id: tmpList)
        materialsForName.insert(make_pair(id.trimmed().toStdString(), materialsForName.size()));

    /* Create mesh name -> mesh id map */
    query.setQuery(namespaceDeclaration + "/COLLADA/library_geometries/geometry/@id/string()");
    tmpList.clear();
    query.evaluateTo(&tmpList);
    std::unordered_map<std::string, unsigned int> meshesForName;
    for(const QString id: tmpList)
        meshesForName.insert(make_pair(id.trimmed().toStdString(), meshesForName.size()));

    /* Create image name -> image id map */
    query.setQuery(namespaceDeclaration + "/COLLADA/library_images/image/@id/string()");
    tmpList.clear();
    query.evaluateTo(&tmpList);
    std::unordered_map<std::string, unsigned int> images2DForName;
    for(const QString id: tmpList)
        images2DForName.insert(make_pair(id.trimmed().toStdString(), images2DForName.size()));

    d = new Document(sceneCount, objectCount, move(camerasForName), move(lightsForName), move(meshesForName), move(materialsForName), move(images2DForName));
    d->filename = filename;
    d->query = query;

    return true;
}

void ColladaImporter::close() {
    if(!d) return;

    delete d;
    d = 0;
}

int ColladaImporter::ColladaImporter::defaultScene() {
    if(!d || d->scenes.empty()) return -1;
    if(!d->scenes[0]) parseScenes();

    return d->defaultScene;
}

SceneData* ColladaImporter::ColladaImporter::scene(unsigned int id) {
    if(!d || id >= d->scenes.size()) return nullptr;
    if(!d->scenes[0]) parseScenes();

    return d->scenes[id];
}

int ColladaImporter::ColladaImporter::objectForName(const string& name) {
    if(d->scenes.empty()) return -1;
    if(!d->scenes[0]) parseScenes();

    auto it = d->objectsForName.find(name);
    return (it == d->objectsForName.end()) ? -1 : it->second;
}

ObjectData* ColladaImporter::ColladaImporter::object(unsigned int id) {
    if(!d || id >= d->objects.size()) return nullptr;
    if(!d->scenes[0]) parseScenes();

    return d->objects[id];
}

int ColladaImporter::ColladaImporter::meshForName(const string& name) {
    auto it = d->meshesForName.find(name);
    return (it == d->meshesForName.end()) ? -1 : it->second;
}

MeshData* ColladaImporter::mesh(unsigned int id) {
    if(!d || id >= d->meshes.size()) return nullptr;
    if(d->meshes[id]) return d->meshes[id];

    QString tmp;

    /* Get mesh name */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_geometries/geometry[%0]/@id/string()").arg(id+1));
    d->query.evaluateTo(&tmp);
    string name(tmp.trimmed().toStdString());

    /** @todo More polylists in one mesh */

    /* Get polygon count */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_geometries/geometry[%0]/mesh/polylist/@count/string()").arg(id+1));
    d->query.evaluateTo(&tmp);
    GLuint polygonCount = ColladaType<GLuint>::fromString(tmp);

    /* Get vertex count per polygon */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_geometries/geometry[%0]/mesh/polylist/vcount/string()").arg(id+1));
    d->query.evaluateTo(&tmp);
    vector<GLuint> vertexCountPerFace = Utility::parseArray<GLuint>(tmp, polygonCount);

    GLuint vertexCount = 0;
    vector<GLuint> quads;
    for(size_t i = 0; i != vertexCountPerFace.size(); ++i) {
        GLuint count = vertexCountPerFace[i];

        if(count == 4) quads.push_back(i);
        else if(count != 3) {
            Error() << "ColladaImporter:" << count << "vertices per face not supported";
            return nullptr;
        }

        vertexCount += count;
    }

    /* Get input count per vertex */
    d->query.setQuery((namespaceDeclaration + "count(/COLLADA/library_geometries/geometry[%0]/mesh/polylist/input)").arg(id+1));
    d->query.evaluateTo(&tmp);
    GLuint stride = ColladaType<GLuint>::fromString(tmp);

    /* Get mesh indices */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_geometries/geometry[%0]/mesh/polylist/p/string()").arg(id+1));
    d->query.evaluateTo(&tmp);
    vector<GLuint> originalIndices = Utility::parseArray<GLuint>(tmp, vertexCount*stride);

    /** @todo assert size()%stride == 0 */

    /* Get unique combinations of vertices, build resulting index array. Key
       is position of unique index combination from original vertex array,
       value is resulting index. */
    unordered_map<unsigned int, unsigned int, IndexHash, IndexEqual> indexCombinations(originalIndices.size()/stride, IndexHash(originalIndices, stride), IndexEqual(originalIndices, stride));
    vector<unsigned int> combinedIndices;
    for(size_t i = 0, end = originalIndices.size()/stride; i != end; ++i)
        combinedIndices.push_back(indexCombinations.insert(make_pair(i, indexCombinations.size())).first->second);

    /* Convert quads to triangles */
    vector<unsigned int>* indices = new vector<unsigned int>;
    size_t quadId = 0;
    for(size_t i = 0; i != vertexCountPerFace.size(); ++i) {
        if(quads.size() > quadId && quads[quadId] == i) {
            indices->push_back(combinedIndices[i*3+quadId]);
            indices->push_back(combinedIndices[i*3+quadId+1]);
            indices->push_back(combinedIndices[i*3+quadId+2]);
            indices->push_back(combinedIndices[i*3+quadId]);
            indices->push_back(combinedIndices[i*3+quadId+2]);
            indices->push_back(combinedIndices[i*3+quadId+3]);

            ++quadId;
        } else {
            indices->push_back(combinedIndices[i*3+quadId]);
            indices->push_back(combinedIndices[i*3+quadId+1]);
            indices->push_back(combinedIndices[i*3+quadId+2]);
        }
    }

    /* Get mesh vertices */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_geometries/geometry[%0]/mesh/polylist/input[@semantic='VERTEX']/@source/string()")
        .arg(id+1));
    d->query.evaluateTo(&tmp);
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_geometries/geometry/mesh/vertices[@id='%0']/input[@semantic='POSITION']/@source/string()")
        .arg(tmp.mid(1).trimmed()));
    d->query.evaluateTo(&tmp);
    vector<Vector3> originalVertices = parseSource<Vector3>(tmp.mid(1).trimmed());

    /* Build vertex array */
    GLuint vertexOffset = attributeOffset(id, "VERTEX");
    vector<Vector4>* vertices = new vector<Vector4>(indexCombinations.size());
    for(auto i: indexCombinations)
        (*vertices)[i.second] = originalVertices[originalIndices[i.first*stride+vertexOffset]];

    QStringList tmpList;
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_geometries/geometry[%0]/mesh/polylist/input/@semantic/string()").arg(id+1));
    d->query.evaluateTo(&tmpList);
    vector<vector<Vector3>*> normals;
    vector<vector<Vector2>*> textureCoords2D;
    for(QString attribute: tmpList) {
        /* Vertices - already built */
        if(attribute == "VERTEX") continue;

        /* Normals */
        else if(attribute == "NORMAL")
            normals.push_back(buildAttributeArray<Vector3>(id, "NORMAL", normals.size(), originalIndices, stride, indexCombinations));

        /* 2D texture coords */
        else if(attribute == "TEXCOORD")
            textureCoords2D.push_back(buildAttributeArray<Vector2>(id, "TEXCOORD", textureCoords2D.size(), originalIndices, stride, indexCombinations));

        /* Something other */
        else Warning() << "ColladaImporter:" << '"' + attribute.toStdString() + '"' << "input semantic not supported";
    }

    d->meshes[id] = new MeshData(name, Mesh::Primitive::Triangles, indices, {vertices}, normals, textureCoords2D);
    return d->meshes[id];
}

int ColladaImporter::ColladaImporter::materialForName(const string& name) {
    auto it = d->materialsForName.find(name);
    return (it == d->materialsForName.end()) ? -1 : it->second;
}

AbstractMaterialData* ColladaImporter::material(unsigned int id) {
    if(!d || id >= d->materials.size()) return nullptr;
    if(d->materials[id]) return d->materials[id];

    QString tmp;

    /* Material name */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_materials/material[%0]/@id/string()").arg(id+1));
    d->query.evaluateTo(&tmp);
    string name(tmp.trimmed().toStdString());

    /* Get effect ID */
    QString effect;
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_materials/material[%0]/instance_effect/@url/string()").arg(id+1));
    d->query.evaluateTo(&effect);
    effect = effect.mid(1).trimmed();

    /* Find out which profile it is */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_effects/effect[@id='%0']/*[substring(name(), 1, 8) = 'profile_']/name()").arg(effect));
    d->query.evaluateTo(&tmp);

    /** @todo Support other profiles */

    if(tmp.trimmed() != "profile_COMMON") {
        Error() << "ColladaImporter:" << ('"'+tmp.trimmed()+'"').toStdString() << "effect profile not supported";
        return nullptr;
    }

    /* Get shader type */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_effects/effect[@id='%0']/profile_COMMON/technique/*/name()").arg(effect));
    d->query.evaluateTo(&tmp);
    tmp = tmp.trimmed();

    /** @todo Other (blinn, goraund) profiles */
    if(tmp != "phong") {
        Error() << "ColladaImporter:" << ('"'+tmp+'"').toStdString() << "shader not supported";
        return nullptr;
    }

    /* Ambient color */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_effects/effect[@id='%0']/profile_COMMON/technique/phong/ambient/color/string()").arg(effect));
    d->query.evaluateTo(&tmp);
    Vector3 ambientColor = Utility::parseVector<Vector3>(tmp);

    /* Diffuse color */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_effects/effect[@id='%0']/profile_COMMON/technique/phong/diffuse/color/string()").arg(effect));
    d->query.evaluateTo(&tmp);
    Vector3 diffuseColor = Utility::parseVector<Vector3>(tmp);

    /* Specular color */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_effects/effect[@id='%0']/profile_COMMON/technique/phong/specular/color/string()").arg(effect));
    d->query.evaluateTo(&tmp);
    Vector3 specularColor = Utility::parseVector<Vector3>(tmp);

    /* Shininess */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_effects/effect[@id='%0']/profile_COMMON/technique/phong/shininess/float/string()").arg(effect));
    d->query.evaluateTo(&tmp);
    GLfloat shininess = ColladaType<GLfloat>::fromString(tmp);

    /** @todo Emission, IOR */

    d->materials[id] = new PhongMaterialData(name, ambientColor, diffuseColor, specularColor, shininess);
    return d->materials[id];
}

int ColladaImporter::ColladaImporter::image2DForName(const string& name) {
    auto it = d->images2DForName.find(name);
    return (it == d->images2DForName.end()) ? -1 : it->second;
}

ImageData2D* ColladaImporter::image2D(unsigned int id) {
    if(!d || id >= d->images2D.size()) return nullptr;
    if(d->images2D[id]) return d->images2D[id];

    QString tmp;

    /* Image name */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_images/image[%0]/@id/string()").arg(id+1));
    d->query.evaluateTo(&tmp);
    string name(tmp.trimmed().toStdString());

    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_images/image[%0]/init_from/string()").arg(id+1));
    d->query.evaluateTo(&tmp);
    tmp = tmp.trimmed();

    if(tmp.right(3) != "tga") {
        Error() << "ColladaImporter:" << '"' + tmp.toStdString() + '"' << "has unsupported format";
        return nullptr;
    }

    TgaImporter::TgaImporter tgaImporter;
    ImageData2D* image;
    if(!tgaImporter.open(Directory::join(Directory::path(d->filename), tmp.toStdString()), name) || !(image = tgaImporter.image2D(0)))
        return nullptr;

    d->images2D[id] = image;
    return d->images2D[id];
}

GLuint ColladaImporter::attributeOffset(unsigned int meshId, const QString& attribute, unsigned int id) {
    QString tmp;

    /* Get attribute offset in indices */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_geometries/geometry[%0]/mesh/polylist/input[@semantic='%1'][%2]/@offset/string()")
        .arg(meshId+1).arg(attribute).arg(id+1));
    d->query.evaluateTo(&tmp);
    return ColladaType<GLuint>::fromString(tmp);
}

void ColladaImporter::parseScenes() {
    QStringList tmpList;
    QString tmp;

    /* Default scene */
    d->defaultScene = 0;
    d->query.setQuery(namespaceDeclaration + "/COLLADA/scene/instance_visual_scene/@url/string()");
    d->query.evaluateTo(&tmp);
    string defaultScene = tmp.trimmed().mid(1).toStdString();

    /* Parse all objects in all scenes */
    for(unsigned int sceneId = 0; sceneId != d->scenes.size(); ++sceneId) {
        /* Is this the default scene? */
        d->query.setQuery((namespaceDeclaration + "/COLLADA/library_visual_scenes/visual_scene[%0]/@id/string()").arg(sceneId+1));
        d->query.evaluateTo(&tmp);
        string name = tmp.trimmed().toStdString();
        if(defaultScene == name)
            d->defaultScene = sceneId;

        unsigned int nextObjectId = 0;
        vector<unsigned int> children;
        d->query.setQuery((namespaceDeclaration + "/COLLADA/library_visual_scenes/visual_scene[%0]/node/@id/string()").arg(sceneId+1));
        tmpList.clear();
        d->query.evaluateTo(&tmpList);
        for(QString childId: tmpList) {
            children.push_back(nextObjectId);
            nextObjectId = parseObject(nextObjectId, childId.trimmed());
        }

        d->scenes[sceneId] = new SceneData(name, children);
    }
}

unsigned int ColladaImporter::parseObject(unsigned int id, const QString& name) {
    QString tmp;
    QStringList tmpList, tmpList2;

    /* Transformations */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_visual_scenes/visual_scene//node[@id='%0']/(translate|rotate|scale)/name()").arg(name));
    d->query.evaluateTo(&tmpList);

    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_visual_scenes/visual_scene//node[@id='%0']/(translate|rotate|scale)/string()").arg(name));
    d->query.evaluateTo(&tmpList2);

    Matrix4 transformation;
    for(size_t i = 0; i != size_t(tmpList.size()); ++i) {
        QString type = tmpList[i].trimmed();
        /* Translation */
        if(type == "translate")
            transformation *= Matrix4::translation(Utility::parseVector<Vector3>(tmpList2[i]));

        /* Rotation */
        else if(type == "rotate") {
            int pos = 0;
            Vector3 axis = Utility::parseVector<Vector3>(tmpList2[i], &pos);
            GLfloat angle = ColladaType<GLfloat>::fromString(tmpList2[i].mid(pos));
            transformation *= Matrix4::rotation(deg(angle), axis);

        /* Scaling */
        } else if(type == "scale")
            transformation *= Matrix4::scaling(Utility::parseVector<Vector3>(tmpList2[i]));

        /* It shouldn't get here */
        else CORRADE_ASSERT(0, ("ColladaImporter: unknown translation " + type).toStdString(), id)
    }

    /* Instance type */
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_visual_scenes/visual_scene//node[@id='%0']/*[substring(name(), 1, 9) = 'instance_']/name()").arg(name));
    d->query.evaluateTo(&tmp);
    tmp = tmp.trimmed();

    /* Camera instance */
    if(tmp == "instance_camera") {
        string cameraName = instanceName(name, "instance_camera");
        auto cameraId = d->camerasForName.find(cameraName);
        if(cameraId == d->camerasForName.end()) {
            Error() << "ColladaImporter: camera" << '"'+cameraName+'"' << "was not found";
            return id;
        }

        d->objects[id] = new ObjectData(name.toStdString(), {}, transformation, ObjectData::InstanceType::Camera, cameraId->second);

    /* Light instance */
    } else if(tmp == "instance_light") {
        string lightName = instanceName(name, "instance_light");
        auto lightId = d->lightsForName.find(lightName);
        if(lightId == d->lightsForName.end()) {
            Error() << "ColladaImporter: light" << '"'+lightName+'"' << "was not found";
            return id;
        }

        d->objects[id] = new ObjectData(name.toStdString(), {}, transformation, ObjectData::InstanceType::Light, lightId->second);

    /* Mesh instance */
    } else if(tmp == "instance_geometry") {
        string meshName = instanceName(name, "instance_geometry");
        auto meshId = d->meshesForName.find(meshName);
        if(meshId == d->meshesForName.end()) {
            Error() << "ColladaImporter: mesh" << '"'+meshName+'"' << "was not found";
            return id;
        }

        d->query.setQuery((namespaceDeclaration + "/COLLADA/library_visual_scenes/visual_scene//node[@id='%0']/instance_geometry/bind_material/technique_common/instance_material/@target/string()").arg(name));
        d->query.evaluateTo(&tmp);
        string materialName = tmp.trimmed().mid(1).toStdString();

        /* Mesh doesn't have bound material, add default one */
        /** @todo Solution for unknown materials etc.: -1 ? */
        if(materialName.empty()) d->objects[id] = new MeshObjectData(name.toStdString(), {}, transformation, meshId->second, 0);

        /* Else find material ID */
        else {
            auto materialId = d->materialsForName.find(materialName);
            if(materialId == d->materialsForName.end()) {
                Error() << "ColladaImporter: material" << '"'+materialName+'"' << "was not found";
                return id;
            }

            d->objects[id] = new MeshObjectData(name.toStdString(), {}, transformation, meshId->second, materialId->second);
        }

    /* Blender group instance */
    } else if(tmp.isEmpty()) {
        d->objects[id] = new ObjectData(name.toStdString(), {}, transformation);

    } else {
        Error() << "ColladaImporter:" << '"'+tmp.toStdString()+'"' << "instance type not supported";
        return id;
    }

    /* Add to object name map */
    d->objectsForName.insert({name.toStdString(), id});

    /* Parse child objects */
    unsigned int nextObjectId = id+1;
    vector<unsigned int> children;
    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_visual_scenes/visual_scene//node[@id='%0']/node/@id/string()").arg(name));
    tmpList.clear();
    d->query.evaluateTo(&tmpList);
    for(QString childId: tmpList) {
        children.push_back(nextObjectId);
        nextObjectId = parseObject(nextObjectId, childId.trimmed());
    }
    d->objects[id]->children().swap(children);

    return nextObjectId;
}

string ColladaImporter::instanceName(const QString& name, const QString& instanceTag) {
    QString tmp;

    d->query.setQuery((namespaceDeclaration + "/COLLADA/library_visual_scenes/visual_scene//node[@id='%0']/%1/@url/string()").arg(name).arg(instanceTag));
    d->query.evaluateTo(&tmp);
    return tmp.trimmed().mid(1).toStdString();
}

}}}
