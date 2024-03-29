#pragma once
#include "linerenderer.h"
#include "ellipsoidrenderer.h"
#include "cylinderrenderer.h"
#include "meshrenderer.h"
#include "pointcloudrenderer.h"
#include "chemicalstructure.h"
#include "drawingstyle.h"
#include "renderselection.h"
#include "rendereruniforms.h"


namespace cx::graphics {
class ChemicalStructureRenderer : public QObject {
    Q_OBJECT
public:
    ChemicalStructureRenderer(ChemicalStructure *, QObject *parent = nullptr);
    void update(ChemicalStructure *);

    void setSelectionHandler(RenderSelection *);

    void setShowHydrogenAtoms(bool show);
    [[nodiscard]] bool showHydrogenAtoms() const;
    void toggleShowHydrogenAtoms();

    void setShowSuppressedAtoms(bool show);
    [[nodiscard]] bool showSuppressedAtoms() const;
    void toggleShowSuppressedAtoms();


    void setAtomStyle(AtomDrawingStyle);
    void setBondStyle(BondDrawingStyle);

    [[nodiscard]] AtomDrawingStyle atomStyle() const;
    [[nodiscard]] BondDrawingStyle bondStyle() const;

    [[nodiscard]] float bondThickness() const;

    void updateRendererUniforms(const RendererUniforms &);

    void updateAtoms();
    void updateBonds();
    void updateInteractions();

    void beginUpdates();
    void endUpdates();

    void draw(bool forPicking=false);

    [[nodiscard]] inline CylinderRenderer *cylinderRenderer() { return m_cylinderRenderer; }
    [[nodiscard]] inline EllipsoidRenderer *ellipsoidRenderer() { return m_ellipsoidRenderer; }
    [[nodiscard]] inline LineRenderer *lineRenderer() { return m_lineRenderer; }

public slots:
    void childAddedToStructure(QObject *);
    void childRemovedFromStructure(QObject *);

private:
    void initStructureChildren();

    void handleAtomsUpdate();
    void handleBondsUpdate();
    void handleInteractionsUpdate();

    [[nodiscard]] bool shouldSkipAtom(int idx) const;

    bool m_atomsNeedsUpdate{true};
    bool m_bondsNeedsUpdate{true};

    bool m_showHydrogens{true};
    bool m_showSuppressedAtoms{false};
    float m_bondThicknessFactor{0.325};

    AtomDrawingStyle m_atomStyle{AtomDrawingStyle::CovalentRadiusSphere};
    BondDrawingStyle m_bondStyle{BondDrawingStyle::Stick};

    RenderSelection * m_selectionHandler{nullptr};
    LineRenderer * m_lineRenderer{nullptr};
    EllipsoidRenderer * m_ellipsoidRenderer{nullptr};
    CylinderRenderer * m_cylinderRenderer{nullptr};
    MeshRenderer *m_meshRenderer{nullptr};
    PointCloudRenderer *m_pointCloudRenderer{nullptr};

    ChemicalStructure * m_structure{nullptr};

    RendererUniforms m_uniforms;
};

}
