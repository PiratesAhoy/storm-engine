#pragma once

#include <math3d/vector.h>
#include <storm/config/config.hpp>
#include <storm/data.hpp>

#include <vector>

namespace storm::rigging {

struct SailConfig {
    // parameters loaded from INI file //
    // --------------------------------------
    // float sailDownStep;
    double SsailWindDepend = 0.05;
    double TsailWindDepend = 0.5;
    double FLEXSPEED = 0.001;
    double MAXSUMWIND = 0.02;
    double WINDANGL_DISCRETE = 0.01;
    double MAXTURNANGL = 0.6;
    double TURNSTEPANGL = 0.002;
    double ROLLINGSPEED = 0.0003;
    int WINDVECTOR_TINCR = 3;
    int WINDVECTOR_TADD = 3;
    int WINDVECTOR_SINCR = 6;
    int WINDVECTOR_SADD = 3;
    int WINDVECTOR_QUANTITY = 60;
    int texQuantity = 1;
    int texNumCommon = 0;
    int texNumEnglish = 0;
    int texNumTriangle = 0;
    double ROLL_Z_VAL = 0.01;
    double ROLL_Z_DELTA = 0.001;
    double ROLL_Y_VAL = 0.04;
    double ROLL_Y_DELTA = 0.001;
    std::vector<double> SSailRollForm;
    std::vector<double> TSailRollForm;
    double TR_FORM_MUL = 2.0;
    double fWindAdding = 0.3;
    double fTHoleFlexDepend = 0.01;
    double fSHoleFlexDepend = 0.01;
    double FALL_SSAIL_ADD_MIN = 0.2;
    double FALL_SSAIL_ADD_RAND = 0.2;
    double FALL_TSAIL_ADD_MIN = 0.2;
    double FALL_TSAIL_ADD_RAND = 0.2;

    double holeDepend{}; // g_fSailHoleDepend
    Vector ts;
    Vector fs;
    Vector ss;

    uint32_t diffuseColor{};
    uint32_t ambientColor{};
    uint32_t specularColor{};
    uint32_t emissiveColor{};
    double materialPower{};

    int GROUP_UPDATE_TIME = 4000;
};

//void to_json(Data &json, const SailConfig &config);
void from_json(const Data &json, SailConfig &config);

} // namespace storm::rigging
