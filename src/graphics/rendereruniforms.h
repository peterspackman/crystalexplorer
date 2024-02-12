#pragma once
#include <QVector3D>
#include <QVector2D>
#include <QMatrix4x4>
#include <QMatrix3x3>

namespace cx::graphics {

struct RendererUniforms {
    float u_time{0.0f};
    float u_screenGamma{2.2f};
    QVector2D u_viewport_size{0.0f, 0.0f};
    float u_ortho{1.0};
    float u_pointSize{1.0};
    int u_renderMode{0};
    int u_toneMapIdentifier{1};
    int u_numLights{4};
    float u_lightingExposure{0.0f};
    QMatrix4x4 u_lightSpecular;
    QMatrix4x4 u_lightPos;
    QVector4D u_lightGlobalAmbient;
    QVector2D u_attenuationClamp;
    bool u_selectionMode{false};
    QVector4D u_selectionColor;
    float u_scale{1.0};
    QMatrix4x4 u_viewMat;
    QMatrix4x4 u_modelMat;
    QMatrix4x4 u_projectionMat;
    QMatrix4x4 u_modelViewMat;
    QMatrix4x4 u_modelViewMatInv;
    QMatrix4x4 u_viewMatInv;
    QMatrix4x4 u_modelViewProjectionMat;
    QMatrix3x3 u_normalMat;
    QVector3D u_cameraPosVec;
    float u_ellipsoidLineWidth{0.05f};
    int u_texture{0};
    float u_materialRoughness{0.4f};
    float u_materialMetallic{0.0f};
    float u_textSDFOutline{0.0f};
    float u_textSDFBuffer{0.15f};
    float u_textSDFSmoothing{0.15f};
    QVector3D u_textColor{0.0f, 0.0f, 0.0f};
    QVector3D u_textOutlineColor{0.5f, 0.5f, 0.5f};
    float u_depthFogDensity{10.0f};
    float u_depthFogOffset{0.1f};
    QVector3D u_depthFogColor;

    // TODO migrate to a UBO rather than this
#define SET_UNIFORM(uniform) \
    prog->setUniformValue(#uniform, uniform)

    template<typename T>
    void apply(T *renderer) {
	auto prog = renderer->program();
	SET_UNIFORM(u_pointSize);
	SET_UNIFORM(u_lightSpecular);
	SET_UNIFORM(u_renderMode);
	SET_UNIFORM(u_numLights);
	SET_UNIFORM(u_lightPos);
	SET_UNIFORM(u_lightGlobalAmbient);
	SET_UNIFORM(u_selectionColor);
	SET_UNIFORM(u_selectionMode);
	SET_UNIFORM(u_scale);
	SET_UNIFORM(u_viewMat);
	SET_UNIFORM(u_modelMat);
	SET_UNIFORM(u_projectionMat);
	SET_UNIFORM(u_modelViewMat);
	SET_UNIFORM(u_modelViewMatInv);
	SET_UNIFORM(u_viewMatInv);
	SET_UNIFORM(u_normalMat);
	SET_UNIFORM(u_modelViewProjectionMat);
	SET_UNIFORM(u_cameraPosVec);
	SET_UNIFORM(u_lightingExposure);
	SET_UNIFORM(u_toneMapIdentifier);
	SET_UNIFORM(u_attenuationClamp);

	SET_UNIFORM(u_viewport_size);
	SET_UNIFORM(u_ortho);
	SET_UNIFORM(u_time);
	SET_UNIFORM(u_screenGamma);
	SET_UNIFORM(u_ellipsoidLineWidth);
	SET_UNIFORM(u_texture);
	SET_UNIFORM(u_materialRoughness);
	SET_UNIFORM(u_materialMetallic);

	SET_UNIFORM(u_textSDFOutline);
	SET_UNIFORM(u_textSDFBuffer);
	SET_UNIFORM(u_textSDFSmoothing);
	SET_UNIFORM(u_textColor);
	SET_UNIFORM(u_textOutlineColor);
	SET_UNIFORM(u_depthFogDensity);
	SET_UNIFORM(u_depthFogColor);
	SET_UNIFORM(u_depthFogOffset);

    }
#undef SET_UNIFORM

};

}
