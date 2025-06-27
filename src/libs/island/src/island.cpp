#include "island.h"

#include "core.h"
#include "foam.h"
#include "math_inlines.h"
#include "shared/messages.h"
#include "shared/sea_ai/script_defines.h"
#include "tga.h"
#include "weather_base.h"
#include <cstdio>
#include <imgui.h>

using storm::Sqr;

CREATE_CLASS(ISLAND)

CREATE_CLASS(CoastFoam)

#define HMAP_EMPTY 0
#define HMAP_START 2.0f
#define HMAP_NUMBERS (255.0f - HMAP_START)
#define HMAP_MAXHEIGHT -20.0f

#define SEA_BED_NODE_NAME "seabed"

ISLAND::~ISLAND()
{
    Uninit();
}

void ISLAND::Uninit()
{
    if (!bForeignModels)
    {
        core.EraseEntity(model_id);
        core.EraseEntity(seabed_id);
    }
}

bool ISLAND::Init()
{
    // core.AddToLayer("system_messages", GetId(), 1);
    SetDevice();

    // calc optimization
    for (uint32_t i = 0; i < 256; i++)
    {
        fDepthHeight[i] = static_cast<float>(HMAP_MAXHEIGHT / HMAP_NUMBERS * (static_cast<float>(i) - HMAP_START));
        if (i == HMAP_EMPTY)
            fDepthHeight[i] = HMAP_MAXHEIGHT;
    }

    return true;
}

void ISLAND::SetDevice()
{
    // core.LayerCreate("island_trace", true, false);

    pCollide = static_cast<COLLIDE *>(core.GetService("COLL"));
    Assert(pCollide);
    pRS = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    Assert(pRS);
    pGS = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    Assert(pGS);
}

