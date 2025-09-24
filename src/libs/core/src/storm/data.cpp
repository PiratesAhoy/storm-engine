#include "storm/data.hpp"

#include "attributes.h"

void to_json(storm::Data &data, const ATTRIBUTES &attributes)
{
    if (attributes.GetAttributesNum() > 0)
    {
        if (attributes.HasValue() && !attributes.GetValue().empty())
        {
            data["__value__"] = attributes.GetValue();
        }

        for (size_t i = 0; i < attributes.GetAttributesNum(); i++)
        {
            const ATTRIBUTES* child = attributes.GetAttributeClass(i);
            std::string key = attributes.GetAttributeName(i);
            if (child != nullptr)
            {
                data[key] = *child;
            }
            else
            {
                data[key] = nullptr;
            }
        }
    }
    else
    {
        data = attributes.GetValue();
    }
}
