#include "legacy_sailors.hpp"

#include <cstdlib>

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
constexpr auto MAN_STATE_COUNT = 10;
constexpr auto SHIP_STATE_COUNT = 4;

constexpr float CREW_PART_PROBS[SHIP_STATE_COUNT][MAN_STATE_COUNT] = {
    {0.7f, 0.0f, 0.0f, 0.1f, 0.05f, 0.05f, 0.05f, 0.05f, 0.0f, 0.30f}, // SAIL
    {0.0f, 0.0f, 0.0f, 0.5f, 0.00f, 0.00f, 0.00f, 0.00f, 0.99f, 0.10f}, // WAR
    {0.1f, 0.0f, 0.0f, 0.9f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.05f}, // STORM
    {0.0f, 0.0f, 0.0f, 1.0f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.10f}, // ABORDAGE
};

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

constexpr auto PI2 = PI * 2.0f;
constexpr auto PID2 = PI / 2.0f;

float Vector2Angle(const CVECTOR &_v)
{
    return atan2(_v.x, _v.z);
}

bool SphereVisible(const PLANE &plane, const CVECTOR &center, float radius)
{
    const CVECTOR plane_vec(plane.Nx, plane.Ny, plane.Nz);
    return ((plane_vec | center) - plane.D) > -radius;
}