void ISLAND::Realize(uint32_t Delta_Time)
{
    uint32_t dwAmbient, dwAmbientOld;

    if (bForeignModels)
        return;

    auto *pModel = static_cast<MODEL *>(core.GetEntityPointer(model_id));
    Assert(pModel);

    uint32_t bFogEnable;
    uint32_t bLighting;
    pRS->GetRenderState(D3DRS_FOGENABLE, &bFogEnable);
    pRS->GetRenderState(D3DRS_AMBIENT, &dwAmbientOld);
    pRS->GetRenderState(D3DRS_LIGHTING, &bLighting);

    pRS->SetRenderState(D3DRS_LIGHTING, dynamicLightsOn);
    dwAmbient = dwAmbientOld & 0xFF;

    CVECTOR vCamPos, vCamAng;
    float fOldNear, fOldFar, fPerspective;
    pRS->GetCamera(vCamPos, vCamAng, fPerspective);
    pRS->GetNearFarPlane(fOldNear, fOldFar);

    auto fRadius = sqrtf(Sqr(vBoxSize.x / 2.0f) + Sqr(vBoxSize.z / 2.0f));
    auto fCamDistance = sqrtf(~(vCamPos - vBoxCenter));
    auto fMaxDistance = fCamDistance + fRadius;

    fCurrentImmersion = 0.0f;
    if (fCamDistance > fImmersionDistance)
        fCurrentImmersion = (fCamDistance / fImmersionDistance - 1.0f) * fImmersionDepth;

    CMatrix mTemp;
    mTemp.BuildPosition(0.0f, -fCurrentImmersion, 0.0f);
    pModel->mtx = mIslandOld * mTemp;

    float fOldFogDensity;
    float fIslandFogDensity;

    fIslandFogDensity = AttributesPointer->GetAttributeAsFloat("FogDensity", 0.0f);

    if (aForts.size() && !AIFortEID) //~!@
    {
        AIFortEID = core.GetEntityId("AIFort");
    }

    pRS->GetRenderState(D3DRS_FOGDENSITY, (uint32_t *)&fOldFogDensity);
    pRS->SetRenderState(D3DRS_FOGDENSITY, F2DW(fIslandFogDensity));
    int32_t j;
    for (j = static_cast<int32_t>(fMaxDistance / fOldFar); j >= 0; j--)
    {
        if (j != 0)
            pRS->SetRenderState(D3DRS_ZWRITEENABLE, false);
        pRS->SetNearFarPlane((j == 0) ? fOldNear : fOldFar * static_cast<float>(j),
                             fOldFar * static_cast<float>(j + 1));
        pModel->ProcessStage(Stage::realize, Delta_Time);
        pRS->SetRenderState(D3DRS_LIGHTING, true);
        D3DLIGHT9 lt, ltold;
        pRS->GetLight(0, &ltold);
        if (!dynamicLightsOn)
        {
            lt = {};
            lt.Type = D3DLIGHT_POINT;
            lt.Diffuse.a = 0.0f;
            lt.Diffuse.r = 1.0f;
            lt.Diffuse.g = 1.0f;
            lt.Diffuse.b = 1.0;
            lt.Ambient.r = 1.0f;
            lt.Ambient.g = 1.0f;
            lt.Ambient.b = 1.0f;
            lt.Specular.r = 1.0f;
            lt.Specular.g = 1.0f;
            lt.Specular.b = 1.0f;
            lt.Position.x = 0.0f;
            lt.Position.y = 0.0f;
            lt.Position.z = 0.0f;
            lt.Attenuation0 = 1.0f;
            lt.Range = 1e9f;
            pRS->SetLight(0, &lt);
        }
        for (uint32_t k = 0; k < aForts.size(); k++)
        {
            auto *const ent = core.GetEntityPointer(aForts[k]);
            auto mOld = static_cast<MODEL *>(ent)->mtx;
            static_cast<MODEL *>(ent)->mtx = mOld * mTemp;

            core.Send_Message(AIFortEID, "li", AI_MESSAGE_FORT_SET_LIGHTS, aForts[k]);
            static_cast<Entity *>(ent)->ProcessStage(Stage::realize, Delta_Time);
            core.Send_Message(AIFortEID, "li", AI_MESSAGE_FORT_UNSET_LIGHTS, aForts[k]);

            static_cast<MODEL *>(ent)->mtx = mOld;
        }
        pRS->SetLight(0, &ltold);
        pRS->SetRenderState(D3DRS_LIGHTING, dynamicLightsOn);
        pRS->SetRenderState(D3DRS_ZWRITEENABLE, true);
    }
    pRS->SetRenderState(D3DRS_FOGDENSITY, F2DW(fOldFogDensity));

    pRS->SetNearFarPlane(fOldNear, fOldFar / 2.0f);
    if (!bDrawReflections)
    {
        pRS->SetRenderState(D3DRS_FOGENABLE, false);

        auto *pSeaBed = static_cast<MODEL *>(core.GetEntityPointer(seabed_id));
        if (pSeaBed)
            pSeaBed->ProcessStage(Stage::realize, Delta_Time);
    }

    pRS->SetNearFarPlane(fOldNear, fOldFar);

    pRS->SetRenderState(D3DRS_LIGHTING, false);
    pRS->SetRenderState(D3DRS_FOGENABLE, bFogEnable);
    pRS->SetRenderState(D3DRS_AMBIENT, dwAmbientOld);
    pRS->SetRenderState(D3DRS_LIGHTING, bLighting);

    uint32_t i;

    if (enableDebugView_)
    {
        std::vector<RS_LINE> aLines;
        for (i = 0; i < AIPath.GetNumEdges(); i++)
        {
            AIFlowGraph::edge_t *pE = AIPath.GetEdge(i);
            aLines.push_back(RS_LINE{AIPath.GetPointPos(pE->dw1), 0xFFFFFF});
            aLines.push_back(RS_LINE{AIPath.GetPointPos(pE->dw2), 0xFFFFFF});
        }
        CMatrix m;
        pRS->SetTransform(D3DTS_WORLD, m);
        pRS->DrawLines(&aLines[0], aLines.size() / 2, "Line");
    }
}

uint64_t ISLAND::ProcessMessage(MESSAGE &message)
{
    entid_t eID;
    switch (message.Long())
    {
    case MSG_ISLAND_ADD_FORT:
        aForts.push_back(message.EntityID());
        break;
    case MSG_LOCATION_ADD_MODEL: {
        eID = message.EntityID();
        const std::string &idstr = message.String();
        const std::string &str = message.String();
        AddLocationModel(eID, idstr, str);
        break;
    }
    case MSG_ISLAND_LOAD_GEO: {
        // from sea
        cFoamDir = message.String();
        cModelsDir = message.String();
        cModelsID = message.String();
        Mount(cModelsID, cModelsDir, nullptr);
        CreateHeightMap(cFoamDir, cModelsID);
    }
    break;
    case MSG_ISLAND_START: // from location
        CreateHeightMap(cModelsDir, cModelsID);
        break;
    case MSG_SEA_REFLECTION_DRAW:
        bDrawReflections = true;
        Realize(0);
        bDrawReflections = false;
        break;
    case MSG_MODEL_SET_MAX_VIEW_DIST:
        core.Send_Message(model_id, "lf", MSG_MODEL_SET_MAX_VIEW_DIST, message.Float());
        break;
    }
    return 1;
}

