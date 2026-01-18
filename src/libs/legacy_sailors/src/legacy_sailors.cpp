#include "legacy_sailors.hpp"

#include "animation.h"
#include "core.h"
#include "math_inlines.h"
#include "rands.h"
#include "shared/messages.h"
#include "shared/sea_ai/script_defines.h"
#include "shared/sea_ai/sea_people.h"
#include "string_compare.hpp"
#include "vma.hpp"

CREATE_CLASS(LegacySailors)

namespace
{
constexpr auto Crew2Visible(const auto n)
{
    return 5 * log(n);
}

constexpr std::array CharacterModels = {
    "Lowcharacters\\Lo_Man_1",
    "Lowcharacters\\Lo_Man_2",
    "Lowcharacters\\Lo_Man_3",
    "Lowcharacters\\Lo_Man_Kamzol_1",
    "Lowcharacters\\Lo_Man_Kamzol_2",
    "Lowcharacters\\Lo_Man_Kamzol_3",
};

constexpr float CREW_PART[] = {0.3f, 0.5f, 0.1f, 1.0f};
constexpr auto MAN_SPEED_DEVIATION_K = 0.35f;
constexpr auto MAN_MIN_TURN = 0.35f;
constexpr auto MAX_VIEW_DISTANCE = 150.0f;

enum LOCATOR_TYPE
{
    LOCATOR_WALK = 0,
    LOCATOR_CANNON = 1,
    LOCATOR_DOOR = 2,
    LOCATOR_SPAWN = 3,
    LOCATOR_TOP = 4,
    LOCATOR_VANT = 5,
    LOCATOR_MARS = 6,
};

float Vector2Angle(const CVECTOR &_v)
{
    auto result = atan2(_v.x, _v.z);

    while (result >= PI * 2)
        result -= PI * 2;
    while (result < 0)
        result += PI * 2;

    return result;
}

}

LegacySailors::LegacySailors() = default;

LegacySailors::~LegacySailors()
{
    for (auto &walk : shipWalks_)
    {
        for (size_t man = 0; man < walk.crewCount; ++man)
        {
            if (walk.crew[man].modelID)
            {
                core.EraseEntity(walk.crew[man].modelID);
            }
        }
    }
}

bool LegacySailors::Init()
{
    return true;
}

void LegacySailors::ProcessStage(Stage stage, uint32_t delta)
{
    if (stage == Stage::realize)
    {
        Realize(delta);
    }
}

uint32_t LegacySailors::AttributeChanged(ATTRIBUTES *attributes)
{
    const std::string_view attribute_name = attributes->GetThisName();

    if (storm::iEquals(attribute_name, "IsOnDeck"))
    {
        cameraOnDeck_ = (attributes->GetAttributeAsDword() != 0);

        for (auto &walk : shipWalks_)
        {
            if (walk.ship->GetACharacter()->GetAttribute("MainCharacter"))
            {
                walk.myShip = true;
            }
            else
            {
                walk.myShip = false;
            }
        }
    }
    else if (storm::iEquals(attribute_name, "HidePeopleOnDeck"))
    {
        hidePeopleOnDeck_ = (attributes->GetAttributeAsDword() != 0);
    }
    else if (storm::iEquals(attribute_name, "isNight"))
    {
        isNight_ = (attributes->GetAttributeAsDword() != 0);
    }

    return Entity::AttributeChanged(attributes);
}

