#include "SecureConnectionManager.h"


int main() {
    SecureConnectionManager* mgr = new SecureConnectionManager(55555);
    mgr->serverRunner();
    return 1;
}