void ISLAND::AddLocationModel(entid_t eid, const std::string_view &pIDStr, const std::string_view &pDir)
{
    Assert(!pDir.empty() && !pIDStr.empty());
    bForeignModels = true;
    cModelsDir = pDir;
    cModelsID = pIDStr;
    core.AddToLayer(ISLAND_TRACE, eid, 10);
}

inline float ISLAND::GetDepthNoCheck(uint32_t iX, uint32_t iZ)
{
    return fDepthHeight[mzDepth.Get(iX, iZ)];
}

bool ISLAND::Check2DBoxDepth(CVECTOR vPos, CVECTOR vSize, float fAngY, float fMinDepth)
{
    const float fCos = cosf(fAngY), fSin = sinf(fAngY);
    for (float z = -vSize.z / 2.0f; z < vSize.z / 2.0f; z += fStepDZ)
        for (float x = -vSize.x / 2.0f; x < vSize.x / 2.0f; x += fStepDX)
        {
            float fDepth, xx = x, zz = z;
            RotateAroundY(xx, zz, fCos, fSin);
            GetDepthFast(vPos.x + xx, vPos.z + zz, &fDepth);
            if (fDepth > fMinDepth)
                return true;
        }
    return false;
}

bool ISLAND::GetDepthFast(float x, float z, float *fRes)
{
    // if (!mzDepth.isLoaded()) { if (fRes) *fRes = -50.0f; return false; }
    x -= vBoxCenter.x;
    z -= vBoxCenter.z;
    if (fabsf(x) >= vRealBoxSize.x || fabsf(z) >= vRealBoxSize.z)
    // if (x < -vRealBoxSize.x || z < -vRealBoxSize.z || x > vRealBoxSize.x || z > vRealBoxSize.z)
    {
        *fRes = -50.0f;
        return false;
    }

    const float fX = (x * fStep1divDX) + static_cast<float>(iDMapSize >> 1);
    const float fZ = (z * fStep1divDZ) + static_cast<float>(iDMapSize >> 1);

    *fRes = GetDepthNoCheck(ftoi(fX), ftoi(fZ));
    return true;
}

bool ISLAND::GetDepth(float x, float z, float *fRes)
{
    // if (!mzDepth.isLoaded()) {if (fRes) *fRes = -50.0f;    return false; }
    x -= vBoxCenter.x;
    z -= vBoxCenter.z;
    if (fabsf(x) >= vRealBoxSize.x || fabsf(z) >= vRealBoxSize.z)
    // if (x < -vRealBoxSize.x || z < -vRealBoxSize.z || x > vRealBoxSize.x || z > vRealBoxSize.z)
    {
        *fRes = -50.0f;
        return false;
    }

    const float fX = (x * fStep1divDX) + static_cast<float>(iDMapSize >> 1);
    const float fZ = (z * fStep1divDZ) + static_cast<float>(iDMapSize >> 1);

    *fRes = GetDepthNoCheck(ftoi(fX), ftoi(fZ));

    return true;
}

bool ISLAND::ActivateCamomileTrace(CVECTOR &vSrc)
{
    const float fRadius = 100.0f;
    const int32_t iNumPetal = 8;
    int32_t iNumInner = 0;

    for (int32_t i = 0; i < iNumPetal; i++)
    {
        TRIANGLE trg;
        CVECTOR vDst, vCross;
        float fAng, fCos, fSin, fRes;

        fAng = static_cast<float>(i) / static_cast<float>(iNumPetal) * PIm2;
        fCos = cosf(fAng);
        fSin = sinf(fAng);

        vDst = vSrc + CVECTOR(fCos * fRadius, 0.0f, fSin * fRadius);
        fRes = Trace(vSrc, vDst);
        if (fRes > 1.0f)
            continue;
        auto *pEnt = static_cast<MODEL *>(core.GetEntityPointer(pCollide->GetObjectID()));
        Assert(pEnt);
        pEnt->GetCollideTriangle(trg);
        vCross = !((trg.vrt[1] - trg.vrt[0]) ^ (trg.vrt[2] - trg.vrt[0]));
        fRes = vCross | (!(vDst - vSrc));
        if (fRes > 0.0f)
            iNumInner++;
        if (iNumInner > 1)
            return true;
    }

    return false;
}

