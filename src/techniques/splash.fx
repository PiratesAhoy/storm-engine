technique splash
{
    pass p0
    {
        CullMode = CCW;
        ColorVertex = true;
        AlphaTestEnable = false;
        AlphaRef = 0x0;
        AlphaBlendEnable = true;
        SrcBlend = srcalpha;
        DestBlend = invsrcalpha;

        ColorOp[0] = modulate4x;
        ColorArg1[0] = texture;
        ColorArg2[0] = current;

        AlphaOp[0] = modulate;
        AlphaArg1[0] = texture;
        AlphaArg2[0] = current;

        ColorOp[1] = disable;
        AlphaOp[1] = disable;
    }
}

technique splash2
{
    pass p0
    {
        CullMode = none;
        ColorVertex = false;
        AlphaTestEnable = true;
        AlphaBlendEnable = true;
        SrcBlend = srcalpha;
        AlphaRef = 0x0;
        DestBlend = invsrcalpha;
        ZWriteEnable = false;
        FogEnable = true;

        ColorOp[0] = modulate4x;
        ColorArg1[0] = texture;
        ColorArg2[0] = diffuse;
        AddressV[0] = CLAMP;

        ColorOp[1] = selectarg1;
        ColorArg1[1] = current;
        AddressV[1] = CLAMP;

        AlphaOp[0] = selectarg1;
        AlphaArg1[0] = texture;

        AlphaOp[1] = lerp;
        AlphaArg0[1] = diffuse;
        AlphaArg1[1] = texture;
        AlphaArg2[1] = current;

        ColorOp[2] = disable;
        AlphaOp[2] = disable;
    }
}