/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   GameContext.hpp
 * Author: austin-z
 *
 * Created on March 22, 2019, 6:27 PM
 */

#ifndef GAMECONTEXT_HPP
#define GAMECONTEXT_HPP

#include <yaml-cpp/yaml.h>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <string>
#include <map>
#include <vector>
#include <exception>
#include <iostream>
#include <experimental/filesystem>

namespace Loader {

    template<typename T>
    class DataTypeLoader {
    public:
        virtual T Load(unsigned char * buffer, size_t offset) = 0;

    };

    template<typename T>
    class CopyLoader : DataTypeLoader<T> {
    public:

        T Load(unsigned char * buffer, size_t offset) override {
            T value = {0};

            memcpy(&value, buffer + offset, sizeof (T));

            return value;
        }

    };

    class StringLoader : DataTypeLoader<std::string> {
    public:

        std::string Load(unsigned char * buffer, size_t offset) override {
            unsigned char * data = buffer + offset;

            short length = data[0] << 8 | data[1];

            char * cbuf = new char[length];

            memcpy(cbuf, &buffer[2], length);

            std::string str(cbuf, length);

            delete[] cbuf;

            return str;
        }

    };

    class DataTypes {
    public:

        const CopyLoader<unsigned char> Byte;
        const CopyLoader<short> Short;
        const CopyLoader<int> Integer;
        const CopyLoader<long long> Long;
        const CopyLoader<float> Float;
        const CopyLoader<double> Double;

    };


}








namespace Game {

    template<typename T>
    using ConstRef = const T &;

    using ConstNode = ConstRef<YAML::Node>;
    using ConstString = ConstRef<std::string>;

    /**
     * Gets the scalar from the given node
     * @param node The scalar node
     * @param str A reference to a constant string
     * @return {@code true} if the parameters are valid and contains a scalar
     */
    static bool GetScalar(ConstNode node, std::string & str) {
        if (!node || !node.IsScalar()) {
            return false;
        }

        str = node.Scalar();

        return true;
    }

    enum ValueType {
        eUnknown,
        eFloat,
        eInt
    };

    struct Value {
        std::shared_ptr<void> ptr;
        size_t count;
        ValueType type;
    };

    class Logger {
    private:
        static int indent;

        static std::list<Logger*> stack;

        std::string context;

    public:

        static bool enabled;

        Logger(ConstString ctx = std::string()) : context(ctx) {
            indent++;
            stack.push_back(this);
        }

        ~Logger() {
            indent--;
            stack.pop_back();
        }

        void SetContext(ConstString ctx) {
            context = ctx;
        }

        void Log(ConstString message) {
            if (enabled) {
                std::cout << std::string(indent, '\t') << "[" << context << "] " << message << std::endl;
            }
        }

        void Log(ConstString subcontext, ConstString message) {
            if (enabled) {
                std::cout << std::string(indent, '\t') << "[" << context << ":" << subcontext << "] " << message << std::endl;
            }
        }

        static void LogTop(ConstString message) {
            if (stack.back()) {
                stack.back()->Log(message);
            }
        }

        static void LogTop(ConstString subcontext, ConstString message) {
            if (stack.back()) {
                stack.back()->Log(subcontext, message);
            }
        }

    };

    class format_error : public std::runtime_error {
    public:

        explicit format_error(ConstString string) : std::runtime_error(string) {
        }
    };

    class ValueParsing {
    public:

        /**
         * Parses the given sequence node into a vector and checks if it was a valid
         * float sequence.
         * @param node The sequence node
         * @param vec The vector
         * @return {@code false} if the node is null, isn't a sequence, or if an element
         *      isn't a float
         */
        static bool ParseFloatSequence(ConstNode node, std::vector<float> & vec);

        /**
         * Parses the given sequence node into a vector and checks if it was a valid
         * int sequence.
         * @param node The sequence node
         * @param vec The vector
         * @return {@code false} if the node is null, isn't a sequence, or if an element
         *      isn't an int
         */
        static bool ParseIntSequence(ConstNode node, std::vector<int> & vec);

        /**
         * Parses a single float from a scalar node
         * @param value The scalar node
         * @param fl The float value
         * @return {@code true} if the node contained a valid float
         */
        static bool TryParseFloat(ConstNode value, float & fl);

        /**
         * Parses a single int from a scalar node
         * @param value The scalar node
         * @param i The int value
         * @param base The int base
         * @return {@code true} if the node contained a valid int
         */
        static bool TryParseInt(ConstNode value, int & i, int base = 10);

