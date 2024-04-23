#include <QDialog>
#include <QtDebug>

#include "wavefunctioncalculationdialog.h"

const QString WavefunctionCalculationDialog::customEntry{"Custom..."};

WavefunctionCalculationDialog::WavefunctionCalculationDialog(QWidget *parent)
    : QDialog(parent) {
	setupUi(this);
	init();
    }

void WavefunctionCalculationDialog::init() {
    setWindowTitle("Wavefunction Calculation");
    setModal(true);

    // put available options in the dialog
    initPrograms();
    initMethod();
    initBasis();
    adjustSize();
}

void WavefunctionCalculationDialog::initPrograms() {
    programComboBox->clear();

    QStringList programs{
	"OCC", "Gaussian", "ORCA"
    };

    QString preferred = "OCC";
    for (const auto &source:programs) {
	programComboBox->addItem(source);
	if (source == preferred) {
	    programComboBox->setCurrentText(source);
	}
    }
}

void WavefunctionCalculationDialog::initMethod() {
    QStringList methods{
	"HF", "B3LYP", "WB97M-V"
    };


    for (const auto &method : methods) {
	methodComboBox->addItem(method);
    }
    methodComboBox->addItem(customEntry);

    connect(methodComboBox, QOverload<int>::of(&QComboBox::activated),
	    [&](int index){
	if (methodComboBox->itemText(index) == customEntry) {
	    methodComboBox->setEditable(true);
	    methodComboBox->clearEditText();
	    methodComboBox->setFocus();
	    methodComboBox->showPopup();
	    methodComboBox->setToolTip(tr("Type here to enter a custom value"));
	} else {
	    methodComboBox->setEditable(false);
	}
    });

}

void WavefunctionCalculationDialog::initBasis() {
    QStringList basisSets{
	"def2-svp",
	"def2-tzvp",
	"6-31G(d,p)",
	"DGDZVP",
	"3-21G",
	"STO-3G",
    };

    for (const auto &basis : basisSets) {
	basisComboBox->addItem(basis);
    }

    basisComboBox->addItem(customEntry);

    connect(basisComboBox, QOverload<int>::of(&QComboBox::activated),
	    [&](int index){
	if (basisComboBox->itemText(index) == customEntry) {
	    basisComboBox->setEditable(true);
	    basisComboBox->clearEditText();
	    basisComboBox->setFocus();
	    basisComboBox->showPopup();
	    basisComboBox->setToolTip(tr("Type here to enter a custom value"));
	} else {
	    basisComboBox->setEditable(false);
	}
    });

}


void WavefunctionCalculationDialog::show() {
  initPrograms(); // wavefunction program availability might have changed so
                  // reinitialise
  QWidget::show();
}


const wfn::Parameters& WavefunctionCalculationDialog::getParameters() const {
    return m_parameters;
}

void WavefunctionCalculationDialog::accept() {
    m_parameters.charge = charge();
    m_parameters.multiplicity = multiplicity();
    m_parameters.method = method();
    m_parameters.basis = basis();

    emit wavefunctionParametersChosen(m_parameters);

    QDialog::accept();
}

QString WavefunctionCalculationDialog::program() const {
  return programComboBox->currentText();
}

QString WavefunctionCalculationDialog::method() const {
  return methodComboBox->currentText();
}

QString WavefunctionCalculationDialog::basis() const {
  return basisComboBox->currentText();
}

void WavefunctionCalculationDialog::setAtomIndices(const std::vector<GenericAtomIndex> &idxs) {
    m_parameters.atoms = idxs;
}

const std::vector<GenericAtomIndex>& WavefunctionCalculationDialog::atomIndices() const {
    return m_parameters.atoms;
}

int WavefunctionCalculationDialog::charge() const { 
    return chargeSpinBox->value();
}

void WavefunctionCalculationDialog::setCharge(int charge) { 
    chargeSpinBox->setValue(charge);
    m_parameters.charge = charge;
}

int WavefunctionCalculationDialog::multiplicity() const { 
    return multiplicitySpinBox->value();
}

void WavefunctionCalculationDialog::setMultiplicity(int mult) { 
    multiplicitySpinBox->setValue(mult);
    m_parameters.multiplicity = mult;
}
