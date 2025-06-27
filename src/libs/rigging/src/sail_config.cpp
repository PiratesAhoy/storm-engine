#include "sail_config.hpp"
#include "storm_assert.h"

void from_json(const storm::Data &node, Vector &vector)
{
    vector.x = 0;
    vector.y = 0;
    vector.z = 0;

    if (node.is_array() ) {
        if (!node.empty()) {
            vector.x = node[0].get<float>();
        }
        if (node.size() >= 2) {
            vector.y = node[1].get<float>();
        }
        if (node.size() >= 2) {
            vector.z = node[0].get<float>();
        }
    }
    else if (node.is_string() ) {
        auto value = node.get<std::string>();
        sscanf(value.c_str(), "%f,%f,%f", &vector.x, &vector.y, &vector.z);
    }
}

namespace storm::rigging {

void from_json(const Data &json, SailConfig &config)
{
    // load texture names
    config.texQuantity = json.value<int>("TextureCount", 1);
    if (config.texQuantity <= 0) {
        config.texQuantity = 1;
    }

    config.texNumCommon = json.value<int>("TexNumCommon", 0);
    config.texNumEnglish = json.value<int>("TexNumEnglish", 0);
    config.texNumTriangle = json.value<int>("TexNumTriangle", json.value<int>("TexNumTreangle", 0) );

    // load speed calculate parameters:
    config.holeDepend = json.value<double>("fHoleDepend", 1.0);
    config.ts = json.value<Vector>("TriangleWindSpeed", json.value<Vector>("TreangleWindSpeed", {0.2, 0.6, 0.8}) );
    config.fs = json.value<Vector>("TrapecidalWindSpeed", {0.4, 0.5, 0.6});
    config.ss = json.value<Vector>("SquareWindSpeed", {0.4, 0.1, 0.6});

    // load wind depend parameters
    config.SsailWindDepend = json.value<double>("fSsailWindDepend", 0.05);
    config.TsailWindDepend = json.value<double>("fTsailWindDepend", 0.5);
    config.fWindAdding = json.value<double>("fWindAdding", 0.3);
    config.FLEXSPEED = json.value<double>("FLEXSPEED", 0.001);
    config.MAXSUMWIND = json.value<double>("MAXSUMWIND", 0.02);
    config.WINDVECTOR_QUANTITY = json.value<int>("WINDVECTOR_QNT", 60);
    config.WINDVECTOR_TINCR = json.value<int>("WINDVECTOR_TINCR", 3);
    config.WINDVECTOR_TADD = json.value<int>("WINDVECTOR_TADD", 3);
    config.WINDVECTOR_SINCR = json.value<int>("WINDVECTOR_SINCR", 6);
    config.WINDVECTOR_SADD = json.value<int>("WINDVECTOR_SADD", 3);

    // load rolling sail parameters
    config.ROLL_Z_VAL = json.value<double>("ROLL_Z_VAL", 0.01);
    config.ROLL_Z_DELTA = json.value<double>("ROLL_Z_DELTA", 0.001);
    config.ROLL_Y_VAL = json.value<double>("ROLL_Y_VAL", 0.04);
    config.ROLL_Y_DELTA = json.value<double>("ROLL_Y_DELTA", 0.001);

    // sail turning parameters
    config.WINDANGL_DISCRETE = json.value<double>("WINDANGLDISCRETE", 0.01);
    config.MAXTURNANGL = json.value<double>("MAXTURNANGL", 0.6);
    config.TURNSTEPANGL = json.value<double>("TURNSTEPANGL", 0.002);
    config.ROLLINGSPEED = json.value<double>("ROLLINGSPEED", 0.0003);

    // load material parameters
    config.diffuseColor = config::GetColor(json["Diffuse"]).value_or(0);
    config.ambientColor = config::GetColor(json["Ambient"]).value_or(0);
    config.specularColor = config::GetColor(json["Specular"]).value_or(0);
    config.emissiveColor = config::GetColor(json["Emissive"]).value_or(0xb3b3b3b3);
    config.materialPower = json.value<double>("Power", 0.0);

    // load ROLLING SAIL form table
    // load square sail form
    config.SSailRollForm = json.value<std::vector<double>>("rollSSailForm", {0.2, 0.8, 1.0, 0.8, 0.4, 1.0, 1.3, 1.0, 0.4, 0.8, 1.0, 0.8, 0.2});
    config.SSailRollForm.resize(13, 0.0);
    config.TSailRollForm = json.value<std::vector<double>>("rollTSailForm", {0.1, 0.6, 0.3, 0.8, 0.2});
    config.TSailRollForm.resize(5, 0.0);
    config.TR_FORM_MUL = json.value<double>("tr_form_mul", 2.0);

    // load hole depend parameters
    config.fTHoleFlexDepend = json.value<double>("fTHoleFlexDepend", 0.01);
    if (config.fTHoleFlexDepend > .1) {
        config.fTHoleFlexDepend = 0.1;
    }
    config.fSHoleFlexDepend = json.value<double>("fSHoleFlexDepend", 0.01);
    if (config.fSHoleFlexDepend > 1.0 / 12.0) {
        config.fSHoleFlexDepend = 1.0 / 12.0;
    }

    // load parameter for sails of fall mast
    // square sails
    config.FALL_SSAIL_ADD_MIN = json.value<double>("fFallSSailAddMin", 0.2);
    config.FALL_SSAIL_ADD_RAND = json.value<double>("fFallSSailAddRand", 0.2);
    // triangle sails
    config.FALL_TSAIL_ADD_MIN = json.value<double>("fFallTSailAddMin", 0.2);
    config.FALL_TSAIL_ADD_RAND = json.value<double>("fFallTSailAddRand", 0.2);

    config.GROUP_UPDATE_TIME = json.value<int>("msecSailUpdateTime", 4000);
}

} // namespace storm::rigging