void ISLAND::CalcBoxParameters(CVECTOR &_vBoxCenter, CVECTOR &_vBoxSize)
{
    GEOS::INFO ginfo;
    float x1 = 1e+8f, x2 = -1e+8f, z1 = 1e+8f, z2 = -1e+8f;

    auto &&entities = core.GetEntityIds(ISLAND_TRACE);
    for (auto ent_id : entities)
    {
        MODEL *pM = static_cast<MODEL *>(core.GetEntityPointer(ent_id));
        if (pM == nullptr)
            continue;

        uint32_t i = 0;
        while (true)
        {
            NODE *pN = pM->GetNode(i);
            i++;
            if (!pN)
                break;
            pN->geo->GetInfo(ginfo);
            CVECTOR vGlobPos = pN->glob_mtx.Pos();
            const CVECTOR vBC = vGlobPos + CVECTOR(ginfo.boxcenter.x, 0.0f, ginfo.boxcenter.z);
            const CVECTOR vBS = CVECTOR(ginfo.boxsize.x, 0.0f, ginfo.boxsize.z) / 2.0f;
            if (vBC.x - vBS.x < x1)
                x1 = vBC.x - vBS.x;
            if (vBC.x + vBS.x > x2)
                x2 = vBC.x + vBS.x;
            if (vBC.z - vBS.z < z1)
                z1 = vBC.z - vBS.z;
            if (vBC.z + vBS.z > z2)
                z2 = vBC.z + vBS.z;
        }
    }
    _vBoxCenter = CVECTOR((x1 + x2) / 2.0f, 0.0f, (z1 + z2) / 2.0f);
    _vBoxSize = CVECTOR(x2 - x1, 0.0f, z2 - z1);
}

