#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#ifdef __cplusplus

// Code courtesy of DaNike
template<typename TRet, typename ...TArgs>
using function_ptr_t = TRet(*)(TArgs...);

#include "../libil2cpp/il2cpp-config-api.h"
#include "../libil2cpp/il2cpp-api.h"
#include "../libil2cpp/il2cpp-api-types.h"
#include "../libil2cpp/il2cpp-class-internals.h"
// #include "../libil2cpp/il2cpp-api-functions.h"

// Taken from: https://github.com/nike4613/BeatMods2/blob/master/BeatMods2/include/util/json.h
#ifndef UTIL_JSON_H
#define UTIL_JSON_H

#define RAPIDJSON_HAS_STDSTRING 1

#include <type_traits>
#include "../../rapidjson/include/rapidjson/document.h"
#include "../../rapidjson/include/rapidjson/prettywriter.h"
#include "../../rapidjson/include/rapidjson/encodings.h"
#include "../../rapidjson/include/rapidjson/reader.h"
#include "../../rapidjson/include/rapidjson/writer.h"
#include "../../rapidjson/include/rapidjson/istreamwrapper.h"
#include "../../rapidjson/include/rapidjson/ostreamwrapper.h"
#include <string>
#include <string_view>
#include <cassert>


namespace Configuration {
    // Provides a class for configuration.
    // You are responsible for Loading and Writing to it as necessary.
    class Config {
        public:
            Config();
            rapidjson::Document document;
            // Loads Config
            static Config Load();
            // Writes Config
            void Write();
    };
}

namespace json {
    
    enum class Endianness {
        Big, Little, Default, Network = Big
    };

    template<typename T> struct sfinae_fail {};

    template<typename, Endianness = Endianness::Default, bool BOM = false> struct encoding_for_char_t;
    template<typename Ch, Endianness E>
    struct encoding_for_char_t<Ch, E, false> {
        static_assert(std::is_integral<Ch>::value);
        static constexpr int size = sizeof(Ch);
        using type = 
            std::conditional<size == 1, rapidjson::UTF8<Ch>,
            std::conditional<size == 2, rapidjson::UTF16<Ch>,
            std::conditional<size == 4, rapidjson::UTF32<Ch>,
            sfinae_fail<Ch>>>>;
    };
    template<typename Ch>
    struct encoding_for_char_t<Ch, Endianness::Default, true> {
        static_assert(std::is_integral<Ch>::value);
        using type = typename encoding_for_char_t<Ch, Endianness::Default, false>::type;
    };
    template<typename Ch>
    struct encoding_for_char_t<Ch, Endianness::Big, true> {
        static_assert(std::is_integral<Ch>::value);
        static constexpr int size = sizeof(Ch);
        using type =
            std::conditional<size == 1, rapidjson::UTF8<Ch>,
            std::conditional<size == 2, rapidjson::UTF16BE<Ch>,
            std::conditional<size == 4, rapidjson::UTF32BE<Ch>,
            sfinae_fail<Ch>>>>;
    };
    template<typename Ch>
    struct encoding_for_char_t<Ch, Endianness::Little, true> {
        static_assert(std::is_integral<Ch>::value);
        static constexpr int size = sizeof(Ch);
        using type =
            std::conditional<size == 1, rapidjson::UTF8<Ch>,
            std::conditional<size == 2, rapidjson::UTF16LE<Ch>,
            std::conditional<size == 4, rapidjson::UTF32LE<Ch>,
            sfinae_fail<Ch>>>>;
    };

    template<typename Ch, Endianness E = Endianness::Default, bool BOM = false>
    using encoding_for_char = typename encoding_for_char_t<Ch, E, BOM>::type;

    template<typename Ch = char, typename Allocator = rapidjson::MemoryPoolAllocator<>>
    std::basic_string_view<Ch> get_string(rapidjson::GenericValue<encoding_for_char<Ch>, Allocator> const& value)
    {
        assert(value.IsString());
        return {value.GetString(), value.GetStringLength()};
    }
}

