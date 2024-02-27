#include "externalprogram.h"


namespace exe {
class Occ : exe::ExternalProgram {

public:
    Occ(const QString &location);
    QPromise<wfn::Result> wavefunction(const wfn::Parameters&) override;;
    QPromise<surface::Result> surface(const surface::Parameters&) override;
    QPromise<interaction::Result> interaction(const interaction::Parameters&) override;
private:

};

}