uint64_t LegacySailors::ProcessMessage(MESSAGE &message)
{
    const auto msgId = message.Long();
    switch (msgId)
    {
    case AI_MESSAGE_ADD_SHIP: {
        const auto ship_id = message.EntityID();
        const int vertex_count = message.Long();
        VDATA *vertex_array = message.ScriptVariablePointer();
        VDATA *group_array = message.ScriptVariablePointer();
        VDATA *type_array = message.ScriptVariablePointer();
        AddShipWalk(ship_id, vertex_count, vertex_array, group_array, type_array);
        break;
    }
    case MSG_SHIP_DELETE: {
        if (auto *const attrs = message.AttributePointer())
        {
            for (auto &walk : shipWalks_)
            {
                if (walk.ship && walk.ship->GetACharacter() == attrs)
                {
                    walk.enabled = false;
                    break;
                }
            }
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

void LegacySailors::Realize(uint32_t)
{
    // Legacy implementation will be added during the port.
}

void LegacySailors::SetShipState(tShipWalk &walk, int state)
{
    if (walk.state == state)
    {
        return;
    }

    walk.state = state;
    int old_show_count = walk.showCount;
    int newShowCount = static_cast<int>(walk.crewCount * CREW_PART[state]);

    if (newShowCount > old_show_count)
    {
        walk.showCount = newShowCount;
        size_t vert_index_1 = 0;
        int vert_index_2 = 0;
        for (vert_index_1 = 0; vert_index_1 < walk.verts.size(); ++vert_index_1)
        {
            if (walk.vertTypes[vert_index_1] == LOCATOR_SPAWN)
            {
                break;
            }
        }
        for (vert_index_2 = static_cast<int>(walk.verts.size()) - 1; vert_index_2 >= 0; --vert_index_2)
        {
            if (walk.vertTypes[static_cast<size_t>(vert_index_2)] == LOCATOR_SPAWN)
            {
                break;
            }
        }

        int vert_index = 0;
        for (int i = old_show_count; i < walk.showCount; ++i)
        {
            if (rand() & 0x1)
            {
                vert_index = vert_index_1;
            }
            else
            {
                vert_index = vert_index_2;
            }

            if (walk.crew[i].sourceI == -1)
            {
                vert_index = walk.graph.FindRandomWithType(LOCATOR_WALK);
            }
            walk.crew[i].sourceI = vert_index;
            walk.crew[i].destI = walk.graph.FindRandomLinkedAnyType(vert_index);
            walk.crew[i].pos = walk.verts[vert_index];
            walk.crew[i].dir = !(walk.verts[walk.crew[i].destI] - walk.crew[i].pos);
            walk.crew[i].ang = Vector2Angle(walk.crew[i].dir);
            walk.crew[i].speed = manSpeed_ + randCentered((manSpeed_ * MAN_SPEED_DEVIATION_K));
            walk.crew[i].state = MAN_RUN;
            // walk.crew[i].destI = vert_index;
            ChooseNewAction(walk, walk.crew[i]);
        }

        // Others shall come out from door
        for (int i = walk.showCount; i < MAX_PEOPLE; ++i)
        {
            walk.crew[i].sourceI = vert_index;
        }
    }
}

void LegacySailors::AddShipWalk(entid_t ship_id, int vertex_count, VDATA *vertex_array, VDATA *group_array,
                                VDATA *type_array)
{
    auto ship_ptr = static_cast<SHIP_BASE *>(core.GetEntityPointer(ship_id));

    shipWalks_.push_back(tShipWalk{
        .ship = ship_ptr,
        .shipModel = ship_ptr->GetModel(),
    });
    auto& ship_walk = shipWalks_.back();

    // Getting graph vertices
    for (auto i = 0; i < vertex_count; i++)
    {
        float x{};
        float y{};
        float z{};
        int32_t vType{};
        vertex_array->Get(x, i * 3);
        vertex_array->Get(y, i * 3 + 1);
        vertex_array->Get(z, i * 3 + 2);
        type_array->Get(vType, i);
        ship_walk.verts.push_back(CVECTOR(x, y, z));
        ship_walk.vertTypes.push_back(vType);
        ship_walk.vertBusy.push_back(false);
    }

    // Getting links between graph vertices
    const auto link_count = group_array->GetElementsNum();
    for (auto i = 0; i < link_count; i++)
    {
        int32_t couple{};
        group_array->Get(couple, i);
        int v1{};
        int v2{};
        v1 = (couple >> 8) & 0xff;
        v2 = couple & 0xff;
        if (couple > 0 && v1 < vertex_count && v2 < vertex_count)
        {
            ship_walk.graph.AddPair(v1, v2, ship_walk.vertTypes[v1], ship_walk.vertTypes[v2]);
        }
    }

    // Visible crew count
    ATTRIBUTES *attr = ship_walk.ship->GetACharacter();
    ATTRIBUTES *crew_attr = attr->FindAClass(attr, "ship.crew");
    ship_walk.crewCount = static_cast<size_t>(ceilf(Crew2Visible(crew_attr->GetAttributeAsFloat("quantity", 1))));
    if (ship_walk.crewCount > MAX_PEOPLE)
    {
        ship_walk.crewCount = MAX_PEOPLE;
    }

    for (int i = 0; i < ship_walk.crewCount; i++)
    {
        InitShipMan(ship_walk, i);
    }

    ship_walk.state = -1;
    ship_walk.showCount = 0;
    ship_walk.enabled = true;

    SetShipState(ship_walk, SHIP_SAIL);
}

void LegacySailors::InitShipMan(tShipWalk& walk, int man)
{
    walk.crew[man].modelID = core.CreateEntity("MODELR");

    core.Send_Message(walk.crew[man].modelID, "ls", MSG_MODEL_LOAD_GEO,
                      CharacterModels[rand() % CharacterModels.size()]);
    core.Send_Message(walk.crew[man].modelID, "ls", MSG_MODEL_LOAD_ANI, "Lo_Man");

    auto *man_model = static_cast<MODEL*>(core.GetEntityPointer(walk.crew[man].modelID));
    auto &animation_player = man_model->GetAnimation()->Player(0);
    animation_player.SetAction("walk");
    animation_player.SetPosition(storm::RandomFloat());

    if (manSpeed_ == 0)
    {
        const char *walk_speed_data = animation_player.GetData("Walk speed");
        manSpeed_ = std::stof(walk_speed_data);

        const char *turn_speed_data = animation_player.GetData("Turn speed");
        manTurnSpeed_ = std::stof(turn_speed_data);

        manSpeed_ /= 1e3f;
        manTurnSpeed_ /= 1e3f;
        approachDistance_ = 1e4f * manSpeed_;
    }

    walk.crew[man].model = man_model;
    walk.crew[man].visible = false;
    walk.crew[man].sourceI = -1;
}
