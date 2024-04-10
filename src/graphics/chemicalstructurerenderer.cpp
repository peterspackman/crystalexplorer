#include "chemicalstructurerenderer.h"
#include "graphics.h"
#include "elementdata.h"
#include "interactions.h"
#include "mesh.h"


namespace cx::graphics {
ChemicalStructureRenderer::ChemicalStructureRenderer(
	ChemicalStructure *structure, QObject *parent) :
    QObject(parent), m_structure(structure) {

    m_ellipsoidRenderer = new EllipsoidRenderer();
    m_cylinderRenderer = new CylinderRenderer();
    m_lineRenderer = new LineRenderer();
    m_meshRenderer = new MeshRenderer();
    m_instanceRenderer = new MeshInstanceRenderer();
    m_transparentMeshRenderer = new MeshRenderer();
    m_transparentMeshRenderer->setAlpha(0.8);
    m_pointCloudRenderer = new PointCloudRenderer();
    connect(m_structure, &ChemicalStructure::childAdded,
	    this, &ChemicalStructureRenderer::childAddedToStructure);
    connect(m_structure, &ChemicalStructure::childRemoved,
	    this, &ChemicalStructureRenderer::childRemovedFromStructure);
    initStructureChildren();

    m_propertyColorMaps = {
	{"None", ColorMapName::CE_None},
	{"dnorm", ColorMapName::CE_bwr},
	{"di", ColorMapName::CE_rgb},
	{"de", ColorMapName::CE_rgb},
	{"di_norm", ColorMapName::CE_bwr},
	{"de_norm", ColorMapName::CE_bwr},
    };
}


void ChemicalStructureRenderer::initStructureChildren() {
    m_meshesNeedsUpdate = true;
}


void ChemicalStructureRenderer::setShowHydrogenAtoms(bool show) {
    if(show != m_showHydrogens) {
	m_showHydrogens = show;
	m_atomsNeedsUpdate = true;
	m_bondsNeedsUpdate = true;
    }
}

void ChemicalStructureRenderer::setSelectionHandler(RenderSelection *ptr) {
    m_selectionHandler = ptr;
}

bool ChemicalStructureRenderer::showHydrogenAtoms() const {
    return m_showHydrogens;
}

void ChemicalStructureRenderer::toggleShowHydrogenAtoms() {
    setShowHydrogenAtoms(!m_showHydrogens);
}

void ChemicalStructureRenderer::setShowSuppressedAtoms(bool show) {
    if(show != m_showSuppressedAtoms) {
	m_showSuppressedAtoms = show;
	m_atomsNeedsUpdate = true;
	m_bondsNeedsUpdate = true;
    }
}

bool ChemicalStructureRenderer::showSuppressedAtoms() const {
    return m_showSuppressedAtoms;
}

void ChemicalStructureRenderer::toggleShowSuppressedAtoms() {
    setShowSuppressedAtoms(!m_showSuppressedAtoms);
}

bool ChemicalStructureRenderer::shouldSkipAtom(int index) const {
    const auto &numbers = m_structure->atomicNumbers();

    if (!showHydrogenAtoms() && (numbers(index) == 1)) {
	return true;
    }
    else if (!showSuppressedAtoms() &&
        (m_structure->testAtomFlag(index, AtomFlag::Suppressed))) {
	return true;
    }
    return false;
}

void ChemicalStructureRenderer::setAtomStyle(AtomDrawingStyle style) {
    if(m_atomStyle == style) return;
    m_atomStyle = style;
    m_atomsNeedsUpdate = true;
}

AtomDrawingStyle ChemicalStructureRenderer::atomStyle() const {
    return m_atomStyle;
}

void ChemicalStructureRenderer::setBondStyle(BondDrawingStyle style) {
    if(m_bondStyle == style) return;
    m_bondStyle = style;
    m_bondsNeedsUpdate = true;
}

BondDrawingStyle ChemicalStructureRenderer::bondStyle() const {
    return m_bondStyle;
}


[[nodiscard]] float ChemicalStructureRenderer::bondThickness() const {
  return ElementData::elementFromAtomicNumber(1)->covRadius() *
         m_bondThicknessFactor;
}

void ChemicalStructureRenderer::updateAtoms() {
    m_atomsNeedsUpdate = true;
    handleAtomsUpdate();
}

void ChemicalStructureRenderer::updateBonds() {
    m_bondsNeedsUpdate = true;
    handleBondsUpdate();
}

void ChemicalStructureRenderer::updateMeshes() {
    m_meshesNeedsUpdate = true;
    emit meshesChanged();
}

void ChemicalStructureRenderer::handleAtomsUpdate() {
    if(!m_structure) return;
    if(!m_atomsNeedsUpdate) return;
    m_ellipsoidRenderer->clear();

    const auto &atomicNumbers = m_structure->atomicNumbers();
    const auto &positions = m_structure->atomicPositions();

    const auto covRadii = m_structure->covalentRadii();
    const auto vdwRadii = m_structure->vdwRadii();

    for (int i = 0; i < m_structure->numberOfAtoms(); i++) {

	if(shouldSkipAtom(i)) continue;
	auto color = m_structure->atomColor(i);
	float radius = covRadii(i) * 0.5;

	if (atomStyle() == AtomDrawingStyle::RoundCapped) {
	    radius = bondThickness();
	} else if (atomStyle() == AtomDrawingStyle::VanDerWaalsSphere) {
	    radius = vdwRadii(i);
	}
	if (m_structure->testAtomFlag(i, AtomFlag::Contact))
	  color = color.lighter();

	quint32 selectionId{0};
	QVector3D selectionIdColor;
	if(m_selectionHandler) {
	    selectionId = m_selectionHandler->add(SelectionType::Atom, i);
	    selectionIdColor = m_selectionHandler->getColorFromId(selectionId);
	}
	QVector3D position(positions(0, i), positions(1, i), positions(2, i));
	cx::graphics::addSphereToEllipsoidRenderer(
	    m_ellipsoidRenderer, position, color, radius, selectionIdColor,
	    m_structure->atomFlagsSet(i, AtomFlag::Selected));
      }
}

void ChemicalStructureRenderer::handleBondsUpdate() {
    if(!m_structure) return;
    if(!m_bondsNeedsUpdate) return;

    m_lineRenderer->clear();
    m_cylinderRenderer->clear();

    float radius = bondThickness();
    const auto &atomPositions = m_structure->atomicPositions();
    const auto &atomicNumbers = m_structure->atomicNumbers();
    const auto &covalentBonds = m_structure->covalentBonds();

    for (int bondIndex = 0; bondIndex < covalentBonds.size(); bondIndex++) {
        const auto [i, j] = covalentBonds[bondIndex];
        if(shouldSkipAtom(i) || shouldSkipAtom(j)) continue;

        QVector3D pointA(atomPositions(0, i), atomPositions(1, i),
                     atomPositions(2, i));
        QVector3D pointB(atomPositions(0, j), atomPositions(1, j),
                     atomPositions(2, j));

        auto colorA = m_structure->atomColor(i);
        auto colorB = m_structure->atomColor(j);

        bool selectedA = m_structure->atomFlagsSet(i, AtomFlag::Selected);
        bool selectedB = m_structure->atomFlagsSet(j, AtomFlag::Selected);

        quint32 bond_id{0};
        QVector3D id_color;

	if(m_selectionHandler) {
	    bond_id = m_selectionHandler->add(SelectionType::Bond, bondIndex);
	    id_color = m_selectionHandler->getColorFromId(bond_id);
	}

        if (bondStyle() == BondDrawingStyle::Line) {
            cx::graphics::addLineToLineRenderer(
		*m_lineRenderer, pointA, 0.5 * pointA + 0.5 * pointB,
		DrawingStyleConstants::bondLineWidth, colorA
	    );
	    cx::graphics::addLineToLineRenderer(
		*m_lineRenderer, pointB, 0.5 * pointA + 0.5 * pointB,
		DrawingStyleConstants::bondLineWidth, colorB
	    );
	} else {
	    cx::graphics::addCylinderToCylinderRenderer(
		m_cylinderRenderer, pointA,
		pointB, colorA, colorB, radius,
                id_color, selectedA, selectedB
	    );
	}
    }
}


void ChemicalStructureRenderer::beginUpdates() {
    m_meshRenderer->beginUpdates();
    m_transparentMeshRenderer->beginUpdates();
    m_lineRenderer->beginUpdates();
    m_cylinderRenderer->beginUpdates();
    m_ellipsoidRenderer->beginUpdates();
}

void ChemicalStructureRenderer::endUpdates() {
    m_meshRenderer->endUpdates();
    m_transparentMeshRenderer->endUpdates();
    m_lineRenderer->endUpdates();
    m_cylinderRenderer->endUpdates();
    m_ellipsoidRenderer->endUpdates();
}


void ChemicalStructureRenderer::draw(bool forPicking) {
    const auto storedRenderMode = m_uniforms.u_renderMode;

    if(forPicking) {
	m_uniforms.u_renderMode = 0;
	m_uniforms.u_selectionMode = true;
    }

    m_ellipsoidRenderer->bind();
    m_uniforms.apply(m_ellipsoidRenderer);
    m_ellipsoidRenderer->draw();
    m_ellipsoidRenderer->release();

    m_cylinderRenderer->bind();
    m_uniforms.apply(m_cylinderRenderer);
    m_cylinderRenderer->draw();
    m_cylinderRenderer->release();

    m_lineRenderer->bind();
    m_uniforms.apply(m_lineRenderer);
    m_lineRenderer->draw();
    m_lineRenderer->release();

    /*
    m_meshRenderer->bind();
    m_uniforms.apply(m_meshRenderer);
    m_meshRenderer->draw();
    m_meshRenderer->release();
    */

    handleMeshesUpdate();
    m_instanceRenderer->bind();
    m_uniforms.apply(m_instanceRenderer);
    m_instanceRenderer->draw();
    m_instanceRenderer->release();

    m_pointCloudRenderer->bind();
    m_uniforms.apply(m_pointCloudRenderer);
    m_pointCloudRenderer->draw();
    m_pointCloudRenderer->release();

    m_transparentMeshRenderer->bind();
    m_uniforms.apply(m_transparentMeshRenderer);
    m_transparentMeshRenderer->draw();
    m_transparentMeshRenderer->release();

    if(forPicking) {
	m_uniforms.u_renderMode = storedRenderMode;
	m_uniforms.u_selectionMode = false;
    }
}

void ChemicalStructureRenderer::updateRendererUniforms(const RendererUniforms &uniforms) {
    m_uniforms = uniforms;
}


quint32 ChemicalStructureRenderer::addMeshInstanceToMeshRenderer(MeshInstance *meshInstance, MeshRenderer *meshRenderer, RenderSelection * selectionHandler) {
    auto *mesh = meshInstance->mesh();
    if(!mesh) return 0;
    std::vector<MeshVertex> vertices;
    std::vector<MeshRenderer::IndexTuple> indices;
    auto verts = meshInstance->vertices();
    auto vertexNormals = meshInstance->vertexNormals();
    qDebug() << "Verts.size:" << verts.cols();
    Eigen::Vector3d centroid = verts.rowwise().mean();
    qDebug() << "Centroid:" << centroid(0) << centroid(1) << centroid(2);
    qDebug() << "normals.size:" << vertexNormals.cols();
    const auto &faces = mesh->faces();
    bool haveVertexNormals = mesh->haveVertexNormals();
    qDebug() << "Have vertex normals" << haveVertexNormals;

    quint32 selectionId{0};
    QVector3D selectionIdColor;
    if(selectionHandler) {
	selectionId = selectionHandler->add(SelectionType::Surface, m_meshIndexToMesh.size());
	selectionIdColor = selectionHandler->getColorFromId(selectionId);
	m_meshIndexToMesh.push_back(meshInstance);
    }

    Mesh::ScalarPropertyValues prop;
    QString propertyName = meshInstance->getSelectedProperty();
    auto availableProperties = mesh->availableVertexProperties();
    qDebug() << "Available vertex properties: " << availableProperties;
    Mesh::ScalarPropertyRange range;
    if(availableProperties.size() > 1) {
	prop = mesh->vertexProperty(propertyName);
	range = mesh->vertexPropertyRange(propertyName);
    }
    else {
	prop = Mesh::ScalarPropertyValues::Zero(verts.cols());
    }

    ColorMapFunc cmap(m_propertyColorMaps.value(propertyName, ColorMapName::Viridis), range.lower, range.upper);

    for (int i = 0; i < mesh->numberOfVertices(); i++) {
	quint32 vertex_id = selectionId + i;
	QVector3D selectionColor;
	if(selectionHandler) {
	    selectionColor = selectionHandler->getColorFromId(vertex_id);
	}
	QVector3D position(verts(0, i), verts(1, i), verts(2, i));

	QColor color = cmap(prop(i));
	QVector3D colorVec(color.redF(), color.greenF(), color.blueF());

	QVector3D normal(0.0f, 0.0f, 0.0f);
	if(haveVertexNormals) {
	    normal = QVector3D(
		vertexNormals(0, i),
		vertexNormals(1, i),
		vertexNormals(2, i)
	    );
	}
	vertices.emplace_back(position, normal, colorVec, selectionColor);
    }
    for (int f = 0; f < mesh->numberOfFaces(); f++) {
	indices.emplace_back(
	    MeshRenderer::IndexTuple{
		static_cast<GLuint>(faces(0, f)),
		static_cast<GLuint>(faces(1, f)),
		static_cast<GLuint>(faces(2, f))
	    }
	);
    }
    meshRenderer->addMesh(vertices, indices);
    qDebug() << "Added mesh" << selectionId;
    return selectionId;
}

void ChemicalStructureRenderer::handleMeshesUpdate() {
    if(!m_meshesNeedsUpdate) return;
    qDebug() << "HandleMeshes update called (needs update)";
    m_instanceRenderer->beginUpdates();
    m_instanceRenderer->clear();
    m_meshIndexToMesh.clear();
    m_meshRenderer->clear();
    m_pointCloudRenderer->clear();
    for(auto * child: m_structure->children()) {
	auto* mesh = qobject_cast<Mesh*>(child);
	if(!mesh) continue;
	m_instanceRenderer->setMesh(mesh);
	if(mesh->numberOfFaces() == 0) {
	    m_pointCloudRenderer->addPoints(
		    cx::graphics::makePointCloudVertices(*mesh)
		    );
	}
	else {
	    const auto &availableProperties = m_instanceRenderer->availableProperties();
	    for(auto * meshChild: child->children()) {
		auto* meshInstance = qobject_cast<MeshInstance*>(meshChild);
		if(!meshInstance || !meshInstance->isVisible()) continue;

		int propertyIndex = availableProperties.indexOf(meshInstance->getSelectedProperty());
		float alpha = meshInstance->isTransparent() ? 0.9 : 1.0;
		qDebug() << "Property index:" << propertyIndex;
		qDebug() << "alpha:" << alpha;

		MeshInstanceVertex v(
		    meshInstance->translationVector(), meshInstance->rotationMatrix(),
		    QVector3D(), propertyIndex, alpha
		);
		m_instanceRenderer->addInstance(v);
	    }
	}
    }
    m_instanceRenderer->endUpdates();
    m_meshesNeedsUpdate = false;
}


void ChemicalStructureRenderer::childVisibilityChanged() {
    // TODO more granularity
    qDebug() << "Child visibility changed";
    updateMeshes();
}

void ChemicalStructureRenderer::childPropertyChanged() {
    // TODO more granularity
    qDebug() << "Child property changed";
    updateMeshes();
}


void ChemicalStructureRenderer::childAddedToStructure(QObject *child) {
    auto* mesh = qobject_cast<Mesh*>(child);
    if (mesh) {
	qDebug() << "Added mesh to structure, connected";
	connect(mesh, &Mesh::visibilityChanged, this, &ChemicalStructureRenderer::childVisibilityChanged);
	connect(mesh, &Mesh::selectedPropertyChanged, this, &ChemicalStructureRenderer::childPropertyChanged);
	connect(mesh, &Mesh::transparencyChanged, this, &ChemicalStructureRenderer::childPropertyChanged);
	m_meshesNeedsUpdate = true;
    }
}

void ChemicalStructureRenderer::childRemovedFromStructure(QObject *child) {
    qDebug() << "Child removed @" << child << "TODO, for now bulk reset";
    auto* mesh = qobject_cast<Mesh*>(child);
    if (mesh) {
	qDebug() << "Child removed (mesh) from structure, disconnected";
	disconnect(mesh, &Mesh::visibilityChanged, this, &ChemicalStructureRenderer::childVisibilityChanged);
	m_meshesNeedsUpdate = true;
    }
}


MeshInstance * ChemicalStructureRenderer::getMesh(size_t index) const {
    if(index >= m_meshIndexToMesh.size()) return nullptr;
    return m_meshIndexToMesh.at(index);
}

}