int GetRandomByProbs(int state, bool allow_crawl)
{
    if (!allow_crawl)
    {
        const int r = rand() % 100;
        int n = 0;
        int i = 0;
        for (i = 0; i < MAN_STATE_COUNT; ++i)
        {
            n += static_cast<int>(100.0f * CREW_PART_PROBS[state][i]);
            if (r < n)
            {
                return i;
            }
        }
        return i;
    }

    const int r = rand() % 100;
    if (r < static_cast<int>(100.0f * CREW_PART_PROBS[state][MAN_CRAWL_WALK_VANT]))
    {
        return MAN_CRAWL_WALK_VANT;
    }

    return MAN_RUN;
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
    if (stage == Stage::execute)
    {
        for (auto &walk : shipWalks_)
        {
            if (!walk.enabled)
            {
                continue;
            }

            ATTRIBUTES *ship_attr = walk.ship->GetACharacter();
            ATTRIBUTES *ship_mode_attr = ship_attr->FindAClass(ship_attr, "ship.POS.mode");
            if (ship_mode_attr)
            {
                const int ship_mode = ship_mode_attr->GetAttributeAsDword();
                SetShipState(walk, ship_mode);
            }
            else
            {
                SetShipState(walk, SHIP_WAR);
            }

            for (int man = 0; man < static_cast<int>(walk.showCount); ++man)
            {
                switch (walk.crew[man].state)
                {
                case MAN_WALK:
                    if (!ProcessManWalk(walk, walk.crew[man], delta))
                    {
                        ChooseNewAction(walk, walk.crew[man]);
                    }
                    break;
                case MAN_CRAWL_WALK_VANT:
                case MAN_CRAWL_VANT_TOP:
                case MAN_CRAWL_TOP_MARS:
                case MAN_CRAWL_MARS:
                case MAN_CRAWL_MARS_TOP:
                case MAN_CRAWL_TOP_VANT:
                case MAN_CRAWL_VANT_WALK:
                    if (!ProcessManCrawl(walk, walk.crew[man], delta))
                    {
                        ChooseNewAction(walk, walk.crew[man]);
                    }
                    break;
                case MAN_TURN_LEFT:
                case MAN_TURN_RIGHT:
                    if (!ProcessManTurn(walk, walk.crew[man], delta))
                    {
                        ChooseNewAction(walk, walk.crew[man]);
                    }
                    break;
                case MAN_RUN:
                    if (!ProcessManWalk(walk, walk.crew[man], delta))
                    {
                        ChooseNewAction(walk, walk.crew[man]);
                    }
                    break;
                case MAN_STAND1:
                case MAN_STAND2:
                case MAN_STAND3:
                case MAN_STAND4:
                case MAN_RELOAD:
                    if (!ProcessManStand(walk, walk.crew[man]))
                    {
                        ChooseNewAction(walk, walk.crew[man]);
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
    else if (stage == Stage::realize)
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

void LegacySailors::Realize(uint32_t delta)
{
    CMatrix m;
    CMatrix m2;
    const CVECTOR man_center(0.0f, 1.0f, 0.0f);
    CVECTOR man_pos;

    int people_count = 0;

    CVECTOR ang;
    float persp = 0.0f;
    auto *renderer = Renderer();
    renderer->GetCamera(cameraPosition_, ang, persp);

    // Update frustum
    PLANE *planes = renderer->GetPlanes();
    clipPlanes_[0] = planes[0];
    clipPlanes_[1] = planes[1];
    clipPlanes_[2] = planes[2];
    clipPlanes_[3] = planes[3];

    for (auto &walk : shipWalks_)
    {
        if (!walk.enabled)
        {
            continue;
        }

        if (hidePeopleOnDeck_)
        {
            if (cameraOnDeck_ && walk.myShip)
            {
                continue;
            }
        }

        walk.ship->SetLights();

        for (int man = 0; man < static_cast<int>(walk.showCount); ++man)
        {
            if (!walk.crew[man].model)
            {
                continue;
            }

            m.BuildPosition(walk.crew[man].pos.x, walk.crew[man].pos.y, walk.crew[man].pos.z);
            m2.BuildRotateY(walk.crew[man].ang);
            walk.crew[man].model->mtx = m2 * m * walk.shipModel->mtx;
            man_pos = walk.crew[man].model->mtx * man_center;

            if (tanf(persp * 0.5f) * ~(man_pos - cameraPosition_) > (MAX_VIEW_DISTANCE * MAX_VIEW_DISTANCE))
            {
                SetManVisible(walk.crew[man], false);
                continue;
            }
            if (!SphereVisible(clipPlanes_[0], man_pos, 1.0f) ||
                !SphereVisible(clipPlanes_[1], man_pos, 1.0f) ||
                !SphereVisible(clipPlanes_[2], man_pos, 1.0f) ||
                !SphereVisible(clipPlanes_[3], man_pos, 1.0f))
            {
                SetManVisible(walk.crew[man], false);
                continue;
            }

            SetManVisible(walk.crew[man], true);
            walk.crew[man].model->ProcessStage(Stage::realize, delta);
            ++people_count;
        }

        walk.ship->UnSetLights();
    }
}

bool LegacySailors::ProcessManWalk(LegacyShipWalk &walk, LegacyShipMan &man, uint32_t delta)
{
    const float d_now = SQR(man.pos.x - walk.verts[man.destI].x) + SQR(man.pos.y - walk.verts[man.destI].y) +
                        SQR(man.pos.z - walk.verts[man.destI].z);
    const float dir_vect = man.speed * delta;
    const float d_future =
        SQR(man.pos.x + dir_vect * man.dir.x - walk.verts[man.destI].x) +
        SQR(man.pos.y + dir_vect * man.dir.y - walk.verts[man.destI].y) +
        SQR(man.pos.z + dir_vect * man.dir.z - walk.verts[man.destI].z);

    if (d_future < d_now)
    {
        man.pos += dir_vect * man.dir;
        return true;
    }

    man.sourceI = man.destI;
    return false;
}

bool LegacySailors::ProcessManCrawl(LegacyShipWalk &walk, LegacyShipMan &man, uint32_t delta)
{
    switch (man.state)
    {
    case MAN_CRAWL_WALK_VANT:
    case MAN_CRAWL_VANT_TOP:
    case MAN_CRAWL_TOP_MARS:
    case MAN_CRAWL_MARS_TOP:
    case MAN_CRAWL_TOP_VANT:
    case MAN_CRAWL_VANT_WALK:
        return ProcessManWalk(walk, man, delta);
    case MAN_CRAWL_MARS:
        return ProcessManStand(walk, man);
    default:
        break;
    }

    return false;
}

bool LegacySailors::ProcessManTurn(LegacyShipWalk &, LegacyShipMan &man, uint32_t delta)
{
    if (fabs(man.ang - man.newAngle) >= MAN_MIN_TURN)
    {
        man.ang += man.speed * delta;
        if (man.ang > PI)
        {
            man.ang = -(PI2 - man.ang);
        }
        if (man.ang < -PI)
        {
            man.ang = PI2 + man.ang;
        }
        return true;
    }

    return false;
}

bool LegacySailors::ProcessManStand(LegacyShipWalk &walk, LegacyShipMan &man)
{
    const bool playing = man.model->GetAnimation()->Player(0).IsPlaying();
    if (!playing && man.state == MAN_RELOAD)
    {
        walk.vertBusy[man.sourceI] = 0;
    }
    return playing;
}

void LegacySailors::SetManVisible(LegacyShipMan &man, bool visible)
{
    if (man.visible == visible)
    {
        return;
    }

    auto &player = man.model->GetAnimation()->Player(0);
    if (!visible)
    {
        player.Stop();
    }
    else
    {
        player.Play();
    }

    man.visible = visible;
}

void LegacySailors::SetShipState(LegacyShipWalk &walk, int state)
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
            walk.crew[i].destI = vert_index;
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

    shipWalks_.push_back(LegacyShipWalk{
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
        ship_walk.vertBusy.push_back(0);
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
    ship_walk.crewCount = static_cast<size_t>(ceilf(Crew2Visible(crew_attr->GetAttributeAsDword("quantity", 1))));
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

void LegacySailors::ChooseNewAction(LegacyShipWalk &walk, LegacyShipMan &man)
{
    if (man.state == MAN_WALK || man.state == MAN_RUN)
    {
        man.state = MAN_TURN_LEFT;
    }
    else if (man.state == MAN_CRAWL_WALK_VANT)
    {
        man.state = MAN_CRAWL_VANT_TOP;
    }
    else if (man.state == MAN_CRAWL_VANT_TOP)
    {
        man.state = MAN_CRAWL_TOP_MARS;
    }
    else if (man.state == MAN_CRAWL_TOP_MARS)
    {
        man.state = MAN_CRAWL_MARS;
    }
    else if (man.state == MAN_CRAWL_MARS)
    {
        man.state = MAN_CRAWL_MARS_TOP;
    }
    else if (man.state == MAN_CRAWL_MARS_TOP)
    {
        man.state = MAN_CRAWL_TOP_VANT;
    }
    else if (man.state == MAN_CRAWL_TOP_VANT)
    {
        man.state = MAN_CRAWL_VANT_WALK;
    }
    else
    {
        const LegacyManState old_state = man.state;
        const int dst = walk.graph.FindRandomLinkedAnyType(man.sourceI);
        if (man.state != MAN_RELOAD)
        {
            man.state = static_cast<LegacyManState>(GetRandomByProbs(walk.state, walk.vertTypes[dst] == LOCATOR_VANT));
        }

        if (man.state == MAN_RELOAD && walk.vertTypes[man.sourceI] == LOCATOR_CANNON && !walk.vertBusy[man.sourceI])
        {
            walk.vertBusy[man.sourceI] = reinterpret_cast<std::uintptr_t>(&man);
            if (man.pos.x < 0.0f)
            {
                man.ang = -PID2;
                man.dir = CVECTOR(-1.0f, 0.0f, 0.0f);
            }
            else
            {
                man.ang = PID2;
                man.dir = CVECTOR(1.0f, 0.0f, 0.0f);
            }
        }
        else if (man.state == MAN_CRAWL_WALK_VANT)
        {
            // crawl transition handled below
        }
        else
        {
            while (static_cast<int>(man.state) > static_cast<int>(MAN_STAND4))
            {
                man.state = static_cast<LegacyManState>(GetRandomByProbs(walk.state, false));
            }
        }
    }

    switch (man.state)
    {
    case MAN_TURN_LEFT: {
        man.sourceI = man.destI;
        man.destI = walk.graph.FindRandomLinkedWithoutType(man.sourceI, LOCATOR_VANT);
        man.newAngle = Vector2Angle(!(walk.verts[man.destI] - man.pos));
        if (man.newAngle > PI)
        {
            man.newAngle = -(PI2 - man.newAngle);
        }
        if (man.newAngle < -PI)
        {
            man.newAngle = PI2 + man.newAngle;
        }
        if ((man.newAngle - man.ang) < 0.0f)
        {
            man.speed = -manTurnSpeed_;
            man.state = MAN_TURN_RIGHT;
            man.model->GetAnimation()->Player(0).SetAction("turn_left");
        }
        else
        {
            man.speed = manTurnSpeed_;
            man.state = MAN_TURN_LEFT;
            man.model->GetAnimation()->Player(0).SetAction("turn_right");
        }
        break;
    }
    case MAN_WALK:
        if (man.destI == -1)
        {
            man.visible = false;
        }
        man.dir = !(walk.verts[man.destI] - man.pos);
        man.ang = Vector2Angle(man.dir);
        man.speed = manSpeed_ + randCentered(manSpeed_ * MAN_SPEED_DEVIATION_K);
        man.model->GetAnimation()->Player(0).SetAction("walk");
        break;
    case MAN_RUN:
        if (man.destI == -1)
        {
            man.visible = false;
        }
        man.dir = !(walk.verts[man.destI] - man.pos);
        man.ang = Vector2Angle(man.dir);
        man.speed = 2.0f * (manSpeed_ + randCentered(manSpeed_ * MAN_SPEED_DEVIATION_K));
        man.model->GetAnimation()->Player(0).SetAction("run");
        break;
    case MAN_STAND1:
        man.model->GetAnimation()->Player(0).SetAction("action1");
        break;
    case MAN_STAND2:
        man.model->GetAnimation()->Player(0).SetAction("action2");
        break;
    case MAN_STAND3:
        man.model->GetAnimation()->Player(0).SetAction("action3");
        break;
    case MAN_STAND4:
        man.model->GetAnimation()->Player(0).SetAction("action4");
        break;
    case MAN_RELOAD:
        man.model->GetAnimation()->Player(0).SetAction("reload");
        break;
    case MAN_CRAWL_WALK_VANT:
        man.destI = walk.graph.FindRandomLinkedWithType(man.sourceI, LOCATOR_VANT);
        if (man.destI == -1)
        {
            man.visible = false;
        }
        man.dir = !(walk.verts[man.destI] - man.pos);
        man.ang = Vector2Angle(man.dir);
        man.speed = 2.0f * (manSpeed_ + randCentered(manSpeed_ * MAN_SPEED_DEVIATION_K));
        man.model->GetAnimation()->Player(0).SetAction("run");
        break;
    case MAN_CRAWL_VANT_TOP:
        man.destI = walk.graph.FindRandomLinkedWithType(man.sourceI, LOCATOR_TOP);
        if (man.destI == -1)
        {
            man.visible = false;
        }
        man.dir = !(walk.verts[man.destI] - man.pos);
        if (man.pos.x < 0.0f)
        {
            man.ang = PID2;
        }
        else
        {
            man.ang = PI + PID2;
        }
        man.speed = 0.5f * (manSpeed_ + randCentered(manSpeed_ * MAN_SPEED_DEVIATION_K));
        man.model->GetAnimation()->Player(0).SetAction("crawl");
        break;
    case MAN_CRAWL_TOP_MARS:
        man.destI = walk.graph.FindRandomLinkedWithType(man.sourceI, LOCATOR_MARS);
        if (man.destI == -1)
        {
            man.visible = false;
        }
        man.dir = !(walk.verts[man.destI] - man.pos);
        man.ang = Vector2Angle(man.dir);
        man.model->GetAnimation()->Player(0).SetAction("crawl");
        break;
    case MAN_CRAWL_MARS:
        man.model->GetAnimation()->Player(0).SetAction("action1");
        break;
    case MAN_CRAWL_MARS_TOP:
        man.destI = walk.graph.FindRandomLinkedWithType(man.sourceI, LOCATOR_TOP);
        if (man.destI == -1)
        {
            man.visible = false;
        }
        man.dir = !(walk.verts[man.destI] - man.pos);
        man.ang = PI + PID2 * static_cast<int>(Vector2Angle(man.dir) / PID2);
        man.model->GetAnimation()->Player(0).SetAction("crawl_down");
        break;
    case MAN_CRAWL_TOP_VANT:
        man.destI = walk.graph.FindRandomLinkedWithType(man.sourceI, LOCATOR_VANT);
        if (man.destI == -1)
        {
            man.visible = false;
        }
        man.dir = !(walk.verts[man.destI] - man.pos);
        man.ang = PI + Vector2Angle(man.dir);
        man.model->GetAnimation()->Player(0).SetAction("crawl_down");
        break;
    case MAN_CRAWL_VANT_WALK:
        man.destI = walk.graph.FindRandomLinkedWithType(man.sourceI, LOCATOR_WALK);
        if (man.destI == -1)
        {
            man.visible = false;
        }
        if (man.destI == man.sourceI)
        {
            man.destI = walk.graph.FindRandomLinkedWithType(man.sourceI, LOCATOR_CANNON);
        }
        man.dir = !(walk.verts[man.destI] - man.pos);
        man.ang = Vector2Angle(man.dir);
        man.speed = 2.0f * (manSpeed_ + randCentered(manSpeed_ * MAN_SPEED_DEVIATION_K));
        man.model->GetAnimation()->Player(0).SetAction("run");
        break;
    default:
        break;
    }

    man.visible = false;
}

void LegacySailors::InitShipMan(LegacyShipWalk& walk, int man)
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
        if (walk_speed_data)
        {
            manSpeed_ = std::strtof(walk_speed_data, nullptr);
        }

        const char *turn_speed_data = animation_player.GetData("Turn speed");
        if (turn_speed_data)
        {
            manTurnSpeed_ = std::strtof(turn_speed_data, nullptr);
        }

        manSpeed_ /= 1e3f;
        manTurnSpeed_ /= 1e3f;
        approachDistance_ = 1e4f * manSpeed_;
    }

    walk.crew[man].model = man_model;
    walk.crew[man].visible = false;
    walk.crew[man].sourceI = -1;
}
