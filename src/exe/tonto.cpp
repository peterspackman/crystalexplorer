#include "tonto.h"
#include "exefileutilities.h"

TontoTask::TontoTask(QObject *parent) : ExternalProgramTask(parent) {
    setExecutable("/Users/285699f/git/crystalexplorer/resources/tonto");
}

void TontoTask::appendBasisSetDirectoryBlock(QString &result) {
    // basis set information
    result.append("\n    basis_directory= \"" + basisSetDirectory() + "\"\n");
    QString slaterBasis = slaterBasisName();
    if (!slaterBasis.isEmpty()) {
	result.append("    slaterbasis_name= \"" + slaterBasis + "\"\n");
    }
}

void TontoTask::appendHeaderBlock(const QString &header, QString &result) {
    result.append("{\n    ! " + header + "\n");
}

void TontoTask::appendFooterBlock(QString &result) {
    result.append("\n}\n");
}


void TontoTask::appendCifDataBlock(const QString &dataBlockName, QString &result) {
    result.append("\n    ! Read the CIF and data block ...\n\n");
    result.append("    CIF= {\n        file_name= \"" + cifFileName() + "\"\n");
    if (!dataBlockName.isEmpty()) {
	result.append("        data_block_name= \"" + dataBlockName + "\"\n");
    }

  if (overrideBondLengths()) {
      // TODO fix settings
      result.append(QString("        CH_bond_length= %1 angstrom\n").arg(1.0));
      result.append(QString("        NH_bond_length= %1 angstrom\n").arg(1.0));
      result.append(QString("        OH_bond_length= %1 angstrom\n").arg(1.0));
      result.append(QString("        BH_bond_length= %1 angstrom\n").arg(1.0));
  }
  result.append("    }\n");
}

bool TontoTask::overrideBondLengths() const {
    return properties().value("override_bond_lengths", true).toBool();
}

QString TontoTask::cifFileName() const {
    return properties().value("file_name", "file.cif").toString();
}

QString TontoTask::basisSetDirectory() const {
    return properties().value("basis_directory", ".").toString();
}

QString TontoTask::slaterBasisName() const {
    return properties().value("slaterbasis_name", "").toString();
}

void TontoTask::start() {
    QString name = baseName();

    if(!exe::writeTextFile("stdin", getInputText())) {
	emit errorOccurred("Could not write input file");
	return;
    }
    emit progressText("Wrote Tonto stdin file");

    FileDependencyList fin = requirements();
    fin.append(FileDependency("stdin"));

    FileDependencyList fout = outputs();
    fout.append(FileDependency("stdout"));

    setRequirements(fin);
    setOutputs(fout);

    emit progressText("Starting Tonto process");

    ExternalProgramTask::start();

    qDebug() << "Finish Tonto task start";
}

// CIF Processing

QString TontoCifProcessingTask::getInputText() {
    QString result;
    result.reserve(2048);
    appendHeaderBlock("Tonto input file for CIF Processing.", result);
    appendBasisSetDirectoryBlock(result);
    appendCifDataBlock("", result);
    result.append("    cx_uses_angstrom= true\n");
    result.append("    CX_file_name= \"file.cxs\"\n");
    result.append("    process_CIF_for_CX\n");
    appendFooterBlock(result);
    return result;
}

TontoCifProcessingTask::TontoCifProcessingTask(QObject *parent) : TontoTask(parent) {}

void TontoCifProcessingTask::start() {
    setRequirements({FileDependency(cifFileName())});
    setOutputs({FileDependency("file.cxs")});
    TontoTask::start();
    /*
  writeCifData(ts, jobParams.inputFilename, jobParams.overrideBondLengths);
  writeProcessCifForCX(ts, jobParams.outputFilename);
  writeFooter(ts);
  */
}
