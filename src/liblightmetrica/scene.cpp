/*
    Lightmetrica - A modern, research-oriented renderer

    Copyright (c) 2015 Hisanari Otsu

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include <pch.h>
#include <lightmetrica/scene.h>
#include <lightmetrica/math.h>
#include <lightmetrica/property.h>
#include <lightmetrica/assets.h>
#include <lightmetrica/primitive.h>
#include <lightmetrica/accel.h>
#include <glm/glm.hpp>

LM_NAMESPACE_BEGIN

class Scene_ final : public Scene
{
public:

    LM_IMPL_CLASS(Scene_, Scene);

public:

    // Scene version support
    using VersionT = std::tuple<int, int, int>;
    const VersionT MinVersion{ 1, 0, 0 };
    const VersionT MaxVersion{ 1, 0, 0 };

public:

    LM_IMPL_F(Initialize) = [this](const PropertyNode* prop, Assets* assets) -> bool
    {
        #pragma region Check root node

        // Scene configuration file must begin with `lightmetrica_scene` node
        const auto* root = prop->Child("lightmetrica_scene");
        if (!root)
        {
            // TODO: Improve error messages
            LM_LOG_ERROR("Missing 'lightmetrica_scene' node");
            return false;
        }

        #pragma endregion

        // --------------------------------------------------------------------------------

        #pragma region Scene version check

        {
            const auto* versionNode = root->Child("version");
            if (!versionNode)
            {
                LM_LOG_ERROR("Missing 'version' node");
                return false;
            }

            // Parse version string
            const auto versionStr = versionNode->As<std::string>();
            std::regex re(R"x((\d)\.(\d)\.(\d))x");
            std::smatch match;
            const bool result = std::regex_match(versionStr, match, re);
            if (!result)
            {
                LM_LOG_ERROR("Invalid version string: " + versionStr);
                return false;
            }

            // Check version
            VersionT version{ std::stoi(match[1]), std::stoi(match[2]), std::stoi(match[3]) };
            if (version < MinVersion || MaxVersion < version)
            {
                LM_LOG_ERROR(boost::str(boost::format("Invalid version [ Expected: (%d.%d.%d)-(%d.%d.%d), Actual: (%d.%d.%d) ]")
                    % std::get<0>(MinVersion) % std::get<1>(MinVersion) % std::get<2>(MinVersion)
                    % std::get<0>(MaxVersion) % std::get<1>(MaxVersion) % std::get<2>(MaxVersion)
                    % std::get<0>(version) % std::get<1>(version) % std::get<2>(version)));
                return false;
            }
        }

        #pragma endregion

        // --------------------------------------------------------------------------------

        #pragma region Initialize asset manager

        {
            const auto* assetsNode = root->Child("assets");
            if (!assetsNode)
            {
                LM_LOG_ERROR("Missing 'assets' node");
                return false;
            }

            if (!assets->Initialize(assetsNode))
            {
                return false;
            }
        }
        
        #pragma endregion

        // --------------------------------------------------------------------------------

        #pragma region Load primitives
        
        {
            #pragma region Sensor node
            
            const auto* sceneNode = root->Child("scene");
            if (!sceneNode)
            {
                // TODO: Add error check (with detailed and human-readable error message)
                LM_LOG_ERROR("Missing 'scene' node");
                return false;
            }

            #pragma endregion

            // --------------------------------------------------------------------------------

            #pragma region Traverse scene nodes and create primitives

            const std::function<bool(const PropertyNode*, const Mat4&)> Traverse = [&](const PropertyNode* propNode, const Mat4& parentTransform) -> bool
            {
                #pragma region Create primitive

                std::unique_ptr<Primitive> primitive(new Primitive);

                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region Parse ID
                
                {
                    const auto* idNode = propNode->Child("id");
                    if (idNode)
                    {
                        primitive->id = idNode->As<std::string>();
                    }
                }

                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region Parse transform

                Mat4 transform;

                {
                    const auto* transformNode = propNode->Child("transform");
                    if (!transformNode)
                    {
                        // Missing `transform` node, identity matrix is assumed
                        primitive->transform = Mat4::Identity();
                    }
                    else
                    {
                        // Parse transform from the node
                        // There are several ways to specify a transformation
                        const auto ParseTransform = [](const PropertyNode* transformNode, Mat4& transform) -> bool
                        {
                            // `matrix` node
                            const auto* matrixNode = transformNode->Child("matrix");
                            if (matrixNode)
                            {
                                // Parse 4x4 matrix
                                transform = matrixNode->As<Mat4>();
                                return true;
                            }

                            // `translate`, `rotate`, or `scale` node
                            const auto* translateNode = transformNode->Child("translate");
                            const auto* rotateNode    = transformNode->Child("rotate");
                            const auto* scaleNode     = transformNode->Child("scale");
                            if (translateNode || rotateNode || scaleNode)
                            {
                                transform = Mat4::Identity();

                                // Parse 'translate' node
                                if (translateNode)
                                {
                                    transform *= Math::Translate(translateNode->As<Vec3>());
                                }

                                // Parse 'rotate' node
                                if (rotateNode)
                                {
                                    const auto angleNode = rotateNode->Child("axis");
                                    const auto axisNode  = rotateNode->Child("angle");
                                    if (!angleNode || !axisNode)
                                    {
                                        LM_LOG_ERROR("Missing 'angle' or 'axis' node");
                                        return false;
                                    }

                                    transform *= Math::Rotate(Math::Radians(axisNode->As<Float>()), angleNode->As<Vec3>());
                                }

                                // Parse 'scale' node
                                if (scaleNode)
                                {
                                    transform *= Math::Scale(scaleNode->As<Vec3>());
                                }

                                return true;
                            }

                            transform = Mat4::Identity();
                            return true;
                        };
                        
                        if (!ParseTransform(transformNode, transform))
                        {
                            return false;
                        }

                        transform = parentTransform * transform;
                        primitive->transform = transform;
                    }
                }

                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region Light

                {
                    //const auto* light = assets->GetByID();
                }

                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region Sensor

                

                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region Triangle mesh

                

                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region BSDF

                

                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region Add primitive

                if (!primitive->id.empty())
                {
                    primitiveIDMap_[primitive->id] = primitive.get();
                }

                primitives_.push_back(std::move(primitive));
                
                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region Traverse child nodes

                const auto* childNode = propNode->Child("child");
                if (childNode)
                {
                    for (int i = 0; i < childNode->Size(); i++)
                    {
                        if (!Traverse(childNode->At(i), transform))
                        {
                            return false;
                        }
                    }
                }

                #pragma endregion

                // --------------------------------------------------------------------------------

                return true;
            };

            // --------------------------------------------------------------------------------

            const auto* nodesNode = sceneNode->Child("nodes");
            for (int i = 0; i < nodesNode->Size(); i++)
            {
                if (!Traverse(nodesNode->At(i), Mat4::Identity()))
                {
                    return false;
                }
            }

            #pragma endregion

            // --------------------------------------------------------------------------------

            #pragma region Main sensor

            {
                const auto mainSensorNode = sceneNode->Child("sensor");
                if (!mainSensorNode)
                {
                    LM_LOG_ERROR("Missing 'sensor' node");
                    return false;
                }

                const auto it = primitiveIDMap_.find(mainSensorNode->As<std::string>());
                if (it == primitiveIDMap_.end())
                {
                    LM_LOG_ERROR("Missing primitive ID: " + mainSensorNode->As<std::string>());
                    return false;
                }

                sensorPrimitive_ = it->second;
            }

            #pragma endregion
        }

        #pragma endregion

        // --------------------------------------------------------------------------------

        return true;
    };

    LM_IMPL_F(PrimitiveByID) = [this](const std::string& id) -> const Primitive*
    {
        const auto it = primitiveIDMap_.find(id);
        return it != primitiveIDMap_.end() ? it->second : nullptr;
    };

    LM_IMPL_F(Sensor) = [this]() -> const Primitive*
    {
        return sensorPrimitive_;
    };

private:

    std::vector<std::unique_ptr<Primitive>> primitives_;            // Primitives
    std::unordered_map<std::string, Primitive*> primitiveIDMap_;    // Mapping from ID to primitive pointer
    Primitive* sensorPrimitive_;                                    // Index of main sensor
    //Accel* accel_;

};

LM_COMPONENT_REGISTER_IMPL(Scene_);

LM_NAMESPACE_END
