#include "wavefunction_parameters.h"


namespace wfn {

QString fileFormatString(FileFormat fmt) {
    switch(fmt) {
	case FileFormat::OccWavefunction: return "OWF JSON";
	case FileFormat::Fchk: return "FCHK";
	case FileFormat::Molden: return "Molden";
    }
}

}
