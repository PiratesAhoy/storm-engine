#pragma once

#include "dx9render.h"
#include "entity_with_renderer.h"
#include "ship_base.h"
#include "walk_graph.hpp"

#include <array>
#include <cstdint>

constexpr size_t MAX_VERTS = 100;
constexpr size_t MAX_PEOPLE = 100;

enum LegacyManState
{
    // common actions (chosen randomly)
    MAN_WALK,
    MAN_TURN_LEFT,
    MAN_TURN_RIGHT,
    MAN_RUN,
    MAN_STAND1,
    MAN_STAND2,
    MAN_STAND3,
    MAN_STAND4,
    // special actions
    MAN_RELOAD,
    // animated action sequences
    MAN_CRAWL_WALK_VANT,
    MAN_CRAWL_VANT_TOP,
    MAN_CRAWL_TOP_MARS,
    MAN_CRAWL_MARS,
    MAN_CRAWL_MARS_TOP,
    MAN_CRAWL_TOP_VANT,
    MAN_CRAWL_VANT_WALK
};

struct LegacyShipMan
{
    int sourceI{};
    int destI{};
    CVECTOR pos{};
    float ang{};
    CVECTOR dir{};
    float speed{};
    MODEL *model{};
    LegacyManState state{};
    float newAngle{};
    bool visible{};
    entid_t modelID{};
};

struct LegacyShipWalk
{
    SHIP_BASE *ship{};
    MODEL *shipModel{};
    WalkGraph graph{};
    std::vector<CVECTOR> verts;
    std::vector<uint8_t> vertTypes;
    std::vector<std::uintptr_t> vertBusy;
    size_t crewCount{};
    size_t showCount{};
    LegacyShipMan crew[MAX_PEOPLE]{};
    int state{};
    bool enabled{};
    bool myShip{false};
};

class LegacySailors : public EntityWithRenderer
{
  public:
    LegacySailors();
    ~LegacySailors() override;

    bool Init() override;
    void ProcessStage(Stage stage, uint32_t delta) override;
    uint32_t AttributeChanged(ATTRIBUTES *attributes) override;
    uint64_t ProcessMessage(MESSAGE &message) override;

  private:
    void Realize(uint32_t delta);

    bool ProcessManCrawl(LegacyShipWalk &walk, LegacyShipMan &man, uint32_t delta);
    bool ProcessManWalk(LegacyShipWalk &walk, LegacyShipMan &man, uint32_t delta);
    bool ProcessManTurn(LegacyShipWalk &walk, LegacyShipMan &man, uint32_t delta);
    bool ProcessManStand(LegacyShipWalk &walk, LegacyShipMan &man);
    void SetManVisible(LegacyShipMan &man, bool visible);

    void SetShipState(LegacyShipWalk &walk, int state);

    void ChooseNewAction(LegacyShipWalk &walk, LegacyShipMan &man);

    void AddShipWalk(entid_t ship_id, int vertex_count, VDATA *vertex_array, VDATA *group_array, VDATA *type_array);
    void InitShipMan(LegacyShipWalk &walk, int man);

    std::vector<LegacyShipWalk> shipWalks_;

    float manSpeed_{};
    float manRunSpeed_{};
    float manTurnSpeed_{};
    float approachDistance_{};

    std::array<PLANE, 4> clipPlanes_{};
    CVECTOR cameraPosition_{};

    bool cameraOnDeck_ = false;
    bool hidePeopleOnDeck_ = true;
    bool isNight_ = false;
};