namespace il2cpp_utils {
    // Returns the first matching class from the given namespace and typeName by searching through all assemblies that are loaded.
    inline Il2CppClass* GetClassFromName(const char* nameSpace, const char* typeName) {
        void *imagehandle = dlopen("/data/app/com.beatgames.beatsaber-1/lib/arm/libil2cpp.so", 0x00000 | 0x00001);
        Il2CppDomain* (*domain_get)(void);
        *(void**)(&domain_get) = dlsym(imagehandle, "il2cpp_domain_get");
        const Il2CppAssembly** (*domain_get_assemblies)(Il2CppDomain*, size_t*);
        *(void**)(&domain_get_assemblies) = dlsym(imagehandle, "il2cpp_domain_get_assemblies");
        const Il2CppImage* (*get_image)(const Il2CppAssembly*);
        *(void**)(&get_image) = dlsym(imagehandle, "il2cpp_assembly_get_image");
        Il2CppClass* (*get_class)(const Il2CppImage*, const char*, const char*);
        *(void**)(&get_class) = dlsym(imagehandle, "il2cpp_class_from_name");

        size_t assemb_count;
        const Il2CppAssembly** allAssemb = domain_get_assemblies(domain_get(), &assemb_count);
        // const Il2CppAssembly** allAssemb = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &assemb_count);

        for (int i = 0; i < assemb_count; i++) {
            auto assemb = allAssemb[i];
            // auto img = il2cpp_assembly_get_image(assemb);
            // auto klass = il2cpp_class_from_name(img, nameSpace, typeName);
            auto img = get_image(assemb);
            auto klass = get_class(img, nameSpace, typeName);
            if (klass) {
                dlclose(imagehandle);
                return klass;
            }
        }
        dlclose(imagehandle);
        return NULL;
    }
    template<typename TObj, typename... TArgs>
    inline TObj* New(Il2CppClass* klass, TArgs* ...args) {
        void *imagehandle = dlopen("/data/app/com.beatgames.beatsaber-1/lib/arm/libil2cpp.so", 0x00000 | 0x00001);
        Il2CppObject* (*object_new)(Il2CppClass*);
        *(void**)(&object_new) = dlsym(imagehandle, "il2cpp_object_new");
        const MethodInfo* (*class_get_methods)(Il2CppClass*, void** iter);
        *(void**)(&class_get_methods) = dlsym(imagehandle, "il2cpp_class_get_methods");
        bool (*type_equals)(const Il2CppType*, const Il2CppType*);
        *(void**)(&type_equals) = dlsym(imagehandle, "il2cpp_type_equals");
        Il2CppObject* (*runtime_invoke)(const MethodInfo*, void*, void**, Il2CppException**);
        *(void**)(&runtime_invoke) = dlsym(imagehandle, "il2cpp_runtime_invoke");
        dlclose(imagehandle);

        void* invoke_params[] = {(reinterpret_cast<void*>(args), ...)};
        // object_new call
        auto obj = object_new(klass);
        // runtime_invoke constructor with right number of args, return null if multiple matches (or take a vector of type pointers to resolve it), return null if constructor errors
        void* myIter = nullptr;
        const MethodInfo* current;
        const MethodInfo* ctor = nullptr;
        constexpr auto count = sizeof...(TArgs);
        auto argarr = {args...};
        while ((current = class_get_methods(klass, &myIter))) {
            if (ctor->parameters_count != count + 1) {
                continue;
            }
            // Start at 1 to ignore 'self' param
            for (int i = 1; i < current->parameters_count; i++) {
                if (!type_equals(current->parameters[i].parameter_type, argarr[i - 1])) {
                    goto next_method;
                }
            }
            ctor = current;
            next_method: continue;
        }
        if (!ctor) {
            return nullptr;
        }
        Il2CppException* exp = nullptr;
        runtime_invoke(ctor, obj, invoke_params, &exp);
        if (exp) {
            return nullptr;
        }
        return reinterpret_cast<TObj*>(obj);
    }
}

#endif

extern "C" {
#endif

// Code courtesy of MichaelZoller

// A C# object
struct Object {
    #ifdef __cplusplus
    Il2CppClass* klass;  // pointer to the class object, which starts with VTable
    #else
    void* klass;
    #endif
    void* monitorData;  // pointer to a MonitorData object, used for thread sync
};

// A C# struct
struct Struct { };

#ifdef __cplusplus
}

template< class T >
struct is_value_type : std::integral_constant< 
    bool,
    std::is_arithmetic<T>::value || std::is_enum<T>::value ||
    std::is_pointer<T>::value ||
    std::is_base_of<Struct, T>::value
> {};

struct ArrayBounds
{
    int32_t length;
    int32_t lower_bound;
};

template<class T>
struct Array : public Object
{
    static_assert(is_value_type<T>::value, "T must be a C# value type! (primitive, pointer or Struct)");
    /* bounds is NULL for szarrays */
    ArrayBounds *bounds;
    /* total number of elements of the array */
    int32_t max_length;
    T values[0];

    int32_t Length() {
        if (bounds) {
            return bounds->length;
        }
        return max_length;
    }
};

