
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016-2017 Esteban Tovagliari, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "appleseedmaya/renderglobalsnode.h"

// Maya headers.
#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnNumericAttribute.h>

// appleseed.foundation headers.
#include "foundation/utility/api/specializedapiarrays.h"

// appleseed.renderer headers.
#include "renderer/api/project.h"
#include "renderer/api/utility.h"
#include "renderer/api/surfaceshader.h"

// appleseed.maya headers.
#include "appleseedmaya/attributeutils.h"
#include "appleseedmaya/config.h"
#include "appleseedmaya/typeids.h"

namespace asf = foundation;
namespace asr = renderer;

const MString RenderGlobalsNode::nodeName("appleseedRenderGlobals");
const MTypeId RenderGlobalsNode::id(RenderGlobalsNodeTypeId);

MObject RenderGlobalsNode::m_pixelSamples;
MObject RenderGlobalsNode::m_passes;
MObject RenderGlobalsNode::m_tileSize;

MStringArray RenderGlobalsNode::m_diagnosticShaderKeys;
MObject RenderGlobalsNode::m_diagnosticShader;

MObject RenderGlobalsNode::m_gi;
MObject RenderGlobalsNode::m_caustics;
MObject RenderGlobalsNode::m_bounces;
MObject RenderGlobalsNode::m_maxRayIntensity;
MObject RenderGlobalsNode::m_lightSamples;
MObject RenderGlobalsNode::m_envSamples;

MObject RenderGlobalsNode::m_backgroundEmitsLight;
MObject RenderGlobalsNode::m_envLightNode;

MObject RenderGlobalsNode::m_renderingThreads;

MObject RenderGlobalsNode::m_imageFormat;

void* RenderGlobalsNode::creator()
{
    return new RenderGlobalsNode();
}