        /**
         * Parses a value node
         * <br>
         * Must have format of
         * node:
         *  type: [vfloat, vint, float, int]
         *  value: [floats]/[ints]/float/int
         * @param value The node
         * @param count The number of elements
         * @param type The type of elements
         * @return The element pointer
         */
        static std::shared_ptr<void> ParseValue(ConstNode node, size_t * count, ValueType * type) {
            return ParseValue(node["type"], node["value"], count, type);
        }

        /**
         * Parses a value node
         * <br>
         * Must have formats of
         *  type: [vfloat, vint, float, int]
         *  value: [floats]/[ints]/float/int
         * @param type The type node
         * @param value The value node
         * @param count The number of elements
         * @param type The type of elements
         * @return The element pointer
         */
        static std::shared_ptr<void> ParseValue(ConstNode typenode, ConstNode value, size_t * count, ValueType * vtype) {
            if (!typenode) {
                return nullptr;
            }

            std::string type;

            if (!GetScalar(typenode, type)) {
                throw format_error("value type must be a scalar");
            }

            return ParseValue(type, value, count, vtype);
        }

        /**
         * Parses a value node
         * <br>
         * Must have format of:
         *  value: [floats]/[ints]/float/int
         * @param type_name The type (must be [v](int/float) )
         * @param value The value node
         * @param count The number of elements
         * @param type The type of elements
         * @return The element pointer
         */
        static std::shared_ptr<void> ParseValue(std::string type_name, ConstNode value, size_t * count, ValueType * type);

        /**
         * Calls ParseValue but returns easy to use structure.
         * @param value The value node
         * @return The pointer, count, and type
         */
        static struct Value ParseValue(ConstNode vnode) {
            struct Value value = {0};
            value.ptr = ParseValue(vnode, &value.count, &value.type);
            return value;
        }

        /**
         * Calls ParseValue but returns easy to use structure.
         * @param value The value node
         * @return The pointer, count, and type
         */
        static struct Value ParseValue(ConstString type, ConstNode vnode) {
            struct Value value = {0};
            value.ptr = ParseValue(type, vnode, &value.count, &value.type);
            return value;
        }

        static vk::ShaderStageFlagBits ParseStage(ConstNode node) {
            std::string value;

            if (!GetScalar(node, value)) {
                throw format_error("Stage node must be a scalar");
            }

            // convert the type to lower case
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);

            if (value == "fragment") {
                return vk::ShaderStageFlagBits::eFragment;
            }

            if (value == "vertex") {
                return vk::ShaderStageFlagBits::eVertex;
            }

            throw format_error("Invalid shader stages " + value);
        }

        /**
         * Tries to parse the node as a string sequence.
         * @param node The node to parse
         * @param vec The list of strings
         * @return 
         */
        static bool ParseStringSequence(ConstNode node, std::vector<std::string> & vec);

        template<typename T>
        static glm::vec2 VectorToVec2(std::vector<T> vec, size_t at = 0) {
            if (at > vec.size() - 2) {
                throw std::out_of_range("at cannot be >= vec.size() - 2");
            }

            return glm::vec2((float) vec[at], (float) vec[at + 1]);
        }

