/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "game/parsing/GameContext.hpp"
#include "VulkanVertex.hpp"

#include <list>
#include <set>
#include <algorithm>

namespace Game {

    int Logger::indent = -1;
    bool Logger::enabled = true;
    std::list<Logger*> Logger::stack;

    using namespace std::experimental;

    bool ValueParsing::ParseFloatSequence(ConstNode node, std::vector<float> & vec) {
        if (!node || !node.IsSequence()) {
            return false;
        }

        char * end;

        vec.clear();

        for (ConstNode value : node) {
            if (!value || !value.IsScalar()) {
                return false;
            }

            vec.push_back((float) std::strtod(value.Scalar().c_str(), &end));

            if (*end != 0) {
                return false;
            }
        }

        return true;
    }

    bool ValueParsing::ParseIntSequence(ConstNode node, std::vector<int> & vec) {
        if (!node || !node.IsSequence()) {
            return false;
        }

        char * end;

        vec.clear();

        for (ConstNode value : node) {
            if (!value || !value.IsScalar()) {
                return false;
            }

            vec.push_back(std::strtol(value.Scalar().c_str(), &end, 10));

            if (*end != 0) {
                return false;
            }
        }

        return true;
    }

    bool ValueParsing::TryParseFloat(ConstNode value, float & fl) {
        if (!value || !value.IsScalar()) {
            return false;
        }

        char * end;

        fl = (float) std::strtod(value.Scalar().c_str(), &end);

        return *end == 0;
    }

    bool ValueParsing::TryParseInt(ConstNode value, int& i, int base) {
        if (!value || !value.IsScalar()) {
            return false;
        }

        char * end;

        i = std::strtol(value.Scalar().c_str(), &end, base);

        return *end == 0;
    }

    std::shared_ptr<void> ValueParsing::ParseValue(std::string type, ConstNode value, size_t* count, ValueType* vtype) {
        if (!value) {
            return nullptr;
        }

        // convert the type to lower case
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);

        if (type == "vfloat") {
            // try to parse a float vector
            std::vector<float> floats;

            if (!ValueParsing::ParseFloatSequence(value, floats)) {
                throw format_error("Invalid value node: vfloat type specified, but value not float vector");
            }

            if (vtype)
                *vtype = ValueType::eFloat;
            if (count)
                *count = floats.size();

            float * fl_buf = new float[floats.size()];
            std::memcpy(fl_buf, floats.data(), sizeof (float) * floats.size());
            return std::shared_ptr<void>(fl_buf);

        } else if (type == "vint") {
            // try to parse an int vector
            std::vector<int> ints;

            if (!ValueParsing::ParseIntSequence(value, ints)) {
                throw format_error("Invalid value node: vint type specified, but value not int vector");
            }

            if (vtype)
                *vtype = ValueType::eInt;
            if (count)
                *count = ints.size();

            int * int_buf = new int[ints.size()];
            std::memcpy(int_buf, ints.data(), sizeof (int) * ints.size());
            return std::shared_ptr<void>(int_buf);

        } else if (type == "float") {
            // try to parse a single float

            float fl;

            if (!ValueParsing::TryParseFloat(value, fl)) {
                throw format_error("Invalid value node: float type specified, but value not float");
            }

            if (vtype)
                *vtype = ValueType::eFloat;
            if (count)
                *count = 1;

            return std::shared_ptr<void>(new float(fl));

        } else if (type == "int") {
            // try to parse a single int

            int i;

            if (!ValueParsing::TryParseInt(value, i)) {
                throw format_error("Invalid value node: int type specified, but value not int");
            }

            if (vtype)
                *vtype = ValueType::eInt;
            if (count)
                *count = 1;

            return std::shared_ptr<void>(new int(i));

        } else {

            // unknown type
            throw format_error("Invalid value node: unknown type " + type);

        }

