#include "occ_external.h"

namespace exe {

Occ::Occ(const QString &location) : ExternalProgram(location) {}

QPromise<wfn::Result> Occ::wavefunction(const wfn::Parameters &params) {

    auto func = [&](pa

            QFuture<void> future = QtConcurrent::run(someFunction);

}

QPromise<surface::Result> Occ::surface(const surface::Parameters &params) {

}

QPromise<interaction::Result> Occ::interaction(const interaction::Parameters &params) {

}

}