        template<typename T>
        static glm::vec3 VectorToVec3(std::vector<T> vec, size_t at = 0) {
            if (at > vec.size() - 3) {
                throw std::out_of_range("at cannot be >= vec.size() - 2");
            }

            return glm::vec3((float) vec[at], (float) vec[at + 1], (float) vec[at + 2]);
        }

    };

    class FieldList {
    public:

        struct Field {
        public:
            std::string name, type;
            size_t count, offset, size;

            Field(ConstString name, ConstString type, size_t count, size_t offset, size_t size) :
            name(name), type(type), count(count), offset(offset), size(size) {

            }
        };

    private:

        std::list<Field> fields;
        std::map<std::string, Field*> byname;

        size_t buffer_size = 0;

    public:

        FieldList() {

        }

        FieldList(ConstNode node);

        template<typename T>
        T & at(ConstString name, unsigned char * buffer, size_t len, size_t index = 0) {
            Field* field = byname.at(name);

            if (field->offset + field->size * (index + 1) >= len) {
                throw std::out_of_range("Illegal parameters: buffer not large enough for requested object");
            }

            if (index > field->count) {
                throw std::out_of_range("index > field count");
            }

            return *(reinterpret_cast<T*> (buffer + field->offset) + index);
        }

        const struct Field * getField(ConstString field) const {
            return byname.at(field);
        }

        ConstRef<std::list<Field>> getFields() const {
            return fields;
        }

        Value parse(ConstString field, ConstNode value) const;

        const size_t size() const {
            return buffer_size;
        }

    };

    class PushConstantInfo {
        std::string name;

        vk::ShaderStageFlags stages;
        FieldList fields;

    public:

        PushConstantInfo() {
        }

        PushConstantInfo(ConstNode node) : name(node.Tag()), stages(ValueParsing::ParseStage(node["stages"])), fields(node["fields"]) {

        }

    };

    class UBOPrototype {
        std::string name;

        FieldList fields;

        size_t count;

    public:

        UBOPrototype() {

        }

        UBOPrototype(ConstNode node) : name(node.Tag()), fields(node["fields"]) {
            Logger logger(name);
            logger.Log("Creating ubo prototype");
            if (node["count"]) {
                int count;
                if (ValueParsing::TryParseInt(node["count"], count)) {
                    this->count = count;
                } else {
                    throw format_error("Invalid count node");
                }
            }
        }

        const FieldList * getFields() const {
            return &fields;
        }

    };

    class UBOInstance {
        std::string name;

        const UBOPrototype * prototype = nullptr;

        const FieldList * list = nullptr;

        // Used when the ubo instance has no prototype and must use its own fieldlist
        std::unique_ptr<FieldList> list_noproto;

        std::map<const FieldList::Field*, Value> values;

    public:

        UBOInstance() {

        }

        UBOInstance(ConstNode node, const std::map<std::string, UBOPrototype> & prototypes);

        std::shared_ptr<void> create();

    };

    struct VertexInfo {
        glm::vec2 position;
        glm::vec3 color;
        glm::vec2 uv;
    };

    class ShaderPrototype {
        std::string name, file;
        vk::ShaderStageFlagBits stage;

        std::vector<std::string> pushconstants;
        std::vector<std::tuple<std::string, int>> textures, ubos;

    public:

        ShaderPrototype() {

        }

        ShaderPrototype(ConstNode node);

    };

    class MaterialPrototype {

        struct _filteredname {
            std::string reference;
            std::vector<std::string> shaders;
        };

        std::string name, vertex, fragment;
        std::vector<std::string> push_constants;
        std::map<std::string, struct _filteredname> ubos, textures;

    public:

        MaterialPrototype(ConstNode node, std::map<std::string, MaterialPrototype*> byname);

    };

    class GameObjectDescriptor {
    };

    class GameContext {
    private:

        std::list<std::tuple<YAML::Node, std::experimental::filesystem::path>> document;

        std::map<std::string, Value> constants;

        std::map<std::string, UBOPrototype> uboprototypes;

        std::map<std::string, UBOInstance> uboinstances;

        std::map<std::string, std::vector<VertexInfo>> vertex_buffers;

        std::map<std::string, std::vector<int>> index_buffers;

        std::map<std::string, PushConstantInfo> push_constants;

        std::map<std::string, ShaderPrototype> shaders;

    public:

        GameContext(ConstString file);

    private:

        /**
         * Reads the reference tree for the current file.
         * Does not load the same file twice.
         * @return The list of dependencies, with the current document last
         */
        std::list<std::tuple<YAML::Node, std::experimental::filesystem::path>> CalculateDependencies(ConstString parent);

        /**
         * Reads the constant sequence from the node into this context's constants.
         * On name conflict, the old constant gets deleted.
         * @param node The node to read from
         */
        void ReadConstants(ConstNode node);

        /**
         * Reads the ubo prototypes in the given node
         * @param node The ubo prototypes list node
         */
        void ReadUBOPrototypes(ConstNode node);

        /**
         * Reads the ubo instances in the given node
         * @param node The ubo instances list node
         */
        void ReadUBOInstances(ConstNode node);

        /**
         * Reads the vertex buffers in the given node
         * @param node The vertex buffer list node
         */
        void ReadVertexBuffers(ConstNode node);

        /**
         * Reads the index buffers in the given node
         * @param node The index buffer list node
         */
        void ReadIndexBuffers(ConstNode node);

        /**
         * Reads the push constants in the given node
         * @param node The push constant list node
         */
        void ReadPushConstants(ConstNode node);

        /**
         * Reads the shaders in the given node
         * @param node The shaders list node
         */
        void ReadShaders(ConstNode node);

    };
};

#endif /* GAMECONTEXT_HPP */