bool ISLAND::CreateHeightMap(const std::string_view &pDir, const std::string_view &pName)
{
    TGA_H tga_head;
    char str_tmp[256];

    std::filesystem::path path = std::filesystem::path() / "resource" / "foam" / pDir / pName;
    std::string fileName = path.string() + ".tga";
    std::string iniName = path.string() + ".ini";

    // calc center and size
    CalcBoxParameters(vBoxCenter, vRealBoxSize);
    vBoxSize = vRealBoxSize + CVECTOR(50.0f, 0.0f, 50.0f);

    rIsland.x1 = vBoxCenter.x - vBoxSize.x / 2.0f;
    rIsland.y1 = vBoxCenter.z - vBoxSize.z / 2.0f;
    rIsland.x2 = vBoxCenter.x + vBoxSize.x / 2.0f;
    rIsland.y2 = vBoxCenter.z + vBoxSize.z / 2.0f;

    bool bLoad = mzDepth.Load(fileName + ".zap");

    if (!bLoad)
    {
        auto fileS = fio->_CreateFile(fileName.c_str(), std::ios::binary | std::ios::in);
        if (fileS.is_open())
        {
            fio->_ReadFile(fileS, &tga_head, sizeof(tga_head));
            iDMapSize = tga_head.width;
            pDepthMap.resize(iDMapSize * iDMapSize);
            fio->_ReadFile(fileS, pDepthMap.data(), pDepthMap.size());
            fio->_CloseFile(fileS);

            mzDepth.DoZip(pDepthMap, iDMapSize);
            mzDepth.Save(fileName + ".zap");
            pDepthMap.clear();
            bLoad = true;
        }
    }

    if (bLoad)
    {
        iDMapSize = mzDepth.GetSizeX();
        for (iDMapSizeShift = 0; iDMapSizeShift < 30; iDMapSizeShift++)
            if (static_cast<uint32_t>(1 << iDMapSizeShift) == iDMapSize)
                break;
        fStepDX = vBoxSize.x / static_cast<float>(iDMapSize);
        fStepDZ = vBoxSize.z / static_cast<float>(iDMapSize);

        fStep1divDX = 1.0f / fStepDX;
        fStep1divDZ = 1.0f / fStepDZ;

        vBoxSize /= 2.0f;
        vRealBoxSize /= 2.0f;

        auto pI = fio->OpenIniFile(iniName.c_str());
        Assert(pI.get());

        CVECTOR vTmpBoxCenter, vTmpBoxSize;
        pI->ReadString("Main", "vBoxCenter", str_tmp, sizeof(str_tmp) - 1, "1.0,1.0,1.0");
        sscanf(str_tmp, "%f,%f,%f", &vTmpBoxCenter.x, &vTmpBoxCenter.y, &vTmpBoxCenter.z);
        pI->ReadString("Main", "vBoxSize", str_tmp, sizeof(str_tmp) - 1, "1.0,1.0,1.0");
        sscanf(str_tmp, "%f,%f,%f", &vTmpBoxSize.x, &vTmpBoxSize.y, &vTmpBoxSize.z);
        if (~(vTmpBoxCenter - vBoxCenter) > 0.1f)
        {
            core.Trace("Island: vBoxCenter not equal, foam error: %s, distance = %.3f", iniName.c_str(),
                       sqrtf(~(vTmpBoxCenter - vBoxCenter)));
            core.Trace("vBoxCenter = %f,%f,%f", vBoxCenter.x, vBoxCenter.y, vBoxCenter.z);
        }
        if (~(vTmpBoxSize - vBoxSize) > 0.1f)
        {
            core.Trace("Island: vBoxSize not equal, foam error: %s", iniName.c_str());
            core.Trace("vBoxSize = %f,%f,%f", vBoxSize.x, vBoxSize.y, vBoxSize.z);
        }

        AIPath.Load(*pI);
        AIPath.BuildTable();

        return true;
    }

    core.Trace("WARN: FOAM: Can't find foam: %s", fileName.c_str());

    int32_t iTestSize = static_cast<int32_t>(vBoxSize.x / 1.5f);
    // fixed maximum depth map to 1024 size!!!!!!!
    iDMapSizeShift = 11;
    // for (iDMapSizeShift=0;iDMapSizeShift<10;iDMapSizeShift++) if ((1<<iDMapSizeShift) >= iTestSize) break;
    iDMapSize = (1 << iDMapSizeShift);

    fStepDX = vBoxSize.x / static_cast<float>(iDMapSize);
    fStepDZ = vBoxSize.z / static_cast<float>(iDMapSize);

    fStep1divDX = 1.0f / fStepDX;
    fStep1divDZ = 1.0f / fStepDZ;

    pDepthMap.resize(iDMapSize * iDMapSize);

    float fEarthPercent = 0.0f;
    float fX, fZ;

    for (fZ = 0; fZ < static_cast<float>(iDMapSize); fZ += 1.0f)
    {
        if ((static_cast<int32_t>(fZ) & 127) == 127)
            core.Trace("Z = %.0f", fZ);
        for (fX = 0; fX < static_cast<float>(iDMapSize); fX += 1.0f)
        {
            int32_t iIdx = static_cast<int32_t>(fX) + static_cast<int32_t>(fZ) * iDMapSize;
            pDepthMap[iIdx] = 255;
            float fXX = (fX - static_cast<float>(iDMapSize) / 2.0f) * fStepDX;
            float fZZ = (fZ - static_cast<float>(iDMapSize) / 2.0f) * fStepDZ;
            CVECTOR vSrc(fXX, 0.0f, fZZ), vDst(fXX, -500.0f, fZZ + 0.001f);
            vSrc += vBoxCenter;
            vDst += vBoxCenter;
            float fRes = Trace(vSrc, vDst);
            Assert(isnan(fRes) == false);
            if (fRes <= 1.0f) // island ocean floor exist
            {
                float fHeight = sqrtf(~(fRes * (vDst - vSrc)));
                if (fHeight > -HMAP_MAXHEIGHT)
                {
                    fHeight = -HMAP_MAXHEIGHT;
                }
                // Activate camomile trace!
                if (ActivateCamomileTrace(vSrc))
                    pDepthMap[iIdx] = static_cast<uint8_t>(HMAP_START);
                else
                    pDepthMap[iIdx] = static_cast<unsigned char>(
                        (HMAP_START + static_cast<float>(HMAP_NUMBERS) * fHeight / -HMAP_MAXHEIGHT));
            }
            else // check for up direction
            {
                vSrc = CVECTOR(fXX, 0.0f, fZZ);
                vDst = CVECTOR(fXX, 1500.0f, fZZ + 0.001f);
                vSrc += vBoxCenter;
                vDst += vBoxCenter;
                float fRes = Trace(vSrc, vDst);
                if (fRes <= 1.0f || ActivateCamomileTrace(vSrc))
                {
                    pDepthMap[iIdx] = static_cast<uint8_t>(HMAP_START);
                }
            }
            if (pDepthMap[iIdx] == static_cast<uint8_t>(HMAP_START))
                fEarthPercent += 1.0f;
        }
    }

    vBoxSize /= 2.0f;
    vRealBoxSize /= 2.0f;

    SaveTga8((char *)fileName.c_str(), pDepthMap.data(), iDMapSize, iDMapSize);

    mzDepth.DoZip(pDepthMap, iDMapSize);
    mzDepth.Save(fileName + ".zap");
    pDepthMap.clear();

    auto pI = fio->OpenIniFile(iniName.c_str());
    if (!pI)
    {
        pI = fio->CreateIniFile(iniName.c_str(), false);
        Assert(pI.get());
    }
    char str[512];
    pI->WriteString("Main", "DepthFile", (char *)fileName.c_str());
    sprintf_s(str, "%f,%f,%f", vBoxCenter.x, vBoxCenter.y, vBoxCenter.z);
    pI->WriteString("Main", "vBoxCenter", str);
    sprintf_s(str, "%f,%f,%f", vBoxSize.x, vBoxSize.y, vBoxSize.z);
    pI->WriteString("Main", "vBoxSize", str);

    return true;
}

