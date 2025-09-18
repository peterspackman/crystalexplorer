#include "selection_information.h"
#include "elementdata.h"
#include <fmt/core.h>

namespace impl {

struct SelectionInfoVisitor {

  QString operator()(const SelectedAtom &atom) const {
    auto *el = ElementData::elementFromAtomicNumber(atom.atomicNumber);
    const auto &atomPosition = atom.position;
    return QString::fromStdString(fmt::format(
        "<b>Atom label</b>:           {}<br/>"
        "<b>Unique fragment</b>:      {}<br/>"
        "<b>Position</b>:             {:9.3f} {:9.3f} {:9.3f}<br/>"
        "<b>Element</b>:              {}<br/>"
        "<b>Covalent radius</b>:      {:9.3f}<br/>"
        "<b>Van der Waals radius</b>: {:9.3f}",
        atom.label.toStdString(), atom.fragmentLabel.toStdString(),
        atomPosition.x(), atomPosition.y(), atomPosition.z(),
        el->symbol().toStdString(), el->covRadius(), el->vdwRadius()));
  }

  QString operator()(const SelectedBond &bond) const {
    QVector3D vec = bond.a.position - bond.b.position;
    float length = vec.length();
    return QString::fromStdString(fmt::format(
        "<b>Bond distance</b>:   {:9.3f}<br/>"
        "<b>Atom label A</b>:    {}<br/>"
        "<b>Atom label B</b>:    {}<br/>"
        "<b>Unique fragment</b>: {}<br/>",
        length, bond.a.label.toStdString(), bond.b.label.toStdString(),
        bond.fragmentLabel.toStdString()));
  }

  QString operator()(const SelectedSurface &selection) const {
    // Assumes the surface has been checked for nullptr
    QVector3D centroid(0.0f, 0.0f, 0.0f);
    auto *mesh = selection.surface->mesh();
    QString surfaceName = mesh->objectName();
    QString surfaceInstance = selection.surface->objectName();
    QString property = selection.property;
    double value = selection.propertyValue;
    double area = mesh->surfaceArea();
    double volume = mesh->volume();
    return QString::fromStdString(
        fmt::format("<b>Surface</b>: {}<br/>"
                    "<b>Instance</b>: {}<br/>"
                    "<b>Centroid</b>:     {:9.3f} {:9.3f} {:9.3f}<br/>"
                    "<b>Volume</b>:       {:9.3f}<br/>"
                    "<b>Surface area</b>: {:9.3f}<br/>"
                    "<b>Property</b>: {}<br/>"
                    "<b>Property Value</b>: {:9.3f}",
                    surfaceName.toStdString(), surfaceInstance.toStdString(),
                    centroid.x(), centroid.y(), centroid.z(), volume, area,
                    property.toStdString(), value));
  }

  QString operator()(const std::monostate &) const { return QString(); }
};

} // namespace impl

QString
getSelectionInformationLabelText(const SelectionInfoVariant &selection) {
  return std::visit(impl::SelectionInfoVisitor(), selection);
}