MStatus RenderGlobalsNode::initialize()
{
    MFnNumericAttribute numAttrFn;
    MFnMessageAttribute msgAttrFn;

    MStatus status;

    // Pixel Samples.
    m_pixelSamples = numAttrFn.create("samples", "samples", MFnNumericData::kInt, 16, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals samples attribute");

    numAttrFn.setMin(1);
    status = addAttribute(m_pixelSamples);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals samples attribute");

    // Render Passes.
    m_passes = numAttrFn.create("passes", "passes", MFnNumericData::kInt, 1, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals passes attribute");

    numAttrFn.setMin(1);
    status = addAttribute(m_passes);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals passes attribute");

    // Tile Size.
    m_tileSize = numAttrFn.create("tileSize", "tileSize", MFnNumericData::kInt, 64, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals tileSize attribute");

    numAttrFn.setMin(1);
    status = addAttribute(m_tileSize);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals tileSize attribute");

    // Diagnostic shader override.
    MFnEnumAttribute enumAttrFn;
    m_diagnosticShader = enumAttrFn.create("diagnostics", "diagnostics", 0, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals diagnostic shader attribute");

    {
        size_t menuIndex = 0;
        enumAttrFn.addField("No Override", menuIndex++);
        m_diagnosticShaderKeys.append("no_override");

        asr::SurfaceShaderFactoryRegistrar factoryRegistrar;
        const asr::ISurfaceShaderFactory *factory = factoryRegistrar.lookup("diagnostic_surface_shader");
        assert(factory);
        asf::DictionaryArray metadata = factory->get_input_metadata();
        const asf::Dictionary& items = metadata[0].dictionary("items");

        asf::StringDictionary::const_iterator it(items.strings().begin());
        asf::StringDictionary::const_iterator e(items.strings().end());
        for(; it != e; ++it)
        {
            enumAttrFn.addField(it.key(), menuIndex++);
            m_diagnosticShaderKeys.append(MString(it.value()));
        }
    }

    status = addAttribute(m_diagnosticShader);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals diagnostic shader attribute");

    // GI.
    m_gi = numAttrFn.create("gi", "gi", MFnNumericData::kBoolean, true, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals gi attribute");

    status = addAttribute(m_gi);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals gi attribute");

    // Caustics.
    m_caustics = numAttrFn.create("caustics", "caustics", MFnNumericData::kBoolean, false, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals caustics attribute");

    status = addAttribute(m_caustics);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals caustics attribute");

    // GI Bounces.
    m_bounces = numAttrFn.create("bounces", "bounces", MFnNumericData::kInt, 8, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals bounces attribute");

    numAttrFn.setMin(0);
    status = addAttribute(m_bounces);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals bounces attribute");

    // Max Ray Intensity.
    m_maxRayIntensity = numAttrFn.create("maxRayIntensity", "maxRayIntensity", MFnNumericData::kFloat, 0.0f, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals maxRayIntensity attribute");
    numAttrFn.setMin(0.0f);
    status = addAttribute(m_maxRayIntensity);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals maxRayIntensity attribute");

    // Light Samples.
    m_lightSamples = numAttrFn.create("lightSamples", "lightSamples", MFnNumericData::kFloat, 1.0f, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals lightSamples attribute");
    numAttrFn.setMin(1.0f);
    status = addAttribute(m_lightSamples);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals lightSamples attribute");

    // Environment Samples.
    m_envSamples = numAttrFn.create("envSamples", "envSamples", MFnNumericData::kFloat, 1.0f, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals envSamples attribute");
    numAttrFn.setMin(1.0f);
    status = addAttribute(m_envSamples);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals envSamples attribute");

    // Background emits light.
    m_backgroundEmitsLight = numAttrFn.create("bgLight", "bgLight", MFnNumericData::kBoolean, true, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals bgLight attribute");

    status = addAttribute(m_backgroundEmitsLight);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals bgLight attribute");

    // Rendering threads.
    m_renderingThreads = numAttrFn.create("threads", "threads", MFnNumericData::kInt, 0, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals threads attribute");

    status = addAttribute(m_renderingThreads);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals threads attribute");

    // Environment light connection.
    m_envLightNode = msgAttrFn.create("envLight", "env", &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals envLight attribute");

    status = addAttribute(m_envLightNode);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals envLight attribute");

    // Image Format
    m_imageFormat = numAttrFn.create("imageFormat", "imageFormat", MFnNumericData::kInt, 0, &status);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to create render globals imageFormat attribute");

    status = addAttribute(m_imageFormat);
    APPLESEED_MAYA_CHECK_MSTATUS_RET_MSG(
        status,
        "appleseedMaya: Failed to add render globals imageFormat attribute");

    return status;
}

MStatus RenderGlobalsNode::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    return MS::kSuccess;
}

void RenderGlobalsNode::applyGlobalsToProject(
  const MObject&    globals,
  asr::Project&     project)
{
    asr::ParamArray& finalParams = project.configurations().get_by_name("final")->get_parameters();
    asr::ParamArray& iprParams   = project.configurations().get_by_name("interactive")->get_parameters();

    int samples;
    if (AttributeUtils::get(MPlug(globals, m_pixelSamples), samples))
    {
        finalParams.insert_path("uniform_pixel_renderer.samples", samples);

        if (samples == 1)
            finalParams.insert_path("uniform_pixel_renderer.force_antialiasing", true);
    }

    int passes;
    if (AttributeUtils::get(MPlug(globals, m_passes), passes))
    {
        finalParams.insert_path("generic_frame_renderer.passes", passes);
        finalParams.insert_path("shading_result_framebuffer", passes == 1 ? "ephemeral" : "permanent");
    }

    int diagnostic;
    if (AttributeUtils::get(MPlug(globals, m_diagnosticShader), diagnostic))
    {
        if (diagnostic != 0)
        {
            finalParams.insert_path(
                "shading_engine.override_shading.mode",
                m_diagnosticShaderKeys[diagnostic]);
            iprParams.insert_path(
                "shading_engine.override_shading.mode",
                m_diagnosticShaderKeys[diagnostic]);
        }
    }

    bool gi;
    if (AttributeUtils::get(MPlug(globals, m_gi), gi))
    {
        if (gi)
        {
            finalParams.insert_path("lighting_engine", "pt");
            iprParams.insert_path("lighting_engine", "pt");
        }
        else
        {
            finalParams.insert_path("lighting_engine", "drt");
            iprParams.insert_path("lighting_engine", "drt");
        }
    }

    int bounces = 0;
    AttributeUtils::get(MPlug(globals, m_bounces), bounces);
    {
        finalParams.insert_path("pt.max_path_length", bounces);
        iprParams.insert_path("pt.max_path_length", bounces);

        finalParams.insert_path("drt.max_path_length", bounces);
        iprParams.insert_path("drt.max_path_length", bounces);
    }

    bool caustics;
    if (AttributeUtils::get(MPlug(globals, m_caustics), caustics))
    {
        finalParams.insert_path("pt.enable_caustics", caustics);
        iprParams.insert_path("pt.enable_caustics", caustics);
    }

    float maxRayIntensity;
    if (AttributeUtils::get(MPlug(globals, m_maxRayIntensity), maxRayIntensity))
    {
        if (maxRayIntensity == 0.0f)
        {
            finalParams.remove_path("pt.max_ray_intensity");
            iprParams.remove_path("pt.max_ray_intensity");
        }
        else
        {
            finalParams.insert_path("pt.max_ray_intensity", maxRayIntensity);
            iprParams.insert_path("pt.max_ray_intensity", maxRayIntensity);
        }
    }


    float lightSamples;
    if (AttributeUtils::get(MPlug(globals, m_lightSamples), lightSamples))
    {
        finalParams.insert_path("pt.dl_light_samples", lightSamples);
        iprParams.insert_path("pt.dl_light_samples", lightSamples);

        finalParams.insert_path("drt.dl_light_samples", lightSamples);
        iprParams.insert_path("drt.dl_light_samples", lightSamples);
    }

    float envSamples;
    if (AttributeUtils::get(MPlug(globals, m_envSamples), envSamples))
    {
        finalParams.insert_path("pt.ibl_env_samples", envSamples);
        iprParams.insert_path("pt.ibl_env_samples", envSamples);

        finalParams.insert_path("drt.ibl_env_samples", envSamples);
        iprParams.insert_path("drt.ibl_env_samples", envSamples);
    }

    int threads;
    if (AttributeUtils::get(MPlug(globals, m_renderingThreads), threads))
    {
        if (threads == 0)
        {
            finalParams.insert_path("rendering_threads", "auto");
            iprParams.insert_path("rendering_threads", "auto");
        }
        else
        {
            finalParams.insert_path("rendering_threads", threads);
            iprParams.insert_path("rendering_threads", threads);
        }
    }
}