bool ISLAND::SaveTga8(char *fname, uint8_t *pBuffer, uint32_t dwSizeX, uint32_t dwSizeY)
{
    TGA_H tga_head{};

    tga_head.type = 3;
    tga_head.width = static_cast<uint16_t>(dwSizeX);
    tga_head.height = static_cast<uint16_t>(dwSizeY);
    tga_head.bpp = 8;
    tga_head.attr8 = 8;

    auto fileS = fio->_CreateFile(fname, std::ios::binary | std::ios::out);
    if (!fileS.is_open())
    {
        core.Trace("Island: Can't create island file! %s", fname);
        return false;
    }
    fio->_WriteFile(fileS, &tga_head, sizeof(tga_head));
    fio->_WriteFile(fileS, pBuffer, dwSizeX * dwSizeY);
    fio->_CloseFile(fileS);

    return true;
}

bool ISLAND::Mount(const std::string_view &fname, const std::string_view &fdir, entid_t *eID)
{
    Uninit();

    SetName(fname);

    const std::filesystem::path path = std::filesystem::path() / fdir / fname;
    const std::string pathStr = path.string();

    //switch dynamic light on/off + diag message if you need  --->
    dynamicLightsOn = AttributesPointer->GetAttributeAsDword("dynamicLightsOn", 0);

    model_id = core.CreateEntity("MODELR");
    core.Send_Message(model_id, "ls", MSG_MODEL_SET_LIGHT_PATH, static_cast<const char*>(AttributesPointer->GetAttribute("LightingPath")));
    core.Send_Message(model_id, "ls", MSG_MODEL_LOAD_GEO, pathStr.c_str());

    // extract subobject(sea_bed) to another model
    auto *pModel = static_cast<MODEL *>(core.GetEntityPointer(model_id));
    NODE *pNode = pModel->FindNode(SEA_BED_NODE_NAME);
    if (pNode)
        seabed_id = pNode->Unlink2Model();
    else
        core.Trace("Island: island %s has no sea bed, check me!", std::string(fname).c_str());

    core.AddToLayer(ISLAND_TRACE, model_id, 10);
    core.AddToLayer(ISLAND_TRACE, seabed_id, 10);
    core.AddToLayer(RAIN_DROPS, model_id, 100);

    auto *pSeaBedModel = static_cast<MODEL *>(core.GetEntityPointer(seabed_id));

    mIslandOld = pModel->mtx;

    const auto lighter_id = core.GetEntityId("lighter");
    core.Send_Message(lighter_id, "ssi", "AddModel", std::string(fname).c_str(), model_id);
    const std::string sSeaBedName = std::string(fname) + "_seabed";
    core.Send_Message(lighter_id, "ssi", "AddModel", (char *)sSeaBedName.c_str(), seabed_id);

    fImmersionDistance = AttributesPointer->GetAttributeAsFloat("ImmersionDistance", 3000.0f);
    fImmersionDepth = AttributesPointer->GetAttributeAsFloat("ImmersionDepth", 25.0f);

    return true;
}

