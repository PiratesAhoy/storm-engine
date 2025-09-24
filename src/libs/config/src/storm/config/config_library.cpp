#include "config_library.hpp"

#include "storm/config/config.hpp"

#include <core.h>
#include <istring.hpp>
#include <s_import_func.h>
#include <v_s_stack.h>
#include <vma.hpp>

namespace storm {

namespace {

using Config = ConfigLibrary;

CREATE_SCRIPTLIBRIARY(Config)

void ConvertTable(ATTRIBUTES *target, const storm::Data &table, entid_t entity_id);

void ConvertValue(ATTRIBUTES *target, const std::string &key, const storm::Data &node, entid_t entity_id)
{
    if (node.is_array())
    {
        ATTRIBUTES *new_element = target->VerifyAttributeClass(key);
        size_t offset = 0;
        std::for_each(node.begin(), node.end(), [&](auto &&el) {
            while(new_element->HasAttribute(std::to_string(offset))) {
                ++offset;
            }
            std::string index_key = std::to_string(offset);
            ConvertValue(new_element, index_key, el, entity_id);
        });
    }
    else if (node.is_object())
    {
        ConvertTable(target->VerifyAttributeClass(key), node, entity_id);
    }
    else if (node.is_string())
    {
        std::string str = node;
        if (key == "__value__")
        {
            target->SetValue(str);
            if (entity_id != invalid_entity)
            {
                core.Entity_AttributeChanged(entity_id, target);
            }
            return;
        }
        else
        {
            target->SetAttribute(key, str);
        }
    }
    else if (node.is_number_float())
    {
        target->SetAttributeUseFloat(key.c_str(), node.get<float>());
    }
    else if (node.is_number_integer())
    {
        target->SetAttribute(key, std::to_string(node.get<int64_t>()));
    }
    else if (node.is_boolean()) {
        target->SetAttribute(key, node.get<bool>() ? "1" : "0");
    }
    ATTRIBUTES *updated_attr = target->GetAttributeClass(key);
    if (entity_id != invalid_entity && updated_attr != nullptr)
    {
        core.Entity_AttributeChanged(entity_id, updated_attr);
    }
}

void ConvertTable(ATTRIBUTES *target, const storm::Data &config, entid_t entity_id)
{
    for (const auto &[key, value] : config.items())
    {
        ConvertValue(target, key, value, entity_id);
    }
}

uint32_t LoadConfigImpl(VS_STACK *pS) {
    auto *pString = (VDATA *)pS->Pop();
    auto* objectPtr = pS->Pop();
    const char *file_path_ptr = pString->GetString();
    if (file_path_ptr) {
        std::string file_path = file_path_ptr;

        const auto &config = LoadConfig(file_path);
        if (config) {
            auto *var = objectPtr->GetVarPointer();
            entid_t entity_id = var->GetEntityID();
            ConvertTable(var->GetAClass(), *config, entity_id);
            return IFUNCRESULT_OK;
        }
        else {
            return IFUNCRESULT_FAILED;
        }
    }
    else {
        return IFUNCRESULT_FAILED;
    }
}

uint32_t SaveConfigImpl(VS_STACK *pS) {
    auto *pString = (VDATA *)pS->Pop();
    auto* objectPtr = pS->Pop();
    const Data config_data = *objectPtr->GetVarPointer()->GetAClass();
    const bool success = SaveConfig(pString->GetString(), config_data);
    if (success)
    {
        return IFUNCRESULT_OK;
    }
    else
    {
        return IFUNCRESULT_FAILED;
    }
}

uint32_t FindConfigImpl(VS_STACK *pS) {
    auto *pString = (VDATA *)pS->Pop();
    const char *file_path_ptr = pString->GetString();
    if (file_path_ptr) {
        std::string file_path = file_path_ptr;
        auto config_file_found = FindConfigFile(file_path);
        if (config_file_found) {
            VDATA *result = pS->Push();
            result->Set(config_file_found->path.string());
            return IFUNCRESULT_OK;
        }
        else {
            return IFUNCRESULT_FAILED;
        }
    }
    else {
        return IFUNCRESULT_FAILED;
    }
}

uint32_t ListFilesImpl(VS_STACK *pS) {
    auto *pString = (VDATA *)pS->Pop();
    auto* objectPtr = pS->Pop();
    const char *root_dir_ptr = pString->GetString();
    if (root_dir_ptr) {
        auto *var = objectPtr->GetVarPointer();

        std::string file_path = fio->ConvertPathResource(root_dir_ptr);
        std::error_code ec;
        std::filesystem::directory_iterator it(file_path);

        auto *attr = var->GetAClass();
        auto *list_attr = attr->CreateSubAClass(attr, "files");
        size_t index = 0;
        for (const auto &entry : it) {
            std::string index_string = std::to_string(index++);
            std::u8string path_str_u8 = entry.path().filename().u8string();
            std::string path_str(path_str_u8.begin(), path_str_u8.end());
            list_attr->SetAttribute(index_string, path_str);
        }

        return ec ? IFUNCRESULT_FAILED : IFUNCRESULT_OK;
    }
    else {
        return IFUNCRESULT_FAILED;
    }
}

} // namespace

bool ConfigLibrary::Init()
{
    IFUNCINFO sIFuncInfo{};
    sIFuncInfo.nArguments = 2;
    sIFuncInfo.pFuncName = "LoadConfig";
    sIFuncInfo.pReturnValueName = "void";
    sIFuncInfo.pFuncAddress = LoadConfigImpl;
    core.SetScriptFunction(&sIFuncInfo);

    sIFuncInfo.nArguments = 2;
    sIFuncInfo.pFuncName = "SaveConfig";
    sIFuncInfo.pReturnValueName = "void";
    sIFuncInfo.pFuncAddress = SaveConfigImpl;
    core.SetScriptFunction(&sIFuncInfo);

    sIFuncInfo = {};
    sIFuncInfo.nArguments = 1;
    sIFuncInfo.pFuncName = "FindConfig";
    sIFuncInfo.pReturnValueName = "string";
    sIFuncInfo.pFuncAddress = FindConfigImpl;
    core.SetScriptFunction(&sIFuncInfo);

    sIFuncInfo = {};
    sIFuncInfo.nArguments = 2;
    sIFuncInfo.pFuncName = "ListFiles";
    sIFuncInfo.pReturnValueName = "void";
    sIFuncInfo.pFuncAddress = ListFilesImpl;
    core.SetScriptFunction(&sIFuncInfo);

    return true;
}

} // namespace storm