        return nullptr;
    }

    bool ValueParsing::ParseStringSequence(ConstNode node, std::vector<std::string>& vec) {
        if (!node || !node.IsSequence()) {
            return false;
        }

        std::string temp;

        for (ConstNode str : node) {
            if (!GetScalar(str, temp)) {
                return false;
            }

            vec.push_back(temp);
        }

        return true;
    }

    FieldList::FieldList(ConstNode node) {

        if (!node || !node.IsSequence()) {
            Logger::LogTop("FieldList", "Field list node must be a sequence");
            throw std::runtime_error("Field list node must be a sequence");
        }

        for (ConstNode field : node) {

            std::string tag = field.Tag();
            std::string type;
            int count = 1;
            size_t size = 0;

            Logger::LogTop("FieldList", "Found field " + tag);

            if (!field.IsMap()) {
                Logger::LogTop("FieldList:" + tag, "Not a map");
                throw format_error("Field " + tag + " must be a map");
            }

            if (!GetScalar(field["type"], type)) {
                Logger::LogTop("FieldList:" + tag, "Missing type node");
                throw format_error("Field " + tag + " is missing a type node");
            }

            if (field["count"] && !ValueParsing::TryParseInt(field["count"], count)) {
                Logger::LogTop("FieldList:" + tag, "Invalid count node");
                throw format_error("Invalid field count ");
            }

            if (type == "float") {
                size = sizeof (float) * count;
            } else if (type == "int") {
                size = sizeof (int) * count;
            } else {
                throw format_error("Unknown type " + type);
            }

            fields.push_back(Field(tag, type, count, buffer_size, size));
            byname[tag] = &fields.back();

            Logger::LogTop("FieldList:" + tag, "type: " + type + " count: " + std::to_string(count) + " offset: " + std::to_string(buffer_size) + " size: " + std::to_string(size));

            buffer_size += size;
        }
    }

    Value FieldList::parse(ConstString fieldname, ConstNode value) const {
        const Field * field = getField(fieldname);

        struct Value val = {0};
        val.ptr = ValueParsing::ParseValue(field->type, value, &val.count, &val.type);
        return val;
    }

    UBOInstance::UBOInstance(ConstNode node, const std::map<std::string, UBOPrototype> & prototypes) {
        if (!node || !node.IsSequence()) {
            throw format_error("UBO Instance node must be sequence");
        }

        name = node.Tag();

        std::string proto_name;

        if (GetScalar(node["prototype"], proto_name)) {

            try {
                prototype = &prototypes.at(proto_name);
            } catch (std::out_of_range) {
                throw format_error("Could not find ubo prototype " + proto_name);
            }

            list = prototype->getFields();
        } else {
            list_noproto = std::move(std::unique_ptr<FieldList>(new FieldList(node["fields"])));

            list = list_noproto.get();
        }

        for (ConstNode value : node["values"]) {
            values[list->getField(value.Tag())] = list->parse(value.Tag(), value);
        }
    }

    std::shared_ptr<void> UBOInstance::create() {
        size_t size = list->size();

        std::shared_ptr<char> ptr(new char[size]);

        memset(ptr.get(), 0, size);

        for (auto & field : list->getFields()) {
            Value * value = &values[&field];
            memcpy(ptr.get() + field.offset, value->ptr.get(), field.size);
        }

        return ptr;
    }

    ShaderPrototype::ShaderPrototype(ConstNode node) : name(node.Tag()) {

        if (!GetScalar(node["file"], file)) {
            throw format_error("Shader " + name + " has invalid file");
        }

        if (node["push constants"] && !ValueParsing::ParseStringSequence(node["push constants"], pushconstants)) {
            throw format_error("Shader " + name + ": push constants must be sequence");
        }

        if (node["textures"]) {
            for (ConstNode node : node["textures"]) {
                std::string name;
                int binding;

                if (!GetScalar(node["name"], name) || !ValueParsing::TryParseInt(node["binding"], binding)) {
                    throw format_error("Invalid texture in shader " + name);
                }

                textures.push_back(std::make_tuple(name, binding));
            }
        }

        if (node["ubos"]) {
            for (ConstNode node : node["ubos"]) {
                std::string name;
                int binding;

                if (!GetScalar(node["name"], name) || !ValueParsing::TryParseInt(node["binding"], binding)) {
                    throw format_error("Invalid ubo in shader " + name);
                }

                ubos.push_back(std::make_tuple(name, binding));
            }
        }
    }

    MaterialPrototype::MaterialPrototype(ConstNode node, std::map<std::string, MaterialPrototype*> byname) {
        if (!node || !node.IsMap()) {
            throw format_error("Material Prototype node must be map");
        }

        name = node.Tag();

        if (!GetScalar(node["fragment"], fragment)) {
            throw format_error("Fragment shader is not specified");
        }

        if (!GetScalar(node["vertex"], vertex)) {
            throw format_error("Vertex shader is not specified");
        }

        if (node["ubos"]) {
            for (ConstNode ubo : node["ubos"]) {
                std::string name, instance;

                if (!GetScalar(ubo["name"], name)) {
                    throw format_error("UBO name not valid");
                }

                if (!GetScalar(ubo["instance"], ubos[name].reference)) {
                    throw format_error("UBO instance not valid");
                }

                if (!ValueParsing::ParseStringSequence(ubo["shaders"], ubos[name].shaders)) {
                    throw format_error("UBO shader sequence missing");
                }
            }
        }

        if (node["textures"]) {
            for (ConstNode texture : node["textures"]) {
                std::string name, instance;

                if (!GetScalar(texture["name"], name)) {
                    throw format_error("Texture name not valid");
                }

                if (!ValueParsing::ParseStringSequence(texture["shaders"], textures[name].shaders)) {
                    throw format_error("Texture shader sequence missing");
                }
            }
        }

    }

    GameContext::GameContext(ConstString filename) {
        document = CalculateDependencies(filename);

        Logger logger(filesystem::path(filename).filename().generic_string());

        for (auto file : document) {
            ConstNode node = std::get<0>(file);
            std::string name = std::get<1>(file).generic_string();

            logger.Log("Reading constants in " + name);
            ReadConstants(node["constants"]);

            logger.Log("Reading ubo prototypes in " + name);
            ReadUBOPrototypes(node["ubo prototypes"]);

            logger.Log("Reading ubo instances in " + name);
            ReadUBOInstances(node["ubo instances"]);

            logger.Log("Reading vertex buffers in " + name);
            ReadVertexBuffers(node["vertex buffers"]);

            logger.Log("Reading index buffers in " + name);
            ReadIndexBuffers(node["index buffers"]);

            logger.Log("Reading push constants in " + name);
            ReadPushConstants(node["push constants"]);

            logger.Log("Reading shaders in " + name);
            ReadShaders(node["shaders"]);
        }
    }

    std::list<std::tuple<YAML::Node, filesystem::path>> GameContext::CalculateDependencies(ConstString file) {
        std::list<std::tuple < YAML::Node, filesystem::path >> file_stack
        {
            std::make_tuple<YAML::Node, filesystem::path>(YAML::LoadFile(file), file)
        };
        std::set<filesystem::path> open_files;

        Logger logger(std::get<1>(file_stack.front()).filename().generic_string());

        logger.Log("Starting with file " + file);

        for (auto & current : file_stack) {
            ConstNode node = std::get<0>(current)["references"];
            filesystem::path & file = std::get<1>(current);

            std::string filename = file.filename().generic_string();

            logger.Log("Checking for references in " + filename);

            if (!node) {
                logger.Log("No references node in " + filename);
                continue;
            }

            if (!node.IsSequence()) {
                throw format_error("references must be a sequence");
            }

            for (ConstNode reference : node) {
                filesystem::path ref(reference.Scalar());

                ref = file.parent_path() / ref;

                logger.Log(filename, "Found reference " + ref.generic_string());

                // if the file isn't already open, open it
                if (open_files.find(ref) == open_files.end()) {
                    open_files.insert(ref);

                    YAML::Node dep = YAML::LoadFile(ref.generic_string());

                    if (!dep) {
                        throw std::runtime_error("could not find file " + ref.generic_string());
                    }

                    file_stack.push_front(std::make_tuple(dep, ref));
                }

            }
        }

        return file_stack;
    }

    void GameContext::ReadConstants(ConstNode constants) {
        if (!constants) {
            return;
        }

        if (!constants.IsSequence()) {
            throw format_error("Constants node must be sequence or missing");
        }


        for (ConstNode _const : constants) {
            std::string name;
            if (!GetScalar(_const["name"], name) || !_const["type"] || !_const["value"]) {
                throw format_error("Constants must have name, type and value elements");
            }
            this->constants[name] = ValueParsing::ParseValue(_const);
        }
    }

    void GameContext::ReadUBOPrototypes(ConstNode ubops) {
        if (!ubops) {
            return;
        }

        if (!ubops.IsSequence()) {
            throw format_error("UBO Prototype node must be sequence or missing");
        }

        for (ConstNode ubop : ubops) {
            if (!ubop.IsMap()) {
                throw format_error("UBO Prototype must be a map");
            }

            uboprototypes[ubop.Tag()] = UBOPrototype(ubop);
        }

    }

    void GameContext::ReadUBOInstances(ConstNode ubois) {
        if (!ubois) {
            return;
        }

        if (!ubois.IsSequence()) {
            throw format_error("UBO Instance node must be sequence or missing");
        }

        for (ConstNode uboi : ubois) {
            if (!uboi.IsMap()) {
                throw format_error("UBO Instance must be a map");
            }

            uboinstances[uboi.Tag()] = UBOInstance(uboi, this->uboprototypes);
        }

    }

    void GameContext::ReadVertexBuffers(ConstNode vbs) {
        if (!vbs) {
            return;
        }

        if (!vbs.IsSequence()) {
            throw format_error("Vertex Buffers node must be sequence or missing");
        }

        for (ConstNode vertexbuffer : vbs) {
            if (!vertexbuffer.IsSequence()) {
                throw format_error("Vertex Buffer " + vertexbuffer.Tag() + " must be sequence");
            }

            int i = 0;

            vertex_buffers[vertexbuffer.Tag()].clear();

            for (ConstNode vertex : vertexbuffer) {
                if (!vertex.IsMap()) {
                    throw format_error("Vertex " + std::to_string(i) + " must be a map with uv, color, and/or position");
                }

                VertexInfo vert = {glm::vec2(0), glm::vec3(0), glm::vec2(0)};

                std::vector<float> vec;

                if (ValueParsing::ParseFloatSequence(vertex["uv"], vec)) {
                    vert.uv = ValueParsing::VectorToVec2(vec);
                }

                if (ValueParsing::ParseFloatSequence(vertex["color"], vec)) {
                    vert.color = ValueParsing::VectorToVec3(vec);
                }

                if (ValueParsing::ParseFloatSequence(vertex["position"], vec)) {
                    vert.position = ValueParsing::VectorToVec2(vec);
                }

                vertex_buffers[vertexbuffer.Tag()].push_back(vert);
            }
        }
    }

    void GameContext::ReadIndexBuffers(ConstNode ibs) {
        if (!ibs) {
            return;
        }

        if (!ibs.IsSequence()) {
            throw format_error("Vertex Buffers node must be sequence or missing");
        }

        for (ConstNode indexbuffer : ibs) {
            if (!indexbuffer.IsSequence()) {
                throw format_error("Index Buffer " + indexbuffer.Tag() + " must be a scalar sequence");
            }

            if (!ValueParsing::ParseIntSequence(indexbuffer, index_buffers[indexbuffer.Tag()])) {
                throw format_error("Index Buffer " + indexbuffer.Tag() + " must be a scalar sequence");
            }
        }
    }

    void GameContext::ReadPushConstants(ConstNode pcs) {
        if (!pcs) {
            return;
        }

        if (!pcs.IsSequence()) {
            throw format_error("Push Constant list must be sequence");
        }

        for (ConstNode node : pcs) {
            if (node.IsMap()) {
                throw format_error("Push Constant must be a map");
            }

            push_constants[node.Tag()] = PushConstantInfo(node);
        }
    }

    void GameContext::ReadShaders(ConstNode shadernode) {
        if (!shadernode) {
            return;
        }

        if (!shadernode.IsSequence()) {
            throw format_error("Shader list must be sequence");
        }

        for (ConstNode shader : shadernode) {
            if (shader.IsMap()) {
                throw format_error("Shader must be a map");
            }

            shaders[shader.Tag()] = ShaderPrototype(shader);
        }
    }

};