float ISLAND::Cannon_Trace(int32_t iBallOwner, const CVECTOR &vSrc, const CVECTOR &vDst)
{
    const float fRes = Trace(vSrc, vDst);
    if (fRes <= 1.0f)
    {
        const CVECTOR vTemp = vSrc + fRes * (vDst - vSrc);
        core.Event(BALL_ISLAND_HIT, "lfff", iBallOwner, vTemp.x, vTemp.y, vTemp.z);
    }
    return fRes;
}

float ISLAND::Trace(const CVECTOR &vSrc, const CVECTOR &vDst)
{
    return pCollide->Trace(core.GetEntityIds(ISLAND_TRACE), vSrc, vDst, nullptr, 0);
}

// Path section
bool ISLAND::GetMovePoint(CVECTOR &vSrc, CVECTOR &vDst, CVECTOR &vRes)
{
    // check for one side
    uint32_t i, j;
    vRes = vDst;
    vSrc.y = vDst.y = 0.1f;

    if ((vSrc.x <= rIsland.x1 && vDst.x <= rIsland.x1) || (vSrc.x >= rIsland.x2 && vDst.x >= rIsland.x2) ||
        (vSrc.z <= rIsland.y1 && vDst.z <= rIsland.y1) || (vSrc.z >= rIsland.y2 && vDst.z >= rIsland.y2))
        return false;

    // one simple trace vSrc - vDst
    if (Trace(vSrc, vDst) >= 1.0f)
        return false;

    CVECTOR vDir = !(vDst - vSrc);

    std::vector<AIFlowGraph::npoint_t> *PointsSrc, *PointsDst;

    PointsSrc = AIPath.GetNearestPoints(vSrc);
    PointsDst = AIPath.GetNearestPoints(vDst);

    const uint32_t dwSizeSrc = ((*PointsSrc).size() > 8) ? 8 : (*PointsSrc).size();
    const uint32_t dwSizeDst = ((*PointsDst).size() > 8) ? 8 : (*PointsDst).size();

    for (i = 0; i < dwSizeDst; i++)
        (*PointsDst)[i].fTemp = Trace(vDst, AIPath.GetPointPos((*PointsDst)[i].dwPnt));

    float fMaxDistance = 1e9f;
    uint32_t dwI = INVALID_ARRAY_INDEX, dwJ;
    for (i = 0; i < dwSizeSrc; i++)
    {
        if (Trace(vSrc, AIPath.GetPointPos((*PointsSrc)[i].dwPnt)) < 1.0f)
            continue;
        const float fDist1 = sqrtf(~(vSrc - AIPath.GetPointPos((*PointsSrc)[i].dwPnt)));
        if (fDist1 < 80.0f)
            continue;
        for (j = 0; j < dwSizeDst; j++)
            if ((*PointsDst)[j].fTemp > 1.0f)
            {
                // if (Trace(vDst,AIPath.GetPointPos((*PointsDst)[j].dwPnt)) < 1.0f) continue;
                const float fDist2 = sqrtf(~(vDst - AIPath.GetPointPos((*PointsDst)[j].dwPnt)));
                const float fDistance = AIPath.GetPathDistance((*PointsSrc)[i].dwPnt, (*PointsDst)[j].dwPnt);
                const float fTotalDist = fDistance + fDist1 + fDist2;
                if (fTotalDist < fMaxDistance && fTotalDist > 0.0f)
                {
                    fMaxDistance = fTotalDist;
                    dwI = i;
                    dwJ = j;
                }
            }
    }

    if (INVALID_ARRAY_INDEX != dwI)
        vRes = AIPath.GetPointPos((*PointsSrc)[dwI].dwPnt);

    STORM_DELETE(PointsSrc);
    STORM_DELETE(PointsDst);

    return true;
}

void ISLAND::ShowEditor()
{
    ImGui::Text("Island");
    ImGui::Checkbox("Debug view", &enableDebugView_);
    ImGui::Checkbox("Dynamic lights", &dynamicLightsOn);

    ImGui::DragFloat("Immersion Depth", &fImmersionDepth, 0.005f, 0.0f, 1.0f, "%.3f");
    ImGui::DragFloat("Immersion Distance", &fImmersionDistance, 0.005f, 0.0f, 1.0f, "%.3f");
    ImGui::DragFloat("Current Immersion", &fCurrentImmersion, 0.005f, 0.0f, 1.0f, "%.3f");
}
