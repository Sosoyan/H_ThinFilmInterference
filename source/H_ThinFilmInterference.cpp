// ---------------------------------------
// H_ThinFilmInterference
// By Ryan Hussain
// Contributer, Chi Chang Chu
// Open sourced under the MIT license

#include <cstring>
#include "HTFIhelpers.h"

AI_SHADER_NODE_EXPORT_METHODS(SimpleMethods);
 
namespace
{
    enum SimpleParams
    {
        p_ior_inside,
        p_ior_outside,
        p_min_thick,
        p_max_thick,
        p_interference,
        p_color_samples,
        p_tfi_mult,
    };
};


node_parameters
{
    // Input Parameters and default values
    AiParameterFlt("ior_inside", 1.3f);
    AiParameterFlt("ior_outside", 1.0f);
    AiParameterFlt("min_thick", 300.0f);
    AiParameterFlt("max_thick", 700.0f);
    AiParameterFlt("interference", 0.0f);
    AiParameterInt("color_samples", 20);
    AiParameterFlt("multiplier", 1.0f);
}


node_initialize
{
    ShaderData * data = new ShaderData;
    AiNodeSetLocalData(node,data);
}


node_update
{
    ShaderData * data = (ShaderData*) AiNodeGetLocalData(node);
}


node_finish
{
    if (AiNodeGetLocalData(node) != NULL)
    {
        ShaderData * data = (ShaderData*) AiNodeGetLocalData(node);
        AiNodeSetLocalData(node, NULL);
        delete data;
    }
}


// TFI Evaluation
shader_evaluate
{
    ShaderData * data = (ShaderData*) AiNodeGetLocalData(node);

    AtVector SURF_P, SURF_N, CAM_P, normI, normN, forwardN, refractray, valsum, ramp1, outTFI;
    float IOR_IN, IOR_OUT, THICKNESS, MIN_THICK, MAX_THICK, tfi_mult, PI_sq, thickEval, toRamp, trig;
    int colorSamples;

    // get shading point position, normals and cam position
    SURF_P = sg->P;
    SURF_N = sg->Nf;
    CAM_P = sg->Ro;

    // get and set user parameters
    IOR_IN = AiShaderEvalParamFlt(p_ior_inside);
    IOR_OUT = AiShaderEvalParamFlt(p_ior_outside);
    MIN_THICK = AiShaderEvalParamFlt(p_min_thick);
    MAX_THICK = AiShaderEvalParamFlt(p_max_thick);
    colorSamples = AiShaderEvalParamInt(p_color_samples);
    THICKNESS = AiShaderEvalParamFlt(p_interference);
    tfi_mult = AiShaderEvalParamFlt(p_tfi_mult);

    // PI squared constant
    PI_sq = 6.2831853071795862;

    // calculate interference
    normI = AiV3Normalize(CAM_P - SURF_P);
    normN = AiV3Normalize(SURF_N);
    forwardN = normN * H_SIGNF(AiV3Dot(normI, normN));
    refractray = H_REFRACT(normI, forwardN, IOR_IN, IOR_OUT);
    thickEval = map(THICKNESS, 0, 1, MIN_THICK, MAX_THICK) * PI_sq * IOR_IN * AiV3Dot(refractray, forwardN);

    // vars for color sample loop
    int _end = colorSamples;
    int _step = 1;
    
    AtVector constantNone(0.0f, 0.0f, 0.0f);
    AtVector _constantNone = constantNone;
    
    // evaluation per color sample
    for(int _i=0; _i <= _end; _i++ )
    {
        toRamp = (float)_i / (_end -1);
        ramp1 = Wavelength_to_RGB( map(toRamp,0,1,380,750) );
        trig = sin(thickEval / map(toRamp, 0, 1, 380, 750));
        valsum = _constantNone + ramp1 * trig * trig;
        _constantNone = valsum;
    }

    // thin film interference output color
    outTFI = ((valsum / colorSamples) * 2);
    AtRGB result = AiPoint2Col(&outTFI)* tfi_mult;
    sg->out.RGB() = result;
}


node_loader
{
    if (i > 0) return false;
 
    node->methods = SimpleMethods;
    node->output_type = AI_TYPE_RGB;
    node->name = "thinFilmInterference";
    node->node_type = AI_NODE_SHADER;
    strcpy(node->version, AI_VERSION);

    return true;
}