// Parses a JSON string, and returns whether it succeeded or not
bool parsejson(rapidjson::Document& doc, std::string_view js);
// Parses the JSON of the filename, and returns whether it succeeded or not
bool parsejsonfile(rapidjson::Document& doc, std::string filename);

extern "C" {
#endif
#ifndef MOD_ID
#error "'MOD_ID' must be defined in the mod!"
#endif
#ifndef VERSION
#error "'VERSION' must be defined in the mod!"
#endif

long getRealOffset(long offset);

#ifdef log
#undef log
#endif
#define log(...) __android_log_print(ANDROID_LOG_INFO, "QuestHook [" MOD_ID " v" VERSION "] ", __VA_ARGS__)

#define MAKE_HOOK(name, addr, retval, ...) \
long addr_ ## name = (long) addr; \
retval (*name)(__VA_ARGS__) = NULL; \
retval hook_ ## name(__VA_ARGS__) 

#define INSTALL_HOOK(name) \
registerInlineHook((uint32_t)getRealOffset(addr_ ## name), (uint32_t)hook_ ## name, (uint32_t **)&name);\
inlineHook((uint32_t)getRealOffset(addr_ ## name));\


#define MAKE_HOOK_NAT(name, addr, retval, ...) \
long addr_ ## name = (long) addr; \
retval (*name)(__VA_ARGS__) = NULL; \
retval hook_ ## name(__VA_ARGS__) 

#define INSTALL_HOOK_NAT(name) \
registerInlineHook((uint32_t)(addr_ ## name), (uint32_t)hook_ ## name, (uint32_t **)&name);\
inlineHook((uint32_t)(addr_ ## name));\

// C# SPECIFIC

// System.string
typedef struct {
    Object object;
    unsigned int len;
    unsigned short str[];
} cs_string;

// BEAT SABER SPECIFIC

// UnityEngine.Color
typedef struct {
    float r;
    float g;
    float b;
    float a;
} Color;

// UnityEngine.Vector2
typedef struct {
    float x;
    float y;
} Vector2;

// UnityEngine.Vector3
typedef struct {
    float x;
    float y;
    float z;
} Vector3;

// OFFSETS

// Create an object using il2cpp_object_new offset
#define OBJ_CREATOR_OFFSET 0x308740
// GameObject.ctor() offset
#define GO_CTOR_OFFSET 0xC86558
// GameObject type offset
#define GO_TYPE_OFFSET 0x19C7998
// System.GetType(string typeName) offset
#define GET_TYPE_OFFSET 0x104B254
// System.String.Concat(cs_string* left, cs_string* right) offset
#define CONCAT_STRING_OFFSET 0x972F2C
// System.String.CreateString(char* array, int start, int length) offset
#define CREATE_STRING_OFFSET 0x9831BC
// System.String.FastAllocateString(int length) offset
#define ALLOCATE_STRING_OFFSET 0x97A704
// System.String.Substring(cs_string* this, int start, int length) offset
#define SUBSTRING_OFFSET 0x96EBEC
// System.String.Replace(cs_string* original, cs_string* old, cs_string* new) offset
#define STRING_REPLACE_OFFSET 0x97FF04

// FUNCTIONS

// Sets the unsigned short array of the given cs_string
void csstrtowstr(cs_string* in, unsigned short* out);
// Sets the character array of the given cs_string
void csstrtostr(cs_string* in, char* out);
// Sets the given cs_string using the given character array and length
void setcsstr(cs_string* in, char* value, size_t length);
// Creates a cs string (allocates it) with the given character array and length and returns it
cs_string* createcsstr(char* characters, size_t length);

// SETTINGS

// ParseError is thrown when failing to parse a JSON file
typedef enum ParseError {
    PARSE_ERROR_FILE_DOES_NOT_EXIST = -1
} ParseError_t;

// WriteError is thrown when failing to create a file
typedef enum WriteError {
    WRITE_ERROR_COULD_NOT_MAKE_FILE = -1
} WriteError_t;

// JSON Parse Errors
typedef enum JsonParseError {
    JSON_PARSE_ERROR = -1
} JsonParseError_t;

// Reads all of the text of a file at the given filename. If the file does not exist, returns NULL
char* readfile(const char* filename);
// Writes all of the text to a file at the given filename. Returns either 0 or WriteError code
int writefile(const char* filename, const char* text);

// CONFIG

#define CONFIG_PATH "/sdcard/Android/data/com.beatgames.beatsaber/files/mods/cfgs/"

#ifdef __cplusplus
// Returns if a file exists and can be written to / read from.
bool fileexists(const char* filename);
}
#endif

#endif /* UTILS_H_INCLUDED */